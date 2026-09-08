#!/usr/bin/env python3
"""Generate real proxy/point value lifetimes for the shared line refresh.

Retail 0x4f9f00 retains one neighbour compound-add, point-copy and proxy-return
call, while its original tile proxy is constructed in place. Its virtual tile
query writes directly into the final local. Keep the canonical interfaces and
all queries; vary member construction, returned proxy lifetime, output/value
tile handling, and the existing grid translation's construction/return form.
Every candidate scores all three RMG TUs because grid arithmetic is shared.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path
import re

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source
from homm3.vc6.source_families import load_manifest

SOURCE = "src/rmg_terrain.cpp"
HEADER = "include/rmg.h"


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_source_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def constructor_forms(parameter="const TRmgGridPoint&"):
    signature = ("TRmgLinePainterTile::TRmgLinePainterTile(\n"
                 f"    TRmgLinePainterInterface* painter, {parameter} point)")
    for name, initializer, body in (
            ("members_copy", " : m_painter(painter), m_point(point)", ""),
            ("members_coordinates", " : m_painter(painter), m_point(point.m_x, point.m_y)", ""),
            ("pointer_then_assignment", " : m_painter(painter)", "    m_point = point;\n"),
            ("point_then_pointer", " : m_point(point)", "    m_painter = painter;\n"),
            ("assign_pointer_point", "", "    m_painter = painter;\n    m_point = point;\n"),
            ("assign_point_pointer", "", "    m_point = point;\n    m_painter = painter;\n")):
        yield name, signature + ("\n   " + initializer if initializer else "") + "\n{\n" + body + "}"


def constructor_definition(source, *, copy=False, required=True):
    parameter = "const TRmgLinePainterTile&" if copy else "TRmgLinePainterInterface*"
    found = [item for item in _source.find_definitions(source, "TRmgLinePainterTile::TRmgLinePainterTile")
             if parameter in source[item.head:item.body_open]]
    if not found and not required:
        return ""
    if len(found) != 1:
        raise ValueError(f"expected one proxy constructor taking {parameter}")
    item = found[0]
    return source[source.rfind("\n", 0, item.head) + 1:item.body_close + 1]


def constructor_parameter(source):
    body = constructor_definition(source)
    for parameter in ("const TRmgGridPoint&", "TRmgGridPoint"):
        if f"TRmgLinePainterInterface* painter, {parameter} point)" in body:
            return parameter
    raise ValueError("review the proxy constructor's point parameter")


def constructor_declaration(source):
    return ("    TRmgLinePainterTile(TRmgLinePainterInterface* painter, "
            + constructor_parameter(source) + " point);")


def return_forms():
    signature = "TRmgLinePainterTile TRmgLinePainterInterface::at(const TRmgGridPoint& point)\n{\n"
    yield "direct", signature + "    return TRmgLinePainterTile(this, point);\n}"
    for name, declaration in (
            ("named", "TRmgLinePainterTile tile(this, point);"),
            ("copy_initialized", "TRmgLinePainterTile tile = TRmgLinePainterTile(this, point);"),
            ("const_named", "const TRmgLinePainterTile tile(this, point);")):
        yield name, signature + "    " + declaration + "\n    return tile;\n}"


def tile_forms():
    yield ("value", "    rmgTerrainTile getTile();",
           "rmgTerrainTile TRmgLinePainterTile::getTile()\n{\n"
           "    rmgTerrainTile tile;\n    m_painter->getTile(m_point, tile);\n    return tile;\n}",
           "    rmgTerrainTile current = tile.getTile();\n")
    yield ("output", "    void getTile(rmgTerrainTile& tile);",
           "void TRmgLinePainterTile::getTile(rmgTerrainTile& tile)\n{\n"
           "    m_painter->getTile(m_point, tile);\n}",
           "    rmgTerrainTile current;\n    tile.getTile(current);\n")


def grid_forms():
    for construction, returned in itertools.product(
            ("coordinates", "copy", "copy_initialized", "assigned"), ("named", "compound")):
        body = {
            "coordinates": "        TRmgGridPoint result(m_x, m_y);\n",
            "copy": "        TRmgGridPoint result(*this);\n",
            "copy_initialized": "        TRmgGridPoint result = *this;\n",
            "assigned": "        TRmgGridPoint result;\n        result = *this;\n",
        }[construction]
        body += ("        result += offset;\n        return result;" if returned == "named"
                 else "        return result += offset;")
        yield construction + "+" + returned, body


def make_axes(header, source):
    helper = helpers()
    constructor = constructor_definition(source)
    returned = helper.definition(source, "TRmgLinePainterInterface::at")
    getter = helper.definition(source, "TRmgLinePainterTile::getTile")
    query = next((row for row in tile_forms() if row[1] in header and row[2] == getter and row[3] in source), None)
    if query is None:
        raise ValueError("review the current proxy tile query and caller together")
    options = [dict(name="baseline", replace=query[1])]
    for name, prototype, body, call in tile_forms():
        if name != query[0]:
            options.append(dict(name=name, replace=prototype, extra_edits=[
                dict(source=SOURCE, find=getter, replace=body),
                dict(source=SOURCE, find=query[3], replace=call)]))
    start = header.index("    TRmgGridPoint operator+(const TPoint& offset) const")
    end = header.index("\n};", start)
    method = header[start:end]
    statement = re.search(r"^        TRmgGridPoint result\b", method, re.M)
    if statement is None:
        raise ValueError("review the grid translation's result lifetime")
    close = method.rindex("\n    }")
    translation = method[statement.start():close]
    return [helper.axis("proxy_constructor", SOURCE, constructor,
                        constructor_forms(constructor_parameter(source))),
            helper.axis("proxy_return", SOURCE, returned, return_forms()),
            dict(name="proxy_tile_query", source=HEADER, find=query[1], options=options),
            helper.axis("grid_translation", HEADER, translation, grid_forms())]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / HEADER).read_text(), (HOMM3_DIR / SOURCE).read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes, evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
