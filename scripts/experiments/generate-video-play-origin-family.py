#!/usr/bin/env python3
"""Retail videoPlay centered-origin ownership and assignment family."""
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

x_pair = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;"""
y_pair = """            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
assert body.count("    POINT pos;\n") == 1
assert body.count(x_pair) == 1
assert body.count(y_pair) == 1


def chain(changed, x_name, y_name):
    changed = changed.replace(
        x_pair.replace("pos.x", x_name),
        "            g_smackX = %s = x + (vw - g_smackVideo->m_width) / 2;" % x_name)
    return changed.replace(
        y_pair.replace("pos.y", y_name),
        "            g_smackY = %s = y + (vh - g_smackVideo->m_height) / 2;" % y_name)


options = [{"name": "point-separate"}]
options.append({"name": "point-chained", "replace": chain(body, "pos.x", "pos.y")})

parameters = body.replace("    POINT pos;\n", "")
parameters = parameters.replace("pos.x", "x").replace("pos.y", "y")
options.append({"name": "parameters-separate", "replace": parameters})
options.append({"name": "parameters-chained", "replace": chain(parameters, "x", "y")})

for typ in ("int", "long"):
    for order in ("xy", "yx"):
        names = (("drawX", "drawY") if order == "xy" else ("drawY", "drawX"))
        declaration = "    %s %s, %s;\n" % (typ, names[0], names[1])
        changed = body.replace("    POINT pos;\n", declaration)
        changed = changed.replace("pos.x", "drawX").replace("pos.y", "drawY")
        options.append({"name": "%s-scalars-%s-separate" % (typ, order), "replace": changed})
        options.append({"name": "%s-scalars-%s-chained" % (typ, order),
                        "replace": chain(changed, "drawX", "drawY")})

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "centered-origin-ownership",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub; retail 0x5972d0 alone supplies the Complete geometry lifetime.",
        "Retail computes the centered x in EAX, stores g_smackX, and later homes the value at ebp-0xc; it similarly homes y at ebp-0x8. Both values feed SmackToBuffer and the later updateScreen.",
        "The current POINT lets VC6 keep pos.x in EBX. Test whether the real owner was the mutable x/y parameter pair or two ordinary signed coordinate locals, and whether the simultaneous global/local assignment was one chained source statement.",
        "Every variant preserves the centered arithmetic, global stores, API arguments, later update rectangle, conditions, and calls.",
        "Twelve finite representations; no padding, dummy use, alternate API, or compiler directive.",
    ],
}, indent=2) + "\n")
