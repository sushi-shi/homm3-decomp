#!/usr/bin/env python3
"""Retail videoPlay initial abort-value birth family."""
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
store = "            aborted = 0;\n"
assert body.count(store) == 1
without = body.replace(store, "")

anchors = [
    ("before-hide", "            g_mouseManager->hidePointer();"),
    ("before-width-test", "            if (vw < 0)"),
    ("before-height-test", "            if (vh < 0)"),
    ("before-x-center", "            pos.x = x + (vw - g_smackVideo->m_width) / 2;"),
    ("before-y-center", "            pos.y = y + (vh - g_smackVideo->m_height) / 2;"),
    ("before-buffer", "            _SmackToBuffer(g_smackVideo, pos.x, pos.y,"),
    ("before-flush-current", "            g_inputManager->flush();"),
    ("before-loop", "            while (1) {"),
]

options = []
for name, anchor in anchors:
    assert without.count(anchor) == 1, anchor
    changed = without.replace(anchor, store + anchor)
    option = {"name": name}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "initial-abort-birth", "find": body, "options": options}],
    "evidence": [
        "Dreamcast 0x14ac38 has no Complete-body locals. Retail 0x5972d0 proves a byte abort outcome initialized to zero before the event loop and set to one only by the accepted skip events.",
        "The zero value is unobservable throughout the successful setup calls. Its source birth may therefore occur at any real setup boundary even when VC6 schedules the sole materialized byte store immediately after SmackToBuffer, as retail does.",
        "why-reg isolates VideoPlay's residual to C1 value-creation state, and measured VC6 behavior makes the first assignment rather than a bare declaration a relevant birth boundary.",
        "Eight finite placements preserve one zero assignment, all values, calls, branches, store order visible outside the function, helper boundaries, and interfaces; no repeated or dead operation.",
    ],
}, indent=2) + "\n")
