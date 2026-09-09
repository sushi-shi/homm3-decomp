#!/usr/bin/env python3
"""Canonical dimension-accessor calls in rmgTerrainPainter's constructor.

Retail 0x5b45f0 and candidate have 29 identical CFG blocks. At +0x8e,
retail stores width before loading height, then reloads width for the area.
The direct dimension/product family did not recover this schedule. Existing
ordinary getWidth/getHeight calls supply dimensions throughout paintTransitions;
test those same canonical boundaries here. No Dreamcast counterpart is known.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants():
    previous = generator("generate-rmg-painter-size-polish-hypotheses.py")
    # Retain the five reviewed dimension ownership forms without carrying the
    # exhausted direct-member area spellings as a separate search axis.
    setups = []
    for name, body in previous.bodies():
        if "m_width_times_m_height+direct" in name:
            setups.append((name.split("+")[0], body[:body.index("    m_packedCells.resize")]))
    if len(setups) != 5:
        raise ValueError("review dimension parent forms")
    products = ("m_width * m_height", "getWidth() * m_height",
                "m_width * getHeight()", "getWidth() * getHeight()")
    for (owner, prefix), product, binding in itertools.product(setups, products, range(3)):
        if binding == 0:
            expression, extra = product, ""
        elif binding == 1:
            expression, extra = "area", f"    const unsigned int area = {product};\n"
        else:
            left, right = product.split(" * ")
            expression, extra = "area", f"    unsigned int area = {left};\n    area *= {right};\n"
        yield f"{owner}+{product}+area_{binding}", prefix + extra + (
            f"    m_packedCells.resize({expression}, TRmgPackedTerrainCell());\n}}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
    previous = generator("generate-rmg-painter-size-polish-hypotheses.py")
    original = previous.make_manifest(source)["axes"][0]["find"]
    forms = list(variants())
    if forms[0][1] != original:
        raise ValueError("review changed painter constructor")
    axes = [generator("generate-rmg-position-family.py").axis(
        "painter_area_accessors", "src/rmg_terrain.cpp", original, forms)]
    args.output.write_text(json.dumps(dict(schema=1, units=["rmg_terrain"],
                                         evidence=__doc__, axes=axes), indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(forms), "painter area states")


if __name__ == "__main__":
    main()
