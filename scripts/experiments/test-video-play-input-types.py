#!/usr/bin/env python3
"""Score 32-bit source types consumed by videoPlay's gate and setup."""
from __future__ import annotations

import concurrent.futures
import itertools
import json
import tempfile
from pathlib import Path

from homm3.core import common
from homm3.vc6 import tu_state_sweep as scoring
from homm3.vc6._unit import compile_text
from homm3.vc6.hypotheses import ScoreContext


repo = common.HOMM3_DIR
source = (repo / "src/smackmgr.cpp").read_text()
headers = {name: (repo / "include" / name).read_text()
           for name in ("smackmgr.h", "soundmgr.h", "prefs.h")}
anchors = {
    "play": ("soundmgr.h", "    int m_playSounds;"),
    "bink": ("prefs.h", "    int m_binkVideo;                // +0x50  \"Bink Video\""),
    "skip": ("smackmgr.h", "extern int g_videoNoSkip;        // .bss 0x699524 - nonzero blocks the user abort"),
    "state": ("smackmgr.h", "extern int* g_videoGameState;    // .bss 0x69923c - the forced-bink state pair"),
}
for filename, anchor in anchors.values():
    assert headers[filename].count(anchor) == 1, anchor

types = ("int", "long", "unsigned long")
states = {}
for play_type, bink_type, skip_type, state_type in itertools.product(types, repeat=4):
    shadows = dict(headers)
    replacements = {
        "play": "    %s m_playSounds;" % play_type,
        "bink": "    %s m_binkVideo;                // +0x50  \"Bink Video\"" % bink_type,
        "skip": "extern %s g_videoNoSkip;        // .bss 0x699524 - nonzero blocks the user abort" % skip_type,
        "state": "extern %s* g_videoGameState;    // .bss 0x69923c - the forced-bink state pair" % state_type,
    }
    for key, replacement in replacements.items():
        filename, anchor = anchors[key]
        shadows[filename] = shadows[filename].replace(anchor, replacement)
    label = "play-%s_bink-%s_skip-%s_state-%s" % (
        play_type.replace(" ", "-"), bink_type.replace(" ", "-"),
        skip_type.replace(" ", "-"), state_type.replace(" ", "-"))
    states[label] = shadows

unit = "smackmgr"
symbol = "?videoPlay@@YIHHHHHH@Z"
report = json.loads((repo / "build/objdiff/report.json").read_text())
scored = tuple((unit, fn["name"]) for entry in report["units"]
               if entry["name"] == unit for fn in entry.get("functions", []))
target = (repo / "build/objdiff/target/smackmgr.c.obj").read_bytes()
context = ScoreContext(unit, scoring._first_pass(unit, target), scored)


def trial(item, root):
    name, shadows = item
    directory = root / name
    directory.mkdir(parents=True)
    for filename, shadow in shadows.items():
        (directory / filename).write_text(shadow)
    obj, error = compile_text(source, unit, directory, "smackmgr", with_listing=False)
    scores = {}
    if obj is not None:
        try:
            base, reference = scoring._normalized_pair(context, obj.read_bytes())
            scores = scoring._report_scores(context, base, reference, directory)
        except Exception as exc:
            error = str(exc)
    return {"name": name, "score": scores.get(symbol) if not error else None,
            "scores": scores, "error": error}


output = repo / "build/preprocessor-audit/continued/video-play-input-types"
output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="compile-", dir=output) as temporary:
    root = Path(temporary)
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        rows = list(pool.map(lambda item: trial(item, root), states.items()))

rows.sort(key=lambda row: (-(row["score"] if row["score"] is not None else -1), row["name"]))
(output / "results.json").write_text(json.dumps({
    "schema": 1,
    "unit": unit,
    "function": symbol,
    "states": len(states),
    "evidence": [
        "Retail proves four-byte storage and the emitted comparisons/stores for these values but does not distinguish the three 32-bit fundamental source types.",
        "The preference and sound-manager field names have external naming evidence; their exact signed source types remain inferred.",
        "Every state preserves layout, addresses, ABI, values, and operations and scores every retained smackmgr function.",
    ],
    "results": rows,
}, indent=2) + "\n")
for row in rows:
    print("%s %s%s" % ("FAIL" if row["score"] is None else "%.9f" % row["score"],
                        row["name"], "  " + row["error"] if row["error"] else ""))
