#!/usr/bin/env python3
"""Shared versus four scoped quadrant records at retail 0x53e9e0.

All 32 blocks, operations and insertion calls currently agree; center/X/Y
stack homes differ. Earlier midpoint families kept one shared quadrant record.
Test independent quadrant lifetimes together with the real midpoint ownership,
without adding operations, helpers or changing the by-value boundary.
No Dreamcast counterpart exists for this Complete-only function.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from experiments._support import generator

NAME = "subdivideRmgNoiseRegion"


def variants(original):
    x = "(region.m_bounds.m_minimumX + region.m_bounds.m_maximumX) / 2"
    y = "(region.m_bounds.m_minimumY + region.m_bounds.m_maximumY) / 2"
    start = f"    int middleY = {y};\n    int middleX = {x};"
    assert original.count(start) == 1
    prefix, body = original.split("    TRmgNoiseRegion part = region;", 1)
    chunks = ("    TRmgNoiseRegion part = region;" + body[:-2]).split("\n\n")
    assert len(chunks) == 4 and all(c.endswith("pending.push_back(part);") for c in chunks)
    coordinates = [start, f"    int middleX = {x};\n    int middleY = {y};",
                   f"    TPoint middle({x}, {y});",
                   f"    TPoint middle;\n    middle.m_y = {y};\n    middle.m_x = {x};"]
    for ownership, coordinate in itertools.product(range(3), range(4)):
        parts = chunks[:]
        if ownership:
            for i, chunk in enumerate(parts):
                if i:
                    chunk = chunk.replace("    part = region;", "    TRmgNoiseRegion part = region;", 1)
                if ownership == 1:
                    chunk = "    {\n" + "\n".join("    " + line for line in chunk.splitlines()) + "\n    }"
                else:
                    # Distinct records live in the same enclosing function scope.
                    chunk = chunk.replace("part", "quadrant" + str(i))
                parts[i] = chunk
        tail = "\n\n".join(parts) + "\n}"
        if coordinate >= 2:
            tail = tail.replace("middleX", "middle.m_x").replace("middleY", "middle.m_y")
        candidate = prefix.replace(start, coordinates[coordinate]) + tail
        yield (f"records_{ownership}+coordinates_{coordinate}", candidate)


def make_axes(source):
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, NAME)
    return [helper.axis("noise_quadrant_ownership", "src/rmg.cpp", original, variants(original))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg"], evidence=__doc__,
                   axes=make_axes((HOMM3_DIR / "src/rmg.cpp").read_text()))
    assert len(payload['axes'][0]['options']) == 12
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("12 quadrant ownership states")


if __name__ == "__main__":
    main()
