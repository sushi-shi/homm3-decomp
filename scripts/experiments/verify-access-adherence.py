"""Measure how far authored C++ adheres to Dreamcast-recorded member visibility.

The DC NB11 CodeView type stream records, for every class/struct member, its
access specifier (private / protected / public) and, for methods, a property
(ordinary / static / virtual / pure-virtual / friend, plus vtable-introducing
variants).  This script joins those positive facts to the authored AST -- read
through libclang under each TU's real VC6-mirror profile -- and reports what
fraction of the members we author agree with the DC access and property.

Read-only.  It never rewrites source, objects, retail bytes or scoring state,
and it is not a byte target: DC is positive source evidence for an older
cross-architecture build, so a divergence is a lead to check against retail,
not a proof.  The "introducing" vtable split is a slot-layout fact, not a C++
keyword, so it is displayed but folded into the virtual family for the verdict.

    PYTHONPATH=scripts python scripts/experiments/verify-access-adherence.py
    PYTHONPATH=scripts python scripts/experiments/verify-access-adherence.py \
        --module herowindow --module adventuremapwindow
    PYTHONPATH=scripts python scripts/experiments/verify-access-adherence.py --all --json

Name correlation reuses the project's case/underscore folding
(`source_facts.name_key`), so `BlitToScreen` and `blitToScreen` correlate.
Only records defined under the repo (not the header mirror or vendored zlib)
count as authored, and only DC classes we author at least one member of are
scored -- the number is about our code, never STL/MFC surface.
"""

import argparse
import ctypes
import glob
import json
import os
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path

from homm3 import manifest
from homm3.analysis.dc_lines import load_symbols
from homm3.analysis.source_facts import name_key
from homm3.build import compilation_database
from homm3.core import clang, common
from homm3.core.cc_wrap import ZLIB_INC
from homm3.core.nb11_types import Types


# DC method property -> comparable family.  A C++ source cannot spell whether a
# virtual introduces a new vtable slot or overrides one, so both collapse here.
PROPERTY_FAMILY = {
    "ordinary": "ordinary",
    "static": "static",
    "virtual": "virtual",
    "introducing virtual": "virtual",
    "pure virtual": "pure-virtual",
    "pure introducing virtual": "pure-virtual",
    "friend": "friend",
}


def find_libstdcxx() -> str | None:
    for pattern in ("*gcc-*-lib/lib/libstdc++.so.6", "*gcc-*/lib*/libstdc++.so.6"):
        hits = sorted(glob.glob(f"/nix/store/{pattern}"))
        if hits:
            return hits[-1]
    return None


def find_libclang() -> str | None:
    if os.environ.get("HOMM3_LIBCLANG"):
        return os.environ["HOMM3_LIBCLANG"]
    # Prefer the libclang whose version matches the clang binary the repo drives.
    version = ""
    try:
        out = subprocess.run([clang.clang_bin() or "clang", "--version"],
                             text=True, capture_output=True, timeout=20).stdout
        for token in out.split():
            if token[:1].isdigit() and "." in token:
                version = token
                break
    except Exception:
        pass
    candidates: list[str] = []
    if version:
        candidates += sorted(glob.glob(f"/nix/store/*clang-{version}-lib/lib/libclang.so"))
    candidates += sorted(glob.glob("/nix/store/*clang-*-lib/lib/libclang.so"))
    candidates += sorted(glob.glob(os.path.expanduser("~/.cache/uv/**/pylibclang/libclang.so*"),
                                   recursive=True))
    return candidates[0] if candidates else None


def load_cindex():
    """Bind libclang, preloading libstdc++ so its dependency resolves in-process."""
    libstdcxx = find_libstdcxx()
    if libstdcxx:
        try:
            ctypes.CDLL(libstdcxx, mode=ctypes.RTLD_GLOBAL)
        except OSError:
            pass
    lib = find_libclang()
    if not lib:
        sys.exit("no libclang.so found; set HOMM3_LIBCLANG to one")
    import clang.cindex as ci
    ci.Config.set_library_file(lib)
    ci.Index.create()  # force-load now, so a bad pairing fails loudly here
    print(f"# libclang: {lib}", file=sys.stderr)
    return ci


