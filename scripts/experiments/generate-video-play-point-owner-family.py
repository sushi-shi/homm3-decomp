#!/usr/bin/env python3
"""Retail videoPlay POINT pointer/reference ownership."""
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

point = "    POINT pos;"
selected = "                && *g_videoGameState != VIDEO_GAME_STATE_FORCED_BINK_HIGH))) {"
inner = "        } else {\n            g_mouseManager->hidePointer();"
xline = "            pos.x = x + (vw - g_smackVideo->m_width) / 2;"
call = "            _SmackToBuffer(g_smackVideo, pos.x, pos.y,"
update = "                g_windowManager->updateScreen(pos.x, pos.y, vw, vh);"
for anchor in (point, selected, inner, xline, call, update):
    assert body.count(anchor) == 1


def owner(kind, placement, interval):
    changed = body
    if kind == "pointer":
        declaration, member = "POINT* position", "position->"
    else:
        declaration, member = "POINT& position", "position."
    if placement == "function":
        changed = changed.replace(point, point + "\n    " + declaration + " = pos;") if kind == "reference" else changed.replace(
            point, point + "\n    " + declaration + " = &pos;")
    elif placement == "selected":
        init = declaration + (" = pos;" if kind == "reference" else " = &pos;")
        changed = changed.replace(selected, selected + "\n        " + init)
    elif placement == "inner":
        init = declaration + (" = pos;" if kind == "reference" else " = &pos;")
        changed = changed.replace(inner, "        } else {\n            " + init + "\n            g_mouseManager->hidePointer();")
    else:
        init = declaration + (" = pos;" if kind == "reference" else " = &pos;")
        changed = changed.replace(xline, "            " + init + "\n" + xline)
    replacement = member
    if interval == "all":
        changed = changed.replace("pos.", replacement)
    elif interval == "draw-update":
        at = changed.index(call)
        changed = changed[:at] + changed[at:].replace("pos.", replacement)
    else:
        changed = changed.replace(update, update.replace("pos.", replacement))
    return changed


options = [{"name": "direct-point"}]
for kind in ("pointer", "reference"):
    for placement in ("function", "selected", "inner", "before-geometry"):
        intervals = ("all", "draw-update", "update") if placement != "before-geometry" else (
            "all", "draw-update")
        for interval in intervals:
            options.append({
                "name": "%s-%s-%s" % (kind, placement, interval),
                "replace": owner(kind, placement, interval),
            })

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "point-owner", "find": body, "options": options}],
    "evidence": [
        "Dreamcast's platform stub gives no Complete local inventory. Retail and candidate agree on the POINT-sized frame region, but retail homes both members while the candidate promotes pos.x to EBX.",
        "A pointer or reference bound to the real POINT at a controlling scope can make its addressability and ownership explicit without adding an object or operation. Test whether geometry, draw, or deferred update consumed that owner.",
        "Twenty-three finite states preserve the POINT, calculations, global writes, SDK arguments, calls, branches, and interfaces; no dummy use or compiler directive.",
    ],
}, indent=2) + "\n")
