#!/usr/bin/env python3
"""Retail videoPlay parameter update combined with POINT storage."""
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

coordinates = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
assert body.count(coordinates) == 1

forms = [
    ("control", coordinates),
    ("update-params-then-copy", """            x += (vw - g_smackVideo->m_width) / 2;
            g_smackX = x;
            pos.x = x;
            y += (vh - g_smackVideo->m_height) / 2;
            g_smackY = y;
            pos.y = y;"""),
    ("update-params-copy-after-both", """            x += (vw - g_smackVideo->m_width) / 2;
            g_smackX = x;
            y += (vh - g_smackVideo->m_height) / 2;
            g_smackY = y;
            pos.x = x;
            pos.y = y;"""),
    ("point-from-compound-param", """            pos.x = (x += (vw - g_smackVideo->m_width) / 2);
            g_smackX = pos.x;
            pos.y = (y += (vh - g_smackVideo->m_height) / 2);
            g_smackY = pos.y;"""),
    ("param-from-point", """            x = pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            y = pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""),
    ("global-point-param-chain", """            g_smackX = pos.x = (x += (vw - g_smackVideo->m_width) / 2);
            g_smackY = pos.y = (y += (vh - g_smackVideo->m_height) / 2);"""),
]

options = []
for name, replacement in forms:
    option = {"name": name}
    if replacement != coordinates:
        option["replace"] = body.replace(coordinates, replacement)
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "parameter-update-and-point-storage",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 has no Complete-body local evidence; retail 0x5972d0 supplies the parameter register and POINT stack evidence.",
        "Updating x/y directly gives the exact retail callee-saved entry permutation x=EBX, vw=ESI, vh=EDI, but loses the retail 0x4c frame and POINT homes. Keeping the actual POINT as the draw/update owner can distinguish whether parameter updates feed that aggregate.",
        "Test direct copies and equivalent assignment expressions. All variants retain POINT, global coordinate writes, centered arithmetic, draw/update arguments, calls, and branches.",
        "Six finite states; no dummy object, API change, or compiler directive.",
    ],
}, indent=2) + "\n")
