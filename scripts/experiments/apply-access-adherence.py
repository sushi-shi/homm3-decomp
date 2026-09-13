"""Align authored member access to Dreamcast-recorded visibility, in place.

This is a declaration-only proposal, never proof that a caller is legal.
Access changes can alter VC6 mangled names, so finish with a full build to
refresh the source-owned claims. Declarations stay in their original order.

    # dry run -- print the plan, touch nothing
    PYTHONPATH=scripts python scripts/experiments/apply-access-adherence.py
    # apply the edits to the headers
    PYTHONPATH=scripts python scripts/experiments/apply-access-adherence.py --apply

Only unambiguously correlated public -> private/protected tightenings are
proposed. A C2248 error is evidence of a missing source relationship: inspect
public wrappers, ordinary helper ownership, and positively supported friends.
Do not revert the recorded access merely to compile. Parse failures abort all
edits; ambiguous overloads are skipped and reported by the read-only verifier.
"""

import argparse
import sys
import re
from collections import defaultdict
from pathlib import Path

from homm3 import manifest
from homm3.analysis.dc_lines import load_symbols
from homm3.analysis.source_facts import name_key
from homm3.analysis.access_facts import load_cindex, is_project_file, dc_visibility, correlate
from homm3.build import compilation_database
from homm3.core import clang, common
from homm3.core.cc_wrap import ZLIB_INC
from homm3.core.nb11_types import Types
from homm3.retail_labels.source import mask_lexical_noise


def qualified_owner(cursor, ci) -> str:
    parts, node = [], cursor.semantic_parent
    kinds = (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL,
             ci.CursorKind.CLASS_TEMPLATE, ci.CursorKind.NAMESPACE, ci.CursorKind.UNION_DECL)
    while node is not None and node.kind in kinds:
        if node.spelling:
            parts.append(node.spelling)
        node = node.semantic_parent
    return "::".join(reversed(parts))


