#!/usr/bin/env python3
"""homm3.carve.fixture_full - the `carve fixture --full` mini-EXE gate.

Links switch_fixture.obj into a real PE with the real VC6 linker, then runs
the carve CORE over that image: Ghidra body carve -> vendored reloc channels
-> tables.attribute -> extents.synthesize. Asserts, per switch function, that
Ghidra's body CLIPS the COFF extent and the synthesis RESTORES it exactly.
Required once and after any attribution change (slow-ish: wine link + a small
Ghidra project).

The .map plays the role S2's structural seeds play on retail: it supplies the
ROOTS (three of the four functions are unreferenced; rooted analysis would
never reach them). Ghidra still derives the bodies itself - the bodies are
what the assertion is about.
"""
from __future__ import annotations

import shutil
import struct
import sys

from homm3.carve import common, extents, fixture, tables
from homm3.carve.find_relocs import Image, code_sites

FIX_DIR = common.CARVE_DIR / "fixture"
EXE = FIX_DIR / "switch_fixture.exe"
MAP = FIX_DIR / "switch_fixture.map"
WORK_EXE = FIX_DIR / "switch_fixture.objdump.exe"
PROJ_DIR = FIX_DIR / "ghidra"


def link_mini_exe(obj) -> None:
    from homm3.build import link as build_link
    rc = build_link.main(["--out", str(EXE), "--map", str(MAP),
                          "--obj", str(obj), "--entry", "dense_trailing"])
    if rc or not EXE.is_file():
        common.die("mini-EXE link failed - run inside `nix develop .#build`")


def map_symbols() -> dict:
    """symbol -> rva from the VC6 .map (linker truth = the fixture's roots)."""
    rvas = {}
    base = None
    for line in MAP.read_text(errors="replace").splitlines():
        parts = line.split()
        if "Preferred" in line and "load" in line:
            base = int(parts[-1], 16)
        if len(parts) >= 4 and parts[1].startswith("_") and parts[3] == "f":
            rvas[parts[1]] = int(parts[2], 16) - (base or 0x400000)
    return rvas


def ghidra_bodies(roots) -> list:
    """Import + analyze the mini-EXE, seed the map roots, export body rows."""
    if PROJ_DIR.exists():
        shutil.rmtree(PROJ_DIR)
    PROJ_DIR.mkdir(parents=True)
    import pyghidra
    pyghidra.start()
    from pyghidra.core import _setup_project, _analyze_program
    from ghidra.program.flatapi import FlatProgramAPI
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd

    gproject, program = _setup_project(
        binary_path=str(EXE), project_location=str(PROJ_DIR),
        project_name="fixture", nested_project_location=False)
    rows = []
    try:
        flat = FlatProgramAPI(program)
        _analyze_program(flat, program)
        base = program.getImageBase().getOffset()
        space = program.getAddressFactory().getDefaultAddressSpace()
        fm = program.getFunctionManager()
        tx = program.startTransaction("seed-map-roots")
        try:
            for rva in roots:
                addr = space.getAddress(base + rva)
                # unreferenced functions were never disassembled; a function
                # created over virgin bytes gets a 1-byte body
                DisassembleCommand(addr, None, True).applyTo(program)
                existing = fm.getFunctionAt(addr)
                if existing is not None:
                    fm.removeFunction(addr)  # recompute the body from flow
                CreateFunctionCmd(addr).applyTo(program)
            flat.analyzeChanges(program)
        finally:
            program.endTransaction(tx, True)
        for fn in fm.getFunctions(True):
            block = program.getMemory().getBlock(fn.getEntryPoint())
            if block is None or not block.isExecute():
                continue
            ranges = []
            iterator = fn.getBody().getAddressRanges(True)
            while iterator.hasNext():
                rng = iterator.next()
                ranges.append("0x%x-0x%x" % (
                    rng.getMinAddress().getOffset() - base,
                    rng.getMaxAddress().getOffset() - base + 1))
            rows.append({"entry_rva": "0x%x" % (fn.getEntryPoint().getOffset()
                                                - base),
                         "body_size": str(fn.getBody().getNumAddresses()),
                         "chunks": str(len(ranges)),
                         "body_ranges": ";".join(ranges),
                         "name": fn.getName()})
    finally:
        gproject.close()
    return rows


def reloc_rows(image) -> list:
    """The S1 code/switch channels over the mini image, as TSV-shaped rows."""
    rows = []
    for section in image.sections:
        if not section.executable:
            continue
        for rva, site in sorted(code_sites(image, section).items()):
            rows.append({"site_rva": f"0x{rva:x}",
                         "value": f"0x{site.target:x}",
                         "channel": site.channel, "detail": site.detail,
                         "target_class": "code", "ctx": "-"})
    return rows


def run(obj) -> int:
    coff = fixture.Coff(obj)
    facts = fixture.coff_truth(coff)

    link_mini_exe(obj)
    symbols = map_symbols()
    wanted = {name: symbols[name] for name in fixture.EXPECT if name in symbols}
    if len(wanted) != len(fixture.EXPECT):
        common.die(f"map lists {sorted(wanted)} - expected "
                   f"{sorted(fixture.EXPECT)}")

    body_rows = ghidra_bodies(sorted(wanted.values()))

    # llvm-objdump may refuse a header-resident load config; zero it if set
    data = bytearray(EXE.read_bytes())
    pe = int.from_bytes(data[0x3C:0x40], "little")
    entry = pe + 24 + 96 + 10 * 8
    data[entry:entry + 8] = b"\0" * 8
    WORK_EXE.write_bytes(bytes(data))
    image = Image(str(WORK_EXE))

    bodies = tables.Bodies(body_rows)
    attributed = tables.attribute(image, reloc_rows(image), bodies)

    funcs = {}
    for row in body_rows:
        entry_rva = int(row["entry_rva"], 16)
        funcs[entry_rva] = {
            "entry": entry_rva, "body_size": int(row["body_size"]),
            "ranges": [tuple(int(x, 16) for x in c.split("-"))
                       for c in row["body_ranges"].split(";")],
            "name": row["name"], "flags": [], "tables": []}
    for t in attributed:
        funcs[t["owner"]]["tables"].append((t["rva"], t["end"]))
    rows, overlaps = extents.synthesize(funcs)
    if overlaps:
        common.die(f"mini-EXE synthesis produced {len(overlaps)} overlaps")
    size_of = {entry: size for entry, size, _fn in rows}

    for name, rva in sorted(wanted.items(), key=lambda kv: kv[1]):
        truth = facts[name]["extent"]
        body = max(hi for _lo, hi in funcs[rva]["ranges"]) - rva
        synth = size_of[rva]
        has_tables = bool(facts[name]["tables"])
        print(f"[fixture --full] {name}: ghidra body {body}, synthesized "
              f"{synth}, COFF truth {truth}")
        if has_tables and body >= truth:
            common.die(f"{name}: Ghidra body {body} not clipped below COFF "
                       f"extent {truth} - the premise no longer holds on a "
                       "linked image, re-review the pipeline")
        if synth != truth:
            common.die(f"{name}: synthesized extent {synth} != COFF truth "
                       f"{truth} - FULL ROUND-TRIP FAILED")
    print("[fixture --full] mini-EXE pipeline restores every COFF extent")
    return 0


if __name__ == "__main__":
    sys.exit(run(common.CARVE_DIR / "fixture/switch_fixture.obj"))
