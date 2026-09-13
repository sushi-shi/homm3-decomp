#!/usr/bin/env python3
"""Test shipyard query-level ownership through the canonical map accessors.

Retail 0x541960 uses the input level directly in the side-water scan, with no
copy to the nearby coordinate's stack slot. Its opposite-side branch computes
x+1 first and overrides it with x-3. The earlier coordinate family retained
the value-position lookup at every site. Cross actual scalar/value lookup
choices at the footprint, water and opposite-side sites with real coordinate
initializations and the existing equivalent opposite-side selection forms.
All loops, probes, terrain/flag checks and canonical helper bodies stay fixed.
There is no Dreamcast counterpart. Sixty alternatives plus the current source
control test the new lookup dimension without padding the family.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def forms(original):
    first = original.index("    int waterOffset;")
    last = original.index("    if (waterOffset == RMG_SHIPYARD_WATER_OFFSET_COUNT)")
    chunks = (original[:first], original[first:last], original[last:])
    # These five forms distinguish the directly observed water-level read
    # from the two other uses, rather than changing the accessor's body.
    queries = ((False, True, False), (True, True, False), (False, True, True),
               (True, True, True), (True, False, True))
    initial = "    TRmgMapPosition nearby = position;"
    side = """    if (g_rmgShipyardWaterOffsets[waterOffset].m_x < 0)
        ++nearby.m_x;
    else
        nearby.m_x -= 3;"""
    for sites, coordinate, opposite in itertools.product(queries,
            ("copy", "assigned", "level_only", "constructed"),
            ("branches", "positive_default", "conditional")):
        body = "".join(chunk.replace("m_map.getMapItem(nearby)",
            "m_map.getMapItem(nearby.m_x, nearby.m_y, position.m_z)") if scalar else chunk
            for chunk, scalar in zip(chunks, sites))
        if coordinate == "assigned":
            body = body.replace(initial, "    TRmgMapPosition nearby;\n    nearby = position;")
        elif coordinate == "level_only":
            body = body.replace(initial, "    TRmgMapPosition nearby;\n    nearby.m_z = position.m_z;")
        elif coordinate == "constructed":
            body = body.replace(initial, "    TRmgMapPosition nearby(position.m_x, position.m_y, position.m_z);")
        if opposite == "positive_default":
            body = body.replace(side, "    nearby.m_x = position.m_x + 1;\n    if (g_rmgShipyardWaterOffsets[waterOffset].m_x >= 0)\n        nearby.m_x = position.m_x - 3;")
        elif opposite == "conditional":
            body = body.replace(side, "    nearby.m_x = g_rmgShipyardWaterOffsets[waterOffset].m_x < 0\n        ? position.m_x + 1 : position.m_x - 3;")
        yield "queries_" + "".join("s" if s else "v" for s in sites) + "+" + coordinate + "+" + opposite, body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    start = source.index("unsigned char type_random_map_generator::canPlaceShipyard(")
    original = source[start:source.index("\n}", start) + 2]
    options = [{"name": "source_control", "replace": original}]
    options += [{"name": name, "replace": body} for name, body in forms(original)]
    assert len({option["replace"] for option in options}) == 61
    payload = {"schema": 1, "source": "src/rmg.cpp", "units": ["rmg"], "evidence": __doc__,
               "axes": [{"name": "shipyard_query_levels", "find": original, "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("60 shipyard query-level alternatives plus the source control")


if __name__ == "__main__":
    main()
