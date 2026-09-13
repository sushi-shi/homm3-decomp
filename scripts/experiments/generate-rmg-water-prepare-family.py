#!/usr/bin/env python3
"""Construction and cell-binding families for retail prepareWaterZoneConnections.

Retail 0x53f470 copies all four bounds and three position members at entry,
then keeps the zone index in EBX across each perimeter row. The initial
candidate instead reloads that saved index for each cell and splits the
minimum-X reload around the outer back edge. Test real aggregate lifetimes
and complete-cell/result bindings; retain the coordinate and flood APIs.
Complete's RMG TU has no mapped Dreamcast counterpart.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

NAME = "type_random_map_generator::prepareWaterZoneConnections"


def selection_origin(original):
    return original.replace("""        TRmgMapItem* selectedItem = m_map.getMapItem(position);
        int range = selectedItem->m_movement.m_zonePathCost / 3 - 5;""",
        "        int range = m_map.getMapItem(position)->m_movement.m_zonePathCost / 3 - 5;")


def baseline_definition(source):
    original = selection_origin(generator("generate-rmg-position-family.py").definition(source, NAME))
    baseline = original.replace("    TRmgMapPosition position;\n    position = zone->m_levelPosition;",
        "    TRmgMapPosition position = zone->m_levelPosition;")
    baseline = baseline.replace("""            TRmgMapItem* item = m_map.getMapItem(position);
            if (item->m_zoneState.m_zone != zoneIndex)""",
        "            if (m_map.getMapItem(position)->m_zoneState.m_zone != zoneIndex)")
    if original not in {body for _, body in variants(baseline)}:
        raise ValueError("review changed preparation source before rebasing")
    return baseline


def variants(original):
    entry = """    TRmgZoneBounds bounds = zone->m_bounds;
    int zoneIndex = zone->m_slot->m_zoneIndex;
    TRmgMapPosition position = zone->m_levelPosition;"""
    test = """            if (m_map.getMapItem(position)->m_zoneState.m_zone != zoneIndex)
                floodWaterZoneDistances(position, zoneIndex);"""
    if original.count(entry) != 1 or original.count(test) != 1:
        raise ValueError("review changed water preparation entry/perimeter")
    bindings = [test]
    for declaration, value in (
        ("TRmgMapItem* item = m_map.getMapItem(position);", "item->m_zoneState.m_zone"),
        ("TRmgMapItem& item = *m_map.getMapItem(position);", "item.m_zoneState.m_zone"),
        ("int otherZone = m_map.getMapItem(position)->m_zoneState.m_zone;", "otherZone"),
    ):
        bindings.append("            " + declaration + "\n" + test.replace(
            "m_map.getMapItem(position)->m_zoneState.m_zone", value))
    for construction, index, binding in itertools.product(range(5), range(3), range(4)):
        changed = entry
        if construction in (1, 3):
            changed = changed.replace("TRmgZoneBounds bounds = zone->m_bounds;",
                "TRmgZoneBounds bounds;\n    bounds = zone->m_bounds;")
        if construction in (2, 3):
            changed = changed.replace("TRmgMapPosition position = zone->m_levelPosition;",
                "TRmgMapPosition position;\n    position = zone->m_levelPosition;")
        if construction == 4:
            changed = "    TRmgMapPosition position;\n" + changed.replace(
                "TRmgMapPosition position =", "position =")
        if index == 1:
            changed = changed.replace("int zoneIndex =", "const int zoneIndex =")
        elif index == 2:
            changed = changed.replace("    int zoneIndex = zone->m_slot->m_zoneIndex;\n", "")
            changed = "    int zoneIndex = zone->m_slot->m_zoneIndex;\n" + changed
        yield f"construction_{construction}+index_{index}+binding_{binding}", original.replace(
            entry, changed).replace(test, bindings[binding])


def loop_variants(original):
    start = original.index("    for (position.m_y = surrounding.m_minimumY;")
    end = original.index("    bounds.m_minimumX = max(bounds.m_minimumX, 3);", start)
    loop = original[start:end]
    body_start = loop.index("\n", loop.index("        for (")) + 1
    body = loop[body_start:loop.rindex("        }\n    }\n")]
    while_loop = """    position.m_y = surrounding.m_minimumY;
    while (position.m_y < surrounding.m_maximumY) {
        position.m_x = surrounding.m_minimumX;
        while (position.m_x < surrounding.m_maximumX) {
BODY            ++position.m_x;
        }
        ++position.m_y;
    }
