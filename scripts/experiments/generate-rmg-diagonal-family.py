#!/usr/bin/env python3
"""Generate clamped-point/helper lifetime alternatives, scoring every RMG TU.

The canonical signed offset tables and query order come from 0x5b6ba0 and
0x5b6e00. Reference-return clamp alternatives preserve the address selection
seen at all six retail sites. No artificial destructor or inline qualifier.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source
from homm3.vc6.source_families import load_manifest


def definition(source, name):
    found = _source.find_definitions(source, name)
    if len(found) != 1:
        raise ValueError(f"expected one definition of {name}")
    item = found[0]
    start = source.rfind("\n", 0, item.head) + 1
    return source[start:item.body_close + 1]


def point_creation(name, x, y, form):
    if form == "reference":
        return [f"    const TRmgGridPoint& {name} = TRmgGridPoint({x}, {y});"]
    if form == "copy_init":
        return [f"    TRmgGridPoint {name} = TRmgGridPoint({x}, {y});"]
    if form == "staged":
        return [f"    TRmgGridPoint {name};", f"    {name}.m_x = {x};", f"    {name}.m_y = {y};"]
    return [f"    TRmgGridPoint {name}({x}, {y});"]


def body(original, which, construction, second, dimensions):
    prefix = original[:original.index("    int terrain =")]
    width = "m_width" if dimensions == "members" else "getWidth()"
    height = "m_height" if dimensions == "members" else "getHeight()"

    def clamp(coordinate, offset, size):
        return (f"clampRmgTerrainCoordinate(static_cast<int>(point.m_{coordinate}) + {offset}, "
                f"0, static_cast<int>({size}) - 1)")

    lines = ["    int terrain = getTerrain(point);"]
    if which == "First":
        lines.append("    const TPoint* pair = offsets[(flip.m_flipY << 1) | flip.m_flipX];")
        x, y = clamp("x", "pair[0].m_x", width), clamp("y", "pair[0].m_y", height)
        next_x, next_y = clamp("x", "pair[1].m_x", width), clamp("y", "pair[1].m_y", height)
        comparison = "=="
    else:
        lines.append("    const TPoint& offset = offsets[(flip.m_flipY << 1) | flip.m_flipX];")
        x, y = clamp("x", "offset.m_x", width), "point.m_y"
        next_x, next_y = "point.m_x", clamp("y", "offset.m_y", height)
        comparison = "!="
    lines += point_creation("nearby", x, y, construction)
    lines += [f"    if (getTerrain(nearby) {comparison} terrain)", "        return 1;"]
    if second == "reuse":
        if construction == "reference":
            raise ValueError("cannot mutate a reference-bound const point")
        lines += [f"    nearby.m_x = {next_x};", f"    nearby.m_y = {next_y};"]
        result = "nearby"
    else:
        lines += point_creation("nextPoint", next_x, next_y, second)
        result = "nextPoint"
    return prefix + "\n".join(lines + [f"    return getTerrain({result}) {comparison} terrain;", "}"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    grid_path = Path(__file__).with_name("rmg-grid-source-family.json")
    grid, _, _ = load_manifest(grid_path, HOMM3_DIR)
    # Each inherited axis keeps its own source; the diagonal axes edit a TU.
    axes = [dict(item, source=item.get("source", grid["source"])) for item in grid["axes"]]
    source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
    for which in ("First", "Second"):
        original = definition(source, "rmgTerrainPainter::check" + which + "Diagonal")
        options = [dict(name="baseline", replace=original)]
        seen = {original}
        for form in itertools.product(("direct", "copy_init", "reference", "staged"),
                                      ("reuse", "direct", "staged", "reference"),
                                      ("members", "accessors")):
            if form[:2] == ("reference", "reuse"):
                continue
            replacement = body(original, which, *form)
            if replacement not in seen:
                options.append(dict(name="+".join(form), replace=replacement))
                seen.add(replacement)
        axes.append(dict(name=which.lower() + "_diagonal", source="src/rmg_terrain.cpp",
                         find=original, options=options))
    clamp = definition(source, "clampRmgTerrainCoordinate")
    head = clamp[:clamp.index("{\n") + 2]
    clamp_forms = [
        ("conditional", head + "    return value < minimum ? minimum : value > maximum ? maximum : value;\n}"),
        ("early_returns", head + "    if (value < minimum)\n        return minimum;\n"
         "    if (value > maximum)\n        return maximum;\n    return value;\n}"),
        ("nested", head + "    if (value >= minimum) {\n"
         "        if (value <= maximum)\n            return value;\n"
         "        return maximum;\n    }\n    return minimum;\n}"),
    ]
    options = [dict(name="baseline", replace=clamp)]
    seen = {clamp}
    for name, replacement in clamp_forms:
        if replacement not in seen:
            options.append(dict(name=name, replace=replacement))
            seen.add(replacement)
    axes.append(dict(name="clamp_control_flow", source="src/rmg_terrain.cpp",
                     find=clamp, options=options))
    payload = dict(schema=1, units=grid["units"], axes=axes,
                   evidence="Retail diagonal tables, six signed reference-select clamps, ordered cache queries and early return predicates stay fixed. Vary point construction/copy/reference, reuse vs separate value lifetimes, actual dimension accessor calls and reference-return helper control flow. Combine with the prior coordinate constructor/copy/assignment/addition axes and score every tracked function in all three TUs.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
