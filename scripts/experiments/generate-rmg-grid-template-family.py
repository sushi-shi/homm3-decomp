#!/usr/bin/env python3
"""Two genuine coordinate ownership models; live game inputs remain untouched.

Run with run-rmg-grid-template-family.py so nominal C++ type renames are
accounted for without altering instruction bytes or the frozen retail target.
The tiny /tmp/holista-grid-template-control experiment independently reproduced
retail insert (342 bytes), comparator (32), and lower bound (89) with the
generic model. This is evidence for a hypothesis, not recovered source text.
"""
import argparse
import json
import os
import re
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    root = Path(os.environ["HOMM3_DIR"])
    header = (root / "include/rmg.h").read_text()
    source = (root / "src/rmg_terrain.cpp").read_text()
    start = header.index("struct TRmgGridPoint {")
    end = header.index("\nstruct TRmgZoneBounds {", start)
    original = header[start:end]
    class_end = original.index("\n};") + 3
    generic = original[:class_end].replace(
        "struct TRmgGridPoint {",
        "template<class Coordinate> struct TRmgCoordinatePoint {")
    generic = generic.replace("TRmgGridPoint", "TRmgCoordinatePoint")
    generic = generic.replace("unsigned int", "Coordinate")
    generic = generic.replace(
        "    VA(0x005B76B0, 0x18)",
        "    // VA instance: TRmgCoordinatePoint<unsigned int>::TRmgCoordinatePoint(const unsigned int&, const unsigned int&)\n"
        "    VA(0x005B76B0, 0x18)")
    generic += ("\n\ntypedef TRmgCoordinatePoint<unsigned int> TRmgGridPoint;\n\n"
                "template<class Coordinate>\n"
                "bool operator<(const TRmgCoordinatePoint<Coordinate>& left,\n"
                "    const TRmgCoordinatePoint<Coordinate>& right);\n")
    changes = [
        ("VA(0x004FA520, 0x16) // anchor-callee 0x4f9f77; thiscall, ret 4\n"
         "TRmgGridPoint::TRmgGridPoint(const TPoint& point)",
         "template<class Coordinate>\n"
         "// VA instance: TRmgCoordinatePoint<unsigned int>::TRmgCoordinatePoint(const TPoint&)\n"
         "VA(0x004FA520, 0x16) // anchor-callee 0x4f9f77; thiscall, ret 4\n"
         "TRmgCoordinatePoint<Coordinate>::TRmgCoordinatePoint(const TPoint& point)"),
        ("VA(0x005B8CA0, 0x20) // anchor-callee 0x5b4e96; fastcall, two point references\n"
         "bool operator<(const TRmgGridPoint& left, const TRmgGridPoint& right)",
         "template<class Coordinate>\n"
         "// VA instance: operator< <unsigned int>\n"
         "VA(0x005B8CA0, 0x20) // anchor-callee 0x5b4e96; fastcall, two point references\n"
         "bool operator<(const TRmgCoordinatePoint<Coordinate>& left,\n"
         "    const TRmgCoordinatePoint<Coordinate>& right)"),
    ]
    # Generated claims describe the canonical specialization, not its public
    # typedef. Their ordinary compiler definitions are already emitted.
    changes += [(match.group(), match.group().replace(
        ', TRmgGridPoint)', ', TRmgCoordinatePoint_unsigned_int)'))
        for match in re.finditer(r'^VA_COMPGEN\([^\n]+, TRmgGridPoint\)$', source, re.M)]
    assert all(source.count(old) == 1 for old, new in changes)
    manifest = {
        "schema": 1,
        "units": ["rmg", "rmg_support", "rmg_terrain", "scenarioinfo",
                  "singleselectionpopups", "singleselectionwindow", "tiles"],
        "evidence": (
            "Concrete unsigned point versus generic coordinate class with its "
            "ordinary function-template comparator. Retail const-reference "
            "coordinate constructor, free comparator ABI and comparator position "
            "inside the STL cluster support testing this provisional ownership. "
            "No inline/throw specification, forced instantiation or dummy call. "
            "The experimental runner audits nominal type aliases and preserves "
            "all raw bytes, relocations and original objects. Require joint "
            "retained ctor/comparator, tree EH, find/bounds and caller review."),
        "axes": [{"name": "grid_coordinate_ownership", "source": "include/rmg.h",
                  "find": original, "options": [
                      {"name": "concrete", "replace": original},
                      {"name": "generic_coordinate", "replace": generic,
                       "extra_edits": [{"source": "src/rmg_terrain.cpp",
                                        "find": old, "replace": new}
                                       for old, new in changes]},
                  ]}],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
    print(args.output)


if __name__ == "__main__":
    main()
