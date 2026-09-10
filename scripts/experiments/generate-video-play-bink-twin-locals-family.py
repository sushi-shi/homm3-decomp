#!/usr/bin/env python3
"""Retail videoPlay local forms guided by its PlayBinkVideo twin."""
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
copies = """        vh = h;
        vw = w;"""
assert body.count(declarations) == 1
assert body.count(copies) == 1


def origin(changed, form):
    if form == "point":
        declaration, xname, yname = "POINT pos;", "pos.x", "pos.y"
    elif form == "int-xy":
        declaration, xname, yname = "int updateX, updateY;", "updateX", "updateY"
    elif form == "int-yx":
        declaration, xname, yname = "int updateY, updateX;", "updateX", "updateY"
    elif form == "long-xy":
        declaration, xname, yname = "long updateX, updateY;", "updateX", "updateY"
    else:
        declaration, xname, yname = "long updateY, updateX;", "updateX", "updateY"
    changed = changed.replace("ORIGIN_DECL", declaration)
    return changed.replace("pos.x", xname).replace("pos.y", yname)


def local_block(form):
    if form == "assigned-height-first":
        dims = "int vh, vw;"
        setup = copies
    elif form == "assigned-width-first":
        dims = "int vw, vh;"
        setup = "        vw = w;\n        vh = h;"
    elif form == "separate-init-height-first":
        dims = "int vh = h;\n    int vw = w;"
        setup = ""
    elif form == "separate-init-width-first":
        dims = "int vw = w;\n    int vh = h;"
        setup = ""
    elif form == "combined-init-height-first":
        dims = "int vh = h, vw = w;"
        setup = ""
    else:
        dims = "int vw = w, vh = h;"
        setup = ""
    block = "    %s\n    ORIGIN_DECL\n    unsigned char result;\n    unsigned char aborted;" % dims
    return block, setup


options = [{"name": "control"}]
dimension_forms = (
    "assigned-height-first", "assigned-width-first",
    "separate-init-height-first", "separate-init-width-first",
    "combined-init-height-first", "combined-init-width-first",
)
for origin_form, dimension_form in itertools.product(
        ("point", "int-xy", "int-yx", "long-xy", "long-yx"), dimension_forms):
    block, setup = local_block(dimension_form)
    changed = body.replace(declarations, block)
    changed = changed.replace(copies, setup)
    changed = origin(changed, origin_form)
    options.append({
        "name": "%s-%s" % (dimension_form, origin_form),
        "replace": changed,
    })

args.output.write_text(json.dumps({
    "schema": 1,
    "source": "src/smackmgr.cpp",
    "units": ["smackmgr"],
    "axes": [{
        "name": "bink-twin-local-order",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast proves videoPlay's five-int signature but its body is a platform stub; retail remains authoritative for emitted operations.",
        "The sibling playBinkVideo implements the same playback loop and declares initialized vh, initialized vw, updateX/updateY, result, then aborted.",
        "Previous videoPlay families varied origin and dimension forms independently, but did not cross scalar coordinate owners with dimensions declared first in the sibling's order.",
        "Thirty-one finite states preserve every value, operation, branch, call, and helper boundary; no dummy declaration or compiler directive.",
    ],
}, indent=2) + "\n")
