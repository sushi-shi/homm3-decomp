#!/usr/bin/env python3
"""60 retail-grounded markRiverTargets states at 0x548c70.

Retail passes scan coordinates directly from x/y/z homes; baseline calls a
three-argument coordinate constructor and has an eight-byte frame surplus.
Its border stores also keep the map-item base separate from the field offset.
Cross five coordinate lifetimes, four actual edge-cell bindings and three
cursor/receiver forms. Preserve canonical coordinate constructors/accessors,
signed z/y/x traversal, live bounds, four cardinal calls and packed edge bits.
Complete-only: no Dreamcast counterpart is claimed.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path
import re

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::markRiverTargets"
BASELINE = """void type_random_map_generator::markRiverTargets()
{
    TRmgMapItem* item = m_map.m_mapItems;
    for (int z = 0; z < m_map.m_numberLevels; ++z) {
        for (int y = 0; y < m_map.m_mapHeight; ++y) {
            for (int x = 0; x < m_map.m_mapWidth; ++x, ++item) {
                if (item->m_tile.m_landType == eTerrainWater) {
                    for (int direction = 0; direction < 8; direction += 2)
                        markRiverCoastTarget(TRmgMapPosition(x, y, z), direction);
                }
            }
        }
    }
    for (z = 0; z < m_map.m_numberLevels; ++z) {
        for (int y = 0; y < m_map.m_mapHeight; ++y) {
            m_map.getMapItem(0, y, z)->m_tileData.m_riverTarget = 1;
            m_map.getMapItem(m_map.m_mapWidth - 1, y, z)->m_tileData.m_riverTarget = 1;
        }
        for (int x = 0; x < m_map.m_mapWidth; ++x) {
            m_map.getMapItem(x, 0, z)->m_tileData.m_riverTarget = 1;
            m_map.getMapItem(x, m_map.m_mapHeight - 1, z)->m_tileData.m_riverTarget = 1;
        }
    }
    if (m_progress)
        m_progress->advance(1000);
}"""


def helpers():
    spec = importlib.util.spec_from_file_location("river_target_helpers",
        Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def coordinate_forms():
    yield "constructor", BASELINE
    split = BASELINE.index("    for (z = 0;")
    for whole in (True, False):
        scan = BASELINE if whole else BASELINE[:split]
        scan = scan.replace("TRmgMapPosition(x, y, z)", "position")
        for component in ("x", "y", "z"):
            scan = scan.replace("for (int " + component, "for (" + component)
            scan = re.sub(r"\b" + component + r"\b", "position.m_" + component, scan)
        scan = scan.replace("    TRmgMapItem* item", "    TRmgMapPosition position;\n    TRmgMapItem* item")
        if not whole:
            scan += BASELINE[split:].replace("    for (z = 0;", "    for (int z = 0;", 1)
        yield "coordinate_loops" if whole else "scan_coordinate", scan
    loop = "                    for (int direction = 0; direction < 8; direction += 2)\n                        markRiverCoastTarget(TRmgMapPosition(x, y, z), direction);"
    values = "TRmgMapPosition position;\n{indent}position.m_x = x;\n{indent}position.m_y = y;\n{indent}position.m_z = z;\n"
    for per_direction in (False, True):
        indent = "                        " if per_direction else "                    "
        capture = indent + values.format(indent=indent)
        replacement = ("                    for (int direction = 0; direction < 8; direction += 2) {\n" + capture +
            "                        markRiverCoastTarget(position, direction);\n                    }") if per_direction else (
            capture + "                    for (int direction = 0; direction < 8; direction += 2)\n                        markRiverCoastTarget(position, direction);")
        yield "direction_coordinate" if per_direction else "water_coordinate", BASELINE.replace(loop, replacement)


def forms():
    for (coordinate, original), edge, receiver in itertools.product(coordinate_forms(), range(4), range(3)):
        body = original
        if edge:
            def bind(match):
                expression = match.group(1)
                if edge == 1:
                    return "            item = " + expression + ";\n            item->m_tileData.m_riverTarget = 1;"
                if edge == 2:
                    return "            {\n                TRmgMapItem* edgeItem = " + expression + ";\n                edgeItem->m_tileData.m_riverTarget = 1;\n            }"
                return "            {\n                TRmgMapItem& edgeItem = *" + expression + ";\n                edgeItem.m_tileData.m_riverTarget = 1;\n            }"
            body, count = re.subn(r"            (m_map\.getMapItem\([^\n]+\))->m_tileData\.m_riverTarget = 1;", bind, body)
            if count != 4:
                raise ValueError("review all four edge stores")
        if receiver == 1:
            body = body.replace("    TRmgMapItem* item = m_map.m_mapItems;", "    TRmgMapItem* item;\n    item = m_map.m_mapItems;")
        elif receiver == 2:
            body = body.replace("m_map.", "map.")
            body = body.replace("    TRmgMapItem* item", "    type_random_map& map = m_map;\n    TRmgMapItem* item", 1)
        yield coordinate + "+edge_" + str(edge) + "+receiver_" + str(receiver), body


def make_axes(source):
    original = helpers().definition(source, FUNCTION)
    alternatives = list(forms())
    if original not in {body for _, body in alternatives}:
        raise ValueError("review the current river target pass")
    result = helpers().axis("river_targets", SOURCE, original, alternatives)
    if len(result["options"]) != 60:
        raise ValueError("expected 60 distinct river target states")
    return [result]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
        axes=make_axes((HOMM3_DIR / SOURCE).read_text()), evidence=__doc__)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 river target states ->", args.output)


if __name__ == "__main__":
    main()
