#!/usr/bin/env python3
"""Retail videoPlay coordinate computation and global-store grouping."""
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
    ("interleaved-control", coordinates),
    ("compute-pair-then-store-pair", """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackX = pos.x;
            g_smackY = pos.y;"""),
    ("aggregate-initialize-then-store", """            POINT centered = {
                x + (vw - g_smackVideo->m_width) / 2,
                y + (vh - g_smackVideo->m_height) / 2
            };
            pos = centered;
            g_smackX = pos.x;
            g_smackY = pos.y;"""),
    ("aggregate-initialize-owner", """            POINT centered = {
                x + (vw - g_smackVideo->m_width) / 2,
                y + (vh - g_smackVideo->m_height) / 2
            };
            g_smackX = centered.x;
            g_smackY = centered.y;"""),
    ("x-local-y-global-then-x-global", """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackY = y + (vh - g_smackVideo->m_height) / 2;
            g_smackX = pos.x;
            pos.y = g_smackY;"""),
]

options = []
for name, replacement in forms:
    changed = body.replace(coordinates, replacement)
    if name == "aggregate-initialize-owner":
        changed = changed.replace("    POINT pos;\n", "")
        changed = changed.replace("pos.x", "centered.x").replace("pos.y", "centered.y")
    option = {"name": name}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "coordinate-computation-grouping",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast's platform stub has no Complete-body locals. Retail computes both centered values, stores the two Smacker globals, and retains an adjacent pair for the draw and deferred update.",
        "Prior families interleaved each local computation with its global store or computed globals before local copies. They did not test one POINT computation group followed by the two observable global stores, or its ordinary aggregate-initializer form.",
        "C2 may legally schedule the independent y computation around the g_smackX store when aliasing is resolved. These finite forms preserve the final values, calls, branches, and interfaces; the reversed-store control is diagnostic only.",
    ],
}, indent=2) + "\n")
