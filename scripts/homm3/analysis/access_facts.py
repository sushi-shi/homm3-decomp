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

    PYTHONPATH=scripts python -m homm3.analysis.access_facts
    PYTHONPATH=scripts python -m homm3.analysis.access_facts \
        --module herowindow --module adventuremapwindow
    PYTHONPATH=scripts python -m homm3.analysis.access_facts --all --json

Name correlation uses the project's naming convention, with exact names and
legacy owning aliases first. Overloads retain their signature/cv alternatives.
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
from homm3.analysis.source_facts import name_key, _aliases, type_differences, type_facts
from homm3.analysis.source_facts import semantic_name_key
from homm3.build import compilation_database
from homm3.core import clang, common
from homm3.core.project import Project
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


def dc_visibility(types: Types) -> dict[tuple[str, str], list[dict]]:
    """Keep overload identities and deduplicate repeated type definitions."""
    out = defaultdict(list)
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
            rec = {"display": f"{cname}::{mname}",
                   "access": field.get("access", "unspecified"),
                   "kind": kind, "family": None, "parameters": None,
                   "this_type": None}
            if kind == "method":
                # DC encodes static-ness as `this type == 0` in the LF_MFUNCTION,
                # not in the property bits (which read "ordinary" for a static),
                # so a method whose signature carries no `this` is static here.
                mf = types.get(field.get("type", 0))
                if mf.get("kind") == "function" and mf.get("this", 0) == 0:
                    rec["family"] = "static"
                else:
                    rec["family"] = PROPERTY_FAMILY.get(field.get("property"))
                arguments = types.get(mf.get("arguments", 0)).get("types")
                if arguments is not None and 0 not in arguments:
                    rec["parameters"] = [types.declaration(t) for t in arguments]
                if mf.get("this"):
                    rec["this_type"] = types.declaration(mf["this"])
            if rec not in out[key]:
                out[key].append(rec)
    return out


def is_project_file(path: Path, root: Path, mirror: Path) -> bool:
    try:
        path = path.resolve()
    except OSError:
        return False
    if not path.is_relative_to(root.resolve()):
        return False
    for skip in (mirror, root / "vendor", root / "build"):
        try:
            if path.is_relative_to(skip.resolve()):
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


def owning_member_keys(source, offset, owner, spelling):
    """Correlate normalized names only through the declaration's own evidence."""
    keys = {name_key(spelling)}
    for alias in _aliases(source, {"loc": {"offset": offset}}):
        if "::" in alias:
            alias_owner, _, alias = alias.rpartition("::")
            if name_key(alias_owner) != name_key(owner):
                continue
        if ";" not in alias:
            keys.add(name_key(alias))
    return sorted(keys)


def collect_authored(ci, commands, root: Path, mirror: Path, want_class: set[str]):
    """usr -> authored member fact, deduped across TUs."""
    ACCESS = {ci.AccessSpecifier.PUBLIC: "public",
              ci.AccessSpecifier.PROTECTED: "protected",
              ci.AccessSpecifier.PRIVATE: "private"}
    method_kinds = (ci.CursorKind.CXX_METHOD, ci.CursorKind.CONSTRUCTOR,
                    ci.CursorKind.DESTRUCTOR)
    record_kinds = (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL,
                    ci.CursorKind.CLASS_TEMPLATE, ci.CursorKind.UNION_DECL)
    index = ci.Index.create()
    facts: dict[str, dict] = {}
    parsed, failures, diagnostics = 0, [], []
    sources = {}
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
            failures.append({"file": str(src), "error": str(exc)})
            continue
        parsed += 1
        diagnostics.extend({"file": str(src), "diagnostic": str(d)}
                           for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error)
        # Prune SDK/STL subtrees before crossing the Python/libclang boundary
        # for every descendant. Each namespace extension has its own cursor.
        pending = list(tu.cursor.get_children())
        while pending:
            cursor = pending.pop()
            loc = cursor.location.file
            if loc is None or not is_project_file(Path(loc.name), root, mirror):
                continue
            if cursor.kind in record_kinds or cursor.kind == ci.CursorKind.NAMESPACE:
                pending.extend(cursor.get_children())
            parent = cursor.semantic_parent
            if parent is None or parent.kind not in record_kinds:
                continue
            is_method = cursor.kind in method_kinds
            is_field = cursor.kind == ci.CursorKind.FIELD_DECL
            is_static_field = (cursor.kind == ci.CursorKind.VAR_DECL)
            if not (is_method or is_field or is_static_field):
                continue
            owner = qualified_owner(cursor, ci)
            if name_key(owner) not in want_class:
                continue
            access = ACCESS.get(cursor.access_specifier)
            if access is None:
                continue
            usr = cursor.get_usr()
            if not usr:
                continue
            path = Path(loc.name)
            if path not in sources:
                sources[path] = path.read_text()
            keys = owning_member_keys(sources[path], cursor.extent.start.offset,
                                      owner, cursor.spelling)
            facts[usr] = {
                "owner": owner, "name": cursor.spelling,
                "class_key": name_key(owner), "member_key": name_key(cursor.spelling),
                "member_keys": sorted(keys),
                "access": access,
                "family": authored_property(cursor, ci) if is_method else None,
                "is_method": is_method,
                "kind": "method" if is_method else "static_member" if is_static_field else "member",
                "parameters": [a.type.get_canonical().spelling for a in cursor.get_arguments()] if is_method else None,
                "const": cursor.is_const_method() if cursor.kind == ci.CursorKind.CXX_METHOD else False,
                "file": str(path.relative_to(root)), "line": cursor.location.line,
            }
    # Cursor offsets belong to the parsed snapshot. A header edited during
    # a corpus scan can otherwise attach an unrelated owning-name comment.
    diagnostics.extend({"file": str(path),
                        "diagnostic": "source changed during audit; rerun on a stable tree"}
                       for path, original in sources.items()
                       if path.read_text() != original)
    return facts, {"parsed": parsed, "failures": failures, "diagnostics": diagnostics}


