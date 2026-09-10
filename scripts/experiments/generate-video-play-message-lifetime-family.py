#!/usr/bin/env python3
"""Retail videoPlay event-message construction and scope family."""
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

declarations = """    POINT pos;
    int vw, vh;
    unsigned char result;
    unsigned char aborted;"""
flush_loop = """            g_inputManager->flush();
            while (1) {"""
loop_head = """            while (1) {
                if (g_smackVideo == 0)"""
copy_init = "                    message msg = g_inputManager->getEvent();"
for anchor in (declarations, flush_loop, loop_head, copy_init):
    assert body.count(anchor) == 1, anchor

options = [{"name": "inner-copy-initialization"}]

changed = body.replace(copy_init,
    "                    message msg;\n                    msg = g_inputManager->getEvent();")
options.append({"name": "inner-default-then-assign", "replace": changed})

changed = body.replace(declarations, declarations + "\n    message msg;")
changed = changed.replace(copy_init, "                    msg = g_inputManager->getEvent();")
options.append({"name": "function-message-assign", "replace": changed})

changed = body.replace(flush_loop,
    "            g_inputManager->flush();\n            message msg;\n            while (1) {")
changed = changed.replace(copy_init, "                    msg = g_inputManager->getEvent();")
options.append({"name": "playback-arm-message-assign", "replace": changed})

changed = body.replace(loop_head,
    "            while (1) {\n                message msg;\n                if (g_smackVideo == 0)")
changed = changed.replace(copy_init, "                    msg = g_inputManager->getEvent();")
options.append({"name": "loop-message-assign", "replace": changed})

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "event-message-lifetime",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub and has no Complete-body scopes. Retail 0x5972d0 supplies the message-copy and stack evidence.",
        "Retail and candidate both receive getEvent into a hidden message at ebp-0x4c and copy eight dwords into the consumed message at ebp-0x2c. The message class has its Dreamcast-proven default constructor and implicit POD assignment.",
        "Test copy initialization or a default-constructed message assigned at the inner block, loop, owning playback arm, or function scope. VC6 may remove overwritten constructor stores while retaining a different C1 object lifetime.",
        "Five finite states preserve the getEvent call, copy, switch, branches, field reads, and loop shape.",
    ],
}, indent=2) + "\n")
