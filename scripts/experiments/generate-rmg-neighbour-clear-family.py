#!/usr/bin/env python3
"""Emit semantic source families for two retail-evidenced RMG bodies.

BuildNeighbourKinds keeps N/S/W/E/NW/NE/SW/SE query order and the one
canonical classifier (retail expands seven calls, retains the eighth).
MapItem::clear keeps the real vector erase and named packed-bit writes:
only independent snapshot/update ordering and iterator lifetimes vary.
No raw-mask overlay, forced inlining, synthetic callers or header noise.
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


def axis(name, source, original, alternatives):
    options = [dict(name="baseline", replace=original)]
    seen = {original}
    for label, replacement in alternatives:
        if replacement not in seen:
            options.append(dict(name=label, replace=replacement))
            seen.add(replacement)
    return dict(name=name, source=source, find=original, options=options)


def bounds(dimensions, shape):
    width = "m_width" if dimensions == "members" else "getWidth()"
    height = "m_height" if dimensions == "members" else "getHeight()"
    specs = [("north", "point.m_y", "> 0", "-"),
             ("south", "point.m_y", f"< {height} - 1", "+"),
             ("west", "point.m_x", "> 0", "-"),
             ("east", "point.m_x", f"< {width} - 1", "+")]
    lines = []
    for name, coordinate, condition, operation in specs:
        if shape == "ternary":
            lines.append(f"    unsigned int {name} = {coordinate} {condition} ? {coordinate} {operation} 1 : {coordinate};")
        elif shape == "branches":
            lines += [f"    unsigned int {name};", f"    if ({coordinate} {condition})",
                      f"        {name} = {coordinate} {operation} 1;", "    else",
                      f"        {name} = {coordinate};"]
        else:
            lines += [f"    unsigned int {name} = {coordinate};",
                      f"    if ({coordinate} {condition})", f"        {operation * 2}{name};"]
    return "\n".join(lines)


def visits(construction, query):
    specs = [("NORTH", "point.m_x", "north"), ("SOUTH", "point.m_x", "south"),
             ("WEST", "west", "point.m_y"), ("EAST", "east", "point.m_y"),
             ("NORTHWEST", "west", "north"), ("NORTHEAST", "east", "north"),
             ("SOUTHWEST", "west", "south"), ("SOUTHEAST", "east", "south")]
    lines = []
    if construction == "reused":
        lines.append("    TRmgGridPoint nearby;")
    for direction, x, y in specs:
        scoped = construction in ("direct", "copy_init", "const_reference")
        if scoped:
            lines.append("    {")
        indent = "        " if scoped else "    "
        point = "nearby" if scoped or construction == "reused" else "nearby" + direction.title()
        if construction == "temporary":
            expression = f"TRmgGridPoint({x}, {y})"
        else:
            expression = point
            if construction == "copy_init":
                statement = f"TRmgGridPoint {point} = TRmgGridPoint({x}, {y});"
            elif construction == "const_reference":
                statement = f"const TRmgGridPoint& {point} = TRmgGridPoint({x}, {y});"
            elif construction == "reused":
                statement = f"{point} = TRmgGridPoint({x}, {y});"
            else:
                statement = f"TRmgGridPoint {point}({x}, {y});"
            lines.append(indent + statement)
        argument = f"getTerrain({expression})"
        if query == "named":
            local = "nearbyTerrain" if scoped else "terrain" + direction.title()
            lines.append(f"{indent}int {local} = {argument};")
            argument = local
        lines += [f"{indent}neighbours[TILE_DIR_{direction}] = getRmgTerrainNeighbourKind(",
                  f"{indent}    terrain, {argument});"]
        if scoped:
            lines.append("    }")
    return "\n".join(lines)


def make_axes(terrain, rmg):
    neighbour = definition(terrain, "rmgTerrainPainter::buildNeighbourKinds")
    clear = definition(rmg, "TRmgMapItem::clear")
    bounds_prefix = neighbour[:neighbour.index("    unsigned int north")]
    separator = neighbour.index("\n\n")
    original_bounds = neighbour[:separator]
    original_visits = neighbour[separator + 2:neighbour.rindex("\n}")]
    reads = ["    TRmgConnectionDecoration connection = m_connection;",
             "    TRmgGroundTile tile = m_tile;", "    TRmgGroundTileData tileData = m_tileData;"]
    update_start = clear.index("    connection.m_present")
    update_end = clear.index("    m_connection = connection;")
    original_updates = clear[update_start:update_end].rstrip("\n")
    groups = ["\n".join(line for line in original_updates.splitlines() if line.startswith("    " + name + "."))
              for name in ("connection", "tile", "tileData")]
    if "\n".join(groups) != original_updates or any(clear.count(read) != 1 for read in reads):
        raise ValueError("clear baseline changed; review the family anchors")
    begin = clear.index("\n{\n") + 3
    original_setup = clear[begin:update_start]
    erase_position = clear.index("    m_objects.erase")
    before = sum(clear.index(read) < erase_position for read in reads)
    iterators = [
        ("expression", "    m_objects.erase(m_objects.begin(), m_objects.end());\n"),
        ("first_last", "    std::vector<type_object*>::iterator first = m_objects.begin();\n"
         "    std::vector<type_object*>::iterator last = m_objects.end();\n    m_objects.erase(first, last);\n"),
        ("last_first", "    std::vector<type_object*>::iterator last = m_objects.end();\n"
         "    std::vector<type_object*>::iterator first = m_objects.begin();\n    m_objects.erase(first, last);\n"),
    ]
    setups = []
    for order, (label, call) in itertools.product(itertools.permutations(range(3)), iterators):
        text = "".join(reads[index] + "\n" for index in order[:before]) + call
        text += "".join(reads[index] + "\n" for index in order[before:]) + "\n"
        setups.append(("".join(map(str, order)) + "+" + label, text))
    axes = [
        axis("neighbour_bounds", "src/rmg_terrain.cpp", original_bounds,
             (("+".join(form), bounds_prefix + bounds(*form)) for form in itertools.product(
                 ("members", "accessors"), ("ternary", "branches", "increment")))),
        axis("neighbour_values", "src/rmg_terrain.cpp", original_visits,
             (("+".join(form), visits(*form)) for form in itertools.product(
                 ("temporary", "direct", "copy_init", "const_reference", "reused", "distinct"),
                 ("nested", "named")))),
        axis("clear_setup", "src/rmg.cpp", original_setup, setups),
        axis("clear_updates", "src/rmg.cpp", original_updates,
             (("+".join(str(i) for i in order), "\n".join(groups[i] for i in order))
              for order in itertools.permutations(range(3)))),
    ]
    return axes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / "src/rmg_terrain.cpp").read_text(),
                     (HOMM3_DIR / "src/rmg.cpp").read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes,
                   evidence="Retail 0x5b68a0: clamped N/S/W/E coordinates; eight ordered terrain queries and classifier calls, final classifier retained. Retail 0x530f10: STL erase and three packed-field snapshots/masks, EDI connection lifetime spans the vector copy guard. Vary meaningful point/query/iterator lifetimes and independent field update order; preserve canonical helpers and packed-bit semantics.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
