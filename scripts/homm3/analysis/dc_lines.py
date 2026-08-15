#!/usr/bin/env python3
"""homm3.analysis.dc_lines - read retail's SOURCE STATEMENT LAYOUT off the
Dreamcast build.

The instrument `docs/dc-line-tables.md` describes, made repeatable. It joins
three records that nothing else in this tree combines:

  * the CodeView **line/addr table** for the compiland - which SOURCE LINE
    each run of instructions came from;
  * the **S_GPROC32 scope tree** (S_REGREL32 parameters/locals + one
    S_BLOCK32 per lexical `{ }`) - the original parameter list and the brace
    structure;
  * capstone's SH4 disassembly of `../orig/dreamcast/H3.EXE`, with the
    literal-pool operands resolved back to DC symbol names.

The result is a statement-by-statement listing of the compiland the
Dreamcast build was made from. The Dreamcast source is an OLDER REVISION of
the same file, which is what makes the two directions asymmetric and both
useful (docs/dc-line-tables.md):

  * DC has a structure and retail's bytes agree with it  -> that is retail's
    spelling too;
  * retail's bytes have code with NO DC LINE at all      -> that code is a
    post-Dreamcast edit, and it is the only place a source element retail
    has and the Dreamcast build does not can live.

The converse is NOT valid: a call the DC listing does not make is not
evidence retail has none.

ANALYSIS OUTPUT, NOT RETAIL EVIDENCE. A DC statement is a fact about the
Dreamcast compiland; promoting it into a reconstruction still needs retail's
x86 bytes to agree.

Usage
-----
    python3 -m homm3.analysis.dc_lines <dc-offset>            # statements
    python3 -m homm3.analysis.dc_lines <dc-offset> --asm      # + SH4 asm
    python3 -m homm3.analysis.dc_lines --find <name-substring>

`<dc-offset>` is the `dc 0x...` tag a `VA()` comment carries, i.e. a
section-1 offset.

Address bases (measured off the DC PE header, and NOT what an earlier draft
of the doc said): the image base is **0x10000** and `.text` is at va 0x1000,
so a literal-pool VA is `dc_offset + 0x11000` and the raw file offset is
`0x400 + dc_offset`. `.rdata`/`.data`/`.pdata` are sections 2..4 at
va 0x19e000 / 0x1a8000 / 0x1e1000.
"""
import argparse
import json
import re
import sys

from ..core import common

DUMP = common.HOMM3_DIR.parent / "homm3-symbols/HoMM3-Dreamcast-Dump/dump.txt"
EXE = common.HOMM3_DIR.parent / "orig/dreamcast/H3.EXE"

IMGBASE = 0x10000
SECVA = {1: 0x1000, 2: 0x19E000, 3: 0x1A8000, 4: 0x1E1000}
TEXT_RAW = 0x400
POOL_BASE = IMGBASE + SECVA[1]          # dc offset -> literal-pool VA

PROC_RE = re.compile(
    r"^\(\w+\) (S_GPROC32|S_LPROC32): \[0001:([0-9A-F]{8})\], "
    r"Cb: ([0-9A-F]{8}), Type:\s+\S+, (.*)$")
BLOCK_RE = re.compile(
    r"^\s*\(\w+\)\s+S_BLOCK32: \[0001:([0-9A-F]{8})\], Cb: ([0-9A-F]{8})")
REGREL_RE = re.compile(
    r"^\s*\(\w+\)\s+S_REGREL32: (\w+)\+([0-9A-F]{8}), Type:\s+\S+, (.*)$")
END_RE = re.compile(r"^\(\w+\) S_END")
LINETAB_RE = re.compile(
    r"^  (\S.*?), 0001:([0-9A-F]{8})-([0-9A-F]{8}), line/addr pairs = (\d+)")
PAIR_RE = re.compile(r"(\d+) ([0-9A-F]{8})")
PUB_RE = re.compile(
    r"S_PUB32: \[000(\d):([0-9A-F]{8})\], Flags: [0-9A-F]+, (.*)$")
DATA_RE = re.compile(
    r"(S_GDATA32|S_LDATA32): \[000(\d):([0-9A-F]{8})\], Type:\s+\S+, (.*)$")


def _dump_lines():
    with DUMP.open(errors="replace") as fh:
        return fh.read().splitlines()


