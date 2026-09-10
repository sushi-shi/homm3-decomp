#!/usr/bin/env python3
"""Retail videoPlay unsigned center-offset arithmetic stages."""
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

x_line = "            pos.x = x + (vw - g_smackVideo->m_width) / 2;"
y_line = "            pos.y = y + (vh - g_smackVideo->m_height) / 2;"
declarations = """    POINT pos;
    int vw, vh;
    unsigned char result;
    unsigned char aborted;"""
assert body.count(x_line) == 1
assert body.count(y_line) == 1
assert body.count(declarations) == 1


options = [{"name": "control"}]
for typ in ("unsigned int", "unsigned long"):
    for mask in range(1, 4):
        for placement in ("at-use", "declared"):
            changed = body
            declarations_to_add = []
            if mask & 1:
                if placement == "at-use":
                    replacement = ("            %s xOffset = (vw - g_smackVideo->m_width) / 2;\n"
                                   "            pos.x = x + xOffset;") % typ
                else:
                    declarations_to_add.append("xOffset")
                    replacement = ("            xOffset = (vw - g_smackVideo->m_width) / 2;\n"
                                   "            pos.x = x + xOffset;")
                changed = changed.replace(x_line, replacement)
            if mask & 2:
                if placement == "at-use":
                    replacement = ("            %s yOffset = (vh - g_smackVideo->m_height) / 2;\n"
                                   "            pos.y = y + yOffset;") % typ
                else:
                    declarations_to_add.append("yOffset")
                    replacement = ("            yOffset = (vh - g_smackVideo->m_height) / 2;\n"
                                   "            pos.y = y + yOffset;")
                changed = changed.replace(y_line, replacement)
            if declarations_to_add:
                changed = changed.replace(declarations, declarations + "\n    " + typ + " "
                                          + ", ".join(declarations_to_add) + ";")
            options.append({
                "name": "%s-%s-%s" % (typ.replace(" ", "-"),
                    ("x" if mask == 1 else "y" if mask == 2 else "xy"), placement),
                "replace": changed,
            })

for typ in ("unsigned int", "unsigned long"):
    changed = body.replace(declarations, declarations + "\n    " + typ + " offset;")
    changed = changed.replace(x_line,
        "            offset = (vw - g_smackVideo->m_width) / 2;\n            pos.x = x + offset;")
    changed = changed.replace(y_line,
        "            offset = (vh - g_smackVideo->m_height) / 2;\n            pos.y = y + offset;")
    options.append({"name": typ.replace(" ", "-") + "-shared-offset", "replace": changed})

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "center-offset-stages",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub; retail 0x5972d0 supplies the Complete arithmetic and register evidence.",
        "Smack width/height are unsigned long and retail divides each difference with SHR. The center offsets are therefore unsigned values even though the destination POINT fields are signed long.",
        "Retail allocates vw first, vh second, and x third; the candidate allocates vw, x, then vh. Test named x/y offset stages at their use or declaration boundary, including one reused offset, while retaining the proven POINT frame.",
        "Fifteen finite states optimize to the same centered arithmetic when the stage has no independent machine lifetime. Calls, stores, branches, and helper boundaries stay unchanged.",
    ],
}, indent=2) + "\n")
