#!/usr/bin/env python3
"""Generate real copy/cursor/placement alternatives from the current RMG source.

Retail 0x4f9be0 keeps the allocation/throw and pair/count loops, but its
expanded copy walks one source pointer plus a destination displacement.
0x5318b0 retains ascending mask indices beside descending map coordinates.
0x531cf0 returns the connectivity call's false byte without clearing AL,
and reads terrain only after the passability guard. Preserve those semantic
operations and canonical helper boundaries; vary their real source lifetimes.
"""
import argparse
import importlib.util
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def outline_module():
    spec = importlib.util.spec_from_file_location(
        "rmg_outline_family", Path(__file__).with_name("generate-rmg-outline-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def copy_variants():
    yield "direct", "    std::copy(patterns, patterns + m_patternCount, m_patterns);\n"
    fields = {
        "first": "const int* first = patterns;",
        "last": "const int* last = patterns + m_patternCount;",
        "output": "int* output = m_patterns;",
    }
    defaults = {"first": "patterns", "last": "patterns + m_patternCount", "output": "m_patterns"}
    for count in (1, 2, 3):
        for order in itertools.permutations(fields, count):
            for scoped in (False, True):
                indent = "        " if scoped else "    "
                setup = "".join(indent + fields[name] + "\n" for name in order)
                arguments = ", ".join(name if name in order else defaults[name] for name in fields)
                body = setup + indent + "std::copy(" + arguments + ");\n"
                if scoped:
                    body = "    {\n" + body + "    }\n"
                yield "+".join(order) + ("+block" if scoped else "+function"), body


def pattern_variants(original):
    original = original.replace("    int* allocated = new int[m_patternCount];\n"
                                "    m_patterns = allocated;\n    if (!allocated)\n",
                                "    m_patterns = new int[m_patternCount];\n    if (!m_patterns)\n")
    thrown = "        throw TAllocationFailure();\n"
    start = original.index(thrown) + len(thrown)
    end = original.index("    for (unsigned int value", start)
    prefix, tail = original[:start], original[end:]
    for (copy_name, copied), allocated in itertools.product(copy_variants(), ("member", "local")):
        head = prefix
        if allocated == "local":
            old = "    m_patterns = new int[m_patternCount];\n    if (!m_patterns)\n"
            new = ("    int* allocated = new int[m_patternCount];\n"
                   "    m_patterns = allocated;\n    if (!allocated)\n")
            if old not in head:
                raise ValueError("review pattern allocation before generating")
            head = head.replace(old, new)
        yield copy_name + "+" + allocated, head + copied + tail


def footprint_variants(original):
    previous = outline_module()
    for storage, order, binding, counters in itertools.product(
            ("point3", "point2", "parameter", "row_point2"),
            ("index_first", "coordinate_first"), ("reference", "pointer"), ("loop", "outer")):
        body = previous.footprint_body(original, "inline", "independent", binding)
        access = "." if binding == "reference" else "->"
        start = body.index("    for (unsigned int y")
        check_start = body.index("            if (prototype", start)
        checks = body[check_start:body.rindex("            }\n") + len("            }\n")]
        if storage == "point3":
            setup = "    TRmgMapPosition nearby = position;\n"
            column = "        nearby.m_x = position.m_x;\n"
            row_dec, col_dec = "--nearby.m_y", "--nearby.m_x"
            query = "getMapItem(nearby)"
        elif storage == "point2":
            setup = "    TPoint nearby(position.m_x, position.m_y);\n"
            column = "        nearby.m_x = position.m_x;\n"
            row_dec, col_dec = "--nearby.m_y", "--nearby.m_x"
            query = "getMapItem(nearby.m_x, nearby.m_y, position.m_z)"
        elif storage == "parameter":
            setup = "    int originX = position.m_x;\n"
            column = "        position.m_x = originX;\n"
            row_dec, col_dec = "--position.m_y", "--position.m_x"
            query = "getMapItem(position)"
        else:
            setup = "    int row = position.m_y;\n"
            column = "        TPoint nearby(position.m_x, row);\n"
            row_dec, col_dec = "--row", "--nearby.m_x"
            query = "getMapItem(nearby.m_x, nearby.m_y, position.m_z)"
        row_step, col_step = "++y, " + row_dec, "++x, " + col_dec
        if order == "coordinate_first":
            row_step, col_step = row_dec + ", ++y", col_dec + ", ++x"
        declared = "unsigned int " if counters == "loop" else ""
        if counters == "outer":
            setup = "    unsigned int x;\n    unsigned int y;\n" + setup
        loops = (setup + f"    for ({declared}y = 0; y < prototype{access}getHeight(); {row_step}) {{\n"
                 + column + f"        for ({declared}x = 0; x < prototype{access}getWidth(); {col_step}) {{\n"
                 + "            TRmgMapItem* item = " + query + ";\n"
                 + checks + "        }\n    }\n    return 0;\n}")
        yield "+".join((storage, order, binding, counters)), body[:start] + loops


def placement_variants(original):
    previous = outline_module()
    original = original.replace("TObjectType& prototype = *properties->m_prototype;",
                                "TObjectType* prototype = properties->m_prototype;")
    original = re.sub(r"\bprototype\.", "prototype->", original)
    for kind in ("int", "TTerrainType"):
        declared = f"    {kind} terrain = item->m_tile.m_landType;\n"
        if declared in original:
            original = original.replace(declared, "")
            original = re.sub(r"\bterrain\b", "item->m_tile.m_landType", original)
    original = original.replace(
        "    if (!item->m_tileData.m_roadPassable)\n        return 0;\n"
        "    if (item->m_tile.m_landType == eTerrainRock)\n        return 0;\n",
        "    if (!item->m_tileData.m_roadPassable || item->m_tile.m_landType == eTerrainRock)\n"
        "        return 0;\n")
    after_outline = "    properties->buildOutline();\n"
    start = original.index(after_outline) + len(after_outline)
    end = original.index("    if (!prototype->m_hasTrigger)", start)
    call_start = original.index("hasConnectedOutline(", start)
    call_end = original.index("zone, 0)", call_start) + len("zone, 0)")
    original = (original[:start] + "    if (!" + original[call_start:call_end]
                + ")\n        return 0;\n" + original[end:])
    canonical = previous.placement_body(original, "scalars", "point", "direct", "byte")
    call_start = canonical.index("    if (!hasConnectedOutline(")
    call_end = canonical.index("    if (!prototype->m_hasTrigger)", call_start)
    guard = canonical[call_start:call_end]
    invocation = guard.removeprefix("    if (!").removesuffix(")\n        return 0;\n")
    if not invocation.endswith("zone, 0)"):
        raise ValueError("review placement connectivity call")
    terrain_guard = ("    if (!item->m_tileData.m_roadPassable || item->m_tile.m_landType == eTerrainRock)\n"
                     "        return 0;\n")
    for connected, terrain, prototype in itertools.product(
            ("guard", "return_byte", "test_byte", "return_bool"),
            ("combined", "split", "int", "TTerrainType"), ("pointer", "reference")):
        body = canonical
        if connected != "guard":
            kind = "bool" if connected == "return_bool" else "unsigned char"
            called = f"    {kind} connected = {invocation};\n    if (!connected)\n"
            called += "        return 0;\n" if connected == "test_byte" else "        return connected;\n"
            body = body.replace(guard, called)
        if terrain != "combined":
            start = body.index(terrain_guard)
            tail = body[start + len(terrain_guard):]
            check = "    if (!item->m_tileData.m_roadPassable)\n        return 0;\n"
            value = "item->m_tile.m_landType"
            if terrain != "split":
                check += f"    {terrain} terrain = {value};\n"
                tail = tail.replace(value, "terrain")
                value = "terrain"
            check += f"    if ({value} == eTerrainRock)\n        return 0;\n"
            body = body[:start] + check + tail
        if prototype == "reference":
            body = body.replace("TObjectType* prototype = properties->m_prototype;",
                                "TObjectType& prototype = *properties->m_prototype;")
            body = re.sub(r"\bprototype->", "prototype.", body)
        yield "+".join((connected, terrain, prototype)), body


def make_axes(source, support):
    helper = outline_module().guard_module()
    axes = []
    for name, path, text, method, generate in (
            ("pattern_copy", "src/rmg_support.cpp", support,
             "TRmgLinePatternTable::TRmgLinePatternTable", pattern_variants),
            ("footprint_cursor", "src/rmg.cpp", source,
             "type_random_map::isPlacementBlocked", footprint_variants),
            ("placement_guards", "src/rmg.cpp", source,
             "type_random_map::canPlaceObject", placement_variants)):
        original = helper.definition(text, method)
        axes.append(helper.axis(name, path, original, generate(original)))
    return axes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / "src/rmg.cpp").read_text(),
                     (HOMM3_DIR / "src/rmg_support.cpp").read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes,
                   evidence=__doc__ + " All configured functions in all three TUs are scored.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