def symbol_map(lines):
    """VA -> name, over every section the dump names."""
    out = {}
    for ln in lines:
        m = PROC_RE.match(ln)
        if m:
            out.setdefault(POOL_BASE + int(m.group(2), 16), m.group(4).strip())
            continue
        m = PUB_RE.search(ln)
        if m:
            sec, off = int(m.group(1)), int(m.group(2), 16)
            if sec in SECVA:
                out.setdefault(IMGBASE + SECVA[sec] + off, m.group(3).strip())
            continue
        m = DATA_RE.search(ln)
        if m:
            sec, off = int(m.group(2)), int(m.group(3), 16)
            if sec in SECVA:
                out.setdefault(IMGBASE + SECVA[sec] + off, m.group(4).strip())
    return out


def find_proc(lines, off):
    """(name, cb, [(reg, off, name)], [(addr, cb)]) for the proc at `off`."""
    want = "%08X" % off
    for i, ln in enumerate(lines):
        m = PROC_RE.match(ln)
        if not m or m.group(2) != want:
            continue
        name, cb = m.group(4).strip(), int(m.group(3), 16)
        locals_, blocks = [], []
        for ln2 in lines[i + 1:]:
            if PROC_RE.match(ln2) or END_RE.match(ln2):
                break
            mb = BLOCK_RE.match(ln2)
            if mb:
                blocks.append((int(mb.group(1), 16), int(mb.group(2), 16)))
            mr = REGREL_RE.match(ln2)
            if mr:
                locals_.append(
                    (mr.group(1), int(mr.group(2), 16), mr.group(3).strip()))
        return name, cb, locals_, blocks
    return None


def line_table(lines, off, cb):
    """Sorted [(addr, line, file)] for every pair inside [off, off+cb)."""
    out, i, n = [], 0, len(lines)
    while i < n:
        m = LINETAB_RE.match(lines[i])
        if m:
            lo, hi = int(m.group(2), 16), int(m.group(3), 16)
            if not (hi < off or lo >= off + cb):
                fname, j = m.group(1), i + 1
                while (j < n and not LINETAB_RE.match(lines[j])
                       and "***" not in lines[j]):
                    for pm in PAIR_RE.finditer(lines[j]):
                        a = int(pm.group(2), 16)
                        if off <= a < off + cb:
                            out.append((a, int(pm.group(1)), fname))
                    j += 1
                i = j
                continue
        i += 1
    out.sort()
    return out


class Sh4(object):
    """Just enough SH4 to name the calls and count the branches.

    A call is `mov.l @(d,pc),Rn; jsr @Rn` (or a near `bsr`), so the pool
    entry IS the callee: decode the `mov.l` ourselves rather than trusting a
    disassembler's operand text, and track the last pool value loaded into
    each register.
    """

    def __init__(self, data):
        self.data = data

    def hw(self, dc):
        o = TEXT_RAW + dc
        return int.from_bytes(self.data[o:o + 2], "little")

    def pool(self, dc):
        """VA held in the pool entry the `mov.l @(d,pc)` at `dc` loads."""
        w = self.hw(dc)
        if (w >> 12) != 0xD:
            return None
        tgt = (((dc + POOL_BASE + 4) & ~3) - POOL_BASE) + (w & 0xFF) * 4
        o = TEXT_RAW + tgt
        return int.from_bytes(self.data[o:o + 4], "little")

    def scan(self, start, end):
        """(calls, conditional-branch count) over [start, end)."""
        regs, calls, branches = {}, [], 0
        dc = start
        while dc < end:
            w = self.hw(dc)
            if (w >> 12) == 0xD:                       # mov.l @(d,pc),Rn
                regs[(w >> 8) & 0xF] = self.pool(dc)
            elif (w & 0xF0FF) == 0x400B:               # jsr @Rn
                rn = (w >> 8) & 0xF
                calls.append(regs.get(rn))
            elif (w & 0xF000) == 0xB000:               # bsr <disp12>
                d = w & 0xFFF
                if d & 0x800:
                    d -= 0x1000
                calls.append(dc + 4 + d * 2 + POOL_BASE)
            elif (w & 0xFF00) in (0x8900, 0x8B00, 0x8D00, 0x8F00):
                branches += 1                          # bt/bf/bt.s/bf.s
            dc += 2
        return calls, branches


