#!/usr/bin/env python3
"""Polish the existing 99.6581% RMG prototype selector with 60 source hypotheses.

Retail 0x546040 retains the complete subtype/category/terrain filter and the
candidate vector through rand()%size and cleanup. Its terrain and prototype
range occupy EDI/ESI, opposite the current candidate. Earlier insertion/result
families did not settle that allocation. Vary real counter initialization,
range-binding order and terrain/index lifetimes; retain ordered push_back,
the checked bitset access, all exception boundaries and the direct result.
The --branches follow-up instead crosses equivalent filter structures with
receiver bindings and the existing public candidate-vector insertion APIs.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source, hypotheses

FUNCTION = "?selectObjectPrototype@type_random_map_generator@@QAEPAUTRmgObjectPropertiesRef@@W4TTerrainType@@HH@Z"
SIGNATURE = "TRmgObjectPropertiesRef* type_random_map_generator::selectObjectPrototype(\n    TTerrainType terrain, int objectType, int subtype)"


def lifetime_bodies():
    for binding, capture, counter in itertools.product(
            ("member", "before", "after"),
            ("parameter", "enum_before", "enum_after", "mask_index"),
            ("for", "initialized_before", "initialized_after", "declared_before", "declared_after")):
        before, after = [], []
        index = "unsigned int index = 0"
        if counter != "for":
            destination = before if counter.endswith("before") else after
            initialized = counter.startswith("initialized")
            destination.append("unsigned int index" + (" = 0;" if initialized else ";"))
            index = "" if initialized else "index = 0"
        vector = "m_objectPrototypes[objectType]"
        if binding != "member":
            (before if binding == "before" else after).append(
                "std::vector<TRmgObjectPropertiesRef*>& prototypes = " + vector + ";")
            vector = "prototypes"
        terrain, bit_index = "terrain", "terrain"
        if capture.startswith("enum"):
            (before if capture.endswith("before") else after).append("TTerrainType allowedTerrain = terrain;")
            terrain = bit_index = "allowedTerrain"
        elif capture == "mask_index":
            after.append("unsigned int terrainIndex = terrain;")
            bit_index = "terrainIndex"
        lines = before + ["std::vector<TRmgObjectPropertiesRef*> candidates;"] + after
        lines += [f"for ({index}; index < {vector}.size(); ++index) {{",
                  f"    TRmgObjectPropertiesRef* properties = {vector}[index];",
                  "    TObjectType* prototype = properties->m_prototype;",
                  "    if (prototype->m_subtype != subtype)", "        continue;",
                  "    if (prototype->m_slotCategory == TObjectType::SLOT_CATEGORY_4",
                  "        || prototype->m_slotCategory == TObjectType::SLOT_CATEGORY_5) {",
                  f"        if ({terrain} == eTerrainWater)", "            continue;",
                  f"    }} else if (!prototype->m_recommendedTerrainMask.test({bit_index})) {{",
                  "        continue;", "    }", "    candidates.push_back(properties);", "}",
                  "if (!candidates.size())", "    return 0;",
                  "return candidates[rand() % candidates.size()];"]
        yield "+".join((binding, capture, counter)), SIGNATURE + "\n{\n" + "".join(
            "    " + line + "\n" for line in lines) + "}"


def branch_bodies():
    for binding, guard, insertion in itertools.product(
            ("pointer", "const_pointer", "reference", "const_reference", "property_reference"),
            ("continue", "conditional", "split_insertion", "boolean"),
            ("push_back", "insert_value", "insert_count")):
        lines = ["std::vector<TRmgObjectPropertiesRef*> candidates;",
                 "for (unsigned int index = 0; index < m_objectPrototypes[objectType].size(); ++index) {"]
        property_type = "TRmgObjectPropertiesRef*&" if binding == "property_reference" else "TRmgObjectPropertiesRef*"
        lines.append(f"    {property_type} properties = m_objectPrototypes[objectType][index];")
        if binding in ("reference", "const_reference"):
            const = "const " if binding == "const_reference" else ""
            lines.append(f"    {const}TObjectType& prototype = *properties->m_prototype;")
            prefix = "prototype."
        else:
            const = "const " if binding == "const_pointer" else ""
            lines.append(f"    {const}TObjectType* prototype = properties->m_prototype;")
            prefix = "prototype->"
        lines += [f"    if ({prefix}m_subtype != subtype)", "        continue;"]
        category = [f"{prefix}m_slotCategory == TObjectType::SLOT_CATEGORY_4",
                    f"{prefix}m_slotCategory == TObjectType::SLOT_CATEGORY_5"]
        mask = prefix + "m_recommendedTerrainMask.test(terrain)"
        insert = {"push_back": "candidates.push_back(properties);",
                  "insert_value": "candidates.insert(candidates.end(), properties);",
                  "insert_count": "candidates.insert(candidates.end(), 1, properties);"}[insertion]
        if guard == "continue":
            lines += [f"    if ({category[0]}", f"        || {category[1]}) {{",
                      "        if (terrain == eTerrainWater)", "            continue;",
                      f"    }} else if (!{mask}) {{", "        continue;", "    }", "    " + insert]
        elif guard == "conditional":
            lines += [f"    if (({category[0]}", f"         || {category[1]})",
                      f"        ? terrain != eTerrainWater : {mask})", "        " + insert]
        elif guard == "split_insertion":
            lines += [f"    if ({category[0]}", f"        || {category[1]}) {{",
                      "        if (terrain != eTerrainWater)", "            " + insert,
                      f"    }} else if ({mask}) {{", "        " + insert, "    }"]
        else:
            lines += ["    bool accepted;", f"    if ({category[0]}", f"        || {category[1]})",
                      "        accepted = terrain != eTerrainWater;", "    else",
                      f"        accepted = {mask};", "    if (accepted)", "        " + insert]
        lines += ["}", "if (!candidates.size())", "    return 0;",
                  "return candidates[rand() % candidates.size()];"]
        yield "+".join((binding, guard, insertion)), SIGNATURE + "\n{\n" + "".join(
            "    " + line + "\n" for line in lines) + "}"


def bodies(branches=False):
    return branch_bodies() if branches else lifetime_bodies()


def make_manifest(source, branches=False):
    definitions = _source.find_definitions(source, FUNCTION)
    if len(definitions) != 1:
        raise ValueError("review the unique prototype selector definition")
    definition = definitions[0]
    start = source.rfind("\n", 0, definition.head) + 1
    original = source[start:definition.body_close + 1]
    options = list(bodies(branches))
    if original not in {body for _, body in itertools.chain(bodies(), bodies(True))}:
        raise ValueError("review the prototype selector before rebasing the family")
    if original not in {body for _, body in options}:
        options.append(("unchanged_control", original))
    options.sort(key=lambda row: row[1] != original)
    return dict(schema=1, unit="rmg", function=FUNCTION, evidence=__doc__, axes=[dict(
        name="selector_filter" if branches else "selector_lifetimes", find=original,
        options=[dict(name=name, replace=body) for name, body in options])])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--branches", action="store_true", help="test equivalent filter structure and public insertion calls")
    args = parser.parse_args()
    payload = make_manifest((HOMM3_DIR / "src/rmg.cpp").read_text(), args.branches)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    parsed = hypotheses.parse_manifest(args.output)
    print("generated", len(hypotheses.variants(parsed[4], parsed[5])), "unique source hypotheses ->", args.output)


if __name__ == "__main__":
    main()
