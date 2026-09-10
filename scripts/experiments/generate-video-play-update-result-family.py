#!/usr/bin/env python3
"""Generate meaningful owners for videoPlay's inlined update predicate."""
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
site = """                if (videoNeedsUpdate())
                    videoDrawRects();"""
assert body.count(site) == 1

options = [{"name": "direct-helper-result"}]

def add(name, changed):
    assert changed != body
    options.append({"name": name, "replace": changed})

for spelling, typename in (("byte", "unsigned char"), ("int", "int"),
                           ("bool", "bool")):
    add("loop-local-" + spelling, body.replace(site,
        "                %s needsUpdate = videoNeedsUpdate();\n"
        "                if (needsUpdate)\n"
        "                    videoDrawRects();" % typename))

add("reuse-return-byte", body.replace(site,
    "                result = videoNeedsUpdate();\n"
    "                if (result)\n"
    "                    videoDrawRects();"))

declarations = "    unsigned char aborted;\n"
assert body.count(declarations) == 1
for spelling, typename in (("byte", "unsigned char"), ("int", "int"),
                           ("bool", "bool")):
    changed = body.replace(declarations,
        declarations + "    %s needsUpdate;\n" % typename)
    changed = changed.replace(site,
        "                needsUpdate = videoNeedsUpdate();\n"
        "                if (needsUpdate)\n"
        "                    videoDrawRects();")
    add("function-owner-" + spelling, changed)

add("explicit-nonzero", body.replace(
    "if (videoNeedsUpdate())", "if (videoNeedsUpdate() != 0)"))
add("double-negation", body.replace(
    "if (videoNeedsUpdate())", "if (!!videoNeedsUpdate())"))

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "inlined-update-result-owner",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Retail and candidate both expand the canonical videoNeedsUpdate helper at the loop tail, with the same call boundary and branch structure.",
        "The global-color trace shows a high-priority byte-constrained group sharing EBX with the centered x lifetime. Naming the real helper result can change that ownership without adding an operation.",
        "Dreamcast 0x14ac38 is a platform stub and supplies no Complete-body local name or type, so byte, promoted-int, bool, and the existing return-byte owner are tested as bounded alternatives.",
        "Every option evaluates videoNeedsUpdate exactly once at the same control point and preserves the helper call, behavior, calls, and branches.",
    ],
}, indent=2) + "\n")
