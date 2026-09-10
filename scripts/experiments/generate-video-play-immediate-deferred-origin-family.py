#!/usr/bin/env python3
"""Retail videoPlay immediate draw versus deferred update coordinate owners."""
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

coordinates = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
call_xy = "_SmackToBuffer(g_smackVideo, pos.x, pos.y,"
update_xy = "g_windowManager->updateScreen(pos.x, pos.y, vw, vh);"
assert body.count(coordinates) == 1
assert body.count(call_xy) == 1
assert body.count(update_xy) == 1

parameter_first = """            x += (vw - g_smackVideo->m_width) / 2;
            g_smackX = x;
            pos.x = x;
            y += (vh - g_smackVideo->m_height) / 2;
            g_smackY = y;
            pos.y = y;"""
point_first = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            x = pos.x;
            g_smackX = x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            y = pos.y;
            g_smackY = y;"""

forms = [("control", body)]
for dataflow, replacement in (("parameter-first", parameter_first),
                              ("point-first", point_first)):
    for immediate, deferred in (("parameter", "point"), ("point", "parameter"),
                                ("parameter", "parameter"), ("point", "point")):
        changed = body.replace(coordinates, replacement)
        if immediate == "parameter":
            changed = changed.replace(call_xy, "_SmackToBuffer(g_smackVideo, x, y,")
        if deferred == "parameter":
            changed = changed.replace(update_xy,
                "g_windowManager->updateScreen(x, y, vw, vh);")
        forms.append((dataflow + "-draw-" + immediate + "-update-" + deferred, changed))

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
        "name": "immediate-and-deferred-coordinate-owners",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 has no Complete body. Retail 0x5972d0 supplies both the immediate SmackToBuffer values and the deferred updateScreen homes.",
        "Direct x/y updates give retail's exact x=EBX, vw=ESI, vh=EDI entry assignment. Retail nevertheless spills the centered coordinates to the two-slot POINT-sized area before SmackToBuffer and reloads both only for updateScreen after the loop.",
        "Distinguish the immediate draw owner from the deferred update owner: update parameters and POINT together, then consume either pair at each real use. This can retain the POINT frame without collapsing the parameter update that fixes allocation.",
        "Nine finite states preserve every value, global store, call, branch, and SDK interface.",
    ],
}, indent=2) + "\n")
