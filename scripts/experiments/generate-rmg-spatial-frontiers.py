#!/usr/bin/env python3
"""Cross three independent retail-backed RMG source frontiers.

0x53b1f0 has the complete bounds CFG and differs only in the last sum's
SIB ordering; preserve output-reference order and canonical long min/max.
0x53cd30 expands its initial single-element insert into count-insert where
retail retains the wrapper; preserve that source call and vary its actual
iterator/endpoint lifetimes. 0x5443a0 agrees except for the neighbour lookup's
level/height multiply scheduling; preserve canonical getMapItem overloads.
Complete-only RMG has no Dreamcast counterpart for these three routines.

Six bounds states x six pending-endpoint states x five query states = 180.
Use three 60-state populations, keeping aggregate and specialist parents with
the existing source-family runner. No helper body is pasted into a caller,
no source qualifier or layout is changed, and no artificial work is added.
"""
import argparse
import importlib.util
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTIONS = ("type_random_map_generator::getInitialZoneBounds",
             "type_random_map_generator::drawIslandBoundary",
             "type_random_map_generator::connectJunctionEntrance")


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_spatial_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def bounds_forms():
    signature = ("void type_random_map_generator::getInitialZoneBounds(int& minimumY, int& minimumX,\n"
                 "    int& maximumY, int& maximumX) const\n{\n")
    initialize = ("    minimumY = 0;\n    minimumX = 0;\n"
                  "    maximumY = 0;\n    maximumX = 0;\n"
                  "    for (int zone = 0; zone < m_zones.size(); ++zone) {\n")
    setup = "        TRmgMapPosition position = m_zones[zone]->getLevelPosition();\n"
    size = "        int size = m_zones[zone]->m_slot->m_size;\n"
    minima = ("        minimumY = std::_cpp_min<long>(minimumY, position.m_y - size);\n"
              "        minimumX = std::_cpp_min<long>(minimumX, position.m_x - size);\n")
    maxima = ("        maximumY = std::_cpp_max<long>(maximumY, position.m_y + size + 1);\n"
              "        maximumX = std::_cpp_max<long>(maximumX, position.m_x + size + 1);\n")
    forms = (
        ("copy_initialized", setup + size, maxima),
        ("direct_copy", "        TRmgMapPosition position(m_zones[zone]->getLevelPosition());\n" + size, maxima),
        ("assigned", "        TRmgMapPosition position;\n"
                     "        position = m_zones[zone]->getLevelPosition();\n" + size, maxima),
        ("bound_temporary", "        const TRmgMapPosition& position = m_zones[zone]->getLevelPosition();\n" + size, maxima),
        ("named_zone", "        TRmgZone* current = m_zones[zone];\n"
                       "        TRmgMapPosition position = current->getLevelPosition();\n"
                       "        int size = current->m_slot->m_size;\n", maxima),
        ("named_extents", setup + size,
         "        int upperY = position.m_y + size + 1;\n"
         "        maximumY = std::_cpp_max<long>(maximumY, upperY);\n"
         "        int upperX = position.m_x + size + 1;\n"
         "        maximumX = std::_cpp_max<long>(maximumX, upperX);\n"),
    )
    for label, construction, upper in forms:
        yield label, signature + initialize + construction + minima + upper + "    }\n}"


def pending_forms():
    declare = "    std::vector<TPoint> pending;\n"
    yield "direct", declare + "    pending.insert(pending.end(), to);\n"
    yield "named_iterator", declare + ("    std::vector<TPoint>::iterator end = pending.end();\n"
                                       "    pending.insert(end, to);\n")
    for label, binding in (
            ("endpoint_reference", "const TPoint& endpoint = to;"),
            ("endpoint_direct_copy", "TPoint endpoint(to);"),
            ("endpoint_copy_initialized", "TPoint endpoint = to;"),
            ("endpoint_assigned", "TPoint endpoint;\n    endpoint = to;")):
        yield label, declare + "    " + binding + "\n    pending.insert(pending.end(), endpoint);\n"


def query_forms():
    indent = "                        "
    yield "scalar_coordinates", indent + "TRmgMapItem* nearby = m_map.getMapItem(column, row, position.m_z);"
    for label, construction in (
            ("direct_position", "TRmgMapPosition adjacent(column, row, position.m_z);"),
            ("copy_initialized_position", "TRmgMapPosition adjacent = TRmgMapPosition(column, row, position.m_z);"),
            ("bound_position", "const TRmgMapPosition& adjacent = TRmgMapPosition(column, row, position.m_z);")):
        yield label, indent + construction + "\n" + indent + "TRmgMapItem* nearby = m_map.getMapItem(adjacent);"
    yield "named_level", (indent + "int adjacentLevel = position.m_z;\n" + indent
                          + "TRmgMapItem* nearby = m_map.getMapItem(column, row, adjacentLevel);")


def current_fragment(text, forms, label):
    matches = [fragment for _, fragment in forms if text.count(fragment) == 1]
    if len(matches) != 1:
        raise ValueError("review the current " + label + " before rebasing")
    return matches[0]


def make_axes(source):
    helper = helpers()
    bounds, island, junction = [helper.definition(source, name) for name in FUNCTIONS]
    bounds_options = list(bounds_forms())
    if bounds not in dict(bounds_options).values():
        raise ValueError("review the current zone-bounds operations before rebasing")
    pending_options, query_options = list(pending_forms()), list(query_forms())
    pending = current_fragment(island, pending_options, "pending insertion")
    query = current_fragment(junction, query_options, "junction query")
    return [helper.axis("zone_bounds_lifetime", SOURCE, bounds, bounds_options),
            helper.axis("island_endpoint_lifetime", SOURCE, island,
                        [(label, island.replace(pending, fragment)) for label, fragment in pending_options]),
            helper.axis("junction_query_lifetime", SOURCE, junction,
                        [(label, junction.replace(query, fragment)) for label, fragment in query_options])]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                   axes=make_axes((HOMM3_DIR / SOURCE).read_text()), evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 180 spatial states (three populations of 60) ->", args.output)


if __name__ == "__main__":
    main()
