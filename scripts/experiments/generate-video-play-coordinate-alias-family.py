#!/usr/bin/env python3
"""Retail videoPlay coordinate parameter-alias lifetime family."""
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
selected = """        vh = h;
        vw = w;
        g_soundManager->m_playSounds = 1;"""
after_show = """        showVideo(id, x, y, vw, vh, 0, 0, 1);
        if (!g_smackVideo) {"""
after_hide = """            g_mouseManager->hidePointer();
            if (vw < 0)"""
coordinates = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
assert body.count(declarations) == 1
assert body.count(selected) == 1
assert body.count(after_show) == 1
assert body.count(after_hide) == 1
assert body.count(coordinates) == 1


def add_value_aliases(changed, scope, axes):
    alias_x, alias_y = axes
    names = []
    if alias_x:
        names.append("drawX = x")
    if alias_y:
        names.append("drawY = y")
    declaration = "int " + ", ".join(names) + ";"
    if scope == "function":
        changed = changed.replace(declarations, declarations + "\n    " + declaration)
    elif scope == "selected":
        changed = changed.replace(selected, "        " + declaration + "\n" + selected)
    elif scope == "after-show":
        changed = changed.replace(after_show,
            "        showVideo(id, x, y, vw, vh, 0, 0, 1);\n"
            "        " + declaration + "\n        if (!g_smackVideo) {")
    else:
        changed = changed.replace(after_hide,
            "            g_mouseManager->hidePointer();\n"
            "            " + declaration + "\n            if (vw < 0)")
    if alias_x:
        changed = changed.replace(
            "pos.x = x + (vw - g_smackVideo->m_width) / 2;",
            "pos.x = drawX + (vw - g_smackVideo->m_width) / 2;")
    if alias_y:
        changed = changed.replace(
            "pos.y = y + (vh - g_smackVideo->m_height) / 2;",
            "pos.y = drawY + (vh - g_smackVideo->m_height) / 2;")
    return changed


def add_reference_aliases(changed, scope, axes):
    alias_x, alias_y = axes
    declarations_here = []
    if alias_x:
        declarations_here.append("int& drawX = x;")
    if alias_y:
        declarations_here.append("int& drawY = y;")
    if scope == "function":
        replacement = declarations + "\n    " + "\n    ".join(declarations_here)
        changed = changed.replace(declarations, replacement)
    elif scope == "selected":
        replacement = "        " + "\n        ".join(declarations_here) + "\n" + selected
        changed = changed.replace(selected, replacement)
    elif scope == "after-show":
        replacement = (
            "        showVideo(id, x, y, vw, vh, 0, 0, 1);\n"
            "        " + "\n        ".join(declarations_here) +
            "\n        if (!g_smackVideo) {")
        changed = changed.replace(after_show, replacement)
    else:
        replacement = (
            "            g_mouseManager->hidePointer();\n"
            "            " + "\n            ".join(declarations_here) +
            "\n            if (vw < 0)")
        changed = changed.replace(after_hide, replacement)
    if alias_x:
        changed = changed.replace(
            "pos.x = x + (vw - g_smackVideo->m_width) / 2;",
            "pos.x = drawX + (vw - g_smackVideo->m_width) / 2;")
    if alias_y:
        changed = changed.replace(
            "pos.y = y + (vh - g_smackVideo->m_height) / 2;",
            "pos.y = drawY + (vh - g_smackVideo->m_height) / 2;")
    return changed


options = [{"name": "parameters-direct"}]
for kind, scope, axes in itertools.product(
        ("value", "reference"),
        ("function", "selected", "after-show", "after-hide"),
        ((1, 0), (0, 1), (1, 1))):
    if kind == "value":
        changed = add_value_aliases(body, scope, axes)
    else:
        changed = add_reference_aliases(body, scope, axes)
    label = ("x" if axes == (1, 0) else "y" if axes == (0, 1) else "xy")
    options.append({
        "name": "%s-%s-%s" % (kind, label, scope),
        "replace": changed,
    })

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "coordinate-parameter-alias-lifetime",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves only the five signed parameters because its videoPlay body is a platform stub; retail 0x5972d0 supplies the Complete-body lifetime evidence.",
        "Retail keeps x in EBX from entry through showVideo and the centered-coordinate calculation, while the current POINT spelling promotes the centered pos.x itself into EDI. A real source alias can distinguish the caller coordinate from the later POINT owner without changing either value.",
        "Test ordinary value and reference aliases at the four lexical boundaries supported by the selected-path uses: function entry, selected arm, after showVideo, and after hidePointer. Vary x alone, y alone, and the pair because VC6 C1 allocates by source birth and use order.",
        "Twenty-five finite states preserve the fallback parameters, POINT lifetime, calls, branches, field accesses, arithmetic, and proven loop. There is no dummy use, compiler directive, or alternate API.",
    ],
}, indent=2) + "\n")
