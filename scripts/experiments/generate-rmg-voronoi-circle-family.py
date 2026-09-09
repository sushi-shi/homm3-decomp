#!/usr/bin/env python3
"""Circumcircle determinant expression and signed-product lifetimes.

Retail addSite +0x1b4:+0x2ab snapshots three points, calls all four canonical
orientations, then computes four signed 32x32-to-64 products. The current
ordinary circle predicate expands its first two orientations instead.
The Graphics Gems IV InCircle expression suggests testing embedded calls
against named areas, not copying orientation arithmetic into this predicate.
Keep the four by-value point parameters, exact shared orientation definition,
and the signed 64-bit product/sum domain. No Dreamcast counterpart is mapped.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg_support.cpp"
NAME = "isRmgPointInsideCircle"


def circle(shape, product, result):
    calls = ["getRmgPointOrientation(second, third, point)",
             "getRmgPointOrientation(first, third, point)",
             "getRmgPointOrientation(first, second, point)",
             "getRmgPointOrientation(first, second, third)"]
    names = ["firstArea", "secondArea", "thirdArea", "pointArea"]
    lines = ["int " + name + " = " + call + ";" for name, call in zip(names, calls)] if shape < 2 else []
    areas = names if shape < 2 else calls
    terms = []
    for point, area in zip(("first", "second", "third", "point"), areas):
        components = [point + ".m_x * " + point + ".m_x", point + ".m_y * " + point + ".m_y"]
        if product & 1:
            components.reverse()
        norm = " + ".join(components)
        if product & 2:
            terms.append("(" + norm + ") * static_cast<__int64>(" + area + ")")
        else:
            terms.append("static_cast<__int64>(" + norm + ") * " + area)
    a, b, c, d = terms
    if shape in (0, 2):
        expression = c + "\n        - " + b + "\n        + " + a + "\n        - " + d
    elif shape in (1, 3):
        expression = a + "\n        - " + b + "\n        + " + c + "\n        - " + d
    else:
        expression = "(" + a + "\n        + " + c + ")\n        - (" + b + "\n        + " + d + ")"
    if result == 0:
        lines += ["__int64 determinant = " + expression + ";", "return determinant > 0;"]
    elif result == 1:
        lines += ["return (" + expression + ") > 0;"]
    else:
        lines += ["unsigned char inside = (" + expression + ") > 0;", "return inside;"]
    return ("static unsigned char " + NAME + "(TPoint first, TPoint second,\n"
            "    TPoint third, TPoint point)\n{\n"
            + "\n".join("    " + line for line in lines) + "\n}")


def variants(source):
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, NAME)
    # Whitespace is not a source alternative. Preserve the exact authored
    # baseline for the corresponding named-area/current-product option.
    if " ".join(original.split()) != " ".join(circle(0, 0, 0).split()):
        raise ValueError("review changed circle predicate")
    for choice in itertools.product(range(5), range(4), range(3)):
        yield dict(name=("original" if choice == (0, 0, 0) else "shape_%d+product_%d+result_%d" % choice),
                   replace=(original if choice == (0, 0, 0) else circle(*choice)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    helper = generator("generate-rmg-position-family.py")
    options = list(variants(source))
    args.output.write_text(json.dumps(dict(schema=1, units=["rmg_support"], evidence=__doc__, axes=[dict(
        name="voronoi_circle_expression", source=SOURCE, find=helper.definition(source, NAME), options=options)]), indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), "circle determinant states")


if __name__ == "__main__":
    main()
