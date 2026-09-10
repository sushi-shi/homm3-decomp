#!/usr/bin/env python3
"""Retail videoPlay immediate scalar coordinates and deferred POINT snapshot."""
import argparse
import itertools
import json
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
start = source.index("int videoPlay(int id, int x, int y, int w, int h)\n{")
body = source[start:source.index("\n}\n", start) + 2]

declarations = """    POINT pos;
    int vw, vh;
    unsigned char result;
    unsigned char aborted;"""
coordinates = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
call = "_SmackToBuffer(g_smackVideo, pos.x, pos.y,"
for anchor in (declarations, coordinates, call):
    assert body.count(anchor) == 1

forms = [("control", body)]
for typ, placement, order, immediate in itertools.product(
        ("int", "long"), ("function", "inner"), ("xy", "yx"),
        ("scalars", "point")):
    changed = body
    if placement == "function":
        names = "centerX, centerY" if order == "xy" else "centerY, centerX"
        changed = changed.replace(declarations, declarations + "\n    %s %s;" % (typ, names))
        x_assignment = "            centerX = x + (vw - g_smackVideo->m_width) / 2;"
        y_assignment = "            centerY = y + (vh - g_smackVideo->m_height) / 2;"
    else:
        x_assignment = "            %s centerX = x + (vw - g_smackVideo->m_width) / 2;" % typ
        y_assignment = "            %s centerY = y + (vh - g_smackVideo->m_height) / 2;" % typ
    assignments = {
        "x": x_assignment + "\n            g_smackX = centerX;",
        "y": y_assignment + "\n            g_smackY = centerY;",
    }
    replacement = assignments[order[0]] + "\n" + assignments[order[1]] + """
            pos.x = centerX;
            pos.y = centerY;"""
    changed = changed.replace(coordinates, replacement)
    if immediate == "scalars":
        changed = changed.replace(call, "_SmackToBuffer(g_smackVideo, centerX, centerY,")
    forms.append(("%s-%s-%s-draw-%s" % (typ, placement, order, immediate), changed))

options = []
for name, changed in forms:
    option = {"name": name}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "immediate-scalars-deferred-point", "find": body, "options": options}],
    "evidence": [
        "Dreamcast's platform stub has no Complete local inventory; retail 0x5972d0 supplies the coordinate lifetimes.",
        "Retail keeps x in EBX, computes the immediate centers in EAX/ECX, stores both globals, then homes the same values at ebp-c/ebp-8 before SmackToBuffer.",
        "Separate immediate scalar owners plus a deferred POINT snapshot can lower the POINT members' register priority while preserving the exact draw and later update values.",
        "The finite family tests signed four-byte spellings, declaration boundaries/order, and which equivalent owner feeds the immediate SDK call; all operations are meaningful.",
    ],
}, indent=2) + "\n")
