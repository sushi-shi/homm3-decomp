#!/usr/bin/env python3
"""ONE-OFF carve script (not a pipeline actor) - extract the NH3API v2.1
IDA database into a browsable reference corpus under evidence/ida/.

The IDB (IDA 7.6, released with NH3API v2.1) describes `Heroes3 HD.exe`
(md5 e41e00f3...), an HD-pressing address space - NOT our pinned image.
Everything extracted here is EXTERNAL REFERENCE MATERIAL: names, class
layouts, enums, prototypes to consult while decompiling; addresses become
retail claims only through a proven bridge (the hdmap correspondence is
joined onto the function table where it lands). evidence/ is scaffolding
and this corpus lives there deliberately.

Reads the IDB in place from decomp-attempt-1 (a quarry, never copied) via
its vendored python-idb.

Outputs (evidence/ida/):
  README.md       provenance + caveats
  types.h         every TIL local type rendered as a C declaration
  structs.h       the structs view with explicit member offsets
  enums.txt       the enum netnodes (names; members where renderable)
  functions.csv   hd_va, name, readable, prototype, flags, retail_rva
                  (retail rva via the hdmap masked-identity bridge)
  names.csv       every name: hd_va, kind, name
  vtables.csv     the ??_7 vtable names
  comments.csv    every stored comment: hd_va, kind, text
"""
from __future__ import annotations

import csv
import sys
from pathlib import Path

from homm3.carve import common
from homm3.carve.naming import demangle

ATTEMPT1 = common.HOMM3_DIR.parent / "decomp-attempt-1"
IDB_PATH = ATTEMPT1 / "build/analysis/ida/H3.exe.idb"
PYTHON_IDB = ATTEMPT1 / "build/python-idb"
HD_MAP = common.EVIDENCE_DIR / "retail-hd-name-map.csv"
OUT = common.EVIDENCE_DIR / "ida"

FF_COMM = 0x800
U32 = 0xFFFFFFFF

README = """\
# NH3API v2.1 IDB - browsable reference corpus

Extracted by `scripts/homm3/carve/idb_extract.py` (one-off) from the IDA
7.6 database released with NH3API v2.1 (asset sha256 `4a39cc47...`, read in
place from `../decomp-attempt-1/build/analysis/ida/H3.exe.idb`, parsed with
its vendored python-idb).

**Addresses are the HD pressing** (`Heroes3 HD.exe`, md5 `e41e00f3...`) -
NOT the pinned retail image. They are address-compatible with our HD Mod
executable (98.3% of NH3API call-macro addresses are call targets there),
so `functions.csv` carries a `retail_rva` column where the hdmap
masked-identity bridge lands; every other address is reference-only.
Names, layouts, enums, and prototypes are external candidates to consult -
an identity still needs retail-byte proof before it is claimed.

| file | contents |
|---|---|
| types.h | all TIL local types as C declarations (the class layouts) |
| structs.h | the structs view with explicit member offsets |
| enums.txt | enum netnodes (member rendering where python-idb allows) |
| functions.csv | functions with prototypes (parameter names!) + bridge |
| names.csv | every name in the database, classified |
| vtables.csv | the `??_7` vtable addresses |
| comments.csv | every stored comment |

Rendering caveats: python-idb draws enum member values with a high-dword
serial artifact (`K_SPELL_COUNT = 0x100000051` - the TRUE value is the low
32 bits, 0x51); struct members appear in declared order without offsets;
the IDA 7.6 `id2` section is unparsed (no known loss for these tables).
"""


def classify(name: str) -> str:
    if name.startswith("??_7"):
        return "vtable"
    if name.startswith("?"):
        return "mangled"
    if name.startswith(("sub_", "loc_", "unk_", "byte_", "dword_", "word_",
                        "off_", "flt_", "dbl_", "asc_", "stru_")):
        return "auto"
    if name.startswith(("a", "sz")) and name[1:2].isupper():
        return "string"
    return "user"