def collect(ci, commands, root, mirror, dc):
    """(file, class_usr) -> ordered member decls with access + target + extent."""
    ACCESS = {ci.AccessSpecifier.PUBLIC: "public",
              ci.AccessSpecifier.PROTECTED: "protected",
              ci.AccessSpecifier.PRIVATE: "private"}
    member_kinds = (ci.CursorKind.CXX_METHOD, ci.CursorKind.CONSTRUCTOR,
                    ci.CursorKind.DESTRUCTOR, ci.CursorKind.FIELD_DECL, ci.CursorKind.VAR_DECL)
    record_kinds = (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL, ci.CursorKind.UNION_DECL)
    index = ci.Index.create()
    classes: dict[tuple, dict] = {}
    for n, row in enumerate(commands, 1):
        src = Path(row["file"])
        print(f"# [{n}/{len(commands)}] {src.name}", file=sys.stderr)
        args = [a for a in row["arguments"][1:]
                if a != "/c" and Path(a).resolve() != src.resolve()]
        args = ["--driver-mode=cl", "-fsyntax-only", "-ferror-limit=0"] + args
        try:
            tu = index.parse(str(src), args=args,
                             options=ci.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
        except ci.TranslationUnitLoadError as exc:
            raise RuntimeError(f"{src}: parse failed: {exc}") from exc
        errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
        if errors:
            raise RuntimeError("\n".join(errors))
        for rec in tu.cursor.walk_preorder():
            if rec.kind not in record_kinds or not rec.is_definition():
                continue
            loc = rec.location.file
            if loc is None or not is_project_file(Path(loc.name), root, mirror):
                continue
            usr = rec.get_usr()
            fkey = (loc.name, usr)
            if fkey in classes:
                continue
            parent_name = qualified_owner(rec, ci)
            owner = f"{parent_name}::{rec.spelling}" if parent_name else rec.spelling
            ckey = name_key(owner)
            source_line = Path(loc.name).read_text().splitlines()[rec.extent.start.line - 1]
            class_indent = indent_of(source_line)
            members = []
            for m in rec.get_children():
                if m.kind not in member_kinds:
                    continue
                if m.location.file is None or m.location.file.name != loc.name:
                    continue
                cur = ACCESS.get(m.access_specifier)
                if cur is None:
                    continue
                mkey = name_key(m.spelling)
                is_method = m.kind in (ci.CursorKind.CXX_METHOD, ci.CursorKind.CONSTRUCTOR,
                                        ci.CursorKind.DESTRUCTOR)
                rec, gap = correlate({
                    "class_key": ckey, "member_key": mkey,
                    "kind": "method" if is_method else "member" if m.kind == ci.CursorKind.FIELD_DECL else "static_member",
                    "is_method": is_method,
                    "parameters": [a.type.get_canonical().spelling for a in m.get_arguments()] if is_method else None,
                    "const": m.is_const_method() if m.kind == ci.CursorKind.CXX_METHOD else False,
                }, dc)
                target = rec["access"] if rec else None
                fix = bool(target in ("private", "protected") and cur == "public")
                members.append({
                    "name": m.spelling, "line": m.extent.start.line,
                    "end": m.extent.end.line, "cur": cur,
                    "target": target if fix else cur, "fix": fix,
                })
            if any(x["fix"] for x in members):
                classes[fkey] = {"file": loc.name, "owner": owner, "indent": class_indent,
                                 "members": sorted(members, key=lambda x: x["line"])}
    return classes


def plan_edits(classes) -> dict[str, list[tuple[int, bool, str]]]:
    """file -> list of (index, is_close, text) inserts, `index` a 0-based
    position to insert BEFORE.  Ordered so a whole file can be applied in
    sequence: descending index (later inserts never shift earlier ones), and at
    an equal index opens are applied before closes so the close label lands on
    top -- i.e. when a run's `public:` restore meets the next run's open label at
    the same boundary, `public:` precedes the new label in the file and the next
    member sits under the intended label, not a stale `public:`."""
    inserts: dict[str, list[tuple[int, bool, str]]] = defaultdict(list)
    for info in classes.values():
        members = info["members"]
        i = 0
        while i < len(members):
            if not members[i]["fix"]:
                i += 1
                continue
            j = i
            tgt = members[i]["target"]
            # extend run over physically-adjacent fix members with same target
            while (j + 1 < len(members) and members[j + 1]["fix"]
                   and members[j + 1]["target"] == tgt):
                j += 1
            first, last = members[i], members[j]
            indent = info["indent"]
            inserts[info["file"]].append((first["line"] - 1, False, f"{indent}{tgt}:"))
            inserts[info["file"]].append((last["end"], True, f"{indent}public:"))
            i = j + 1
    return {f: sorted(items, key=lambda e: (-e[0], e[1])) for f, items in inserts.items()}


def indent_of(line: str) -> str:
    return line[: len(line) - len(line.lstrip())]


def remove_empty_access_labels(text: str) -> str:
    """Drop labels with no declaration before the next label or class end."""
    masked = mask_lexical_noise(text)
    empty = re.compile(r"^[ \t]*(?:public|private|protected):[ \t]*\n"
                       r"(?=\s*(?:(?:public|private|protected):|}))", re.M)
    for match in reversed(list(empty.finditer(masked))):
        original = text[match.start():match.end()]
        remainder = re.sub(r"^[ \t]*(?:public|private|protected):[ \t]*",
                           "", original, count=1)
        replacement = indent_of(original) + remainder if remainder.strip() else ""
        text = text[:match.start()] + replacement + text[match.end():]
    return text


def apply_file(path: str, inserts: list[tuple[int, bool, str]]):
    lines = Path(path).read_text().splitlines(keepends=True)
    for idx, _is_close, label in inserts:  # pre-ordered by plan_edits
        lines.insert(idx, label + "\n")
    Path(path).write_text(remove_empty_access_labels("".join(lines)))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--apply", action="store_true", help="write edits (default: dry run)")
    ap.add_argument("--module", action="append", default=[], help="limit TUs parsed")
    args = ap.parse_args()
    ci = load_cindex()
    root = common.HOMM3_DIR
    mirror = clang.mirror()
    if mirror is None:
        ap.error("VC6 header mirror unavailable; run a build first")
    types = Types.from_symbols(load_symbols())
    dc = dc_visibility(types)
    print(f"# DC member names: {len(dc)}", file=sys.stderr)

    commands = compilation_database.commands(
        manifest.load(root / "config/units.toml"), root, clang.clang_bin(),
        [mirror, root / "include", root / ZLIB_INC])
    commands = [r for r in commands if Path(r["file"]).exists()]
    if args.module:
        want = {m.removesuffix(".cpp") for m in args.module}
        missing = want - {Path(r["file"]).stem for r in commands}
        if missing:
            ap.error(f"no TU matched {sorted(missing)}")
        commands = [r for r in commands if Path(r["file"]).stem in want]

    if not commands:
        ap.error("no translation units selected")
    try:
        classes = collect(ci, commands, root, mirror, dc)
    except RuntimeError as exc:
        print(f"No edits applied: {exc}", file=sys.stderr)
        return 2
    total_fix = sum(1 for info in classes.values() for m in info["members"] if m["fix"])
    edits = plan_edits(classes)
    n_labels = sum(len(v) for v in edits.values())

    print("=" * 66)
    print(f"classes to touch : {len(classes)}")
    print(f"members tightened: {total_fix}")
    print(f"labels inserted  : {n_labels}   across {len(edits)} files")
    print("=" * 66)
    for info in sorted(classes.values(), key=lambda x: x["owner"]):
        fixes = [m for m in info["members"] if m["fix"]]
        rel = Path(info["file"]).name
        print(f"\n{info['owner']}  ({rel})")
        for m in fixes:
            print(f"    L{m['line']:<5} {m['name']:34} public -> {m['target']}")

    if args.apply:
        for f, items in edits.items():
            apply_file(f, items)
        print(f"\nAPPLIED {n_labels} labels to {len(edits)} files.")
    else:
        print(f"\n(dry run; re-run with --apply to write {n_labels} labels)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
