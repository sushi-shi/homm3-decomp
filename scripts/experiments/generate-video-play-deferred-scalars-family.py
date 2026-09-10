#!/usr/bin/env python3
"""Retail videoPlay parameter updates with deferred scalar rectangle owners."""
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
coordinates = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
call = "_SmackToBuffer(g_smackVideo, pos.x, pos.y,"
full_call = """            _SmackToBuffer(g_smackVideo, pos.x, pos.y,
                g_windowManager->m_screenBitmap->m_pitch,
                g_windowManager->m_screenBitmap->m_height,
                g_windowManager->m_screenBitmap->m_map, g_smackBufferFlags);"""
update = "g_windowManager->updateScreen(pos.x, pos.y, vw, vh);"
assert body.count(declaration) == 1
assert body.count(coordinates) == 1
assert body.count(call) == 1
assert body.count(full_call) == 1
assert body.count(update) == 1

parameter_updates = """            x += (vw - g_smackVideo->m_width) / 2;
            g_smackX = x;
            y += (vh - g_smackVideo->m_height) / 2;
            g_smackY = y;"""

forms = [("control", body)]
for typ, order, copy_placement, immediate in itertools.product(
        ("int", "long"), ("xy", "yx"), ("before", "after"),
        ("parameters", "deferred")):
    names = (("updateX", "updateY") if order == "xy"
             else ("updateY", "updateX"))
    changed = body.replace(declaration,
        "    %s %s, %s;" % (typ, names[0], names[1]))
    copy = "            updateX = x;\n            updateY = y;"
    if copy_placement == "before":
        changed = changed.replace(coordinates, parameter_updates + "\n" + copy)
    else:
        changed = changed.replace(coordinates, parameter_updates)
        changed = changed.replace(full_call, full_call + "\n" + copy)
    changed = changed.replace(call, "_SmackToBuffer(g_smackVideo, " +
        ("x, y," if immediate == "parameters" else "updateX, updateY,"))
    changed = changed.replace(update,
        "g_windowManager->updateScreen(updateX, updateY, vw, vh);")
    forms.append(("%s-%s-copy-%s-call-%s" %
                  (typ, order, copy_placement, immediate), changed))

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
        "name": "parameter-update-and-deferred-scalars",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub; retail 0x5972d0 supplies the Complete-body coordinate lifetime.",
        "The Bink twin owns its later rectangle through updateX/updateY scalars. Retail videoPlay likewise stores two four-byte coordinates at ebp-c/ebp-8 before SmackToBuffer and reloads them only for the deferred updateScreen.",
        "Direct x/y updates recover retail's entry callee-save roles but earlier POINT copies coalesce differently. Test the natural twin spelling with distinct scalar deferred owners and either pair at the immediate SDK call.",
        "Seventeen finite states preserve values, global stores, calls, branches, and interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
