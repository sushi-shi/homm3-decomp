"""Restore the accessor population around canonical combat-path helpers.

The helper-only family exposes large FindCombatPath inline differences.
DC positively names army::Is, get_owning_side, get_spell_time,
OffsetToFront(-1), ValidHex and searchArray::get_hex in these source arms.
Use those existing canonical definitions, including the range guard inside
check_enemy_armies, not just the flattened operations around a pragma.

Cross the old control and two fully recovered helper parents (compound or
nested mark guard) with four accessor groups. No parent introduces a new
helper or inline-policy override. The get_hex option also corrects its
header declaration to the DC-proven const qualification.

Historical control: use frozen context e39aa0ed39b14fca4d84. The helper
parent's anchors intentionally reject the adopted canonical source state.
"""

import argparse
import importlib.util
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CPP = "src/findpath.cpp"
HEADER = "include/findpath.h"


def replace(source, before, after, count=1):
    if source.count(before) != count:
        raise ValueError("Review accessor anchor: " + before)
    return source.replace(before, after)


def render_parent(manifest, original, choices):
    result = dict(original)
    for axis, choice in zip(manifest["axes"], choices):
        option = axis["options"][choice]
        path = axis.get("source", manifest["source"])
        if "replace" in option:
            result[path] = replace(result[path], axis["find"], option["replace"])
        for edit in option.get("extra_edits", []):
            path = edit.get("source", manifest["source"])
            result[path] = replace(result[path], edit["find"], edit["replace"])
    return result


def make_manifest():
    path = Path(__file__).with_name("generate-findpath-helper-family.py")
    spec = importlib.util.spec_from_file_location("helpers", path)
    parent = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(parent)
    manifest = parent.make_manifest()
    original = {name: (ROOT / name).read_text() for name in (CPP, HEADER)}
    options = []
    for parent_choice in ([0, 0], [3, 7], [5, 7]):
        seed = render_parent(manifest, original, parent_choice)
        for predicates, geometry, validity, cells in itertools.product(range(2), repeat=4):
            source = seed[CPP]
            header = seed[HEADER]
            begin = source.index("VA(0x004b3400,")
            end = source.index("// THE ONE RETAIL ROW WITH NO DREAMCAST TWIN.", begin)
            old_main = source[begin:end]
            main = old_main
            begin = source.index("unsigned char searchArray::checkEnemyArmies(") if not parent_choice[0] else source.index("bool searchArray::checkEnemyArmies(")
            end = source.index("\n}\n", begin) + 3
            old_check = source[begin:end]
            check = old_check
            if predicates:
                check = replace(check, """    if (enemy == 0 || enemy->m_combatSide == currentGroup)
        return 0;""", """    if (enemy == 0)
        return 0;
    if (enemy->getOwningSide() == currentGroup)
        return 0;""")
                check = replace(check, "enemy->m_monInfo.m_attributes & 1", "enemy->is(1)")
                main = replace(main, "currentArmy->m_monInfo.m_attributes & 1", "currentArmy->is(1)", 3)
                main = replace(main, """((static_cast<unsigned>(currentArmy->m_monInfo.m_attributes) >> 1)
                            & 1)""", "currentArmy->is(2)")
            if geometry:
                main = replace(main, "currentArmy->m_facing ? 1 : -1", "currentArmy->offsetToFront(-1)", 3)
            if validity:
                check = replace(check, "    const army* enemy =", """    if (!combatManager::validHex(hex))
        return 0;
    const army* enemy =""")
                main = replace(main, "destination >= 0 && destination < COMBAT_GRID_CELLS", "combatManager::validHex(destination)")
                main = replace(main, "adjacent < 0 || adjacent >= COMBAT_GRID_CELLS", "!combatManager::validHex(adjacent)")
                main = replace(main, """if (tail >= 0 && tail < COMBAT_GRID_CELLS
                                && checkEnemyArmies(tail, enemyCost,
                                                      currentGroup,
                                                      destination))""", """if (checkEnemyArmies(tail, enemyCost,
                                             currentGroup, destination))""")
            source = replace(source, old_main, main)
            source = replace(source, old_check, check)
            if geometry:
                source = replace(source, "currentArmy->m_spellInfluence[72]", "currentArmy->getSpellTime(72)")
            if validity:
                source = replace(source, "destination < 0 || destination >= COMBAT_GRID_CELLS", "!combatManager::validHex(destination)")
            if cells:
                # All of these are in the measured combat-path helper chain.
                source = replace(source, "pathCell* cell = getCellData(hex);", "pathCell* cell = getHex(hex);")
                if not parent_choice[0]:
                    source = replace(source, "pathCell* cell = search->getCellData(hex);", "pathCell* cell = search->getHex(hex);")
                    source = replace(source, "pathCell* stepCell = search->getCellData(endHex);", "pathCell* stepCell = search->getHex(endHex);")
                else:
                    source = replace(source, "pathCell* stepCell = getCellData(endHex);", "pathCell* stepCell = getHex(endHex);")
                source = replace(source, "pathCell* reached = getCellData(adjacent);", "pathCell* reached = getHex(adjacent);")
                header = replace(header, "    pathCell* getHex(long x)\n", "    pathCell* getHex(long x) const\n")
            option = dict(name="parent-" + "-".join(map(str, parent_choice))
                + f"-predicates-{predicates}-geometry-{geometry}-validity-{validity}-cells-{cells}")
            if source != original[CPP]:
                option["replace"] = source
            if header != original[HEADER]:
                option["extra_edits"] = [dict(source=HEADER, find=original[HEADER], replace=header)]
            options.append(option)
    return dict(schema=1, source=CPP, units=["findpath"], evidence=__doc__,
                axes=[dict(name="helper-and-accessor-boundaries", find=original[CPP], options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
