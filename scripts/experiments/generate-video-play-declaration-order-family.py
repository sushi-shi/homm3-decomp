#!/usr/bin/env python3
"""Exhaustive declaration ordering for retail videoPlay's proven locals."""
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

items = (
    ("point", "    POINT pos;"),
    ("dimensions", "    int vw, vh;"),
    ("result", "    unsigned char result;"),
    ("aborted", "    unsigned char aborted;"),
)

options = []
for order in itertools.permutations(items):
    replacement = "\n".join(item[1] for item in order)
    option = {"name": "-".join(item[0] for item in order)}
    if replacement != declarations:
        option["replace"] = body.replace(declarations, replacement)
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "proven-local-declaration-order",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub and supplies no local ordering; retail proves a descriptor address, one POINT-sized deferred rectangle, two mutable dimensions, and two byte-valued outcomes.",
        "why-reg isolates one C1 pseudo-processing-order difference that permutes x, vw, and vh across EBX/ESI/EDI without changing control flow, calls, or frame size. Declaration order is the remaining direct front-end input to test exhaustively.",
        "Earlier notes cover ten declaration spellings but do not inventory all 24 orderings of the four existing declarations. This family closes that finite gap without adding, removing, or changing a local.",
        "Twenty-four finite states; no dummy operations, API changes, or compiler directives.",
    ],
}, indent=2) + "\n")
