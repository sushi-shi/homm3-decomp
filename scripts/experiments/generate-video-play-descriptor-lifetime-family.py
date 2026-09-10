#!/usr/bin/env python3
"""Retail videoPlay descriptor address lifetime without changing its gate CFG."""
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
condition_use = "!g_videoDescriptors[id].m_useBink"
fade_use = "g_videoDescriptors[id].m_fadeOnAbort"
assert body.count(declarations) == 1
assert body.count(condition_use) == 1
assert body.count(fade_use) == 1


def pointer(initializer, condition, const=False):
    qualifier = "const " if const else ""
    changed = body.replace(declarations, declarations + "\n    %sSVideoDescriptor* descriptor%s;" % (
        qualifier, " = " + initializer if initializer else ""))
    changed = changed.replace(condition_use, condition)
    return changed.replace(fade_use, "descriptor->m_fadeOnAbort")


forms = [("control", body)]
for expression_name, expression in (
        ("subscript", "&g_videoDescriptors[id]"),
        ("addition", "g_videoDescriptors + id")):
    forms.append(("entry-pointer-" + expression_name,
                  pointer(expression, "!descriptor->m_useBink")))
    forms.append(("entry-const-pointer-" + expression_name,
                  pointer(expression, "!descriptor->m_useBink", True)))

forms.append(("condition-assigned-pointer",
              pointer("", "!(descriptor = &g_videoDescriptors[id])->m_useBink")))
forms.append(("condition-assigned-const-pointer",
              pointer("", "!(descriptor = &g_videoDescriptors[id])->m_useBink", True)))

reference = body.replace(declarations,
    declarations + "\n    SVideoDescriptor& descriptor = g_videoDescriptors[id];")
reference = reference.replace(condition_use, "!descriptor.m_useBink")
reference = reference.replace(fade_use, "descriptor.m_fadeOnAbort")
forms.append(("entry-reference", reference))

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
        "name": "descriptor-address-lifetime",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub; retail 0x5972d0 supplies the Complete descriptor lifetime.",
        "Retail computes g_videoDescriptors + 20*id only after the id range branch, stores the address at ebp-4, reads m_useBink through it, and reloads the same home for m_fadeOnAbort after the playback loop.",
        "The current repeated subscripts let C2 synthesize that home. Test the missing natural source fact directly: one pointer or reference owner spanning the gate and fade read, expressed without changing the proven short-circuit predicate or fallback CFG.",
        "Eight finite states preserve reads, values, calls, branches, POINT storage, and SDK interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
