#!/usr/bin/env python3
"""homm3.analysis.dc_extract - extract the Dreamcast CodeView stream into a
browsable reference corpus under evidence/dreamcast/.

The vostok-project pattern (pdb_parser's "structure"): debug symbols are
worth more as a greppable materialized corpus than as a 26 MB dump. This
is the Dreamcast build of RoE (WinCE SH, compiler 12.17.8370, project
config `Release_with_debug` - an OPTIMIZED build with full symbols), i.e.
ANOTHER PRESSING: names/types/layouts are reference evidence; addresses
are DC .text offsets, never retail claims.

Self-contained parser over ../homm3-symbols/HoMM3-Dreamcast-Dump/dump.txt
(the carve-era Dump class is archived; this does not import it).

Outputs (evidence/dreamcast/):
  README.md        provenance, build-mode findings, inventory
  functions.csv    offset, cb, kind, name, module, file, line,
                   debug_start, debug_end, params, locals
  variables.csv    every named parameter and LOCAL: proc, kind,
                   sp_offset, type, name
  globals.csv      typed S_GDATA32/S_LDATA32 (incl. Class::`vftable')
  publics.csv      every S_PUB32
  constants.csv    S_CONSTANT name/type/value
  classes.csv      class name, size, member/base counts
  members.csv      class, offset, type, member name
  enums.csv        enum, enumerator, value
  compile.csv      per-module compiler records
"""
from __future__ import annotations

import csv
import re
import sys
from collections import defaultdict

from homm3.core import common

DUMP = common.HOMM3_DIR.parent / "homm3-symbols/HoMM3-Dreamcast-Dump/dump.txt"
OUT = common.EVIDENCE_DIR / "dreamcast"

PROC_RE = re.compile(r"S_([GL])PROC32: \[0001:([0-9A-F]{8})\], "
                     r"Cb: ([0-9A-F]{8}), Type:\s+\S+, (.+)$")
DEBUG_RE = re.compile(r"Debug start: ([0-9A-F]{8}), Debug end: ([0-9A-F]{8})")
REGREL_RE = re.compile(r"S_REGREL32: (\S+?)\+([0-9A-F]{8}), "
                       r"Type:\s+([^,]+), (.+)$")
DATA_RE = re.compile(r"S_([GL])DATA32: \[(\d{4}):([0-9A-F]{8})\], "
                     r"Type:\s+([^,]+), (.+)$")
PUB_RE = re.compile(r"S_PUB32: \[(\d{4}):([0-9A-F]{8})\], Flags: \S+, (.+)$",
                    re.M)
CONST_RE = re.compile(r"S_CONSTANT: Type:\s+([^,]+), Value: ([^,]+), (.+)$",
                      re.M)
RECORD_RE = re.compile(r"^0x([0-9a-f]{4,}) : Length = \d+, "
                       r"Leaf = 0x[0-9a-f]+ (LF_\w+)")
FILE_RANGE_RE = re.compile(r"^\s+(\S+), 0001:([0-9A-F]{8})-([0-9A-F]{8}), "
                           r"line/addr pairs = (\d+)")
PAIRS_RE = re.compile(r"(\d+)\s+([0-9A-F]{8})")

README = """\
# Dreamcast CodeView corpus (RoE pressing - reference evidence)

Extracted by `python3 -m homm3.analysis.dc_extract` from the cvdump text
in `../homm3-symbols/HoMM3-Dreamcast-Dump/` (itself the NB11 stream
embedded in the GD-ROM's `H3.EXE`, sha256 `cdbc7e75...`).

**Build**: WinCE SH (S_COMPILE says SH3, the linker says SH4), compiler
`Microsoft 32-bit C/C++ Optimizing Compiler 12.17.8370` (the eMbedded
VC / CE Platform Builder generation; most modules carry the CE
`MJ.MN.XXXX` version-stamp placeholder), project configuration
`Release_with_debug` - an **optimized release build with full debug
info**, not a debug build.

**Addresses are DC `.text` offsets** of another pressing. Names, types,
layouts, parameters, and locals are reference evidence for the retail
decompilation; retail claims still need the usual proof chain
(`evidence/retail-dc-name-map.csv` is the bridge where it exists).

| file | contents |
|---|---|
| functions.csv | every proc: extent, prologue/epilogue, file:line, counts |
| variables.csv | every named parameter and local (sp-relative, typed) |
| globals.csv | typed globals incl. `Class::`vftable'` symbols |
| publics.csv | all publics |
| constants.csv | named typed constants |
| classes.csv / members.csv | class layouts with member offsets + bases |
| enums.csv | enumerators with values |
| compile.csv | per-module compiler records |
"""


