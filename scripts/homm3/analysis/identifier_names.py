"""Audit project-owned C++ type and function names with libclang.

The report is deliberately declaration-based: a spelling that merely appears in
a comment, an SDK header, vendored code, or a standard-library instantiation is
not a project identifier.  Rows are TSV so a reviewed subset can become input to
a later rewrite rather than making an unreviewed tree-wide edit.

T-prefixed types are reported with ``alias-review`` because changing the actual
record/enum tag changes VC6 decorated names.  A clean typedef may be used by
authored code while the recovered tag remains the ABI identity.  Snake-case
functions are reported as direct renames, except C-linkage declarations.

Run inside the build shell after generating the Clang header mirror::

    PYTHONPATH=scripts python -m homm3.analysis.identifier_names --all \
        > build/identifier-names.tsv
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, replace
from pathlib import Path

from homm3 import manifest
from homm3.analysis.access_facts import is_project_file, load_cindex
from homm3.build import compilation_database
from homm3.core import clang, common
from homm3.core.cc_wrap import ZLIB_INC


TYPE_PREFIX = re.compile(r"^T[A-Z][A-Za-z0-9_]*$")
SNAKE_FUNCTION = re.compile(r"^[a-z][A-Za-z0-9]*_[A-Za-z0-9_]+$")
CARCASS_FUNCTION = re.compile(
    r"^[ \t]*(?:[A-Za-z_][A-Za-z0-9_:<>,*& \t]*[ \t]+)"
    r"(?P<qualified>[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*)"
    r"[ \t]*\(",
)


@dataclass(frozen=True)
class Finding:
    kind: str
    qualified_name: str
    current: str
    suggested: str
    strategy: str
    confidence: int
    file: str
    line: int
    usr: str


def lower_camel(spelling: str) -> str:
    """Convert an underscore-separated identifier without guessing semantics."""
    words = [word for word in spelling.split("_") if word]
    if not words:
        return spelling
    first = words[0].lower()
    return first + "".join(word[:1].upper() + word[1:].lower()
                           for word in words[1:])


def qualified_name(cursor) -> str:
    parts = [cursor.spelling]
    parent = cursor.semantic_parent
    while parent is not None and parent.kind.name != "TRANSLATION_UNIT":
        if parent.spelling:
            parts.append(parent.spelling)
        parent = parent.semantic_parent
    return "::".join(reversed(parts))


def confidence_for_function(name: str) -> int:
    # Acronyms and synthetic numeric fragments need human semantic review.
    words = name.split("_")
    if any(any(ch.isupper() for ch in word) for word in words):
        return 7
    if any(any(ch.isdigit() for ch in word) for word in words):
        return 6
    return 9


def make_finding(cursor, ci, root: Path) -> Finding | None:
    location = cursor.location
    if location.file is None or not cursor.spelling:
        return None
    path = Path(location.file.name)
    try:
        relative = str(path.resolve().relative_to(root.resolve()))
    except (OSError, ValueError):
        return None

    record_kinds = {
        ci.CursorKind.CLASS_DECL,
        ci.CursorKind.STRUCT_DECL,
        ci.CursorKind.UNION_DECL,
        ci.CursorKind.ENUM_DECL,
        ci.CursorKind.CLASS_TEMPLATE,
    }
    function_kinds = {
        ci.CursorKind.FUNCTION_DECL,
        ci.CursorKind.CXX_METHOD,
        ci.CursorKind.FUNCTION_TEMPLATE,
    }
    name = cursor.spelling
    qualified = qualified_name(cursor)
    if qualified.startswith("std::"):
        return None

    if cursor.kind in record_kinds and TYPE_PREFIX.fullmatch(name):
        return Finding(
            "type", qualified, name, name[1:], "alias-review", 9,
            relative, location.line, cursor.get_usr(),
        )

    if cursor.kind in function_kinds and SNAKE_FUNCTION.fullmatch(name):
        # C linkage is an external ABI spelling, even when its declaration is
        # maintained in a project header.
        if cursor.linkage == ci.LinkageKind.EXTERNAL and cursor.type.spelling.startswith("extern \"C\""):
            return None
        suggested = lower_camel(name)
        if suggested == name:
            return None
        return Finding(
            "function", qualified, name, suggested, "direct-review",
            confidence_for_function(name), relative, location.line,
            cursor.get_usr(),
        )
    return None


def commands_for_modules(root: Path, modules: list[str]):
    mirror = clang.mirror()
    if mirror is None:
        sys.exit("VC6 header mirror unavailable; run a build first")
    rows = compilation_database.commands(
        manifest.load(root / "config/units.toml"), root,
        clang.clang_bin() or "clang", [mirror, root / "include", root / ZLIB_INC],
    )
    wanted = {Path(module).stem.lower() for module in modules}
    if wanted:
        rows = [row for row in rows if Path(row["file"]).stem.lower() in wanted]
        missing = wanted - {Path(row["file"]).stem.lower() for row in rows}
        if missing:
            sys.exit("unknown module(s): " + ", ".join(sorted(missing)))
    return rows, mirror


def parse_translation_unit(ci, row, root: Path, mirror: Path):
    source = Path(row["file"])
    args = [arg for arg in row["arguments"][1:]
            if arg != "/c" and Path(arg).resolve() != source.resolve()]
    args = ["--driver-mode=cl", "-fsyntax-only", "-ferror-limit=0"] + args
    try:
        tu = ci.Index.create().parse(
            str(source), args=args,
            options=ci.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES,
        )
    except ci.TranslationUnitLoadError as exc:
        return [], set(), str(exc)
    found = []
    declarations = set()
    pending = list(tu.cursor.get_children())
    while pending:
        cursor = pending.pop()
        location = cursor.location.file
        if location is None or not is_project_file(Path(location.name), root, mirror):
            continue
        pending.extend(cursor.get_children())
        if cursor.kind in {
                ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL,
                ci.CursorKind.UNION_DECL, ci.CursorKind.ENUM_DECL,
                ci.CursorKind.CLASS_TEMPLATE, ci.CursorKind.FUNCTION_DECL,
                ci.CursorKind.CXX_METHOD, ci.CursorKind.FUNCTION_TEMPLATE}:
            declarations.add(qualified_name(cursor))
        finding = make_finding(cursor, ci, root)
        if finding is not None and finding.usr:
            found.append(finding)
    return found, declarations, None


def collect(ci, rows, root: Path, mirror: Path, limit: int | None, jobs: int):
    findings: dict[str, Finding] = {}
    declarations = set()
    failures = 0
    selected = rows[:limit]
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        results = pool.map(lambda row: parse_translation_unit(ci, row, root, mirror),
                           selected)
        for number, (row, result) in enumerate(zip(selected, results), 1):
            found, parsed_declarations, error = result
            source = Path(row["file"])
            print(f"# [{number}/{len(selected)}] {source.name}", file=sys.stderr)
            if error is not None:
                failures += 1
                print(f"# parse failed: {source}: {error}", file=sys.stderr)
                continue
            declarations.update(parsed_declarations)
            for finding in found:
                previous = findings.get(finding.usr)
                if previous is None or (finding.file, finding.line) < (previous.file, previous.line):
                    findings[finding.usr] = finding
    if limit is None:
        source_paths = {Path(row["file"]).resolve() for row in rows}
        for finding in collect_carcass_functions(root, source_paths):
            key = f"lexical:{finding.file}:{finding.line}:{finding.qualified_name}"
            findings.setdefault(key, finding)
    for key, finding in list(findings.items()):
        owner, separator, _ = finding.qualified_name.rpartition("::")
        proposed = owner + separator + finding.suggested
        if proposed in declarations and proposed != finding.qualified_name:
            findings[key] = replace(finding, strategy="collision-review",
                                    confidence=min(finding.confidence, 3))
    return sorted(findings.values(), key=lambda row: (row.kind, row.qualified_name,
                                                       row.file, row.line)), failures


def collect_carcass_functions(root: Path,
                               source_paths: set[Path] | None = None) -> list[Finding]:
    """Audit declarations hidden from Clang by explicit carcass blocks."""
    findings = []
    for path in sorted((root / "src").glob("*.cpp")):
        if source_paths is not None and path.resolve() not in source_paths:
            continue
        depth = 0
        for line_number, line in enumerate(path.read_text().splitlines(), 1):
            if re.match(r"^[ \t]*#if[ \t]+0[ \t]+//[ \t]*@carcass", line):
                depth += 1
                continue
            if depth and re.match(r"^[ \t]*#if\b", line):
                depth += 1
                continue
            if depth and re.match(r"^[ \t]*#endif\b", line):
                depth -= 1
                continue
            if not depth or line.lstrip().startswith("//"):
                continue
            match = CARCASS_FUNCTION.match(line)
            if not match:
                continue
            qualified = match.group("qualified")
            if qualified.startswith("std::"):
                continue
            name = qualified.rsplit("::", 1)[-1]
            owner = qualified.rsplit("::", 1)[0].rsplit("::", 1)[-1] \
                if "::" in qualified else ""
            if owner == name:
                continue
            if name == "scalar_deleting_destructor":
                continue
            if not SNAKE_FUNCTION.fullmatch(name):
                continue
            findings.append(Finding(
                "function", qualified, name, lower_camel(name),
                "lexical-review", max(5, confidence_for_function(name) - 1),
                str(path.relative_to(root)), line_number, "",
            ))
    return findings


def write_tsv(findings: list[Finding]) -> None:
    writer = csv.writer(sys.stdout, delimiter="\t", lineterminator="\n")
    writer.writerow(Finding.__dataclass_fields__.keys())
    for finding in findings:
        writer.writerow((finding.kind, finding.qualified_name, finding.current,
                         finding.suggested, finding.strategy, finding.confidence,
                         finding.file, finding.line, finding.usr))


def apply_carcass_plan(root: Path, plan: Path, min_confidence: int) -> int:
    """Apply verified non-colliding inactive declarations from an audit TSV."""
    edits: dict[Path, list[dict[str, str]]] = {}
    with plan.open(newline="") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            if (row["kind"] != "function"
                    or row["strategy"] != "lexical-review"
                    or int(row["confidence"]) < min_confidence):
                continue
            path = (root / row["file"]).resolve()
            if not path.is_relative_to((root / "src").resolve()):
                sys.exit(f"refusing non-src carcass edit: {path}")
            edits.setdefault(path, []).append(row)

    rendered: dict[Path, str] = {}
    for path, rows in edits.items():
        lines = path.read_text().splitlines(keepends=True)
        for row in sorted(rows, key=lambda item: int(item["line"]), reverse=True):
            index = int(row["line"]) - 1
            if not 0 <= index < len(lines):
                sys.exit(f"stale plan line: {path}:{index + 1}")
            old, new = row["current"], row["suggested"]
            pattern = re.compile(rf"\b{re.escape(old)}\b")
            if len(pattern.findall(lines[index])) != 1:
                sys.exit(f"stale/ambiguous plan token: {path}:{index + 1}: {old}")
            lines[index] = pattern.sub(new, lines[index], count=1)
            indent = lines[index][:len(lines[index]) - len(lines[index].lstrip())]
            comment = f"{indent}// Before normalization (function): {row['qualified_name']}.\n"
            if index == 0 or lines[index - 1] != comment:
                lines.insert(index, comment)
        rendered[path] = "".join(lines)

    for path, contents in rendered.items():
        path.write_text(contents)
    print(f"applied {sum(len(rows) for rows in edits.values())} carcass rename(s) "
          f"across {len(edits)} file(s)", file=sys.stderr)
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module", action="append", default=[],
                        help="TU stem to parse; repeatable")
    parser.add_argument("--all", action="store_true",
                        help="parse all configured TUs")
    parser.add_argument("--limit", type=int, help="cap TUs for diagnostics")
    parser.add_argument("--jobs", type=int, default=1,
                        help="parallel libclang parses (default: 1)")
    parser.add_argument("--apply-carcass", type=Path, metavar="TSV",
                        help="apply non-colliding lexical-review rows from TSV")
    parser.add_argument("--min-confidence", type=int, default=8,
                        help="minimum confidence for --apply-carcass (default: 8)")
    args = parser.parse_args()
    root = common.HOMM3_DIR
    if args.apply_carcass is not None:
        if args.all or args.module or args.limit is not None:
            parser.error("--apply-carcass cannot be combined with audit selection")
        if not 1 <= args.min_confidence <= 10:
            parser.error("--min-confidence must be between 1 and 10")
        return apply_carcass_plan(root, args.apply_carcass, args.min_confidence)
    if not args.all and not args.module:
        parser.error("choose --all or at least one --module")
    if args.limit is not None and args.limit <= 0:
        parser.error("--limit must be positive")
    if args.jobs <= 0:
        parser.error("--jobs must be positive")

    ci = load_cindex()
    rows, mirror = commands_for_modules(root, args.module)
    findings, failures = collect(ci, rows, root, mirror, args.limit, args.jobs)
    write_tsv(findings)
    print(f"# {len(findings)} unique finding(s), {failures} parse failure(s)",
          file=sys.stderr)
    return 2 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
