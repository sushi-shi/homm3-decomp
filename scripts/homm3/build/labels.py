#!/usr/bin/env python3
"""homm3.build.labels - derive build/gen/symbol_names.csv, the synth-PDB
inventory, from source annotations + the admitted config maps.

Schema (homm2 shape): rva,name,unit,size,kind,provenance - one row per
function in the executable plus the data rows the delinker needs. Sources,
in authority order:

  src-VA           `VA(0x<va>, 0x<size>)` macros scanned lexically from
                   src/ (include/va.h contract v2). The clang-IR path that
                   binds mangled names arrives with the first compiling
                   game TU; until then the working label at that rva
                   (evidence/retail-symbols.csv) names the function.
                   The full macro family - VA_COMPGEN, DATA,
                   DATA_COMPGEN(_GUARD), VTBL, VTBL2 - is scanned NOW
                   (lesson 1) even though src/ carries only VA today.
  zlib-map         config/retail-zlib-map.tsv, unit `zlib`.
  runtime-map      config/retail-runtime-map.tsv, empty unit - vostok
                   buckets these into _msvc_internal objects, which is
                   correct: runtime code is never a reconstruction target.
  working-label    every remaining function of the universe
                   (config/retail-functions.tsv), named by the naming
                   layer, bucketed `seg_%04x` gruntz-style.
  data rows        vtable starts (config/retail-vtables.tsv; `??_7X@@6B@`
                   where class-labeled at offset 0), IAT slots (parsed from
                   the import directory; `__imp_` spellings are PLACEHOLDER
                   undecorated names until import-lib proof), and every
                   absolute-relocation target in .rdata/.data
                   (config/retail-reloc-evidence.tsv) as
                   const_/data_/bss_<rva> - dense naming, required because
                   vostok panics on an .rdata target below every named
                   constant and skips targets outside known symbol sizes.

Gates (all fatal): VA addresses must be absolute (>= image base) and land
on a carved function entry; an annotation must be followed by a
declaration (the orphan-annotation incident from gruntz); no claim may
name a volatile `$E<n>` compiler ordinal; duplicate rvas and duplicate
names refuse to write.
"""
from __future__ import annotations

import csv
import re
import struct
import sys
from pathlib import Path

from homm3.carve import common

OUT = common.HOMM3_DIR / "build/gen/symbol_names.csv"
SRC_DIR = common.HOMM3_DIR / "src"
ZLIB_MAP = common.HOMM3_DIR / "config/retail-zlib-map.tsv"
RUNTIME_MAP = common.HOMM3_DIR / "config/retail-runtime-map.tsv"
FUNCTIONS = common.HOMM3_DIR / "config/retail-functions.tsv"
VTABLES = common.HOMM3_DIR / "config/retail-vtables.tsv"
RELOC_EVIDENCE = common.HOMM3_DIR / "config/retail-reloc-evidence.tsv"
SYMBOLS = common.EVIDENCE_DIR / "retail-symbols.csv"
VTABLE_SYMBOLS = common.EVIDENCE_DIR / "retail-vtable-symbols.csv"

IMAGE_BASE = 0x400000
BUCKET_SHIFT = 16

VA_RE = re.compile(r"^\s*VA\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*"
                   r"(0x[0-9a-fA-F]+|\d+)\s*\)")
VA_COMPGEN_RE = re.compile(
    r"^\s*VA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)"
    r"\s*,\s*(\w+)\s*,\s*(\w+)\s*\)")
DATA_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)\s*\)")
DATA_COMPGEN_RE = re.compile(
    r"\bDATA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(\w+)\s*,")
DATA_COMPGEN_GUARD_RE = re.compile(
    r"\bDATA_COMPGEN_GUARD\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(\w+)\s*,"
    r"\s*(\w+)\s*\)")
VTBL_RE = re.compile(r"^\s*VTBL\s*\(\s*(\w+)\s*,\s*(0x[0-9a-fA-F]+)\s*\)")
VTBL2_RE = re.compile(r"^\s*VTBL2\s*\(\s*(\w+)\s*,\s*(\w+)\s*,"
                      r"\s*(0x[0-9a-fA-F]+)\s*\)")
ANNOTATION_RE = re.compile(r"^\s*(?:VA|VA_COMPGEN|DATA|DC_ONLY)\s*\(")
VOLATILE_E_RE = re.compile(r"^_?\$E[0-9]+$")
COMPGEN_KINDS = {"STATIC_INIT_DISPATCH", "STATIC_ATEXIT", "STATIC_DTOR",
                 "STATIC_CTOR"}


