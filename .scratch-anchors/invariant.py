"""Within a TU, retail address order must reproduce DC source line order."""
import re, sys
from pathlib import Path
SRC = Path("src")
_ORIGIN = re.compile(r"^//\s+(\S+?):(\d+)\s*$")
_VA = re.compile(r"^VA\(\s*(0x[0-9a-fA-F]+)\s*,")
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
        f = mo.group(1).replace("\\", "/").rsplit("/", 1)[-1].lower()
        if f != own: continue
        seq.append((int(m.group(1), 16), int(mo.group(2))))
    if len(seq) < 2: continue
    seq.sort()
    tot += len(seq)
    bad = [(a, b) for (a, _), (b_a, b) in zip(seq, seq[1:]) for _ in [0] if False]
    prev_a, prev_l = seq[0]
    for a, l in seq[1:]:
        if l < prev_l:
            inv += 1
            print(f"  INVERSION {p.stem}: 0x{prev_a:x}:L{prev_l} -> 0x{a:x}:L{l}")
        prev_a, prev_l = a, l
print(f"{tot} own-cpp anchors checked, {inv} inversions")
