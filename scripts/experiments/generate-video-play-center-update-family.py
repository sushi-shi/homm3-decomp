#!/usr/bin/env python3
"""Retail videoPlay centered-coordinate update forms."""
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

x_line = "            pos.x = x + (vw - g_smackVideo->m_width) / 2;"
y_line = "            pos.y = y + (vh - g_smackVideo->m_height) / 2;"
assert body.count(x_line) == 1
assert body.count(y_line) == 1


def forms(member, origin, extent, field):
    delta = "(%s - g_smackVideo->%s)" % (extent, field)
    indent = "            "
    return {
        "sum-divide": "%s%s = %s + %s / 2;" % (indent, member, origin, delta),
        "sum-shift": "%s%s = %s + (%s >> 1);" % (indent, member, origin, delta),
        "delta-divide-add": "%s%s = %s / 2;\n%s%s += %s;" % (
            indent, member, delta, indent, member, origin),
        "delta-shift-add": "%s%s = %s >> 1;\n%s%s += %s;" % (
            indent, member, delta, indent, member, origin),
        "origin-add-divide": "%s%s = %s;\n%s%s += %s / 2;" % (
            indent, member, origin, indent, member, delta),
        "origin-add-shift": "%s%s = %s;\n%s%s += %s >> 1;" % (
            indent, member, origin, indent, member, delta),
    }


x_forms = forms("pos.x", "x", "vw", "m_width")
y_forms = forms("pos.y", "y", "vh", "m_height")
options = []
for x_name, y_name in itertools.product(x_forms, y_forms):
    changed = body.replace(x_line, x_forms[x_name]).replace(y_line, y_forms[y_name])
    option = {"name": "%s-%s" % (x_name, y_name)}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "centered-coordinate-update",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub; retail 0x5972d0 proves the Complete-only centered-coordinate operations.",
        "Smack width and height are unsigned long, so both division by two and right shift by one emit the retail SHR. Retail computes the half-delta, adds the caller origin, stores each Smack global, and preserves the same values in a POINT-sized frame area.",
        "Test whether each POINT member owns the origin or half-delta before the add. These are natural one- or two-statement spellings of the observed update and change the C1 expression/value creation boundary implicated by why-reg.",
        "Thirty-six finite states preserve arithmetic, values, store and call order, branches, helper boundaries, and interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
