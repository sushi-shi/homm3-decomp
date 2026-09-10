#!/usr/bin/env python3
"""Retail videoPlay caller-coordinate width bindings."""
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
selected = """        vh = h;
        vw = w;
        g_soundManager->m_playSounds = 1;"""
after_show = """        showVideo(id, x, y, vw, vh, 0, 0, 1);
        if (!g_smackVideo) {"""
after_hide = """            g_mouseManager->hidePointer();
            if (vw < 0)"""
x_expr = "pos.x = x + (vw - g_smackVideo->m_width) / 2;"
y_expr = "pos.y = y + (vh - g_smackVideo->m_height) / 2;"
for anchor in (selected, after_show, after_hide, x_expr, y_expr):
    assert body.count(anchor) == 1

options = [{"name": "direct-int-parameters"}]
for typ, scope, axes in itertools.product(
        ("long", "unsigned long"),
        ("selected", "after-show", "after-hide"),
        ((1, 0), (0, 1), (1, 1))):
    declarations = []
    if axes[0]: declarations.append("drawX = x")
    if axes[1]: declarations.append("drawY = y")
    declaration = "%s %s;" % (typ, ", ".join(declarations))
    changed = body
    if scope == "selected":
        changed = changed.replace(selected, "        " + declaration + "\n" + selected)
    elif scope == "after-show":
        changed = changed.replace(after_show,
            "        showVideo(id, x, y, vw, vh, 0, 0, 1);\n"
            "        " + declaration + "\n        if (!g_smackVideo) {")
    else:
        changed = changed.replace(after_hide,
            "            g_mouseManager->hidePointer();\n"
            "            " + declaration + "\n            if (vw < 0)")
    if axes[0]: changed = changed.replace(x_expr, x_expr.replace("x +", "drawX +"))
    if axes[1]: changed = changed.replace(y_expr, y_expr.replace("y +", "drawY +"))
    options.append({
        "name": "%s-%s-%s" % (typ.replace(" ", "-"),
            "xy" if axes == (1, 1) else "x" if axes[0] else "y", scope),
        "replace": changed,
    })

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "coordinate-width-binding", "find": body, "options": options}],
    "evidence": [
        "Dreamcast 0x14ac38 proves int x/y parameters; retail 0x5972d0 proves their centered results are Win32 long-width POINT members and unsigned half-delta arithmetic.",
        "Test real long-width coordinate bindings at the selected, post-open, or draw-setup scope. They preserve all 32-bit values while giving C1 a distinct owner from the int parameter that remains needed by the Bink fallback.",
        "Nineteen finite states preserve operations, calls, branches, POINT ownership, and interfaces; no dummy use or compiler directive.",
    ],
}, indent=2) + "\n")
