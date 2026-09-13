#!/usr/bin/env python3
"""Generate 60 reviewed river-presence writer alternatives for homm3 hypotheses.

Retail 0x532730 writes the four-bit river kind, then sets tile-data bit 29
from the full integer argument. The neighboring tile setter 0x53259b..0x5325ac
corroborates this flag. Bit 30 is a separate routing endpoint, not this field.
Compare real receiver/cell addressing and predicate lifetimes; do not truncate
the input before testing it, overwrite packed siblings, or introduce helpers.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source, hypotheses

FUNCTION = "?setOverlay@TRmgMapAdapter@@UAEXABUTRmgGridPoint@@H@Z"
SIGNATURE = "void TRmgMapAdapter::setOverlay(const TRmgGridPoint& point, int value)"


def cell_forms():
    expression = "m_map->m_mapItems[point.m_y * m_map->m_mapWidth + point.m_x]"
    yield "cell_reference", f"TRmgMapItem& item = {expression};", "item."
    yield "cell_pointer", f"TRmgMapItem* item = &{expression};", "item->"
    yield "map_reference", ("type_random_map& map = *m_map;\n"
                            "TRmgMapItem& item = map.m_mapItems[point.m_y * map.m_mapWidth + point.m_x];"), "item."
    yield "map_pointer", ("type_random_map* map = m_map;\n"
                          "TRmgMapItem& item = map->m_mapItems[point.m_y * map->m_mapWidth + point.m_x];"), "item."
    for name, fields in (("coordinate_xy", ("x", "y")), ("coordinate_yx", ("y", "x"))):
        yield name, "\n".join([
            *(f"unsigned int {field} = point.m_{field};" for field in fields),
            "TRmgMapItem& item = m_map->m_mapItems[y * m_map->m_mapWidth + x];"]), "item."


def flag_forms(prefix):
    write_kind = prefix + "m_tile.m_riverType = value;"
    flag = prefix + "m_tileData.m_hasRiver"
    yield "direct", [write_kind, flag + " = value != 0;"]
    for spelling, ctype in (("byte", "unsigned char"), ("boolean", "bool"), ("integer", "int")):
        snapshot = f"{ctype} present = value != 0;"
        for order in ("after", "before"):
            statements = [write_kind, snapshot] if order == "after" else [snapshot, write_kind]
            yield spelling + "_" + order, [*statements, flag + " = present;"]
    yield "signed_byte_before", ["signed char present = value != 0;", write_kind, flag + " = present;"]
    yield "byte_cast", [write_kind, flag + " = static_cast<unsigned char>(value != 0);"]
    yield "byte_conditional", ["unsigned char present = value ? 1 : 0;", write_kind, flag + " = present;"]


def bodies():
    for cell_name, setup, prefix in cell_forms():
        for flag_name, statements in flag_forms(prefix):
            lines = [*setup.splitlines(), *statements]
            yield cell_name + "+" + flag_name, SIGNATURE + "\n{\n" + "".join("    " + line + "\n" for line in lines) + "}"


def make_manifest(source):
    definitions = _source.find_definitions(source, FUNCTION)
    if len(definitions) != 1:
        raise ValueError("review the unique concrete river overlay definition")
    definition = definitions[0]
    start = source.rfind("\n", 0, definition.head) + 1
    original = source[start:definition.body_close + 1]
    options = list(bodies())
    if original not in [body for _, body in options]:
        raise ValueError("review the river-presence writer before rebasing the family")
    options.sort(key=lambda row: row[1] != original)
    return dict(schema=1, unit="rmg", function=FUNCTION, evidence=__doc__, axes=[dict(
        name="river_presence", find=original,
        options=[dict(name=name, replace=body) for name, body in options])])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = make_manifest((HOMM3_DIR / "src/rmg.cpp").read_text())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    parsed = hypotheses.parse_manifest(args.output)
    print("generated", len(hypotheses.variants(parsed[4], parsed[5])), "unique source hypotheses ->", args.output)


if __name__ == "__main__":
    main()
