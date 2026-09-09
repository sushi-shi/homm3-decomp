#!/usr/bin/env python3
"""Test a shared search boundary with caller-owned connection-flood insertion.

Retail floodConnectionCosts +0x3c6/+0x3de inserts cost before position;
the other eight sorted position-worklist sites insert position first. The
same top-tested descending binary search precedes both orders. The connection
flood passes its existing neighbor coordinate; road/river sites make another
three-dword copy. Test one ordinary index search shared by the retained
position-first insertion helper and this cost-first caller. This is a
provisional source-boundary hypothesis, not a recovered original signature.
Do not duplicate the insertion helper, flatten it in its existing callers,
or add an inline control. Score every rmg function, including all its callers.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
NAME = "type_random_map::floodConnectionCosts"
PARAMETERS = "std::vector<TRmgMapPosition>& positions, std::vector<int>& costs, TRmgMapPosition position, int cost"
BASELINE = """void type_random_map::floodConnectionCosts(TRmgMapPosition position, unsigned char waterZone)
{
    std::vector<int> costs;
    std::vector<TRmgMapPosition> positions;
    TRmgMapItem* seed = getMapItem(position);
    int zone = seed->m_zoneState.m_zone;
    positions.insert(positions.end(), position);
    costs.insert(costs.end(), 0);
    seed->m_movement.m_cost = 0;
    seed->m_previousTile.m_x = -1;
    seed->m_previousTile.m_y = -1;
    seed->m_previousTile.m_z = -1;
    while (positions.size()) {
        TRmgMapPosition currentPosition = positions.back();
        int queuedCost = costs.back();
        TRmgMapItem* current = getMapItem(currentPosition);
        positions.erase(positions.end() - 1);
        costs.erase(costs.end() - 1);
        int currentZone = current->m_zoneState.m_zone;
        int currentCost = currentZone == zone
            ? current->m_movement.m_cost : current->m_movement.m_zonePathCost;
        int direction = 8;
        if (current->isRoadEntrance()) {
            int objectType = current->m_objects[0]->m_properties->m_prototype->m_objectType;
            if (!g_adventureObjectLandBlocked[objectType][1])
                direction = 5;
        }
        while (direction--) {
            int nextCost = currentCost + 1;
            TRmgMapPosition nextPosition;
            nextPosition.m_x = currentPosition.m_x + g_rmgDirections[direction].m_x;
            nextPosition.m_y = currentPosition.m_y + g_rmgDirections[direction].m_y;
            nextPosition.m_z = currentPosition.m_z;
            if (nextPosition.m_x < 0 || nextPosition.m_x >= m_mapWidth
                || nextPosition.m_y < 0 || nextPosition.m_y >= m_mapHeight)
                continue;
            TRmgMapItem* next = getMapItem(nextPosition);
            if (next->m_zoneState.m_zone < 0 || !next->m_tileData.m_roadPassable
                || next->m_tile.m_landType == eTerrainRock)
                continue;
            if (next->isRoadEntrance()) {
                int objectType = next->m_objects[0]->m_properties->m_prototype->m_objectType;
                const unsigned char* traits = g_adventureObjectLandBlocked[objectType];
                if (traits[0] && !traits[2])
                    continue;
                if (!traits[1] && direction > 0 && direction < 4)
                    continue;
            }
            if (next->m_zoneState.m_zone != zone) {
                nextCost = currentCost + 10;
                if (currentZone != zone && currentZone != next->m_zoneState.m_zone)
                    continue;
                if (next->m_movement.m_zonePathCost <= nextCost)
                    continue;
                next->m_movement.m_zonePathCost = nextCost;
                next->m_tileData.m_connectionDirection = direction - 4;
                next->m_zoneState.m_connectionEligibility = zone;
            } else {
                if (currentZone != zone)
                    continue;
                if (next->m_tile.m_landType == eTerrainWater)
                    nextCost = currentCost + 10;
                if (next->m_movement.m_cost <= nextCost)
                    continue;
                if (!currentCost && next->hasSubterraneanGate()
                    && (next->m_tile.m_landType != eTerrainWater || waterZone))
                    nextCost = 0;
                next->setMovementCost(nextCost, currentPosition);
            }
            insertRmgWorkItem(positions, costs, nextPosition, nextCost);
        }
    }
}"""
SEARCH = """static int findRmgWorkItemInsertion(const std::vector<int>& costs, int last, int cost)
{
    int first = 0;
    int middle;
    while (1) {
        middle = (first + last) >> 1;
        if (first >= last)
            break;
        if (cost < costs[middle])
            first = middle + 1;
        else
            last = middle;
    }
    return middle;
}
"""


def definition(source, name, **kwargs):
    return generator("generate-rmg-position-family.py").definition(source, name, **kwargs)


def helper_pair(source):
    original = definition(source, "insertRmgWorkItem", parameters=PARAMETERS)
    start = original.index("    int first = 0;")
    end = original.index("    positions.insert(", start)
    expected = SEARCH[SEARCH.index("    int first = 0;"):SEARCH.index("    return middle;")]
    expected = expected.replace("    int first = 0;\n", "    int first = 0;\n    int last = positions.size();\n") + "\n"
    if original[start:end] != expected:
        raise ValueError("review changed worklist search")
    rewritten = original[:start] + "    int middle = findRmgWorkItemInsertion(costs, positions.size(), cost);\n\n" + original[end:]
    return original, SEARCH + "\n" + rewritten


def options(source):
    original = definition(source, NAME)
    old_helper, new_helper = helper_pair(source)
    call = "            insertRmgWorkItem(positions, costs, nextPosition, nextCost);"
    coordinate = """            TRmgMapPosition nextPosition;
            nextPosition.m_x = currentPosition.m_x + g_rmgDirections[direction].m_x;
            nextPosition.m_y = currentPosition.m_y + g_rmgDirections[direction].m_y;
            nextPosition.m_z = currentPosition.m_z;"""
    coordinates = [coordinate,
        """            TRmgMapPosition nextPosition;
            nextPosition.m_z = currentPosition.m_z;
            nextPosition.m_x = currentPosition.m_x + g_rmgDirections[direction].m_x;
            nextPosition.m_y = currentPosition.m_y + g_rmgDirections[direction].m_y;""",
        """            TRmgMapPosition nextPosition = currentPosition;
            nextPosition += g_rmgDirections[direction];"""]
    if original.count(call) != 1 or original.count(coordinate) != 1:
        raise ValueError("review changed connection-flood ownership")
    yield dict(name="unchanged", replace=original)
    for count, seed, pop, coord, cost in itertools.product(range(2), range(2), range(2), range(3), range(2)):
        insertion = """            int middle = findRmgWorkItemInsertion(costs, positions.size(), nextCost);
            costs.insert(costs.begin() + middle, NEXT_COUNTnextCost);
            positions.insert(positions.begin() + middle, NEXT_COUNTnextPosition);"""
        body = original.replace(call, insertion.replace("NEXT_COUNT", "1, " if count else ""))
        body = body.replace(coordinate, coordinates[coord])
        if seed:
            body = body.replace("positions.insert(positions.end(), position);", "positions.push_back(position);")
            body = body.replace("costs.insert(costs.end(), 0);", "costs.push_back(0);")
        if pop:
            body = body.replace("positions.erase(positions.end() - 1);", "positions.pop_back();")
            body = body.replace("costs.erase(costs.end() - 1);", "costs.pop_back();")
        if cost:
            body = body.replace("int nextCost = currentCost + 1;", "queuedCost = currentCost + 1;")
            body = body.replace("nextCost", "queuedCost")
        yield dict(name=f"count_{count}+seed_{seed}+pop_{pop}+coordinate_{coord}+cost_{cost}",
                   replace=body, extra_edits=[dict(source=SOURCE, find=old_helper, replace=new_helper)])


def signature_forms(body, helpers):
    """Keep the real bound's owner and cost vector, varying read-only binding.

    The retained helper originally computes positions.size() internally.
    Moving that bound across the new index-search boundary is observable to
    VC6 even though no source call changes its value. No ABI is claimed for
    this provisionally inferred, fully expanded search.
    """
    if SEARCH not in helpers:
        raise ValueError("parent does not use the reviewed search boundary")
    for const_cost in (False, True):
        # const_cost=True is the unchanged parent, already retained below.
        if not const_cost:
            yield "mutable_costs+scalar_bound", body, helpers.replace(
                "const std::vector<int>& costs, int last", "std::vector<int>& costs, int last", 1)
        for const_positions in (False, True):
            position_type = ("const " if const_positions else "") + "std::vector<TRmgMapPosition>& positions"
            cost_type = ("const " if const_cost else "") + "std::vector<int>& costs"
            new_search = SEARCH.replace("const std::vector<int>& costs, int last", position_type + ", " + cost_type)
            new_search = new_search.replace("    int first = 0;", "    int first = 0;\n    int last = positions.size();")
            replacement = helpers.replace(SEARCH, new_search)
            old_call = "findRmgWorkItemInsertion(costs, positions.size(), "
            new_call = "findRmgWorkItemInsertion(positions, costs, "
            yield f"positions_{int(const_positions)}+costs_{int(const_cost)}", body.replace(old_call, new_call), replacement.replace(old_call, new_call)
    # A separately initialized search bound is a real lifetime, not padding.
    replacement = helpers.replace("const std::vector<int>& costs, int last, int cost", "const std::vector<int>& costs, int count, int cost", 1)
    replacement = replacement.replace("    int first = 0;", "    int first = 0;\n    int last = count;", 1)
    yield "local_bound", body, replacement


def pointer_forms(body, helpers):
    """Retail loads the cost array once before each search, then indexes it.

    A public begin() argument models that observed input directly. Output
    index ownership and the upper-bound local remain genuine value lifetimes.
    """
    for mutable in (False, True):
        pointer = "int* costs" if mutable else "const int* costs"
        search = SEARCH.replace("const std::vector<int>& costs", pointer)
        for result in ("value", "output", "bound_local"):
            new_body = body.replace("findRmgWorkItemInsertion(costs,", "findRmgWorkItemInsertion(costs.begin(),")
            new_helpers = helpers.replace(SEARCH, search).replace("findRmgWorkItemInsertion(costs,", "findRmgWorkItemInsertion(costs.begin(),")
            if result == "output":
                output_search = search.replace("static int find", "static void find")
                output_search = output_search.replace("int cost)", "int cost, int& middle)")
                output_search = output_search.replace("    int middle;\n", "").replace("    return middle;\n", "")
                new_helpers = new_helpers.replace(search, output_search)
                for cost in ("nextCost", "queuedCost", "cost"):
                    old = f"int middle = findRmgWorkItemInsertion(costs.begin(), positions.size(), {cost});"
                    new = f"int middle;\n            findRmgWorkItemInsertion(costs.begin(), positions.size(), {cost}, middle);"
                    new_body = new_body.replace(old, new)
                    new_helpers = new_helpers.replace(old, new.replace("            find", "    find"))
            elif result == "bound_local":
                new_helpers = new_helpers.replace("int last, int cost", "int count, int cost", 1)
                new_helpers = new_helpers.replace("    int first = 0;", "    int first = 0;\n    int last = count;", 1)
            yield f"pointer_{int(mutable)}+{result}", new_body, new_helpers


def range_forms(body, helpers):
    """An ordinary index-range search owns both mutable interval bounds.

    The lower zero initialization is present in every retail copy. Passing
    that actual bound at the boundary preserves the search algorithm, unlike
    adding a dummy parameter or an inline-budget-only expression.
    """
    for cost_type in ("const std::vector<int>&", "std::vector<int>&", "const int*", "int*"):
        search = SEARCH.replace("const std::vector<int>& costs, int last", cost_type + " costs, int first, int last")
        search = search.replace("    int first = 0;\n", "")
        argument = "costs.begin()" if "*" in cost_type else "costs"
        old = "findRmgWorkItemInsertion(costs, positions.size(),"
        new = f"findRmgWorkItemInsertion({argument}, 0, positions.size(),"
        yield cost_type.replace(" ", "_") + "+range", body.replace(old, new), helpers.replace(SEARCH, search).replace(old, new)


def local_options(source):
    """Withdraw only the contradicted insertion-helper attribution.

    Unlike its eight supported calls, this flood has cost-first insertion
    and no separate by-value coordinate snapshot. Reconstruct that distinct
    local routine; do not flatten or modify the actual position-first helper
    or any of its other callers. The preceding shared-index families test
    a stronger abstraction, but introduce retained calls absent from retail.
    """
    for choice in options(source):
        if choice["name"] == "unchanged":
            yield choice
            continue
        body = choice["replace"]
        cost = "queuedCost" if "int nextCost =" not in body else "nextCost"
        old = f"            int middle = findRmgWorkItemInsertion(costs, positions.size(), {cost});"
        new = f"""            int first = 0;
            int last = positions.size();
            int middle;
            while (1) {{
                middle = (first + last) >> 1;
                if (first >= last)
                    break;
                if ({cost} < costs[middle])
                    first = middle + 1;
                else
                    last = middle;
            }}"""
        if body.count(old) != 1:
            raise ValueError("changed local-search anchor")
        yield dict(name="local+" + choice["name"], replace=body.replace(old, new))


def origin_source(source):
    """Recognize an adopted local child; never reinterpret a changed algorithm."""
    current = definition(source, NAME)
    if current == BASELINE:
        return source
    original = source.replace(current, BASELINE)
    admitted = {choice["replace"] for choice in local_options(original)}
    if current not in admitted:
        raise ValueError("review changed connection-flood implementation")
    return original


def frontier(source, checkpoint_path, *, pointers=False, ranges=False, lookups=False):
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get("generation", 0) < 1 or len(checkpoint["elites"]) != 10:
        raise ValueError("unfinished ten-parent queue population")
    if len(checkpoint["records"]) != 49 or any(not row["scores"] for row in checkpoint["records"]):
        raise ValueError("queue population is not fully scored")
    _, originals, axes = source_families.load_manifest(context / "input.json", HOMM3_DIR)
    for folder in ("src", "include"):
        frozen, live = context / "snapshot" / folder, HOMM3_DIR / folder
        paths = {p.relative_to(frozen) for p in frozen.rglob("*") if p.is_file()}
        live_paths = {p.relative_to(live) for p in live.rglob("*") if p.is_file() and "build" not in p.relative_to(live).parts}
        if paths != live_paths or any((frozen / p).read_bytes() != (live / p).read_bytes() for p in paths):
            raise ValueError("changed queue snapshot: " + folder)
    baseline = definition(source, NAME)
    old_helper, _ = helper_pair(source)
    choices = [dict(name="unchanged", replace=baseline)]
    parents = []
    for elite in checkpoint["elites"]:
        rendered = source_families.render(originals, axes, tuple(elite["choices"]))
        repeated = context / "candidates" / elite["id"] / "repeat"
        result = json.loads((repeated / "result.json").read_text())
        for key in ("scores", "object_hash", "source_hashes", "choices"):
            if result[key] != elite[key]:
                raise ValueError("queue parent did not reproduce " + key)
        for relative, text in rendered.items():
            if (repeated / "tree" / relative).read_text() != text or result["source_hashes"][relative] != source_families.digest(text.encode()):
                raise ValueError("changed reproduced queue source")
        if elite["choices"] == [0]:
            continue
        body = definition(rendered[SOURCE], NAME)
        if lookups:
            retained_helper = definition(rendered[SOURCE], "insertRmgWorkItem", parameters=PARAMETERS)
            if retained_helper != old_helper or "findRmgWorkItemInsertion" in rendered[SOURCE]:
                raise ValueError("lookup parent changed the canonical insertion helper")
            if source.replace(baseline, body) != rendered[SOURCE]:
                raise ValueError("lookup parent has an uncarried source edit")
            parents.append((elite["id"], body, old_helper))
            choices.append(dict(name=elite["id"] + "+parent", replace=body))
            continue
        helpers = definition(rendered[SOURCE], "findRmgWorkItemInsertion") + "\n\n"
        helpers += definition(rendered[SOURCE], "insertRmgWorkItem", parameters=PARAMETERS)
        if source.replace(baseline, body).replace(old_helper, helpers) != rendered[SOURCE]:
            raise ValueError("queue parent has an uncarried source edit")
        parents.append((elite["id"], body, helpers))
        choices.append(dict(name=elite["id"] + "+parent", replace=body,
                            extra_edits=[dict(source=SOURCE, find=old_helper, replace=helpers)]))
    for identity, body, helpers in parents:
        if lookups:
            # Both real map-lookup overloads already exist and are canonical.
            # The fully expanded retail calls do not identify which overload
            # supplied their coordinate arguments. Never paste the indexing.
            for mask in range(1, 8):
                new_body = body
                for bit, coordinate in enumerate(("position", "currentPosition", "nextPosition")):
                    if mask & (1 << bit):
                        old = f"getMapItem({coordinate})"
                        new = f"getMapItem({coordinate}.m_x, {coordinate}.m_y, {coordinate}.m_z)"
                        if body.count(old) != 1:
                            raise ValueError("changed coordinate lookup parent")
                        new_body = new_body.replace(old, new)
                choices.append(dict(name=identity + f"+lookups_{mask}", replace=new_body))
            continue
        make_forms = range_forms if ranges else pointer_forms if pointers else signature_forms
        for label, new_body, new_helpers in make_forms(body, helpers):
            choices.append(dict(name=identity + "+" + label, replace=new_body,
                                extra_edits=[dict(source=SOURCE, find=old_helper, replace=new_helpers)]))
    if len(choices) != (81 if lookups else 46 if ranges else 64):
        raise ValueError("unexpected queue parent coverage")
    return choices


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path, help="search parameter ownership from ten reproduced parents")
    parser.add_argument("--pointers-from", type=Path, help="public cost-array input from the first ten reproduced parents")
    parser.add_argument("--ranges-from", type=Path, help="explicit index-range input from the first ten reproduced parents")
    parser.add_argument("--local", action="store_true", help="withdraw the contradicted helper attribution only in this flood")
    parser.add_argument("--lookups-from", type=Path, help="canonical lookup overloads from the local-routine parents")
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    if sum(bool(path) for path in (args.parents_from, args.pointers_from, args.ranges_from, args.local, args.lookups_from)) > 1:
        parser.error("choose one frontier")
    parent = args.parents_from or args.pointers_from or args.ranges_from or args.lookups_from
    if parent:
        choices = frontier(source, parent, pointers=bool(args.pointers_from), ranges=bool(args.ranges_from), lookups=bool(args.lookups_from))
    else:
        original = origin_source(source)
        choices = list(local_options(original) if args.local else options(original))
        current = definition(source, NAME)
        if current != choices[0]["replace"]:
            choices = [dict(name="current", replace=current)] + [choice for choice in choices
                if choice["replace"] != current or choice.get("extra_edits")]
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="connection_queue", source=SOURCE, find=choices[0]["replace"], options=choices)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(choices), "connection-queue states")


if __name__ == "__main__":
    main()
