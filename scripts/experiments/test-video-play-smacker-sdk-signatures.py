#!/usr/bin/env python3
"""Score videoPlay under the finite Smacker 3.2h declaration differences."""
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

buffer_current = "unsigned long destheight, void* buf, unsigned long flags);"
buffer_sdk = "unsigned long destheight, const void* buf, unsigned long flags);"
mmx_current = "__declspec(dllimport) void __stdcall _SmackUseMMX(unsigned long on);"
mmx_sdk = "__declspec(dllimport) unsigned long __stdcall _SmackUseMMX(unsigned long on);"
open_current = "__declspec(dllimport) Smack* __stdcall _SmackOpen(void* handle, unsigned long flags, long extra);"
open_sdk_extra = "__declspec(dllimport) Smack* __stdcall _SmackOpen(void* handle, unsigned long flags, unsigned long extra);"

for anchor in (buffer_current, mmx_current, open_current):
    assert header.count(anchor) == 1, anchor

states = {}
for const_buffer, mmx_result, unsigned_extra in itertools.product((False, True), repeat=3):
    shadow = header
    if const_buffer:
        shadow = shadow.replace(buffer_current, buffer_sdk)
    if mmx_result:
        shadow = shadow.replace(mmx_current, mmx_sdk)
    if unsigned_extra:
        shadow = shadow.replace(open_current, open_sdk_extra)
    name = "buffer-%s_mmx-%s_extra-%s" % (
        "const" if const_buffer else "mutable",
        "u32" if mmx_result else "void",
        "u32" if unsigned_extra else "s32",
    )
    states[name] = shadow

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


output = repo / "build/preprocessor-audit/continued/video-play-smacker-sdk-signatures"
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
        "RAD Smacker 3.2h declares SmackToBuffer's buffer as const void* and SmackUseMMX as returning u32.",
        "Complete's archive-handle SmackOpen use proves its first parameter differs from the filename SDK form; only the signedness of the unused extra argument is varied.",
        "All combinations preserve the retail import ABI and score every retained smackmgr function.",
    ],
    "results": rows,
}, indent=2) + "\n")
for row in rows:
    print("%s %s%s" % ("FAIL" if row["score"] is None else "%.9f" % row["score"],
                        row["name"], "  " + row["error"] if row["error"] else ""))
