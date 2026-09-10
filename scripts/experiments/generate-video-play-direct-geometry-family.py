#!/usr/bin/env python3
"""Retail videoPlay direct parameter geometry ownership."""
import argparse
import itertools
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

declarations = """    POINT pos;
    int vw, vh;
    unsigned char result;
    unsigned char aborted;"""
copies = """        vh = h;
        vw = w;"""
assert body.count(declarations) == 1
assert body.count(copies) == 1


def geometry(changed, direct_extents, direct_origin, compound):
    if direct_extents:
        changed = changed.replace(declarations,
                                  declarations.replace("    int vw, vh;\n", ""))
        changed = changed.replace(copies + "\n", "")
        changed = re.sub(r"\bvw\b", "w", changed)
        changed = re.sub(r"\bvh\b", "h", changed)
    if direct_origin:
        changed = changed.replace("    POINT pos;\n", "")
        changed = changed.replace("pos.x", "x").replace("pos.y", "y")
        if compound:
            changed = changed.replace(
                "x = x + (w - g_smackVideo->m_width) / 2;" if direct_extents else
                "x = x + (vw - g_smackVideo->m_width) / 2;",
                "x += (w - g_smackVideo->m_width) / 2;" if direct_extents else
                "x += (vw - g_smackVideo->m_width) / 2;")
            changed = changed.replace(
                "y = y + (h - g_smackVideo->m_height) / 2;" if direct_extents else
                "y = y + (vh - g_smackVideo->m_height) / 2;",
                "y += (h - g_smackVideo->m_height) / 2;" if direct_extents else
                "y += (vh - g_smackVideo->m_height) / 2;")
    return changed


options = []
for extents, origin, compound in itertools.product(
        ("copies", "parameters"), ("point", "parameters"), (False, True)):
    if origin == "point" and compound:
        continue
    changed = geometry(body, extents == "parameters", origin == "parameters", compound)
    option = {"name": "%s-%s-%s" % (
        extents, origin, "compound" if compound else "assignment")}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "geometry-ownership",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves the five int parameters but is a platform stub. Retail 0x5972d0 writes repaired w/h to the incoming argument homes and spills centered x/y at ebp-c/ebp-8.",
        "The prior direct-origin state reproduces retail's EBX/ESI/EDI entry mapping but retains copied vw/vh, placing vh at ebp-4 and the descriptor offset at ebp-8. Retail has the descriptor at ebp-4 and no extent-local home.",
        "Cross the two previously isolated ownership choices: update w/h and x/y directly. This yields one source owner for every retail argument home and removes the synthetic POINT and extent copies.",
        "Six finite states preserve all calculations, values, global stores, calls, branches, and interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
