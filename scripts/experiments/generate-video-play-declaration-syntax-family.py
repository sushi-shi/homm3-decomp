#!/usr/bin/env python3
"""Retail videoPlay equivalent local declaration syntax."""
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
assert body.count(declarations) == 1

point_types = ("POINT", "tagPOINT", "struct tagPOINT")
dimension_decls = ("int vw, vh;", "int vh, vw;", "long vw, vh;", "long vh, vw;")
flag_decls = (
    "unsigned char result;\n    unsigned char aborted;",
    "unsigned char result, aborted;",
    "unsigned char aborted, result;",
)

options = []
for point_type, dimensions, flags in itertools.product(
        point_types, dimension_decls, flag_decls):
    replacement = "    %s pos;\n    %s\n    %s" % (point_type, dimensions, flags)
    changed = body.replace(declarations, replacement)
    option = {"name": "%s-%s-%s" % (
        point_type.replace(" ", "-"), dimensions.replace(" ", "-").rstrip(";"),
        "flags-separate" if "\n" in flags else flags.split()[2].rstrip(";"))}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "local-declaration-syntax",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves the five-int signature but its platform stub exposes no Complete locals. Retail proves an eight-byte point, two signed 32-bit extents, and two byte outcomes.",
        "Equivalent POINT/tagPOINT spellings and one-declaration forms for the extents and outcome bytes preserve those types and lifetimes while changing C1XX local-symbol creation order.",
        "why-reg identifies the remaining mismatch as a pseudo processing-order permutation; previous declaration-order and storage families kept the two byte declarations separate.",
        "Thirty-six finite states preserve every operation, value, scope, call, branch, and ABI; no dummy declaration or compiler directive.",
    ],
}, indent=2) + "\n")
