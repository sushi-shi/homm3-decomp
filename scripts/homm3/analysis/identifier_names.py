"""Audit project-owned C++ type and function names with libclang.

The report is deliberately declaration-based: a spelling that merely appears in
a comment, an SDK header, vendored code, or a standard-library instantiation is
not a project identifier.  Rows are TSV so a reviewed subset can become input to
a later rewrite rather than making an unreviewed tree-wide edit.

T-prefixed types and snake-case functions are reported as direct reviews.
Applying a type plan preserves the recovered spelling in the owning source
comment; the normal build/delink pass then regenerates labels for the authored
name. C-linkage functions remain external ABI spellings and are excluded.

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
            "type", qualified, name, name[1:], "direct-review", 9,
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


def rewrite_code_identifiers(contents: str, replacements: dict[str, str],
                             contextual: set[str] | None = None) -> str:
    """Rewrite identifier tokens while leaving comments and literals intact.

    T-prefixed names are sufficiently distinctive to replace as identifiers.
    Explicit legacy lower-case class names use conservative type-position
    checks so a parameter such as ``int town`` remains untouched.
    """
    contextual = contextual or set()
    result = []
    index = 0
    length = len(contents)
    state = "code"
    while index < length:
        if state == "code" and contents.startswith("//", index):
            state = "line-comment"
        elif state == "code" and contents.startswith("/*", index):
            state = "block-comment"

        if state == "line-comment":
            end = contents.find("\n", index)
            if end < 0:
                result.append(contents[index:])
                break
            result.append(contents[index:end + 1])
            index = end + 1
            state = "code"
            continue
        if state == "block-comment":
            end = contents.find("*/", index + 2)
            if end < 0:
                result.append(contents[index:])
                break
            result.append(contents[index:end + 2])
            index = end + 2
            state = "code"
            continue
        if contents[index] in {'"', "'"}:
            quote = contents[index]
            end = index + 1
            closed = False
            while end < length:
                if quote == "'" and contents[end] == "\n":
                    break
                if contents[end] == "\\":
                    end += 2
                    continue
                end += 1
                if contents[end - 1] == quote:
                    closed = True
                    break
            # CodeView's spelling for compiler-generated functions contains
            # an unmatched apostrophe: `scalar deleting destructor'. It is
            # punctuation, not a C++ character literal.
            if quote == "'" and not closed:
                result.append(contents[index])
                index += 1
                continue
            result.append(contents[index:end])
            index = end
            continue
        if contents[index].isalpha() or contents[index] == "_":
            end = index + 1
            while (end < length
                   and (contents[end].isalnum() or contents[end] == "_")):
                end += 1
            token = contents[index:end]
            replacement = replacements.get(token)
            if replacement is not None and (token not in contextual
                                               or is_type_position(
                                                   contents, index, end, token)):
                result.append(replacement)
            else:
                result.append(token)
            index = end
            continue
        result.append(contents[index])
        index += 1
    return "".join(result)


def refresh_compatibility_macros(contents: str,
                                 replacements: dict[str, str]) -> str:
    """Retire obsolete clean-to-recovered preprocessor aliases."""
    for old, new in replacements.items():
        contents = re.sub(
            rf"^[ \t]*#ifndef[ \t]+{re.escape(new)}[ \t]*\n"
            rf"[ \t]*#define[ \t]+{re.escape(new)}[ \t]+"
            rf"(?:{re.escape(old)}|{re.escape(new)})[ \t]*\n"
            rf"[ \t]*#endif[ \t]*\n",
            "", contents, flags=re.MULTILINE)
    return contents


def is_type_position(contents: str, start: int, end: int, name: str) -> bool:
    """Recognize conservative contexts for an explicitly named legacy type."""
    line_start = contents.rfind("\n", 0, start) + 1
    line_end = contents.find("\n", end)
    if line_end < 0:
        line_end = len(contents)
    left = contents[line_start:start]
    right = contents[end:line_end]
    if re.search(r"\b(?:class|struct|union|enum|new)\s*$", left):
        return True
    if re.match(r"\s*\*\s*\(", right):
        return False
    if re.match(r"\s*\*\s*[0-9]", right):
        return False
    if re.match(r"\s*(?:\*|&|::)", right):
        return True
    open_angle = left.rfind("<")
    close_angle = left.rfind(">")
    if (open_angle > close_angle and re.search(r"(?:<|,\s*)$", left)
            and re.match(r"\s*(?:,|>)", right)):
        return True
    if re.search(r"(?:~|::)\s*$", left) and re.match(r"\s*\(", right):
        return True
    if not left.strip() and re.match(r"\s*\(", right):
        return True
    if not left.strip() and re.match(r"\s+[A-Za-z_]\w*\s*(?:[;=(\[])", right):
        return True
    if (re.search(r"\b(?:extern|static|const|volatile|mutable)\s*$", left)
            and re.match(r"\s+[A-Za-z_]\w*\s*(?:[;=(\[])", right)):
        return True
    if (re.search(r"\b(?:VA_COMPGEN|SIZE)\s*\(", left)
            and re.match(r"\s*[,)]", right)):
        return True
    return False


def parse_type_rename(value: str) -> tuple[str, str]:
    old, separator, new = value.partition("=")
    if (not separator or not re.fullmatch(r"[A-Za-z_]\w*", old)
            or not re.fullmatch(r"[A-Za-z_]\w*", new)):
        raise argparse.ArgumentTypeError("type rename must be OLD=NEW")
    return old, new


def apply_type_plan(root: Path, plan: Path, min_confidence: int,
                    explicit: list[tuple[str, str]]) -> int:
    """Apply reviewed types while preserving recovered VC6 type identities."""
    source_files = sorted((root / "include").glob("**/*.h"))
    source_files += sorted((root / "src").glob("**/*.cpp"))
    initial = {path: path.read_text() for path in source_files}
    existing_aliases = {
        (match.group(1), match.group(2))
        for contents in initial.values()
        for match in re.finditer(
            r"\btypedef\s+([A-Za-z_]\w*)\s+([A-Za-z_]\w*)\s*;", contents)
    }
    replacements: dict[str, str] = {}
    owners: list[tuple[Path, int, str, str]] = []
    with plan.open(newline="") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            if (row["kind"] != "type"
                    or row["strategy"] not in {"alias-review", "direct-review"}
                    or int(row["confidence"]) < min_confidence):
                continue
            old, new = row["current"], row["suggested"]
            if (old, new) in existing_aliases:
                continue
            if old in replacements and replacements[old] != new:
                sys.exit(f"conflicting type plan for {old}")
            replacements[old] = new
            owners.append((root / row["file"], int(row["line"]),
                           row["qualified_name"], old))

    contextual = set()
    for old, new in explicit:
        # A reviewed explicit spelling may resolve a macro or semantic
        # collision discovered after the mechanical suggestion was emitted.
        replacements[old] = new
        if not TYPE_PREFIX.fullmatch(old):
            contextual.add(old)

    rendered = dict(initial)

    # Put lineage at the declaration selected by libclang. Line hints may have
    # shifted since report generation, so select the nearest declaration line.
    owner_edits: dict[Path, list[tuple[int, str]]] = {}
    for path, hint, qualified, old in owners:
        lines = rendered[path].splitlines(keepends=True)
        comment = f"// Before normalization (type): {qualified}.\n"
        # A prior repair may move lineage from a forward declaration to the
        # concrete definition in its owning header. Treat that as satisfied
        # when an updated report still points at the old declaration site.
        if any(comment in contents for contents in rendered.values()):
            continue
        pattern = re.compile(
            rf"\b(?:class|struct|union|enum)\s+{re.escape(old)}\b")
        candidates = [number for number, line in enumerate(lines)
                      if pattern.search(line)]
        if not candidates:
            sys.exit(f"cannot find owning declaration for {qualified} in {path}")
        index = min(candidates, key=lambda number: abs(number + 1 - hint))
        nearby = "".join(lines[max(0, index - 4):index])
        if comment.strip() not in nearby:
            owner_edits.setdefault(path, []).append((index, comment))

    # Explicit lower-case classes are not in the T-prefix report. Comment the
    # concrete definition, not every forward declaration.
    for old, _ in explicit:
        comment = f"// Before normalization (type): {old}.\n"
        if any(comment in contents for contents in rendered.values()):
            continue
        pattern = re.compile(
            rf"\b(?:class|struct|union|enum)\s+{re.escape(old)}\b[^;]*\{{")
        matches = []
        for path, contents in rendered.items():
            for match in pattern.finditer(contents):
                matches.append((path, contents.count("\n", 0, match.start())))
        if len(matches) != 1:
            sys.exit(f"expected one definition for explicit type {old}, found {len(matches)}")
        path, index = matches[0]
        lines = rendered[path].splitlines(keepends=True)
        nearby = "".join(lines[max(0, index - 4):index])
        if comment.strip() not in nearby:
            owner_edits.setdefault(path, []).append((index, comment))

    for path, edits in owner_edits.items():
        lines = rendered[path].splitlines(keepends=True)
        for index, comment in sorted(edits, reverse=True):
            lines.insert(index, comment)
        rendered[path] = "".join(lines)

    changed = 0
    for path, contents in rendered.items():
        updated = rewrite_code_identifiers(contents, replacements, contextual)
        # Retire an earlier alias when its recovered tag is now directly named.
        for new in replacements.values():
            updated = re.sub(
                rf"^[ \t]*typedef[ \t]+{re.escape(new)}[ \t]+{re.escape(new)}[ \t]*;[ \t]*\n",
                "", updated, flags=re.MULTILINE)
        # Compiler/debug identities are normalized by the evidence layer;
        # authored C++ no longer needs preprocessor aliases for legacy tags.
        updated = refresh_compatibility_macros(updated, replacements)
        if updated != path.read_text():
            path.write_text(updated)
            changed += 1

    # These columns describe authored identities. Keep provenance/reason text
    # and the raw retail inventories untouched.
    win_only = root / "config/win_only.tsv"
    lines = win_only.read_text().splitlines(keepends=True)
    updated_lines = []
    for line in lines:
        fields = line.rstrip("\n").split("\t")
        if len(fields) >= 3 and not line.startswith("#") and fields[0] != "file":
            fields[1] = rewrite_code_identifiers(fields[1], replacements, contextual)
            fields[2] = rewrite_code_identifiers(fields[2], replacements, contextual)
            line = "\t".join(fields) + ("\n" if line.endswith("\n") else "")
        updated_lines.append(line)
    updated = "".join(updated_lines)
    if updated != win_only.read_text():
        win_only.write_text(updated)
        changed += 1

    vtables = root / "config/retail-vtables.tsv"
    lines = vtables.read_text().splitlines(keepends=True)
    updated_lines = []
    for line in lines:
        fields = line.rstrip("\n").split("\t")
        if len(fields) == 3 and fields[2] in replacements:
            fields[2] = replacements[fields[2]]
            line = "\t".join(fields) + ("\n" if line.endswith("\n") else "")
        updated_lines.append(line)
    updated = "".join(updated_lines)
    if updated != vtables.read_text():
        vtables.write_text(updated)
        changed += 1
    print(f"applied {len(replacements)} type rename(s) across {changed} file(s)",
          file=sys.stderr)
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
    parser.add_argument("--apply-types", type=Path, metavar="TSV",
                        help="apply non-colliding reviewed type rows from TSV")
    parser.add_argument("--type-rename", action="append", default=[],
                        type=parse_type_rename, metavar="OLD=NEW",
                        help="also rename an explicit legacy class (repeatable)")
    parser.add_argument("--min-confidence", type=int, default=8,
                        help="minimum confidence for --apply-carcass (default: 8)")
    args = parser.parse_args()
    root = common.HOMM3_DIR
    if args.apply_carcass is not None:
        if (args.all or args.module or args.limit is not None
                or args.apply_types is not None or args.type_rename):
            parser.error("--apply-carcass cannot be combined with audit selection")
        if not 1 <= args.min_confidence <= 10:
            parser.error("--min-confidence must be between 1 and 10")
        return apply_carcass_plan(root, args.apply_carcass, args.min_confidence)
    if args.apply_types is not None:
        if args.all or args.module or args.limit is not None:
            parser.error("--apply-types cannot be combined with audit selection")
        if not 1 <= args.min_confidence <= 10:
            parser.error("--min-confidence must be between 1 and 10")
        return apply_type_plan(root, args.apply_types, args.min_confidence,
                               args.type_rename)
    if args.type_rename:
        parser.error("--type-rename requires --apply-types")
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
