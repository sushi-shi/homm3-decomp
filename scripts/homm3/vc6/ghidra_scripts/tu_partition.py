# -*- coding: utf-8 -*-
"""Anchor C2.DLL functions to their VC6 back-end translation units.

C2.DLL retains 48 compiler-source-path strings (E:\\8447\\vc98\\p2\\src\\...)
in .rdata, each referenced from assertion/ICE sites INSIDE its own translation
unit (the error formatter takes "%s(%ld) : " with the path + line). A function
that references inline.c's path string therefore lies inside inline.obj - a
link-order anchor no other channel on this stripped DLL provides.

Two independent SITE-discovery channels, unioned per (function, tu, string):
  ghidra-ref  Ghidra's analysis references TO the string address
  imm-scan    raw little-endian imm32 occurrences of the string VA in .text
              bytes (via the gated _toolchain.Binary reader - no Ghidra
              involvement)
Where both agree the channel is "both".

Site -> function attribution is by LINK-ORDER INTERVAL: the function whose
[entry, next_entry) span contains the site. Ghidra's getFunctionContaining is
deliberately NOT used for attribution - on this DLL its auto-analysis merges
distant blocks into non-contiguous function bodies (measured: the 21
p2symtab.c ICE sites, all physically in 0x833b3..0x84060, came back owned by
13 functions scattered across the whole image). The ICE site's PHYSICAL
address is what proves TU membership (the string literal is lexed in the TU
itself), so the interval owner is the honest anchor; Ghidra's opinion is kept
per row as a diagnostic (n_frag = sites where it disagreed).

Raw outputs (build/re/vc6/raw/, gitignored; atlas.py post-processes into
evidence/vc6/c2-tu-map.tsv with the bracket propagation):
  c2_funcs_raw.tsv     entry_rva  size  name        (every .text function)
  c2_anchors_raw.tsv   func_rva  tu  string_rva  n_ref  n_imm  n_frag  channel

The string discovery reads the gated FILE bytes, not the Ghidra DB, so the
string set is identical for both channels and for atlas.py's corroboration.
A count other than 48 is a hard abort - that means the wrong pressing or a
broken scan, never a partial answer.
"""
from __future__ import annotations

import bisect
import re
import struct
import sys
from pathlib import Path

if __package__ in (None, ""):  # standalone: put scripts/ on the path
    sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from homm3.vc6 import _common

PATH_RE = re.compile(rb"E:\\8447\\vc98\\p2\\src\\[\x20-\x7e]+")
N_EXPECTED = 48


def tu_strings(binary):
    """[(string_rva, va, path, tu)] for the 48 source-path strings, by rva."""
    out = []
    for m in PATH_RE.finditer(binary.data):
        rva = binary.off_to_rva(m.start())
        if rva is None:
            continue
        path = m.group(0).decode("latin1")
        out.append((rva, binary.image_base + rva, path, path.rsplit("\\", 1)[1]))
    out.sort()
    if len(out) != N_EXPECTED:
        _common.die(f"tu_partition: found {len(out)} source-path strings, "
                    f"expected {N_EXPECTED} - wrong pressing or broken scan")
    return out


def imm_sites(binary, va: int) -> list[int]:
    """RVAs of every little-endian imm32 occurrence of `va` in .text bytes."""
    text = next(s for s in binary.sections if s.name == ".text")
    blob = binary.data[text.raw:text.raw + text.rsize]
    needle = struct.pack("<I", va)
    sites, start = [], 0
    while True:
        i = blob.find(needle, start)
        if i < 0:
            break
        sites.append(text.vaddr + i)
        start = i + 1
    return sites


def _write_raw(path: Path, header: list[str], rows) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w") as fh:
        fh.write("\t".join(header) + "\n")
        for row in rows:
            fh.write("\t".join(str(c) for c in row) + "\n")