def mask_lexical_noise(blob: str) -> str:
    """Blank comments and string/char literals byte-for-byte (newlines
    kept) so the macro regexes can never fire inside them. Ported from
    homm2's annotated_data._mask_lexical_noise."""
    out = list(blob)
    i, n = 0, len(blob)
    state = None  # None | "line" | "block" | '"' | "'"
    while i < n:
        c = blob[i]
        if state is None:
            if c == "/" and i + 1 < n and blob[i + 1] == "/":
                state = "line"
                out[i] = out[i + 1] = " "
                i += 2
                continue
            if c == "/" and i + 1 < n and blob[i + 1] == "*":
                state = "block"
                out[i] = out[i + 1] = " "
                i += 2
                continue
            if c == '"':
                state = c
                i += 1
                continue
            if c == "'":
                # enter char-literal state only when it closes nearby: the
                # carcass carries MSVC `scalar deleting destructor' names
                # whose lone apostrophe would otherwise swallow the file
                closer = blob.find("'", i + 1, i + 5)
                if closer != -1 and "\n" not in blob[i:closer]:
                    state = c
                i += 1
                continue
            i += 1
            continue
        if state == "line":
            if c == "\n":
                state = None
            else:
                out[i] = " "
            i += 1
            continue
        if state == "block":
            if c == "*" and i + 1 < n and blob[i + 1] == "/":
                out[i] = out[i + 1] = " "
                state = None
                i += 2
                continue
            if c != "\n":
                out[i] = " "
            i += 1
            continue
        # string/char literal
        if c == "\\" and i + 1 < n:
            out[i] = out[i + 1] = " "
            i += 2
            continue
        if c == state:
            state = None
        elif c != "\n":
            out[i] = " "
        i += 1
    return "".join(out)


def read_tsv_body(path):
    rows = [line.rstrip("\n").split("\t") for line in path.open()
            if not line.startswith("#")]
    return rows[0], rows[1:]


def load_csv(path):
    with path.open() as fh:
        return list(csv.DictReader(
            line for line in fh if not line.startswith("#")))


def rva_of(addr_text: str, where: str) -> int:
    value = int(addr_text, 16)
    if value < IMAGE_BASE:
        common.die(f"{where}: address {addr_text} below image base - the "
                   "v2 contract uses ABSOLUTE VAs")
    return value - IMAGE_BASE


def scan_sources(functions):
    """All annotation rows from src/ (lexical pass)."""
    rows = []
    for path in sorted(SRC_DIR.glob("*.c*")):
        unit = path.stem
        text = mask_lexical_noise(path.read_text(errors="replace"))
        lines = text.splitlines()
        for index, line in enumerate(lines):
            where = f"{path.name}:{index + 1}"
            m = VA_RE.match(line)
            if m:
                rva = rva_of(m.group(1), where)
                if rva not in functions:
                    common.die(f"{where}: VA {m.group(1)} is not a carved "
                               "function entry")
                declared = int(m.group(2), 0)
                follower = next((l for l in lines[index + 1:index + 4]
                                 if l.strip()
                                 and not ANNOTATION_RE.match(l)), None)
                if follower is None:
                    common.die(f"{where}: orphan VA annotation - no "
                               "declaration follows")
                rows.append({"rva": rva, "unit": unit, "size": declared,
                             "kind": "func", "provenance": "src-VA"})
                continue
            m = VA_COMPGEN_RE.match(line)
            if m:
                rva = rva_of(m.group(1), where)
                if m.group(3) not in COMPGEN_KINDS:
                    common.die(f"{where}: unknown VA_COMPGEN kind "
                               f"{m.group(3)}")
                name = f"__h3cg${unit}${m.group(3).lower()}${m.group(4)}"
                rows.append({"rva": rva, "unit": unit,
                             "size": int(m.group(2), 0), "kind": "func",
                             "name": name,
                             "provenance": "src-VA_COMPGEN"})
                continue
            m = VTBL_RE.match(line) or VTBL2_RE.match(line)
            if m:
                if len(m.groups()) == 2:
                    cls, addr = m.groups()
                    name = f"??_7{cls}@@6B@"
                else:
                    derived, base, addr = m.groups()
                    name = f"??_7{derived}@@6B{base}@@@"
                rows.append({"rva": rva_of(addr, where), "unit": unit,
                             "size": "", "kind": "data", "name": name,
                             "provenance": "src-VTBL"})
                continue
            for m in DATA_COMPGEN_GUARD_RE.finditer(line):
                name = f"__h3cg${unit}$static_init_guard${m.group(2)}"
                rows.append({"rva": rva_of(m.group(1), where),
                             "unit": unit, "size": 4, "kind": "data",
                             "name": name,
                             "provenance": "src-DATA_COMPGEN_GUARD"})
            for m in DATA_COMPGEN_RE.finditer(line):
                if DATA_COMPGEN_GUARD_RE.search(line):
                    continue
                name = f"__h3cg${unit}$data${m.group(2)}"
                rows.append({"rva": rva_of(m.group(1), where),
                             "unit": unit, "size": "", "kind": "data",
                             "name": name,
                             "provenance": "src-DATA_COMPGEN"})
            for m in DATA_RE.finditer(line):
                rows.append({"rva": rva_of(m.group(1), where),
                             "unit": unit, "size": "", "kind": "data",
                             "name": f"data_{rva_of(m.group(1), where):x}",
                             "provenance": "src-DATA"})
    return rows


