#!/usr/bin/env python3
"""Retail videoPlay sound store versus dimension setup order and owner."""
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

setup = """        vh = h;
        vw = w;
        g_soundManager->m_playSounds = 1;"""
declarations = """    POINT pos;
    int vw, vh;
    unsigned char result;
    unsigned char aborted;"""
selected = "                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {"
assert body.count(setup) == 1
assert body.count(declarations) == 1
assert body.count(selected) == 1

orders = {
    "height-width-sound": ("vh = h;", "vw = w;", "SOUND"),
    "sound-height-width": ("SOUND", "vh = h;", "vw = w;"),
    "height-sound-width": ("vh = h;", "SOUND", "vw = w;"),
    "width-height-sound": ("vw = w;", "vh = h;", "SOUND"),
    "sound-width-height": ("SOUND", "vw = w;", "vh = h;"),
    "width-sound-height": ("vw = w;", "SOUND", "vh = h;"),
}
owners = ("global", "at-use-pointer", "function-pointer", "selected-pointer", "at-use-reference")

options = []
for order, owner in itertools.product(orders, owners):
    changed = body
    if owner == "global":
        sound = "g_soundManager->m_playSounds = 1;"
    elif owner == "at-use-pointer":
        sound = "soundManager* sound = g_soundManager;\n        sound->m_playSounds = 1;"
    elif owner == "at-use-reference":
        sound = "soundManager& sound = *g_soundManager;\n        sound.m_playSounds = 1;"
    elif owner == "function-pointer":
        changed = changed.replace(declarations, declarations + "\n    soundManager* sound;")
        sound = "sound = g_soundManager;\n        sound->m_playSounds = 1;"
    else:
        changed = changed.replace(selected, selected + "\n        soundManager* sound;")
        sound = "sound = g_soundManager;\n        sound->m_playSounds = 1;"
    replacement = "\n".join("        " + line.replace("SOUND", sound)
                            for line in orders[order])
    changed = changed.replace(setup, replacement)
    option = {"name": "%s-%s" % (order, owner)}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "sound-and-dimension-setup", "find": body, "options": options}],
    "evidence": [
        "Retail emits the sound-manager store before materializing showVideo's stack arguments; the vw/vh source copies themselves have no observable store at that point, so their source order relative to the sound statement remains open.",
        "The sound manager is the only object receiver at the exact boundary where C2 assigns x/vw/vh to callee-saved registers. Test all six real statement orders and direct, pointer, or reference ownership.",
        "Thirty finite states preserve values, the single sound side effect, showVideo arguments, calls, branches, POINT, and interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
