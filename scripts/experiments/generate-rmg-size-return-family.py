#!/usr/bin/env python3
"""Map-size return ownership, preserving both canonical grid constructors.

Retail 0x532240 loads the hidden result before width; our source reverses
those independent loads. The ICF adapters at 0x532790 forward one virtual
query, then copy its returned value with a different register schedule.
No Dreamcast counterpart is mapped. Earlier isolated return forms are
controls here; the new axis couples dimension-temporary reference lifetimes
with base and adapter return construction, without changing a shared type.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def body(owner, lines):
    return "TRmgGridPoint " + owner + "::getSize()\n{\n" + "\n".join(
        "    " + line for line in lines) + "\n}"


def base_forms():
    for capture, returned in itertools.product(range(2), range(6)):
        lines = []
        x, y = "m_mapWidth", "m_mapHeight"
        if capture:
            lines = ["const unsigned int& width = m_mapWidth;",
                     "const unsigned int& height = m_mapHeight;"]
            x, y = "width", "height"
        point = "TRmgGridPoint(" + x + ", " + y + ")"
        forms = [
            ["return " + point + ";"],
            ["TRmgGridPoint size(" + x + ", " + y + ");", "return size;"],
            ["const TRmgGridPoint& size = " + point + ";", "return size;"],
            ["TRmgGridPoint size;", "size.m_x = " + x + ";",
             "size.m_y = " + y + ";", "return size;"],
            ["TRmgGridPoint size;", "size = " + point + ";", "return size;"],
            ["const TRmgGridPoint size = " + point + ";", "return size;"],
        ]
        yield "capture_%d+return_%d" % (capture, returned), body(
            "type_random_map", lines + forms[returned])


def adapter_forms(owner):
    for name, lines in [
        ("named", ["TRmgGridPoint size = m_map->getSize();", "return size;"]),
        ("reference", ["const TRmgGridPoint& size = m_map->getSize();", "return size;"]),
        ("assignment", ["TRmgGridPoint size;", "size = m_map->getSize();", "return size;"]),
        ("direct", ["return m_map->getSize();"]),
        ("const_value", ["const TRmgGridPoint size = m_map->getSize();", "return size;"]),
    ]:
        yield name, body(owner, lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    helper = generator("generate-rmg-position-family.py")
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    base = list(base_forms())
    river = list(adapter_forms("TRmgMapAdapter"))
    road = list(adapter_forms("TRmgRoadMapAdapter"))
    for owner, forms in (("type_random_map", base), ("TRmgMapAdapter", river),
                         ("TRmgRoadMapAdapter", road)):
        if helper.definition(source, owner + "::getSize") != forms[0][1]:
            raise ValueError("review changed size getter before rebasing: " + owner)
    adapters = helper.axis("adapter_return", "src/rmg.cpp", river[0][1], river)
    for option, (_, road_body) in zip(adapters["options"], road):
        option["extra_edits"] = [dict(source="src/rmg.cpp", find=road[0][1], replace=road_body)]
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[
        helper.axis("base_return", "src/rmg.cpp", base[0][1], base), adapters])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("60 paired size-return states; both adapter ICF owners remain identical")


if __name__ == "__main__":
    main()
