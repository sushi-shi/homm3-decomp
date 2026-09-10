#!/usr/bin/env python3
"""Cross byte-flat, evidence-compatible videoPlay source lifetimes."""
import argparse
import itertools
import json
import random
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
p.add_argument("--states", type=int, default=720)
p.add_argument("--seed", type=int, default=0x5972D0)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
start = source.index("int videoPlay(int id, int x, int y, int w, int h)\n{")
body = source[start:source.index("\n}\n", start) + 2]

point_decl = "    POINT pos;\n"
dimension_decl = "    int vw, vh;\n"
result_decl = "    unsigned char result;\n"
aborted_decl = "    unsigned char aborted;\n"
dimension_stores = "        vh = h;\n        vw = w;"
selected = "                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {\n"
after_show = "        showVideo(id, x, y, vw, vh, 0, 0, 1);\n"
inner = "        } else {\n            g_mouseManager->hidePointer();"
abort_init = "            aborted = 0;"
flush_loop = "            g_inputManager->flush();\n            while (1) {"
loop_head = "            while (1) {\n                if (g_smackVideo == 0)"
message_init = "                    message msg = g_inputManager->getEvent();"
coordinate_pair = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
for anchor in (point_decl, dimension_decl, result_decl, aborted_decl,
               dimension_stores, selected, after_show, inner, abort_init,
               flush_loop, loop_head, message_init, coordinate_pair):
    assert body.count(anchor) == 1, anchor


def geometry(changed, mode):
    if mode == "point-separate":
        return changed
    if mode == "point-chained":
        return changed.replace(coordinate_pair, """            g_smackX = pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackY = pos.y = y + (vh - g_smackVideo->m_height) / 2;""")
    return changed.replace(coordinate_pair, """            x += (vw - g_smackVideo->m_width) / 2;
            pos.x = x;
            g_smackX = pos.x;
            y += (vh - g_smackVideo->m_height) / 2;
            pos.y = y;
            g_smackY = pos.y;""")


def point_scope(changed, mode):
    if mode == "function":
        return changed
    changed = changed.replace(point_decl, "")
    if mode == "selected":
        return changed.replace(selected, selected + "        POINT pos;\n")
    return changed.replace(inner, "        } else {\n            POINT pos;\n            g_mouseManager->hidePointer();")


def dimensions(changed, mode):
    changed = changed.replace(dimension_decl, "")
    changed = changed.replace(dimension_stores, "")
    if mode == "function-vw-vh":
        changed = changed.replace(point_decl, point_decl + dimension_decl)
        return changed.replace(selected, selected + dimension_stores + "\n")
    if mode == "function-vh-vw":
        changed = changed.replace(point_decl, point_decl + "    int vh, vw;\n")
        return changed.replace(selected, selected + dimension_stores + "\n")
    if mode == "selected-assigned":
        return changed.replace(selected, selected + "        int vw, vh;\n" + dimension_stores + "\n")
    return changed.replace(selected, selected + "        int vh = h, vw = w;\n")


def flag_scopes(changed, mode):
    changed = changed.replace(result_decl, "").replace(aborted_decl, "")
    if mode == "function":
        return changed.replace("\n    if (id >=", result_decl + aborted_decl + "\n    if (id >=")
    if mode == "selected-both":
        return changed.replace(selected, selected + "        unsigned char result;\n        unsigned char aborted;\n")
    if mode.startswith("after-show-result"):
        changed = changed.replace(after_show, after_show + "        unsigned char result;\n")
    else:
        changed = changed.replace(selected, selected + "        unsigned char result;\n")
    if mode.endswith("inner-abort"):
        return changed.replace(inner, "        } else {\n            unsigned char aborted;\n            g_mouseManager->hidePointer();")
    return changed.replace(abort_init, "            unsigned char aborted;\n" + abort_init)


def message_lifetime(changed, mode):
    if mode == "inner-copy":
        return changed
    if mode == "inner-assign":
        return changed.replace(message_init,
            "                    message msg;\n                    msg = g_inputManager->getEvent();")
    changed = changed.replace(message_init, "                    msg = g_inputManager->getEvent();")
    if mode == "function-assign":
        return changed.replace("\n    if (id >=", "    message msg;\n\n    if (id >=")
    if mode == "arm-assign":
        return changed.replace(flush_loop,
            "            g_inputManager->flush();\n            message msg;\n            while (1) {")
    return changed.replace(loop_head,
        "            while (1) {\n                message msg;\n                if (g_smackVideo == 0)")


