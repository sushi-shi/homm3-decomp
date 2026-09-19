#!/usr/bin/env python3
"""Recover retained owning-map construction in TRmgGeneratorBase 0x536070.

The owning constructor 0x530fb0 is exact, but its generator-base caller
expands allocation/array construction instead of retaining the retail call
(48.0526%). Preserve the ordinary constructor and canonical cell array.
Vary meaningful dimension ownership, allocation-size calculation, and when
the ownership flag is initialized. Score the constructor and every RMG caller.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
NAME = "type_random_map::type_random_map"
HEAD = "type_random_map::type_random_map(int width, int height, int levels)"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def variants():
    seen = set()
    for dimensions, count, ownership in itertools.product(range(4), range(5), range(3)):
        head = HEAD
        dims = "    m_mapWidth = width;\n    m_mapHeight = height;\n    m_numberLevels = levels;\n"
        if dimensions == 1:
            head += "\n    : m_mapWidth(width), m_mapHeight(height), m_numberLevels(levels)"
            dims = ""
        elif dimensions >= 2:
            kind = "TPoint" if dimensions == 2 else "TRmgGridPoint"
            dims = (f"    {kind} size(width, height);\n"
                    "    m_mapWidth = size.getX();\n    m_mapHeight = size.getY();\n"
                    "    m_numberLevels = levels;\n")
        allocation = (
            "    m_mapItems = new TRmgMapItem[width * height * levels];\n",
            "    m_mapItems = new TRmgMapItem[m_mapWidth * m_mapHeight * m_numberLevels];\n",
            "    int cellCount = width * height * levels;\n    m_mapItems = new TRmgMapItem[cellCount];\n",
            "    int planeSize = width * height;\n    m_mapItems = new TRmgMapItem[planeSize * levels];\n",
            "    int cellCount = m_mapWidth * m_mapHeight;\n    cellCount *= m_numberLevels;\n    m_mapItems = new TRmgMapItem[cellCount];\n",
        )[count]
        flag = "    m_ownsMapItems = 1;\n"
        body = (dims + flag + allocation, flag + dims + allocation, dims + allocation + flag)[ownership]
        text = head + "\n{\n" + body + "}"
        if text not in seen:
            seen.add(text)
            yield dict(name=f"dimensions_{dimensions}+count_{count}+ownership_{ownership}", replace=text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    original = definition((HOMM3_DIR / SOURCE).read_text())
    options = list(variants())
    options.sort(key=lambda row: row["replace"] != original)
    assert options[0]["replace"] == original
    assert len(options) == 55
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="owning_map_construction", source=SOURCE, find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("55 owning-map construction states")


if __name__ == "__main__":
    main()
