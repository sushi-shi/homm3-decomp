#!/usr/bin/env python3
"""Recover tile-value operations shared by the retained painter and terrain TU.

Retail 0x55edc0 copies two dwords and two flip bytes into a forwarded value.
0x55f350 obtains an adapter return value and writes four fields through its
output reference. Keep those boundaries and the 12-byte natural layout;
compare canonical copy/assignment definitions and returned-value lifetimes.
All three TUs, including the already matched terrain users, are measured.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

FIELDS = ("terrain", "frame", "flipX", "flipY")
COPY_NOTE = ("    // 0x55edc0 constructs its snapshot separately from adapter return values.\n"
             "    // Those returns keep an implicit copy boundary: a custom copy constructor\n"
             "    // changes the retained 0x5b3dd0 fill and its expanded terrain callers.\n")


def helper_module():
    spec = importlib.util.spec_from_file_location(
        "rmg_source_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def copy_forms():
    yield "implicit", COPY_NOTE
    signature = COPY_NOTE + "    rmgTerrainTile(const rmgTerrainTile& other)\n"
    yield "initializers", (signature + "        : m_terrain(other.m_terrain), m_frame(other.m_frame),\n"
                           "          m_flipX(other.m_flipX), m_flipY(other.m_flipY) {}\n")
    for order in itertools.permutations(FIELDS):
        body = "".join(f"        m_{field} = other.m_{field};\n" for field in order)
        yield "+".join(order), signature + "    {\n" + body + "    }\n"


def assignment_forms():
    comment = "    // The output-reference wrapper 0x55f350 assigns the same four fields.\n"
    yield "implicit", comment
    for argument, order in itertools.product(("reference", "value"), itertools.permutations(FIELDS)):
        parameter = "const rmgTerrainTile& other" if argument == "reference" else "rmgTerrainTile other"
        body = "".join(f"        m_{field} = other.m_{field};\n" for field in order)
        yield argument + "+" + "+".join(order), (comment
              + "    rmgTerrainTile& operator=(" + parameter + ")\n    {\n"
              + body + "        return *this;\n    }\n")


def getters(class_name):
    signature = f"void {class_name}::getTile(const TRmgGridPoint& point, rmgTerrainTile& tile)\n{{\n"
    return [
        ("direct", signature + "    tile = m_adapter->getTile(point);\n}"),
        ("snapshot", signature + "    rmgTerrainTile snapshot = m_adapter->getTile(point);\n"
         "    tile = snapshot;\n}"),
        ("reference", signature + "    const rmgTerrainTile& snapshot = m_adapter->getTile(point);\n"
         "    tile = snapshot;\n}"),
    ]


def setters(class_name):
    signature = f"void {class_name}::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)\n{{\n"
    setups = [
                ("copy", "    rmgTerrainTile snapshot = tile;\n"),
                ("direct", "    rmgTerrainTile snapshot(tile);\n"),
                ("assigned", "    rmgTerrainTile snapshot;\n    snapshot = tile;\n")]
    # A forwarded snapshot may be constructed from the two scalar values,
    # then receive its real flip flags. This calls the existing constructor;
    # it does not change the canonical copy boundary of adapter return values.
    for order in (("flipX", "flipY"), ("flipY", "flipX")):
        setups.append(("constructed+" + "+".join(order),
                       "    rmgTerrainTile snapshot(tile.m_terrain, tile.m_frame);\n"
                       + "".join(f"    snapshot.m_{field} = tile.m_{field};\n" for field in order)))
    return [(name, signature + setup + "    m_adapter->setTile(point, snapshot);\n}")
            for name, setup in setups]


def make_axes(header, source, terrain_source, *, refine=False):
    helper = helper_module()
    start = header.index(COPY_NOTE)
    assignment_start = header.index("    // The output-reference wrapper 0x55f350", start)
    end = header.index("};", assignment_start)
    copy, assignment = header[start:assignment_start], header[assignment_start:end]
    copies, assignments = list(copy_forms()), list(assignment_forms())
    if copy not in dict(copies).values() or assignment not in dict(assignments).values():
        raise ValueError("review the current tile special members before generating")
    axes = [helper.axis("tile_copy", "include/rmg_terrain.h", copy, copies),
            helper.axis("tile_assignment", "include/rmg_terrain.h", assignment, assignments)]
    road = helper.definition(source, "TRmgRoadLinePainter::getTile")
    river = helper.definition(source, "TRmgLinePainter::getTile")
    road_forms, river_forms = getters("TRmgRoadLinePainter"), dict(getters("TRmgLinePainter"))
    if road not in dict(road_forms).values() or river not in river_forms.values():
        raise ValueError("review the current tile getter implementations before generating")
    item = helper.axis("tile_result", "src/rmg_support.cpp", road, road_forms)
    for option in item["options"]:
        if option["name"] != "baseline":
            option["extra_edits"] = [dict(source="src/rmg_support.cpp", find=river,
                                         replace=river_forms[option["name"]])]
    axes.append(item)
    road = helper.definition(source, "TRmgRoadLinePainter::setTile")
    river = helper.definition(source, "TRmgLinePainter::setTile")
    road_forms, river_forms = setters("TRmgRoadLinePainter"), dict(setters("TRmgLinePainter"))
    if road not in dict(road_forms).values() or river not in river_forms.values():
        raise ValueError("review the current tile setter implementations before generating")
    item = helper.axis("tile_snapshot", "src/rmg_support.cpp", river, list(river_forms.items()))
    for option in item["options"]:
        if option["name"] != "baseline":
            option["extra_edits"] = [dict(source="src/rmg_support.cpp", find=road,
                                         replace=dict(road_forms)[option["name"]])]
    axes.append(item)
    bindings = [("copy", "    rmgTerrainTile tile = m_adapter->getTile(point);"),
                ("reference", "    const rmgTerrainTile& tile = m_adapter->getTile(point);"),
                ("direct", "    rmgTerrainTile tile(m_adapter->getTile(point));")]
    matches = [body for _, body in bindings if body in terrain_source]
    if len(matches) != 1:
        raise ValueError("review the packed-cell returned tile binding before generating")
    axes.append(helper.axis("packed_tile_binding", "src/rmg_terrain.cpp", matches[0], bindings))
    if refine:
        # Keep the authored source as the zero control, but concentrate the
        # population on the implicit-copy parent that recovered terrain users.
        axes[0]["options"] = [option for option in axes[0]["options"]
                              if option["name"] in ("baseline", "implicit")]
        axes[4]["options"] = axes[4]["options"][:1]
    return axes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--refine", action="store_true",
                        help="focus on implicit copies and snapshot construction")
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / "include/rmg_terrain.h").read_text(),
                     (HOMM3_DIR / "src/rmg_support.cpp").read_text(),
                     (HOMM3_DIR / "src/rmg_terrain.cpp").read_text(), refine=args.refine)
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes, evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
