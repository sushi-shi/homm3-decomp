#!/usr/bin/env python3
"""Residual ranker: normalized `sema disasm` vs `--base`, real vs artefact.

usage: rank.py <0xva> [--show N]
Prints total rows, differing rows, real vs artefact split, and (with --show)
the first N real differing rows.
"""
import difflib
import re
import subprocess
import sys

ROOT = "/home/sheep/Projects/homm3/wt-cold-windows"


def run(va, base):
    cmd = ["homm3", "sema", "disasm", va, "--verbose"]
    if base:
        cmd.append("--base")
    p = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    return p.stdout


RE_ADDR = re.compile(r"^\s*([0-9a-fA-F]{4,8}):?\s")


def parse(text):
    """Return list of (text, has_reloc). Reloc lines attach to previous row."""
    rows = []
    for ln in text.splitlines():
        s = ln.rstrip()
        if not s.strip():
            continue
        if s.startswith("[disasm") or re.match(r"^[0-9a-f]{8} <", s):
            continue
        if "IMAGE_REL" in s or s.lstrip().startswith("->"):
            if rows:
                rows[-1] = (rows[-1][0], True)
            continue
        rows.append((s, False))
    # drop trailing alignment padding (nop / int3 / lea esp,[esp])
    while rows and re.search(r"\b(nop|int3)\b", rows[-1][0]):
        rows.pop()
    return rows


def norm(s):
    # drop leading address / byte columns: "  0044b960: 55  push ebp"
    s = re.sub(r"^\s*[0-9a-fA-F]+:\s*", "", s)
    s = re.sub(r"^(?:[0-9a-fA-F]{2}\s)+", "", s)
    s = s.strip()
    # normalize branch/call targets
    s = re.sub(r"\b(j[a-z]+|call|loop\w*)\s+.*$", r"\1 <T>", s)
    return s


def blank_lit(s):
    return re.sub(r"0x[0-9a-fA-F]+|\b\d+\b", "#", s)


def main():
    va = sys.argv[1]
    show = 0
    if "--show" in sys.argv:
        show = int(sys.argv[sys.argv.index("--show") + 1])
    t_raw, b_raw = run(va, False), run(va, True)
    T, B = parse(t_raw), parse(b_raw)
    tn = [norm(x[0]) for x in T]
    bn = [norm(x[0]) for x in B]
    sm = difflib.SequenceMatcher(None, tn, bn, autojunk=False)
    real = art = 0
    shown = []
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            continue
        ti, bi = list(range(i1, i2)), list(range(j1, j2))
        n = max(len(ti), len(bi))
        for k in range(n):
            a = tn[ti[k]] if k < len(ti) else ""
            b = bn[bi[k]] if k < len(bi) else ""
            ar = T[ti[k]][1] if k < len(ti) else False
            br = B[bi[k]][1] if k < len(bi) else False
            if a and b and blank_lit(a) == blank_lit(b) and (ar or br):
                art += 1
            else:
                real += 1
                if len(shown) < show:
                    shown.append(f"  R {a!r:60} | {b!r}")
    print(f"va={va} retail_rows={len(T)} ours={len(B)} "
          f"delta={len(B)-len(T):+d} real={real} artefact={art}")
    for s in shown:
        print(s)


if __name__ == "__main__":
    main()
