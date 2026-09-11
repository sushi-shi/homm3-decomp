"""Align authored member access to Dreamcast-recorded visibility, in place.

Every divergence the audit finds is `public` in our source but `private`/
`protected` in the DC CodeView field list.  Access is codegen-neutral under
MSVC (not mangled, no effect on layout or vtable) *as long as no declaration
moves*, so this transformer never reorders: it brackets each affected member
with an access label before it and restores the prior access after it, coalescing
physically-adjacent members that share a target.  Data-member offsets and virtual
slot order are therefore preserved exactly; only the access label changes.

    # dry run -- print the plan, touch nothing
    PYTHONPATH=scripts python scripts/experiments/apply-access-adherence.py
    # apply the edits to the headers
    PYTHONPATH=scripts python scripts/experiments/apply-access-adherence.py --apply

Only `public -> private/protected` tightenings backed by an unanimous DC access
for that name are applied; ambiguous names (overloads whose DC access disagrees)
are skipped.  Build afterwards: a tightening that fails to compile means our code
has an out-of-class caller DC modelled differently -- relax that one and note it.
"""

import argparse
import ctypes
import glob
import os
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

from homm3 import manifest
from homm3.analysis.dc_lines import load_symbols
from homm3.analysis.source_facts import name_key
from homm3.build import compilation_database
from homm3.core import clang, common
from homm3.core.cc_wrap import ZLIB_INC
from homm3.core.nb11_types import Types


def load_cindex():
    for pat in ("*gcc-*-lib/lib/libstdc++.so.6", "*gcc-*/lib*/libstdc++.so.6"):
        hits = sorted(glob.glob(f"/nix/store/{pat}"))
        if hits:
            try:
                ctypes.CDLL(hits[-1], mode=ctypes.RTLD_GLOBAL)
            except OSError:
                pass
            break
    lib = os.environ.get("HOMM3_LIBCLANG")
    if not lib:
        cands = sorted(glob.glob("/nix/store/*clang-*-lib/lib/libclang.so"))
        cands += sorted(glob.glob(os.path.expanduser(
            "~/.cache/uv/**/pylibclang/libclang.so*"), recursive=True))
        lib = cands[0] if cands else None
    if not lib:
        sys.exit("no libclang.so; set HOMM3_LIBCLANG")
    import clang.cindex as ci
    ci.Config.set_library_file(lib)
    ci.Index.create()
    print(f"# libclang: {lib}", file=sys.stderr)
    return ci


def dc_access_map(types: Types) -> dict[tuple[str, str], str]:
    """(classKey, memberKey) -> access, only where every DC entry agrees."""
    seen: dict[tuple[str, str], set[str]] = defaultdict(set)
    for index in list(types.records):
        item = types.get(index)
        if item.get("kind") not in ("class", "struct") or item.get("forward"):
            continue
        cname = item.get("name")
        if not cname or cname.startswith("cv_type_"):
            continue
        for field in types.fields(item.get("fields", 0)):
            if field.get("kind") not in ("method", "member", "static_member"):
                continue
            mname = field.get("name")
            if not mname:
                continue
            seen[(name_key(cname), name_key(mname))].add(field.get("access", "unspecified"))
    return {key: next(iter(accs)) for key, accs in seen.items()
            if len(accs) == 1 and next(iter(accs)) in ("private", "protected")}


def is_project_file(path: Path, root: Path, mirror: Path) -> bool:
    try:
        rp = str(path.resolve())
    except OSError:
        return False
    if not rp.startswith(str(root.resolve())):
        return False
    for skip in (mirror, root / ZLIB_INC, root / "vendor", root / "build"):
        try:
            if rp.startswith(str(skip.resolve())):
                return False
        except OSError:
            continue
    return True


def qualified_owner(cursor, ci) -> str:
    parts, node = [], cursor.semantic_parent
    kinds = (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL,
             ci.CursorKind.CLASS_TEMPLATE, ci.CursorKind.NAMESPACE, ci.CursorKind.UNION_DECL)
    while node is not None and node.kind in kinds:
        if node.spelling:
            parts.append(node.spelling)
        node = node.semantic_parent
    return "::".join(reversed(parts))


def load_exclude(path: str | None) -> set[tuple[str, str]]:
    """`Class::member` per line -> {(classKey, memberKey)} kept public.

    These are members DC records private/protected but our source legitimately
    calls from outside the class (a caller DC modelled as a friend, or a genuine
    call-graph platform difference); tightening them would not compile.
    """
    out: set[tuple[str, str]] = set()
    if not path or not Path(path).exists():
        return out
    for line in Path(path).read_text().splitlines():
        line = line.split("#", 1)[0].strip()
        if "::" in line:
            cls, _, mem = line.rpartition("::")
            out.add((name_key(cls), name_key(mem)))
    return out


def collect(ci, commands, root, mirror, dc, exclude):
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
            print(f"#   parse failed: {exc}", file=sys.stderr)
            continue
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
            owner = qualified_owner(rec, ci) or rec.spelling
            ckey = name_key(owner)
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
                target = dc.get((ckey, mkey))
                fix = bool(target and cur == "public" and target in ("private", "protected")
                           and (ckey, mkey) not in exclude)
                members.append({
                    "name": m.spelling, "line": m.extent.start.line,
                    "end": m.extent.end.line, "cur": cur,
                    "target": target if fix else cur, "fix": fix,
                })
            if any(x["fix"] for x in members):
                classes[fkey] = {"file": loc.name, "owner": owner,
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
            inserts[info["file"]].append((first["line"] - 1, False, f"{tgt}:"))
            inserts[info["file"]].append((last["end"], True, "public:"))
            i = j + 1
    return {f: sorted(items, key=lambda e: (-e[0], e[1])) for f, items in inserts.items()}


def indent_of(line: str) -> str:
    return line[: len(line) - len(line.lstrip())]


def apply_file(path: str, inserts: list[tuple[int, bool, str]]):
    lines = Path(path).read_text().splitlines(keepends=True)
    for idx, _is_close, label in inserts:  # pre-ordered by plan_edits
        ref = lines[idx] if idx < len(lines) else ""
        if not ref.strip() or ref.lstrip().startswith("}"):
            ref = lines[idx - 1] if idx > 0 else ref  # blank/brace: borrow member indent
        text = indent_of(ref).rstrip("\n") + label + "\n"
        lines.insert(idx, text)
    Path(path).write_text("".join(lines))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--apply", action="store_true", help="write edits (default: dry run)")
    ap.add_argument("--module", action="append", default=[], help="limit TUs parsed")
    ap.add_argument("--exclude-file", help="`Class::member` lines kept public (external callers)")
    args = ap.parse_args()
    exclude = load_exclude(args.exclude_file)
    if exclude:
        print(f"# excluded (kept public): {len(exclude)}", file=sys.stderr)

    ci = load_cindex()
    root = common.HOMM3_DIR
    mirror = clang.mirror()
    types = Types.from_symbols(load_symbols())
    dc = dc_access_map(types)
    print(f"# DC unanimous private/protected members: {len(dc)}", file=sys.stderr)

    commands = compilation_database.commands(
        manifest.load(root / "config/units.toml"), root, clang.clang_bin(),
        [mirror, root / "include", root / ZLIB_INC])
    commands = [r for r in commands if Path(r["file"]).exists()]
    if args.module:
        want = {m.removesuffix(".cpp") for m in args.module}
        commands = [r for r in commands if Path(r["file"]).stem in want]

    classes = collect(ci, commands, root, mirror, dc, exclude)
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
