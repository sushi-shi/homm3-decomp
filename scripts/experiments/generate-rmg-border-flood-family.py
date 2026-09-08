#!/usr/bin/env python3
"""Generate border-bound storage and flood-worklist source alternatives.

Retail 0x540fc0 has a 0x18 frame, four signed bound selections and an exact
row-major decoration walk. Retail 0x541780 uses a LIFO position vector,
cardinal neighbours, byte predicates and retained STL copy/destruction.
Preserve those operations; vary real value lifetimes and public STL calls.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source
from homm3.vc6.source_families import load_manifest


def definition(source, name):
    found = _source.find_definitions(source, name)
    if len(found) != 1:
        raise ValueError(f"expected one definition of {name}")
    item = found[0]
    start = source.rfind("\n", 0, item.head) + 1
    return source[start:item.body_close + 1]


def axis(name, original, alternatives):
    options = [dict(name="baseline", replace=original)]
    seen = {original}
    for label, replacement in alternatives:
        if replacement not in seen:
            options.append(dict(name=label, replace=replacement))
            seen.add(replacement)
    return dict(name=name, find=original, options=options)


def border(original, storage, order):
    names = ("minimumX", "maximumX", "minimumY", "maximumY")
    values = ("max(position.m_x - 1, 0)", "min(position.m_x + 2, m_map.m_mapWidth)",
              "max(position.m_y - 1, 0)", "min(position.m_y + 2, m_map.m_mapHeight)")
    fields = ("m_minimumX", "m_maximumX", "m_minimumY", "m_maximumY")
    old_shapes = [(["    int " + name + " =" for name in names], names),
                  (["    TRmgZoneBounds bounds;"], ["bounds." + field for field in fields]),
                  (["    TPoint lower;"], ["lower.m_x", "upper.m_x", "lower.m_y", "upper.m_y"])]
    matches = [(min(original.index(anchor) for anchor in anchors), references)
               for anchors, references in old_shapes if all(anchor in original for anchor in anchors)]
    if len(matches) != 1:
        raise ValueError("review the current border bound storage before generating")
    start, old_references = matches[0]
    end = original.index("    for (int y =")
    if any(original[start:end].count(value) != 1 for value in values):
        raise ValueError("border bound expressions changed; review their retail semantics")
    tail = original[end:]
    for reference, name in zip(old_references, names):
        tail = tail.replace(reference, name)
    if storage == "rectangle":
        declarations = ["    TRmgZoneBounds bounds;"]
        references = ["bounds." + field for field in fields]
    elif storage == "corners":
        declarations = ["    TPoint lower;", "    TPoint upper;"]
        references = ["lower.m_x", "upper.m_x", "lower.m_y", "upper.m_y"]
    else:
        declarations = []
        references = names
    for name, reference in zip(names, references):
        tail = tail.replace(name, reference)
    for index in order:
        typename = "int " if storage == "scalars" else ""
        declarations.append(f"    {typename}{references[index]} = {values[index]};")
    return original[:start] + "\n".join(declarations) + "\n" + tail


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    marking = definition(source, "type_random_map_generator::markBorderObjectArea")
    flood = definition(source, "type_random_map_generator::floodConnectionRegion")
    seed_end = flood.index("    m_map.getMapItem(position)->")
    seed = flood[:seed_end]
    pop = "        position = openPositions.back();\n        openPositions.pop_back();"
    point = "            TRmgMapPosition nearby = position;\n            nearby += g_rmgDirections[direction];"
    predicate_start = flood.index("            unsigned char visited =")
    predicate_end = flood.index("            item->m_tileData.m_connectionVisited = 1;")
    predicate = flood[predicate_start:predicate_end]
    push_forms = [
        ("push_back", "                openPositions.push_back(nearby);"),
        ("insert_value", "                openPositions.insert(openPositions.end(), nearby);"),
        ("insert_count", "                openPositions.insert(openPositions.end(), 1, nearby);"),
    ]
    current_push = [text for _, text in push_forms if text in flood]
    if len(current_push) != 1:
        raise ValueError("review the current flood enqueue before generating")
    push = current_push[0]
    named = ("            unsigned char visited = item->m_tileData.m_connectionVisited;\n"
             "            if (visited)\n                continue;\n")
    axes = [
        axis("border_bounds", marking,
             ((storage + "+" + "".join(map(str, order)), border(marking, storage, order))
              for storage in ("scalars", "corners", "rectangle")
              for order in itertools.permutations(range(4)))),
        axis("flood_seed", seed, [
            ("insert_value", seed.replace("openPositions.push_back(position)",
                                          "openPositions.insert(openPositions.end(), position)")),
            ("insert_count", seed.replace("openPositions.push_back(position)",
                                          "openPositions.insert(openPositions.end(), 1, position)")),
        ]),
        axis("flood_pop", pop, [
            ("erase_last", "        position = openPositions.back();\n"
             "        openPositions.erase(openPositions.end() - 1);"),
            ("erase_range", "        position = openPositions.back();\n"
             "        openPositions.erase(openPositions.end() - 1, openPositions.end());"),
        ]),
        axis("flood_point", point, [
            ("direct_copy", "            TRmgMapPosition nearby(position);\n"
             "            nearby += g_rmgDirections[direction];"),
            ("assigned", "            TRmgMapPosition nearby;\n            nearby = position;\n"
             "            nearby += g_rmgDirections[direction];"),
        ]),
        axis("flood_predicates", predicate, [
            ("short_circuit", named + "            if (!item->hasSubterraneanGate()\n"
             "                && static_cast<unsigned char>(item->m_tile.m_landType) == eTerrainWater)\n"
             "                continue;\n"),
            ("named_gate", named + "            unsigned char gate = item->hasSubterraneanGate();\n"
             "            if (!gate) {\n                unsigned char terrain = item->m_tile.m_landType;\n"
             "                if (terrain == eTerrainWater)\n                    continue;\n            }\n"),
        ]),
        axis("flood_push", push, push_forms),
    ]
    axes[3]["options"].append(dict(name="loop_reuse",
        replace="            nearby = position;\n            nearby += g_rmgDirections[direction];",
        extra_edits=[dict(insert_after="    m_map.getMapItem(position)->m_tileData.m_connectionVisited = 1;",
                         text="\n    TRmgMapPosition nearby;")]))
    payload = dict(schema=1, source="src/rmg.cpp", units=["rmg", "rmg_support", "rmg_terrain"], axes=axes,
                   evidence="Retail 0x540fc0 matches the initial body's 25 blocks/instructions but needs a 0x18 frame: one four-field bounds value plus min/max operand homes. Preserve its row-major walk, packed-field semantics and predecessor cleanup. Retail 0x541780 retains vector copy/_Destroy at removal; source alternatives preserve LIFO order, cardinal point translation, byte predicates and water-only enqueue. No library body is pasted or pinned.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
