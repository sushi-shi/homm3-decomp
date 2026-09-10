#!/usr/bin/env python3
"""Retail videoPlay outcome initialization birth points."""
import argparse
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
result_store = "            result = 0;"
aborted_store = "            aborted = 0;"
for anchor in (result_decl, aborted_decl, result_store, aborted_store):
    assert body.count(anchor) == 1, anchor


def initialize(result, aborted, combined=False):
    changed = body
    if combined:
        changed = changed.replace(result_decl + "\n" + aborted_decl,
                                  "    unsigned char result = 0, aborted = 0;")
    else:
        if result:
            changed = changed.replace(result_decl, "    unsigned char result = 0;")
        if aborted:
            changed = changed.replace(aborted_decl, "    unsigned char aborted = 0;")
    if result:
        changed = changed.replace(result_store, "            // result is the initialized failure value")
    if aborted:
        changed = changed.replace(aborted_store, "            // playback begins un-aborted")
    return changed


forms = [
    ("assigned-at-use", body),
    ("result-initialized", initialize(True, False)),
    ("aborted-initialized", initialize(False, True)),
    ("both-separate-initializers", initialize(True, True)),
    ("both-one-declaration", initialize(True, True, True)),
]

options = []
for name, changed in forms:
    option = {"name": name}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "outcome-initialization",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast's 0x14ac38 platform stub exposes no Complete-body initializers. Retail emits the result-zero store only on showVideo failure and the abort-zero store only immediately before flush.",
        "Those placements can arise from assignments at use or from ordinary declaration initializers that VC6 sinks into the only paths where their values survive. Initializer birth points alter C1 pseudo ordering even when the final stores remain sunk.",
        "Test the two actual byte owners independently and as one declaration, removing the corresponding later assignment so no dead or duplicate store is introduced.",
        "Five finite states preserve all values, reads, calls, branches, scopes, and interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
