import re, sys
from pathlib import Path
SRC = Path("src")
_ORIGIN = re.compile(r"^//\s+(\S+?):(\d+)\s*$")
_VA = re.compile(r"^VA\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+|\d+)")
rows=[]
for p in sorted(SRC.glob("*.cpp")):
    lines = p.read_text(errors="replace").splitlines()
    own = f"{p.stem}.cpp".lower()
    tot=0; anch=0; un=[]
    for i,l in enumerate(lines):
        m=_VA.match(l)
        if not m: continue
        tot+=1
        prev = lines[i-1] if i>0 else ""
        mo=_ORIGIN.match(prev)
        if mo:
            f=mo.group(1).replace("\\","/").rsplit("/",1)[-1].lower()
            if f==own: anch+=1
            else: pass
            continue
        un.append((i+1, l.strip()))
    if un:
        rows.append((p.stem, tot, anch, un))
for stem,tot,anch,un in sorted(rows, key=lambda r:-len(r[3])):
    print(f"=== {stem}: {tot} claims, {anch} anchored, {len(un)} unanchored")
    for ln,txt in un:
        print(f"   {ln}: {txt}")
