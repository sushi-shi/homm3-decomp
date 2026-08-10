# -*- coding: utf-8 -*-
"""Read-only queries over the persisted c2-atlas Ghidra project for the
phase-5 handle-order work (docs/vc6/handle-order.md).

The front end's handle ASSIGNMENT order is measured by compile probes
(homm3.vc6._handles - C1XX is not in the Ghidra project); what this script
reaches is the C2 SIDE of the story: p2symtab.c, where the numeric handles
baked into the IL become back-end symbol-table state - the `[handle &
0x3ff]` bucket walk that makes C2's behaviour sensitive to handle VALUES
(the C1 non-monotonicity suspect, il-format.md section 5), and the hash
lookup regalloc.md already pinned at rva 0x232ec with buckets at .bssbe
0x9d88c.

Reuses import_c2.open_program() and regasg_probe's helpers - the analyzed
project under build/re/vc6/ is opened as-is (a missing project is an
error; run `homm3 vc6 atlas --regen` first).  Output goes to gitignored
build/re/vc6/raw/handles/ - working RE data, not evidence.

Subcommands
-----------
  dump  [--lo RVA --hi RVA]  decompile + disassemble every function whose
                             entry lies in [lo, hi); default = the
                             p2symtab.c neighbourhood 0x8339f..0x84d9b
                             (atlas anchors 0x8339f/0x83a78/0x83cc2/
                             0x84045 + the p2symtab.c|reader.c brackets,
                             bounded by reader.c's pure block).
  hash                       decompile the back-end symbol-hash lookup fn
                             (0x232ec) and list every code reference to
                             the bucket array (.bssbe 0x9d88c) - the
                             handle-keyed structure whose iteration order
                             a renumbering can permute.

Standalone:  python3 scripts/homm3/vc6/ghidra_scripts/handle_probe.py hash
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

if __package__ in (None, ""):  # standalone: put scripts/ on the path
    sys.path.insert(0, str(Path(__file__).resolve().parents[3]))

# sibling modules, same directory (ghidra_scripts/ has no __init__ by design)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import import_c2   # noqa: E402
import regasg_probe as _rp  # noqa: E402  (helper reuse: _functions, ...)

OUT_DIR = import_c2.RAW_DIR / "handles"
BASE = import_c2.IMAGE_BASE

# p2symtab.c per evidence/vc6/c2-tu-map.tsv: ICE-string anchors 0x8339f /
# 0x83a78 / 0x83cc2 / 0x84045, then p2symtab.c|reader.c brackets down to
# 0x84cb9+84; reader.c's pure block starts at 0x84d9b and bounds the span.
P2SYMTAB_LO = 0x8339F
P2SYMTAB_HI = 0x84D9B
# Back-end symbol hash (docs/vc6/regalloc.md address ledger): lookup fn
# `[handle & 0x3ff]`, buckets .bssbe 0x9d88c (key at sym+0x1c, chain +0x2c).
HASH_LOOKUP_FN = 0x232EC
HASH_BUCKETS = 0x9D88C


def cmd_dump(program, args):
    # regasg_probe.cmd_dump writes into its own OUT_DIR; replicate the walk
    # here with ours (the helper functions are shared, the sink is not).
    import bisect
    funcs = _rp._functions(program)
    picked = [(r, s, n, f) for r, s, n, f in funcs if args.lo <= r < args.hi]
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    ifc = _rp._decompiler(program)
    from ghidra.util.task import ConsoleTaskMonitor
    mon = ConsoleTaskMonitor()
    index = []
    entries = [r for r, _s, _n, _f in funcs]
    for rva, size, name, fn in picked:
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
            "\n".join(_rp._disasm_span(program, rva, span_hi)) + "\n")
        index.append((rva, size, span_hi - rva, name))
        print("[handle-probe] dumped 0x%x %-28s size=%d span=%d" %
              (rva, name, size, span_hi - rva), flush=True)
    ifc.dispose()
    with (OUT_DIR / "index.tsv").open("w") as fh:
        fh.write("entry_rva\tghidra_size\tspan\tname\n")
        for rva, size, span, name in index:
            fh.write("0x%x\t%d\t%d\t%s\n" % (rva, size, span, name))
    print("[handle-probe] %d functions -> %s" % (len(index), OUT_DIR))
    return 0


def cmd_hash(program, args):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    funcs = _rp._functions(program)
    fn = next((f for r, _s, _n, f in funcs if r == HASH_LOOKUP_FN), None)
    if fn is None:
        # entry may sit inside a fragmented body: take the interval owner
        entries = [r for r, _s, _n, _f in funcs]
        owner = _rp._interval_owner(entries, HASH_LOOKUP_FN)
        fn = next((f for r, _s, _n, f in funcs if r == owner), None)
        print("[handle-probe] 0x%x not a carved entry; owner 0x%x"
              % (HASH_LOOKUP_FN, owner or 0))
    if fn is not None:
        ifc = _rp._decompiler(program)
        from ghidra.util.task import ConsoleTaskMonitor
        res = ifc.decompileFunction(fn, 90, ConsoleTaskMonitor())
        dec = res.getDecompiledFunction() if res.decompileCompleted() else None
        text = dec.getC() if dec else "/* decompile FAILED */\n"
        (OUT_DIR / ("hash_lookup_%06x.c" % HASH_LOOKUP_FN)).write_text(text)
        print(text)
        ifc.dispose()
    print("== references to the bucket array .bssbe 0x%x ==" % HASH_BUCKETS)

    class _A:
        addrs = "%x" % HASH_BUCKETS
    return _rp.cmd_refs(program, _A())


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ss = ap.add_subparsers(dest="cmd", required=True)
    hx = lambda s: int(s, 16)  # noqa: E731
    pd = ss.add_parser("dump")
    pd.add_argument("--lo", type=hx, default=P2SYMTAB_LO)
    pd.add_argument("--hi", type=hx, default=P2SYMTAB_HI)
    ss.add_parser("hash")
    args = ap.parse_args(argv)

    gproject, program = import_c2.open_program()
    try:
        return {"dump": cmd_dump, "hash": cmd_hash}[args.cmd](program, args)
    finally:
        import_c2.close_program(gproject, program)


if __name__ == "__main__":
    sys.exit(main())
