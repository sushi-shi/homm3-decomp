#!/usr/bin/env python3
"""Check the shipyard table's recovered source position against its users.

Retail emits the 0x541910 table initializer between floodConnectionRegion
and canPlaceShipyard, its first table user. The authored definition currently
sits with the direction table at the file's beginning. Reopen the same unnamed
namespace at that first use, retaining its initializer, ownership and values.
The independent unchanged-source control measures whether that supported
source-order recovery changes the inlining or register state of any sibling.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    start = source.index("// Shipyards are three tiles wide.")
    end = source.index("\n};", start) + 4
    block = source[start:end]
    if start > source.index("void type_random_map_generator::floodConnectionRegion("):
        whole = "namespace {\n\n" + block + "\n} // namespace\n"
        payload = {"schema": 1, "source": "src/rmg.cpp", "units": ["rmg"],
                   "evidence": __doc__, "axes": [{"name": "shipyard_table_position", "find": whole,
                    "options": [{"name": "before_first_user", "replace": whole},
                                {"name": "file_start", "replace": "",
                                 "extra_edits": [{"insert_before": "// Before normalization: gLandRiverDeltaIndex.",
                                                   "text": block + "\n"}]}]}]}
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(payload, indent=2) + "\n")
        load_manifest(args.output, HOMM3_DIR)
        print("2 shipyard-table source-order controls (adopted baseline)")
        return
    payload = {"schema": 1, "source": "src/rmg.cpp", "units": ["rmg"],
               "evidence": __doc__, "axes": [{"name": "shipyard_table_position", "find": block,
                "options": [{"name": "file_start", "replace": block},
                            {"name": "before_first_user", "replace": "",
                             "extra_edits": [{"insert_before": "// Retail first checks the six land cells at x-2..x, y..y+1, then searches",
                                               "text": "namespace {\n\n" + block + "\n} // namespace\n\n"}]}]}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("2 shipyard-table source-order controls")


if __name__ == "__main__":
    main()
