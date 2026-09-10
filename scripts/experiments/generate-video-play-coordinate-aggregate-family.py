#!/usr/bin/env python3
"""Retail videoPlay Win32 coordinate aggregate family."""
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
assert body.count("    POINT pos;") == 1
assert body.count("pos.x") == 4
assert body.count("pos.y") == 4

forms = [
    ("POINT", "POINT", "pos.x", "pos.y"),
    ("POINTL", "POINTL", "pos.x", "pos.y"),
    ("tagPOINT", "tagPOINT", "pos.x", "pos.y"),
    ("struct-tagPOINT", "struct tagPOINT", "pos.x", "pos.y"),
    ("struct-_POINTL", "struct _POINTL", "pos.x", "pos.y"),
    ("SIZE", "SIZE", "pos.cx", "pos.cy"),
    ("tagSIZE", "tagSIZE", "pos.cx", "pos.cy"),
    ("struct-tagSIZE", "struct tagSIZE", "pos.cx", "pos.cy"),
    ("RECT-left-top", "RECT", "pos.left", "pos.top"),
    ("tagRECT-left-top", "tagRECT", "pos.left", "pos.top"),
    ("long-array", "long", "pos[0]", "pos[1]"),
]

options = []
for name, typ, xmember, ymember in forms:
    declaration = "    long pos[2];" if name == "long-array" else "    %s pos;" % typ
    changed = body.replace("    POINT pos;", declaration)
    changed = changed.replace("pos.x", xmember).replace("pos.y", ymember)
    option = {"name": name}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "coordinate-aggregate",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 has no Complete-body locals. Retail 0x5972d0 proves two adjacent signed 32-bit centered-coordinate homes consumed as x/y values, but does not identify their source aggregate tag.",
        "Test the ABI-equivalent Win32 two-coordinate aggregates POINT, POINTL, and SIZE, plus RECT's leading pair and a plain long pair. Their real members carry every observed coordinate use and can change the C1 type/value creation state without changing the API.",
        "Eleven finite states preserve all arithmetic, global stores, draw/update arguments, calls, branches, and helper boundaries; no dummy object or compiler directive.",
    ],
}, indent=2) + "\n")
