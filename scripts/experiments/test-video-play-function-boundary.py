#!/usr/bin/env python3
"""Score equivalent videoPlay definition and prior-declaration boundaries."""
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
canonical_source = (repo / "src/smackmgr.cpp").read_text()
canonical_header = (repo / "include/smackmgr.h").read_text()
definition = "int videoPlay(int id, int x, int y, int w, int h)"
prototype = "int videoPlay(int id, int x, int y, int w, int h);"
assert canonical_source.count(definition) == 1
assert canonical_header.count(prototype) == 1

declarations = {
    "named": prototype,
    "unnamed": "int videoPlay(int, int, int, int, int);",
    "descriptive": "int videoPlay(int videoId, int left, int top, int width, int height);",
    "extern-named": "extern int videoPlay(int id, int x, int y, int w, int h);",
}
states = {}
for explicit_definition, explicit_prototype, declaration_name in itertools.product(
        (False, True), (False, True), declarations):
    source = canonical_source
    header = canonical_header
    if explicit_definition:
        source = source.replace(definition,
            "int __fastcall videoPlay(int id, int x, int y, int w, int h)")
    declaration = declarations[declaration_name]
    if explicit_prototype:
        declaration = declaration.replace("int videoPlay(", "int __fastcall videoPlay(", 1)
    header = header.replace(prototype, declaration)
    states["definition-%s_prototype-%s_%s" % (
        "fastcall" if explicit_definition else "default",
        "fastcall" if explicit_prototype else "default", declaration_name)] = (source, header)

unit = "smackmgr"
symbol = "?videoPlay@@YIHHHHHH@Z"
report = json.loads((repo / "build/objdiff/report.json").read_text())
scored = tuple((unit, fn["name"]) for entry in report["units"]
               if entry["name"] == unit for fn in entry.get("functions", []))
target = (repo / "build/objdiff/target/smackmgr.c.obj").read_bytes()
context = ScoreContext(unit, scoring._first_pass(unit, target), scored)


def trial(item, root):
    name, (source, header) = item
    directory = root / name
    directory.mkdir(parents=True)
    (directory / "smackmgr.h").write_text(header)
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


output = repo / "build/preprocessor-audit/continued/video-play-function-boundary"
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
        "Retail proves the five-int fastcall ABI; Dreamcast proves the parameter names on the definition but not the prior header declaration.",
        "The canonical profile supplies fastcall through /Gr, while an explicit __fastcall spelling is type- and ABI-equivalent.",
        "All states preserve the definition parameter names, operations, calls, and runtime behavior, and score every retained smackmgr function.",
    ],
    "results": rows,
}, indent=2) + "\n")
for row in rows:
    print("%s %s%s" % ("FAIL" if row["score"] is None else "%.9f" % row["score"],
                        row["name"], "  " + row["error"] if row["error"] else ""))
