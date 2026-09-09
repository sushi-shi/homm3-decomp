#!/usr/bin/env python3
"""Junction preparation coordinate ownership at retail 0x5446a0.

Retail retains two floods and one connection call, but no three-coordinate
constructor. The current body retains three constructors and has a 0x48
frame versus 0x34. Test real default/assigned/reused coordinate objects and
scan induction ownership, leaving constructors, reset and flood APIs intact.
No Dreamcast counterpart is mapped for this Complete-only function.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

NAME = "type_random_map_generator::prepareJunctionZone"


def authored_definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def baseline_definition(source):
    current = authored_definition(source)
    if "    TRmgMapPosition first;\n" not in current:
        return current
    original = current.replace(
        "    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {\n        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {\n            TRmgMapItem* item = m_map.getMapItem(position.m_x, position.m_y, level);",
        "    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {\n        for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {\n            TRmgMapItem* item = m_map.getMapItem(x, y, level);")
    original = replace_once(original,
        "    TRmgMapPosition first;\n    first.m_x = zone->m_entrances[0].m_x;\n    first.m_y = zone->m_entrances[0].m_y;\n    first.m_z = level;",
        "    TRmgMapPosition first(zone->m_entrances[0].m_x,\n        zone->m_entrances[0].m_y, level);")
    original = replace_once(original,
        "        TRmgMapPosition previous;\n        previous.m_x = from.m_x;\n        previous.m_y = from.m_y;\n        previous.m_z = level;",
        "        TRmgMapPosition previous(from.m_x, from.m_y, level);")
    original = replace_once(original,
        "        TRmgMapPosition next;\n        next.m_x = from.m_x;\n        next.m_y = from.m_y;\n        next.m_z = level;\n        m_map.floodConnectionCosts(next, 0);",
        "        m_map.floodConnectionCosts(TRmgMapPosition(from.m_x, from.m_y, level), 0);")
    if list(variants(original))[26][1] != current:
        raise ValueError("review changed adopted junction implementation")
    return original


def replace_once(source, old, new):
    if source.count(old) != 1:
        raise ValueError("review changed junction source anchor: " + old)
    return source.replace(old, new)


def variants(original):
    for first, tail, scan in itertools.product(range(4), range(5), range(3)):
        result = original
        seed = "    TRmgMapPosition first(zone->m_entrances[0].m_x,\n        zone->m_entrances[0].m_y, level);"
        if first:
            initializers = {
                1: "    TRmgMapPosition first;\n    first.m_x = zone->m_entrances[0].m_x;\n    first.m_y = zone->m_entrances[0].m_y;\n    first.m_z = level;",
                2: "    position.m_x = zone->m_entrances[0].m_x;\n    position.m_y = zone->m_entrances[0].m_y;",
                3: "    TRmgMapPosition first = position;\n    first.m_x = zone->m_entrances[0].m_x;\n    first.m_y = zone->m_entrances[0].m_y;",
            }
            result = replace_once(result, seed, initializers[first])
            if first == 2:
                result = replace_once(result, "getMapItem(first.m_x, first.m_y, first.m_z)",
                                      "getMapItem(position.m_x, position.m_y, position.m_z)")
                result = replace_once(result, "floodConnectionCosts(first, 0)", "floodConnectionCosts(position, 0)")
        if tail in (1, 3, 4):
            result = replace_once(result, "        TRmgMapPosition previous(from.m_x, from.m_y, level);",
                                  "        TRmgMapPosition previous;\n        previous.m_x = from.m_x;\n        previous.m_y = from.m_y;\n        previous.m_z = level;")
        if tail in (2, 3, 4):
            replacement = ("        TRmgMapPosition next;\n        next.m_x = from.m_x;\n        next.m_y = from.m_y;\n        next.m_z = level;\n        m_map.floodConnectionCosts(next, 0);"
                           if tail != 4 else "        previous.m_x = from.m_x;\n        previous.m_y = from.m_y;\n        previous.m_z = level;\n        m_map.floodConnectionCosts(previous, 0);")
            result = replace_once(result, "        m_map.floodConnectionCosts(TRmgMapPosition(from.m_x, from.m_y, level), 0);", replacement)
        if scan:
            point = "scan" if scan == 1 else "position"
            result = replace_once(result,
                "    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {\n        for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {\n            TRmgMapItem* item = m_map.getMapItem(x, y, level);",
                ("    TPoint scan;\n" if scan == 1 else "")
                + "    for (" + point + ".m_y = bounds.m_minimumY; " + point + ".m_y < bounds.m_maximumY; ++" + point + ".m_y) {\n"
                + "        for (" + point + ".m_x = bounds.m_minimumX; " + point + ".m_x < bounds.m_maximumX; ++" + point + ".m_x) {\n"
                + "            TRmgMapItem* item = m_map.getMapItem(" + point + ".m_x, " + point + ".m_y, level);")
        yield "first_%d+tail_%d+scan_%d" % (first, tail, scan), result


def refinement(parent, form):
    if form == 0:
        return replace_once(parent, "    TRmgMapPosition position = zone->getLevelPosition();",
                            "    TRmgMapPosition position;\n    position = zone->getLevelPosition();")
    if form == 1:
        return replace_once(parent, "        TPoint from = zone->m_entrances[entrance];",
                            "        TPoint from;\n        from = zone->m_entrances[entrance];")
    if form == 2:
        return replace_once(parent,
            "    item->m_previousTile.m_x = -1;\n    item->m_previousTile.m_y = -1;\n    item->m_previousTile.m_z = -1;",
            "    TRmgMapPosition& seedPrevious = item->m_previousTile;\n    seedPrevious.m_x = -1;\n    seedPrevious.m_y = -1;\n    seedPrevious.m_z = -1;")
    if form == 3:
        return replace_once(parent, "    int level = position.m_z;", "    const int& level = position.m_z;")
    if form == 4:
        parent = replace_once(parent, "    int level = position.m_z;",
                              "    int level = position.m_z;\n    TRmgMapItem* item;")
        parent = parent.replace("TRmgMapItem* item =", "item =")
        return parent
    raise ValueError("unknown junction refinement")


def entrance_refinement(parent, form):
    alternatives = (
        "        TPoint from = zone->m_entrances[entrance];",
        "        TPoint from;\n        from = zone->m_entrances[entrance];")
    anchors = [anchor for anchor in alternatives if parent.count(anchor) == 1]
    if len(anchors) != 1:
        raise ValueError("review changed entrance snapshot")
    expressions = [
        "        const TPoint from = zone->m_entrances[entrance];",
        "        TPoint from(zone->m_entrances[entrance].m_x, zone->m_entrances[entrance].m_y);",
        "        TPoint from;\n        from.m_x = zone->m_entrances[entrance].m_x;\n        from.m_y = zone->m_entrances[entrance].m_y;",
        "        const TPoint& entrancePoint = zone->m_entrances[entrance];\n        TPoint from(entrancePoint.m_x, entrancePoint.m_y);",
        "        const TPoint* entrancePoint = &zone->m_entrances[entrance];\n        TPoint from;\n        from.m_y = entrancePoint->m_y;\n        from.m_x = entrancePoint->m_x;",
    ]
    return parent.replace(anchors[0], expressions[form])


def predecessor_refinement(parent, form):
    if form == 3:
        return replace_once(parent, "        unsigned int cost = item->m_movement.m_cost;",
                            "        const unsigned int& cost = item->m_movement.m_cost;")
    if form in (0, 1, 4):
        anchor = "        TRmgMapPosition previous;\n        previous.m_x = from.m_x;\n        previous.m_y = from.m_y;\n        previous.m_z = level;"
        declaration = ("        TRmgMapPosition previous;\n        previous = position;" if form == 1
                       else "        TRmgMapPosition previous = position;")
        replacement = declaration + "\n        previous.m_x = from.m_x;\n        previous.m_y = from.m_y;"
        parent = replace_once(parent, anchor, replacement)
    if form in (2, 4):
        anchor = "        TRmgMapPosition next;\n        next.m_x = from.m_x;\n        next.m_y = from.m_y;\n        next.m_z = level;"
        if anchor in parent:
            parent = replace_once(parent, anchor,
                "        TRmgMapPosition next = position;\n        next.m_x = from.m_x;\n        next.m_y = from.m_y;")
        else:
            parent = replace_once(parent,
                "        previous.m_x = from.m_x;\n        previous.m_y = from.m_y;\n        previous.m_z = level;\n        m_map.floodConnectionCosts(previous, 0);",
                "        previous = position;\n        previous.m_x = from.m_x;\n        previous.m_y = from.m_y;\n        m_map.floodConnectionCosts(previous, 0);")
    return parent


def frontier(source, checkpoint_path, refine=refinement):
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get("generation", 0) < 1 or len(checkpoint["elites"]) != 10:
        raise ValueError("unfinished ten-parent junction population")
    _, originals, axes = load_manifest(context / "input.json", HOMM3_DIR)
    if len(checkpoint["records"]) != 60 or any(not row["scores"] for row in checkpoint["records"]):
        raise ValueError("junction population is not fully scored")
    for folder in ("src", "include"):
        frozen, live = context / "snapshot" / folder, HOMM3_DIR / folder
        paths = [p.relative_to(frozen) for p in frozen.rglob("*") if p.is_file()]
        live_paths = [p.relative_to(live) for p in live.rglob("*") if p.is_file() and "build" not in p.relative_to(live).parts]
        if set(paths) != set(live_paths) or any((frozen / p).read_bytes() != (live / p).read_bytes() for p in paths):
            raise ValueError("changed junction snapshot: " + folder)
    current = authored_definition(source)
    forms, seen, parents = [("unchanged", current)], {current}, []
    for elite in checkpoint["elites"]:
        rendered = source_families.render(originals, axes, tuple(elite["choices"]))
        repeated = context / "candidates" / elite["id"] / "repeat"
        result = json.loads((repeated / "result.json").read_text())
        for key in ("scores", "object_hash", "source_hashes", "choices"):
            if result[key] != elite[key]:
                raise ValueError("junction parent did not reproduce " + key)
        for relative, text in rendered.items():
            if (repeated / "tree" / relative).read_text() != text or result["source_hashes"][relative] != source_families.digest(text.encode()):
                raise ValueError("changed reproduced junction source")
        parent = authored_definition(rendered["src/rmg.cpp"])
        parents.append((elite["id"], parent))
        if parent not in seen:
            seen.add(parent)
            forms.append((elite["id"] + "+parent", parent))
    for form in range(5):
        for identity, parent in parents:
            body = refine(parent, form)
            if body not in seen:
                seen.add(body)
                forms.append((identity + "+binding_%d" % form, body))
            if len(forms) == 60:
                return forms
    return forms


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--bindings-from", type=Path)
    parser.add_argument("--entrances-from", type=Path)
    parser.add_argument("--predecessors-from", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    original = authored_definition(source)
    forms = frontier(source, args.bindings_from) if args.bindings_from else list(variants(baseline_definition(source)))
    if args.entrances_from:
        forms = frontier(source, args.entrances_from, entrance_refinement)
    if args.predecessors_from:
        forms = frontier(source, args.predecessors_from, predecessor_refinement)
    helper = generator("generate-rmg-position-family.py")
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[
        helper.axis("junction_coordinates", "src/rmg.cpp", original, forms)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(len(forms), "junction coordinate states")


if __name__ == "__main__":
    main()
