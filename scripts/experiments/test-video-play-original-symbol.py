#!/usr/bin/env python3
"""Test the Dreamcast-proven VideoPlay spelling against the retail body."""
from __future__ import annotations

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
old_definition = "int videoPlay(int id, int x, int y, int w, int h)"
new_definition = "int VideoPlay(int id, int x, int y, int w, int h)"
old_prototype = old_definition + ";"
new_prototype = new_definition + ";"
assert source.count(old_definition) == 1
assert header.count(old_prototype) == 1

unit = "smackmgr"
old_symbol = "?videoPlay@@YIHHHHHH@Z"
new_symbol = "?VideoPlay@@YIHHHHHH@Z"
report = json.loads((repo / "build/objdiff/report.json").read_text())
scored = tuple(
    (unit, new_symbol if fn["name"] == old_symbol else fn["name"])
    for entry in report["units"] if entry["name"] == unit
    for fn in entry.get("functions", []))
target = (repo / "build/objdiff/target/smackmgr.c.obj").read_bytes()
assert target.count(old_symbol.encode()) >= 1
target = target.replace(old_symbol.encode(), new_symbol.encode())
context = ScoreContext(unit, scoring._first_pass(unit, target), scored)

output = repo / "build/preprocessor-audit/continued/video-play-original-symbol"
output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="compile-", dir=output) as temporary:
    directory = Path(temporary)
    (directory / "smackmgr.h").write_text(
        header.replace(old_prototype, new_prototype))
    candidate_source = source.replace(old_definition, new_definition)
    obj, error = compile_text(candidate_source, unit, directory,
                              "smackmgr", with_listing=False)
    scores = {}
    if obj is not None:
        try:
            base, reference = scoring._normalized_pair(context, obj.read_bytes())
            score_dir = output / "score"
            score_dir.mkdir(exist_ok=True)
            scores = scoring._report_scores(context, base, reference, score_dir)
        except Exception as exc:
            error = str(exc)

row = {
    "name": "dreamcast-original-VideoPlay",
    "score": scores.get(new_symbol) if not error else None,
    "scores": scores,
    "error": error,
}
(output / "results.json").write_text(json.dumps({
    "schema": 1,
    "unit": unit,
    "function": new_symbol,
    "evidence": [
        "Dreamcast CodeView records `int VideoPlay(int id, int x, int y, int w, int h)` at smackmgr.cpp:130.",
        "The project normalized this project-owned spelling to videoPlay; the retail bytes can test whether the original symbol changes VC6's function-local handle state.",
        "The target COFF symbol is case-adjusted by the same one-byte spelling change solely so objdiff can pair the renamed candidate section.",
    ],
    "result": row,
}, indent=2) + "\n")
print("FAIL" if row["score"] is None else "%.9f" % row["score"],
      row["name"], row["error"])
