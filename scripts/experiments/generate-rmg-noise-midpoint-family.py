#!/usr/bin/env python3
"""Noise midpoint/value ownership at retail 0x53e9e0.

Retail and candidate have all 32 CFG blocks and the same operation schedule;
only the center/X/Y homes differ. Treat the midpoint as a point or independent
coordinates and vary the real region-copy and center-sample lifetimes. Keep
quadrant order, complete region copies, public vector insertion and signatures.
There is no Dreamcast procedure counterpart for this Complete-only helper.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.test_rmg_families import generator
from homm3.vc6 import source_families


def variants(original):
    x = "(region.m_bounds.m_minimumX + region.m_bounds.m_maximumX) / 2"
    y = "(region.m_bounds.m_minimumY + region.m_bounds.m_maximumY) / 2"
    old = f"    int middleY = {y};\n    int middleX = {x};\n    TRmgNoiseRegion part = region;"
    if original.count(old) != 1:
        raise ValueError("review changed subdivision body")
    coordinate_forms = [
        f"    int middleY = {y};\n    int middleX = {x};",
        f"    TPoint middle({x}, {y});",
        f"    TPoint middle;\n    middle.m_y = {y};\n    middle.m_x = {x};",
        f"    TPoint middle;\n    middle.m_x = {x};\n    middle.m_y = {y};",
        f"    const TPoint& middle = TPoint({x}, {y});",
    ]
    for coordinate, copy, center in itertools.product(range(5), range(4), range(3)):
        setup = coordinate_forms[coordinate]
        if copy == 0:
            setup += "\n    TRmgNoiseRegion part = region;"
        elif copy == 1:
            setup += "\n    TRmgNoiseRegion part;\n    part = region;"
        elif copy == 2:
            setup = "    TRmgNoiseRegion part = region;\n" + setup
        else:
            setup = "    TRmgNoiseRegion part;\n" + setup + "\n    part = region;"
        tail = original[original.index(old) + len(old):]
        if coordinate:
            tail = tail.replace("middleX", "middle.m_x").replace("middleY", "middle.m_y")
        if center:
            local = "    const int center = centerValue;"
            setup = local + "\n" + setup if center == 1 else setup + "\n" + local
            tail = tail.replace("centerValue", "center")
        yield f"midpoint_{coordinate}+copy_{copy}+center_{center}", original[:original.index(old)] + setup + tail


def make_axes(source):
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, "subdivideRmgNoiseRegion")
    return [helper.axis("noise_midpoint", "src/rmg.cpp", original, variants(original))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__,
                   axes=make_axes((HOMM3_DIR / "src/rmg.cpp").read_text()))
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "noise midpoint states")


if __name__ == "__main__":
    main()
