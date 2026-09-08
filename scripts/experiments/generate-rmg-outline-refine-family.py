#!/usr/bin/env python3
"""Refine retained placement helpers using retail-supported source lifetimes.

0x532c80: size/zero setup, direction copies, translation and point comparison.
0x531170: independent predicate initialization and previous-state lifetime.
0x5318b0: descending map coordinates beside ascending mask coordinates.
0x531cf0: entry-time coordinates retained across the three helper calls.
No added work, alternate declarations, helper flattening or inline controls.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def previous_module():
    spec = importlib.util.spec_from_file_location(
        "rmg_outline_family", Path(__file__).with_name("generate-rmg-outline-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def known_span(source, alternatives, description):
    matches = {body for _, body in alternatives if body in source}
    if len(matches) != 1:
        raise ValueError(f"review the current {description} before generating")
    return matches.pop()


def outline_entries():
    test = "static_cast<unsigned int>(-position.m_x) < m_prototype->getWidth()"
    body = ("        if (!m_prototype->m_passableMask.test(CObjectType::getBitPos(-position.m_x, 0))"
            " || m_prototype->m_triggerMask.test(CObjectType::getBitPos(-position.m_x, 0)))\n"
            "            break;\n")
    initializations = {
        "constructed_while": "    TPoint position(0, 0);\n",
        "xy_while": "    TPoint position;\n    position.m_x = 0;\n    position.m_y = 0;\n",
        "yx_while": "    TPoint position;\n    position.m_y = 0;\n    position.m_x = 0;\n",
        "constructed_for": "    TPoint position(0, 0);\n",
        "y_for_x": "    TPoint position;\n    position.m_y = 0;\n",
    }
    result = []
    for size, condition in (("truth", "m_outline.size()"), ("positive", "m_outline.size() > 0"),
                            ("empty", "!m_outline.empty()")):
        for name, setup in initializations.items():
            if name.endswith("while"):
                loop = f"    while ({test}) {{\n" + body + "        --position.m_x;\n    }\n"
            else:
                initial = "position.m_x = 0" if name == "y_for_x" else ""
                loop = f"    for ({initial}; {test}; --position.m_x) {{\n" + body + "    }\n"
            result.append((size + "+" + name, f"    if ({condition})\n        return;\n" + setup + loop))
    return result


def outline_nearby():
    x, y = (f"g_rmgDirections[direction].m_{field}" for field in "xy")
    vector = f"TRmgVector({x}, {y})"
    return [
        ("constructed", f"            TPoint nearby(position.m_x + {x},\n                position.m_y + {y});"),
        ("copy_offset", "            TPoint offset = g_rmgDirections[direction];\n"
         "            TPoint nearby(position.m_x + offset.m_x, position.m_y + offset.m_y);"),
        ("reference_offset", "            const TPoint& offset = g_rmgDirections[direction];\n"
         "            TPoint nearby(position.m_x + offset.m_x, position.m_y + offset.m_y);"),
        ("copy_position", "            TPoint nearby = position;\n            nearby += " + vector + ";"),
        ("copy_direction", "            TPoint nearby = g_rmgDirections[direction];\n"
         "            nearby += TRmgVector(position.m_x, position.m_y);"),
        ("addition", "            TPoint nearby = position + " + vector + ";"),
    ]


def outline_translation():
    x, y = (f"g_rmgDirections[direction].m_{field}" for field in "xy")
    vector = f"TRmgVector({x}, {y})"
    return [
        ("compound", "        position += " + vector + ";"),
        ("constructed", f"        position = TPoint(position.m_x + {x}, position.m_y + {y});"),
        ("addition", "        position = position + " + vector + ";"),
        ("copy_offset", "        TPoint offset = g_rmgDirections[direction];\n"
         "        position = TPoint(position.m_x + offset.m_x, position.m_y + offset.m_y);"),
    ]


def connected_variants(original):
    begin = original.index("\n{\n") + 3
    end = original.index("    for (unsigned int index", begin)
    prefix = original[:begin]
    tail = original[end:]
    previous = "        unsigned char previouslyBlocked = blocked;"
    tail = tail.replace("        previouslyBlocked = blocked;", previous)
    if previous not in tail:
        raise ValueError("review connected-outline previous-state lifetime")
    # Each initializer reads independent state and there are no intervening calls.
    lines = ["    unsigned char waterZone = zone->m_terrain == eTerrainWater;\n",
             "    int zoneIndex = zone->m_slot->m_zoneIndex;\n",
             "    unsigned char blocked = 1;\n", "    unsigned char foundBoundary = 0;\n"]
    for order in itertools.permutations(range(4)):
        for lifetime in ("iteration", "function"):
            body = prefix + "".join(lines[index] for index in order)
            rest = tail
            if lifetime == "function":
                body += "    unsigned char previouslyBlocked;\n"
                rest = rest.replace(previous, "        previouslyBlocked = blocked;")
            yield "".join(str(index) for index in order) + "+" + lifetime, body + rest


def footprint_variants(original):
    previous = previous_module()
    for binding, coordinates, scope, increment in itertools.product(
            ("pointer", "reference"), ("scalars", "point"), ("outer", "inner"), ("comma", "body")):
        body = previous.footprint_body(original, "inline", "independent", binding)
        access = "->" if binding == "pointer" else "."
        outer = f"    for (unsigned int y = 0; y < prototype{access}getHeight(); ++y) {{\n"
        inner = f"        for (unsigned int x = 0; x < prototype{access}getWidth(); ++x) {{\n"
        query = "            TRmgMapItem* item = getMapItem(position.m_x - x, position.m_y - y, position.m_z);"
        if coordinates == "scalars":
            setup = "    int row = position.m_y;\n"
            if scope == "outer":
                setup += "    int column;\n"
            column = "        " + ("int " if scope == "inner" else "") + "column = position.m_x;\n"
            row_dec, col_dec = "--row", "--column"
            lookup = "getMapItem(column, row, position.m_z)"
        else:
            setup = "    TRmgMapPosition nearby = position;\n"
            column = "        nearby.m_x = position.m_x;\n"
            if scope == "inner":
                # The row loop owns the point while its y coordinate is explicit.
                setup = "    int row = position.m_y;\n"
                column = "        TRmgMapPosition nearby(position.m_x, row, position.m_z);\n"
                row_dec = "--row"
            else:
                row_dec = "--nearby.m_y"
            col_dec = "--nearby.m_x"
            lookup = "getMapItem(nearby)"
        new_outer, new_inner = outer, inner
        if increment == "comma":
            new_outer = outer.replace("++y)", f"++y, {row_dec})")
            new_inner = inner.replace("++x)", f"++x, {col_dec})")
        else:
            closing = "        }\n    }\n    return 0;"
            if closing not in body:
                raise ValueError("review footprint loop tails")
            body = body.replace(closing, f"            {col_dec};\n        }}\n        {row_dec};\n    }}\n    return 0;")
        body = body.replace(outer, setup + new_outer + column).replace(inner, new_inner)
        body = body.replace(query, "            TRmgMapItem* item = " + lookup + ";")
        yield "+".join((binding, coordinates, scope, increment)), body


def placement_variants(original):
    previous = previous_module()
    for field in "xy":
        original = original.replace(f"    int {field} = position.m_{field};\n", "")
    for order, terrain, result, lookup in itertools.product(
            ("xy_first", "yx_first", "xy_after_prototype", "yx_after_prototype"),
            ("direct", "int", "TTerrainType"), ("expression", "byte"), ("point", "scalars")):
        body = previous.placement_body(original, "scalars", lookup, terrain, result)
        old = ("    int x = position.m_x - prototype->m_triggerCell.m_x;\n"
               "    int y = position.m_y - prototype->m_triggerCell.m_y;\n")
        body = body.replace(old, "    x -= prototype->m_triggerCell.m_x;\n"
                                 "    y -= prototype->m_triggerCell.m_y;\n")
        declarations = "".join(f"    int {field} = position.m_{field};\n" for field in order[:2])
        if order.endswith("first"):
            body = body.replace("{\n", "{\n" + declarations, 1)
        else:
            prototype = "    TObjectType* prototype = properties->m_prototype;\n"
            body = body.replace(prototype, prototype + declarations, 1)
        yield "+".join((order, terrain, result, lookup)), body


def make_axes(source):
    helper = previous_module().guard_module()
    axes = []
    outline = helper.definition(source, "TRmgObjectPropertiesRef::buildOutline")
    for name, alternatives in (
            ("outline_entry", outline_entries()), ("outline_nearby", outline_nearby()),
            ("outline_translation", outline_translation()),
            ("outline_closure", [("position_first", "    } while (position != start);"),
                                 ("start_first", "    } while (start != position);")])):
        original = known_span(outline, alternatives, name)
        axes.append(helper.axis(name, "src/rmg.cpp", original, alternatives))
    for name, method, generate in (
            ("connected_storage", "hasConnectedOutline", connected_variants),
            ("footprint_coordinates", "isPlacementBlocked", footprint_variants),
            ("placement_lifetime", "canPlaceObject", placement_variants)):
        original = helper.definition(source, "type_random_map::" + method)
        axes.append(helper.axis(name, "src/rmg.cpp", original, generate(original)))
    return axes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / "src/rmg.cpp").read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes,
                   evidence=__doc__ + " All configured functions in all three TUs are scored.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
