#!/usr/bin/env python3
"""Generate 60 source hypotheses for the Complete-only river tile writer.

Retail 0x532520 writes the packed river sprite, tests the full integer kind,
marks the clipped 3x3 neighbourhood impassable, then clears routing targets
in the clipped 5x5 neighbourhood only on cells with a zero stored river kind.
The two y-major loops and signed, end-exclusive clamps stay fixed. Retail loads
all four sprite inputs before either packed store; compare scalar snapshots
with the direct-read control and a canonical tile copy. Vary real bounds and
predicate lifetimes without adding helper boundaries.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source, hypotheses

FUNCTION = "?setTile@TRmgMapAdapter@@UAEXABUTRmgGridPoint@@ABUrmgTerrainTile@@@Z"
SIGNATURE = "void TRmgMapAdapter::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)"


def indent(lines):
    return ["    " + line for line in lines]


def neighbourhood(radius, storage, receiver):
    record = storage.endswith("record")
    shared = storage.startswith("shared")
    names = ["minimumX", "minimumY", "maximumX", "maximumY"]
    fields = ["bounds.m_" + name if record else name for name in names]
    lines = []
    if record and (not shared or radius == 1):
        lines.append("TRmgZoneBounds bounds;")
    if storage == "scoped_origin":
        lines += ["int originX = static_cast<int>(point.m_x);",
                  "int originY = static_cast<int>(point.m_y);"]
        x, y = "originX", "originY"
    else:
        x, y = "static_cast<int>(point.m_x)", "static_cast<int>(point.m_y)"
    expressions = [f"max({x} - {radius}, 0)", f"max({y} - {radius}, 0)",
                   f"min({x} + {radius + 1}, m_map->m_mapWidth)",
                   f"min({y} + {radius + 1}, m_map->m_mapHeight)"]
    for name, expression in zip(fields, expressions):
        declare = not record and (not shared or radius == 1)
        lines.append(("int " if declare else "") + name + " = " + expression + ";")
    # Separate real loop scopes also compile with VC6's pre-standard for scope.
    loop = [f"for (int y = {fields[1]}; y < {fields[3]}; ++y) {{",
            f"    for (int x = {fields[0]}; x < {fields[2]}; ++x) {{"]
    cell = "m_map->m_mapItems[y * m_map->m_mapWidth + x]"
    statements = []
    if radius == 2:
        statements.append("TRmgMapItem& neighbour = " + cell + ";")
        prefix = "neighbour."
    elif receiver == "mixed":
        prefix = cell + "."
    elif receiver == "pointer":
        statements.append("TRmgMapItem* neighbour = &" + cell + ";")
        prefix = "neighbour->"
    elif receiver == "flags":
        statements.append("TRmgGroundTileData& flags = " + cell + ".m_tileData;")
        statements.append("flags.m_impassable = 1;")
        prefix = None
    elif receiver == "index":
        statements.append("int index = y * m_map->m_mapWidth + x;")
        statements.append("TRmgMapItem& neighbour = m_map->m_mapItems[index];")
        prefix = "neighbour."
    else:
        statements.append("TRmgMapItem& neighbour = " + cell + ";")
        prefix = "neighbour."
    if radius == 1 and prefix is not None:
        statements.append(prefix + "m_tileData.m_impassable = 1;")
    elif radius == 2:
        statements += ["if (" + prefix + "m_tile.m_riverType == 0) {",
                       "    " + prefix + "m_tileData.m_riverTarget = 0;", "}"]
    loop += indent(indent(statements)) + ["    }", "}"]
    if shared:
        return lines + ["{"] + indent(loop) + ["}"]
    return ["{"] + indent(lines + loop) + ["}"]


def bodies(refine=False):
    storage_options = ("scoped_scalar", "shared_scalar", "scoped_record", "shared_record")
    if refine:
        combinations = ((storage, "scalar_retail", receiver) for storage, receiver in itertools.product(
            storage_options, ("mixed", "reference", "pointer", "flags", "index")))
    else:
        combinations = ((storage, snapshot, "mixed") for storage, snapshot in itertools.product(
            (*storage_options, "scoped_origin"), ("direct", "scalar_retail", "scalar_fields", "tile_copy")))
    for storage, snapshot, receiver in combinations:
        for predicate in ("unsigned char", "bool", "int"):
            lines = [
                "TRmgMapItem& item = m_map->m_mapItems[point.m_y * m_map->m_mapWidth + point.m_x];"]
            if snapshot.startswith("scalar"):
                fields = ("flipX", "terrain", "flipY", "frame") if snapshot == "scalar_retail" else (
                    "terrain", "frame", "flipX", "flipY")
                for field in fields:
                    ctype = "unsigned char" if field.startswith("flip") else "int"
                    lines.append(f"{ctype} {field} = tile.m_{field};")
                prefix = ""
            elif snapshot == "tile_copy":
                lines.append("rmgTerrainTile snapshot(tile);")
                prefix = "snapshot.m_"
            else:
                prefix = "tile.m_"
            lines += ["item.m_tile.m_riverType = " + prefix + "terrain;",
                      "item.m_tile.m_riverFrame = " + prefix + "frame;",
                      "item.m_tileData.m_riverFlipX = " + prefix + "flipX;",
                      "item.m_tileData.m_riverFlipY = " + prefix + "flipY;",
                      predicate + " present = tile.m_terrain != 0;",
                      "item.m_tileData.m_hasRiver = present;"]
            lines += ["if (tile.m_terrain != 0) {"]
            lines += indent(neighbourhood(1, storage, receiver) + neighbourhood(2, storage, receiver))
            lines.append("}")
            name = "+".join((storage, snapshot, receiver, predicate.replace(" ", "_")))
            yield name, SIGNATURE + "\n{\n" + "\n".join(indent(lines)) + "\n}"


def make_manifest(source, refine=False):
    definitions = _source.find_definitions(source, FUNCTION)
    if len(definitions) != 1:
        raise ValueError("review the unique concrete river tile definition")
    definition = definitions[0]
    start = source.rfind("\n", 0, definition.head) + 1
    original = source[start:definition.body_close + 1]
    options = list(bodies(refine))
    if original not in {body for _, body in itertools.chain(bodies(), bodies(True))}:
        raise ValueError("review the river tile writer before rebasing the family")
    # Preserve the complete finite matrix when rebasing from the other phase.
    # The runner also measures a separate canonical baseline before its batch.
    if original not in {body for _, body in options}:
        options.append(("unchanged_control", original))
    options.sort(key=lambda row: row[1] != original)
    return dict(schema=1, unit="rmg", function=FUNCTION, evidence=__doc__, axes=[dict(
        name="river_tile", find=original,
        options=[dict(name=name, replace=body) for name, body in options])])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--refine", action="store_true", help="test the packed-field receiver around scalar-snapshot leaders")
    args = parser.parse_args()
    payload = make_manifest((HOMM3_DIR / "src/rmg.cpp").read_text(), args.refine)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    parsed = hypotheses.parse_manifest(args.output)
    print("generated", len(hypotheses.variants(parsed[4], parsed[5])), "unique source hypotheses ->", args.output)


if __name__ == "__main__":
    main()