def loop_expression(changed, mode):
    loop, guard = mode
    return changed.replace("while (1)", "while (%s)" % loop).replace(
        "if (g_smackVideo == 0)\n                    break;",
        "if (%s)\n                    break;" % guard)


def event_exit(changed, mode):
    if mode == "shared-break":
        return changed
    accepted = """                            if (!g_videoNoSkip) {
                                aborted = 1;
                                break;
                            }"""
    shared = """                    if (aborted)
                        break;
"""
    close = "            SmackManager::closeSmacker();"
    assert changed.count(accepted) == 1
    assert changed.count(shared) == 1
    assert changed.count(close) == 1
    changed = changed.replace(accepted, """                            if (!g_videoNoSkip) {
                                aborted = 1;
                                goto stopPlayback;
                            }""")
    changed = changed.replace(shared, "")
    return changed.replace(close, "stopPlayback:\n" + close)


def failure_shape(changed, mode):
    if mode == "if-else":
        return changed
    head = """        if (!g_smackVideo) {
            result = 0;
        } else {
"""
    tail = "        g_smackPaused = 0;"
    block_start = changed.index(head)
    tail_start = changed.index(tail)
    current = changed[block_start:tail_start]
    assert current.endswith("        }\n")
    success = current[len(head):-len("        }\n")]
    replacement = ("        if (!g_smackVideo) {\n"
                   "            result = 0;\n"
                   "            goto playbackDone;\n"
                   "        }\n" + success.replace("            ", "        ", 1)
                   + "playbackDone:\n")
    return changed[:block_start] + replacement + changed[tail_start:]


geometries = ("point-separate", "point-chained", "parameter-copy-each")
point_scopes = ("function", "selected", "inner")
dimension_modes = ("function-vw-vh", "function-vh-vw",
                   "selected-assigned", "selected-initialized")
flag_modes = ("function", "selected-both", "selected-result-inner-abort",
              "selected-result-init-abort", "after-show-result-inner-abort",
              "after-show-result-init-abort")
message_modes = ("inner-copy", "inner-assign", "function-assign",
                 "arm-assign", "loop-assign")
loop_modes = tuple(itertools.product(
    ("1", "1L", "true", "1 == 1", "!0"),
    ("g_smackVideo == 0", "!g_smackVideo", "g_smackVideo == NULL",
     "NULL == g_smackVideo", "0 == g_smackVideo")))
failure_modes = ("if-else", "failure-goto")
event_modes = ("shared-break", "direct-goto")

product = list(itertools.product(geometries, point_scopes, dimension_modes,
                                 flag_modes, message_modes, loop_modes,
                                 failure_modes, event_modes))
control = ("point-separate", "function", "function-vw-vh", "function",
           "inner-copy", ("1", "g_smackVideo == 0"), "if-else",
           "shared-break")
rng = random.Random(args.seed)
rng.shuffle(product)
states = [control]
states.extend(item for item in product if item != control)
states = states[:args.states]

options = []
seen = set()
for state in states:
    geom, pscope, dims, flags, msg, loop, failure, event = state
    # Move dimensions before the POINT so its stable declaration anchor exists.
    changed = dimensions(body, dims)
    changed = geometry(changed, geom)
    changed = point_scope(changed, pscope)
    changed = flag_scopes(changed, flags)
    changed = message_lifetime(changed, msg)
    changed = loop_expression(changed, loop)
    changed = event_exit(changed, event)
    changed = failure_shape(changed, failure)
    if changed in seen:
        continue
    seen.add(changed)
    label = "%s_%s_%s_%s_%s_%s-%s_%s_%s" % (
        geom, pscope, dims, flags, msg, loop[0].replace(" ", ""),
        loop[1].replace(" ", "").replace("!", "not"), failure, event)
    option = {"name": label}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "cross-lifetimes", "find": body, "options": options}],
    "evidence": [
        "Each constituent choice previously compiled to the 87.1912% plateau with the retail CFG, calls, frame, operations, and helper boundaries preserved.",
        "C1 IL differs between at least the canonical and parameter-copy-each forms despite byte-identical C2 output. Cross their real coordinate, scope, extent, byte-result, message, loop-expression, and shared-cleanup owners to test non-additive front-end state.",
        "The deterministic sample contains only existing operations and source-equivalent lifetimes; it adds no dummy statement, false helper, compiler directive, or volatile object.",
    ],
}, indent=2) + "\n")
