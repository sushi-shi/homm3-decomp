#!/usr/bin/env python3
"""Test canonical coordinate translation at treasure transfer's retained call.

Retail commitTreasureGroup 0x5469b0 retains the three-coordinate constructor
for a translated destination. Its corrected TU ownership exposes that call
at depth one (cost 47, budget 1562) and VC6 expands it. The existing ordinary
position-plus-point helper already owns this exact translated construction.
Compare calling that helper with genuine value/reference result lifetimes,
preserving coordinate snapshots, both canonical map lookups, bounds and all
live virtual queries. RMG has no Dreamcast counterpart. These are source
hypotheses, not recovered text; score every rmg.cpp claim and inspect the
named constructor call, not only the aggregate score.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    original = generator("generate-rmg-position-family.py").definition(
        source, "type_random_map_generator::commitTreasureGroup")
    old = ("            TRmgMapItem* destination = m_map.getMapItem(\n"
           "                TRmgMapPosition(point.m_x + position.m_x, point.m_y + position.m_y, position.m_z));\n")
    assert original.count(old) == 1
    lookup = "            TRmgMapItem* destination = m_map.getMapItem(destinationPosition);\n"
    forms = [("direct_constructor", old),
             ("translation_expression", "            TRmgMapItem* destination = m_map.getMapItem(position + point);\n")]
    for label, declaration in (
            ("copy_initialized", "TRmgMapPosition destinationPosition = position + point;"),
            ("direct_initialized", "TRmgMapPosition destinationPosition(position + point);"),
            ("bound_temporary", "const TRmgMapPosition& destinationPosition = position + point;"),
            ("assigned", "TRmgMapPosition destinationPosition;\n            destinationPosition = position + point;")):
        forms.append((label, "            " + declaration + "\n" + lookup))
    payload = dict(schema=1, source="src/rmg.cpp", units=["rmg"], evidence=__doc__,
                   axes=[dict(name="destination_translation", find=original,
                              options=[dict(name=label, replace=original.replace(old, body)) for label, body in forms])])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("6 canonical destination translation controls")


if __name__ == "__main__":
    main()
