#!/usr/bin/env python3
"""Canonical lookup base-pointer lifetime in the underground plane query.

At decorateUnderground +0x31 retail reads m_mapItems before the index
multiply; the 99.7982% candidate reads it after the LEA. The initial view
is the only remaining differing block. Prior row/result-only families did
not name the base pointer independently. Preserve the scalar inline API,
signed arithmetic and row-major formula, measuring all seven consumers.
No Dreamcast counterpart is mapped for this Complete-only helper/caller.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants(original):
    signature = generator("generate-rmg-map-accessor-family.py").SIGNATURE
    seen = set()
    for base, offset, dimensions in itertools.product(range(5), range(4), range(3)):
        lines = []
        height, width = "m_mapHeight", "m_mapWidth"
        pointer = "m_mapItems"
        pointer_line = ""
        if base:
            typename = {1: "TRmgMapItem*", 2: "TRmgMapItem*", 3: "TRmgMapItem* const&", 4: "TRmgMapItem*&"}[base]
            pointer_line = f"{typename} items = m_mapItems;"
            pointer = "items"
            if base != 2:
                lines.append(pointer_line)
        if dimensions == 1:
            lines += ["int width = m_mapWidth;", "int height = m_mapHeight;"]
            width, height = "width", "height"
        elif dimensions == 2:
            lines.append("TPoint dimensions(m_mapWidth, m_mapHeight);")
            width, height = "dimensions.m_x", "dimensions.m_y"
        index = f"(z * {height} + y) * {width} + x"
        if offset == 1:
            lines.append(f"int row = z * {height} + y;")
            index = f"row * {width} + x"
        elif offset == 2:
            lines.append(f"int index = {index};")
            index = "index"
        elif offset == 3:
            lines += [f"int index = z * {height};", "index += y;", f"index *= {width};", "index += x;"]
            index = "index"
        if base == 2:
            lines.append(pointer_line)
        lines.append(f"return {pointer} + {index};")
        body = signature + "\n    {\n" + "\n".join("        " + line for line in lines) + "\n    }"
        if body not in seen:
            seen.add(body)
            yield "base_%d+offset_%d+dimensions_%d" % (base, offset, dimensions), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    helper = generator("generate-rmg-map-accessor-family.py")
    original = helper.definition((HOMM3_DIR / "include/rmg.h").read_text())
    forms = list(variants(original))
    if forms[0][1] != original:
        raise ValueError("review changed canonical accessor before rebasing")
    payload = dict(schema=1, units=helper.UNITS, evidence=__doc__, axes=[
        generator("generate-rmg-position-family.py").axis("map_base_pointer", "include/rmg.h", original, forms)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(forms), "canonical map-base states")


if __name__ == "__main__":
    main()
