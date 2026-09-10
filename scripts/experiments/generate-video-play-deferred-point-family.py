#!/usr/bin/env python3
"""Retail videoPlay immediate coordinates versus deferred POINT storage."""
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
call = """            _SmackToBuffer(g_smackVideo, pos.x, pos.y,
                g_windowManager->m_screenBitmap->m_pitch,
                g_windowManager->m_screenBitmap->m_height,
                g_windowManager->m_screenBitmap->m_map, g_smackBufferFlags);"""
assert body.count(coordinates) == 1
assert body.count(call) == 1

updates = {
    "separate": """            x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = x;
            y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = y;""",
    "compound": """            x += (vw - g_smackVideo->m_width) / 2;
            g_smackX = x;
            y += (vh - g_smackVideo->m_height) / 2;
            g_smackY = y;""",
}
copy = """            pos.x = x;
            pos.y = y;"""
param_call = call.replace("pos.x, pos.y", "x, y")

forms = [("control", body)]
for update_name, update in updates.items():
    changed = body.replace(coordinates, update + "\n" + copy)
    changed = changed.replace(call, param_call)
    forms.append((update_name + "-copy-before-call-parameters", changed))

    changed = body.replace(coordinates, update)
    changed = changed.replace(call, param_call + "\n" + copy)
    forms.append((update_name + "-copy-after-call-parameters", changed))

    changed = body.replace(coordinates, update + "\n" + copy)
    forms.append((update_name + "-copy-before-call-point", changed))

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
        "name": "immediate-parameter-and-deferred-point",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub; retail 0x5972d0 supplies the Complete-body coordinate lifetime.",
        "Updating x and y directly gives retail's entry roles x=EBX, vw=ESI, vh=EDI, while the retained body homes a POINT-sized deferred rectangle at ebp-c/ebp-8 before SmackToBuffer and reloads it only after the loop.",
        "Earlier families copied each POINT member alongside its parameter update. Test the remaining natural split: calculate both immediate parameters, copy them as one deferred POINT, and let either pair feed the immediate SDK call.",
        "Seven finite states preserve all values, global stores, calls, branches, and interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
