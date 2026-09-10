#!/usr/bin/env python3
"""Retail videoPlay mutable dimension local types."""
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

declaration = "    int vw, vh;"
assert body.count(declaration) == 1

types = (("int", "int"), ("long", "long"), ("signed-long", "signed long"))
options = []
for (width_name, width_type), (height_name, height_type) in itertools.product(types, repeat=2):
    if width_type == height_type:
        replacement = "    %s vw, vh;" % width_type
    else:
        replacement = "    %s vw;\n    %s vh;" % (width_type, height_type)
    changed = body.replace(declaration, replacement)
    option = {"name": "width-%s-height-%s" % (width_name, height_name)}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "dimension-local-types",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves int w/h parameters but its platform stub has no Complete-body locals. Retail's mutable copies are signed 32-bit values, while the Smacker fields and POINT members use long-width SDK types.",
        "On Win32 VC6, int, long, and signed long are distinct source types with the same signed 32-bit representation. The remaining residual is C1 pseudo ordering, so test both mutable owners independently without changing any operation or conversion result.",
        "Nine finite states preserve values, signed correction guards, calls, branches, and SDK interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
