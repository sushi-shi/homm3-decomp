"""Same invariant, but against DC EMISSION order (offset) not source line."""
import csv, re
from pathlib import Path
SRC = Path("src")
_ORIGIN = re.compile(r"^//\s+(\S+?):(\d+)\s*$")
_VA = re.compile(r"^VA\(\s*(0x[0-9a-fA-F]+)\s*,")
_DC = re.compile(r"\bdc\s+(0x[0-9a-fA-F]+)")
dc = {}
with open("evidence/dreamcast/functions.csv") as fh:
    for r in csv.DictReader(l for l in fh if not l.startswith("#")):
        dc[int(r["offset"], 16)] = (r["file"], r["line"], r["name"])
tot = inv = 0
for p in sorted(SRC.glob("*.cpp")):
    own = f"{p.stem}.cpp".lower()
    lines = p.read_text(errors="replace").splitlines()
    seq = []
    for i, l in enumerate(lines):
        m = _VA.match(l)
        if not m or i == 0: continue
        mo = _ORIGIN.match(lines[i-1])
        if not mo: continue
        if mo.group(1).replace("\\", "/").rsplit("/", 1)[-1].lower() != own: continue
        md = _DC.search(l)
        if not md: continue
        seq.append((int(m.group(1), 16), int(md.group(1), 16)))
    if len(seq) < 2: continue
    seq.sort(); tot += len(seq)
    pa, pd = seq[0]
    for a, d in seq[1:]:
        if d < pd:
            inv += 1
            print(f"  INV {p.stem}: 0x{pa:x}/dc {pd:#x} {dc.get(pd,('','',''))[2]}"
                  f"  ->  0x{a:x}/dc {d:#x} {dc.get(d,('','',''))[2]}")
        pa, pd = a, d
print(f"{tot} dc-noted own-cpp anchors checked, {inv} DC-emission-order inversions")
