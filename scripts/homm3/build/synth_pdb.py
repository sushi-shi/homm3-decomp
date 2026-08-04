#!/usr/bin/env python3
"""homm3.build.synth_pdb - fabricate the PDB vostok consumes (P2.2).

No PDB ever existed for HEROES3.EXE (no CodeView stream either); vostok
wants a modern PDB 7.0. Built here from build/gen/symbol_names.csv:

    symbol_names.csv
        -> PDB-YAML: one DBI module per unit (`c:\\proj\\<unit>.c` - the
           path vostok strips against --engine-path to decide the output
           object, so the delink mirrors src/); per-function !Lines blocks
           carry TU ownership; S_GPROC32 gives every function's boundary,
           S_LDATA32 names every datum/IAT slot/reloc target (dense .rdata
           naming dodges vostok's all-constants-must-be-named panic).
           Runtime-map functions sit in a module WITHOUT line info, so
           vostok buckets them into _msvc_internal objects - correct,
           library code is not a reconstruction target.
        -> `llvm-pdbutil yaml2pdb`
        -> byte-patch DBIHeader.symbol_records_stream (offset 0x14 of the
           DBI stream's first block) to an existing EMPTY stream: yaml2pdb
           writes the nil index 0xFFFF there and vostok's pdb2 crate
           refuses to open such a PDB.

Structure ported from vostok's own examples/04-synthetic-pdb (at the
pinned rev 1393e24) and gruntz/homm2 synth_pdb; deterministic output
(zero GUID, no timestamps).
"""
from __future__ import annotations

import csv
import hashlib
import re
import struct
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

from homm3.carve import common

INVENTORY = common.HOMM3_DIR / "build/gen/symbol_names.csv"
OUT = common.HOMM3_DIR / "build/pdb/HEROES3.pdb"
ENGINE_PREFIX = "c:\\proj\\"
DATA_UNIT = "_data"
RUNTIME_UNIT = "_runtime"


def read_sections(exe: Path):
    """[(name, 1-based segment, virtual start rva, virtual end rva)]."""
    data = exe.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    osz = struct.unpack_from("<H", data, pe + 20)[0]
    out = []
    for i in range(nsec):
        off = pe + 24 + osz + i * 40
        name = data[off:off + 8].rstrip(b"\0").decode("latin-1")
        vs, va, rs, _ro = struct.unpack_from("<4I", data, off + 8)
        out.append((name, i + 1, va, va + max(vs, rs)))
    return out


def q(name: str) -> str:
    return "'" + name.replace("'", "''") + "'"


def module_yaml(unit: str, funcs, data, with_lines: bool):
    """One DBI module. funcs: [(seg, off, size, name)]; data: [(seg, off,
    name)]."""
    source = f"{ENGINE_PREFIX}{unit}.c"
    md5 = hashlib.md5(source.encode()).hexdigest().upper()
    out = [
        f"    - Module:          {q(source[:-2])}",
        f"      ObjFile:         {q(source[:-2])}",
        "      SourceFiles:",
        f"        - {q(source)}",
        "      Subsections:",
        "        - !FileChecksums",
        "          Checksums:",
        f"            - FileName:        {q(source)}",
        "              Kind:            MD5",
        f"              Checksum:        {md5}",
    ]
    if with_lines:
        for seg, off, size, _name in funcs:
            out += [
                "        - !Lines",
                f"          CodeSize:        {size}",
                "          Flags:           [  ]",
                f"          RelocOffset:     {off}",
                f"          RelocSegment:    {seg}",
                "          Blocks:",
                f"            - FileName:        {q(source)}",
                "              Lines:",
                "                - Offset:          0",
                "                  LineStart:       1",
                "                  EndDelta:        0",
                "                  IsStatement:     true",
                "              Columns:         []",
            ]
    out += ["      Modi:", "        Records:"]
    for seg, off, size, name in funcs:
        out += [
            "          - Kind:            S_GPROC32",
            "            ProcSym:",
            f"              CodeSize:        {size}",
            "              DbgStart:        0",
            "              DbgEnd:          0",
            "              FunctionType:    0",
            f"              Offset:          {off}",
            f"              Segment:         {seg}",
            "              Flags:           [  ]",
            f"              DisplayName:     {q(name)}",
            "          - Kind:            S_END",
            "            ScopeEndSym:     {}",
        ]
    for seg, off, name in data:
        out += [
            "          - Kind:            S_LDATA32",
            "            DataSym:",
            "              Type:            0",
            f"              Offset:          {off}",
            f"              Segment:         {seg}",
            f"              DisplayName:     {q(name)}",
        ]
    return out, source


