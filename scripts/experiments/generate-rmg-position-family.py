#!/usr/bin/env python3
"""Generate real three-dimensional coordinate copy/assignment/helper families.

Complete's RMG uses 12-byte coordinate values, not the terrain painter's
eight-byte grid coordinates. Retail 0x5408e0/0x541780/0x542930 preserve
coordinate snapshots and different helper expansion depths. Keep the exact
three-int constructor and existing arithmetic interfaces. Explore implicit
versus explicit copies, independent field order, returned-value lifetimes
and the source order of the three ordinary arithmetic definitions.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source
from homm3.vc6.source_families import load_manifest


def definition(source, name, *, parameters=None):
    found = _source.find_definitions(source, name)
    if parameters is not None:
        expected = " ".join(parameters.split())
        found = [item for item in found
                 if " ".join(source[item.par_open + 1:item.par_close].split()) == expected]
    if len(found) != 1:
        raise ValueError(f"expected one definition of {name}")
    item = found[0]
    return source[source.rfind("\n", 0, item.head) + 1:item.body_close + 1]


def axis(name, source, original, alternatives):
    options = [dict(name="baseline", replace=original)]
    seen = {original}
    for label, replacement in alternatives:
        if replacement not in seen:
            options.append(dict(name=label, replace=replacement))
            seen.add(replacement)
    return dict(name=name, source=source, find=original, options=options)


def copy_forms():
    head = "    TRmgMapPosition() {}\n"
    signature = "    TRmgMapPosition(const TRmgMapPosition& other)"
    forms = [("implicit", head),
             ("member_initializers", head + signature
              + "\n        : m_x(other.m_x), m_y(other.m_y), m_z(other.m_z) {}\n")]
    for order in itertools.permutations("xyz"):
        stores = "\n".join(f"        m_{field} = other.m_{field};" for field in order)
        forms.append(("stores_" + "".join(order), head + signature + "\n    {\n" + stores + "\n    }\n"))
    return forms


def assignment_forms():
    head = "    TRmgMapPosition(int newX, int newY, int newZ);\n\n"
    forms = [("implicit", head)]
    for order in itertools.permutations("xyz"):
        stores = "\n".join(f"        m_{field} = other.m_{field};" for field in order)
        forms.append(("stores_" + "".join(order), head
                      + "    TRmgMapPosition& operator=(const TRmgMapPosition& other)\n    {\n"
                      + stores + "\n        return *this;\n    }\n\n"))
    return forms


def arithmetic(construction, returned, plus_order, minus_order, definitions):
    creations = {
        "coordinates": "    TRmgMapPosition result(m_x, m_y, m_z);",
        "constructed_copy": "    TRmgMapPosition result = TRmgMapPosition(m_x, m_y, m_z);",
        "copy": "    TRmgMapPosition result = *this;",
        "direct_copy": "    TRmgMapPosition result(*this);",
        "assigned": "    TRmgMapPosition result;\n    result = *this;",
        "constructed_assignment": "    TRmgMapPosition result;\n    result = TRmgMapPosition(m_x, m_y, m_z);",
    }
    result = ("    return result += offset;" if returned == "compound"
              else "    result += offset;\n    return result;")
    bodies = {"add": "TRmgMapPosition TRmgMapPosition::operator+(TPoint offset) const\n{\n"
                      + creations[construction] + "\n" + result + "\n}"}
    for name, op, order in (("plus", "+=", plus_order), ("minus", "-=", minus_order)):
        stores = "\n".join(f"    m_{field} {op} offset.m_{field};" for field in order)
        bodies[name] = (f"TRmgMapPosition& TRmgMapPosition::operator{op}(const TPoint& offset)\n{{\n"
                        + stores + "\n    return *this;\n}")
    return "\n\n".join(bodies[name] for name in definitions)


def make_axes(header, source):
    default = header.index("    TRmgMapPosition() {}")
    coordinates = header.index("    TRmgMapPosition(int newX, int newY, int newZ);")
    comment = header.index("    // ConnectZones constructs", coordinates)
    copy = header[default:coordinates]
    assignment = header[coordinates:comment]
    copies, assignments = copy_forms(), assignment_forms()
    if copy not in dict(copies).values() or assignment not in dict(assignments).values():
        raise ValueError("review the current map-position special members before generating")
    methods = [definition(source, "TRmgMapPosition::operator" + op) for op in ("+", "+=", "-=")]
    start = min(source.index(method) for method in methods)
    end = max(source.index(method) + len(method) for method in methods)
    original = source[start:end]
    if sorted(original.split("\n\n")) != sorted(methods):
        raise ValueError("the map-position arithmetic definitions must form one contiguous group")
    alternatives = []
    for form in itertools.product(
            ("coordinates", "constructed_copy", "copy", "direct_copy", "assigned", "constructed_assignment"),
            ("named", "compound"), ("xy", "yx"), ("xy", "yx"),
            itertools.permutations(("add", "plus", "minus"))):
        construction, returned, plus_order, minus_order, definitions = form
        name = "+".join(form[:4]) + "+" + "_".join(definitions)
        alternatives.append((name, arithmetic(*form)))
    return [axis("position_copy", "include/rmg.h", copy, copies),
            axis("position_assignment", "include/rmg.h", assignment, assignments),
            axis("position_arithmetic", "src/rmg.cpp", original, alternatives)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / "include/rmg.h").read_text(), (HOMM3_DIR / "src/rmg.cpp").read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes,
                   evidence="Retail three-dword snapshots at 0x5408e0, 0x541780 and 0x542930 motivate real special-member alternatives. Keep field layout, the retained three-int constructor at 0x5355c0 and every arithmetic signature. Copy/assignment stores preserve x/y/z independently; operator+ always calls the canonical operator+=. No random includes, alternate declarations, dummy operations or inline pins. Score all functions across the three TUs for collateral changes.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