def dc_visibility(types: Types) -> dict[tuple[str, str], dict]:
    """(classKey, memberKey) -> {accesses, families, kinds, display} from NB11."""
    out: dict[tuple[str, str], dict] = defaultdict(
        lambda: {"accesses": set(), "families": set(), "kinds": set(),
                 "methods": 0, "display": None})
    for index in list(types.records):
        item = types.get(index)
        if item.get("kind") not in ("class", "struct") or item.get("forward"):
            continue
        cname = item.get("name")
        if not cname or cname.startswith("cv_type_"):
            continue
        for field in types.fields(item.get("fields", 0)):
            kind = field.get("kind")
            if kind not in ("method", "member", "static_member"):
                continue
            mname = field.get("name")
            if not mname:
                continue
            key = (name_key(cname), name_key(mname))
            rec = out[key]
            rec["display"] = rec["display"] or f"{cname}::{mname}"
            rec["accesses"].add(field.get("access", "unspecified"))
            rec["kinds"].add(kind)
            if kind == "method":
                rec["methods"] += 1
                # DC encodes static-ness as `this type == 0` in the LF_MFUNCTION,
                # not in the property bits (which read "ordinary" for a static),
                # so a method whose signature carries no `this` is static here.
                mf = types.get(field.get("type", 0))
                if mf.get("kind") == "function" and mf.get("this", 0) == 0:
                    rec["families"].add("static")
                else:
                    rec["families"].add(PROPERTY_FAMILY.get(field.get("property"), "ordinary"))
    return out


def is_project_file(path: Path, root: Path, mirror: Path) -> bool:
    try:
        path = path.resolve()
    except OSError:
        return False
    if not str(path).startswith(str(root.resolve())):
        return False
    for skip in (mirror, root / ZLIB_INC, root / "vendor", root / "build"):
        try:
            if str(path).startswith(str(skip.resolve())):
                return False
        except OSError:
            continue
    return True


def qualified_owner(cursor, ci) -> str:
    parts = []
    node = cursor.semantic_parent
    kinds = (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL,
             ci.CursorKind.CLASS_TEMPLATE, ci.CursorKind.NAMESPACE, ci.CursorKind.UNION_DECL)
    while node is not None and node.kind in kinds:
        if node.spelling:
            parts.append(node.spelling)
        node = node.semantic_parent
    return "::".join(reversed(parts))


def authored_property(cursor, ci) -> str:
    if cursor.is_static_method():
        return "static"
    if cursor.is_pure_virtual_method():
        return "pure-virtual"
    if cursor.is_virtual_method():
        return "virtual"
    return "ordinary"


