"""Insert `// <origin>` lines above unanchored VA() claims.

Sources of truth, in order:
  1. the claim's own `dc 0x...` note -> exact DC roster row (file+line)
  2. declarator name looked up in the unit's own .obj, UNIQUE only (table below)
"""
import csv, re, sys
from pathlib import Path

SRC = Path("src")
_ORIGIN = re.compile(r"^//\s+(\S+?):(\d+)\s*$")
_VA = re.compile(r"^VA\(\s*(0x[0-9a-fA-F]+)\s*,")
_DC = re.compile(r"\bdc\s+(0x[0-9a-fA-F]+)")

dc = {}
with open("evidence/dreamcast/functions.csv") as fh:
    for r in csv.DictReader(l for l in fh if not l.startswith("#")):
        dc[int(r["offset"], 16)] = (r["file"], r["line"])

# declarator-resolved (unique in the unit's own .obj) - va -> origin
BYNAME = {
    0x005f5340: r"E:\gamedcs\viewarmywindow.cpp:671",
    0x005f55d0: r"E:\gamedcs\viewarmywindow.cpp:689",
    0x005f5dd0: r"E:\gamedcs\viewarmywindow.cpp:756",
    0x005f6070: r"E:\gamedcs\viewarmywindow.cpp:774",
    0x005f62f0: r"E:\gamedcs\viewarmywindow.cpp:789",
    0x004f79b0: r"E:\gamedcs\kb.cpp:5954",
    0x004f79e0: r"E:\gamedcs\kb.cpp:5960",
    0x0047c430: r"E:\gamedcs\cspriteframe.cpp:202",
    0x0055fd10: r"E:\gamedcs\sacrifice_window.cpp:213",
    0x0055fd40: r"E:\gamedcs\sacrifice_window.cpp:243",
    0x0049f040: r"E:\gamedcs\events.cpp:647",
    0x004a0c20: r"E:\gamedcs\events.cpp:1114",
    0x00522f30: r"E:\gamedcs\palette.cpp:640",
    0x0045ad00: r"E:\gamedcs\campaignbrief.cpp:192",
    0x0044ed50: r"E:\gamedcs\bitmap24.cpp:64",
    0x0044ee00: r"E:\gamedcs\bitmap24.cpp:80",
    0x0044efd0: r"E:\gamedcs\bitmap24.cpp:274",
}
# left alone deliberately (ambiguous / no roster row)
SKIP = {
    0x005578d0,  # remote  ~CNetMsgHandler - dc note 0x201f4 has no roster row
    0x0044e240, 0x0044fe30, 0x0047bd50, 0x00522b40, 0x00522f70, 0x0044f180,
    0x0047c560,  # _vslot2 working names - no roster declarator
    0x005bbe20, 0x005bc160,  # GetSize working names
    0x00522e30,  # 4th TPalette24 ctor - three roster ctors, none free
    0x00599b90, 0x00599c40,  # Resume/PauseSamples - absent from the DC roster
    0x004276c0,  # type_monster_vector copy ctor - absent from the DC roster
}

units = sys.argv[1:]
added = skipped = 0
for u in units:
    p = SRC / f"{u}.cpp"
    lines = p.read_text(errors="replace").splitlines(keepends=True)
    out = []
    for i, l in enumerate(lines):
        m = _VA.match(l)
        if m and not (out and _ORIGIN.match(out[-1].rstrip("\n"))):
            va = int(m.group(1), 16)
            origin = None
            if va in SKIP:
                skipped += 1
            elif va in BYNAME:
                origin = BYNAME[va]
            else:
                md = _DC.search(l)
                if md and int(md.group(1), 16) in dc:
                    f, ln = dc[int(md.group(1), 16)]
                    origin = f"{f}:{ln}"
                else:
                    skipped += 1
            if origin:
                out.append(f"// {origin}\n")
                added += 1
        out.append(l)
    p.write_text("".join(out))
print(f"added {added} origin lines, left {skipped} alone")