def iat_slots(exe_path: Path):
    """slot rva -> __imp_ spelling, from the import directory."""
    data = exe_path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    osz = struct.unpack_from("<H", data, pe + 20)[0]
    sections = []
    for i in range(nsec):
        off = pe + 24 + osz + i * 40
        vs, va, rs, ro = struct.unpack_from("<4I", data, off + 8)
        sections.append((va, max(vs, rs), ro))

    def raw(rva):
        for va, span, ro in sections:
            if va <= rva < va + span:
                return ro + (rva - va)
        common.die(f"import walk: rva 0x{rva:x} in no section")

    imp_rva = struct.unpack_from("<II", data, pe + 24 + 96 + 1 * 8)[0]
    slots = {}
    off = raw(imp_rva)
    while True:
        ilt, _ts, _fc, name_rva, iat = struct.unpack_from("<IIIII",
                                                          data, off)
        if not (ilt or name_rva):
            break
        dll = data[raw(name_rva):raw(name_rva) + 64].split(b"\0")[0] \
            .decode("latin-1")
        entry = raw(ilt)
        index = 0
        while True:
            thunk = struct.unpack_from("<I", data, entry + index * 4)[0]
            if not thunk:
                break
            slot = iat + index * 4
            if thunk & 0x80000000:
                stem = dll.rsplit(".", 1)[0].lower()
                slots[slot] = (f"__imp__{stem}_ordinal_{thunk & 0xFFFF}",
                               "iat-ordinal")
            else:
                name = data[raw(thunk) + 2:raw(thunk) + 2 + 256] \
                    .split(b"\0")[0].decode("latin-1")
                prefix = "__imp_" if name.startswith("?") else "__imp__"
                slots[slot] = (prefix + name, "iat-undecorated")
            index += 1
        off += 20
    return slots


