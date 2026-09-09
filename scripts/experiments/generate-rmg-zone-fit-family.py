#!/usr/bin/env python3
"""Zone separation predicate's signed locals and returned positions.

Retail 0x53ad60 has one additional register move in the signed subtraction/
square block. The frame, branches, sqrt/_ftol calls and remaining instructions
agree. Existing all-int arithmetic/declaration families did not recover it.
Test signed int/long differences and distance-result lifetimes together with
actual returned-position copies. Both int and long are signed 32-bit in VC6.
Keep every shared declaration, canonical getter and integer-before-double sum.
No Dreamcast counterpart was found. No padding, helper or inlining pin is added.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
NAME = "type_random_map_generator::canPlaceZone"


def variants(source):
    original = generator("generate-rmg-position-family.py").definition(source, NAME)
    copy = "        TRmgMapPosition otherPosition = otherZone->getLevelPosition();"
    copies = [copy,
        "        TRmgMapPosition otherPosition;\n        otherPosition = otherZone->getLevelPosition();",
        "        const TRmgMapPosition& otherPosition = otherZone->getLevelPosition();",
        "        const TRmgMapPosition otherPosition = otherZone->getLevelPosition();",
        "        otherPosition = otherZone->getLevelPosition();"]
    result = "        int distance = static_cast<int>(sqrt(static_cast<double>(dx * dx + dy * dy)));"
    results = [result,
        "        long distance = static_cast<long>(sqrt(static_cast<double>(dx * dx + dy * dy)));",
        "        double realDistance = sqrt(static_cast<double>(dx * dx + dy * dy));\n        int distance = static_cast<int>(realDistance);"]
    if original.count(copy) != 1 or original.count(result) != 1:
        raise ValueError("review changed zone-fit arithmetic")
    for copy_form, pair, result_form in itertools.product(range(5), range(4), range(3)):
        body = original.replace(copy, copies[copy_form]).replace(result, results[result_form])
        if copy_form == 4:
            body = body.replace("    for (int other = 0;", "    TRmgMapPosition otherPosition;\n    for (int other = 0;")
        if pair & 1:
            body = body.replace("        int dx =", "        long dx =")
        if pair & 2:
            body = body.replace("        int dy =", "        long dy =")
        yield dict(name="copy_%d+signed_types_%d+result_%d" % (copy_form, pair, result_form), replace=body)


def displacement_frontier(source):
    original = next(variants(source))["replace"]
    start = original.index("        int dy =")
    end = original.index("        if (10 * distance", start)
    yield dict(name="unchanged", replace=original)
    seen = {original}
    for typename, construction, squares, order in itertools.product(("TPoint", "TRmgVector"), range(4), range(3), ("yx", "xy")):
        lines = []
        if construction == 0:
            lines.append(typename + " difference(otherPosition.m_x, otherPosition.m_y);")
            lines += ["difference.m_" + field + " -= position.m_" + field + ";" for field in order]
        elif construction == 1:
            lines.append(typename + " difference;")
            lines += ["difference.m_" + field + " = otherPosition.m_" + field + " - position.m_" + field + ";" for field in order]
        elif construction == 2:
            lines.append(typename + " difference(otherPosition.m_x - position.m_x, otherPosition.m_y - position.m_y);")
        else:
            lines.append(typename + " difference;")
            lines += ["difference.m_" + field + " = otherPosition.m_" + field + ";" for field in order]
            lines += ["difference.m_" + field + " -= position.m_" + field + ";" for field in order]
        expression = "difference.m_x * difference.m_x + difference.m_y * difference.m_y"
        if squares == 1:
            lines += ["int squaredDistance = difference.m_x * difference.m_x;",
                      "squaredDistance += difference.m_y * difference.m_y;"]
            expression = "squaredDistance"
        elif squares == 2:
            lines += ["difference.m_" + field + " *= difference.m_" + field + ";" for field in order]
            expression = "difference.m_x + difference.m_y"
        lines.append("int distance = static_cast<int>(sqrt(static_cast<double>(" + expression + ")));")
        body = original[:start] + "".join("        " + line + "\n" for line in lines) + original[end:]
        if body not in seen:
            seen.add(body)
            yield dict(name=typename + "+construction_%d+squares_%d+" % (construction, squares) + order, replace=body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--displacements", action="store_true", help="existing point/vector displacement values and scalar square results")
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    options = list(displacement_frontier(source) if args.displacements else variants(source))
    args.output.write_text(json.dumps(dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="zone_fit_locals", source=SOURCE, find=options[0]["replace"], options=options)]), indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), "zone-fit states")


if __name__ == "__main__":
    main()
