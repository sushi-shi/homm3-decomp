#!/usr/bin/env python3
"""Water-distance flood source families for retail 0x53f1a0.

Retail +0x1a1 branches before adding either 2 or 3, stores the resulting
cost, and later copies it through the ordinary position-first queue helper.
Seed single-insert and popped erase calls remain out of line in retail.
Test real arithmetic types, branch boundaries, coordinate construction and
public vector APIs without changing that canonical helper or its other sites.
No Dreamcast counterpart is mapped. Signed/unsigned cost alternatives share
the bounded nonnegative 16-bit distance domain, not arbitrary signed inputs.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

NAME = "type_random_map_generator::floodWaterZoneDistances"


def seed_control_origin(body):
    body = body.replace("""    std::vector<TRmgMapPosition>::iterator positionEnd = positions.end();
    positions.insert(positionEnd, position);""", "    positions.insert(positions.end(), position);")
    return body.replace("""    std::vector<int>::iterator costEnd = costs.end();
    costs.insert(costEnd, 0);""", "    costs.insert(costs.end(), 0);")


def baseline_definition(source):
    original = generator("generate-rmg-position-family.py").definition(source, NAME)
    direct = seed_control_origin(original).replace("            TPoint offset = g_rmgDirections[direction];\n", "")
    direct = direct.replace("position.m_x + offset.m_x", "position.m_x + g_rmgDirections[direction].m_x")
    direct = direct.replace("position.m_y + offset.m_y", "position.m_y + g_rmgDirections[direction].m_y")
    baseline = direct.replace("costs.pop_back();", "costs.erase(costs.end() - 1);")
    baseline = baseline.replace("positions.pop_back();", "positions.erase(positions.end() - 1);")
    baseline = baseline.replace("""            unsigned nextCost;
            if (direction & 1)
                nextCost = currentCost + 3;
            else
                nextCost = currentCost + 2;""",
        "            unsigned nextCost = currentCost + ((direction & 1) ? 3 : 2);")
    if direct not in {body for _, body in variants(baseline)}:
        raise ValueError("review changed water-flood body")
    return baseline


def variants(original):
    coordinate = """            TRmgMapPosition next;
            next.m_x = position.m_x + g_rmgDirections[direction].m_x;
            next.m_y = position.m_y + g_rmgDirections[direction].m_y;
            next.m_z = position.m_z;"""
    coordinates = [coordinate,
        """            TRmgMapPosition next = position;
            next += g_rmgDirections[direction];""",
        """            TRmgMapPosition next;
            next.m_x = position.m_x;
            next.m_y = position.m_y;
            next += g_rmgDirections[direction];
            next.m_z = position.m_z;"""]
    expression = "            unsigned nextCost = currentCost + ((direction & 1) ? 3 : 2);"
    if original.count(coordinate) != 1 or original.count(expression) != 1:
        raise ValueError("review changed water-flood coordinates or costs")
    for current_type, next_type, branch, coord, pop in itertools.product(
            ("unsigned", "int"), ("unsigned", "int"), range(2), range(3), range(2)):
        body = original.replace(coordinate, coordinates[coord])
        body = body.replace("unsigned currentCost =", current_type + " currentCost =")
        cost = (f"            {next_type} nextCost;\n"
                "            if (direction & 1)\n"
                "                nextCost = currentCost + 3;\n"
                "            else\n"
                "                nextCost = currentCost + 2;") if branch else expression.replace("unsigned", next_type)
        body = body.replace(expression, cost)
        if pop:
            body = body.replace("costs.erase(costs.end() - 1);", "costs.pop_back();")
            body = body.replace("positions.erase(positions.end() - 1);", "positions.pop_back();")
        yield f"current_{current_type}+next_{next_type}+branch_{branch}+coordinate_{coord}+pop_{pop}", body


def frontier_variants(original):
    old = """            TRmgMapPosition next;
            next.m_x = position.m_x + g_rmgDirections[direction].m_x;
            next.m_y = position.m_y + g_rmgDirections[direction].m_y;
            next.m_z = position.m_z;"""
    if original.count(old) != 1 or "unsigned nextCost;" not in original:
        raise ValueError("frontier requires the adopted explicit-cost source")
    forms = [old]
    for declaration, access in (("TPoint offset = g_rmgDirections[direction];", "offset."),
                                ("const TPoint& offset = g_rmgDirections[direction];", "offset."),
                                ("const TPoint* offset = &g_rmgDirections[direction];", "offset->")):
        forms.append("            " + declaration + "\n" + old.replace(
            "g_rmgDirections[direction].", access))
    forms.append(old.replace("position.m_x + g_rmgDirections[direction].m_x",
                            "g_rmgDirections[direction].m_x + position.m_x").replace(
                            "position.m_y + g_rmgDirections[direction].m_y",
                            "g_rmgDirections[direction].m_y + position.m_y"))
    for offset, seed, pop in itertools.product(range(5), range(3), range(4)):
        body = original.replace(old, forms[offset])
        if seed == 1:
            body = body.replace("positions.insert(positions.end(), position);", "positions.push_back(position);")
            body = body.replace("costs.insert(costs.end(), 0);", "costs.push_back(0);")
        elif seed == 2:
            body = body.replace("positions.insert(positions.end(), position);", "positions.insert(positions.end(), 1, position);")
            body = body.replace("costs.insert(costs.end(), 0);", "costs.insert(costs.end(), 1, 0);")
        if pop & 1:
            body = body.replace("costs.pop_back();", "costs.erase(costs.end() - 1);")
        if pop & 2:
            body = body.replace("positions.pop_back();", "positions.erase(positions.end() - 1);")
        yield f"offset_{offset}+seed_{seed}+pop_{pop}", body


def lifetime_variants(original):
    seed = """    positions.insert(positions.end(), position);
    costs.insert(costs.end(), 0);"""
    seeds = [seed]
    for qualifier in ("", "const "):
        seeds.append("""    positions.insert(positions.end(), position);
    QUALIFIERint seedCost = 0;
    costs.insert(costs.end(), seedCost);""".replace("QUALIFIER", qualifier))
    for named_cost in (False, True):
        seeds.append("""    std::vector<TRmgMapPosition>::iterator positionEnd = positions.end();
    positions.insert(positionEnd, position);
