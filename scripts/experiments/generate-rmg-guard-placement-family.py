#!/usr/bin/env python3
"""Generate guard filtering/serialization and zone-placement source families.

Retail 0x540b20 has reverse trait/selection walks, ordered random draws and
an expanded canonical base constructor. 0x5331f0 narrows the serialized
count. 0x542930 adjusts height before width and copies the selected position.
Keep those operations and helper boundaries; vary their real source homes.
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
    return source[source.rfind("\n", 0, item.head) + 1:item.body_close + 1]


def axis(name, source, original, alternatives):
    options = [dict(name="baseline", replace=original)]
    seen = {original}
    for label, replacement in alternatives:
        if replacement not in seen:
            options.append(dict(name=label, replace=replacement))
            seen.add(replacement)
    return dict(name=name, source=source, find=original, options=options)


def guard_loop(limit_order, condition):
    limit = "        creatureLimit = RMG_GUARD_ROE_CREATURE_LIMIT;\n"
    exclusion = ("        for (int creature = RMG_GUARD_CREATURE_COUNT - 1;\n"
                 "             creature >= RMG_GUARD_ROE_EXCLUDED_FIRST; --creature)\n"
                 "            prototypeIndices[creature] = -1;\n")
    loop = ("    for (int creature = creatureLimit - 1; creature >= 0; --creature) {\n"
            if condition == "separate" else
            "    for (int creature = creatureLimit; --creature >= 0;) {\n")
    return ("    if (m_mapVersion < RMG_MAP_ARMAGEDDONS_BLADE) {\n"
            + (limit + exclusion if limit_order == "before" else exclusion + limit)
            + "    }\n" + loop)


def guard_quantity(storage, jitter):
    count = "count" if storage == "local" else "value"
    declare = "int " if storage == "local" else ""
    lines = ["    int aiValue = g_creatureTypeTraits[creature].m_aiValue;",
             f"    {declare}{count} = (value + aiValue / 2) / aiValue;",
             f"    int variation = {count} / 4 + 1;", "    if (variation > 1) {"]
    if jitter == "pair":
        lines += ["        int increase = rand() % variation;",
                  "        int decrease = rand() % variation;",
                  f"        {count} += increase - decrease;"]
    elif jitter == "delta":
        lines += ["        int delta = rand() % variation;",
                  "        delta -= rand() % variation;", f"        {count} += delta;"]
    else:
        lines += ["        int increase = rand() % variation;",
                  "        int decrease = rand() % variation;",
                  "        int delta = increase - decrease;", f"        {count} += delta;"]
    lines += ["    }", f"    return new rmgMonsterObject(properties, m_nextObjectId++, {count});"]
    return "\n".join(lines) + "\n"


def constructors():
    signature = "    rmgMonsterObject(TRmgObjectPropertiesRef* properties, int objectId, int count)\n"
    forms = [("initializers", signature
              + "        : type_object(properties), m_objectId(objectId), m_count(count),\n"
                "          m_disposition(RMG_GUARD_DISPOSITION) {}\n")]
    fields = {"id": "m_objectId = objectId;", "count": "m_count = count;",
              "disposition": "m_disposition = RMG_GUARD_DISPOSITION;"}
    for order in itertools.permutations(fields):
        forms.append(("body_" + "_".join(order), signature
                      + "        : type_object(properties)\n    {\n"
                      + "".join("        " + fields[name] + "\n" for name in order) + "    }\n"))
    # Mixed initialization is observable around the derived vptr store.
    # Keep initializer declaration order; permute only independent body stores.
    initializers = {"id": "m_objectId(objectId)", "count": "m_count(count)",
                    "disposition": "m_disposition(RMG_GUARD_DISPOSITION)"}
    for size in (1, 2):
        for initialized in itertools.combinations(fields, size):
            for order in itertools.permutations(name for name in fields if name not in initialized):
                forms.append(("init_" + "_".join(initialized) + "+body_" + "_".join(order),
                              signature + "        : type_object(properties), "
                              + ", ".join(initializers[name] for name in initialized)
                              + "\n    {\n"
                              + "".join("        " + fields[name] + "\n" for name in order) + "    }\n"))
    return forms


def writer_count(kind):
    declarations = {
        "int": "int intBuffer = m_count;",
        "short": "short intBuffer = m_count;",
        "unsigned_short": "unsigned short intBuffer = m_count;",
        "narrowed_int": "int intBuffer = static_cast<short>(m_count);",
    }
    return ("    {\n        " + declarations[kind]
            + "\n        outfile->write(&intBuffer, sizeof(short));\n    }\n")


def placement(original, order, construction, selection, insertion):
    # Keep the interface and all domain predicates from the reviewed body.
    start = original.index("    TRmgZoneBounds bounds =")
    loop = original.index("    for (position.m_y", start)
    checks_end = original.index("    if (!candidates.size())", loop)
    bounds = ("    TRmgZoneBounds bounds = zone->m_bounds;\n"
              "    int zoneIndex = zone->m_slot->m_zoneIndex;\n")
    updates = {
        "x": "    bounds.m_minimumX += prototype->getWidth() - 1;\n",
        "y": "    bounds.m_minimumY += prototype->getHeight() - 1;\n",
    }
    copies = {
        "copy": "    TRmgMapPosition position = zone->m_levelPosition;\n",
        "direct": "    TRmgMapPosition position(zone->m_levelPosition);\n",
        "assign": "    TRmgMapPosition position;\n    position = zone->m_levelPosition;\n",
        "coordinates": "    TRmgMapPosition position(zone->m_levelPosition.m_x,\n"
                       "        zone->m_levelPosition.m_y, zone->m_levelPosition.m_z);\n",
    }
    calls = {"push": "candidates.push_back(position);",
             "insert": "candidates.insert(candidates.end(), position);",
             "count": "candidates.insert(candidates.end(), 1, position);"}
    walk = original[loop:checks_end]
    # Rebase any already retained public-API alternative.
    old_call = [call for call in calls.values() if call in walk]
    if len(old_call) != 1:
        raise ValueError("review the placement candidate insertion before generating")
    walk = walk.replace(old_call[0], calls[insertion])
    selected = {
        "assign": "    position = candidates[rand() % candidates.size()];\n    addObject(object, position);\n",
        "copy": "    TRmgMapPosition selected = candidates[rand() % candidates.size()];\n"
                "    addObject(object, selected);\n",
        "direct": "    addObject(object, candidates[rand() % candidates.size()]);\n",
    }
    return (original[:start] + bounds + "".join(updates[field] for field in order)
            + copies[construction] + walk + "    if (!candidates.size())\n        return 0;\n"
            + selected[selection] + "    return 1;\n}")


def make_axes(header, source):
    guard = definition(source, "type_random_map_generator::createGuard")
    loop_start = guard.index("    if (m_mapVersion <")
    loop_end = guard.index("        const TCreatureTypeTraits& traits =", loop_start)
    quantity_start = guard.index("    int aiValue =")
    quantity_end = guard.rfind("}")
    constructor_start = header.index("    rmgMonsterObject(")
    constructor_end = header.index("    virtual void write", constructor_start)
    writer = definition(source, "rmgMonsterObject::write")
    count_forms = [(kind, writer_count(kind)) for kind in ("int", "short", "unsigned_short", "narrowed_int")]
    old_count = [text for name, text in count_forms if text in writer]
    if len(old_count) != 1:
        raise ValueError("review the monster count serialization before generating")
    placed = definition(source, "type_random_map_generator::placeObjectInZone")
    return [
        axis("guard_reverse_loop", "src/rmg.cpp", guard[loop_start:loop_end],
             [("+".join(form), guard_loop(*form)) for form in itertools.product(
                 ("after", "before"), ("separate", "predecrement"))]),
        axis("guard_quantity", "src/rmg.cpp", guard[quantity_start:quantity_end],
             [("+".join(form), guard_quantity(*form)) for form in itertools.product(
                 ("local", "parameter"), ("pair", "delta", "named_delta"))]),
        axis("monster_construction", "include/rmg.h", header[constructor_start:constructor_end], constructors()),
        axis("monster_serialized_count", "src/rmg.cpp", old_count[0], count_forms),
        axis("zone_placement", "src/rmg.cpp", placed,
             [("+".join(form), placement(placed, *form)) for form in itertools.product(
                 ("xy", "yx"), ("copy", "direct", "assign", "coordinates"),
                 ("assign", "copy", "direct"), ("push", "insert", "count"))]),
    ]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / "include/rmg.h").read_text(), (HOMM3_DIR / "src/rmg.cpp").read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes,
                   evidence="Retail 0x540b20: 145 prototype slots, literal RoE exclusion 144..118, trait evaluation 116..0, ordered random remainders and canonical base construction. 0x5331f0 writes 2-byte count from a narrow load; preserve its base writer call and twelve writes. 0x542930: height-before-width footprint adjustment, row-major candidate traversal and selected coordinate copy. Alternatives change real C++ lifetimes/orders and public STL APIs. No include noise, dummy operations or inline pins.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
