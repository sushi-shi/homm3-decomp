#!/usr/bin/env python3
"""Retail videoPlay message scope crossed with declaration order."""
import argparse
import itertools
import json
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
start = source.index("int videoPlay(int id, int x, int y, int w, int h)\n{")
body = source[start:source.index("\n}\n", start) + 2]

declarations = """    POINT pos;
    int vw, vh;
    unsigned char result;
    unsigned char aborted;"""
copy_init = "                    message msg = g_inputManager->getEvent();"
selected = """                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {
        vh = h;"""
flush_loop = """            g_inputManager->flush();
            while (1) {"""
loop_head = """            while (1) {
                if (g_smackVideo == 0)"""
for anchor in (declarations, copy_init, selected, flush_loop, loop_head):
    assert body.count(anchor) == 1, anchor

groups = (
    "POINT pos;",
    "int vw, vh;",
    "unsigned char result;",
    "unsigned char aborted;",
    "message msg;",
)

options = [{"name": "inner-copy-control"}]
for order in itertools.permutations(groups):
    # The four existing declaration groups were already exhaustively flat.
    # Retain one representative for each position and neighboring owner of msg.
    msg_index = order.index("message msg;")
    left = order[msg_index - 1] if msg_index else "entry"
    right = order[msg_index + 1] if msg_index + 1 < len(order) else "body"
    if any(option["name"] == "function-msg-%d-after-%s-before-%s" % (
            msg_index, left.split()[0], right.split()[0]) for option in options):
        continue
    changed = body.replace(declarations, "\n".join("    " + line for line in order))
    changed = changed.replace(copy_init, "                    msg = g_inputManager->getEvent();")
    options.append({
        "name": "function-msg-%d-after-%s-before-%s" % (
            msg_index, left.split()[0], right.split()[0]),
        "replace": changed,
    })

changed = body.replace(copy_init, "                    msg = g_inputManager->getEvent();")
changed = changed.replace(selected, selected.replace(
    "        vh = h;", "        message msg;\n        vh = h;"))
options.append({"name": "selected-message", "replace": changed})

changed = body.replace(copy_init, "                    msg = g_inputManager->getEvent();")
changed = changed.replace(flush_loop,
    "            message msg;\n            g_inputManager->flush();\n            while (1) {")
options.append({"name": "before-flush-message", "replace": changed})

changed = body.replace(copy_init, "                    msg = g_inputManager->getEvent();")
changed = changed.replace(flush_loop,
    "            g_inputManager->flush();\n            message msg;\n            while (1) {")
options.append({"name": "before-loop-message", "replace": changed})

changed = body.replace(copy_init, "                    msg = g_inputManager->getEvent();")
changed = changed.replace(loop_head,
    "            while (1) {\n                message msg;\n                if (g_smackVideo == 0)")
options.append({"name": "loop-head-message", "replace": changed})

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "message-scope-and-declaration-order",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub. Retail proves a 32-byte message result at ebp-4c and a second consumed copy at ebp-2c, but does not prove the Complete source scope or declaration order.",
        "Copy initialization and default construction followed by assignment already emit identical message code. Cross the latter with the actual POINT, dimension, and byte-outcome declarations because all participate in one interference graph even when their bare declaration order is byte-flat.",
        "Function, selected-arm, pre-flush, pre-loop, and loop scopes preserve the one real message object and its getEvent assignment. Every state preserves calls, copies, branches, field reads, and the proven loop.",
        "The finite representatives cover each message position and neighboring declaration pair without duplicating the already exhausted four-owner permutations.",
    ],
}, indent=2) + "\n")