def run(program, binary, raw_dir: Path) -> dict:
    """Export .text functions + per-function TU anchors. Returns counts."""
    memory = program.getMemory()
    fm = program.getFunctionManager()
    refmgr = program.getReferenceManager()
    space = program.getAddressFactory().getDefaultAddressSpace()
    base = program.getImageBase().getOffset()

    # --- every .text function -------------------------------------------
    funcs = []
    for fn in fm.getFunctions(True):
        entry = fn.getEntryPoint()
        blk = memory.getBlock(entry)
        if blk is None or not blk.isExecute():
            continue
        funcs.append((entry.getOffset() - base,
                      int(fn.getBody().getNumAddresses()),
                      str(fn.getName())))
    funcs.sort()
    _write_raw(raw_dir / "c2_funcs_raw.tsv",
               ["entry_rva", "size", "name"],
               (("0x%x" % r, s, n) for r, s, n in funcs))

    # --- anchors: both site channels, attributed by link-order interval -
    strings = tu_strings(binary)
    entries = [rva for rva, _size, _name in funcs]

    def interval_owner(site: int) -> int:
        i = bisect.bisect_right(entries, site) - 1
        return entries[i] if i >= 0 else -1

    hits: dict[tuple[int, str, int], dict[str, int]] = {}

    def bump(site: int, tu: str, string_rva: int, channel: str,
             ghidra_fn) -> None:
        frva = interval_owner(site)
        rec = hits.setdefault((frva, tu, string_rva),
                              {"ghidra-ref": 0, "imm-scan": 0, "frag": 0})
        rec[channel] += 1
        if ghidra_fn is None or \
                ghidra_fn.getEntryPoint().getOffset() - base != frva:
            rec["frag"] += 1  # Ghidra's containing-function opinion diverges

    for string_rva, va, _path, tu in strings:
        addr = space.getAddress(va)
        for ref in refmgr.getReferencesTo(addr):
            frm = ref.getFromAddress()
            blk = memory.getBlock(frm)
            if blk is None or not blk.isExecute():
                continue  # data->data refs are not anchors
            site = frm.getOffset() - base
            bump(site, tu, string_rva, "ghidra-ref",
                 fm.getFunctionContaining(frm))
        for site in imm_sites(binary, va):
            bump(site, tu, string_rva, "imm-scan",
                 fm.getFunctionContaining(space.getAddress(base + site)))

    rows = []
    for (frva, tu, string_rva) in sorted(hits, key=lambda k: (k[0], k[1], k[2])):
        rec = hits[(frva, tu, string_rva)]
        channel = ("both" if rec["ghidra-ref"] and rec["imm-scan"]
                   else "ghidra-ref" if rec["ghidra-ref"] else "imm-scan")
        rows.append(("-" if frva < 0 else "0x%x" % frva, tu,
                     "0x%x" % string_rva, rec["ghidra-ref"], rec["imm-scan"],
                     rec["frag"], channel))
    _write_raw(raw_dir / "c2_anchors_raw.tsv",
               ["func_rva", "tu", "string_rva", "n_ref", "n_imm", "n_frag",
                "channel"],
               rows)

    anchored_funcs = {k[0] for k in hits if k[0] >= 0}
    anchored_tus = {k[1] for k in hits if k[0] >= 0}
    counts = {
        "functions": len(funcs),
        "anchor_rows": len(rows),
        "anchored_funcs": len(anchored_funcs),
        "anchored_tus": len(anchored_tus),
        "frag_sites": sum(rec["frag"] for rec in hits.values()),
    }
    print(f"[atlas] tu_partition: {counts['functions']} .text functions; "
          f"{counts['anchored_funcs']} anchored to {counts['anchored_tus']}/"
          f"{N_EXPECTED} TUs ({counts['frag_sites']} sites where Ghidra's "
          "containing-function opinion diverged)", flush=True)
    return counts


def main(argv=None) -> int:
    from homm3.vc6 import _toolchain
    import import_c2  # sibling, same directory
    gproject, program = import_c2.open_program()
    try:
        run(program, _toolchain.Binary("C2.DLL"), import_c2.RAW_DIR)
    finally:
        import_c2.close_program(gproject, program)
    return 0


if __name__ == "__main__":
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    sys.exit(main())
