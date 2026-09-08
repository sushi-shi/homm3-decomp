#!/usr/bin/env python3
"""Generate real source alternatives for the retained RMG placement cluster.

The retail footprint/outline checks are separate calls. Keep each helper,
byte query, mask test, ordered perimeter walk and trigger-entry check.
Vary genuine point/bit-index lifetimes, public STL APIs and return shapes.
The guard axes carry the preceding family's source alternatives forward,
adding mixed member/body initialization around the derived vptr store.
"""
import argparse
import importlib.util
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def guard_module():
    path = Path(__file__).with_name("generate-rmg-guard-placement-family.py")
    spec = importlib.util.spec_from_file_location("rmg_guard_family", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def replace_form(text, forms, selected, description):
    """Rebase a reviewed family from any of its previously adopted members."""
    matches = [form for form in forms.values() if form in text]
    if len(matches) != 1:
        raise ValueError(f"review the current {description} before generating")
    return text.replace(matches[0], forms[selected], 1)


def mask_bits(text, expression, indent, selected):
    declaration = " " * indent + f"unsigned int bit = {expression};\n"
    text = text.replace(declaration, "").replace(f".test({expression})", ".test(bit)")
    if selected == "independent":
        return text.replace(".test(bit)", f".test({expression})")
    start = text.index(" " * indent + "if (")
    return text[:start] + declaration + text[start:]


def outline_body(original, first_bits, step_bits, point, insertion):
    text = original
    start = text.index("    while (static_cast<unsigned int>(-position.m_x)")
    end = text.index("    if (position.m_x ==", start)
    first = mask_bits(text[start:end], "CObjectType::getBitPos(-position.m_x, 0)", 8, first_bits)
    text = text[:start] + first + text[end:]
    start = text.index("                break;\n", text.index("            if (nearby.m_x > 0")) + len("                break;\n")
    end = text.index("        } while (++attempts", start)
    step = mask_bits(text[start:end], "CObjectType::getBitPos(-nearby.m_x, -nearby.m_y)", 12, step_bits)
    text = text[:start] + step + text[end:]
    vector = "TRmgVector(g_rmgDirections[direction].m_x, g_rmgDirections[direction].m_y)"
    old_point = "            TPoint nearby = position;\n            nearby += " + vector + ";"
    forms = {
        "copy": old_point,
        "assign": "            TPoint nearby;\n            nearby = position;\n            nearby += " + vector + ";",
        "constructed": "            TPoint nearby(position.m_x + g_rmgDirections[direction].m_x,\n"
                       "                position.m_y + g_rmgDirections[direction].m_y);",
        "addition": "            TPoint nearby = position + " + vector + ";",
        "copy_offset": "            TPoint offset = g_rmgDirections[direction];\n"
                       "            TPoint nearby(position.m_x + offset.m_x, position.m_y + offset.m_y);",
        "reference_offset": "            const TPoint& offset = g_rmgDirections[direction];\n"
                            "            TPoint nearby(position.m_x + offset.m_x, position.m_y + offset.m_y);",
    }
    text = replace_form(text, forms, point, "outline point construction")
    calls = {"push": "m_outline.push_back(position);",
             "insert": "m_outline.insert(m_outline.end(), position);",
             "count": "m_outline.insert(m_outline.end(), 1, position);"}
    return replace_form(text, calls, insertion, "outline insertion")


def canonical_footprint(original):
    """Keep the mask arms verbatim while rebasing reviewed coordinate loops."""
    text = original.replace("TObjectType& prototype = *properties->m_prototype;",
                            "TObjectType* prototype = properties->m_prototype;")
    text = text.replace("prototype.", "prototype->")
    prefix_end = text.index("        return 1;\n") + len("        return 1;\n")
    start = text.index("            if (prototype->m_triggerMask.test(", prefix_end)
    end = text.rindex("            }\n") + len("            }\n")
    checks = text[start:end].replace(".test(bit)", ".test(CObjectType::getBitPos(x, y))")
    if text.count("getMapItem(") != 1 or checks.count("getBitPos(x, y)") != 2:
        raise ValueError("review footprint masks and map lookup before rebasing")
    return (text[:prefix_end]
            + "    for (unsigned int y = 0; y < prototype->getHeight(); ++y) {\n"
              "        for (unsigned int x = 0; x < prototype->getWidth(); ++x) {\n"
              "            TRmgMapItem* item = getMapItem(position.m_x - x, position.m_y - y, position.m_z);\n"
            + checks + "        }\n    }\n    return 0;\n}")


def footprint_body(original, coordinates, bits, binding):
    text = canonical_footprint(original)
    old = "            TRmgMapItem* item = getMapItem(position.m_x - x, position.m_y - y, position.m_z);"
    forms = {
        "inline": old,
        "scalars": "            int column = position.m_x - x;\n            int row = position.m_y - y;\n"
                   "            TRmgMapItem* item = getMapItem(column, row, position.m_z);",
        "point": "            TRmgMapPosition nearby(position.m_x - x, position.m_y - y, position.m_z);\n"
                 "            TRmgMapItem* item = getMapItem(nearby);",
    }
    text = replace_form(text, forms, coordinates, "footprint query")
    text = mask_bits(text, "CObjectType::getBitPos(x, y)", 12, bits)
    if binding == "reference":
        text = text.replace("TObjectType* prototype = properties->m_prototype;",
                            "TObjectType& prototype = *properties->m_prototype;")
        text = text.replace("prototype->", "prototype.")
    return text


def connected_body(original, point, returned):
    old = "        TPoint offset = outline[index % outline.size()];"
    forms = {
        "copy": old,
        "reference": "        const TPoint& offset = outline[index % outline.size()];",
        "direct": "        TPoint offset(outline[index % outline.size()]);",
    }
    text = replace_form(original, forms, point, "connected-outline point")
    old_return = "    return !blocked || foundBoundary;"
    returns = {
        "expression": old_return,
        "byte": "    unsigned char result = !blocked || foundBoundary;\n    return result;",
        "guard": "    if (blocked && !foundBoundary)\n        return 0;\n    return 1;",
        "split": "    if (!blocked)\n        return 1;\n    return foundBoundary;",
    }
    return replace_form(text, returns, returned, "connected-outline return")


def placement_body(original, coordinates, lookup, terrain, returned):
    original = original.replace("TObjectType& prototype = *properties->m_prototype;",
                                "TObjectType* prototype = properties->m_prototype;")
    original = re.sub(r"\bprototype\.", "prototype->", original)
    trigger_guard = "    if (!prototype->m_hasTrigger)\n        return 1;\n"
    start = original.index(trigger_guard) + len(trigger_guard)
    end = original.index("    if (!item->m_tileData.m_roadPassable", start)
    if coordinates == "scalars":
        setup = ("    int x = position.m_x - prototype->m_triggerCell.m_x;\n"
                 "    int y = position.m_y - prototype->m_triggerCell.m_y;\n    ++y;\n")
        x, y, z = "x", "y", "position.m_z"
        if lookup == "point":
            setup += "    TRmgMapPosition entrance(x, y, position.m_z);\n"
        point = "entrance"
    else:
        point = "position" if coordinates == "parameter" else "entrance"
        setup = "" if point == "position" else "    TRmgMapPosition entrance = position;\n"
        setup += (f"    {point}.m_x -= prototype->m_triggerCell.m_x;\n"
                  f"    {point}.m_y -= prototype->m_triggerCell.m_y;\n    ++{point}.m_y;\n")
        x, y, z = (point + ".m_" + field for field in "xyz")
    setup += f"    if ({y} >= m_mapHeight)\n        return 0;\n"
    args = point if lookup == "point" else f"{x}, {y}, {z}"
    setup += f"    TRmgMapItem* item = getMapItem({args});\n"
    tail = original[end:]
    if " terrain = item->m_tile.m_landType;" in original[start:end]:
        tail = re.sub(r"\bterrain\b", "item->m_tile.m_landType", tail)
    if "    unsigned char result = (zone->m_terrain" in tail:
        tail = tail.replace("    unsigned char result = (zone->m_terrain", "    return (zone->m_terrain")
        tail = tail.replace("\n    return result;", "")
    if terrain != "direct":
        setup += f"    {terrain} terrain = item->m_tile.m_landType;\n"
        tail = tail.replace("item->m_tile.m_landType", "terrain")
    if returned == "byte":
        line = next(line for line in tail.splitlines() if line.startswith("    return (zone->m_terrain"))
        expr = line.removeprefix("    return ")
        tail = tail.replace(line, "    unsigned char result = " + expr + "\n    return result;")
    return original[:start] + setup + tail


def make_axes(header, source):
    guard = guard_module()
    axes = guard.make_axes(header, source)[:3]
    families = [
        ("outline_walk", "TRmgObjectPropertiesRef::buildOutline", outline_body,
         (("shared", "independent"), ("shared", "independent"),
          ("copy", "assign", "constructed", "addition", "copy_offset", "reference_offset"),
          ("push", "insert", "count"))),
        ("footprint", "type_random_map::isPlacementBlocked", footprint_body,
         (("inline", "scalars", "point"), ("shared", "independent"), ("pointer", "reference"))),
        ("outline_connectivity", "type_random_map::hasConnectedOutline", connected_body,
         (("copy", "reference", "direct"), ("expression", "byte", "guard", "split"))),
        ("placement_entry", "type_random_map::canPlaceObject", placement_body,
         (("parameter", "scalars", "copy"), ("point", "scalars"),
          ("direct", "int", "TTerrainType"), ("expression", "byte"))),
    ]
    for name, method, generate, choices in families:
        original = guard.definition(source, method)
        axes.append(guard.axis(name, "src/rmg.cpp", original,
                              [("+".join(form), generate(original, *form))
                               for form in itertools.product(*choices)]))
    return axes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / "include/rmg.h").read_text(), (HOMM3_DIR / "src/rmg.cpp").read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes,
                   evidence="Retail placement cluster 0x531170/0x5318b0/0x531cf0/0x532c80: retained helper boundaries, byte queries, trigger-before-passable mask tests, circular perimeter and trigger-entry terrain checks. Vary used coordinate/mask-index locals, canonical operations, public vector APIs and boolean return structure. Guard 0x540b20 retains reverse filtering and ordered draws; mixed initialization tests its derived-vptr/store boundary. No random includes, fake operations or inline pins. All three TUs scored for each candidate.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
