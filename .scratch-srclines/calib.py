#!/usr/bin/env python3
"""Calibrate: our statement lines vs DC census lines, over exact functions."""
import os
import pickle
import re
import subprocess
import sys
import bisect

ROOT = "/home/sheep/Projects/homm3/wt-cold-windows"
DUMP = "/home/sheep/Projects/homm3/homm3-symbols/HoMM3-Dreamcast-Dump/dump.txt"
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import stmt as S
import srclines as SL

# --- DC proc table: offset -> Cb
PROC = {}
RE_P = re.compile(r"S_GPROC32: \[0001:([0-9A-F]{8})\], Cb: ([0-9A-F]{8})")
cache = os.path.join(HERE, "procs.pkl")
if os.path.exists(cache):
    PROC = pickle.load(open(cache, "rb"))
else:
    for ln in open(DUMP, errors="replace"):
        m = RE_P.search(ln)
        if m:
            PROC[int(m.group(1), 16)] = int(m.group(2), 16)
    pickle.dump(PROC, open(cache, "wb"))

RECS = SL.load()
KEYS = [r[0] for r in RECS]


def dc_lines(dc, cb):
    lo = bisect.bisect_left(KEYS, dc)
    hi = bisect.bisect_left(KEYS, dc + cb)
    per = {}
    for addr, line, f, mod in RECS[lo:hi]:
        per.setdefault(f, set()).add(line)
    return per


# --- baseline scores
score = {}
for ln in open(os.path.join(ROOT, "config/match_baseline.tsv")):
    if ln.startswith("#"):
        continue
    p = ln.rstrip("\n").split("\t")
    if len(p) >= 3 and p[0] != "unit":
        try:
            score[(p[0], p[1])] = float(p[2])
        except ValueError:
            pass

RE_VA = re.compile(r"^VA(?:_\w+)?\((0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+)"
                   r".*?\bdc (0x[0-9a-fA-F]+)")


def spans(path):
    """Yield (va, cb, dc, start_line, end_line) for each VA-annotated body."""
    lines = open(path, errors="replace").read().splitlines()
    hits = []
    for i, ln in enumerate(lines):
        m = RE_VA.match(ln)
        if m:
            hits.append((i + 1, int(m.group(1), 16), int(m.group(2), 16),
                         int(m.group(3), 16)))
    txt = S.strip_comments("\n".join(lines)).splitlines()
    for k, (ln0, va, cb, dc) in enumerate(hits):
        # brace-balance from the first '{' at or after the VA line
        depth = 0
        started = False
        end = len(lines)
        for j in range(ln0, len(lines)):
            s = re.sub(r'"(\\.|[^"\\])*"', '""',
                       re.sub(r"'(\\.|[^'\\])*'", "''", txt[j]))
            for ch in s:
                if ch == "{":
                    depth += 1
                    started = True
                elif ch == "}":
                    depth -= 1
            if started and depth <= 0:
                end = j + 1
                break
        yield va, cb, dc, ln0, end


def main():
    import glob
    out = []
    for path in sorted(glob.glob(os.path.join(ROOT, "src", "*.cpp"))):
        unit = os.path.basename(path)[:-4]
        for va, cb, dc, a, b in spans(path):
            cbdc = PROC.get(dc)
            if not cbdc:
                continue
            per = dc_lines(dc, cbdc)
            n_dc = sum(len(v) for v in per.values())
            if not n_dc:
                continue
            sys.stdout = open(os.devnull, "w")
            try:
                raw = S.load(path, a, b)
                body = S.strip_comments("\n".join(raw)).splitlines()
                n = 0
                prev_open = True
                for lnx in body:
                    s = lnx.strip()
                    if not s or s.startswith("#"):
                        continue
                    core = s.strip("{}();, \t")
                    if not core:
                        prev_open = True
                        continue
                    if re.fullmatch(r"(case\s+[^:]+|default|\w+)\s*:", s):
                        prev_open = True
                        continue
                    if prev_open:
                        n += 1
                    prev_open = bool(re.search(r"[;{}]\s*$", s)) or s.endswith(":")
            finally:
                sys.stdout = sys.__stdout__
            # find score by va
            sc = None
            for (u, fn), v in score.items():
                pass
            out.append((unit, hex(va), hex(dc), n, n_dc, len(per), a, b))
    # score lookup by va from baseline col 6
    vamap = {}
    for ln in open(os.path.join(ROOT, "config/match_baseline.tsv")):
        if ln.startswith("#"):
            continue
        p = ln.rstrip("\n").split("\t")
        if len(p) >= 6:
            try:
                vamap[int(p[5], 16) + 0x400000] = (float(p[2]), p[1])
            except ValueError:
                pass
    ex = []
    print("unit\tva\tdc\tours\tdc_lines\tratio\tscore")
    for unit, va, dc, n, n_dc, nf, a, b in out:
        s, fn = vamap.get(int(va, 16), (None, ""))
        r = n / n_dc if n_dc else 0
        print(f"{unit}\t{va}\t{dc}\t{n}\t{n_dc}\t{r:.3f}\t{s}")
        if s == 100.0:
            ex.append(r)
    if ex:
        ex.sort()
        print(f"# EXACT n={len(ex)} median={ex[len(ex)//2]:.3f} "
              f"mean={sum(ex)/len(ex):.3f} "
              f"q1={ex[len(ex)//4]:.3f} q3={ex[3*len(ex)//4]:.3f}",
              file=sys.stderr)


if __name__ == "__main__":
    main()
