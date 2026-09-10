#!/usr/bin/env python3
"""Retail videoPlay centered-coordinate scalar type family."""
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
assert body.count("    POINT pos;") == 1
assert body.count("pos.x") == 4
assert body.count("pos.y") == 4

types = ("int", "long", "unsigned int", "unsigned long")
options = [{"name": "POINT-control"}]
for x_type, y_type, order in itertools.product(types, types, ("xy", "yx")):
    declarations = {
        "x": "    %s drawX;" % x_type,
        "y": "    %s drawY;" % y_type,
    }
    declaration = "\n".join(declarations[axis] for axis in order)
    changed = body.replace("    POINT pos;", declaration)
    changed = changed.replace("pos.x", "drawX").replace("pos.y", "drawY")
    options.append({
        "name": "%s-x_%s-y_%s-order" % (
            x_type.replace(" ", "-"), y_type.replace(" ", "-"), order),
        "replace": changed,
    })

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "centered-coordinate-types",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast VideoPlay is a platform stub and supplies no Complete-body local types. Retail proves two independent 32-bit centered coordinates but never applies a signed comparison to either value.",
        "SmackToBuffer consumes unsigned 32-bit left/top while updateScreen consumes int x/y, so signed and unsigned 32-bit local owners preserve every observed bit pattern and conversion.",
        "Test the finite fundamental-type pairs and declaration orders. Calls, arithmetic, global stores, branches, helper boundaries, and the proven loop remain fixed.",
    ],
}, indent=2) + "\n")
