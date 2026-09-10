#!/usr/bin/env python3
"""Diagnostic retail videoPlay parameter storage qualifiers."""
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
signature = "int videoPlay(int id, int x, int y, int w, int h)"
assert body.count(signature) == 1

forms = [("control", signature)]
for storage, mask in itertools.product(("const", "register"), range(1, 16)):
    parameters = ["int id"]
    for bit, name in zip((1, 2, 4, 8), ("x", "y", "w", "h")):
        parameters.append((storage + " int " if mask & bit else "int ") + name)
    forms.append((storage + "-" + "".join(
        name for bit, name in zip((1, 2, 4, 8), "xywh") if mask & bit),
        "int videoPlay(" + ", ".join(parameters) + ")"))

options = []
for name, replacement in forms:
    option = {"name": name}
    if replacement != signature:
        option["replace"] = body.replace(signature, replacement)
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "parameter-storage-qualifiers",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Diagnostic only: why-reg v2 identifies a C1 pseudo-processing-order divergence after source owner, POINT, coordinate, and compatible-header states fail to move the callee-saved permutation.",
        "Top-level const and register storage do not alter the external five-int function type or any expression. Test whether retail's C1 order can be attributed to an original parameter storage qualifier.",
        "Dreamcast's platform stub proves the int parameter types but supplies no body or local storage evidence. Do not retain a winning qualifier without retail codegen reproduction and a documented source interpretation.",
        "Thirty-one diagnostic states; no operation, branch, API, or helper boundary changes.",
    ],
}, indent=2) + "\n")
