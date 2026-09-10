#!/usr/bin/env python3
"""Retail videoPlay state from its predecessor's ignored parameter symbol."""
import argparse
import json
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
signature = "void videoSoundOnOff(int on)"
assert source.count(signature) == 1

forms = [
    ("on", signature),
    ("unnamed", "void videoSoundOnOff(int)"),
    ("enabled", "void videoSoundOnOff(int enabled)"),
    ("enable", "void videoSoundOnOff(int enable)"),
    ("state", "void videoSoundOnOff(int state)"),
    ("soundOn", "void videoSoundOnOff(int soundOn)"),
    ("onOff", "void videoSoundOnOff(int onOff)"),
    ("flag", "void videoSoundOnOff(int flag)"),
    ("value", "void videoSoundOnOff(int value)"),
    ("mode", "void videoSoundOnOff(int mode)"),
    ("bOn", "void videoSoundOnOff(int bOn)"),
    ("fOn", "void videoSoundOnOff(int fOn)"),
]
options = []
for name, replacement in forms:
    option = {"name": name}
    if replacement != signature:
        option["replace"] = replacement
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "predecessor-ignored-parameter-symbol",
        "find": signature,
        "options": options,
    }],
    "evidence": [
        "Retail calls videoSoundOnOff with one fastcall int and its exact body never reads that argument. Dreamcast's older four-byte stub has no argument, so the reconstructed local name `on` has no source evidence.",
        "An unnamed parameter is the canonical representation of an intentionally ignored value; common historical source names are finite controls. All forms preserve the exact function type, mangled name, body, calls, and runtime behavior.",
        "videoSoundOnOff precedes videoPlay in the same TU. why-reg classifies videoPlay's only residual as C1 front-end pseudo order, so the predecessor's local symbol can affect the next function even when its own object remains exact.",
        "Twelve states; retain only a state that keeps every exact sibling exact and reproduces videoPlay at retail bytes.",
    ],
}, indent=2) + "\n")
