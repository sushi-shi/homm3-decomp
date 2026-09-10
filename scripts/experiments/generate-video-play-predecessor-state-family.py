#!/usr/bin/env python3
"""Byte-exact predecessor source states immediately before retail videoPlay."""
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

combined = """void videoSoundOnOff(int on)
{
    if (g_smackVideo || g_smackVideo2 || g_binkVideo || g_binkVideo2)
        g_soundManager->serviceSounds();
}"""

shared_tail = """void videoSoundOnOff(int on)
{
    if (g_smackVideo || g_smackVideo2)
        goto service;
    if (!(g_binkVideo || g_binkVideo2))
        return;
service:
    g_soundManager->serviceSounds();
}"""

realign = """    g_binkBuffer = static_cast<unsigned char*>(static_cast<void*>(g_windowManager->m_screenBitmap->getMap(g_binkX, g_binkY)));
    g_binkPitch = g_windowManager->m_screenBitmap->m_pitch;
    g_binkHeight = g_windowManager->m_screenBitmap->m_height;"""
assert source.count(realign) == 1

accessors = """    g_binkBuffer = static_cast<unsigned char*>(static_cast<void*>(g_windowManager->m_screenBitmap->getMap(g_binkX, g_binkY)));
    g_binkPitch = g_windowManager->m_screenBitmap->getPitch();
    g_binkHeight = g_windowManager->m_screenBitmap->getHeight();"""

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [
        {
            "name": "sound-predecessor-source-form",
            "find": sound,
            "options": [
                {"name": "separate-else-if"},
                {"name": "combined-condition", "replace": combined},
                {"name": "shared-tail", "replace": shared_tail},
            ],
        },
        {
            "name": "realign-exact-accessor-form",
            "find": realign,
            "options": [
                {"name": "direct-fields"},
                {"name": "canonical-accessors", "replace": accessors},
            ],
        },
    ],
    "evidence": [
        "Dreamcast identifies the two immediately preceding functions but all three video bodies are platform stubs; Complete retail bytes remain authoritative.",
        "The current and canonical-accessor videoRealignBuffers forms are independently byte exact. An older byte-exact videoSoundOnOff reconstruction used one combined condition, while the current source preserves two source arms that share the same retail tail.",
        "why-reg classifies videoPlay's sole residual as front-end pseudo processing order. Test whether meaningful, source-equivalent predecessor forms alter that carried state while requiring both predecessor functions and all other exact siblings to remain exact.",
        "Six finite states; no dummy operations, false helpers, compiler directives, or reordered translation-unit declarations.",
    ],
}, indent=2) + "\n")
