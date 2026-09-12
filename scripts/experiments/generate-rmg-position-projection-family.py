#!/usr/bin/env python3
"""Distinguish a position conversion from base-point copying at entrance stores.

Retail createMonolithConnection 0x542ce0 copies the object's x then y into
a two-coordinate stack temporary before each entrance-vector insertion.
The authored TPoint(x,y) evaluates the y argument first. A separate base-point
experiment preserves the retained position constructor and recovers an
unrelated factory, but that incidental gain does not prove inheritance.
Compare a canonical TPoint(TRmgMapPosition const&) conversion with a TPoint
base and the actual one-argument copy expressions at these four sites.
Include model-only controls and ordinary named two-scalar temporaries.
Keep all allocations, insertion APIs, guard helpers and value-return ABIs.
No Dreamcast RMG records prove either relationship.
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
    header = (HOMM3_DIR / "include/rmg.h").read_text()
    start = source.index("void type_random_map_generator::createMonolithConnection(")
    current = source[start:source.index("\n}", start) + 2]
    original = current
    copy = "TPoint(object->m_position.m_x, object->m_position.m_y)"
    for spaces in (8, 12):
        indent = " " * spaces
        for zone in ("source", "destination"):
            block = (indent + "TPoint entrance;\n" + indent + "entrance.m_x = object->m_position.m_x;\n"
                     + indent + "entrance.m_y = object->m_position.m_y;\n"
                     + indent + zone + "->m_entrances.push_back(entrance);")
            original = original.replace(block, indent + zone + "->m_entrances.push_back(" + copy + ");")
    assert original.count(copy) == 4
    options = [{"name": "source_control", "replace": current}]
    if original != current:
        options.append({"name": "two_scalar_control", "replace": original})
    start = header.index("struct TRmgMapPosition {")
    position = header[start:header.index("\n};", start) + 3]
    fields = """    // Before normalization: x.
    int m_x;
    // Before normalization: y.
    int m_y;
"""
    derived = position.replace("struct TRmgMapPosition {", "struct TRmgMapPosition : public TPoint {").replace(fields, "")
    ctor = """TRmgMapPosition::TRmgMapPosition(int newX, int newY, int newZ)
    : m_x(newX), m_y(newY), m_z(newZ)
{
}"""
    base_edits = [
        {"source": "include/rmg.h", "find": position, "replace": ""},
        {"source": "include/rmg.h", "insert_before": "// The retained 0x5fdd20/0x5fdd40 bodies pass both eight-byte operands on",
         "text": derived + "\n\n"},
        {"source": "src/rmg_support.cpp", "find": ctor,
         "replace": ctor.replace(": m_x(newX), m_y(newY), m_z(newZ)", ": TPoint(newX, newY), m_z(newZ)")}]
    conversion_edits = [{"source": "include/rmg.h", "insert_after": "    TPoint(int newX, int newY) : m_x(newX), m_y(newY) {}",
                         "text": "\n    TPoint(const TRmgMapPosition& position)\n        : m_x(position.m_x), m_y(position.m_y) {}"}]
    for model, edits in (("conversion", conversion_edits), ("point_base", base_edits)):
        for form in ("model_only", "temporary_copy", "named_copy", "assigned_copy"):
            body = original
            if form == "temporary_copy":
                body = body.replace(copy, "TPoint(object->m_position)")
            elif form in ("named_copy", "assigned_copy"):
                lines = []
                for line in body.splitlines():
                    if copy in line:
                        indent = line[:len(line) - len(line.lstrip())]
                        if form == "named_copy":
                            lines.append(indent + "TPoint entrance = object->m_position;")
                        else:
                            lines += [indent + "TPoint entrance;", indent + "entrance = object->m_position;"]
                        line = line.replace(copy, "entrance")
                    lines.append(line)
                body = "\n".join(lines)
            options.append({"name": model + "+" + form, "replace": body, "extra_edits": edits})
    for assigned in (False, True):
        lines = []
        for line in original.splitlines():
            if copy in line:
                indent = line[:len(line) - len(line.lstrip())]
                if assigned:
                    lines += [indent + "TPoint entrance;", indent + "entrance.m_x = object->m_position.m_x;",
                              indent + "entrance.m_y = object->m_position.m_y;"]
                else:
                    lines.append(indent + "TPoint entrance(object->m_position.m_x, object->m_position.m_y);")
                line = line.replace(copy, "entrance")
            lines.append(line)
        replacement = "\n".join(lines)
        if replacement != current:
            options.append({"name": "direct_fields+" + ("assigned" if assigned else "constructed"), "replace": replacement})
    payload = {"schema": 1, "source": "src/rmg.cpp", "evidence": __doc__,
               "units": ["rmg", "rmg_support", "rmg_terrain", "tiles",
                         "singleselectionpopups", "singleselectionwindow", "scenarioinfo"],
               "axes": [{"name": "position_projection", "find": current, "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(str(len(options)) + " position-projection ownership and caller controls")


if __name__ == "__main__":
    main()
