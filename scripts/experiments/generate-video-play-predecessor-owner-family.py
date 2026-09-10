#!/usr/bin/env python3
"""Retail videoPlay state after exact VideoSoundOnOff owner forms."""
import argparse
import json
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
sound = """void videoSoundOnOff(int on)
{
    if (g_smackVideo || g_smackVideo2)
        g_soundManager->serviceSounds();
    else if (g_binkVideo || g_binkVideo2)
        g_soundManager->serviceSounds();
}"""
assert source.count(sound) == 1


forms = [
    ("control", sound),
    ("pointer-owner", """void videoSoundOnOff(int on)
{
    soundManager* manager = g_soundManager;
    if (g_smackVideo || g_smackVideo2)
        manager->serviceSounds();
    else if (g_binkVideo || g_binkVideo2)
        manager->serviceSounds();
}"""),
    ("const-pointer-owner", """void videoSoundOnOff(int on)
{
    soundManager* const manager = g_soundManager;
    if (g_smackVideo || g_smackVideo2)
        manager->serviceSounds();
    else if (g_binkVideo || g_binkVideo2)
        manager->serviceSounds();
}"""),
    ("reference-owner", """void videoSoundOnOff(int on)
{
    soundManager& manager = *g_soundManager;
    if (g_smackVideo || g_smackVideo2)
        manager.serviceSounds();
    else if (g_binkVideo || g_binkVideo2)
        manager.serviceSounds();
}"""),
    ("assigned-pointer-owner", """void videoSoundOnOff(int on)
{
    soundManager* manager;
    manager = g_soundManager;
    if (g_smackVideo || g_smackVideo2)
        manager->serviceSounds();
    else if (g_binkVideo || g_binkVideo2)
        manager->serviceSounds();
}"""),
    ("arm-local-pointers", """void videoSoundOnOff(int on)
{
    if (g_smackVideo || g_smackVideo2) {
        soundManager* manager = g_soundManager;
        manager->serviceSounds();
    } else if (g_binkVideo || g_binkVideo2) {
        soundManager* manager = g_soundManager;
        manager->serviceSounds();
    }
}"""),
    ("arrow-through-index", """void videoSoundOnOff(int on)
{
    if (g_smackVideo || g_smackVideo2)
        g_soundManager[0].serviceSounds();
    else if (g_binkVideo || g_binkVideo2)
        g_soundManager[0].serviceSounds();
}"""),
]

options = []
for name, changed in forms:
    option = {"name": name}
    if changed != sound:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "exact-sound-manager-owner-predecessor",
        "find": sound,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac30 identifies VideoSoundOnOff immediately before VideoRealignBuffers and VideoPlay, but the platform body is a stub. Complete retail bytes prove the two-arm sound service body.",
        "Both arms call the same global sound-manager receiver. Test natural pointer/reference ownership forms that may optimize to the exact retained predecessor while changing its real local-handle count before VideoPlay is parsed.",
        "why-reg classifies VideoPlay's remaining callee-saved permutation as C1 front-end state. A state is admissible only if VideoSoundOnOff, VideoRealignBuffers, and every exact sibling remain byte exact as VideoPlay closes.",
        "Seven finite states; no unused declaration, dummy operation, compiler directive, or false helper boundary.",
    ],
}, indent=2) + "\n")
