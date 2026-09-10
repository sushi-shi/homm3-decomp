#!/usr/bin/env python3
"""Retail videoPlay grouped dimension initialization."""
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

decl = "    int vw, vh;\n"
stores = "        vh = h;\n        vw = w;"
selected = "                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {\n"
for anchor in (decl, stores, selected):
    assert body.count(anchor) == 1

options = [{"name": "assigned-control"}]
for scope, order, grouping in itertools.product(
        ("function", "selected"), ("width-height", "height-width"),
        ("one-declaration", "two-declarations")):
    pairs = (("vw", "w"), ("vh", "h")) if order == "width-height" else (
             ("vh", "h"), ("vw", "w"))
    if grouping == "one-declaration":
        init = "int %s = %s, %s = %s;" % (
            pairs[0][0], pairs[0][1], pairs[1][0], pairs[1][1])
    else:
        init = "int %s = %s;\n%sint %s = %s;" % (
            pairs[0][0], pairs[0][1], "    " if scope == "function" else "        ",
            pairs[1][0], pairs[1][1])
    changed = body.replace(decl, "").replace(stores, "")
    if scope == "function":
        changed = changed.replace("    POINT pos;\n", "    POINT pos;\n    " + init + "\n")
    else:
        changed = changed.replace(selected, selected + "        " + init + "\n")
    options.append({"name": "%s-%s-%s" % (scope, order, grouping), "replace": changed})

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "dimension-initializer-group", "find": body, "options": options}],
    "evidence": [
        "Dreamcast proves the parameter types but not the Complete locals. Retail materializes width before height in ESI/EDI and preserves both across showVideo.",
        "The prior dimension family tested separate initialized declarations only. One declaration with two initializers is a distinct C1XX creation boundary and a natural original spelling.",
        "Nine finite states retain the two signed dimension owners and every runtime value, call, branch, scope, and interface.",
    ],
}, indent=2) + "\n")
