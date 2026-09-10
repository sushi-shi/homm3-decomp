#!/usr/bin/env python3
"""Retail videoPlay POINT initialization and centered update lifetime."""
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

declaration = "    POINT pos;"
selected = """                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {
        vh = h;"""
inner = """        } else {
            g_mouseManager->hidePointer();"""
show = "        showVideo(id, x, y, vw, vh, 0, 0, 1);"
x_assignment = "            pos.x = x + (vw - g_smackVideo->m_width) / 2;"
y_assignment = "            pos.y = y + (vh - g_smackVideo->m_height) / 2;"
for anchor in (declaration, selected, inner, show, x_assignment, y_assignment):
    assert body.count(anchor) == 1, anchor


initializers = [
    ("function-xy", "function", "        pos.x = x;\n        pos.y = y;"),
    ("function-yx", "function", "        pos.y = y;\n        pos.x = x;"),
    ("aggregate", "aggregate", ""),
    ("selected-xy", "selected", "        pos.x = x;\n        pos.y = y;"),
    ("selected-yx", "selected", "        pos.y = y;\n        pos.x = x;"),
    ("inner-xy", "inner", "            pos.x = x;\n            pos.y = y;"),
    ("inner-yx", "inner", "            pos.y = y;\n            pos.x = x;"),
]

options = [{"name": "control"}]
for init_name, placement, initialization in initializers:
    for expression in ("compound", "expanded"):
        show_modes = ("parameters", "point") if placement != "inner" else ("parameters",)
        for show_mode in show_modes:
            changed = body
            if placement == "aggregate":
                changed = changed.replace(declaration, "    POINT pos = { x, y };")
            elif placement == "function":
                changed = changed.replace(declaration,
                    declaration + "\n" + initialization.replace("        ", "    "))
            elif placement == "selected":
                changed = changed.replace(selected, selected.replace("        vh = h;", initialization + "\n        vh = h;"))
            else:
                changed = changed.replace(inner, inner.replace(
                    "            g_mouseManager->hidePointer();",
                    initialization + "\n            g_mouseManager->hidePointer();"))
            operator = "+=" if expression == "compound" else "= pos.x +"
            changed = changed.replace(x_assignment,
                "            pos.x %s (vw - g_smackVideo->m_width) / 2;" % operator)
            operator = "+=" if expression == "compound" else "= pos.y +"
            changed = changed.replace(y_assignment,
                "            pos.y %s (vh - g_smackVideo->m_height) / 2;" % operator)
            if show_mode == "point":
                changed = changed.replace(show, "        showVideo(id, pos.x, pos.y, vw, vh, 0, 0, 1);")
            options.append({
                "name": "%s-%s-show-%s" % (init_name, expression, show_mode),
                "replace": changed,
            })

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "point-initialization-and-update",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub; retail 0x5972d0 supplies the Complete-only POINT lifetime.",
        "Retail preserves x in EBX from entry and y in its incoming stack home, then adds the centered offsets and spills the updated coordinates at ebp-0xc/ebp-0x8.",
        "That live-range split is consistent with a POINT initialized from x/y before showVideo and updated after the track opens. The current source creates each POINT member only after showVideo and assigns x to EDI instead.",
        "Test aggregate or member initialization at real control boundaries, compound versus expanded updates, and whether showVideo consumes the POINT. All forms preserve values, calls, branches, and SDK interfaces.",
        "Twenty-five finite source states; no dummy use, alternate API, or compiler directive.",
    ],
}, indent=2) + "\n")
