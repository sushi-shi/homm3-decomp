#!/usr/bin/env python3
"""60 retail-based states for markRiverObjectTargets (0x5497a0).

All 19 blocks, twelve branches and the progress call agree. The first
divergence is the final cell-address expression: retail loads the cell base
earlier and keeps the tile-data displacement in the store. Cross five public
accessor/result forms, four actual offset values and three receiver bindings.
Keep canonical point types, unsigned half-size arithmetic and the river bit.
Complete-only: no Dreamcast counterpart is claimed.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::markRiverObjectTargets"
BASELINE = """void type_random_map_generator::markRiverObjectTargets()
{
    for (unsigned int index = 0; index < m_positions.size(); ++index) {
        type_object* object = m_positions[index];
        TObjectType* prototype = object->m_properties->m_prototype;
        if (prototype->m_objectType == TERRAIN_MOUNTAIN
            || prototype->m_objectType == TERRAIN_LAKE
            || (prototype->m_objectType == MINE && prototype->m_subtype == GEMS)) {
            TRmgMapPosition position = object->m_position;
            int offsetX;
            int offsetY;
            if (prototype->m_hasTrigger) {
                offsetX = prototype->m_triggerCell.m_x;
                offsetY = prototype->m_triggerCell.m_y;
            } else {
                offsetX = static_cast<unsigned int>(prototype->m_imageInfo.m_objectSize.m_x) / 2;
                offsetY = static_cast<unsigned int>(prototype->m_imageInfo.m_objectSize.m_y) / 2;
            }
            position.m_x -= offsetX;
            position.m_y -= offsetY;
            if (position.m_x >= 0 && position.m_x < m_map.m_mapWidth
                && position.m_y >= 0 && position.m_y < m_map.m_mapHeight)
                m_map.getMapItem(position.m_x, position.m_y, position.m_z)->m_tileData.m_hasRiver = 1;
        }
    }
    if (m_progress)
        m_progress->advance(1000);
}"""


def helpers():
    spec = importlib.util.spec_from_file_location("river_object_target_helpers",
        Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def forms():
    getter = "m_map.getMapItem(position.m_x, position.m_y, position.m_z)"
    store = "                " + getter + "->m_tileData.m_hasRiver = 1;"
    stores = [store,
        "            {\n                TRmgMapItem* item = " + getter + ";\n                item->m_tileData.m_hasRiver = 1;\n            }",
        "            {\n                TRmgMapItem& item = *" + getter + ";\n                item.m_tileData.m_hasRiver = 1;\n            }",
        "            {\n                TRmgGroundTileData& tileData = " + getter + "->m_tileData;\n                tileData.m_hasRiver = 1;\n            }",
        store.replace(getter, "m_map.getMapItem(position)")]
    for write, offset, binding in itertools.product(range(5), range(4), range(3)):
        body = BASELINE.replace(store, stores[write])
        if offset:
            declaration = "TPoint" if offset == 3 else "TObjectType::TPoint"
            body = body.replace("            int offsetX;\n            int offsetY;", "            " + declaration + " offset;")
            body = body.replace("offsetX", "offset.m_x").replace("offsetY", "offset.m_y")
            if offset == 2:
                body = body.replace("                offset.m_x = prototype->m_triggerCell.m_x;\n                offset.m_y = prototype->m_triggerCell.m_y;",
                    "                offset = prototype->m_triggerCell;")
        if binding:
            body = body.replace("TObjectType* prototype", "const TObjectType* prototype")
            if binding == 2:
                body = body.replace("type_object* object", "const type_object* object")
        yield "write_" + str(write) + "+offset_" + str(offset) + "+binding_" + str(binding), body


def make_axes(source):
    original = helpers().definition(source, FUNCTION)
    alternatives = list(forms())
    if original not in {body for _, body in alternatives}:
        raise ValueError("review the current river object-target pass")
    result = helpers().axis("river_object_targets", SOURCE, original, alternatives)
    if len(result["options"]) != 60:
        raise ValueError("expected 60 distinct object-target states")
    return [result]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
        axes=make_axes((HOMM3_DIR / SOURCE).read_text()), evidence=__doc__)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 river object-target states ->", args.output)


if __name__ == "__main__":
    main()
