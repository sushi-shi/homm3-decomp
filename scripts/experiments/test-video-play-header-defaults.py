#!/usr/bin/env python3
"""Score videoPlay under plausible source-level default arguments."""
from __future__ import annotations

import concurrent.futures
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
prototype = "int videoPlay(int id, int x, int y, int w, int h);"
assert header.count(prototype) == 1

forms = {
    "control": prototype,
    "h-minus-one": "int videoPlay(int id, int x, int y, int w, int h = -1);",
    "wh-minus-one": "int videoPlay(int id, int x, int y, int w = -1, int h = -1);",
    "xy-zero-wh-minus-one": "int videoPlay(int id, int x = 0, int y = 0, int w = -1, int h = -1);",
    "xywh-minus-one": "int videoPlay(int id, int x = -1, int y = -1, int w = -1, int h = -1);",
    "y-zero-wh-minus-one": "int videoPlay(int id, int x, int y = 0, int w = -1, int h = -1);",
}
states = {name: header.replace(prototype, replacement)
          for name, replacement in forms.items()}

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


output = repo / "build/preprocessor-audit/continued/video-play-header-defaults"
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
        "Dreamcast proves the five-int function type but does not expose prior declaration defaults.",
        "Every known Complete caller supplies all five arguments, so these defaults preserve the observed ABI and caller behavior.",
        "The finite forms cover the natural full-screen and native-size defaults used by this interface.",
    ],
    "results": rows,
}, indent=2) + "\n")
for row in rows:
    print("%s %s%s" % ("FAIL" if row["score"] is None else "%.9f" % row["score"],
                        row["name"], "  " + row["error"] if row["error"] else ""))
