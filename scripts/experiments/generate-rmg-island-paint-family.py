#!/usr/bin/env python3
"""Island-paint coordinate and extent ownership at retail 0x53efa0.

All calls and branch destinations already agree. Retail keeps both paint
coordinates below the brush, recycles the bounds argument for the mask,
and preserves height in a local; the reconstruction instead reuses the
argument for height and then y. Its frame is eight bytes smaller. Test real
coordinate lifetimes, dimension ownership and allocation products without
changing the canonical borrowed-map constructor or brush lifetime boundary.
No Dreamcast counterpart is mapped. Dimensions are bounded and nonnegative.
"""
import argparse
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

NAME = "type_random_map_generator::createWaterZoneIsland"


def shared_point_definition(source):
    original = generator("generate-rmg-position-family.py").definition(source, NAME)
    return original.replace("    TRmgMapPosition point;", "    TPoint point;").replace(
        "    point.m_z = level;\n", "").replace("m_map.getMapItem(point.m_x, point.m_y, point.m_z)",
        "m_map.getMapItem(point.m_x, point.m_y, level)").replace("m_map.getMapItem(point)",
        "m_map.getMapItem(point.m_x, point.m_y, level)")


def baseline_definition(source):
    original = generator("generate-rmg-position-family.py").definition(source, NAME)
    shared = shared_point_definition(source)
    baseline = shared.replace("    TPoint point;\n", "")
    baseline = baseline.replace("for (point.m_y =", "for (int y =").replace("for (point.m_x =", "for (int x =")
    baseline = baseline.replace("point.m_x", "x").replace("point.m_y", "y")
    baseline = baseline.replace("""    int width = bounds.m_maximumX - bounds.m_minimumX;
    int height = bounds.m_maximumY - bounds.m_minimumY;""", """    int height = bounds.m_maximumY - bounds.m_minimumY;
    int width = bounds.m_maximumX - bounds.m_minimumX;""")
    if original not in {body for _, body in variants(baseline)} and original not in {
            body for _, body in level_variants(shared)}:
        raise ValueError("review changed island-paint body")
    return baseline


def variants(original):
    dimensions = """    int height = bounds.m_maximumY - bounds.m_minimumY;
    int width = bounds.m_maximumX - bounds.m_minimumX;"""
    dimension_forms = [dimensions,
        """    int width = bounds.m_maximumX - bounds.m_minimumX;
    int height = bounds.m_maximumY - bounds.m_minimumY;""",
        """    TPoint extent;
    extent.m_y = bounds.m_maximumY - bounds.m_minimumY;
    extent.m_x = bounds.m_maximumX - bounds.m_minimumX;""",
        dimensions.replace("    int ", "    const int ")]
    start = original.index("        for (int y = bounds.m_minimumY;")
    end = original.index("    delete[] mask;", start)
    first_end = original.index("    }\n    for (int y =", start) + len("    }\n")
    if original.count(dimensions) != 1:
        raise ValueError("review changed island dimensions")
    for dims, scan, product in itertools.product(range(4), range(5), range(3)):
        first = original[start:first_end]
        second = original[first_end:end]
        before = original[:start]
        if scan in (1, 2, 3):
            first = first.replace("for (int y =", "for (point.m_y =").replace("for (int x =", "for (point.m_x =")
            first = re.sub(r"(?<![\w.])y\b", "point.m_y", first)
            first = re.sub(r"(?<![\w.])x\b", "point.m_x", first)
            declaration = "        TPoint point;\n"
            if scan == 1:
                before += declaration
            elif scan == 2:
                before = before.replace("        type_random_map map(", declaration + "        type_random_map map(")
            else:
                before = before.replace("    int terrain =", "    TPoint point;\n    int terrain =")
                second = second.replace("for (int y =", "for (point.m_y =").replace("for (int x =", "for (point.m_x =")
                second = re.sub(r"(?<![\w.])y\b", "point.m_y", second)
                second = re.sub(r"(?<![\w.])x\b", "point.m_x", second)
        elif scan == 4:
            before = before.replace("        type_random_map map(", "        int x;\n        int y;\n        type_random_map map(")
            first = first.replace("for (int y =", "for (y =").replace("for (int x =", "for (x =")
        body = (before + first + second + original[end:]).replace(dimensions, dimension_forms[dims])
        if product == 1:
            body = body.replace("new unsigned char[width * height]", "new unsigned char[height * width]")
        elif product == 2:
            body = body.replace("    unsigned char* mask = new unsigned char[width * height];",
                "    int area = height * width;\n    unsigned char* mask = new unsigned char[area];")
        if dims == 2:
            body = re.sub(r"\bwidth\b", "extent.m_x", body)
            body = re.sub(r"\bheight\b", "extent.m_y", body)
        yield f"dimensions_{dims}+scan_{scan}+product_{product}", body


