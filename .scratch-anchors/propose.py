import csv, re, sys
from pathlib import Path
SRC = Path("src")
_ORIGIN = re.compile(r"^//\s+(\S+?):(\d+)\s*$")
_VA = re.compile(r"^VA\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)")
_DC = re.compile(r"\bdc\s+(0x[0-9a-fA-F]+)")

dc = {}
dcname = {}
with open("evidence/dreamcast/functions.csv") as fh:
    for r in csv.DictReader(l for l in fh if not l.startswith("#")):
        off = int(r["offset"], 16)
        dc[off] = (r["file"], r["line"], r["name"], r["module"], int(r["cb"]))
        dcname.setdefault((r["module"].lower(), r["name"]), []).append(
            (r["file"], r["line"], off, int(r["cb"])))

units = sys.argv[1:]
for u in units:
    p = SRC / f"{u}.cpp"
    lines = p.read_text(errors="replace").splitlines()
    own = f"{u}.cpp".lower()
    print(f"=== {u}")
    for i, l in enumerate(lines):
        m = _VA.match(l)
        if not m: continue
        if i > 0 and _ORIGIN.match(lines[i-1]): continue
        va = int(m.group(1), 16); sz = int(m.group(2), 0)
        md = _DC.search(l)
        # declarator: next non-blank, non-comment line
        decl = ""
        for j in range(i+1, min(i+6, len(lines))):
            t = lines[j].strip()
            if t and not t.startswith("//"):
                decl = t; break
        if md:
            off = int(md.group(1), 16)
            e = dc.get(off)
            if e:
                mod_ok = e[3].lower() == f"{u}.obj"
                print(f"  L{i+1} 0x{va:08x} {sz:#x} -> {e[0]}:{e[1]}  [{e[2]}] cb={e[4]:#x} mod={e[3]}{'' if mod_ok else '  !!MODULE-MISMATCH'}{'' if e[4]==sz else '  !!SIZE '+hex(e[4])}")
            else:
                print(f"  L{i+1} 0x{va:08x} {sz:#x} -> DC OFFSET {md.group(1)} NOT FOUND | {decl[:70]}")
        else:
            print(f"  L{i+1} 0x{va:08x} {sz:#x} -> NO-DC-NOTE | {decl[:80]}")
