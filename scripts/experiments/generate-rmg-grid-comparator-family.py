#!/usr/bin/env python3
"""Test a function-template comparison against a fixed-type inline overload.

Retail's unsigned coordinate-reference constructor (0x5b76b0) and comparator
among the tree instantiations (0x5b8ca0) support a coordinate-template model.
The template comparator preserves insert's _Lockit unwind scope (0x5b7cd0).
The negative control keeps the same coordinate specialization and comparison
expression, but exposes a fixed-type inline overload. No include or caller
body changes participate. The Dreamcast build has no RMG counterpart.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families


TEMPLATE = """template<class Coordinate>
bool operator<(const TRmgGridPointT<Coordinate>& left, const TRmgGridPointT<Coordinate>& right)
{
    return left.m_y < right.m_y || (left.m_y == right.m_y && left.m_x < right.m_x);
}"""
ORDINARY = TEMPLATE.replace("template<class Coordinate>\n", "inline ").replace(
    "TRmgGridPointT<Coordinate>", "TRmgGridPoint")
CLAIM = """template<class Coordinate>
VA(0x005B8CA0, 0x20) // anchor-callee 0x5b4e96; fastcall, two point references
bool operator<(const TRmgGridPointT<Coordinate>& left,
               const TRmgGridPointT<Coordinate>& right);"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = {
        "schema": 1,
        "units": ["rmg", "rmg_support", "rmg_terrain", "tiles",
                  "singleselectionpopups", "singleselectionwindow", "scenarioinfo"],
        "evidence": __doc__,
        "axes": [{
            "name": "comparison_definition",
            "source": "include/rmg.h",
            "find": TEMPLATE,
            "options": [
                {"name": "coordinate_template"},
                {"name": "fixed_type_inline", "replace": ORDINARY,
                 "extra_edits": [{
                     "source": "src/rmg_terrain.cpp", "find": CLAIM,
                     "replace": CLAIM.replace("template<class Coordinate>\n", "").replace(
                         "TRmgGridPointT<Coordinate>", "TRmgGridPoint"),
                 }]},
            ],
        }],
    }
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("2 coordinate-comparator states")


if __name__ == "__main__":
    main()
