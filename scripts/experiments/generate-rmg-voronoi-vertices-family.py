#!/usr/bin/env python3
"""Voronoi circumcenter return-value lifetimes at retail 0x5fdb40.

Retail retains two subtractions and five composed vector operations while
the current source expands all seven. The aggregate return/copy slots and
nested call-argument setup motivate named results, reference-bound results,
and separately scoped arithmetic. Keep canonical operators and exact integer
division order; no Dreamcast counterpart is known.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "TRmgVoronoi::buildVertices"


def helpers():
    spec = importlib.util.spec_from_file_location("vertices_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def variants(original):
    result = "            TPoint position = origin + (axis + perpendicular * numerator / denominator) / 2;"
    expressions = [
        result,
        "            TRmgVector displacement = perpendicular * numerator / denominator;\n            TPoint position = origin + (axis + displacement) / 2;",
        "            TRmgVector diameter = axis + perpendicular * numerator / denominator;\n            TPoint position = origin + diameter / 2;",
        "            TRmgVector radius = (axis + perpendicular * numerator / denominator) / 2;\n            TPoint position = origin + radius;",
        "            TRmgVector scaled = perpendicular * numerator;\n            TRmgVector displacement = scaled / denominator;\n            TRmgVector diameter = axis + displacement;\n            TRmgVector radius = diameter / 2;\n            TPoint position = origin + radius;",
        "            TPoint position;\n            position = origin + (axis + perpendicular * numerator / denominator) / 2;",
    ]
    numerator = "            int numerator = secondSide.m_x * thirdSide.m_x + secondSide.m_y * thirdSide.m_y;\n"
    denominator = "            int denominator = perpendicular.m_x * thirdSide.m_x + perpendicular.m_y * thirdSide.m_y;\n"
    if original.count(result) != 1 or original.count(numerator + denominator) != 1:
        raise ValueError("review changed circumcenter body")
    for expression, capture, order in itertools.product(range(6), range(5), range(2)):
        body = original.replace(result, expressions[expression])
        if order:
            body = body.replace(numerator + denominator, denominator + numerator)
        if capture == 1:
            for name in ("axis", "secondSide", "thirdSide"):
                body = body.replace("            TRmgVector " + name + " =", "            TRmgVector " + name + ";\n            " + name + " =")
        elif capture == 2:
            for name in ("axis", "secondSide", "thirdSide"):
                body = body.replace("TRmgVector " + name + " =", "const TRmgVector& " + name + " =")
        elif capture == 3:
            for name in ("secondSide", "thirdSide"):
                body = body.replace("TRmgVector " + name + " =", "const TRmgVector& " + name + " =")
        elif capture == 4:
            start = body.index("            TRmgVector axis")
            end = body.index("            edge->m_position = position;")
            calculation = body[start:end].replace("            TPoint position =", "            position =").replace("            TPoint position;\n", "")
            body = body[:start] + "            TPoint position;\n            {\n" + "\n".join("    " + line for line in calculation.rstrip().split("\n")) + "\n            }\n" + body[end:]
        yield f"expression_{expression}+capture_{capture}+order_{order}", body


def make_axes(source):
    original = helpers().definition(source, FUNCTION)
    return [helpers().axis("circumcenter", SOURCE, original, variants(original))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=make_axes(source), evidence=__doc__)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated", len(payload["axes"][0]["options"]), "circumcenter states ->", args.output)


if __name__ == "__main__":
    main()