def main(argv=None) -> int:
    if not IDB_PATH.is_file():
        common.die(f"IDB not found at {IDB_PATH}")
    sys.path.insert(0, str(PYTHON_IDB))
    import idb
    import idb.analysis as ana
    import idb.netnode as nn

    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "README.md").write_text(README)

    bridge = {}
    if HD_MAP.is_file():
        with HD_MAP.open() as fh:
            for r in csv.DictReader(
                    line for line in fh if not line.startswith("#")):
                if r["our_state"] == "entry":
                    bridge[int(r["hd_va"], 16)] = int(r["rva"], 16)

    with idb.from_file(str(IDB_PATH)) as db:
        api = idb.IDAPython(db)
        idc, idautils = api.idc, api.idautils

        # ---- types.h --------------------------------------------------
        lines = ["/* All TIL local types of the NH3API v2.1 IDB, rendered "
                 "by python-idb. HD-pressing reference. */", ""]
        rendered = failed = 0
        for d in db.til.types.defs:
            name = getattr(d, "name", None)
            if not name:
                continue
            try:
                lines.append(d.type.get_typestr() + ";")
                rendered += 1
            except Exception:
                lines.append(f"/* unrenderable: {name} */")
                failed += 1
            lines.append("")
        (OUT / "types.h").write_text("\n".join(lines) + "\n")
        print(f"[idb extract] types.h: {rendered} rendered, {failed} "
              "unrenderable")

        # ---- structs.h ------------------------------------------------
        node = nn.Netnode(db, "$ structs")
        lines = ["/* IDB structs view (members in declared order; python-idb"
                 " exposes no offsets). HD-pressing reference. */", ""]
        count = 0
        for i in node.alts():
            sid = (node.altval(i) & U32) - 1
            try:
                st = ana.Struct(db, sid)
                name = st.get_name()
                if not name or name.startswith("$ "):
                    continue
                members = list(st.get_members())
            except Exception:
                continue
            lines.append(f"struct {name} {{")
            for m in members:
                try:
                    mtype = str(m.get_type() or "?")
                except Exception:
                    mtype = "?"
                comment = ""
                try:
                    c = (m.get_member_comment()
                         or m.get_repeatable_member_comment())
                    if c:
                        comment = f"  /* {c} */"
                except Exception:
                    pass
                lines.append(f"    {mtype:24} {m.get_name()};{comment}")
            lines.append("};")
            lines.append("")
            count += 1
        (OUT / "structs.h").write_text("\n".join(lines) + "\n")
        print(f"[idb extract] structs.h: {count} structs")

        # ---- enums.txt ------------------------------------------------
        enode = nn.Netnode(db, "$ enums")
        lines = ["# IDB enum netnodes (HD-pressing reference).", ""]
        til_by_name = {getattr(d, "name", None): d
                       for d in db.til.types.defs}
        count = 0
        for i in enode.alts():
            eid = (enode.altval(i) & U32) - 1
            try:
                name = nn.Netnode(db, eid).name()
            except Exception:
                continue
            if not name:
                continue
            count += 1
            til_def = til_by_name.get(name)
            if til_def is not None:
                try:
                    lines.append(til_def.type.get_typestr() + ";")
                    lines.append("")
                    continue
                except Exception:
                    pass
            lines.append(f"enum {name}; /* members not renderable via "
                         "python-idb */")
            lines.append("")
        (OUT / "enums.txt").write_text("\n".join(lines) + "\n")
        print(f"[idb extract] enums.txt: {count} enums")

        # ---- functions.csv --------------------------------------------
        functions = ana.Functions(db).functions
        rows = []
        with_sig = bridged = 0
        for fva in sorted(functions):
            try:
                f = ana.Function(db, fva)
                name = f.get_name()
            except Exception:
                continue
            proto = ""
            try:
                sig = f.get_signature()
                if sig is not None:
                    proto = sig.get_typestr()
                    with_sig += 1
            except Exception:
                pass
            retail = bridge.get(fva)
            if retail is not None:
                bridged += 1
            rows.append({
                "hd_va": f"0x{fva:x}",
                "name": name,
                "readable": demangle(name) if name.startswith("?") else "",
                "prototype": " ".join(proto.split()),
                "retail_rva": f"0x{retail:x}" if retail is not None else "",
            })
        with (OUT / "functions.csv").open("w", newline="") as fh:
            fh.write("# NH3API v2.1 IDB functions; hd_va is the HD "
                     "pressing; retail_rva only where the hdmap\n# "
                     "masked-identity bridge lands (crossbuild-verified "
                     "correspondence, name still external).\n")
            w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()))
            w.writeheader()
            w.writerows(rows)
        print(f"[idb extract] functions.csv: {len(rows)} functions, "
              f"{with_sig} prototyped, {bridged} bridged to retail")

        # ---- names.csv / vtables.csv ----------------------------------
        names = sorted(idautils.Names())
        with (OUT / "names.csv").open("w", newline="") as fh, \
                (OUT / "vtables.csv").open("w", newline="") as vh:
            fh.write("# every name in the IDB (HD-pressing addresses).\n")
            w = csv.writer(fh)
            w.writerow(["hd_va", "kind", "name", "readable"])
            vh.write("# ??_7 vtable names (HD-pressing addresses).\n")
            vw = csv.writer(vh)
            vw.writerow(["hd_va", "name", "class"])
            vt = 0
            for ea, name in names:
                kind = classify(name)
                w.writerow([f"0x{ea:x}", kind, name,
                            demangle(name) if name.startswith("?") else ""])
                if kind == "vtable":
                    cls = name[4:].split("@@", 1)[0]
                    vw.writerow([f"0x{ea:x}", name, cls])
                    vt += 1
        print(f"[idb extract] names.csv: {len(names)}; vtables.csv: {vt}")

        # ---- comments.csv ---------------------------------------------
        with (OUT / "comments.csv").open("w", newline="") as fh:
            fh.write("# every stored comment (HD-pressing addresses); kind "
                     "r=repeatable, n=normal.\n")
            w = csv.writer(fh)
            w.writerow(["hd_va", "kind", "text"])
            count = 0
            for start in idautils.Segments():
                end = idc.SegEnd(start)
                ea = start
                while ea < end:
                    if idc.GetFlags(ea) & FF_COMM:
                        for repeatable, kind in ((False, "n"), (True, "r")):
                            try:
                                c = idc.GetCommentEx(ea, repeatable)
                            except Exception:
                                c = None
                            if c:
                                w.writerow([f"0x{ea:x}", kind, c])
                                count += 1
                    ea += 1
        print(f"[idb extract] comments.csv: {count} comments")
    print(f"[idb extract] -> {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
