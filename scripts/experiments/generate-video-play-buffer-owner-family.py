#!/usr/bin/env python3
"""Retail videoPlay Smacker and screen-buffer owner lifetimes."""
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

hide = "            g_mouseManager->hidePointer();"
declarations = """    POINT pos;
    int vw, vh;
    unsigned char result;
    unsigned char aborted;"""
buffer_call = """            _SmackToBuffer(g_smackVideo, pos.x, pos.y,
                g_windowManager->m_screenBitmap->m_pitch,
                g_windowManager->m_screenBitmap->m_height,
                g_windowManager->m_screenBitmap->m_map, g_smackBufferFlags);"""
assert body.count(hide) == 1
assert body.count(declarations) == 1
assert body.count(buffer_call) == 1


def smacker_owner(changed, form):
    if form == "none":
        return changed
    if form == "pointer":
        changed = changed.replace(hide, hide + "\n            Smack* smk = g_smackVideo;")
        receiver, argument = "smk->", "smk"
    elif form == "reference":
        changed = changed.replace(hide, hide + "\n            Smack& smk = *g_smackVideo;")
        receiver, argument = "smk.", "&smk"
    else:
        changed = changed.replace(declarations, declarations + "\n    Smack* smk;")
        changed = changed.replace(hide, hide + "\n            smk = g_smackVideo;")
        receiver, argument = "smk->", "smk"
    changed = changed.replace("g_smackVideo->m_width", receiver + "m_width")
    changed = changed.replace("g_smackVideo->m_height", receiver + "m_height")
    return changed.replace("_SmackToBuffer(g_smackVideo,", "_SmackToBuffer(" + argument + ",")


def screen_owner(changed, form):
    call_start = "            _SmackToBuffer("
    call_at = changed.index(call_start)
    if form == "none":
        return changed
    if form in ("pointer", "reference"):
        if form == "pointer":
            declaration = "            Bitmap16Bit* screen = g_windowManager->m_screenBitmap;\n"
            owner = "screen->"
        else:
            declaration = "            Bitmap16Bit& screen = *g_windowManager->m_screenBitmap;\n"
            owner = "screen."
        changed = changed[:call_at] + declaration + changed[call_at:]
    elif form == "function-pointer":
        changed = changed.replace(declarations, declarations + "\n    Bitmap16Bit* screen;")
        call_at = changed.index(call_start)
        changed = changed[:call_at] + "            screen = g_windowManager->m_screenBitmap;\n" + changed[call_at:]
        owner = "screen->"
    else:
        changed = changed.replace(hide, hide + "\n            Bitmap16Bit* screen = g_windowManager->m_screenBitmap;")
        owner = "screen->"
    return changed.replace("g_windowManager->m_screenBitmap->", owner)


options = []
for smacker, screen in itertools.product(
        ("none", "pointer", "reference", "function-pointer"),
        ("none", "pointer", "reference", "function-pointer", "early-pointer")):
    changed = screen_owner(smacker_owner(body, smacker), screen)
    option = {"name": "smacker-%s-screen-%s" % (smacker, screen)}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "draw-owner-lifetimes",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub; retail 0x5972d0 is authoritative for the Complete-only draw call.",
        "Retail keeps the Smack receiver in ESI across four field reads and passes it to SmackToBuffer. It also loads the screen bitmap once into EDX, then reads map, height, and pitch from that receiver.",
        "The current source repeats both global owner paths. The earlier 40-state family tested bitmap accessor calls and geometry scopes, but never the actual bitmap receiver lifetime.",
        "Test real pointer/reference bindings at the call, the owning arm, or function declaration boundary, alone and with the active-track binding. The loop continues to reload g_smackVideo.",
        "Twenty finite states preserve all field values, calls, branches, and the proven loop.",
    ],
}, indent=2) + "\n")
