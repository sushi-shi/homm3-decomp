#!/usr/bin/env python3
"""Retail videoPlay scalar coordinate and extent ownership scopes."""
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

point = "    POINT pos;\n"
dims = "    int vw, vh;\n"
copies = "        vh = h;\n        vw = w;"
selected = "                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {\n"
success = "        } else {\n            g_mouseManager->hidePointer();"
geometry = "            pos.x = x + (vw - g_smackVideo->m_width) / 2;"
for anchor in (point, dims, copies, selected, success, geometry):
    assert body.count(anchor) == 1, anchor


def coordinate_scope(changed, typ, order, scope):
    first, second = (("updateX", "updateY") if order == "xy"
                     else ("updateY", "updateX"))
    declaration = "%s %s, %s;" % (typ, first, second)
    changed = changed.replace(point, "")
    if scope == "function":
        changed = changed.replace(dims, "    " + declaration + "\n" + dims)
    elif scope == "selected":
        changed = changed.replace(selected, selected + "        " + declaration + "\n")
    elif scope == "success":
        changed = changed.replace(success,
            "        } else {\n            " + declaration + "\n            g_mouseManager->hidePointer();")
    else:
        changed = changed.replace(geometry, "            " + declaration + "\n" + geometry)
    return changed.replace("pos.x", "updateX").replace("pos.y", "updateY")


def dimension_scope(changed, scope):
    if scope == "function":
        return changed
    changed = changed.replace(dims, "")
    changed = changed.replace(copies, "")
    if scope == "selected-assigned":
        text = "        int vw, vh;\n        vh = h;\n        vw = w;\n"
    elif scope == "selected-init-separate":
        text = "        int vh = h;\n        int vw = w;\n"
    else:
        text = "        int vh = h, vw = w;\n"
    return changed.replace(selected, selected + text)


options = [{"name": "control"}]
for typ, order, coordinate, dimensions in itertools.product(
        ("int", "long"), ("xy", "yx"),
        ("function", "selected", "success", "geometry"),
        ("function", "selected-assigned", "selected-init-separate", "selected-init-combined")):
    changed = dimension_scope(coordinate_scope(body, typ, order, coordinate), dimensions)
    options.append({
        "name": "%s-%s-%s-coordinates-%s-dimensions" % (
            typ, order, coordinate, dimensions),
        "replace": changed,
    })

args.output.write_text(json.dumps({
    "schema": 1,
    "source": "src/smackmgr.cpp",
    "units": ["smackmgr"],
    "axes": [{
        "name": "scalar-coordinate-and-extent-scopes",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast proves the five-int signature but its videoPlay body is a platform stub; retail fixes the emitted operations and stack homes.",
        "The sibling playBinkVideo names the centered values updateX/updateY. Retail videoPlay does not require either centered value before the successful Smacker arm.",
        "Previous families tested scalar owners only at function scope and POINT scopes separately. Cross ordinary int/long scalar declarations at function, selected, success, and geometry scopes with the real extent declaration scopes.",
        "Sixty-five finite states preserve every value, operation, branch, call, and helper boundary; no dummy declaration or compiler directive.",
    ],
}, indent=2) + "\n")
