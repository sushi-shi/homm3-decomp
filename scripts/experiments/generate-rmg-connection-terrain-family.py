#!/usr/bin/env python3
"""Bounded final pass on buildZoneConnectionPaths, retail 0x5405d0.

At +0x131 retail reads the low six terrain bits without sign extension.
Keep the canonical signed bitfield used by other callers; recover this
caller's unsigned encoding projection. No helper or control-flow changes.
"""
import argparse
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.test_rmg_families import generator
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::buildZoneConnectionPaths"
READ = "unsigned terrain = current->m_tile.m_landType;"
FORMS = (
    READ,
    "unsigned terrain = current->m_tile.m_landType & 0x3f;",
    "unsigned terrain = static_cast<unsigned>(current->m_tile.m_landType) & 0x3f;",
    "unsigned terrain = current->m_tile.m_landType;\n                        terrain &= 0x3f;",
    "const unsigned terrain = current->m_tile.m_landType & 0x3f;",
    "int terrain = current->m_tile.m_landType & 0x3f;",
    "const int terrain = current->m_tile.m_landType & 0x3f;",
)


def axes(source):
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, FUNCTION)
    # Select the longest recognized form so the two-statement form is atomic.
    found = max((form for form in FORMS if form in original), key=len)
    options = [("terrain_%d" % i, original.replace(found, form)) for i, form in enumerate(FORMS)]
    options.sort(key=lambda option: option[1] != original)
    return [helper.axis("connection_terrain", SOURCE, original, options)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg"], axes=axes((HOMM3_DIR / SOURCE).read_text()), evidence=__doc__)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)


if __name__ == "__main__":
    main()
