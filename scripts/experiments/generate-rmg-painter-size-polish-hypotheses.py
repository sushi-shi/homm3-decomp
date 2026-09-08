#!/usr/bin/env python3
"""Generate 60 dimension-lifetime hypotheses for the 97.9205% painter constructor.

Retail 0x5b45f0 agrees through the assigned virtual size query and differs at
the dimension stores/product feeding vector resize. Keep the existing member
initializers, one query, canonical grid assignment and packed-cell constructor.
Cross real dimension snapshots with six equivalent area sources and direct or
named area arguments. No vector API, helper, declaration or inlining pin changes.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source, hypotheses

FUNCTION = "??0rmgTerrainPainter@@QAE@PAVTRmgMapInterface@@HH@Z"
SIGNATURE = "rmgTerrainPainter::rmgTerrainPainter(\n    TRmgMapInterface* newAdapter, int terrain, int strength)\n    : m_adapter(newAdapter), m_paintTerrain(terrain), m_transitionStrength(strength)"


def bodies():
    areas = ("m_width * m_height", "m_height * m_width", "size.m_x * size.m_y",
             "size.m_y * size.m_x", "m_width * size.m_y", "size.m_x * m_height")
    for storage, area, argument in itertools.product(
            ("direct", "width", "height", "both", "assignment_reference"), areas, ("direct", "named")):
        lines = ["TRmgGridPoint size;"]
        if storage == "assignment_reference":
            lines += ["TRmgGridPoint& dimensions = (size = m_adapter->getSize());",
                      "m_width = dimensions.m_x;", "m_height = dimensions.m_y;"]
        else:
            lines.append("size = m_adapter->getSize();")
            if storage in ("width", "both"):
                lines.append("unsigned int width = size.m_x;")
            if storage in ("height", "both"):
                lines.append("unsigned int height = size.m_y;")
            lines += ["m_width = " + ("width" if storage in ("width", "both") else "size.m_x") + ";",
                      "m_height = " + ("height" if storage in ("height", "both") else "size.m_y") + ";"]
        if argument == "named":
            lines.append("unsigned int area = " + area + ";")
            expression = "area"
        else:
            expression = area
        lines.append("m_packedCells.resize(" + expression + ", TRmgPackedTerrainCell());")
        name = "+".join((storage, area.replace(" * ", "_times_").replace(".", "_"), argument))
        yield name, SIGNATURE + "\n{\n" + "".join("    " + line + "\n" for line in lines) + "}"


def make_manifest(source):
    definitions = _source.find_definitions(source, FUNCTION)
    if len(definitions) != 1:
        raise ValueError("review the unique painter constructor definition")
    definition = definitions[0]
    start = source.rfind("\n", 0, definition.head) + 1
    original = source[start:definition.body_close + 1]
    options = list(bodies())
    if original not in {body for _, body in options}:
        raise ValueError("review the painter dimension setup before rebasing the family")
    options.sort(key=lambda row: row[1] != original)
    return dict(schema=1, unit="rmg_terrain", function=FUNCTION, evidence=__doc__, axes=[dict(
        name="painter_dimension_lifetimes", find=original,
        options=[dict(name=name, replace=body) for name, body in options])])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = make_manifest((HOMM3_DIR / "src/rmg_terrain.cpp").read_text())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    parsed = hypotheses.parse_manifest(args.output)
    print("generated", len(hypotheses.variants(parsed[4], parsed[5])), "unique source hypotheses ->", args.output)


if __name__ == "__main__":
    main()
