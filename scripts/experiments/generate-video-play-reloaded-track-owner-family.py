#!/usr/bin/env python3
"""Retail videoPlay selected Smack owner with a post-hide reload."""
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
show_test = """        showVideo(id, x, y, vw, vh, 0, 0, 1);
        if (!g_smackVideo) {"""
hide = "            g_mouseManager->hidePointer();"
call = "_SmackToBuffer(g_smackVideo, pos.x, pos.y,"
for anchor in (declarations, show_test, hide, call):
    assert body.count(anchor) == 1


def use_geometry(changed, name):
    changed = changed.replace("g_smackVideo->m_width", name + "->m_width")
    changed = changed.replace("g_smackVideo->m_height", name + "->m_height")
    return changed.replace(call, "_SmackToBuffer(" + name + ", pos.x, pos.y,")


forms = [("control", body)]
for declaration in ("function", "selected"):
    for initial_use in ("test", "test-only"):
        for reload_at in ("after-hide", "before-hide"):
            changed = body
            if declaration == "function":
                changed = changed.replace(declarations, declarations + "\n    Smack* smk;")
                changed = changed.replace(show_test,
                    "        showVideo(id, x, y, vw, vh, 0, 0, 1);\n        smk = g_smackVideo;\n        if (!smk) {")
            else:
                changed = changed.replace(show_test,
                    "        showVideo(id, x, y, vw, vh, 0, 0, 1);\n        Smack* smk = g_smackVideo;\n        if (!smk) {")
            assignment = "            smk = g_smackVideo;"
            if initial_use == "test":
                changed = changed.replace(hide,
                    (hide + "\n" + assignment) if reload_at == "after-hide" else
                    (assignment + "\n" + hide))
                changed = use_geometry(changed, "smk")
            forms.append(("%s-%s-%s" % (declaration, initial_use, reload_at), changed))

# Distinct owners make the source's two handle snapshots explicit.
changed = body.replace(show_test,
    "        showVideo(id, x, y, vw, vh, 0, 0, 1);\n        Smack* opened = g_smackVideo;\n        if (!opened) {")
changed = changed.replace(hide, hide + "\n            Smack* smk = g_smackVideo;")
forms.append(("two-selected-owners", use_geometry(changed, "smk")))

options = []
seen = set()
for name, changed in forms:
    if changed in seen:
        continue
    seen.add(changed)
    option = {"name": name}
    if changed != body:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "reloaded-track-owner", "find": body, "options": options}],
    "evidence": [
        "Retail loads g_smackVideo for the post-show success test, calls hidePointer, then reloads g_smackVideo into ESI before the geometry fields. A single cached value across hidePointer would not preserve that observable global reload.",
        "Model one real pointer owner assigned from the global at both observed points, or two owners for the two snapshots. The loop continues to read the global directly.",
        "The earlier active-owner family created its pointer only around geometry and did not bind the initial success test. These finite forms preserve every load boundary, value, field access, call, branch, POINT, and interface.",
    ],
}, indent=2) + "\n")
