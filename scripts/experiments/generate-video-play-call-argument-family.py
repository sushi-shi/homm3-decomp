#!/usr/bin/env python3
"""Retail videoPlay dimension copies at the ShowVideo boundary."""
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

setup = """        vh = h;
        vw = w;
        g_soundManager->m_playSounds = 1;
        showVideo(id, x, y, vw, vh, 0, 0, 1);"""
assert body.count(setup) == 1

forms = [
    ("control", setup),
    ("assign-in-call", """        g_soundManager->m_playSounds = 1;
        showVideo(id, x, y, vw = w, vh = h, 0, 0, 1);"""),
    ("assign-in-call-height-first-comma", """        g_soundManager->m_playSounds = 1;
        showVideo(id, x, y, (vh = h, vw = w), vh, 0, 0, 1);"""),
    ("assign-after-call", """        g_soundManager->m_playSounds = 1;
        showVideo(id, x, y, w, h, 0, 0, 1);
        vh = h;
        vw = w;"""),
    ("width-before-sound-height-in-call", """        vw = w;
        g_soundManager->m_playSounds = 1;
        showVideo(id, x, y, vw, vh = h, 0, 0, 1);"""),
    ("height-before-sound-width-in-call", """        vh = h;
        g_soundManager->m_playSounds = 1;
        showVideo(id, x, y, vw = w, vh, 0, 0, 1);"""),
    ("both-before-width-first", """        vw = w;
        vh = h;
        g_soundManager->m_playSounds = 1;
        showVideo(id, x, y, vw, vh, 0, 0, 1);"""),
]

options = []
for name, replacement in forms:
    option = {"name": name}
    if replacement != setup:
        option["replace"] = body.replace(setup, replacement)
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "show-video-dimension-boundary",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves the five-int signature but is a platform stub; retail 0x5972d0 supplies the Complete call-boundary schedule.",
        "Retail materializes vw in ESI before vh in EDI, passes both to ShowVideo, and preserves their corrected values in the original stack homes for the post-loop update.",
        "The current separate copies produce the opposite C1 processing order despite matching definition slots. Test whether the copies were part of the real ShowVideo argument expressions or immediately followed that call; all variants retain the same values and later owners.",
        "Seven finite states preserve calls, branches, POINT, SDK interfaces, and short-circuit behavior; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
