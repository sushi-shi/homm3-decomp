#!/usr/bin/env python3
"""Coordinate ownership across appendZonePositions' three insertion sites.

Retail 0x53ae80 preserves the center position, tests candidate zone positions,
and reloads each accepted position before appending. Candidate's last insertion
expands past retail's helper boundaries (39 versus 31 blocks). Test the actual
position constructor, assignment, and existing ordinary zone setter together
with meaningful accepted-position lifetimes. No DC RMG counterpart exists.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from experiments._support import generator

SOURCE = "src/rmg.cpp"
NAME = "type_random_map_generator::appendZonePositions"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def replace(body, old, new, count=1):
    if body.count(old) != count:
        raise ValueError("review append-zone anchor: " + old)
    return body.replace(old, new)


def construction(body, form):
    if form == 0:
        return body
    y = "static_cast<int>(position.m_y + radius * g_rmgDirectionSines[direction])"
    x = "static_cast<int>(position.m_x + radius * g_rmgDirectionCosines[direction])"
    first = f"        TRmgMapPosition candidate;\n        candidate.m_y = {y};\n        candidate.m_x = {x};\n        candidate.m_z = position.m_z;"
    middle = "    TRmgMapPosition candidate;\n    candidate.m_x = position.m_x;\n    candidate.m_y = position.m_y;\n    candidate.m_z = level;"
    last = f"        candidate.m_y = {y};\n        candidate.m_x = {x};\n        candidate.m_z = level;"
    if form in (1, 2):
        if form == 1:
            a = f"        TRmgMapPosition candidate({x}, {y}, position.m_z);"
            b = "    TRmgMapPosition candidate(position.m_x, position.m_y, level);"
        else:
            a = f"        TRmgMapPosition candidate;\n        candidate = TRmgMapPosition({x}, {y}, position.m_z);"
            b = "    TRmgMapPosition candidate;\n    candidate = TRmgMapPosition(position.m_x, position.m_y, level);"
        c = f"        candidate = TRmgMapPosition({x}, {y}, level);"
    elif form == 3:
        a = f"        TRmgMapPosition candidate = position;\n        candidate.m_y = {y};\n        candidate.m_x = {x};"
        b = "    TRmgMapPosition candidate = position;\n    candidate.m_z = level;"
        c = last
    else:
        body = replace(body, "    TRmgMapPosition position = center->getLevelPosition();",
            "    TRmgMapPosition position = center->getLevelPosition();\n    TRmgMapPosition candidate;")
        a = first.replace("        TRmgMapPosition candidate;\n", "")
        b = middle.replace("    TRmgMapPosition candidate;\n", "")
        c = last
    for old, new in ((first, a), (middle, b), (last, c)):
        body = replace(body, old, new)
    return body


def updates(body, form):
    if form == 0:
        return body
    for indent, count in (("        ", 2), ("    ", 1)):
        old = "\n".join(indent + f"zone->m_levelPosition.m_{axis} = candidate.m_{axis};" for axis in "xyz")
        new = indent + ("zone->m_levelPosition = candidate;" if form == 1
                        else "zone->setLevelPosition(candidate);")
        body = replace(body, old, new, count)
    return body


def accepted_positions(body, form):
    if form == 0:
        return body
    declarations = {
        1: "TRmgMapPosition accepted = zone->getLevelPosition();",
        2: "TRmgMapPosition accepted;\n{indent}    accepted = zone->getLevelPosition();",
        3: "const TRmgMapPosition& accepted = zone->getLevelPosition();",
    }
    for indent, count in (("        ", 2), ("    ", 1)):
        old = indent + "if (canPlaceZone(zone))\n" + indent + "    candidates.push_back(zone->getLevelPosition());"
        new = (indent + "if (canPlaceZone(zone)) {\n" + indent + "    "
               + declarations[form].format(indent=indent) + "\n" + indent
               + "    candidates.push_back(accepted);\n" + indent + "}")
        body = replace(body, old, new, count)
    return body


def variants(original):
    for point, update, accepted in itertools.product(range(5), range(3), range(4)):
        body = accepted_positions(updates(construction(original, point), update), accepted)
        yield dict(name=f"point_{point}+update_{update}+accepted_{accepted}", replace=body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    original = definition((HOMM3_DIR / SOURCE).read_text())
    rows = list(variants(original))
    assert rows[0]["replace"] == original
    assert len(rows) == len({row["replace"] for row in rows}) == 60
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="append_zone_positions", source=SOURCE, find=original, options=rows)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(rows), "append-zone states")


if __name__ == "__main__":
    main()