def correlate(fact, dc):
    """Resolve a member, or keep the missing/ambiguous overload explicit.

    A unique name/arity is a source correlation, not a type-equality verdict.
    Equal-arity overloads require positive matching parameter and cv facts.
    Different access/property alternatives never count as an automatic match.
    """
    candidates = []
    for key in fact.get("member_keys", [fact["member_key"]]):
        for rec in dc.get((fact["class_key"], key), []):
            if rec["kind"] == fact["kind"] and rec not in candidates:
                candidates.append(rec)
    if not candidates and fact.get("name"):
        # Work from original case/prefix spelling, before lossy key folding.
        key = semantic_name_key(fact["name"])
        for (owner, _), records in dc.items():
            if owner != fact["class_key"]:
                continue
            for rec in records:
                if (rec["kind"] == fact["kind"]
                        and semantic_name_key(rec["display"].rsplit("::", 1)[-1]) == key
                        and rec not in candidates):
                    candidates.append(rec)
    if not candidates:
        return None, "name"
    if fact["is_method"]:
        candidates = [r for r in candidates if r["parameters"] is not None
                      and len(r["parameters"]) == len(fact["parameters"])]
        if not candidates:
            return None, "signature"
        if len(candidates) > 1:
            candidates = [r for r in candidates
                          if all(type_differences(a, b) == ([], [])
                                 for a, b in zip(r["parameters"], fact["parameters"]))
                          and (r["family"] == "static" or
                               object_const(r["this_type"]) == fact["const"])]
    if len(candidates) != 1:
        return None, "ambiguous"
    if candidates[0]["access"] not in ("public", "private", "protected"):
        return None, "unspecified-access"
    return candidates[0], None


def object_const(this_type):
    parsed = type_facts(this_type or "")
    return None if parsed is None else "const" in parsed.qualifiers[0]


