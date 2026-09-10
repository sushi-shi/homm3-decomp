#!/usr/bin/env python3
"""Retail videoPlay loop-zero ownership and birth point."""
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

flush_loop = """            g_inputManager->flush();
            while (1) {
                if (g_smackVideo == 0)"""
abort_flush = """            aborted = 0;
            g_inputManager->flush();
            while (1) {
                if (g_smackVideo == 0)"""
show = "            g_mouseManager->showPointer(0);"
fade = "                g_windowManager->fadeScreen(1, 4, 0);"
for anchor in (flush_loop, abort_flush, show, fade):
    assert body.count(anchor) == 1, anchor

forms = [("control", body)]

changed = body.replace(flush_loop, """            g_inputManager->flush();
            int zero = 0;
            while (1) {
                if (g_smackVideo == zero)""")
forms.append(("int-at-loop-guard", changed))
forms.append(("int-at-loop-guard-and-late-arguments",
              changed.replace(show, "            g_mouseManager->showPointer(zero);")
                     .replace(fade, "                g_windowManager->fadeScreen(1, 4, zero);")))

changed = body.replace(abort_flush, """            int zero = 0;
            aborted = zero;
            g_inputManager->flush();
            while (1) {
                if (g_smackVideo == zero)""")
forms.append(("int-at-abort-initialization", changed))
forms.append(("int-at-abort-and-late-arguments",
              changed.replace(show, "            g_mouseManager->showPointer(zero);")
                     .replace(fade, "                g_windowManager->fadeScreen(1, 4, zero);")))

changed = body.replace(flush_loop, """            g_inputManager->flush();
            Smack* noVideo = 0;
            while (1) {
                if (g_smackVideo == noVideo)""")
forms.append(("pointer-at-loop-guard", changed))

changed = body.replace(flush_loop, """            g_inputManager->flush();
            bool noVideo = false;
            while (1) {
                if ((g_smackVideo == 0) != noVideo)""")
forms.append(("bool-at-loop-guard", changed))

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
        "name": "loop-zero-owner",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub. Retail first materializes zero in EBX at the wait-loop header, then reuses it across the loop guard, inlined videoNeedsUpdate, inlined closeSmacker, fade/show arguments, and cleanup stores.",
        "The current source uses literal zero throughout and keeps pos.x in EBX, leaving no zero register; the compiler emits repeated load/test sequences. A real sentinel born after flush is the source-level lifetime that can force the POINT home while preserving retail's late zero initialization.",
        "Test signed-int, null-pointer, and boolean owners at the observed birth point, plus actual later zero arguments. Every comparison, assignment, call, branch, and helper boundary remains semantically unchanged.",
        "Seven finite states; no unused declaration, dummy expression, alternate API, or compiler directive.",
    ],
}, indent=2) + "\n")
