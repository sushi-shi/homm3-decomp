"""Historical pre-adoption family for TViewArmyWindow's source boundaries.

The popup source anchors are from ab315284; the measured six-TU snapshot
already includes the header-request/reset cleanup described in the audit.
The adopted popup deliberately fails these anchors instead of reintroducing
its old int member, copied helper bodies, union adapters or depth fences.

DC CodeView types ArmyType as TCreatureType, but Upgrade and the third
constructor's army_type parameter as int. Preserve those int boundaries.
DC battle constructor lines 70/98/107 call GetName and append both help
strings; retail 0x5f3360 retains string append calls. DC group constructor
line 159 calls GetArmyName(ArmyType, 2), while its help strings use assignment.
The family crosses these corrections with deletion of the two existing
battle-description depth fences, checking all six direct header consumers.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = "src/viewarmywindow.cpp"
HEADER = "include/viewarmywindow.h"
BATTLE_NAME = """    const char* name;
    if (thisArmy->m_creatureType >= 0 && thisArmy->m_creatureType <= 150) {
        if (thisArmy->m_numTroops == 1)
            name = g_creatureTypeTraits[thisArmy->m_creatureType].m_name;
        else
            name = g_creatureTypeTraits[thisArmy->m_creatureType].m_pluralName;
    } else {
        name = g_emptyRolloverText;
    }
    createNameWidget(name);
"""
GROUP_NAME = """    const char* name;
    if (m_armyType >= 0 && m_armyType <= 150)
        name = g_creatureTypeTraits[m_armyType].m_pluralName;
    else
        name = g_emptyRolloverText;
    createNameWidget(name);
"""
SHOWN = """    union {
        // Before normalization: value.
        int m_value;
        // Before normalization: creature.
        TCreatureType m_creature;
    } shownType;
    shownType.m_value = m_armyType;
"""


def replace(source, before, after, count=1):
    if source.count(before) != count:
        raise ValueError("Review changed source: " + before)
    return source.replace(before, after)


def typed_source(source):
    source = replace(source, SHOWN, "", 2)
    source = replace(source, "\n".join("    " + line if line else line
                                     for line in SHOWN.split("\n")), "")
    source = replace(source, "shownType.m_creature", "m_armyType", 4)
    source = replace(source, "creatureTypeFromInt(m_armyType)", "m_armyType", 2)
    source = replace(source, "m_armyType(group->m_armies[iarmy])",
                     "m_armyType(group->m_armyTypes[iarmy])")
    source = replace(source, "m_armyType(armyType)",
                     "m_armyType(creatureTypeFromInt(armyType))")
    source = replace(source, "} armyType, upgradeType;", "} upgradeType;")
    source = replace(source, "                armyType.m_value = m_armyType;\n", "")
    return replace(source, "getUpgradeCost(armyType.m_creature,",
                   "getUpgradeCost(m_armyType,")


def remove_description_pins(source, mask):
    # Select only the two battle-description regions, never the iterator pin.
    before, group = source.split("VA(0x005f3b50,", 1)
    parts = before.split("#pragma inline_depth(0)\n")
    if len(parts) != 3:
        raise ValueError("Review changed battle-description fences")
    result = parts[0]
    for index, part in enumerate(parts[1:]):
        if mask & (1 << index):
            result += replace(part, "#pragma inline_depth()\n", "")
        else:
            result += "#pragma inline_depth(0)\n" + part
    return result + "VA(0x005f3b50," + group


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    header = (ROOT / HEADER).read_text()
    typed_header = replace(header, '#include "advmgr_popup.h"\n',
                           '#include "advmgr_popup.h"\n#include "armygrp.h"\n')
    typed_header = replace(typed_header, "    int m_armyType;\n",
                           "    TCreatureType m_armyType;\n")
    options = []
    for typed, names, append, pins in itertools.product(range(2), range(2),
                                                      range(2), range(4)):
        candidate = remove_description_pins(source, pins)
        if typed:
            candidate = typed_source(candidate)
        if names:
            candidate = replace(candidate, BATTLE_NAME,
                                "    createNameWidget(thisArmy->getName());\n")
            candidate = replace(candidate, GROUP_NAME,
                                "    createNameWidget(getArmyName(m_armyType, 2));\n")
        if append:
            candidate = replace(candidate, "m_moraleHelp = ourGroup->",
                                "m_moraleHelp += ourGroup->")
            candidate = replace(candidate, "m_luckHelp = ourGroup->",
                                "m_luckHelp += ourGroup->")
        option = dict(name=f"enum-{typed}-names-{names}-append-{append}-unpin-{pins}")
        if candidate != source:
            option["replace"] = candidate
        if typed:
            option["extra_edits"] = [dict(source=HEADER, find=header,
                                         replace=typed_header)]
        options.append(option)
    return dict(schema=1, source=SOURCE,
                units=["viewarmywindow", "cmbtmgr", "game", "recruit",
                       "sacrifice_window", "hillfortwindow"], evidence=__doc__,
                axes=[dict(name="proven-source-boundaries", find=source,
                           options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
