#!/usr/bin/env python3
"""Retail videoPlay parameter updates combined with active-track ownership."""
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
coordinates = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
call = "_SmackToBuffer(g_smackVideo, pos.x, pos.y,"
assert body.count(hide) == 1
assert body.count(declarations) == 1
assert body.count(coordinates) == 1
assert body.count(call) == 1


def coordinate_form(changed, form):
    if form == "point":
        return changed
    if form == "parameter-copy-each":
        replacement = """            x += (vw - g_smackVideo->m_width) / 2;
            pos.x = x;
            g_smackX = pos.x;
            y += (vh - g_smackVideo->m_height) / 2;
            pos.y = y;
            g_smackY = pos.y;"""
    elif form == "parameter-copy-pair":
        replacement = """            x += (vw - g_smackVideo->m_width) / 2;
            g_smackX = x;
            y += (vh - g_smackVideo->m_height) / 2;
            g_smackY = y;
            pos.x = x;
            pos.y = y;"""
    else:
        replacement = """            x += (vw - g_smackVideo->m_width) / 2;
            g_smackX = x;
            pos.x = x;
            y += (vh - g_smackVideo->m_height) / 2;
            g_smackY = y;
            pos.y = y;"""
    return changed.replace(coordinates, replacement)


def smacker_owner(changed, form):
    if form == "global":
        return changed
    if form == "branch-pointer":
        changed = changed.replace(hide, hide + "\n            Smack* smk = g_smackVideo;")
        receiver, argument = "smk->", "smk"
    elif form == "function-pointer":
        changed = changed.replace(declarations, declarations + "\n    Smack* smk;")
        changed = changed.replace(hide, hide + "\n            smk = g_smackVideo;")
        receiver, argument = "smk->", "smk"
    else:
        changed = changed.replace(hide, hide + "\n            Smack& smk = *g_smackVideo;")
        receiver, argument = "smk.", "&smk"
    changed = changed.replace("g_smackVideo->m_width", receiver + "m_width")
    changed = changed.replace("g_smackVideo->m_height", receiver + "m_height")
    return changed.replace("_SmackToBuffer(g_smackVideo,", "_SmackToBuffer(" + argument + ",")


options = []
for coordinate, owner in itertools.product(
        ("point", "parameter-copy-each", "parameter-copy-pair", "parameter-copy-interleaved"),
        ("global", "branch-pointer", "function-pointer", "branch-reference")):
    changed = smacker_owner(coordinate_form(body, coordinate), owner)
    option = {"name": "%s-%s" % (coordinate, owner)}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "coordinate-parameter-and-track-owner",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub; retail 0x5972d0 supplies the Complete-body register and ownership evidence.",
        "Updating x and y produces retail's entry mapping x=EBX, vw=ESI, vh=EDI, but by itself promotes the centered x and shrinks the frame. An active Smack owner by itself is byte flat, but retail reuses ESI from vw for g_smackVideo through both field pairs and SmackToBuffer.",
        "Combine those two meaningful source facts and vary only whether the actual POINT copy happens per coordinate or after both calculations. The deferred POINT still owns updateScreen's rectangle.",
        "Sixteen finite states preserve all calls, branches, fields, SDK interfaces, and the proven loop; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
