#!/usr/bin/env python3
"""Test whether the three-coordinate map position extends the signed point.

Several retail RMG copies retain a two-coordinate lifetime independently of
the level (canPlaceShipyard 0x541960 and recenterZone 0x53d0d0). The current
three-direct-field model has exhausted ordinary copy/lifetime alternatives.
A TPoint base plus an int level preserves the twelve-byte non-polymorphic
layout, field access, value ABI and coordinate operations while expressing
that possible ownership boundary. No Dreamcast RMG record proves the base;
it is a retail hypothesis. Include a moved-declaration control to distinguish
the necessary header order change from inheritance itself, then test actual
base initialization versus member assignment in the retained constructor.
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
    header = (HOMM3_DIR / "include/rmg.h").read_text()
    start = header.index("struct TRmgMapPosition {")
    block = header[start:header.index("\n};", start) + 3]
    fields = """    // Before normalization: x.
    int m_x;
    // Before normalization: y.
    int m_y;
"""
    assert block.count(fields) == 1
    derived = block.replace("struct TRmgMapPosition {", "struct TRmgMapPosition : public TPoint {").replace(fields, "")
    ctor = """TRmgMapPosition::TRmgMapPosition(int newX, int newY, int newZ)
    : m_x(newX), m_y(newY), m_z(newZ)
{
}"""
    options = [{"name": "source_control", "replace": block}]
    anchor = "// The retained 0x5fdd20/0x5fdd40 bodies pass both eight-byte operands on"
    for name, declaration, replacement in (
            ("moved_direct_fields", block, ctor),
            ("point_base_initialized", derived, """TRmgMapPosition::TRmgMapPosition(int newX, int newY, int newZ)
    : TPoint(newX, newY), m_z(newZ)
{
}"""),
            ("point_base_assigned", derived, """TRmgMapPosition::TRmgMapPosition(int newX, int newY, int newZ)
    : m_z(newZ)
{
    m_x = newX;
    m_y = newY;
}""")):
        options.append({"name": name, "replace": "", "extra_edits": [
            {"insert_before": anchor, "text": declaration + "\n\n"},
            {"source": "src/rmg_support.cpp", "find": ctor, "replace": replacement}]})
    payload = {"schema": 1, "source": "include/rmg.h", "evidence": __doc__,
               "units": ["rmg", "rmg_support", "rmg_terrain", "tiles",
                         "singleselectionpopups", "singleselectionwindow", "scenarioinfo"],
               "axes": [{"name": "position_base", "find": block, "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("4 position base/declaration-order controls")


if __name__ == "__main__":
    main()
