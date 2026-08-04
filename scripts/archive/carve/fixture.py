#!/usr/bin/env python3
"""homm3.carve.fixture - the compiler settles the size contract, not argument.

Compiles fixture/switch_fixture.c with the real VC6 SP3 cl (/O2 /Gy /c /TC,
through homm3.core.cc_wrap) and treats the COFF object as ground truth: with
/Gy every function is its own COMDAT section, and its SizeOfRawData is the
contribution the linker will place. The assertions pin, empirically:

  * switch jump tables (dword) AND byte case-maps live INSIDE the function's
    COMDAT, trailing the code - so a code-only carve is short (the premise);
  * the contribution is code + tables + trailing 0x90 alignment NOPs, i.e.
    extent-with-tables == section size minus trailing padding;
  * tables.py's pure walkers, run over the reloc-applied section bytes,
    recover exactly that extent (the round-trip);
  * a deliberately body-only size FAILS the truth check (negative control -
    gates must be able to fail).

Layout measured on the pinned toolchain (VC6 SP3 cl 12.00.8168):
    [code][00-align][dword table(s)][byte case-map][90 x pad-to-16]

`--full` additionally links the object into a mini-EXE with the real VC6
linker and runs the carve core (Ghidra body export -> table attribution ->
extent synthesis) over it, asserting Ghidra clips and the synthesis restores
the COFF extents on a real image.
"""
from __future__ import annotations

import struct
import subprocess
import sys
from pathlib import Path

from homm3.carve import common

SRC = Path(__file__).resolve().parent / "fixture/switch_fixture.c"
FIX_DIR = common.CARVE_DIR / "fixture"
OBJ = FIX_DIR / "switch_fixture.obj"

DIR32 = 6

# COFF truth for the four fixture functions: (dispatches, dword tables,
# byte case-maps) each function must exhibit. Extents are derived, not pinned,
# so the fixture survives compiler codegen jitter.
EXPECT = {
    "_dense_trailing": (1, 1, 0),
    "_double_switch": (2, 2, 0),
    "_sparse_casemap": (1, 1, 1),
    "_no_switch_control": (0, 0, 0),
}


class Coff:
    """The parts of a VC6 object this package reasons about (stdlib only).

    Accepts a path or raw bytes - dna.py parses archive members in memory."""

    def __init__(self, source):
        data = source if isinstance(source, bytes) else Path(source).read_bytes()
        self.data = data
        _machine, nsec, _, symptr, nsyms, optsz, _ = struct.unpack_from(
            "<2H3I2H", data, 0)
        strtab = data[symptr + nsyms * 18:]

        def name_of(raw: bytes) -> str:
            if raw[:4] == b"\0\0\0\0":
                off = struct.unpack_from("<I", raw, 4)[0]
                return strtab[off:strtab.index(b"\0", off)].decode()
            return raw.rstrip(b"\0").decode()

        # symbols keyed by RAW table index (reloc SymbolTableIndex counts aux
        # entries), value = (name, value, section_number, type, storage_class)
        self.symbols = {}
        index = 0
        while index < nsyms:
            raw = data[symptr + index * 18: symptr + (index + 1) * 18]
            value, secnum, typ = struct.unpack_from("<IhH", raw, 8)
            self.symbols[index] = (name_of(raw[:8]), value, secnum, typ, raw[16])
            index += 1 + raw[17]

        # sections: (name, blob, relocs [(va, symidx, type)])
        self.sections = []
        for i in range(nsec):
            off = 20 + optsz + i * 40
            name = data[off:off + 8].rstrip(b"\0").decode()
            rawsize, rawptr, relptr = struct.unpack_from("<3I", data, off + 16)
            nrel = struct.unpack_from("<H", data, off + 32)[0]
            relocs = [struct.unpack_from("<2IH", data, relptr + j * 10)
                      for j in range(nrel)]
            self.sections.append((name, data[rawptr:rawptr + rawsize], relocs))

    def functions(self) -> dict:
        """External function symbols -> their COMDAT section index (0-based)."""
        return {name: secnum - 1
                for name, value, secnum, typ, sclass in self.symbols.values()
                if sclass == 2 and typ == 0x20 and secnum > 0}


