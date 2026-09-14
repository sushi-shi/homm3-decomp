#!/usr/bin/env python3
"""Integer line-membership boundary inside the Voronoi segment predicate.

Retail addSite +0x8c:+0xc1 computes dx, dy, then
first.y*dx-first.x*dy-point.y*dx+point.x*dy and materializes equality in AL.
This is a four-product line equation, unlike the retained two-product
getRmgPointOrientation operation. The published Graphics Gems IV OnEdge/Line
code suggests separating line membership from segment-length guards; its
floating-point normalization is NOT present in Complete and is not imported.
No Dreamcast counterpart exists. Keep all three canonical squared-distance
calls and the exact shared edge-side predicate used elsewhere.

Cross five real coefficient/value lifetimes, four point-ownership signatures,
and three distance-guard forms. The new helper is ordinary file-local C++,
with no false inline qualifier, pragma, or copied orientation implementation.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg_support.cpp"
NAME = "isRmgPointOnLine"


def line_object(lifetime, ownership):
    point_type = "const TPoint&" if ownership in (1, 3) else "TPoint"
    endpoint_type = "const TPoint&" if ownership in (2, 3) else "TPoint"
    signature = endpoint_type + " first, " + endpoint_type + " second"
    dx, dy = "second.m_x - first.m_x", "second.m_y - first.m_y"
    constant = "first.m_y * m_dx - first.m_x * m_dy"
    initializer = ""
    lines = []
    if lifetime == 0:
        initializer = f"\n    : m_dx({dx}), m_dy({dy}), m_constant({constant})"
    elif lifetime in (1, 3):
        lines = ["m_dx = " + dx + ";", "m_dy = " + dy + ";"]
        if lifetime == 3:
            lines.reverse()
        lines += ["m_constant = " + constant + ";"]
    elif lifetime == 2:
        lines = ["int dx = " + dx + ";", "int dy = " + dy + ";",
                 "m_dx = dx;", "m_dy = dy;", "m_constant = first.m_y * dx - first.m_x * dy;"]
    else:
        initializer = "\n    : m_dx(" + dx + ")"
        lines = ["m_dy = " + dy + ";", "m_constant = " + constant + ";"]
    return ("namespace {\nstruct TRmgLineEquation {\n"
            "    int m_dx;\n    int m_dy;\n    int m_constant;\n"
            "    TRmgLineEquation(" + signature + ");\n"
            "    unsigned char contains(" + point_type + " point) const;\n};\n\n"
            "TRmgLineEquation::TRmgLineEquation(" + signature + ")" + initializer + "\n{\n"
            + "\n".join("    " + line for line in lines) + ("\n" if lines else "") + "}\n\n"
            "unsigned char TRmgLineEquation::contains(" + point_type + " point) const\n{\n"
            "    return m_constant - point.m_y * m_dx + point.m_x * m_dy == 0;\n}\n"
            "} // namespace")


def line_object_code(source):
    marker = "namespace {\nstruct TRmgLineEquation {"
    if marker not in source:
        return ""
    start = source.index(marker)
    end = source.index("} // namespace", start) + len("} // namespace")
    return source[start:end]


def line_predicate(lifetime, ownership):
    point_type = "const TPoint&" if ownership in (1, 3) else "TPoint"
    endpoint_type = "const TPoint&" if ownership in (2, 3) else "TPoint"
    lines = ["int dx = second.m_x - first.m_x;", "int dy = second.m_y - first.m_y;"]
    if lifetime == 1:
        lines.reverse()
    if lifetime in (0, 1):
        lines += ["int constant = first.m_y * dx - first.m_x * dy;",
                  "return constant - point.m_y * dx + point.m_x * dy == 0;"]
    elif lifetime == 2:
        lines += ["return first.m_y * dx - first.m_x * dy - point.m_y * dx + point.m_x * dy == 0;"]
    elif lifetime == 3:
        lines += ["int constant = first.m_y * dx - first.m_x * dy;",
                  "int value = constant - point.m_y * dx + point.m_x * dy;",
                  "return value == 0;"]
    else:
        lines = ["int a = second.m_y - first.m_y;", "int b = first.m_x - second.m_x;",
                 "int c = -(a * first.m_x + b * first.m_y);",
                 "return a * point.m_x + b * point.m_y + c == 0;"]
    return ("static unsigned char " + NAME + "(" + point_type + " point, "
            + endpoint_type + " first, " + endpoint_type + " second)\n{\n"
            + "\n".join("    " + line for line in lines) + "\n}")


def segment(original, guard):
    start = original.index("    return firstDistance <= edgeDistance")
    prefix = original[:start]
    call = NAME + "(point, edge->m_sitePosition, opposite)"
    if guard == 0:
        tail = "    return firstDistance <= edgeDistance && secondDistance <= edgeDistance\n        && " + call + ";\n}"
    elif guard == 1:
        tail = ("    if (firstDistance > edgeDistance)\n        return 0;\n"
                "    if (secondDistance > edgeDistance)\n        return 0;\n"
                "    return " + call + ";\n}")
    else:
        tail = ("    if (firstDistance > edgeDistance || secondDistance > edgeDistance)\n        return 0;\n"
                "    return " + call + ";\n}")
    return prefix + tail


def variants(source, use_object=False):
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, "isRmgPointOnSegment")
    template = original
    if "getRmgPointOrientation(edge->m_sitePosition, opposite, point) == 0" not in original:
        guard = "    if (firstDistance > edgeDistance || secondDistance > edgeDistance)"
        if guard not in original or "return dy * point.m_x - dx * point.m_y + c == 0;" not in original:
            raise ValueError("review changed segment predicate")
        template = (original[:original.index(guard)]
                    + "    return firstDistance <= edgeDistance && secondDistance <= edgeDistance\n"
                    + "        && getRmgPointOrientation(edge->m_sitePosition, opposite, point) == 0;\n}")
    yield dict(name="original", replace=original)
    for lifetime, ownership, guard in itertools.product(range(5), range(4), range(3)):
        body = segment(template, guard)
        if use_object:
            definition = line_object(lifetime, ownership)
            body = body.replace(NAME + "(point, edge->m_sitePosition, opposite)", "line.contains(point)")
            before = "    return firstDistance" if guard == 0 else "    return line.contains"
            body = body.replace(before, "    TRmgLineEquation line(edge->m_sitePosition, opposite);\n" + before)
        else:
            definition = line_predicate(lifetime, ownership)
        yield dict(name=f"lifetime_{lifetime}+ownership_{ownership}+guard_{guard}",
                   replace=definition + "\n\n" + body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--line-object", action="store_true",
                        help="test ordinary line construction/contains instead of a three-point helper")
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    helper = generator("generate-rmg-position-family.py")
    options = list(variants(source, args.line_object))
    args.output.write_text(json.dumps(dict(schema=1, units=["rmg_support"], evidence=__doc__, axes=[dict(
        name="voronoi_segment_boundary", source=SOURCE,
        find=helper.definition(source, "isRmgPointOnSegment"), options=options)]), indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), "line/segment states")


if __name__ == "__main__":
    main()