def patch_symbol_records_stream(pdb: Path):
    """Point DBIHeader.symbol_records_stream at an existing EMPTY stream
    (vostok example, verbatim technique)."""
    dump = subprocess.run(["llvm-pdbutil", "dump", "--streams", str(pdb)],
                          capture_output=True, text=True,
                          check=True).stdout
    empty = next((int(m.group(1)) for line in dump.splitlines()
                  if (m := re.search(r"Stream\s+(\d+)\s+\(\s*0 bytes\)",
                                     line))), None)
    if empty is None:
        common.die("[synth] no empty stream to point symbol records at")
    d = bytearray(pdb.read_bytes())
    bs = struct.unpack_from("<I", d, 32)[0]
    num_dir_bytes = struct.unpack_from("<I", d, 44)[0]
    blk_map_addr = struct.unpack_from("<I", d, 52)[0]
    ndir = (num_dir_bytes + bs - 1) // bs
    dir_blocks = [struct.unpack_from("<I", d, blk_map_addr * bs + 4 * i)[0]
                  for i in range(ndir)]
    directory = b"".join(d[b * bs:b * bs + bs]
                         for b in dir_blocks)[:num_dir_bytes]
    nstreams = struct.unpack_from("<I", directory, 0)[0]
    sizes = [struct.unpack_from("<i", directory, 4 + 4 * i)[0]
             for i in range(nstreams)]
    pos = 4 + 4 * nstreams
    blocks = []
    for s in sizes:
        nb = 0 if s < 0 else (s + bs - 1) // bs
        blocks.append([struct.unpack_from("<I", directory, pos + 4 * j)[0]
                       for j in range(nb)])
        pos += 4 * nb
    struct.pack_into("<H", d, blocks[3][0] * bs + 0x14, empty)
    pdb.write_bytes(d)


def main(argv=None) -> int:
    exe = Path(common.resolve_exe())
    sections = read_sections(exe)

    def seg_of(rva):
        return next(((seg, rva - base) for _n, seg, base, end in sections
                     if base <= rva < end), (None, None))

    with INVENTORY.open() as fh:
        rows = list(csv.DictReader(
            line for line in fh if not line.startswith("#")))

    per_unit_funcs = defaultdict(list)
    data_rows = []
    for r in rows:
        rva = int(r["rva"], 16)
        seg, off = seg_of(rva)
        if seg is None:
            common.die(f"0x{rva:x} {r['name']}: outside every section")
        if r["kind"] == "func":
            unit = r["unit"] or RUNTIME_UNIT
            per_unit_funcs[unit].append(
                (seg, off, int(r["size"], 0), r["name"]))
        else:
            data_rows.append((seg, off, r["name"]))

    yaml = [
        "MSF:",
        "  SuperBlock:",
        "    BlockSize:       4096",
        "    FreeBlockMap:    2",
        "    NumBlocks:       0",
        "    NumDirectoryBytes: 0",
        "    Unknown1:        0",
        "    BlockMapAddr:    0",
        "PdbStream:",
        "  Age:             1",
        "  Guid:            '{00000000-0000-0000-0000-000000000000}'",
        "  Signature:       0",
        "  Features:        [ VC140 ]",
        "  Version:         VC70",
        "DbiStream:",
        "  VerHeader:       V70",
        "  Age:             1",
        "  BuildNumber:     0",
        "  PdbDllVersion:   0",
        "  PdbDllRbld:      0",
        "  Flags:           0",
        "  MachineType:     x86",
        "  Modules:",
    ]
    sources = []
    for unit in sorted(per_unit_funcs):
        with_lines = unit != RUNTIME_UNIT
        block, source = module_yaml(unit, sorted(per_unit_funcs[unit]),
                                    [], with_lines)
        yaml += block
        sources.append(source)
    block, source = module_yaml(DATA_UNIT, [], sorted(data_rows), False)
    yaml += block
    sources.append(source)
    yaml += ["StringTable:"] + [f"  - {q(s)}" for s in sources] + [""]

    OUT.parent.mkdir(parents=True, exist_ok=True)
    yaml_path = OUT.with_suffix(".yaml")
    yaml_path.write_text("\n".join(yaml))
    if OUT.exists():
        OUT.unlink()
    subprocess.run(["llvm-pdbutil", "yaml2pdb", "-pdb", str(OUT),
                    str(yaml_path)], check=True)
    patch_symbol_records_stream(OUT)

    nfuncs = sum(len(v) for v in per_unit_funcs.values())
    print(f"[build synth_pdb] {nfuncs} functions in "
          f"{len(per_unit_funcs)} modules + {len(data_rows)} data symbols "
          f"-> {OUT} ({OUT.stat().st_size:,} B)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
