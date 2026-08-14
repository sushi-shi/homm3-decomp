#!/usr/bin/env python3
"""Statement census: parse *** SRCLINES *** from the DC CodeView dump.

Usage:
  srclines.py index                       # build the pickle index
  srclines.py q <dc_hex> <cb_hex>         # lines in [dc, dc+cb)
  srclines.py proc <name-substr>          # find S_GPROC32 offsets/Cb
"""
import bisect
import os
import pickle
import re
import sys

DUMP = "/home/sheep/Projects/homm3/homm3-symbols/HoMM3-Dreamcast-Dump/dump.txt"
IDX = os.path.join(os.path.dirname(os.path.abspath(__file__)), "srclines.pkl")

RE_MOD = re.compile(r"^\s*\*\*\* Module (\S+) at ([0-9A-F]+)")
RE_FILE = re.compile(
    r"^\s*(\S.*?),\s*0001:([0-9A-F]{8})-([0-9A-F]{8}), line/addr pairs = (\d+)")
RE_PAIR = re.compile(r"(\d+)\s+([0-9A-F]{8})")


def build():
    recs = []          # (addr, line, file, module)
    with open(DUMP, "r", errors="replace") as fh:
        # fast-forward to SRCLINES
        for ln in fh:
            if ln.startswith("*** SRCLINES ***"):
                break
        module = None
        cur_file = None
        for ln in fh:
            if ln.startswith("*** ") and "SRCLINES" not in ln:
                break
            m = RE_MOD.match(ln)
            if m:
                module = os.path.basename(m.group(1).replace("\\", "/"))
                cur_file = None
                continue
            m = RE_FILE.match(ln)
            if m and module:
                cur_file = m.group(1)
                continue
            if cur_file and module:
                for line, addr in RE_PAIR.findall(ln):
                    recs.append((int(addr, 16), int(line), cur_file, module))
    recs.sort()
    with open(IDX, "wb") as fh:
        pickle.dump(recs, fh)
    print(f"{len(recs)} line records", file=sys.stderr)
    return recs


def load():
    if not os.path.exists(IDX):
        return build()
    with open(IDX, "rb") as fh:
        return pickle.load(fh)


def query(dc, cb):
    recs = load()
    keys = [r[0] for r in recs]
    lo = bisect.bisect_left(keys, dc)
    hi = bisect.bisect_left(keys, dc + cb)
    return recs[lo:hi]


def main():
    if sys.argv[1] == "index":
        build()
        return
    if sys.argv[1] == "q":
        dc = int(sys.argv[2], 16)
        cb = int(sys.argv[3], 16)
        rows = query(dc, cb)
        byfile = {}
        for addr, line, f, mod in rows:
            byfile.setdefault(f, []).append((line, addr))
        for f, ls in sorted(byfile.items(), key=lambda kv: -len(kv[1])):
            uniq = sorted(set(l for l, _ in ls))
            print(f"{f}: {len(ls)} pairs, {len(uniq)} unique lines "
                  f"[{uniq[0]}..{uniq[-1]}]")
            print("   ", " ".join(str(x) for x in uniq))
        tot = sum(len(set(l for l, _ in ls)) for ls in byfile.values())
        print(f"TOTAL unique lines across files: {tot}")
        return
    if sys.argv[1] == "proc":
        pat = sys.argv[2]
        with open(DUMP, "r", errors="replace") as fh:
            for ln in fh:
                if "S_GPROC32" in ln and pat in ln:
                    print(ln.rstrip())
        return


if __name__ == "__main__":
    main()
