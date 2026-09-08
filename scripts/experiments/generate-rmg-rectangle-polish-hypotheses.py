#!/usr/bin/env python3
"""Generate 60 bound/query/tile-lifetime hypotheses for the rectangle painter.

Retail 0x5b4960 walks one mutable unsigned grid point through end-exclusive
bounds, queries its terrain, and either paints it or selects and writes a base
tile. It expands initializePackedCell through the cache query; the current
source retains that nested call. Its configured-terrain read follows cache
initialization. Test the existing canonical isPaintTerrain predicate as well
as the direct comparison, actual bound locals, and constructed-tile lifetimes.
No copied helper body, artificial work, false inline declaration or pin.
The Complete-only RMG cluster has no Dreamcast source counterpart.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source, hypotheses

FUNCTION = "?paintRectangle@rmgTerrainPainter@@QAEXIIII@Z"
SIGNATURE = ("void rmgTerrainPainter::paintRectangle(\n"
             "    unsigned int x, unsigned int y,\n"
             "    unsigned int rectangleWidth, unsigned int rectangleHeight)")


LOOPS = ("for", "outer_while", "inner_while", "while", "outer_do", "do")


def loop_scope(variable, initial, bound, statements, style):
    condition = variable + " < " + bound
    if style == "for":
        return [f"for ({variable} = {initial}; {condition}; ++{variable}) {{",
                *("    " + line for line in statements), "}"]
    if style == "while":
        return [f"{variable} = {initial};", f"while ({condition}) {{",
                *("    " + line for line in statements), f"    ++{variable};", "}"]
    return [f"{variable} = {initial};", f"if ({condition}) {{", "    do {",
            *("        " + line for line in statements),
            f"    }} while (++{variable} < {bound});", "}"]


def bodies(loop="for"):
    if loop not in LOOPS:
        raise ValueError("review the rectangle loop form")
    for bounds, query, tile in itertools.product(
            ("in_place", "reversed_in_place", "assignment", "named", "named_const"),
            ("comparison", "predicate"),
            ("named", "const_frame", "temporary", "direct_temporary", "direct_named", "assigned")):
        end_x, end_y = "rectangleWidth", "rectangleHeight"
        if bounds == "in_place":
            setup = ["rectangleWidth += x;", "rectangleHeight += y;"]
        elif bounds == "reversed_in_place":
            setup = ["rectangleHeight += y;", "rectangleWidth += x;"]
        elif bounds == "assignment":
            setup = ["rectangleWidth = x + rectangleWidth;", "rectangleHeight = y + rectangleHeight;"]
        else:
            const = "const " if bounds == "named_const" else ""
            setup = [const + "unsigned int endX = x + rectangleWidth;",
                     const + "unsigned int endY = y + rectangleHeight;"]
            end_x, end_y = "endX", "endY"
        lines = ["    " + line for line in setup]
        condition = "getPaintTerrain() != getTerrain(point)" if query == "comparison" else "!isPaintTerrain(point)"
        lines.append("    TRmgGridPoint point;")
        visit = ["if (" + condition + ") {", "    paintPoint(point);", "} else {"]
        frame = "selectBaseFrame(point, m_paintTerrain, -1)"
        if tile not in ("direct_temporary", "direct_named"):
            visit.append("    " + ("const " if tile == "const_frame" else "") + "int frame = " + frame + ";")
            frame = "frame"
        value = "rmgTerrainTile(m_paintTerrain, " + frame + ")"
        if tile in ("temporary", "direct_temporary"):
            visit.append("    setTile(point, " + value + ");")
        else:
            if tile == "assigned":
                visit += ["    rmgTerrainTile tile;", "    tile = " + value + ";"]
            else:
                visit.append("    rmgTerrainTile tile(m_paintTerrain, " + frame + ");")
            visit.append("    setTile(point, tile);")
        visit.append("}")
        inner_style = "while" if loop in ("inner_while", "while") else "do" if loop == "do" else "for"
        outer_style = "while" if loop in ("outer_while", "while") else "do" if loop in ("outer_do", "do") else "for"
        inner = loop_scope("point.m_x", "x", end_x, visit, inner_style)
        outer = loop_scope("point.m_y", "y", end_y, inner, outer_style)
        lines += ["    " + line for line in outer]
        yield "+".join((bounds, query, tile)), SIGNATURE + "\n{\n" + "\n".join(lines) + "\n}"


def make_manifest(source):
    found = _source.find_definitions(source, FUNCTION)
    if len(found) != 1:
        raise ValueError("review the unique rectangle painter")
    item = found[0]
    start = source.rfind("\n", 0, item.head) + 1
    original = source[start:item.body_close + 1]
    options = list(bodies())
    if original not in {body for loop in LOOPS for _, body in bodies(loop)}:
        raise ValueError("review the rectangle painter before rebasing the family")
    if original not in {body for _, body in options}:
        options.append(("unchanged_control", original))
    options.sort(key=lambda row: row[1] != original)
    return dict(schema=1, unit="rmg_terrain", function=FUNCTION, evidence=__doc__, axes=[dict(
        name="rectangle_lifetimes", find=original,
        options=[dict(name=name, replace=body) for name, body in options])])


def make_loop_manifest(source, parent_labels):
    payload = make_manifest(source)
    choices = {name for name, _ in bodies()}
    if len(parent_labels) != 10 or len(set(parent_labels)) != 10 or any(name not in choices for name in parent_labels):
        raise ValueError("review ten unique successful rectangle parents")
    options = []
    for loop in LOOPS:
        candidates = dict(bodies(loop))
        options += [dict(name=parent + "+" + loop, replace=candidates[parent]) for parent in parent_labels]
    payload["axes"][0]["options"] = options
    payload["evidence"] += ("\nFollow-up: cross ten retained parents with for/while/guarded-do loop forms, "
                            "preserving unsigned end-exclusive bounds, one mutable point and y-major order. "
                            "The runner separately compiles the canonical baseline before all sixty parents.")
    return payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--loop-parents-from", type=Path, help="cross the top ten completed source parents with six loop forms")
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
    if args.loop_parents_from:
        parent = json.loads(args.loop_parents_from.read_text())
        if (parent.get("schema") != 1 or parent.get("function") != FUNCTION
                or parent.get("source_sha256") != hashlib.sha256(source.encode()).hexdigest()):
            raise ValueError("review the rectangle parents against the current source")
        rows = [row for row in parent["results"] if not row["error"] and row["score"] is not None]
        payload = make_loop_manifest(source, [row["labels"]["rectangle_lifetimes"] for row in rows[:10]])
        payload["parent_results"] = str(args.loop_parents_from.resolve())
    else:
        payload = make_manifest(source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    parsed = hypotheses.parse_manifest(args.output)
    print("generated", len(hypotheses.variants(parsed[4], parsed[5])), "unique source hypotheses ->", args.output)


if __name__ == "__main__":
    main()