def exit_status(summary):
    """Same status for text and JSON: 2 incomplete, 1 findings, 0 checked clean."""
    if (summary["parse_failures"] or summary["clang_errors"]
            or summary["name_correlation_gaps"] or summary["signature_correlation_gaps"]
            or summary["ambiguous_members"] or summary["unspecified_access"]
            or summary["coverage_missing_dc_members"]
            or summary.get("empty_scope", False)):
        return 2
    return int(bool(summary["access"]["mismatch"] or summary["property"]["mismatch"]))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--module", action="append", default=[],
                    help="module[.cpp] to parse; repeatable (default: whole tree)")
    ap.add_argument("--all", action="store_true", help="parse every TU (the default)")
    ap.add_argument("--limit", type=int, help="cap TUs parsed (debugging)")
    ap.add_argument("--show", type=int, default=25, help="mismatches to list per category")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()
    if args.limit is not None and args.limit <= 0:
        ap.error("--limit must be positive")

    ci = load_cindex()
    root = common.HOMM3_DIR
    mirror = clang.mirror()
    if mirror is None:
        sys.exit("VC6 header mirror unavailable; run a build first")

    types = Types.from_symbols(load_symbols())
    dc = dc_visibility(types)
    print(f"# DC recorded members: {sum(map(len, dc.values()))}", file=sys.stderr)

    commands = compilation_database.commands(
        manifest.load(root / "config/units.toml"), root, clang.clang_bin(),
        [mirror, *Project(root).includes])
    commands = [r for r in commands if Path(r["file"]).exists()]
    if args.module:
        wanted = {m.removesuffix(".cpp") for m in args.module}
        missing = wanted - {Path(r["file"]).stem for r in commands}
        if missing:
            ap.error(f"no TU matched {sorted(missing)}")
        commands = [r for r in commands if Path(r["file"]).stem in wanted]
    if args.limit:
        commands = commands[: args.limit]

    want_class = {ck for (ck, _mk) in dc}
    authored, parsing = collect_authored(ci, commands, root, mirror, want_class)

    # Score every authored member that DC also records for the same class.
    access_ok = access_bad = 0
    prop_ok = prop_bad = 0
    gaps = Counter()
    gap_members = []
    access_mismatch, prop_mismatch = [], []
    scored_classes: set[str] = set()
    covered = set()
    for fact in authored.values():
        rec, gap = correlate(fact, dc)
        if rec is None:
            gaps[gap] += 1
            gap_members.append({"member": f"{fact['owner']}::{fact['name']}",
                                "parameters": fact["parameters"], "reason": gap,
                                "file": fact["file"], "line": fact["line"]})
            continue
        scored_classes.add(fact["class_key"])
        covered.add(json.dumps(rec, sort_keys=True))
        if fact["access"] == rec["access"]:
            access_ok += 1
        else:
            access_bad += 1
            access_mismatch.append((fact, [rec["access"]], rec["display"]))
        if fact["is_method"] and rec["family"] and rec["family"] != "friend":
            if fact["family"] == rec["family"]:
                prop_ok += 1
            else:
                prop_bad += 1
                prop_mismatch.append((fact, [rec["family"]], rec["display"]))

    # Coverage: DC members of a scored class that we never authored a match for.
    cover_missing = [rec["display"] + ("(" + ", ".join(rec["parameters"]) + ")"
                                      if rec["parameters"] is not None else "")
                     for (ck, _), records in dc.items() for rec in records
                     if ck in scored_classes and json.dumps(rec, sort_keys=True) not in covered]

    def pct(ok, bad):
        total = ok + bad
        return 100.0 * ok / total if total else 0.0

    summary = {
        "tus_requested": len(commands),
        "tus_parsed": parsing["parsed"],
        "authored_members": len(authored),
        "empty_scope": not commands or not authored,
        "parse_failures": len(parsing["failures"]),
        "clang_errors": len(parsing["diagnostics"]),
        "dc_members": sum(map(len, dc.values())),
        "scored_classes": len(scored_classes),
        "access": {"match": access_ok, "mismatch": access_bad, "adherence_pct": round(pct(access_ok, access_bad), 2)},
        "property": {"match": prop_ok, "mismatch": prop_bad, "adherence_pct": round(pct(prop_ok, prop_bad), 2)},
        "name_correlation_gaps": gaps["name"],
        "signature_correlation_gaps": gaps["signature"],
        "ambiguous_members": gaps["ambiguous"],
        "unspecified_access": gaps["unspecified-access"],
        "coverage_missing_dc_members": len(cover_missing),
    }

    if args.json:
        print(json.dumps({
            "summary": summary,
            "access_mismatches": [
                {"member": f"{f['owner']}::{f['name']}", "authored": f["access"], "dc": d,
                 "parameters": f["parameters"], "file": f["file"], "line": f["line"]}
                for f, d, _ in access_mismatch],
            "property_mismatches": [
                {"member": f"{f['owner']}::{f['name']}", "authored": f["family"], "dc": d,
                 "parameters": f["parameters"], "const": f["const"],
                 "file": f["file"], "line": f["line"]}
                for f, d, _ in prop_mismatch],
            "coverage_missing": sorted(cover_missing),
            "correlation_gaps": gap_members,
            "parse_failures": parsing["failures"],
            "clang_errors": parsing["diagnostics"],
        }, indent=2))
        return exit_status(summary)

    line = "=" * 68
    print(line)
    print("Dreamcast member-visibility adherence")
    print(line)
    print(f"TUs parsed              {summary['tus_parsed']}")
    print(f"Parse failures / errors {summary['parse_failures']} / {summary['clang_errors']}")
    print(f"DC recorded members     {summary['dc_members']}")
    print(f"Classes scored (ours)   {summary['scored_classes']}")
    print()
    print(f"ACCESS   match {access_ok:5d}   mismatch {access_bad:4d}   "
          f"adherence {summary['access']['adherence_pct']:6.2f}%")
    print(f"PROPERTY match {prop_ok:5d}   mismatch {prop_bad:4d}   "
          f"adherence {summary['property']['adherence_pct']:6.2f}%")
    print(f"Name-correlation gaps   {gaps['name']}  (authored member, DC has class but not the name)")
    print(f"Signature gaps          {gaps['signature']}")
    print(f"Ambiguous members       {gaps['ambiguous']}")
    print(f"Unspecified access      {gaps['unspecified-access']}")
    print(f"Coverage gaps           {len(cover_missing)}  (DC members of our classes with no authored match)")

    if access_mismatch:
        print(f"\n-- ACCESS mismatches (showing {min(args.show, len(access_mismatch))} of {len(access_mismatch)}) --")
        for fact, dcacc, disp in access_mismatch[: args.show]:
            print(f"  {fact['owner']}::{fact['name']:32}  authored={fact['access']:9}  dc={'/'.join(dcacc)}   [{disp}]")
    if prop_mismatch:
        print(f"\n-- PROPERTY mismatches (showing {min(args.show, len(prop_mismatch))} of {len(prop_mismatch)}) --")
        for fact, fam, disp in prop_mismatch[: args.show]:
            print(f"  {fact['owner']}::{fact['name']:32}  authored={fact['family']:12}  dc={'/'.join(fam)}   [{disp}]")

    return exit_status(summary)


if __name__ == "__main__":
    raise SystemExit(main())
