#!/usr/bin/env python3
"""Retail videoPlay parameter updates combined with real draw owners."""
import argparse
import itertools
import json
import re
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
copies = """        vh = h;
        vw = w;"""
hide = "            g_mouseManager->hidePointer();"
call = """            _SmackToBuffer(g_smackVideo, pos.x, pos.y,
                g_windowManager->m_screenBitmap->m_pitch,
                g_windowManager->m_screenBitmap->m_height,
                g_windowManager->m_screenBitmap->m_map, g_smackBufferFlags);"""
for anchor in (declarations, copies, hide, call):
    assert body.count(anchor) == 1, anchor


def direct_parameters(changed):
    changed = changed.replace(declarations, declarations.replace("    int vw, vh;\n", ""))
    changed = changed.replace(copies + "\n", "")
    changed = re.sub(r"\bvw\b", "w", changed)
    return re.sub(r"\bvh\b", "h", changed)


def smacker_owner(changed, form):
    if form == "global":
        return changed
    if form == "arm-pointer":
        changed = changed.replace(hide, hide + "\n            Smack* smk = g_smackVideo;")
        receiver, argument = "smk->", "smk"
    elif form == "arm-reference":
        changed = changed.replace(hide, hide + "\n            Smack& smk = *g_smackVideo;")
        receiver, argument = "smk.", "&smk"
    elif form == "function-pointer":
        changed = changed.replace("    POINT pos;", "    POINT pos;\n    Smack* smk;")
        changed = changed.replace(hide, hide + "\n            smk = g_smackVideo;")
        receiver, argument = "smk->", "smk"
    else:
        changed = changed.replace("        if (!g_smackVideo) {",
                                  "        Smack* smk = g_smackVideo;\n        if (!smk) {")
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
        changed = changed.replace("    POINT pos;", "    POINT pos;\n    Bitmap16Bit* screen;")
        declaration = "            screen = g_windowManager->m_screenBitmap;\n"
        receiver = "screen->"
    changed = changed[:call_at] + declaration + changed[call_at:]
    return changed.replace("g_windowManager->m_screenBitmap->", receiver)


options = []
for dimensions, smacker, screen in itertools.product(
        ("copies", "parameters"),
        ("global", "arm-pointer", "arm-reference", "function-pointer", "selected-pointer"),
        ("global", "call-pointer", "call-reference", "function-pointer")):
    # The all-global copied-dimension state is the canonical baseline. Other
    # copied-dimension owner combinations were measured by buffer-owner-family.
    if dimensions == "copies" and (smacker != "global" or screen != "global"):
        continue
    changed = body if dimensions == "copies" else direct_parameters(body)
    changed = screen_owner(smacker_owner(changed, smacker), screen)
    option = {"name": "%s-smacker-%s-screen-%s" % (dimensions, smacker, screen)}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "parameter-and-draw-owner-lifetimes",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves the five int parameters but has no Complete body. Retail writes w and h back to their incoming homes and assigns x/w/h to EBX/ESI/EDI; direct parameter updates already reproduce that entry mapping.",
        "The direct-parameter probe has a 0x48 frame because POINT.x remains promoted. Retail has a 0x4c frame, homes both POINT members, retains the active Smack receiver in ESI across geometry, and reads one screen bitmap receiver for the draw call.",
        "Combine direct updates with the real Smack and Bitmap16Bit owners at their natural selected-video and draw-call lifetimes. Those owners can supply the register pressure that homes POINT.x without inventing storage.",
        "Twenty-one finite states preserve values, field reads, calls, branches, the proven POINT object, and the loop; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
