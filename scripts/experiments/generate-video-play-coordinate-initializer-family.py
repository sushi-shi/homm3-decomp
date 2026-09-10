#!/usr/bin/env python3
"""Retail videoPlay centered-coordinate declaration initializers."""
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

point_decl = "    POINT pos;\n"
dimension_decl = "    int vw, vh;\n"
dimension_setup = "        vh = h;\n        vw = w;"
x_pair = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;"""
y_pair = """            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
for anchor in (point_decl, dimension_decl, dimension_setup, x_pair, y_pair):
    assert body.count(anchor) == 1, anchor


def dimensions(changed, form):
    if form == "assigned-vh-vw":
        return changed
    changed = changed.replace(dimension_decl, "")
    changed = changed.replace(dimension_setup, "")
    if form == "initialized-vh-vw":
        declaration = "    int vh = h;\n    int vw = w;\n"
    elif form == "initialized-vw-vh":
        declaration = "    int vw = w;\n    int vh = h;\n"
    else:
        declaration = "    int vh = h, vw = w;\n"
    return changed.replace(point_decl, declaration)


def coordinates(changed, typ, order, qualifier, chain):
    x_name = "updateX"
    y_name = "updateY"
    if order == "yx-declared":
        # Declare the later-born y owner first, while keeping the retail x
        # computation/global store before the y computation.
        declaration = "    %s%s updateY;\n" % (qualifier, typ)
        changed = changed.replace("    unsigned char result;",
                                  declaration + "    unsigned char result;")
        y_decl = "            updateY ="
    else:
        y_decl = "            %s%s updateY =" % (qualifier, typ)
    x_decl = "            %s%s updateX =" % (qualifier, typ)
    if chain == "initializer-then-global":
        x_text = "%s x + (vw - g_smackVideo->m_width) / 2;\n            g_smackX = updateX;" % x_decl
        y_text = "%s y + (vh - g_smackVideo->m_height) / 2;\n            g_smackY = updateY;" % y_decl
    elif chain == "global-in-initializer":
        x_text = "%s (g_smackX = x + (vw - g_smackVideo->m_width) / 2);" % x_decl
        y_text = "%s (g_smackY = y + (vh - g_smackVideo->m_height) / 2);" % y_decl
    else:
        x_text = "            g_smackX = %s x + (vw - g_smackVideo->m_width) / 2;" % x_decl.strip()
        y_text = "            g_smackY = %s y + (vh - g_smackVideo->m_height) / 2;" % y_decl.strip()
    changed = changed.replace(x_pair, x_text).replace(y_pair, y_text)
    return changed.replace("pos.x", x_name).replace("pos.y", y_name).replace(point_decl, "")


options = [{"name": "point-control"}]
for typ, order, qualifier, chain, dimension_form in itertools.product(
        ("int", "long"),
        ("xy-born", "yx-declared"),
        ("", "const "),
        ("initializer-then-global", "global-in-initializer"),
        ("assigned-vh-vw", "initialized-vh-vw", "initialized-vw-vh",
         "combined-vh-vw")):
    # A const y owner cannot be declared without its initializer.
    if qualifier and order == "yx-declared":
        continue
    changed = dimensions(body, dimension_form)
    changed = coordinates(changed, typ, order, qualifier, chain)
    options.append({
        "name": "%s-%s-%s-%s-%s" % (
            qualifier.strip() or "mutable", typ, order, chain, dimension_form),
        "replace": changed,
    })

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "coordinate-declaration-initializers",
        "find": body,
        "options": options,
    }],
    "evidence": [
        "Dreamcast 0x14ac38 proves the five-int signature but its platform stub has no Complete-body locals; retail 0x5972d0 supplies the centered-coordinate operations and homes.",
        "Prior scalar families declared updateX/updateY before their first assignments. VC6 creates optimizer values at first assignment, and a declaration initializer is a distinct C1 boundary even when it emits the same arithmetic.",
        "Test int/long and const/mutable coordinate owners born at each retail computation, with the exact x-global-y-global order, crossed with the natural extent initialization forms already supported by the playBinkVideo twin.",
        "Every state preserves values, global stores, draw/update arguments, calls, branches, and interfaces; no dummy operation or compiler directive.",
    ],
}, indent=2) + "\n")
