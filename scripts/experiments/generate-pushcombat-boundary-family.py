"""Test PushCombatPoint's attested accessor, loop and insert boundaries.

DC 0xa0f54 names const get_hex, a midpoint-before-break search loop, and
the two-argument vector::insert versus push_back arms. Its lines 1412-1417
store point.x, visited, point.y, direction, cost, flight_cost in that order;
the actual SH4 argument loads distinguish direction from flight_cost.
Cross those source facts with the old Complete field order and deletion of
the remaining insert fence. No substitute inline override or helper is added.
All tracked findpath rows are scored against the post-helper-recovery state.

Historical pre-adoption control: frozen context aba7f0923bc87620316b.
The adopted insertion and canonical accessor claim change these anchors.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = "src/findpath.cpp"


def replace(source, before, after):
    if source.count(before) != 1:
        raise ValueError("Review PushCombatPoint anchor: " + before)
    return source.replace(before, after)


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    start = source.index("VA(0x004b3bb0,")
    end = source.index("// E:\\gamedcs\\findpath.cpp:1427", start)
    original = source[start:end]
    loop = """    int middle = last / 2;
    while (last > first) {
        if (cost < m_queue[middle].m_cost)
            first = middle + 1;
        else
            last = middle;
        middle = (first + last) / 2;
    }"""
    dc_loop = """    int middle;
    for (;;) {
        middle = (first + last) / 2;
        if (first >= last)
            break;
        if (cost < m_queue[middle].m_cost)
            first = middle + 1;
        else
            last = middle;
    }"""
    fields = """    currentPathCell.m_point.m_x = index;
    currentPathCell.m_visited = 1;
    currentPathCell.m_cost = cost;
    currentPathCell.m_direction = direction;
    currentPathCell.m_point.m_y = 0;
    currentPathCell.m_flightCost = flightCost;"""
    dc_fields = """    currentPathCell.m_point.m_x = index;
    currentPathCell.m_visited = 1;
    currentPathCell.m_point.m_y = 0;
    currentPathCell.m_direction = direction;
    currentPathCell.m_cost = cost;
    currentPathCell.m_flightCost = flightCost;"""
    insertion = """        pathCell* pos = m_queue.begin() + middle;
#pragma inline_depth(0)
        m_queue.insert(pos, 1, currentPathCell);
#pragma inline_depth()"""
    options = []
    for cells, search, stores, insert, unpin in itertools.product(
            range(2), range(2), range(2), range(4), range(2)):
        candidate = original
        if cells:
            candidate = replace(candidate, "getCellData(index)", "getHex(index)")
        if search:
            candidate = replace(candidate, loop, dc_loop)
        if stores:
            candidate = replace(candidate, fields, dc_fields)
        direct = insert // 2
        single = insert % 2
        lines = [] if direct else ["        pathCell* pos = m_queue.begin() + middle;"]
        if not unpin:
            lines.append("#pragma inline_depth(0)")
        position = "m_queue.begin() + middle" if direct else "pos"
        arguments = position + (", " if single else ", 1, ") + "currentPathCell"
        lines.append("        m_queue.insert(" + arguments + ");")
        if not unpin:
            lines.append("#pragma inline_depth()")
        candidate = replace(candidate, insertion, "\n".join(lines))
        option = dict(name=f"cells-{cells}-loop-{search}-stores-{stores}-insert-{insert}-unpin-{unpin}")
        if candidate != original:
            option["replace"] = candidate
        options.append(option)
    return dict(schema=1, source=SOURCE, units=["findpath"], evidence=__doc__,
                axes=[dict(name="push-combat-boundaries", find=original, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
