#!/usr/bin/env python3
"""Diagnostic storage classes on retail videoPlay's existing locals."""
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

decls = (
    ("point", "    POINT pos;"),
    ("dimensions", "    int vw, vh;"),
    ("result", "    unsigned char result;"),
    ("aborted", "    unsigned char aborted;"),
)
for _, declaration in decls:
    assert body.count(declaration) == 1

options = []
for mask in range(16):
    changed = body
    names = []
    for bit, (name, declaration) in enumerate(decls):
        if mask & (1 << bit):
            changed = changed.replace(declaration,
                declaration.replace("    ", "    register ", 1))
            names.append(name)
    option = {"name": "register-" + ("-".join(names) if names else "none")}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "local-storage-class",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Diagnostic only: Dreamcast's platform stub does not expose Complete's locals. Retail and candidate agree on the local widths, frame, branches, and calls, while why-reg isolates a C1 pseudo processing-order divergence.",
        "VC6 still accepts the historical register storage hint and may use it when ranking local pseudos even when final storage is constrained by calls. Test it only on the four existing source declarations; the emitted object decides whether it has any effect.",
        "Sixteen finite states preserve every value, lifetime, operation, interface, and scope. Retain a state only if it reaches retail exact and reproduces under the canonical profile.",
    ],
}, indent=2) + "\n")
