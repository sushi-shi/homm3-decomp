#!/usr/bin/env python3
"""Exact closeSmacker source forms and their videoPlay inline expansion."""
import argparse
import itertools
import json
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
old = """void closeSmacker()
{
    if (g_smackVideo)
        _SmackClose(g_smackVideo);
    if (g_smackVideo2)
        _SmackClose(g_smackVideo2);
    g_smackVideo2 = 0;
    g_smackVideo = 0;
    g_smackPaused = 0;
    g_smackFrameReady = 0;
    g_smackDirty = 0;
}"""
assert source.count(old) == 1

closes = {
    "direct": """    if (g_smackVideo)
        _SmackClose(g_smackVideo);
    if (g_smackVideo2)
        _SmackClose(g_smackVideo2);""",
    "sequential-local": """    Smack* smk = g_smackVideo;
    if (smk)
        _SmackClose(smk);
    smk = g_smackVideo2;
    if (smk)
        _SmackClose(smk);""",
    "two-locals": """    Smack* video = g_smackVideo;
    Smack* audio = g_smackVideo2;
    if (video)
        _SmackClose(video);
    if (audio)
        _SmackClose(audio);""",
    "assigned-local": """    Smack* smk;
    if (smk = g_smackVideo)
        _SmackClose(smk);
    if (smk = g_smackVideo2)
        _SmackClose(smk);""",
}
nulls = {
    "separate": """    g_smackVideo2 = 0;
    g_smackVideo = 0;""",
    "video-from-audio-chain": "    g_smackVideo = g_smackVideo2 = 0;",
    "audio-from-video-chain": "    g_smackVideo2 = g_smackVideo = 0;",
}
statuses = {
    "separate": """    g_smackPaused = 0;
    g_smackFrameReady = 0;
    g_smackDirty = 0;""",
    "right-to-left-chain": "    g_smackDirty = g_smackFrameReady = g_smackPaused = 0;",
    "left-to-right-chain": "    g_smackPaused = g_smackFrameReady = g_smackDirty = 0;",
}

options = []
for close_form, null_form, status_form in itertools.product(closes, nulls, statuses):
    replacement = "void closeSmacker()\n{\n%s\n%s\n%s\n}" % (
        closes[close_form], nulls[null_form], statuses[status_form])
    option = {"name": "%s-%s-%s" % (close_form, null_form, status_form)}
    if replacement != old:
        option["replace"] = replacement
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "close-smacker-inline-source",
        "find": old,
        "options": options,
    }],
    "evidence": [
        "closeSmacker is an ordinary retained helper whose exact body is also expanded into videoPlay. Dreamcast proves this helper boundary and retail proves the two closes followed by five teardown stores.",
        "Equivalent direct/local handle owners and separate/chained zero assignments can preserve the standalone bytes while changing the saved inline IL consumed by videoPlay's register allocator.",
        "A state is admissible only if closeSmacker stays 100%, videoPlay reaches 100%, and every exact sibling remains exact. Thirty-six finite states preserve calls, conditions, values, store set, helper boundary, and ABI.",
    ],
}, indent=2) + "\n")
