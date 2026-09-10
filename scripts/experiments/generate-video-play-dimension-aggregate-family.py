#!/usr/bin/env python3
"""Retail videoPlay mutable dimension aggregate family."""
import argparse
import json
import re
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
start = source.index("int videoPlay(int id, int x, int y, int w, int h)\n{")
body = source[start:source.index("\n}\n", start) + 2]
decl = "    int vw, vh;"
assert body.count(decl) == 1
assert len(re.findall(r"\bvw\b", body)) == 7
assert len(re.findall(r"\bvh\b", body)) == 7

forms = [
    ("scalar-control", decl, "vw", "vh"),
    ("SIZE", "    SIZE videoSize;", "videoSize.cx", "videoSize.cy"),
    ("SIZEL", "    SIZEL videoSize;", "videoSize.cx", "videoSize.cy"),
    ("tagSIZE", "    tagSIZE videoSize;", "videoSize.cx", "videoSize.cy"),
    ("struct-tagSIZE", "    struct tagSIZE videoSize;", "videoSize.cx", "videoSize.cy"),
    ("POINT", "    POINT videoSize;", "videoSize.x", "videoSize.y"),
    ("POINTL", "    POINTL videoSize;", "videoSize.x", "videoSize.y"),
    ("int-array", "    int videoSize[2];", "videoSize[0]", "videoSize[1]"),
    ("long-array", "    long videoSize[2];", "videoSize[0]", "videoSize[1]"),
    ("RECT-right-bottom", "    RECT videoSize;", "videoSize.right", "videoSize.bottom"),
]

options = []
for name, declaration, width, height in forms:
    changed = body.replace(decl, declaration)
    changed = re.sub(r"\bvw\b", width, changed)
    changed = re.sub(r"\bvh\b", height, changed)
    option = {"name": name}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "mutable-dimension-aggregate",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves signed int w/h parameters but has no Complete-body locals. Retail 0x5972d0 proves two independently mutable signed 32-bit extents copied from them, passed to ShowVideo, repaired from the Smacker fields when negative, and reused for the final update.",
        "Those values can be source-owned as two scalar ints or an ABI-equivalent Win32 SIZE/POINT pair. Test the real aggregate members through every observed use; VC6 may scalar-replace them into the same incoming parameter homes while assigning a different C1 value order.",
        "Ten finite states preserve values, signed tests, calls, branches, the coordinate POINT, helper boundaries, and interfaces; no dummy member or compiler directive.",
    ],
}, indent=2) + "\n")
