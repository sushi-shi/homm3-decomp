"""Restore combat-path helper ownership and remove matching-only extraction.

DC findpath.cpp:1136/1172/1187 proves private build_combat_path,
mark_enemy and check_enemy_armies members in that order. mark_enemy's
1176/1177 scopes support a nested early return before the two stores.
Both mark_teleport and check_enemy_armies call that same ordinary member.
The retained getCellData body and each caller's expansion are independent.

The old source explicitly labels combatWalkLimits, combatSiegePressure and
clearCombatCellMarks as budget probes. Put their real operations back into
FindCombatPath, preserving Complete's behavior and source statement order.
Do not add assertions or replace the duplicate mark helper with another pin.

This first 48-state pass scores all findpath functions. A member-declaration
adoption requires a full shared-header build before accepting its collateral.

Historical control: context 94c8b2f5a3af5fa41569 predates adoption of these
private helper bodies. Its frozen source/header snapshot retains the inputs.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = "src/findpath.cpp"
MARK_START = "// E:\\gamedcs\\findpath.cpp:1172\ninline void searchArray::markEnemy"
TELEPORT = "// E:\\gamedcs\\findpath.cpp:1004\n"
COPY_START = "// THE SEARCH SIDE'S COPY OF mark_enemy, AND THE BYTES REQUIRE A SECOND ONE.\n"
CHECK_START = "// E:\\gamedcs\\findpath.cpp:1187\n"
BUILD_START = "// E:\\gamedcs\\findpath.cpp:1136 - the THIRD of the DC roster's three\n"
WALK_START = "// FindCombatPath's speed/limit pick, lifted for the /Ob2 BUDGET probe.\n"
SIEGE_START = "// FindCombatPath's siege-pressure preamble, lifted for the /Ob2 BUDGET probe.\n"
CLEAR_START = "// FindCombatPath's 187-cell mark wipe, lifted for the /Ob2 BUDGET probe.\n"
FIND_START = "// E:\\gamedcs\\findpath.cpp:1218\n"
MARK_DECL = "    void markEnemy(long hex, long cost);"
CHECK_DECL = """    unsigned char checkEnemyArmies(long hex, long cost, long currentGroup,
                                     long destination);"""
WALK_CALL = """    combatWalkLimits(currentArmy, inPlacementPhase, &limit,
                       &baseSpeed);"""
SIEGE_CALL = """    unsigned char siegePressure =
        combatSiegePressure(currentArmy, currentGroup);"""
CLEAR_CALL = "    clearCombatCellMarks();"
BUILD_CALL = """    return buildCombatPath(this, currentArmy, startHex, bestHex,
                             destination);"""


def span(source, start, end):
    if source.count(start) != 1 or source.count(end) != 1:
        raise ValueError("Review changed helper anchor: " + start)
    begin = source.index(start)
    return source[begin:source.index(end, begin)]


def replace(source, before, after, count=1):
    if source.count(before) != count:
        raise ValueError("Review changed source: " + before)
    return source.replace(before, after)


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    old_mark = span(source, MARK_START, TELEPORT)
    copied_mark = span(source, COPY_START, CHECK_START)
    old_build = span(source, BUILD_START, WALK_START)
    cluster = span(source, MARK_START, WALK_START)
    helper_options = []
    for mark, member in itertools.product(range(3), range(2)):
        candidate = cluster
        edits = []
        new_mark = old_mark.replace("inline void", "void")
        if mark == 2:
            new_mark = replace(new_mark, """    if (!combatCell->m_validMove || cell->m_cost > cost) {
        combatCell->m_validMove = 1;
        cell->m_cost = static_cast<unsigned short>(cost);
    }""", """    if (combatCell->m_validMove) {
        if (cell->m_cost <= cost)
            return;
    }
    combatCell->m_validMove = 1;
    cell->m_cost = static_cast<unsigned short>(cost);""")
        if mark:
            candidate = replace(candidate, old_mark, "")
            candidate = replace(candidate, copied_mark, new_mark)
            candidate = replace(candidate, "markEnemySearched(this, ", "markEnemy(", 2)
            candidate = replace(candidate,
                "unsigned char searchArray::checkEnemyArmies(",
                "bool searchArray::checkEnemyArmies(")
            edits.append(dict(source="include/findpath.h", find=CHECK_DECL,
                replace="private:\n" + CHECK_DECL.replace("unsigned char", "bool") + "\npublic:"))
        if member:
            body = old_build[old_build.index("{\n"):]
            body = body.replace("search->", "")
            body = replace(body, """        // Retail calls vector<pathCell*>::insert (0x54d120) at both push
        // sites (+0x672 and +0x734), keeping end() inline. The 2026-09-09
        // whole-TU control preserves those decisions without a depth pin.
        pathCell** tail = m_result.end();
        m_result.insert(tail, 1, stepCell);""",
                "        m_result.push_back(stepCell);")
            new_build = ("// E:\\gamedcs\\findpath.cpp:1136\n"
                "bool searchArray::buildCombatPath(const army* currentArmy,\n"
                "                                  int startHex, int endHex, int destination)\n" + body)
            candidate = replace(candidate, old_build, "")
            before = new_mark if mark else copied_mark
            candidate = replace(candidate, before, new_build + before)
            edits.append(dict(find=BUILD_CALL, replace=
                "    return buildCombatPath(currentArmy, startHex, bestHex, destination);"))
        if mark or member:
            decl = "private:\n"
            if member:
                decl += ("    // Before normalization (function): searchArray::build_combat_path.\n"
                         "    bool buildCombatPath(const army* currentArmy, int startHex,\n"
                         "                         int endHex, int destination);\n")
            decl += MARK_DECL + "\npublic:"
            edits.append(dict(source="include/findpath.h", find=MARK_DECL, replace=decl))
        option = dict(name=f"canonical-mark-{mark}-member-build-{member}")
        if candidate != cluster:
            option["replace"] = candidate
        if edits:
            option["extra_edits"] = edits
        helper_options.append(option)

    prelude = span(source, WALK_START, FIND_START)
    helpers = [span(source, WALK_START, SIEGE_START),
               span(source, SIEGE_START, CLEAR_START),
               span(source, CLEAR_START, FIND_START)]
    operations = []
    for helper in helpers:
        operations.append(helper[helper.index("{\n") + 2:helper.rindex("}\n")].rstrip())
    operations[0] = operations[0].replace("*baseSpeed", "baseSpeed").replace("*limit", "limit")
    operations[1] = operations[1].removesuffix("\n    return siegePressure;")
    prelude_options = []
    for bits in itertools.product(range(2), repeat=3):
        candidate = prelude
        edits = []
        for selected, helper, call, body in zip(bits, helpers,
                (WALK_CALL, SIEGE_CALL, CLEAR_CALL), operations):
            if selected:
                candidate = replace(candidate, helper, "")
                edits.append(dict(find=call, replace=body))
        option = dict(name="restore-prelude-" + "".join(map(str, bits)))
        if candidate != prelude:
            option.update(replace=candidate, extra_edits=edits)
        prelude_options.append(option)
    return dict(schema=1, source=SOURCE, units=["findpath"], evidence=__doc__, axes=[
        dict(name="canonical-helper-ownership", find=cluster, options=helper_options),
        dict(name="restore-caller-operations", find=prelude, options=prelude_options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
