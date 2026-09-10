#!/usr/bin/env python3
"""Retail videoPlay result and abort flag source types."""
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

result_decl = "    unsigned char result;"
aborted_decl = "    unsigned char aborted;"
assert body.count(result_decl) == 1
assert body.count(aborted_decl) == 1

types = (
    ("uchar", "unsigned char"),
    ("bool", "bool"),
    ("char", "char"),
    ("schar", "signed char"),
    ("int", "int"),
)

options = []
for (result_name, result_type), (abort_name, abort_type) in itertools.product(types, repeat=2):
    changed = body.replace(result_decl, "    %s result;" % result_type)
    changed = changed.replace(aborted_decl, "    %s aborted;" % abort_type)
    option = {"name": "result-%s-aborted-%s" % (result_name, abort_name)}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "outcome-local-types",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves only the int return type and supplies no Complete-body locals. Retail stores both the selected-arm result and abort state through byte-width frame aliases.",
        "Unsigned char, bool, plain char, and signed char all have one-byte storage in VC6 but can create different conversion pseudos; int is the negative width control. The remaining mismatch is isolated to C1 pseudo processing order.",
        "Test the two actual outcome owners independently while preserving every assignment, condition, return, call, and branch.",
        "Twenty-five finite states; no dummy operation, alternate API, or compiler directive.",
    ],
}, indent=2) + "\n")
