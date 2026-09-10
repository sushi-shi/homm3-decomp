#!/usr/bin/env python3
"""Retail videoPlay loop-constant and top-guard expression family."""
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
assert body.count("while (1)") == 1
assert body.count("if (g_smackVideo == 0)\n                    break;") == 1

loop_conditions = {
    "int-one": "1",
    "long-one": "1L",
    "bool-true": "true",
    "one-equals-one": "1 == 1",
    "not-zero": "!0",
}
guards = {
    "pointer-equals-int-zero": "g_smackVideo == 0",
    "not-pointer": "!g_smackVideo",
    "pointer-equals-NULL": "g_smackVideo == NULL",
    "NULL-equals-pointer": "NULL == g_smackVideo",
    "int-zero-equals-pointer": "0 == g_smackVideo",
}

options = []
for (loop_name, loop), (guard_name, guard) in itertools.product(
        loop_conditions.items(), guards.items()):
    changed = body.replace("while (1)", "while (%s)" % loop)
    changed = changed.replace("if (g_smackVideo == 0)\n                    break;",
                              "if (%s)\n                    break;" % guard)
    option = {"name": "%s_%s" % (loop_name, guard_name)}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "loop-and-top-guard-expression",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Retail and candidate prove the unrotated infinite loop with a single top break, but bytes do not distinguish the C++ constant type or equivalent null-pointer comparison spelling.",
        "These finite expressions retain the same loop, guard, calls, branches, and values while presenting distinct front-end expression nodes that can affect whole-function C2 coloring.",
        "No statement, local, helper, or compiler directive is added.",
    ],
}, indent=2) + "\n")
