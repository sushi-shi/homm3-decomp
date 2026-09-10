#!/usr/bin/env python3
"""Retail videoPlay equivalent extent tests and centered expressions."""
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

width_test = "if (vw < 0)"
height_test = "if (vh < 0)"
x_expr = "x + (vw - g_smackVideo->m_width) / 2"
y_expr = "y + (vh - g_smackVideo->m_height) / 2"
for anchor in (width_test, height_test, x_expr, y_expr):
    assert body.count(anchor) == 1

tests = {
    "less-zero": "if ({v} < 0)",
    "zero-greater": "if (0 > {v})",
    "le-minus-one": "if ({v} <= -1)",
    "minus-one-ge": "if (-1 >= {v})",
}
expressions = {
    "origin-first": "{o} + ({v} - {field}) / 2",
    "delta-first": "({v} - {field}) / 2 + {o}",
}

options = []
for wt, ht, xe, ye in itertools.product(tests, tests, expressions, expressions):
    changed = body.replace(width_test, tests[wt].format(v="vw"))
    changed = changed.replace(height_test, tests[ht].format(v="vh"))
    changed = changed.replace(x_expr, expressions[xe].format(
        o="x", v="vw", field="g_smackVideo->m_width"))
    changed = changed.replace(y_expr, expressions[ye].format(
        o="y", v="vh", field="g_smackVideo->m_height"))
    option = {"name": "%s-%s-%s-%s" % (wt, ht, xe, ye)}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "extent-test-and-center-expression",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 exposes no Complete body. Retail proves signed negative-extent branches followed by unsigned half-difference centering for both axes.",
        "Equivalent relational directions and commuted center sums retain those exact operations but present different front-end expression trees, a remaining real source lever for the C1 pseudo order reported by why-reg.",
        "Sixty-four finite combinations preserve values, field reads, calls, branches, POINT ownership, types, and interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
