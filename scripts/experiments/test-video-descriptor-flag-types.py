#!/usr/bin/env python3
"""Score byte-layout-equivalent source types for video descriptor flags."""
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
header = (repo / "include/smackmgr.h").read_text()
use_line = "    unsigned char m_useBink;      // +8"
fade_line = "    unsigned char m_fadeOnAbort;  // +0x0a"
other_lines = (
    "    unsigned char m_fadeInSecondTrack;      // +9",
    "    unsigned char m_noFrameSkip;      // +0x0b",
)
for anchor in (use_line, fade_line, *other_lines):
    assert header.count(anchor) == 1, anchor

states = {}
types = ("unsigned char", "bool", "char", "signed char")
for use_type, fade_type, other_type in itertools.product(types, types, ("unsigned char", "bool")):
    shadow = header.replace(use_line, use_line.replace("unsigned char", use_type))
    shadow = shadow.replace(fade_line, fade_line.replace("unsigned char", fade_type))
    for line in other_lines:
        shadow = shadow.replace(line, line.replace("unsigned char", other_type))
    states["use-%s_fade-%s_other-%s" % (
        use_type.replace(" ", "-"), fade_type.replace(" ", "-"),
        other_type.replace(" ", "-"))] = shadow

unit = "smackmgr"
symbol = "?videoPlay@@YIHHHHHH@Z"
report = json.loads((repo / "build/objdiff/report.json").read_text())
scored = tuple((unit, fn["name"]) for entry in report["units"]
               if entry["name"] == unit for fn in entry.get("functions", []))
target = (repo / "build/objdiff/target/smackmgr.c.obj").read_bytes()
context = ScoreContext(unit, scoring._first_pass(unit, target), scored)


def trial(item, root):
    name, shadow = item
    directory = root / name
    directory.mkdir(parents=True)
    (directory / "smackmgr.h").write_text(shadow)
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


output = repo / "build/preprocessor-audit/continued/video-descriptor-flag-types"
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
        "Retail proves four consecutive one-byte descriptor fields and byte tests but does not distinguish bool, char, signed char, and unsigned char source types.",
        "videoPlay consumes m_useBink and m_fadeOnAbort; showVideo and the Bink frame pump consume the other two fields.",
        "Every state preserves layout, offsets, ABI, values, and operations and scores every retained smackmgr function.",
    ],
    "results": rows,
}, indent=2) + "\n")
for row in rows:
    print("%s %s%s" % ("FAIL" if row["score"] is None else "%.9f" % row["score"],
                        row["name"], "  " + row["error"] if row["error"] else ""))
