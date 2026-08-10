# -*- coding: utf-8 -*-
"""Read-only queries over the persisted c2-atlas Ghidra project for the
phase-3 inline.c reverse-engineering (docs/vc6/inliner.md).

Reuses import_c2.open_program() - the analyzed project under build/re/vc6/
is opened as-is (never re-imported/re-analyzed here; a missing project is an
error, run `homm3 vc6 atlas --regen` first). All output goes to gitignored
build/re/vc6/raw/inline/ - working RE data, not evidence.

Subcommands
-----------
  dump   [--lo RVA --hi RVA]   decompile + disassemble every function whose
                               entry lies in [lo, hi); default = the inline.c
                               module neighbourhood 0x93c23-0x95e9f from the
                               atlas (stack.c|inline.c .. inline.c|dlp.c).
  refs   [--addrs r1,r2,...]   every code reference to the given .bssbe/.databe
                               rvas, with owning function (link-order interval,
                               same rationale as the atlas) + the instruction;
                               default = the atlas's inline-budget candidates.
  callers [--lo RVA --hi RVA]  call edges INTO [lo, hi) from outside it - who
                               drives the inline module (pass driver).

Standalone:  python3 scripts/homm3/vc6/ghidra_scripts/inline_probe.py dump
"""
from __future__ import annotations

import argparse
import bisect
import sys
from pathlib import Path

if __package__ in (None, ""):  # standalone: put scripts/ on the path
    sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

# sibling module, same directory (ghidra_scripts/ has no __init__ by design)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_c2  # noqa: E402

OUT_DIR = import_c2.RAW_DIR / "inline"
BASE = import_c2.IMAGE_BASE

# inline.c's gap-neighbourhood per evidence/vc6/c2-tu-map.tsv:
# stack.c|inline.c rows from 0x93d28, anchor 0x94521, inline.c|dlp.c rows to
# 0x95e48+87; the dlp.c anchor at 0x95e9f bounds it above.
DEFAULT_LO = 0x93C23
DEFAULT_HI = 0x95E9F

# the atlas's inline-budget hunting cluster (docs/vc6/c2-atlas.md par.3) +
# the other .databe/.bssbe rows whose writer_tus include the inline.c gaps.
DEFAULT_GLOBALS = [
    0xAC094, 0xAC098, 0xAC0A0, 0xAC0D4,          # stack.c|inline.c writers
    0x9F258, 0x9F280, 0xADFCC, 0xAE248, 0xAE24C, 0xAE250,  # inline.c|dlp.c
]


def _functions(program):
    """[(entry_rva, size, name, fn)] for every .text function, sorted."""
    memory = program.getMemory()
    out = []
    for fn in program.getFunctionManager().getFunctions(True):
        entry = fn.getEntryPoint()
        blk = memory.getBlock(entry)
        if blk is None or not blk.isExecute():
            continue
        out.append((entry.getOffset() - BASE,
                    int(fn.getBody().getNumAddresses()), str(fn.getName()), fn))
    out.sort(key=lambda t: t[0])
    return out


def _interval_owner(entries, site_rva):
    i = bisect.bisect_right(entries, site_rva) - 1
    return entries[i] if i >= 0 else None


def _decompiler(program):
    from ghidra.app.decompiler import DecompInterface
    ifc = DecompInterface()
    ifc.openProgram(program)
    return ifc


def _disasm_span(program, lo_rva, hi_rva):
    """Flat listing text for [lo, hi): 'rva  bytes  mnemonic operands'."""
    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    lines = []
    it = listing.getInstructions(space.getAddress(BASE + lo_rva), True)
    for ins in it:
        rva = ins.getAddress().getOffset() - BASE
        if rva >= hi_rva:
            break
        raw = " ".join("%02x" % (b & 0xFF) for b in ins.getBytes())
        lines.append("%6x  %-24s %s" % (rva, raw, str(ins)))
    return lines


