#!/usr/bin/env python3
"""Retail videoPlay Smacker dimension value lifetimes."""
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

hide = "            g_mouseManager->hidePointer();"
geometry = """            if (vw < 0)
                vw = g_smackVideo->m_width;
            if (vh < 0)
                vh = g_smackVideo->m_height;
            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
call = "_SmackToBuffer(g_smackVideo, pos.x, pos.y,"
assert body.count(hide) == 1
assert body.count(geometry) == 1
assert body.count(call) == 1


def owner(changed, mode):
    if mode == "global":
        return changed, "g_smackVideo"
    if mode == "pointer":
        declaration = "\n            Smack* smk = g_smackVideo;"
        expression = "smk"
    else:
        declaration = "\n            Smack& smk = *g_smackVideo;"
        expression = "&smk"
    changed = changed.replace(hide, hide + declaration)
    changed = changed.replace("g_smackVideo->", expression + "->") if mode == "pointer" else changed.replace("g_smackVideo->", "smk.")
    changed = changed.replace(call, "_SmackToBuffer(" + expression + ", pos.x, pos.y,")
    return changed, expression


def dimensions(changed, mode):
    prefix = "smk." if "Smack& smk" in changed else ("smk->" if "Smack* smk" in changed else "g_smackVideo->")
    field_width = prefix + "m_width"
    field_height = prefix + "m_height"
    checks = """            if (vw < 0)
                vw = FIELD_WIDTH;
            if (vh < 0)
                vh = FIELD_HEIGHT;""".replace("FIELD_WIDTH", field_width).replace(
                    "FIELD_HEIGHT", field_height)
    if mode == "control":
        return changed
    typ, layout = mode.split("-", 1)
    if layout == "at-use":
        replacement = checks + """
            %s videoWidth = FIELD_WIDTH;
            pos.x = x + (vw - videoWidth) / 2;
            g_smackX = pos.x;
            %s videoHeight = FIELD_HEIGHT;
            pos.y = y + (vh - videoHeight) / 2;
            g_smackY = pos.y;""" % (typ, typ)
    elif layout == "both-before":
        replacement = checks + """
            %s videoWidth = FIELD_WIDTH;
            %s videoHeight = FIELD_HEIGHT;
            pos.x = x + (vw - videoWidth) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - videoHeight) / 2;
            g_smackY = pos.y;""" % (typ, typ)
    else:
        replacement = checks + """
            %s videoHeight = FIELD_HEIGHT;
            %s videoWidth = FIELD_WIDTH;
            pos.x = x + (vw - videoWidth) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - videoHeight) / 2;
            g_smackY = pos.y;""" % (typ, typ)
    replacement = replacement.replace("FIELD_WIDTH", field_width).replace(
        "FIELD_HEIGHT", field_height)
    return changed.replace(geometry, replacement)


forms = [("control", body)]
dimension_modes = [
    "int-at-use", "long-at-use", "unsigned-long-at-use",
    "int-both-before", "long-both-before", "unsigned-long-both-before",
    "int-height-first", "long-height-first", "unsigned-long-height-first",
]
for owner_mode, dimension_mode in itertools.product(
        ("global", "pointer", "reference"), dimension_modes):
    changed = dimensions(body, dimension_mode)
    changed, _ = owner(changed, owner_mode)
    forms.append((owner_mode + "-" + dimension_mode, changed))

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
        "name": "smacker-dimension-value-lifetimes",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 is a platform stub; retail 0x5972d0 supplies the Complete-body value and register evidence.",
        "After the two negative-extent repairs, retail loads m_width into EDI immediately before the x calculation and m_height into EBX immediately before y. Those values are each consumed once and their registers reuse the dead initial vh and x roles.",
        "Test ordinary signed or SDK-native unsigned value bindings at those exact consumption points, alone or with the byte-flat active Smacker owner. The fields used by the repair tests remain direct and are reloaded as retail requires.",
        "Twenty-eight finite states preserve values, load count, calls, branches, deferred POINT, and SDK interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