SEED_COST    std::vector<int>::iterator costEnd = costs.end();
    costs.insert(costEnd, SEED_VALUE);""".replace("SEED_COST", "    const int seedCost = 0;\n" if named_cost else "").replace("SEED_VALUE", "seedCost" if named_cost else "0"))
    if original.count(seed) != 1 or original.count("            unsigned nextCost;") != 1:
        raise ValueError("review water-flood seed or cost ownership")
    for binding, pop, lifetime in itertools.product(range(5), range(4), range(3)):
        body = original.replace(seed, seeds[binding])
        if pop & 1:
            body = body.replace("        costs.pop_back();", """        std::vector<int>::iterator lastCost = costs.end();
        --lastCost;
        costs.erase(lastCost);""")
        if pop & 2:
            body = body.replace("        positions.pop_back();", """        std::vector<TRmgMapPosition>::iterator lastPosition = positions.end();
        --lastPosition;
        positions.erase(lastPosition);""")
        if lifetime:
            body = body.replace("            unsigned nextCost;\n", "")
            anchor = "            TPoint offset = g_rmgDirections[direction];" if lifetime == 1 else "        for (int direction = 0; direction < 8; ++direction) {"
            declaration = "            unsigned nextCost;\n" if lifetime == 1 else "        unsigned nextCost;\n"
            if body.count(anchor) != 1:
                raise ValueError("review water-flood neighbor lifetime")
            body = body.replace(anchor, declaration + anchor)
        yield f"seed_binding_{binding}+pop_iterator_{pop}+cost_lifetime_{lifetime}", body


def iterator_controls(original):
    for position, cost in itertools.product(range(2), repeat=2):
        body = original
        if position:
            body = body.replace("    positions.insert(positions.end(), position);", """    std::vector<TRmgMapPosition>::iterator positionEnd = positions.end();
    positions.insert(positionEnd, position);""")
        if cost:
            body = body.replace("    costs.insert(costs.end(), 0);", """    std::vector<int>::iterator costEnd = costs.end();
    costs.insert(costEnd, 0);""")
        yield f"position_end_{position}+cost_end_{cost}", body


def make_axes(source, frontier=False, lifetimes=False, controls=False):
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, NAME)
    baseline = baseline_definition(source)
    initial_forms = list(variants(baseline))
    if controls:
        alternatives = iterator_controls(seed_control_origin(original))
    elif lifetimes:
        alternatives = lifetime_variants(seed_control_origin(original))
    elif frontier:
        parent = dict(initial_forms)["current_unsigned+next_unsigned+branch_1+coordinate_0+pop_1"]
        alternatives = frontier_variants(parent)
    else:
        alternatives = initial_forms
    return [helper.axis("water_queue", "src/rmg.cpp", original, alternatives)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--frontier", action="store_true", help="test direction captures and independent vector APIs from the adopted source")
    mode.add_argument("--lifetimes", action="store_true", help="test seed values, iterator bindings and neighbor-cost scope")
    mode.add_argument("--iterator-controls", action="store_true", help="isolate the two seed iterator bindings")
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg"], evidence=__doc__,
                   axes=make_axes((HOMM3_DIR / "src/rmg.cpp").read_text(), args.frontier, args.lifetimes, args.iterator_controls))
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "water-queue states")


if __name__ == "__main__":
    main()
