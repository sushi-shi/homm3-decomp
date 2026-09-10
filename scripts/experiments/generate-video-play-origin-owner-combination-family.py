#!/usr/bin/env python3
"""Retail videoPlay direct coordinate parameters with real draw owners."""
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
assert body.count("    POINT pos;\n") == 1
assert body.count(hide) == 1


def direct_origin(changed):
    changed = changed.replace("    POINT pos;\n", "")
    return changed.replace("pos.x", "x").replace("pos.y", "y")


def smacker_owner(changed, form):
    if form == "global":
        return changed
    if form == "arm-pointer":
        changed = changed.replace(hide, hide + "\n            Smack* smk = g_smackVideo;")
        receiver, argument = "smk->", "smk"
    elif form == "arm-reference":
        changed = changed.replace(hide, hide + "\n            Smack& smk = *g_smackVideo;")
        receiver, argument = "smk.", "&smk"
    else:
        changed = changed.replace("    int vw, vh;", "    Smack* smk;\n    int vw, vh;")
        changed = changed.replace(hide, hide + "\n            smk = g_smackVideo;")
        receiver, argument = "smk->", "smk"
    changed = changed.replace("g_smackVideo->", receiver)
    return changed.replace("_SmackToBuffer(g_smackVideo,", "_SmackToBuffer(" + argument + ",")


def screen_owner(changed, form):
    if form == "global":
        return changed
    call_at = changed.index("            _SmackToBuffer(")
    if form == "call-pointer":
        declaration = "            Bitmap16Bit* screen = g_windowManager->m_screenBitmap;\n"
        receiver = "screen->"
    elif form == "call-reference":
        declaration = "            Bitmap16Bit& screen = *g_windowManager->m_screenBitmap;\n"
        receiver = "screen."
    else:
        changed = changed.replace("    int vw, vh;", "    Bitmap16Bit* screen;\n    int vw, vh;")
        declaration = "            screen = g_windowManager->m_screenBitmap;\n"
        receiver = "screen->"
    changed = changed[:call_at] + declaration + changed[call_at:]
    return changed.replace("g_windowManager->m_screenBitmap->", receiver)


options = [{"name": "point-global-global"}]
for smacker, screen in itertools.product(
        ("global", "arm-pointer", "arm-reference", "function-pointer"),
        ("global", "call-pointer", "call-reference", "function-pointer")):
    changed = screen_owner(smacker_owner(direct_origin(body), smacker), screen)
    options.append({
        "name": "parameters-smacker-%s-screen-%s" % (smacker, screen),
        "replace": changed,
    })

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "origin-and-draw-owner-lifetimes",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves the five int parameters but has no Complete body. Retail and the prior direct-origin candidate assign x/vw/vh to EBX/ESI/EDI through showVideo.",
        "That direct-origin candidate lacks enough pressure to home centered x/y like retail. Retail keeps the active Smack receiver across geometry and reads one screen bitmap receiver for SmackToBuffer.",
        "Combine direct x/y updates with natural Smack and Bitmap16Bit bindings at the owning arm, draw call, or function declaration boundary. These are the real values alive where retail spills x/y.",
        "Seventeen finite states preserve calculations, globals, calls, branches, extent copies, and the proven loop; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
