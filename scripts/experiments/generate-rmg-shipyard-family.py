#!/usr/bin/env python3
"""Generate shipyard coordinate lifetimes and flood-vector construction forms.

Retail 0x541960 scans the six land cells in row-major order, searches the
four canonical side offsets and tests the opposite side. Keep these checks
and ordinary point/map helpers. Retail 0x541780 retains different public
vector insertion depths at its seed and neighbour sites.
"""
import argparse
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


def axis(name, original, alternatives):
    options = [dict(name="baseline", replace=original)]
    seen = {original}
    for label, replacement in alternatives:
        if replacement not in seen:
            options.append(dict(name=label, replace=replacement))
            seen.add(replacement)
    return dict(name=name, find=original, options=options)


def shipyard_axes(original):
    initial_end = original.index("    for (nearby.m_y =")
    initial = original[:initial_end]
    declaration = "    TRmgMapPosition nearby = position;"
    water = ("    for (waterOffset = 0; waterOffset < RMG_SHIPYARD_WATER_OFFSET_COUNT; ++waterOffset) {\n"
             "        nearby = position;\n        nearby += g_rmgShipyardWaterOffsets[waterOffset];")
    water_head = water[:water.index("        nearby =")]
    direct_water = water
    water_start = original.index(water_head)
    water = original[water_start:original.index("        if (nearby.m_x < 0", water_start)].rstrip()
    side_start = original.index("    nearby = position;\n    if (g_rmgShipyardWaterOffsets")
    side_end = original.index("    if (nearby.m_x < 0", side_start)
    side = original[side_start:side_end]
    side_head = "    nearby = position;\n"
    condition = "g_rmgShipyardWaterOffsets[waterOffset].m_x < 0"
    if initial.count(declaration) != 1 or water not in original:
        raise ValueError("review the current shipyard construction before generating")
    axes = [
        axis("shipyard_initial_position", initial, [
            ("direct_copy", initial.replace(declaration, "    TRmgMapPosition nearby(position);")),
            ("assigned", initial.replace(declaration, "    TRmgMapPosition nearby;\n    nearby = position;")),
            ("coordinates", initial.replace(declaration, "    TRmgMapPosition nearby(position.m_x, position.m_y, position.m_z);")),
            ("level_only", initial.replace(declaration, "    TRmgMapPosition nearby;\n    nearby.m_z = position.m_z;")),
        ]),
        axis("shipyard_water_translation", water, [
            ("direct_offset", direct_water),
            ("offset_copy_first", water_head + "        TPoint offset = g_rmgShipyardWaterOffsets[waterOffset];\n"
             "        nearby = position;\n        nearby += offset;"),
            ("offset_reference_first", water_head + "        const TPoint& offset = g_rmgShipyardWaterOffsets[waterOffset];\n"
             "        nearby = position;\n        nearby += offset;"),
            ("offset_copy_after", water_head + "        nearby = position;\n"
             "        TPoint offset = g_rmgShipyardWaterOffsets[waterOffset];\n        nearby += offset;"),
            ("returned_position", water_head + "        nearby = position + g_rmgShipyardWaterOffsets[waterOffset];"),
            ("reuse_level", water_head + "        nearby.m_x = position.m_x;\n"
             "        nearby.m_y = position.m_y;\n        nearby += g_rmgShipyardWaterOffsets[waterOffset];"),
        ]),
        axis("shipyard_opposite_side", side, [
            ("conditional", side_head + f"    nearby.m_x = {condition} ? position.m_x + 1 : position.m_x - 3;\n"),
            ("positive_default", side_head + "    nearby.m_x = position.m_x + 1;\n"
             "    if (g_rmgShipyardWaterOffsets[waterOffset].m_x >= 0)\n        nearby.m_x = position.m_x - 3;\n"),
            ("negative_default", side_head + "    nearby.m_x = position.m_x - 3;\n"
             f"    if ({condition})\n        nearby.m_x = position.m_x + 1;\n"),
        ]),
    ]
    types = ("unsigned char", "char", "signed char", "TTerrainType", "int")
    query_template = ("        {type} terrain = item->m_tile.m_landType;\n"
                      "        if (terrain == eTerrainWater && item->hasSubterraneanGate())")
    final_template = ("    {type} terrain = m_map.getMapItem(nearby)->m_tile.m_landType;\n"
                      "    return terrain != eTerrainWater;")
    current = [name for name in types if query_template.format(type=name) in original
               and final_template.format(type=name) in original]
    if len(current) != 1:
        raise ValueError("review the current shipyard terrain snapshots before generating")
    query = query_template.format(type=current[0])
    final = final_template.format(type=current[0])
    terrain_axis = axis("shipyard_terrain_snapshot", query,
                        [(name, query_template.format(type=name)) for name in types])
    for option in terrain_axis["options"][1:]:
        option["extra_edits"] = [dict(find=final, replace=final_template.format(type=option["name"]))]
    axes.append(terrain_axis)
    return axes


def flood_axis(original):
    end = original.index("    m_map.getMapItem(position)->")
    initial = original[:end]
    head = initial[:initial.index("    std::vector<TRmgMapPosition> openPositions;")]
    vector = "    std::vector<TRmgMapPosition> openPositions;\n"
    return axis("flood_seed_construction", initial, [
        ("insert_value", head + vector + "    openPositions.insert(openPositions.end(), position);\n"),
        ("insert_count", head + vector + "    openPositions.insert(openPositions.end(), 1, position);\n"),
        ("count_constructor", head + "    std::vector<TRmgMapPosition> openPositions(1, position);\n"),
        ("named_insert_position", head + vector + "    std::vector<TRmgMapPosition>::iterator end = openPositions.end();\n"
         "    openPositions.insert(end, position);\n"),
        ("range_constructor", head + "    std::vector<TRmgMapPosition> openPositions(&position, &position + 1);\n"),
    ])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    axes = shipyard_axes(definition(source, "type_random_map_generator::canPlaceShipyard"))
    axes.append(flood_axis(definition(source, "type_random_map_generator::floodConnectionRegion")))
    payload = dict(schema=1, source="src/rmg.cpp", units=["rmg", "rmg_support", "rmg_terrain"], axes=axes,
                   evidence="Retail 0x541960: ordered six-cell footprint, four side offsets, gate-marked water, opposite non-water tile. Vary only coordinate lifetimes, canonical point arithmetic, terrain snapshot types and equivalent opposite-side selection. A level-only position is initialized in x/y by the footprint loops before lookup. Retail 0x541780: preserve the LIFO vector and neighbour insertion, varying its real one-element initialization through public STL interfaces. No synthetic code or inline pins.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
