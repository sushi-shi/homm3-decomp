#!/usr/bin/env python3
"""Retail videoPlay POINT initialization crossed with parameter updates."""
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

declaration = "    POINT pos;"
selected = """                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {
        vh = h;"""
show = "        showVideo(id, x, y, vw, vh, 0, 0, 1);"
coordinates = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
draw = "_SmackToBuffer(g_smackVideo, pos.x, pos.y,"
for anchor in (declaration, selected, show, coordinates, draw):
    assert body.count(anchor) == 1, anchor


def initialize(changed, form):
    if form == "aggregate-function":
        return changed.replace(declaration, "    POINT pos = { x, y };")
    assignments = ("        pos.x = x;\n        pos.y = y;" if form == "selected-xy"
                   else "        pos.y = y;\n        pos.x = x;")
    return changed.replace(selected, selected.replace("        vh = h;",
        assignments + "\n        vh = h;"))


def update(changed, form):
    if form == "parameter-each":
        replacement = """            x += (vw - g_smackVideo->m_width) / 2;
            pos.x = x;
            g_smackX = pos.x;
            y += (vh - g_smackVideo->m_height) / 2;
            pos.y = y;
            g_smackY = pos.y;"""
    elif form == "parameter-pair":
        replacement = """            x += (vw - g_smackVideo->m_width) / 2;
            g_smackX = x;
            y += (vh - g_smackVideo->m_height) / 2;
            g_smackY = y;
            pos.x = x;
            pos.y = y;"""
    elif form == "parameter-chain":
        replacement = """            g_smackX = pos.x = (x += (vw - g_smackVideo->m_width) / 2);
            g_smackY = pos.y = (y += (vh - g_smackVideo->m_height) / 2);"""
    else:
        replacement = """            pos.x += (vw - g_smackVideo->m_width) / 2;
            x = pos.x;
            g_smackX = x;
            pos.y += (vh - g_smackVideo->m_height) / 2;
            y = pos.y;
            g_smackY = y;"""
    return changed.replace(coordinates, replacement)


options = [{"name": "control"}]
for init_form, update_form, draw_form in itertools.product(
        ("aggregate-function", "selected-xy", "selected-yx"),
        ("parameter-each", "parameter-pair", "parameter-chain", "point-first"),
        ("point", "parameters")):
    changed = initialize(body, init_form).replace(
        show, "        showVideo(id, pos.x, pos.y, vw, vh, 0, 0, 1);")
    changed = update(changed, update_form)
    if draw_form == "parameters":
        changed = changed.replace(draw, "_SmackToBuffer(g_smackVideo, x, y,")
    options.append({
        "name": "%s-%s-draw-%s" % (init_form, update_form, draw_form),
        "replace": changed,
    })

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "point-and-parameter-live-range-split",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves x/y but has no Complete local inventory. Retail 0x5972d0 proves the original coordinates remain live through showVideo and that the centered pair is homed in a POINT-sized stack region before SmackToBuffer.",
        "Updating x/y directly gives retail's entry allocation but lets C2 retain centered x in EBX and eliminate the POINT. Initializing and consuming the actual POINT before showVideo creates a real pre-center lifetime, while later parameter updates can split the incoming and centered values.",
        "Test aggregate or ordered member initialization, four assignment dataflows, and whether the immediate SDK draw consumes POINT or parameters. Every initialization feeds showVideo, so no state is a dead-store padding probe.",
        "Twenty-five finite states preserve all values, globals, calls, branches, the deferred update rectangle, and SDK interfaces.",
    ],
}, indent=2) + "\n")