def main(argv=None) -> int:
    text = DUMP.read_text(errors="replace")
    sym_lo = text.index("*** SYMBOLS")
    sym_hi = text.index("*** Compacted")
    types_lo = text.index("*** GLOBAL TYPES")
    src_lo = text.index("*** SRCLINES")
    src_hi = text.index("*** SEGMENT MAP")

    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "README.md").write_text(README)

    # --- SRCLINES: offset -> (file, line) -----------------------------
    line_of = {}
    current_file = None
    for line in text[src_lo:src_hi].splitlines():
        m = FILE_RANGE_RE.match(line)
        if m:
            current_file = m.group(1)
            continue
        if current_file and re.match(r"^\s+\d+ [0-9A-F]{8}", line):
            for number, addr in PAIRS_RE.findall(line):
                line_of.setdefault(int(addr, 16), (current_file, int(number)))

    # --- SYMBOLS section ---------------------------------------------
    functions, variables, globals_, constants, compile_rows = \
        [], [], [], [], []
    module = "?"
    cur = None            # current proc row (mutable list)
    after_endarg = False
    compile_pending = None
    for line in text[sym_lo:sym_hi].splitlines():
        if line.startswith(".\\"):
            module = line.rsplit("\\", 1)[-1].strip()
            continue
        if "S_COMPILE:" in line:
            compile_pending = {"module": module, "processor": "", "version": ""}
            compile_rows.append(compile_pending)
            continue
        if compile_pending is not None:
            if "Target processor:" in line:
                compile_pending["processor"] = line.split(":", 1)[1].strip()
            elif "Compiler Version:" in line:
                compile_pending["version"] = line.split(":", 1)[1].strip()
                compile_pending = None
            continue
        m = PROC_RE.search(line)
        if m:
            kind, offset, cb, name = m.groups()
            cur = {"offset": int(offset, 16), "cb": int(cb, 16),
                   "kind": "global" if kind == "G" else "static",
                   "name": name.strip(), "module": module,
                   "debug_start": "", "debug_end": "",
                   "params": 0, "locals": 0}
            functions.append(cur)
            after_endarg = False
            continue
        if cur is not None:
            dm = DEBUG_RE.search(line)
            if dm:
                cur["debug_start"] = int(dm.group(1), 16)
                cur["debug_end"] = int(dm.group(2), 16)
                continue
            if "S_ENDARG" in line:
                after_endarg = True
                continue
            rm = REGREL_RE.search(line)
            if rm:
                reg, off, typ, name = rm.groups()
                which = "local" if after_endarg else "param"
                cur[which + "s"] += 1
                variables.append((cur["name"], cur["module"], which,
                                  f"{reg}+0x{int(off, 16):x}",
                                  typ.strip(), name.strip()))
                continue
        dm = DATA_RE.search(line)
        if dm:
            scope, seg, off, typ, name = dm.groups()
            globals_.append((f"{int(seg, 10)}", f"0x{int(off, 16):x}",
                             "global" if scope == "G" else "static",
                             typ.strip(), name.strip(), module))
            continue

    for f in functions:
        hit = line_of.get(f["offset"])
        f["file"], f["line"] = (hit if hit else ("", ""))

    # --- publics + constants (they live in the Compacted section) -----
    publics = [(f"{int(m.group(1), 10)}", f"0x{int(m.group(2), 16):x}",
                m.group(3).strip())
               for m in PUB_RE.finditer(text)]
    seen_const = set()
    for m in CONST_RE.finditer(text):
        row = (m.group(3).strip(), m.group(1).strip(), m.group(2).strip())
        if row not in seen_const:
            seen_const.add(row)
            constants.append(row)

    # --- GLOBAL TYPES: classes, field lists, enums --------------------
    records = {}
    rec_id, rec_kind, body = None, None, []
    for line in text[types_lo:sym_lo].splitlines():
        m = RECORD_RE.match(line)
        if m:
            if rec_id is not None:
                records[rec_id] = (rec_kind, body)
            rec_id, rec_kind, body = int(m.group(1), 16), m.group(2), []
        elif rec_id is not None:
            body.append(line)
    if rec_id is not None:
        records[rec_id] = (rec_kind, body)

    fieldlists = {}
    for rid, (kind, body) in records.items():
        if kind != "LF_FIELDLIST":
            continue
        entries = []
        pending = None
        for line in body:
            mm = re.search(r"LF_MEMBER, (\w+), type = ([^,]+), "
                           r"offset = (\d+)", line)
            if mm:
                pending = ["member", mm.group(2).strip(),
                           int(mm.group(3)), None]
                entries.append(pending)
                continue
            sm = re.search(r"LF_STATICMEMBER, (\w+), type = ([^,]+)", line)
            if sm:
                pending = ["static", sm.group(2).strip(), "", None]
                entries.append(pending)
                continue
            nm = re.search(r"member name = '([^']*)'", line)
            if nm and pending is not None:
                pending[3] = nm.group(1)
                pending = None
                continue
            snm = re.search(r"STATICMEMBER.*name = '([^']*)'", line)
            bm = re.search(r"LF_BCLASS, \w+, type = (0x[0-9a-f]+|\S+), "
                           r"offset = (\d+)", line)
            if bm:
                entries.append(["base", bm.group(1), int(bm.group(2)), ""])
                continue
            em = re.search(r"LF_ENUMERATE, \w+, value = ([^,]+), "
                           r"name = '([^']*)'", line)
            if em:
                entries.append(["enumerate", "", em.group(1).strip(),
                                em.group(2)])
        fieldlists[rid] = entries

    name_of = {}
    classes, enums = [], []
    for rid, (kind, body) in records.items():
        text_body = "\n".join(body)
        if kind == "LF_INTERFACE":
            cm = re.search(r"Size = (\d+), class name = (\S+)", text_body)
            fm = re.search(r"field list type (0x[0-9a-f]+)", text_body)
            if cm:
                name_of[rid] = cm.group(2).rstrip(",")
                classes.append({"id": rid, "name": name_of[rid],
                                "size": int(cm.group(1)),
                                "fieldlist": int(fm.group(1), 16)
                                if fm else None})
        elif kind == "LF_ENUM":
            em = re.search(r"enum name = (\S+)", text_body)
            fm = re.search(r"field list type (0x[0-9a-f]+)", text_body)
            if em:
                name_of[rid] = em.group(1).rstrip(",")
                enums.append({"name": name_of[rid],
                              "fieldlist": int(fm.group(1), 16)
                              if fm else None})

    def type_name(ref: str) -> str:
        if ref.startswith("0x"):
            return name_of.get(int(ref, 16), ref)
        return ref

    # per-TU compilation repeats type records; keep each name's FULLEST
    # definition (most members), never a forward declaration
    best_class = {}
    for c in classes:
        if not c["fieldlist"]:
            continue
        entries = fieldlists.get(c["fieldlist"], [])
        count = sum(1 for e in entries if e[0] == "member")
        prev = best_class.get(c["name"])
        if prev is None or count > prev[0]:
            best_class[c["name"]] = (count, c, entries)

    member_rows, class_rows = [], []
    for name in sorted(best_class):
        _count, c, entries = best_class[name]
        members = [e for e in entries if e[0] == "member"]
        statics = [e for e in entries if e[0] == "static"]
        bases = [type_name(e[1]) for e in entries if e[0] == "base"]
        class_rows.append((name, c["size"], len(members), len(statics),
                           ";".join(bases)))
        for e in members:
            member_rows.append((name, e[2], type_name(e[1]), e[3] or ""))

    best_enum = {}
    for e in enums:
        entries = fieldlists.get(e["fieldlist"], []) if e["fieldlist"] else []
        values = [x for x in entries if x[0] == "enumerate"]
        prev = best_enum.get(e["name"])
        if values and (prev is None or len(values) > len(prev)):
            best_enum[e["name"]] = values
    enum_rows = [(name, x[3], x[2])
                 for name in sorted(best_enum)
                 for x in best_enum[name]]

    # --- write everything --------------------------------------------
    def write(name, header, rows):
        with (OUT / name).open("w", newline="") as fh:
            fh.write("# Dreamcast CodeView corpus - DC offsets, reference "
                     "evidence (see README.md).\n")
            w = csv.writer(fh)
            w.writerow(header)
            w.writerows(rows)
        print(f"[dc extract] {name}: {len(rows)} rows")

    write("functions.csv",
          ["offset", "cb", "kind", "name", "module", "file", "line",
           "debug_start", "debug_end", "params", "locals"],
          [[f"0x{f['offset']:x}", f["cb"], f["kind"], f["name"],
            f["module"], f["file"], f["line"], f["debug_start"],
            f["debug_end"], f["params"], f["locals"]] for f in functions])
    write("variables.csv",
          ["proc", "module", "kind", "sp_offset", "type", "name"], variables)
    write("globals.csv",
          ["seg", "offset", "scope", "type", "name", "module"], globals_)
    write("publics.csv", ["seg", "offset", "name"], publics)
    write("constants.csv", ["name", "type", "value"], constants)
    write("classes.csv", ["name", "size", "members", "statics", "bases"],
          class_rows)
    write("members.csv", ["class", "offset", "type", "name"], member_rows)
    write("enums.csv", ["enum", "name", "value"], enum_rows)
    write("compile.csv", ["module", "processor", "version"],
          [[c["module"], c["processor"], c["version"]]
           for c in compile_rows])
    print(f"[dc extract] -> {OUT}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