def main(argv=None) -> int:
    _header, fn_rows = read_tsv_body(FUNCTIONS)
    functions = {int(r[0], 16): int(r[1]) for r in fn_rows}
    labels = {int(r["rva"], 16): r["name"] for r in load_csv(SYMBOLS)}

    rows = {}       # rva -> row dict (first writer wins per authority order)
    problems = []

    def put(rva, name, unit, size, kind, provenance):
        if VOLATILE_E_RE.match(name):
            common.die(f"0x{rva:x}: claim names volatile compiler ordinal "
                       f"{name!r} - record it as evidence, never a label")
        if rva in rows:
            problems.append(f"duplicate rva 0x{rva:x}: "
                            f"{rows[rva]['name']} vs {name}")
            return
        rows[rva] = {"rva": rva, "name": name, "unit": unit,
                     "size": size, "kind": kind, "provenance": provenance}

    # 1. src annotations
    src_rows = scan_sources(functions)
    for r in src_rows:
        if r["kind"] == "func" and "name" not in r:
            if r["rva"] not in labels:
                common.die(f"VA 0x{r['rva']:x}: no working label - naming "
                           "layer out of date")
            r["name"] = labels[r["rva"]]
        put(r["rva"], r["name"], r["unit"], r["size"], r["kind"],
            r["provenance"])

    # 2. zlib map - per-MEMBER units so the delinked objects pair 1:1
    # against our compiled base objs (inflate.c.obj vs base/inflate.obj).
    # Member attribution from the DNA pass; the few unattributed stragglers
    # inherit the nearest preceding member (contributions are contiguous
    # in link order).
    member_of = {}
    for line in (common.EVIDENCE_DIR /
                 "retail-function-libraries.tsv").open():
        if not line.startswith("0x"):
            continue
        cells = line.rstrip("\n").split("\t")
        if cells[1] == "zlib" and cells[3].endswith(".obj"):
            member_of[int(cells[0], 16)] = cells[3][:-4]
    _h, zlib_rows = read_tsv_body(ZLIB_MAP)
    ordered = sorted(zlib_rows, key=lambda r: int(r[0], 16))
    members = [member_of.get(int(r[0], 16)) for r in ordered]
    for i in range(1, len(members)):          # forward fill
        members[i] = members[i] or members[i - 1]
    for i in range(len(members) - 2, -1, -1):  # backward fill the head
        members[i] = members[i] or members[i + 1]
    if not any(members):
        common.die("zlib map: no member attribution at all")
    for (rva_text, size, name), member in zip(ordered, members):
        put(int(rva_text, 16), name, member, int(size), "func", "zlib-map")

    # 3. runtime map (sizes from the universe)
    _h, runtime_rows = read_tsv_body(RUNTIME_MAP)
    for rva_text, name in runtime_rows:
        rva = int(rva_text, 16)
        put(rva, name, "", functions[rva], "func", "runtime-map")

    # 4. the rest of the universe, working labels, seg buckets
    for rva, size in sorted(functions.items()):
        if rva in rows:
            continue
        put(rva, labels[rva], f"seg_{rva >> BUCKET_SHIFT:04x}", size,
            "func", "working-label")

    # 5. data rows -------------------------------------------------------
    image, info = common.load_image()
    secmap = {s.name: s for s in image.sections}
    rdata, dat = secmap[".rdata"], secmap[".data"]

    vt_class = {}
    for r in load_csv(VTABLE_SYMBOLS):
        if r["class"] and r["class_addr_offset"] == "0":
            vt_class.setdefault(int(r["vtable_rva"], 16), r["class"])
    _h, vt_rows = read_tsv_body(VTABLES)
    for rva_text, count in vt_rows:
        rva = int(rva_text, 16)
        cls = vt_class.get(rva)
        name = f"??_7{cls}@@6B@" if cls else f"vtbl_{rva:x}"
        put(rva, name, "", int(count) * 4, "data",
            "vtable-class" if cls else "vtable")

    for slot, (name, provenance) in sorted(iat_slots(
            Path(info["path"])).items()):
        put(slot, name, "", 4, "data", provenance)

    _h, ev_rows = read_tsv_body(RELOC_EVIDENCE)
    col = {c: i for i, c in enumerate(_h)}
    skipped_targets = 0
    for r in ev_rows:
        if r[col["target_class"]] not in ("data", "literal-start",
                                          "literal-interior"):
            continue
        target = int(r[col["value"]], 16) - IMAGE_BASE
        if target in rows:
            continue
        if rdata.rva <= target < rdata.rva + rdata.mapped:
            put(target, f"const_{target:x}", "", "", "data", "reloc-target")
        elif dat.rva <= target < dat.rva + dat.size:
            put(target, f"data_{target:x}", "", "", "data", "reloc-target")
        elif dat.rva + dat.size <= target < dat.rva + dat.mapped:
            put(target, f"bss_{target:x}", "", "", "data", "reloc-target")
        else:
            skipped_targets += 1

    if problems:
        for p in problems[:10]:
            print(f"[build labels] {p}", file=sys.stderr)
        common.die(f"{len(problems)} duplicate-rva claims")
    names = [r["name"] for r in rows.values()]
    if len(set(names)) != len(names):
        from collections import Counter
        dup = [n for n, c in Counter(names).items() if c > 1][:5]
        common.die(f"duplicate names, first {dup}")
    missing = set(functions) - set(rows)
    if missing:
        common.die(f"{len(missing)} functions uncovered - first "
                   f"0x{min(missing):x}")

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with OUT.open("w", newline="") as fh:
        fh.write("# GENERATED: python3 -m homm3.build.labels - the "
                 "synth-PDB inventory.\n")
        for prov in common.provenance("homm3.build.labels"):
            fh.write(prov + "\n")
        writer = csv.writer(fh)
        writer.writerow(["rva", "name", "unit", "size", "kind",
                         "provenance"])
        for rva in sorted(rows):
            r = rows[rva]
            size = (f"0x{r['size']:x}" if isinstance(r["size"], int)
                    else r["size"])
            writer.writerow([f"0x{rva:x}", r["name"], r["unit"], size,
                             r["kind"], r["provenance"]])

    funcs = sum(1 for r in rows.values() if r["kind"] == "func")
    data = len(rows) - funcs
    by_prov = {}
    for r in rows.values():
        by_prov[r["provenance"]] = by_prov.get(r["provenance"], 0) + 1
    print(f"[build labels] {len(rows)} rows ({funcs} func, {data} data) "
          f"-> {OUT}")
    for prov in sorted(by_prov, key=by_prov.get, reverse=True):
        print(f"  {prov}: {by_prov[prov]}")
    if skipped_targets:
        print(f"  reloc targets outside .rdata/.data: {skipped_targets} "
              "skipped")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