def collect_authored(ci, commands, root: Path, mirror: Path, want_class: set[str]) -> dict[str, dict]:
    """usr -> authored member fact, deduped across TUs."""
    ACCESS = {ci.AccessSpecifier.PUBLIC: "public",
              ci.AccessSpecifier.PROTECTED: "protected",
              ci.AccessSpecifier.PRIVATE: "private"}
    method_kinds = (ci.CursorKind.CXX_METHOD, ci.CursorKind.CONSTRUCTOR,
                    ci.CursorKind.DESTRUCTOR)
    record_kinds = (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL, ci.CursorKind.UNION_DECL)
    index = ci.Index.create()
    facts: dict[str, dict] = {}
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
        for cursor in tu.cursor.walk_preorder():
            parent = cursor.semantic_parent
            if parent is None or parent.kind not in record_kinds:
                continue
            is_method = cursor.kind in method_kinds
            is_field = cursor.kind == ci.CursorKind.FIELD_DECL
            is_static_field = (cursor.kind == ci.CursorKind.VAR_DECL)
            if not (is_method or is_field or is_static_field):
                continue
            loc = cursor.location.file
            if loc is None or not is_project_file(Path(loc.name), root, mirror):
                continue
            owner = qualified_owner(cursor, ci)
            if name_key(owner) not in want_class:
                continue
            access = ACCESS.get(cursor.access_specifier)
            if access is None:
                continue
            usr = cursor.get_usr()
            facts[usr] = {
                "owner": owner, "name": cursor.spelling,
                "class_key": name_key(owner), "member_key": name_key(cursor.spelling),
                "access": access,
                "family": authored_property(cursor, ci) if is_method else None,
                "is_method": is_method,
            }
    return facts


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--module", action="append", default=[],
                    help="module[.cpp] to parse; repeatable (default: whole tree)")
    ap.add_argument("--all", action="store_true", help="parse every TU (the default)")
    ap.add_argument("--limit", type=int, help="cap TUs parsed (debugging)")
    ap.add_argument("--show", type=int, default=25, help="mismatches to list per category")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()

    ci = load_cindex()
    root = common.HOMM3_DIR
    mirror = clang.mirror()
    if mirror is None:
        sys.exit("VC6 header mirror unavailable; run a build first")

    types = Types.from_symbols(load_symbols())
    dc = dc_visibility(types)
    print(f"# DC recorded members: {len(dc)}", file=sys.stderr)

    commands = compilation_database.commands(
        manifest.load(root / "config/units.toml"), root, clang.clang_bin(),
        [mirror, root / "include", root / ZLIB_INC])
    commands = [r for r in commands if Path(r["file"]).exists()]
    if args.module:
        wanted = {m.removesuffix(".cpp") for m in args.module}
        commands = [r for r in commands if Path(r["file"]).stem in wanted]
        if not commands:
            sys.exit(f"no TU matched {sorted(wanted)}")
    if args.limit:
        commands = commands[: args.limit]

    want_class = {ck for (ck, _mk) in dc}
    authored = collect_authored(ci, commands, root, mirror, want_class)

    # Score every authored member that DC also records for the same class.
    access_ok = access_bad = 0
    prop_ok = prop_bad = 0
    corr_gap = 0                       # authored member; class in DC, name not
    access_mismatch, prop_mismatch = [], []
    scored_classes: set[str] = set()
    # Method property (virtual/static/...) is only comparable for a name with a
    # single method on both sides.  An overloaded name -- a static plus an
    # instance overload, a virtualized plus a non-virtual variant -- collapses
    # under name_key, so a legitimate extra overload would masquerade as a
    # property divergence.  Access does not suffer this (overloads share it).
    authored_methods = Counter((f["class_key"], f["member_key"])
                               for f in authored.values() if f["is_method"])
    for fact in authored.values():
        key = (fact["class_key"], fact["member_key"])
        rec = dc.get(key)
        if rec is None:
            corr_gap += 1
            continue
        scored_classes.add(fact["class_key"])
        if rec["accesses"] == {"unspecified"}:
            pass  # DC gave no usable access for this member
        elif fact["access"] in rec["accesses"]:
            access_ok += 1
        else:
            access_bad += 1
            access_mismatch.append((fact, sorted(rec["accesses"]), rec["display"]))
        if (fact["is_method"] and rec["families"] and "friend" not in rec["families"]
                and rec["methods"] == 1 and authored_methods[key] == 1):
            if fact["family"] in rec["families"]:
                prop_ok += 1
            else:
                prop_bad += 1
                prop_mismatch.append((fact, sorted(rec["families"]), rec["display"]))

    # Coverage: DC members of a scored class that we never authored a match for.
    authored_pairs = {(f["class_key"], f["member_key"]) for f in authored.values()}
    cover_missing = [rec["display"] for (ck, mk), rec in dc.items()
                     if ck in scored_classes and (ck, mk) not in authored_pairs]

    def pct(ok, bad):
        total = ok + bad
        return 100.0 * ok / total if total else 0.0

    summary = {
        "tus_parsed": len(commands),
        "dc_members": len(dc),
        "scored_classes": len(scored_classes),
        "access": {"match": access_ok, "mismatch": access_bad, "adherence_pct": round(pct(access_ok, access_bad), 2)},
        "property": {"match": prop_ok, "mismatch": prop_bad, "adherence_pct": round(pct(prop_ok, prop_bad), 2)},
        "name_correlation_gaps": corr_gap,
        "coverage_missing_dc_members": len(cover_missing),
    }

    if args.json:
        print(json.dumps({
            "summary": summary,
            "access_mismatches": [
                {"member": f"{f['owner']}::{f['name']}", "authored": f["access"], "dc": d}
                for f, d, _ in access_mismatch],
            "property_mismatches": [
                {"member": f"{f['owner']}::{f['name']}", "authored": f["family"], "dc": d}
                for f, d, _ in prop_mismatch],
            "coverage_missing": sorted(cover_missing),
        }, indent=2))
        return 0

    line = "=" * 68
    print(line)
    print("Dreamcast member-visibility adherence")
    print(line)
    print(f"TUs parsed              {summary['tus_parsed']}")
    print(f"DC recorded members     {summary['dc_members']}")
    print(f"Classes scored (ours)   {summary['scored_classes']}")
    print()
    print(f"ACCESS   match {access_ok:5d}   mismatch {access_bad:4d}   "
          f"adherence {summary['access']['adherence_pct']:6.2f}%")
    print(f"PROPERTY match {prop_ok:5d}   mismatch {prop_bad:4d}   "
          f"adherence {summary['property']['adherence_pct']:6.2f}%")
    print(f"Name-correlation gaps   {corr_gap}  (authored member, DC has class but not the name)")
    print(f"Coverage gaps           {len(cover_missing)}  (DC members of our classes with no authored match)")

    if access_mismatch:
        print(f"\n-- ACCESS mismatches (showing {min(args.show, len(access_mismatch))} of {len(access_mismatch)}) --")
        for fact, dcacc, disp in access_mismatch[: args.show]:
            print(f"  {fact['owner']}::{fact['name']:32}  authored={fact['access']:9}  dc={'/'.join(dcacc)}   [{disp}]")
    if prop_mismatch:
        print(f"\n-- PROPERTY mismatches (showing {min(args.show, len(prop_mismatch))} of {len(prop_mismatch)}) --")
        for fact, fam, disp in prop_mismatch[: args.show]:
            print(f"  {fact['owner']}::{fact['name']:32}  authored={fact['family']:12}  dc={'/'.join(fam)}   [{disp}]")

    return 1 if (access_bad or prop_bad) else 0


if __name__ == "__main__":
    raise SystemExit(main())
