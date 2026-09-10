#!/usr/bin/env python3
"""Score videoPlay under finite shadow smackmgr.h prototype orders."""
from __future__ import annotations

import concurrent.futures
import json
import tempfile
from pathlib import Path

from homm3 import manifest
from homm3.core import common
from homm3.vc6 import tu_state_sweep as scoring
from homm3.vc6._unit import compile_text
from homm3.vc6.hypotheses import ScoreContext


repo = common.HOMM3_DIR
source_path = repo / "src/smackmgr.cpp"
header_path = repo / "include/smackmgr.h"
source = source_path.read_text()
header = header_path.read_text()
block_start = header.index("// Live prototypes")
block_end = header.index("// The video-archive directory record.", block_start)
prototype_block = header[block_start:block_end]
without = header[:block_start] + header[block_end:]
guard_anchor = "#define HOMM3_SMACKMGR_H\n"
end_anchor = "\n#endif  /* HOMM3_SMACKMGR_H */"
assert without.count(guard_anchor) == 1
assert without.count(end_anchor) == 1

declarations = [
    "void videoSoundOnOff(int on);",
    "void videoRealignBuffers();",
    "int videoPlay(int id, int x, int y, int w, int h);",
    "void videoOpen(int id, int x, int y, int w, int h, int a6, int a7, int a8);",
    "void videoClose();",
    "void videoNextFrame();",
    "void videoDrawCurrentFrame();",
    "void videoPause();",
    "void videoResume();",
    "void videoRestart();",
    "unsigned char videoNeedsUpdate();",
    "unsigned char videoPlaying();",
    "void videoDrawRects();",
    "void videoShutDown();",
    "void deleteSoundHeaders();",
    "void deleteAnimHeaders();",
]


def minimal(rows):
    return "// Shadow order experiment: declarations retain their proven ABI.\n" + "\n".join(rows) + "\n\n"


orders = {
    "control": header,
    "whole-block-first": without.replace(guard_anchor, guard_anchor + prototype_block),
    "whole-block-last": without.replace(end_anchor, "\n" + prototype_block + end_anchor),
    "minimal-current-first": without.replace(guard_anchor, guard_anchor + minimal(declarations)),
    "minimal-reverse-first": without.replace(guard_anchor, guard_anchor + minimal(list(reversed(declarations)))),
    "minimal-play-first": without.replace(guard_anchor, guard_anchor + minimal(
        [declarations[2], *declarations[:2], *declarations[3:]])),
    "minimal-play-last": without.replace(guard_anchor, guard_anchor + minimal(
        [*declarations[:2], *declarations[3:], declarations[2]])),
    "minimal-current-last": without.replace(end_anchor, "\n" + minimal(declarations) + end_anchor),
    "minimal-reverse-last": without.replace(end_anchor, "\n" + minimal(list(reversed(declarations))) + end_anchor),
}

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


output = repo / "build/preprocessor-audit/continued/video-play-header-prototype-order"
output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="compile-", dir=output) as temporary:
    root = Path(temporary)
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        rows = list(pool.map(lambda item: trial(item, root), orders.items()))

rows.sort(key=lambda row: (-(row["score"] if row["score"] is not None else -1), row["name"]))
(output / "results.json").write_text(json.dumps({
    "schema": 1,
    "unit": unit,
    "function": symbol,
    "states": len(orders),
    "evidence": [
        "Dreamcast smackmgr.obj proves the public function roster and source order but its module declaration stream differs from the generated Complete header.",
        "Retail and candidate videoPlay have identical CFG, calls, frame and definition slots; why-reg identifies C1 declaration state as the remaining class.",
        "Each shadow retains every proven ABI declaration and changes only their finite parse order/boundary. All retained TU functions are scored.",
    ],
    "results": rows,
}, indent=2) + "\n")
for row in rows:
    print("%s %s%s" % ("FAIL" if row["score"] is None else "%.9f" % row["score"],
                        row["name"], "  " + row["error"] if row["error"] else ""))