""".replace("BODY", body)
    do_loop = """    position.m_y = surrounding.m_minimumY;
    if (position.m_y < surrounding.m_maximumY) {
        do {
            position.m_x = surrounding.m_minimumX;
            if (position.m_x < surrounding.m_maximumX) {
                do {
BODY                    ++position.m_x;
                } while (position.m_x < surrounding.m_maximumX);
            }
            ++position.m_y;
        } while (position.m_y < surrounding.m_maximumY);
    }
""".replace("BODY", "\n".join("        " + line for line in body.rstrip().splitlines()) + "\n")
    goto_loop = """    position.m_y = surrounding.m_minimumY;
waterRow:
    if (position.m_y >= surrounding.m_maximumY)
        goto waterRowsDone;
    position.m_x = surrounding.m_minimumX;
waterCell:
    if (position.m_x >= surrounding.m_maximumX)
        goto waterNextRow;
    {
BODY    }
    ++position.m_x;
    goto waterCell;
waterNextRow:
    ++position.m_y;
    goto waterRow;
waterRowsDone:
""".replace("BODY", body)
    scope_start = original.index("    TRmgZoneBounds surrounding;")
    scoped = original[:scope_start] + "    {\n" + original[scope_start:end] + "    }\n" + original[end:]
    yield "perimeter_scope", scoped
    for label, replacement in (("while", while_loop), ("guarded_do", do_loop), ("goto", goto_loop),
            ("reversed_guard", loop.replace("position.m_y < surrounding.m_maximumY", "surrounding.m_maximumY > position.m_y").replace(
                "position.m_x < surrounding.m_maximumX", "surrounding.m_maximumX > position.m_x"))):
        yield label, original[:start] + replacement + original[end:]


def getter_variants(original):
    rhs = "zone->m_levelPosition"
    if original.count(rhs) != 1:
        raise ValueError("review changed position source")
    yield "getter", original.replace(rhs, "zone->getLevelPosition()")
    statement = "    TRmgMapPosition position = " + rhs + ";"
    assigned = "    position = " + rhs + ";"
    if statement in original:
        # Keep the real default constructor, then consume the returned value.
        yield "getter_assignment", original.replace(statement,
            "    TRmgMapPosition position;\n    position = zone->getLevelPosition();")
    else:
        yield "getter_initialization", original.replace("    TRmgMapPosition position;\n", "").replace(
            assigned, "    TRmgMapPosition position = zone->getLevelPosition();")
    for label, receiver in (("returned_reference", "zone->getLevelPosition()"),
                            ("member_reference", rhs)):
        if statement in original:
            replacement = "    TRmgMapPosition position;\n"
            anchor = statement
        else:
            replacement = ""
            anchor = assigned
        replacement += "    {\n        const TRmgMapPosition& initialPosition = " + receiver + ";\n"
        replacement += "        position = initialPosition;\n    }"
        yield label, original.replace(anchor, replacement)
    bound_rhs = "zone->m_bounds"
    if original.count(bound_rhs) != 1:
        raise ValueError("review changed bounds source")
    anchor = "    TRmgZoneBounds bounds"
    bound_reference = original.replace(anchor,
        "    const TRmgZoneBounds& initialBounds = zone->m_bounds;\n" + anchor)
    # Replace the bound copy's source, not the new reference initialization.
    bound_reference = bound_reference.replace("bounds = zone->m_bounds", "bounds = initialBounds")
    yield "getter_bounds_reference", bound_reference.replace(rhs, "zone->getLevelPosition()")


def selection_variants(original):
    """Retail +0x2d2:+0x339 forms the whole cell address before reading cost.

    The selected coordinate's three field loads and both ordered random draws
    already agree. Test genuine selected-value and complete-cell bindings,
    keeping the unsigned distance division and signed random remainder intact.
    """
    selection = "        position = candidates[rand() % candidates.size()];"
    lookup = "        int range = m_map.getMapItem(position)->m_movement.m_zonePathCost / 3 - 5;"
    radius = """        int radius = rand() % range + 3;
        if (radius > 6)
            radius = 6;"""
    if any(original.count(anchor) != 1 for anchor in (selection, lookup, radius)):
        raise ValueError("review changed island selection or radius source")
    selections = [selection,
        """        unsigned selectedIndex = rand() % candidates.size();
        position = candidates[selectedIndex];""",
        """        int selectionRandom = rand();
        position = candidates[selectionRandom % candidates.size()];""",
        """        const TRmgMapPosition& selected = candidates[rand() % candidates.size()];
        position = selected;""",
        """        TRmgMapPosition selected = candidates[rand() % candidates.size()];
        position = selected;"""]
    lookups = [lookup,
        """        TRmgMapItem* selectedItem = m_map.getMapItem(position);
        int range = selectedItem->m_movement.m_zonePathCost / 3 - 5;""",
        """        TRmgMapItem& selectedItem = *m_map.getMapItem(position);
        int range = selectedItem.m_movement.m_zonePathCost / 3 - 5;""",
        """        unsigned distance = m_map.getMapItem(position)->m_movement.m_zonePathCost;
        int range = distance / 3 - 5;"""]
    radii = [radius,
        "        int radius = min(rand() % range + 3, 6);",
        """        int rawRadius = rand() % range + 3;
        int radius = rawRadius;
        if (radius > 6)
            radius = 6;"""]
    for selected, item, clamp in itertools.product(range(5), range(4), range(3)):
        yield f"selected_{selected}+item_{item}+clamp_{clamp}", original.replace(
            selection, selections[selected]).replace(lookup, lookups[item]).replace(radius, radii[clamp])


def candidate_scan_variants(original):
    """The candidate-list scan has the other remaining expanded cost lookup."""
    test = "                if (m_map.getMapItem(position)->m_movement.m_zonePathCost >= 20)"
    append = "                    candidates.push_back(position);"
    if original.count(test) != 1 or original.count(append) != 1:
        raise ValueError("review changed candidate-list scan")
    for label, declaration, value in (
        ("scan_pointer", "TRmgMapItem* candidateItem = m_map.getMapItem(position);", "candidateItem->m_movement.m_zonePathCost"),
        ("scan_reference", "TRmgMapItem& candidateItem = *m_map.getMapItem(position);", "candidateItem.m_movement.m_zonePathCost"),
        ("scan_cost", "unsigned candidateCost = m_map.getMapItem(position)->m_movement.m_zonePathCost;", "candidateCost"),
    ):
        yield label, original.replace(test, "                " + declaration + "\n                if (" + value + " >= 20)")
    yield "scan_insert", original.replace(append, "                    candidates.insert(candidates.end(), position);")
    yield "scan_named_insert", original.replace(test + "\n" + append, test + " {\n"
        "                    std::vector<TRmgMapPosition>::iterator candidateEnd = candidates.end();\n"
        "                    candidates.insert(candidateEnd, position);\n                }")


def upper_bound_variants(original):
    """Retail uses ADD -4 at both inset limits; current C++ emits SUB 4."""
    width = "    bounds.m_maximumX = min(bounds.m_maximumX, m_map.m_mapWidth - 4);"
    height = "    bounds.m_maximumY = min(bounds.m_maximumY, m_map.m_mapHeight - 4);"
    if original.count(width) != 1 or original.count(height) != 1:
        raise ValueError("review changed inset upper bounds")
    for label, x, y in (("width_add", True, False), ("height_add", False, True), ("both_add", True, True)):
        body = original
        if x:
            body = body.replace(width, width.replace("m_mapWidth - 4", "m_mapWidth + -4"))
        if y:
            body = body.replace(height, height.replace("m_mapHeight - 4", "m_mapHeight + -4"))
        yield label, body
    for label, operation in (("named_limits", " - 4"), ("named_add_limits", " + -4")):
        body = original.replace(width, "    int maximumX = m_map.m_mapWidth" + operation + ";\n"
            "    bounds.m_maximumX = min(bounds.m_maximumX, maximumX);")
        body = body.replace(height, "    int maximumY = m_map.m_mapHeight" + operation + ";\n"
            "    bounds.m_maximumY = min(bounds.m_maximumY, maximumY);")
        yield label, body


def parent_frontier(source, checkpoint_path, refinement):
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get("generation", 0) < 1 or len(checkpoint["elites"]) != 10:
        raise ValueError("unfinished ten-parent water preparation population")
    _, originals, axes = source_families.load_manifest(context / "input.json", HOMM3_DIR)
    if len(checkpoint["records"]) != 60 or any(not row["scores"] for row in checkpoint["records"]):
        raise ValueError("water preparation population is not fully scored")
    for folder in ("src", "include"):
        frozen = context / "snapshot" / folder
        live = HOMM3_DIR / folder
        paths = [p.relative_to(frozen) for p in frozen.rglob("*") if p.is_file()]
        live_paths = [p.relative_to(live) for p in live.rglob("*") if p.is_file() and "build" not in p.relative_to(live).parts]
        if set(paths) != set(live_paths) or any((frozen / p).read_bytes() != (live / p).read_bytes() for p in paths):
            raise ValueError("changed water preparation snapshot: " + folder)
    helper = generator("generate-rmg-position-family.py")
    current = helper.definition(source, NAME)
    alternatives = [("unchanged", current)]
    seen = {current}
    parents = []
    for elite in checkpoint["elites"]:
        rendered = source_families.render(originals, axes, tuple(elite["choices"]))
        repeated = context / "candidates" / elite["id"] / "repeat"
        result = json.loads((repeated / "result.json").read_text())
        for key in ("scores", "object_hash", "source_hashes", "choices"):
            if result[key] != elite[key]:
                raise ValueError("water preparation parent did not reproduce " + key)
        for relative, text in rendered.items():
            if (repeated / "tree" / relative).read_text() != text or result["source_hashes"][relative] != source_families.digest(text.encode()):
                raise ValueError("changed reproduced water preparation source")
        parent = helper.definition(rendered["src/rmg.cpp"], NAME)
        parents.append((elite["id"], list(refinement(parent))))
        if parent not in seen:
            seen.add(parent)
            alternatives.append((elite["id"] + "+parent", parent))
    for form in range(5):
        for identity, forms in parents:
            label, body = forms[form]
            if body in seen:
                continue
            seen.add(body)
            alternatives.append((identity + "+" + label, body))
            if len(alternatives) == 60:
                return alternatives
    return alternatives


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--loops-from", type=Path, help="loop lifetimes from ten reproduced parents")
    mode.add_argument("--getters-from", type=Path, help="canonical position-getter lifetimes from ten reproduced parents")
    mode.add_argument("--selection", action="store_true", help="selected coordinate, cell and radius value lifetimes")
    mode.add_argument("--scan-from", type=Path, help="candidate-scan bindings from ten reproduced selection parents")
    mode.add_argument("--bounds-from", type=Path, help="inset upper-limit expressions from ten reproduced scan parents")
    args = parser.parse_args()
    helper = generator("generate-rmg-position-family.py")
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    original = helper.definition(source, NAME)
    if args.loops_from:
        alternatives = parent_frontier(source, args.loops_from, loop_variants)
    elif args.getters_from:
        alternatives = parent_frontier(source, args.getters_from, getter_variants)
    elif args.selection:
        alternatives = selection_variants(selection_origin(original))
    elif args.scan_from:
        alternatives = parent_frontier(source, args.scan_from, candidate_scan_variants)
    elif args.bounds_from:
        alternatives = parent_frontier(source, args.bounds_from, upper_bound_variants)
    else:
        alternatives = variants(baseline_definition(source))
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[
        helper.axis("water_prepare", "src/rmg.cpp", original, alternatives)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "water-prepare states")


if __name__ == "__main__":
    main()
