#!/usr/bin/env python3
"""Retail videoPlay global/local coordinate assignment dataflow."""
import argparse
import json
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
start = source.index("int videoPlay(int id, int x, int y, int w, int h)\n{")
body = source[start:source.index("\n}\n", start) + 2]

pair = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
assert body.count(pair) == 1

forms = [
    ("local-then-global", pair),
    ("global-then-local", """            g_smackX = x + (vw - g_smackVideo->m_width) / 2;
            pos.x = g_smackX;
            g_smackY = y + (vh - g_smackVideo->m_height) / 2;
            pos.y = g_smackY;"""),
    ("both-globals-then-locals", """            g_smackX = x + (vw - g_smackVideo->m_width) / 2;
            g_smackY = y + (vh - g_smackVideo->m_height) / 2;
            pos.x = g_smackX;
            pos.y = g_smackY;"""),
    ("chained-global-first", """            g_smackX = pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackY = pos.y = y + (vh - g_smackVideo->m_height) / 2;"""),
    ("chained-local-first", """            pos.x = g_smackX = x + (vw - g_smackVideo->m_width) / 2;
            pos.y = g_smackY = y + (vh - g_smackVideo->m_height) / 2;"""),
]

options = []
for name, replacement in forms:
    option = {"name": name}
    if replacement != pair:
        option["replace"] = body.replace(pair, replacement)
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "coordinate-assignment-flow",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 has no Complete body; retail 0x5972d0 is authoritative for this assignment interval.",
        "Retail computes each center, stores g_smackX/g_smackY, and later spills the retained draw coordinates at ebp-0xc/ebp-0x8 before SmackToBuffer.",
        "The current local-first POINT form keeps x in EBX through the loop. Test equivalent global-first copy and chained forms that preserve both global side effects and local values across calls.",
        "Five finite states preserve arithmetic, API arguments, update rectangle, branches, and call order.",
    ],
}, indent=2) + "\n")
