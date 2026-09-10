#!/usr/bin/env python3
"""Retail videoPlay parameter copies and POINT lifetime family."""
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

point_decl = "    POINT pos;\n"
dimension_decl = "    int vw, vh;\n"
dimension_stores = "        vh = h;\n        vw = w;"
outer_head = """                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {
"""
inner_head = """        } else {
            g_mouseManager->hidePointer();"""
assert body.count(point_decl) == 1
assert body.count(dimension_decl) == 1
assert body.count(dimension_stores) == 1
assert body.count(outer_head) == 1
assert body.count(inner_head) == 1


def dimensions(changed, mode):
    declaration_order, store_order, scope, initialization = mode
    names = ("vw", "vh") if declaration_order == "vw-vh" else ("vh", "vw")
    stores = ("        vw = w;\n        vh = h;" if store_order == "vw-vh"
              else "        vh = h;\n        vw = w;")
    changed = changed.replace(dimension_decl, "")
    if initialization:
        init = []
        for name in names:
            init.append("int %s = %s;" % (name, "w" if name == "vw" else "h"))
        indent = "    " if scope == "function" else "        "
        text = indent + ("\n" + indent).join(init)
        changed = changed.replace(dimension_stores, "")
        if scope == "function":
            changed = changed.replace(point_decl, point_decl + text + "\n")
        else:
            changed = changed.replace(outer_head, outer_head + text + "\n")
    else:
        decl = "int %s, %s;" % names
        if scope == "function":
            changed = changed.replace(point_decl, point_decl + "    " + decl + "\n")
        else:
            changed = changed.replace(outer_head, outer_head + "        " + decl + "\n")
        changed = changed.replace(dimension_stores, stores)
    return changed


def point_scope(changed, scope):
    if scope == "function":
        return changed
    changed = changed.replace(point_decl, "")
    if scope == "outer":
        return changed.replace(outer_head, outer_head + "        POINT pos;\n")
    return changed.replace(inner_head, "        } else {\n            POINT pos;\n            g_mouseManager->hidePointer();")


modes = []
for declaration_order, store_order in itertools.product(("vw-vh", "vh-vw"), repeat=2):
    modes.append((declaration_order, store_order, "function", False))
    modes.append((declaration_order, store_order, "outer", False))
for order in ("vw-vh", "vh-vw"):
    modes.append((order, order, "function", True))
    modes.append((order, order, "outer", True))

options = []
for mode, point in itertools.product(modes, ("function", "outer", "inner")):
    changed = point_scope(dimensions(body, mode), point)
    label = "%s-decl-%s-store-%s-%s-point" % (mode[0], mode[1],
            (mode[2] + ("-initialized" if mode[3] else "-assigned")), point)
    option = {"name": label}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "dimension-copies-and-point-lifetime",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves the five int parameters but is a platform stub with no Complete-body local evidence.",
        "Retail saves x in EBX, then loads and saves vw in ESI before vh in EDI. The current candidate saves x in EDI and initializes vh before vw, producing the reversed callee-saved assignment.",
        "Retail homes both POINT members at ebp-0xc and ebp-0x8 before SmackToBuffer; the candidate promotes pos.x. Test the real POINT declaration in the function, selected-video arm, or owning Smacker arm.",
        "Dimension variants retain named signed int copies and test declaration, assignment, and initialization boundaries around their actual owning arm. Calls, conditions, SDK interfaces, and the proven loop stay unchanged.",
        "Thirty-six finite states; no dummy operations, alternate APIs, or compiler directives.",
    ],
}, indent=2) + "\n")