def cmd_dump(program, args):
    lo, hi = args.lo, args.hi
    funcs = _functions(program)
    picked = [(r, s, n, f) for r, s, n, f in funcs if lo <= r < hi]
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    ifc = _decompiler(program)
    from ghidra.util.task import ConsoleTaskMonitor
    mon = ConsoleTaskMonitor()

    index = []
    entries = [r for r, _s, _n, _f in funcs]
    for rva, size, name, fn in picked:
        # link-order span: to the NEXT function entry (Ghidra bodies fragment)
        i = bisect.bisect_right(entries, rva)
        span_hi = entries[i] if i < len(entries) else rva + size
        res = ifc.decompileFunction(fn, 90, mon)
        dec = res.getDecompiledFunction() if res.decompileCompleted() else None
        ctext = dec.getC() if dec else "/* decompile FAILED: %s */\n" % \
            res.getErrorMessage()
        stem = "%06x_%s" % (rva, "".join(c if c.isalnum() else "_"
                                         for c in name)[:40])
        (OUT_DIR / (stem + ".c")).write_text(ctext)
        (OUT_DIR / (stem + ".asm")).write_text(
            "\n".join(_disasm_span(program, rva, span_hi)) + "\n")
        index.append((rva, size, span_hi - rva, name))
        print("[inline-probe] dumped 0x%x %-28s size=%d span=%d" %
              (rva, name, size, span_hi - rva), flush=True)
    ifc.dispose()
    with (OUT_DIR / "index.tsv").open("w") as fh:
        fh.write("entry_rva\tghidra_size\tspan\tname\n")
        for rva, size, span, name in index:
            fh.write("0x%x\t%d\t%d\t%s\n" % (rva, size, span, name))
    print("[inline-probe] %d functions -> %s" % (len(index), OUT_DIR))
    return 0


def cmd_refs(program, args):
    rvas = [int(x, 16) for x in args.addrs.split(",")] if args.addrs \
        else DEFAULT_GLOBALS
    funcs = _functions(program)
    entries = [r for r, _s, _n, _f in funcs]
    name_of = {r: n for r, _s, n, _f in funcs}
    refmgr = program.getReferenceManager()
    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    memory = program.getMemory()
    for g in rvas:
        addr = space.getAddress(BASE + g)
        print("== global 0x%x ==" % g)
        rows = []
        for ref in refmgr.getReferencesTo(addr):
            frm = ref.getFromAddress()
            blk = memory.getBlock(frm)
            if blk is None or not blk.isExecute():
                continue
            site = frm.getOffset() - BASE
            owner = _interval_owner(entries, site)
            ins = listing.getInstructionAt(frm)
            rows.append((site, owner, str(ref.getReferenceType()),
                         str(ins) if ins else "?"))
        for site, owner, rtype, text in sorted(rows):
            print("  site 0x%-7x in 0x%-7x %-24s %-8s %s" %
                  (site, owner or 0, name_of.get(owner, "?"), rtype, text))
    return 0


def cmd_callers(program, args):
    lo, hi = args.lo, args.hi
    funcs = _functions(program)
    entries = [r for r, _s, _n, _f in funcs]
    name_of = {r: n for r, _s, n, _f in funcs}
    refmgr = program.getReferenceManager()
    space = program.getAddressFactory().getDefaultAddressSpace()
    memory = program.getMemory()
    inside = [(r, f) for r, _s, _n, f in funcs if lo <= r < hi]
    for rva, fn in inside:
        callers = set()
        for ref in refmgr.getReferencesTo(space.getAddress(BASE + rva)):
            frm = ref.getFromAddress()
            blk = memory.getBlock(frm)
            if blk is None or not blk.isExecute():
                continue
            site = frm.getOffset() - BASE
            owner = _interval_owner(entries, site)
            if owner is not None and not (lo <= owner < hi):
                callers.add((owner, site))
        if callers:
            print("0x%x %s  <- %s" % (rva, name_of.get(rva, "?"), ", ".join(
                "0x%x(%s)@0x%x" % (o, name_of.get(o, "?"), s)
                for o, s in sorted(callers))))
    return 0


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ss = ap.add_subparsers(dest="cmd", required=True)
    hx = lambda s: int(s, 16)  # noqa: E731
    pd = ss.add_parser("dump")
    pd.add_argument("--lo", type=hx, default=DEFAULT_LO)
    pd.add_argument("--hi", type=hx, default=DEFAULT_HI)
    pr = ss.add_parser("refs")
    pr.add_argument("--addrs", help="comma-separated hex rvas")
    pc = ss.add_parser("callers")
    pc.add_argument("--lo", type=hx, default=DEFAULT_LO)
    pc.add_argument("--hi", type=hx, default=DEFAULT_HI)
    args = ap.parse_args(argv)

    gproject, program = import_c2.open_program()
    try:
        return {"dump": cmd_dump, "refs": cmd_refs,
                "callers": cmd_callers}[args.cmd](program, args)
    finally:
        import_c2.close_program(gproject, program)


if __name__ == "__main__":
    sys.exit(main())
