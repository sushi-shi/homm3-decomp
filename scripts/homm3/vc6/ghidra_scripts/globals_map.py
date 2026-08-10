# -*- coding: utf-8 -*-
"""Map C2.DLL's back-end state sections: every referenced global in
.bssbe (0x10799000, ~26 KB zero-init) and .databe (0x107ac000, ~9 KB init).

These two sections hold the optimizer's mutable state - the hunting ground
for the /Ob2 inline budget counter and the register-allocator state the later
vc6 phases model. For each referenced address we record every referencing
site, whether it reads or writes, and the access width, so atlas.py can fold
this into evidence/vc6/c2-globals.tsv (joined with the TU map for writer_tus).

Access classification per referencing instruction, best channel first:
  pcode-exact   a ram-space varnode of the instruction's raw pcode covers the
                target VA: direct `mov [glob], eax` style access. Width is the
                varnode size; input = read, output = write.
  pcode-approx  no ram varnode matches (base+displacement / indexed access -
                the address is computed, so pcode uses LOAD/STORE). Direction
                comes from the analyzer's reference read/write bits; width is
                the LOAD/STORE data size (the ELEMENT width for arrays).
  reftype       neither channel: address-taken (lea/push imm) or a reference
                from data. Width 0, kind "addr" unless the analyzer knew.

Raw output (build/re/vc6/raw/c2_globals_raw.tsv, gitignored):
  target_rva  section  site_rva  func_rva  kind  width  channel
"""
from __future__ import annotations

import sys
from pathlib import Path

if __package__ in (None, ""):  # standalone: put scripts/ on the path
    sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

from homm3.vc6 import _common

SECTIONS = (".bssbe", ".databe")


def _kind_from_reftype(reftype) -> str:
    rd, wr = bool(reftype.isRead()), bool(reftype.isWrite())
    if rd and wr:
        return "rw"
    if rd:
        return "r"
    if wr:
        return "w"
    return "addr"


def _access(listing, frm, dest_va: int, reftype):
    """(kind, width, channel) for one referencing instruction."""
    from ghidra.program.model.pcode import PcodeOp

    instr = listing.getInstructionAt(frm)
    if instr is None:
        return _kind_from_reftype(reftype), 0, "reftype"
    rd = wr = 0
    loads, stores = [], []
    for op in instr.getPcode():
        out = op.getOutput()
        if out is not None and out.getAddress().isMemoryAddress():
            o, s = int(out.getOffset()), int(out.getSize())
            if o <= dest_va < o + s:
                wr = max(wr, s)
        for i in range(op.getNumInputs()):
            vn = op.getInput(i)
            if vn is not None and vn.getAddress().isMemoryAddress():
                o, s = int(vn.getOffset()), int(vn.getSize())
                if o <= dest_va < o + s:
                    rd = max(rd, s)
        opc = op.getOpcode()
        if opc == PcodeOp.LOAD and out is not None:
            loads.append(int(out.getSize()))
        elif opc == PcodeOp.STORE and op.getNumInputs() >= 3:
            stores.append(int(op.getInput(2).getSize()))
    if rd or wr:
        kind = "rw" if (rd and wr) else ("r" if rd else "w")
        return kind, max(rd, wr), "pcode-exact"
    kind = _kind_from_reftype(reftype)
    widths = []
    if kind in ("r", "rw") and loads:
        widths += loads
    if kind in ("w", "rw") and stores:
        widths += stores
    if widths:
        return kind, max(widths), "pcode-approx"
    return kind, 0, "reftype"


def run(program, raw_dir: Path) -> dict:
    """Export every referenced .bssbe/.databe address. Returns counts."""
    from ghidra.program.model.address import AddressSet

    memory = program.getMemory()
    fm = program.getFunctionManager()
    refmgr = program.getReferenceManager()
    listing = program.getListing()
    base = program.getImageBase().getOffset()

    targets = AddressSet()
    bounds = {}  # name -> (start_rva, end_rva exclusive)
    for name in SECTIONS:
        blk = memory.getBlock(name)
        if blk is None:
            _common.die(f"globals_map: C2.DLL has no {name} block in Ghidra")
        targets.add(blk.getStart(), blk.getEnd())
        bounds[name] = (blk.getStart().getOffset() - base,
                        blk.getEnd().getOffset() - base + 1)

    def section_of(rva: int) -> str:
        for name, (lo, hi) in bounds.items():
            if lo <= rva < hi:
                return name
        return "?"

    rows = []
    it = refmgr.getReferenceDestinationIterator(targets, True)
    for dest in it:
        dest_va = dest.getOffset()
        drva = dest_va - base
        sec = section_of(drva)
        for ref in refmgr.getReferencesTo(dest):
            frm = ref.getFromAddress()
            blk = memory.getBlock(frm)
            in_text = blk is not None and blk.isExecute()
            frva = frm.getOffset() - base
            if in_text:
                fn = fm.getFunctionContaining(frm)
                func_rva = (fn.getEntryPoint().getOffset() - base
                            if fn is not None else -1)
                kind, width, channel = _access(listing, frm, dest_va,
                                               ref.getReferenceType())
            else:
                func_rva = -1
                kind = _kind_from_reftype(ref.getReferenceType())
                width, channel = 0, "data-ref"
            rows.append((drva, sec, frva,
                         "-" if func_rva < 0 else "0x%x" % func_rva,
                         kind, width, channel))

    rows.sort(key=lambda r: (r[0], r[2]))
    out = raw_dir / "c2_globals_raw.tsv"
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w") as fh:
        fh.write("target_rva\tsection\tsite_rva\tfunc_rva\tkind\twidth\tchannel\n")
        for drva, sec, srva, func, kind, width, channel in rows:
            fh.write(f"0x{drva:x}\t{sec}\t0x{srva:x}\t{func}\t{kind}\t"
                     f"{width}\t{channel}\n")

    addrs = {r[0] for r in rows}
    counts = {
        "referenced_addrs": len(addrs),
        "ref_rows": len(rows),
        "bssbe_addrs": sum(1 for a in addrs
                           if bounds[".bssbe"][0] <= a < bounds[".bssbe"][1]),
        "databe_addrs": sum(1 for a in addrs
                            if bounds[".databe"][0] <= a < bounds[".databe"][1]),
    }
    print(f"[atlas] globals_map: {counts['referenced_addrs']} referenced "
          f"addresses ({counts['bssbe_addrs']} .bssbe, "
          f"{counts['databe_addrs']} .databe), {counts['ref_rows']} ref rows",
          flush=True)
    return counts


def main(argv=None) -> int:
    import import_c2  # sibling, same directory
    gproject, program = import_c2.open_program()
    try:
        run(program, import_c2.RAW_DIR)
    finally:
        import_c2.close_program(gproject, program)
    return 0


if __name__ == "__main__":
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    sys.exit(main())
