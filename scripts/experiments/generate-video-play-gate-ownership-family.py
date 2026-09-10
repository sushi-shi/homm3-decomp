#!/usr/bin/env python3
"""Retail videoPlay primary/fallback gate ownership family."""
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

head = """    if (id >= VIDEO_ID_FIRST_TABLED
        && (!g_videoDescriptors[id].m_useBink || !g_unnamed698758.m_binkVideo
            || (id == VIDEO_ID_STATE_GATED
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_LOW
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {"""
tail = """        return result;
    }
    return playBinkVideo(id, x, y, w, h);"""
assert body.count(head) == 1
assert body.count(tail) == 1


def guarded(guard):
    changed = body.replace(head, guard + "\n    {")
    return changed.replace(tail, "        return result;\n    }")


negated = """    if (!(id >= VIDEO_ID_FIRST_TABLED
        && (!g_videoDescriptors[id].m_useBink || !g_unnamed698758.m_binkVideo
            || (id == VIDEO_ID_STATE_GATED
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_LOW
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))))
        return playBinkVideo(id, x, y, w, h);"""

failure = """    if (id < VIDEO_ID_FIRST_TABLED
        || (g_videoDescriptors[id].m_useBink && g_unnamed698758.m_binkVideo
            && (id != VIDEO_ID_STATE_GATED
                || *g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_LOW
                || *g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH)))
        return playBinkVideo(id, x, y, w, h);"""

nested = """    if (id < VIDEO_ID_FIRST_TABLED)
        return playBinkVideo(id, x, y, w, h);
    if (g_videoDescriptors[id].m_useBink && g_unnamed698758.m_binkVideo) {
        if (id != VIDEO_ID_STATE_GATED)
            return playBinkVideo(id, x, y, w, h);
        if (*g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_LOW
            || *g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH)
            return playBinkVideo(id, x, y, w, h);
    }"""

options = [
    {"name": "positive-primary-arm"},
    {"name": "negated-early-fallback", "replace": guarded(negated)},
    {"name": "explicit-failure-early-fallback", "replace": guarded(failure)},
    {"name": "nested-failure-early-fallback", "replace": guarded(nested)},
]

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "primary-and-fallback-gate",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves the signature only. Retail 0x5972d0 and the exact videoOpen sibling prove the ordered id/descriptor/bink/state gate.",
        "Retail places the Bink fallback at the function tail and every failed gate branch jumps there; the selected Smacker path remains fallthrough. This is compatible with either a positive owning arm or an early fallback guard.",
        "Test the same predicate as a negated guard, its ordered De Morgan failure form, or nested early failures. All preserve short-circuit reads, calls, values, and the selected path.",
        "Four finite control-source forms; retain only identical retail CFG and call evidence.",
    ],
}, indent=2) + "\n")
