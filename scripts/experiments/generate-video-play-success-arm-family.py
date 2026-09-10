#!/usr/bin/env python3
"""Retail videoPlay post-open success/failure ownership family."""
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

head = """        if (!g_smackVideo) {
            result = 0;
        } else {
"""
tail_marker = "        g_smackPaused = 0;"
assert body.count(head) == 1
assert body.count(tail_marker) == 1
block_start = body.index(head)
tail_start = body.index(tail_marker)
current_block = body[block_start:tail_start]
assert current_block.endswith("        }\n")
success = current_block[len(head):-len("        }\n")]
assert "            result = !aborted;\n" in success


def replace_block(replacement):
    return body[:block_start] + replacement + body[tail_start:]


positive = (
    "        if (g_smackVideo) {\n" + success +
    "        } else {\n"
    "            result = 0;\n"
    "        }\n")

default_then_success = (
    "        result = 0;\n"
    "        if (g_smackVideo) {\n" + success +
    "        }\n")

default_then_negated_guard = (
    "        result = 0;\n"
    "        if (!g_smackVideo)\n"
    "            goto playbackDone;\n" +
    success.replace("            ", "        ", 1) +
    "playbackDone:\n")

failure_goto = (
    "        if (!g_smackVideo) {\n"
    "            result = 0;\n"
    "            goto playbackDone;\n"
    "        }\n" +
    success.replace("            ", "        ", 1) +
    "playbackDone:\n")

# Keep the same source operations but place the result selection after the
# success arm. This form is valid because aborted is defined on every success
# path and the failure arm jumps over its read.
success_without_result = success.replace("            result = !aborted;\n", "")
late_result = (
    "        if (!g_smackVideo) {\n"
    "            result = 0;\n"
    "        } else {\n" + success_without_result +
    "            result = !aborted;\n"
    "        }\n")

forms = [
    ("negated-failure-current", body),
    ("positive-success-else-failure", replace_block(positive)),
    ("default-result-positive-success", replace_block(default_then_success)),
    ("default-result-negated-guard-goto", replace_block(default_then_negated_guard)),
    ("failure-arm-goto", replace_block(failure_goto)),
    ("negated-failure-late-result", replace_block(late_result)),
]

options = []
for name, changed in forms:
    option = {"name": name}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "post-open-success-and-result-owner",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub. Retail 0x5972d0 tests g_smackVideo after showVideo, leaves failure as fallthrough, stores result zero there, and jumps to the shared cleanup; the success path stores !aborted before that cleanup.",
        "That exact CFG can arise from the current negated if/else, a positive success arm with an else, or a zero default followed by a success-only arm. The source ownership changes result's live range and can change the x/height interference priority while preserving emitted control flow.",
        "Two explicit cleanup labels are finite controls for the observed shared epilogue. Every state preserves one result-zero store, one !aborted selection, all calls and side effects, the POINT, dimensions, and playback loop.",
        "Six finite source shapes; no dummy operation, helper rewrite, compiler directive, or repeated expression.",
    ],
}, indent=2) + "\n")