def render(off, asm=False, out=sys.stdout):
    lines = _dump_lines()
    got = find_proc(lines, off)
    if not got:
        print("no S_GPROC32/S_LPROC32 at dc 0x%x" % off, file=out)
        return 1
    name, cb, locals_, blocks = got
    syms = symbol_map(lines)
    lt = line_table(lines, off, cb)
    data = EXE.read_bytes()
    sh4 = Sh4(data)

    def sym(va):
        return syms.get(va) or ("0x%x" % va if va else "<indirect>")

    starts, ends = {}, {}
    for a, bcb in blocks:
        starts[a] = starts.get(a, 0) + 1
        ends[a + bcb] = ends.get(a + bcb, 0) + 1

    print("=== dc 0x%05x  %s  Cb=0x%x (%d B) ===" % (off, name, cb, cb),
          file=out)
    for reg, o, nm in locals_:
        print("   %s+0x%-4x  %s" % (reg, o, nm), file=out)
    files = sorted({f for _, _, f in lt})
    print("   %d line/addr pair(s) from %s" % (len(lt), ", ".join(files)),
          file=out)
    print(file=out)

    bounds = [a for a, _, _ in lt] + [off + cb]
    for k, (a, line, _) in enumerate(lt):
        end = bounds[k + 1]
        calls, branches = sh4.scan(a, end)
        marks = " {" * starts.get(a, 0) + " }" * ends.get(a, 0)
        print("  line %-5d dc 0x%05x %4dB%s%s"
              % (line, a, end - a,
                 ("  br=%d" % branches) if branches else "", marks), file=out)
        for c in calls:
            print("        -> %s" % sym(c), file=out)
        if asm:
            _dump_asm(sh4, data, a, end, syms, out)
    return 0


def _dump_asm(sh4, data, start, end, syms, out):
    try:
        import capstone
    except ImportError:
        print("        (capstone not importable - --asm unavailable)",
              file=out)
        return
    md = capstone.Cs(capstone.CS_ARCH_SH,
                     capstone.CS_MODE_SH4 | capstone.CS_MODE_LITTLE_ENDIAN)
    raw = data[TEXT_RAW + start:TEXT_RAW + end]
    dc = start
    while dc < end:
        decoded = False
        for ins in md.disasm(raw[dc - start:], dc + POOL_BASE):
            note = ""
            v = sh4.pool(ins.address - POOL_BASE)
            if v is not None:
                nm = syms.get(v)
                note = "   ; %s" % (nm if nm else "= 0x%x" % v)
            print("          %05x  %-9s %-26s%s"
                  % (ins.address - POOL_BASE, ins.mnemonic, ins.op_str, note),
                  file=out)
            dc = ins.address - POOL_BASE + ins.size
            decoded = True
        if dc < end:
            # a literal pool sits inside the body; step over one halfword
            # and resume (capstone stops at the first non-instruction).
            print("          %05x  .word     0x%04x" % (dc, sh4.hw(dc)),
                  file=out)
            dc += 2
        elif not decoded:
            break


def find(pattern, out=sys.stdout):
    lines = _dump_lines()
    hits = 0
    for ln in lines:
        m = PROC_RE.match(ln)
        if m and pattern.lower() in m.group(4).lower():
            print("  dc 0x%-7x Cb=0x%-5x %s"
                  % (int(m.group(2), 16), int(m.group(3), 16),
                     m.group(4).strip()), file=out)
            hits += 1
    if not hits:
        print("  no proc name contains %r" % pattern, file=out)
    return 0


def main(argv):
    ap = argparse.ArgumentParser(prog="homm3.analysis.dc_lines",
                                 description=__doc__.split("\n")[0])
    ap.add_argument("offset", nargs="?",
                    help="dc offset, e.g. 0x55df4 (a VA() `dc 0x...` tag)")
    ap.add_argument("--asm", action="store_true",
                    help="interleave the SH4 disassembly under each statement")
    ap.add_argument("--find", metavar="SUBSTRING",
                    help="list DC procs whose name contains SUBSTRING")
    args = ap.parse_args(argv)
    if args.find:
        return find(args.find)
    if not args.offset:
        ap.error("give a dc offset or --find")
    return render(int(args.offset, 16), asm=args.asm)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
