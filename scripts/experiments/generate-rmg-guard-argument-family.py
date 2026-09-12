#!/usr/bin/env python3
"""Test input ownership at ordinary position-addition and guard boundaries.

Monolith retail 0x542ce0 expands its translated-coordinate constructor and
retains the value-position map accessor. The candidate retains the constructor
and expands that accessor to its scalar overload. Shipyard preserves a separate
coordinate snapshot. These Complete-only helpers have no retained declarations
or Dreamcast records. Compare value and const-reference inputs consistently at
their declaration and definition; leave all call expressions, the constructor
body, implicit special members and field order intact. In particular, the two
constructor calls proven in markRiverCoastTarget must remain part of review.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    add = "TRmgMapPosition TRmgMapPosition::operator+(TPoint offset) const"
    guard = "void type_random_map_generator::placeGuard(TRmgMapPosition position, int value)"
    add_decl = "    TRmgMapPosition operator+(TPoint offset) const;"
    guard_decl = "    void placeGuard(TRmgMapPosition position, int value);"
    options = []
    for direction, position, value in itertools.product((False, True), repeat=3):
        add_new = add.replace("TPoint offset", "const TPoint& offset") if direction else add
        guard_new, decl_new = guard, guard_decl
        for old, new, changed in (("TRmgMapPosition position", "const TRmgMapPosition& position", position),
                                  ("int value", "const int& value", value)):
            if changed:
                guard_new, decl_new = guard_new.replace(old, new), decl_new.replace(old, new)
        options.append({"name": "direction_" + str(direction) + "+position_" + str(position) + "+value_" + str(value),
                        "replace": add_new, "extra_edits": [
                            {"find": guard, "replace": guard_new},
                            {"source": "include/rmg.h", "find": add_decl,
                             "replace": add_decl.replace("TPoint offset", "const TPoint& offset") if direction else add_decl},
                            {"source": "include/rmg.h", "find": guard_decl, "replace": decl_new}]})
    payload = {"schema": 1, "source": "src/rmg.cpp", "evidence": __doc__,
               "units": ["rmg", "rmg_support", "rmg_terrain", "tiles", "singleselectionpopups",
                         "singleselectionwindow", "scenarioinfo"],
               "axes": [{"name": "guard_arguments", "find": add, "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("8 ordinary position/guard input-ownership states across seven units")


if __name__ == "__main__":
    main()