def level_variants(original):
    view = """        type_random_map map(m_map.getMapItem(0, 0, level),
            m_map.m_mapWidth, m_map.m_mapHeight);"""
    if original.count("    TPoint point;\n") != 1 or original.count(view) != 1:
        raise ValueError("level family requires the shared-point island body")
    views = [view,
        """        int mapHeight = m_map.m_mapHeight;
        type_random_map map(m_map.getMapItem(0, 0, level),
            m_map.m_mapWidth, mapHeight);""",
        """        int mapWidth = m_map.m_mapWidth;
        type_random_map map(m_map.getMapItem(0, 0, level),
            mapWidth, m_map.m_mapHeight);""",
        """        TPoint mapSize;
        mapSize.m_y = m_map.m_mapHeight;
        mapSize.m_x = m_map.m_mapWidth;
        type_random_map map(m_map.getMapItem(0, 0, level),
            mapSize.m_x, mapSize.m_y);"""]
    for coordinate, binding, lifetime in itertools.product(range(5), range(4), range(3)):
        body = original.replace(view, views[binding])
        declaration = "    TPoint point;\n" if coordinate == 0 else "    TRmgMapPosition point;\n"
        if coordinate in (1, 3):
            declaration += "    point.m_z = level;\n"
        body = body.replace("    TPoint point;\n", "")
        anchor = ("    int terrain =", "    unsigned char* mask =", "    int width =")[lifetime]
        body = body.replace(anchor, declaration + anchor)
        if coordinate in (2, 4):
            anchor = "\n    for (point.m_y = bounds.m_minimumY;"
            if body.count(anchor) != 1:
                raise ValueError("review changed post-brush tagging scope")
            body = body.replace(anchor, "\n    point.m_z = level;" + anchor)
        if coordinate:
            call = "m_map.getMapItem(point.m_x, point.m_y, level)"
            body = body.replace(call, "m_map.getMapItem(point)" if coordinate >= 3 else
                "m_map.getMapItem(point.m_x, point.m_y, point.m_z)")
        yield f"level_coordinate_{coordinate}+view_binding_{binding}+point_lifetime_{lifetime}", body


def origin_variants(original):
    view = """        type_random_map map(m_map.getMapItem(0, 0, level),
            m_map.m_mapWidth, m_map.m_mapHeight);"""
    if original.count(view) != 1 or original.count("    TRmgMapPosition point;") != 1:
        raise ValueError("origin family requires the level-carrying coordinate")
    bindings = [None, ("const int&", "const int&"), ("const int&", "int"),
                ("int", "const int&"), ("const int", "const int")]
    for origin, binding, initialization in itertools.product(range(4), range(5), range(3)):
        if origin == 0 and initialization:
            continue
        setup = ""
        body = original
        lookup = "m_map.getMapItem(0, 0, level)"
        if origin:
            name = "origin" if origin == 3 else "point"
            if origin == 3:
                setup += "        TRmgMapPosition origin;\n"
            else:
                body = body.replace("\n    point.m_z = level;\n", "\n")
            stores = [f"        {name}.m_x = 0;", f"        {name}.m_y = 0;", f"        {name}.m_z = level;"]
            if initialization == 1:
                stores = stores[2:] + stores[:2]
            elif initialization == 2:
                stores = [f"        {name}.m_x = {name}.m_y = 0;", stores[2]]
            setup += "\n".join(stores) + "\n"
            lookup = f"m_map.getMapItem({name}.m_x, {name}.m_y, {name}.m_z)" if origin == 1 else f"m_map.getMapItem({name})"
        width, height = "m_map.m_mapWidth", "m_map.m_mapHeight"
        if binding:
            wtype, htype = bindings[binding]
            setup += f"        {htype} mapHeight = m_map.m_mapHeight;\n        {wtype} mapWidth = m_map.m_mapWidth;\n"
            width, height = "mapWidth", "mapHeight"
        replacement = setup + f"        type_random_map map({lookup},\n            {width}, {height});"
        body = body.replace(view, replacement)
        yield f"origin_{origin}+dimension_binding_{binding}+initialization_{initialization}", body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--levels", action="store_true", help="test level-carrying coordinates and borrowed-view dimension bindings")
    mode.add_argument("--origins", action="store_true", help="test canonical plane-origin lookups and dimension reference/value bindings")
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, NAME)
    if args.origins:
        alternatives = origin_variants(original)
    else:
        alternatives = level_variants(shared_point_definition(source)) if args.levels else variants(baseline_definition(source))
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[
        helper.axis("island_paint", "src/rmg.cpp", original, alternatives)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "island-paint states")


if __name__ == "__main__":
    main()
