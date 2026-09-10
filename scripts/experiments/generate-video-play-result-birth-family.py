#!/usr/bin/env python3
"""Retail videoPlay failure-result birth family."""
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
failure = """        if (!g_smackVideo) {
            result = 0;
        } else {"""
assert body.count(failure) == 1
without = body.replace(failure, """        if (!g_smackVideo) {
        } else {""")

anchors = [
    ("selected-entry", "        vh = h;"),
    ("after-dimensions", "        g_soundManager->m_playSounds = 1;"),
    ("before-show", "        showVideo(id, x, y, vw, vh, 0, 0, 1);"),
    ("after-show", "        if (!g_smackVideo) {"),
]
options = [{"name": "failure-arm-current"}]
for name, anchor in anchors:
    assert without.count(anchor) == 1
    changed = without.replace(anchor, "        result = 0;\n" + anchor)
    options.append({"name": name, "replace": changed})

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "failure-result-birth", "find": body, "options": options}],
    "evidence": [
        "Dreamcast 0x14ac38 has no Complete-body locals. Retail 0x5972d0 proves a byte result of zero when ShowVideo does not open a Smacker and !aborted on the successful playback path.",
        "A zero default within the selected-video arm is overwritten on every success path and consumed on failure. VC6 can sink that real default into the failure edge while its source birth changes the C1 value state implicated by why-reg.",
        "Five finite placements preserve one zero definition on every failure path, every value, call, branch, side effect, helper boundary, and interface; no repeated or dead definition.",
    ],
}, indent=2) + "\n")
