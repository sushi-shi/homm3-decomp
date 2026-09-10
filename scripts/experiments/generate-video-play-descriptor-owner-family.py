#!/usr/bin/env python3
"""Retail videoPlay descriptor ownership and primary-arm nesting."""
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

condition = """    if (id >= VIDEO_ID_FIRST_TABLED
        && (!g_videoDescriptors[id].m_useBink || !g_unnamed698758.m_binkVideo
            || (id == VIDEO_ID_STATE_GATED
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_LOW
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {"""
tail = """        return result;
    }
    return playBinkVideo(id, x, y, w, h);"""
assert body.count(condition) == 1
assert body.count(tail) == 1
assert body.count("g_videoDescriptors[id].m_fadeOnAbort") == 1

gate = """!OWNER.m_useBink || !g_unnamed698758.m_binkVideo
            || (id == VIDEO_ID_STATE_GATED
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_LOW
                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH)"""


def owned(kind, nested):
    if kind == "reference":
        declaration = "        SVideoDescriptor& descriptor = g_videoDescriptors[id];"
        owner = "descriptor"
    elif kind == "pointer":
        declaration = "        SVideoDescriptor* descriptor = &g_videoDescriptors[id];"
        owner = "(*descriptor)"
    else:
        declaration = "        const SVideoDescriptor* descriptor = &g_videoDescriptors[id];"
        owner = "(*descriptor)"

    primary = gate.replace("OWNER", owner)
    changed = body
    if nested:
        replacement = """    if (id >= VIDEO_ID_FIRST_TABLED) {
%s
        if (%s) {""" % (declaration, primary)
        changed = changed.replace(condition, replacement)
        changed = changed.replace(tail, """            return result;
        }
    }
    return playBinkVideo(id, x, y, w, h);""")
        changed = changed.replace("\n        vh = h;", "\n            vh = h;", 1)
        # The function body's selected arm gained one level. Indentation does
        # not affect the source-family replacement or generated object.
    else:
        # Keep the short-circuit CFG while introducing a real pointer owner
        # only after the id range test through an assignment expression.
        decl = ("    SVideoDescriptor* descriptor;\n" if kind != "const-pointer"
                else "    const SVideoDescriptor* descriptor;\n")
        changed = changed.replace("    POINT pos;\n", "    POINT pos;\n" + decl)
        assignment_owner = "*(descriptor = &g_videoDescriptors[id])"
        changed = changed.replace(condition, condition.replace(
            "g_videoDescriptors[id]", assignment_owner, 1))
        changed = changed.replace("g_videoDescriptors[id].m_fadeOnAbort",
                                  "descriptor->m_fadeOnAbort")
        return changed

    changed = changed.replace("g_videoDescriptors[id].m_fadeOnAbort",
                              owner + ".m_fadeOnAbort")
    return changed


forms = [("control", body)]
for kind in ("reference", "pointer", "const-pointer"):
    forms.append(("nested-" + kind, owned(kind, True)))
for kind in ("pointer", "const-pointer"):
    forms.append(("short-circuit-" + kind, owned(kind, False)))

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
        "name": "descriptor-owner-and-gate",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub; retail 0x5972d0 supplies the Complete-body ownership evidence.",
        "Retail computes one descriptor-row address after the id range branch, homes it at ebp-4, reads useBink through it at entry, and reloads the same home for fadeOnAbort after the playback loop.",
        "The current repeated subscripts let C2 synthesize that common address. Test whether the source owned the record through an ordinary reference or pointer, with the id guard expressed as its natural outer scope.",
        "Six finite states preserve values, short-circuiting, calls, branches, the deferred POINT, and SDK interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
