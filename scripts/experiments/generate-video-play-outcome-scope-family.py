#!/usr/bin/env python3
"""Retail videoPlay result and abort flag lexical lifetimes."""
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

result_decl = "    unsigned char result;\n"
aborted_decl = "    unsigned char aborted;\n"
selected = """                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {
"""
show = "        showVideo(id, x, y, vw, vh, 0, 0, 1);\n"
inner = """        } else {
            g_mouseManager->hidePointer();"""
initialize = "            aborted = 0;"
for anchor in (result_decl, aborted_decl, selected, show, inner, initialize):
    assert body.count(anchor) == 1, anchor


def result_scope(changed, scope):
    if scope == "function":
        return changed
    changed = changed.replace(result_decl, "")
    if scope == "selected":
        return changed.replace(selected, selected + "        unsigned char result;\n")
    return changed.replace(show, show + "        unsigned char result;\n")


def aborted_scope(changed, scope):
    if scope == "function":
        return changed
    changed = changed.replace(aborted_decl, "")
    if scope == "selected":
        return changed.replace(selected, selected + "        unsigned char aborted;\n")
    if scope == "inner":
        return changed.replace(inner, "        } else {\n            unsigned char aborted;\n            g_mouseManager->hidePointer();")
    return changed.replace(initialize, "            unsigned char aborted;\n" + initialize)


options = []
for result, aborted in itertools.product(
        ("function", "selected", "after-show"),
        ("function", "selected", "inner", "at-initialization")):
    changed = aborted_scope(result_scope(body, result), aborted)
    option = {"name": "result-%s-aborted-%s" % (result, aborted)}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "outcome-lexical-lifetimes",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast's 0x14ac38 platform stub exposes no Complete-body scopes. Retail uses one result byte only in the selected Smacker arm and one abort byte only after a Smacker handle opens.",
        "Both byte locals are currently declared at function scope despite those narrower ownership intervals. why-reg isolates a C1 pseudo processing-order divergence, and lexical birth points are a measured VC6 lever even when stack coloring later reuses argument bytes.",
        "Test each actual owner's natural controlling scope and first-use boundary while preserving every statement, value, call, branch, and type.",
        "Twelve finite states; no dummy operation, alternate API, or compiler directive.",
    ],
}, indent=2) + "\n")
