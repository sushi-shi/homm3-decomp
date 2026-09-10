#!/usr/bin/env python3
"""Retail videoPlay interacting coordinate, extent, and byte local forms."""
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
flags = "    unsigned char result;\n    unsigned char aborted;"
selected = "                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {\n"
inner = "        } else {\n            g_mouseManager->hidePointer();"
for anchor in (point_decl, dimension_decl, dimension_stores, flags, selected, inner):
    assert body.count(anchor) == 1, anchor


def origin_form(changed, mode):
    if mode == "point-function":
        return changed
    if mode == "point-selected":
        return changed.replace(point_decl, "").replace(selected, selected + "        POINT pos;\n")
    if mode == "point-inner":
        return changed.replace(point_decl, "").replace(
            inner, "        } else {\n            POINT pos;\n            g_mouseManager->hidePointer();")
    if mode == "int-scalars-xy":
        declaration, xname, yname = "    int updateX, updateY;\n", "updateX", "updateY"
    elif mode == "int-scalars-yx":
        declaration, xname, yname = "    int updateY, updateX;\n", "updateX", "updateY"
    elif mode == "long-scalars-xy":
        declaration, xname, yname = "    long updateX, updateY;\n", "updateX", "updateY"
    else:
        declaration, xname, yname = "    int update[2];\n", "update[0]", "update[1]"
    changed = changed.replace(point_decl, declaration)
    return changed.replace("pos.x", xname).replace("pos.y", yname)


def dimension_form(changed, mode):
    changed = changed.replace(dimension_decl, "")
    changed = changed.replace(dimension_stores, "")
    decl_order = "int vw, vh;"
    stores = dimension_stores
    scope = "function"
    initialization = None
    if mode == "function-width-first":
        stores = "        vw = w;\n        vh = h;"
    elif mode == "function-vh-vw-height-first":
        decl_order = "int vh, vw;"
    elif mode == "function-vh-vw-width-first":
        decl_order = "int vh, vw;"
        stores = "        vw = w;\n        vh = h;"
    elif mode == "selected-height-first":
        scope = "selected"
    elif mode == "selected-width-first":
        scope = "selected"
        stores = "        vw = w;\n        vh = h;"
    elif mode == "selected-init-height-first":
        scope, initialization = "selected", "int vh = h, vw = w;"
    elif mode == "selected-init-width-first":
        scope, initialization = "selected", "int vw = w, vh = h;"
    if scope == "function":
        changed = changed.replace(flags, "    " + decl_order + "\n" + flags)
        changed = changed.replace(selected, selected + stores + "\n")
    else:
        text = initialization if initialization is not None else decl_order + "\n" + stores.strip()
        changed = changed.replace(selected, selected + "        " + text.replace("\n", "\n        ") + "\n")
    return changed


def flag_form(changed, mode):
    if mode == "separate":
        return changed
    replacement = ("    unsigned char result, aborted;" if mode == "result-aborted"
                   else "    unsigned char aborted, result;")
    return changed.replace(flags, replacement)


origin_modes = (
    "point-function", "point-selected", "point-inner", "int-scalars-xy",
    "int-scalars-yx", "long-scalars-xy", "int-array",
)
dimension_modes = (
    "function-height-first", "function-width-first",
    "function-vh-vw-height-first", "function-vh-vw-width-first",
    "selected-height-first", "selected-width-first",
    "selected-init-height-first", "selected-init-width-first",
)
flag_modes = ("separate", "result-aborted", "aborted-result")

options = []
for origin, dimensions, flag in itertools.product(origin_modes, dimension_modes, flag_modes):
    changed = flag_form(dimension_form(origin_form(body, origin), dimensions), flag)
    option = {"name": "%s-%s-%s" % (origin, dimensions, flag)}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "local-interactions", "find": body, "options": options}],
    "evidence": [
        "Dreamcast proves only the five-int signature. Retail proves two 32-bit centered-coordinate homes, two signed extents and two byte outcomes, but not whether the coordinate pair was POINT or scalars.",
        "Single-axis families found several byte-flat declaration, scope and syntax choices without moving the C1 allocation. VC6 front-end state is non-additive, so cross the real owners rather than inventing filler declarations.",
        "One hundred sixty-eight finite states preserve all values, reads, writes, calls, branches, helper boundaries, and interfaces; no dummy operation or compiler directive. Every exact sibling is scored.",
    ],
}, indent=2) + "\n")