def compile_fixture() -> Path:
    """Through the real cc_wrap path; refuses stale objects."""
    if OBJ.is_file() and OBJ.stat().st_mtime >= SRC.stat().st_mtime:
        return OBJ
    cmd = [sys.executable, "-m", "homm3.core.cc_wrap", "--out", str(OBJ),
           "--src", str(SRC), "--", "/nologo", "/O2", "/Gy", "/c", "/TC"]
    proc = subprocess.run(cmd, cwd=str(common.HOMM3_DIR))
    if proc.returncode or not OBJ.is_file():
        common.die("fixture compile failed - run inside `nix develop .#build` "
                   "(wine + VC6 required)")
    return OBJ


def _reloc_target(coff: Coff, blob: bytes, va: int, symidx: int) -> int:
    """COFF semantics: field value = symbol value + stored addend."""
    return coff.symbols[symidx][1] + struct.unpack_from("<I", blob, va)[0]


def analyze(coff: Coff, section_index: int) -> dict:
    """Structural facts for one COMDAT: dispatches, tables, case-maps, extent."""
    _name, blob, relocs = coff.sections[section_index]
    size = len(blob)
    dir32 = sorted((va, symidx) for va, symidx, typ in relocs if typ == DIR32)

    dispatches, casemap_refs = [], []
    for va, symidx in dir32:
        if (va >= 3 and blob[va - 3] == 0xFF and blob[va - 2] == 0x24
                and (blob[va - 1] & 0xC7) == 0x85):
            dispatches.append((va, _reloc_target(coff, blob, va, symidx)))
        elif (va >= 2 and (blob[va - 1] & 0xC0) == 0x80
              and (blob[va - 2] == 0x8A
                   or (va >= 3 and blob[va - 3] == 0x0F
                       and blob[va - 2] in (0xB6, 0xB7)))):
            # mov r8,[r32+disp32] / movzx - the byte case-map load
            casemap_refs.append((va, _reloc_target(coff, blob, va, symidx)))

    reloc_vas = {va for va, _ in dir32}
    bases = {base for _va, base in dispatches}
    tables = []
    for _va, base in sorted(dispatches, key=lambda d: d[1]):
        # dword entries are exactly the consecutive relocated slots from
        # base, stopping where the next dispatch's table begins (VC6 packs
        # a function's tables back-to-back)
        end = base
        while end + 4 <= size and end in reloc_vas and not (
                end != base and end in bases):
            end += 4
        tables.append((base, end, (end - base) // 4))

    casemaps = []
    for _va, base in sorted(casemap_refs, key=lambda c: c[1]):
        end = size
        while end > base and blob[end - 1] == 0x90:
            end -= 1
        casemaps.append((base, end))

    covered_ends = [end for _b, end, _n in tables] + [e for _b, e in casemaps]
    extent = max(covered_ends) if covered_ends else None
    if extent is None:
        code_end = size
        while code_end and blob[code_end - 1] == 0x90:
            code_end -= 1
        extent = code_end

    # body-only end: last code byte before the first table's 00-alignment gap
    body_end = extent
    if tables:
        body_end = min(base for base, _e, _n in tables)
        while body_end and blob[body_end - 1] == 0x00:
            body_end -= 1

    return {"size": size, "dispatches": dispatches, "tables": tables,
            "casemaps": casemaps, "extent": extent, "body_end": body_end}


def coff_truth(coff: Coff) -> dict:
    """The premise assertions. Raises through common.die on any violation."""
    facts = {}
    functions = coff.functions()
    for name, expect in EXPECT.items():
        if name not in functions:
            common.die(f"fixture symbol missing from object: {name}")
        f = analyze(coff, functions[name])
        n_disp, n_tab, n_map = expect
        if (len(f["dispatches"]), len(f["tables"]), len(f["casemaps"])) != expect:
            common.die(f"{name}: expected {n_disp} dispatches/{n_tab} tables/"
                       f"{n_map} case-maps, found {len(f['dispatches'])}/"
                       f"{len(f['tables'])}/{len(f['casemaps'])} - "
                       "codegen changed shape, refit the fixture")
        for base, end, entries in f["tables"]:
            if not (f["body_end"] <= base and end <= f["size"]):
                common.die(f"{name}: dword table [{base},{end}) escapes the "
                           f"COMDAT contribution (size {f['size']}) - "
                           "SIZE CONTRACT PREMISE VIOLATED")
        for base, end in f["casemaps"]:
            if not (f["body_end"] <= base and end <= f["size"]):
                common.die(f"{name}: byte case-map [{base},{end}) escapes the "
                           "COMDAT contribution - PREMISE VIOLATED")
        if (n_tab or n_map) and f["extent"] <= f["body_end"]:
            common.die(f"{name}: tables do not extend past the code "
                       f"(extent {f['extent']} <= body {f['body_end']}) - "
                       "SIZE CONTRACT PREMISE VIOLATED")
        pad = f["size"] - f["extent"]
        if pad and set(coff.sections[functions[name]][1][f["extent"]:]) != {0x90}:
            common.die(f"{name}: bytes after extent {f['extent']} are not "
                       "0x90 alignment padding - extent derivation is wrong")
        facts[name] = f
        print(f"[fixture] {name}: section {f['size']:4d} B = body {f['body_end']:3d}"
              f" + tables {[(b, e) for b, e, _ in f['tables']]}"
              f"{' + case-map ' + str(f['casemaps']) if f['casemaps'] else ''}"
              f" + pad {pad}")
    return facts


def negative_control(facts: dict) -> None:
    """Gates must be able to fail: a body-only size must be rejected."""
    for name, f in facts.items():
        if not f["tables"]:
            continue
        claimed = f["body_end"]  # the deliberately clipped, Ghidra-style size
        if claimed == f["extent"]:
            common.die(f"{name}: negative control is vacuous - body-only size "
                       "equals the true extent, the gate cannot fail")
    print("[fixture] negative control: body-only sizes are rejected for "
          f"{sum(1 for f in facts.values() if f['tables'])} table-owning "
          "functions (gate can fail)")


def linked_blob(coff: Coff, section_index: int) -> bytes:
    """Apply every DIR32 in-section: the section becomes a mini flattened
    image based at 0, which is what the retail-facing walkers consume."""
    name, blob, relocs = coff.sections[section_index]
    out = bytearray(blob)
    for va, symidx, typ in relocs:
        if typ == DIR32:
            struct.pack_into("<I", out, va,
                             _reloc_target(coff, out, va, symidx))
    return bytes(out)


def round_trip(coff: Coff, facts: dict) -> None:
    """tables.py over the linked bytes must recover exactly the COFF extents."""
    from homm3.carve import tables

    functions = coff.functions()
    for name, f in facts.items():
        if not f["tables"] and not f["casemaps"]:
            continue
        blob = linked_blob(coff, functions[name])
        size = len(blob)
        known = {base for base, _e, _n in f["tables"]}
        recovered = f["body_end"]
        for base, true_end, true_entries in f["tables"]:
            end, entries = tables.walk_dword_table(
                blob, base, lambda v: 0 <= v < size, known, size)
            if (end, entries) != (true_end, true_entries):
                common.die(f"{name}: walk_dword_table([{base}...) -> "
                           f"({end},{entries}), COFF truth ({true_end},"
                           f"{true_entries}) - ROUND-TRIP FAILED")
            recovered = max(recovered, end)
        for base, true_end in f["casemaps"]:
            end = tables.byte_table_extent(blob, base, size)
            if end != true_end:
                common.die(f"{name}: byte_table_extent({base}) -> {end}, "
                           f"COFF truth {true_end} - ROUND-TRIP FAILED")
            recovered = max(recovered, end)
        if recovered != f["extent"]:
            common.die(f"{name}: synthesized extent {recovered} != COFF "
                       f"extent {f['extent']} - ROUND-TRIP FAILED")
        print(f"[fixture] {name}: round-trip recovered extent {recovered} "
              f"== COFF truth (body-only would claim {f['body_end']})")


def main(argv=None) -> int:
    argv = list(argv or [])
    obj = compile_fixture()
    coff = Coff(obj)
    facts = coff_truth(coff)
    negative_control(facts)
    if "--coff-only" in argv:
        print("[fixture] COFF truth + negative control OK (--coff-only)")
        return 0
    round_trip(coff, facts)
    if "--full" in argv:
        from homm3.carve import fixture_full
        return fixture_full.run(obj)
    print("[fixture] all assertions green")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
