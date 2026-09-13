#!/usr/bin/env python3
"""Test one canonical point-inset helper shared by the initial and loop point.

Retail 0x53d1c0 repeats the displacement/length/clamp/translation operation at
+0x47..+0xc6 and +0xed..+0x16d, with only length retained in each copy.
The sixty caller-lifetime states do not recover the counter/length register
roles. Test whether the repeated operation belongs to an ordinary expanded
helper, keeping the retained vector operators and final boundary/fill calls.
The helper name and parameter ownership are hypotheses; no DC RMG compiland
exists. Four real input/output ownership forms, three clamp-result lifetimes
and five caller lifetimes produce sixty alternatives plus the source control.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

HELPER = "insetRmgBoundaryPoint"


def forms():
    for ownership, clamp, caller in itertools.product(
            ("inplace", "value_position", "value_point", "reference_point"),
            ("sequential_long", "nested_long", "expression"),
            ("shared", "unsigned", "range_reference", "assigned_point", "shared_previous")):
        result = "void" if ownership == "inplace" else "TPoint"
        point_type = "TPoint&" if ownership == "inplace" else "TPoint"
        center_type = "const TRmgMapPosition&"
        if ownership == "value_point":
            center_type = "TPoint"
        elif ownership == "reference_point":
            point_type, center_type = "const TPoint&", "const TPoint&"
        param = "input" if ownership == "reference_point" else "point"
        helper = "static " + result + " " + HELPER + "(" + point_type + " " + param + ", " + center_type + " center)\n{\n"
        if ownership == "reference_point":
            helper += "    TPoint point = input;\n"
        helper += "    TRmgVector delta(center.m_x - point.m_x, center.m_y - point.m_y);\n    int length = delta.length();\n    if (length > 0) {\n"
        nested = "std::_cpp_min<long>(std::_cpp_max<long>(4, length / 4), length / 2)"
        if clamp == "sequential_long":
            helper += "        long displacement = std::_cpp_max<long>(4, length / 4);\n        displacement = std::_cpp_min<long>(displacement, length / 2);\n        delta = delta * displacement / length;\n"
        elif clamp == "nested_long":
            helper += "        long displacement = " + nested + ";\n        delta = delta * displacement / length;\n"
        else:
            helper += "        delta = delta * " + nested + " / length;\n"
        helper += "        point += delta;\n    }\n"
        if result != "void":
            helper += "    return point;\n"
        helper += "}"
        center = "center" if ownership in ("inplace", "value_position") else "TPoint(center.m_x, center.m_y)"
        initial = "zone->m_boundary[0]"
        current = "zone->m_boundary[count]"
        body = "void type_random_map_generator::insetIslandZone(TRmgZone* zone)\n{\n    int zoneIndex = zone->m_slot->m_zoneIndex;\n    TRmgMapPosition center = zone->getLevelPosition();\n"
        if caller == "range_reference":
            body += "    std::vector<TPoint>& boundary = zone->m_boundary;\n"
            initial, current = "boundary[0]", "boundary[count]"
        body += "    " + ("unsigned int" if caller == "unsigned" else "int") + " count = " + ("boundary" if caller == "range_reference" else "zone->m_boundary") + ".size();\n"
        initial_value = initial if result == "void" else HELPER + "(" + initial + ", " + center + ")"
        if caller == "assigned_point":
            body += "    TPoint point;\n    point = " + initial_value + ";\n"
        else:
            body += "    TPoint point = " + initial_value + ";\n"
        if result == "void":
            body += "    " + HELPER + "(point, " + center + ");\n"
        if caller == "shared_previous":
            body += "    TPoint previous;\n"
        body += "    while (count--) {\n        " + ("previous" if caller == "shared_previous" else "TPoint previous") + " = point;\n"
        if result == "void":
            body += "        point = " + current + ";\n        " + HELPER + "(point, " + center + ");\n"
        else:
            body += "        point = " + HELPER + "(" + current + ", " + center + ");\n"
        body += "        drawIslandBoundary(point, previous, zoneIndex, center.m_z, zone->m_boundaryRoughness / 2);\n    }\n    fillIslandInterior(zone);\n}"
        yield ownership + "+" + clamp + "+" + caller, helper, body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    start = source.index("void type_random_map_generator::insetIslandZone(")
    original = source[start:source.index("\n}", start) + 2]
    options = [{"name": "source_control", "replace": original}]
    for name, helper, body in forms():
        options.append({"name": name, "replace": body,
                        "extra_edits": [{"insert_before": "VA(0x0053D1C0, 0x1B9)", "text": helper + "\n\n"}]})
    assert len(options) == 61
    payload = {"schema": 1, "source": "src/rmg.cpp", "units": ["rmg"],
               "evidence": __doc__, "axes": [{"name": "inset_helper", "find": original, "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("60 canonical inset-helper alternatives plus the source control")


if __name__ == "__main__":
    main()
