#!/usr/bin/env python3
"""Retail videoPlay global immediate origin versus deferred POINT."""
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
call = "_SmackToBuffer(g_smackVideo, pos.x, pos.y,"
assert body.count(coordinates) == 1
assert body.count(call) == 1

flows = {
    "local-then-global": coordinates,
    "global-then-local": """            g_smackX = x + (vw - g_smackVideo->m_width) / 2;
            pos.x = g_smackX;
            g_smackY = y + (vh - g_smackVideo->m_height) / 2;
            pos.y = g_smackY;""",
    "both-globals-then-locals": """            g_smackX = x + (vw - g_smackVideo->m_width) / 2;
            g_smackY = y + (vh - g_smackVideo->m_height) / 2;
            pos.x = g_smackX;
            pos.y = g_smackY;""",
    "global-first-chain": """            g_smackX = pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackY = pos.y = y + (vh - g_smackVideo->m_height) / 2;""",
    "local-first-chain": """            pos.x = g_smackX = x + (vw - g_smackVideo->m_width) / 2;
            pos.y = g_smackY = y + (vh - g_smackVideo->m_height) / 2;""",
}

options = [{"name": "point-immediate-control"}]
for name, flow in flows.items():
    changed = body.replace(coordinates, flow).replace(
        call, "_SmackToBuffer(g_smackVideo, g_smackX, g_smackY,")
    options.append({"name": name + "-global-immediate", "replace": changed})

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "immediate-global-origin", "find": body,
              "options": options}],
    "evidence": [
        "Retail stores g_smackX/g_smackY immediately before SmackToBuffer and passes the same two values while they remain live in registers; it cannot distinguish local-member from global source arguments at that call.",
        "The deferred updateScreen reads two private stack homes after the loop. Test the natural split in which the Smacker SDK call consumes its global origin while the POINT retains the deferred redraw rectangle.",
        "All forms retain one computation and one global store per axis, the POINT, every call and branch, and both public interfaces.",
    ],
}, indent=2) + "\n")
