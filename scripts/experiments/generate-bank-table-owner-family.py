"""Recover both creature-bank tables' real enum and function-static owners.

Dreamcast NB11 globals guard_types/reward_types (0x5601/0x5602) are mutable
TCreatureType[11][5]/[11], both owned by procedure record 1828, the loader
at dc:0x7112c. Retail 0x6702a0/0x67037c contains the same 55+11 dwords in
writable .data. Restoring these owners removes the reward's scalar union
adapter, rather than replacing it with another conversion. Six missing
enumerators are positively named by DC and used by those retail table cells.

The finite two-state family checks every current armygrp.h build consumer.
Its baseline is untouched. The recovery changes type, lifetime, original
semantic names, constness and all actual users together. It introduces no
casts, alternate views, helper copies, assertions or inline overrides.
Two of the six creature names already live in ai.h's narrower unrelated
enum. Move them to the canonical domain atomically; leaving duplicate
enumerators makes the opposite-corner compile fail and is not a valid state.
"""

import argparse
import json
from pathlib import Path
import subprocess
import tomllib


ROOT = Path(__file__).resolve().parents[2]

ENUM_VALUES = """    // Creature-bank table owners (DC guard_types/reward_types, 0x5601/0x5602).
    // Complete's 0x6702a0/0x67037c tables corroborate these stored values.
    // Original DC names: eCreatureCyclops, eCreatureImp, eCreatureNagaSentinel,
    // eCreatureDragonFly, eCreatureVampire, eCreatureWyvern.
    CREATURE_CYCLOPS = 94,
    CREATURE_IMP = 42,
    CREATURE_NAGA_SENTINEL = 38,
    CREATURE_DRAGON_FLY = 105,
    CREATURE_VAMPIRE = 62,
    CREATURE_WYVERN = 108,
"""

TABLES = """    // DC procedure 0x7112c owns these two mutable function-static arrays:
    // guard_types (type 0x5601) and reward_types (0x5602), both TCreatureType.
    // Retail keeps all 66 dwords in writable .data, at these exact addresses.
    // Unspecified guard cells after CREATURE_NONE are zero-initialized padding.
    DATA(0x006702a0)
    static TCreatureType guardTypes[CREATURE_BANK_COUNT][5] = {
        { CREATURE_CYCLOPS, CREATURE_NONE },
        { CREATURE_DWARF, CREATURE_NONE },
        { CREATURE_GRIFFIN, CREATURE_NONE },
        { CREATURE_IMP, CREATURE_NONE },
        { CREATURE_MEDUSA, CREATURE_NONE },
        { CREATURE_NAGA_SENTINEL, CREATURE_NONE },
        { CREATURE_DRAGON_FLY, CREATURE_NONE },
        { CREATURE_WIGHT, CREATURE_NONE },
        { CREATURE_WATER_ELEMENTAL, CREATURE_NONE },
        { CREATURE_SKELETON, CREATURE_WALKING_DEAD,
          CREATURE_WIGHT, CREATURE_VAMPIRE, CREATURE_NONE },
        { CREATURE_GREEN_DRAGON, CREATURE_RED_DRAGON,
          CREATURE_GOLD_DRAGON, CREATURE_BLACK_DRAGON, CREATURE_NONE }
    };
    DATA(0x0067037c)
    static TCreatureType rewardTypes[CREATURE_BANK_COUNT] = {
        CREATURE_NONE, CREATURE_NONE, CREATURE_ANGEL, CREATURE_NONE,
        CREATURE_NONE, CREATURE_NONE, CREATURE_WYVERN, CREATURE_NONE,
        CREATURE_NONE, CREATURE_NONE, CREATURE_NONE
    };

"""


def replace(source, old, new, count=1):
    if source.count(old) != count:
        raise ValueError("Review bank table source anchor: " + old)
    return source.replace(old, new)


def consumers():
    configured = {row["unit"] for row in tomllib.loads(
        (ROOT / "config/units.toml").read_text())["unit"]}
    output = subprocess.check_output(["ninja", "-t", "deps"], cwd=ROOT, text=True)
    selected = set()
    for block in output.split("\n\n"):
        lines = block.splitlines()
        if not lines or str(ROOT / "include/armygrp.h") not in (line.strip() for line in lines[1:]):
            continue
        if "(VALID)" not in lines[0]:
            raise ValueError("Run the full build before reading consumers: " + lines[0])
        unit = Path(lines[0].split(":", 1)[0]).stem
        if unit not in configured:
            raise ValueError("Unconfigured header consumer: " + unit)
        selected.add(unit)
    if "creature_bank" not in selected or "game" not in selected:
        raise ValueError("Missing current build dependencies for armygrp.h")
    return sorted(selected)


def make_manifest():
    source = (ROOT / "src/creature_bank.cpp").read_text()
    begin = source.index("// The two per-bank tables crbanks.txt does NOT carry.")
    end = source.index("// E:\\gamedcs\\creature_bank.cpp:25.", begin)
    candidate = source[:begin] + source[end:]
    candidate = replace(candidate, "    int row = 2;\n", TABLES + "    int row = 2;\n")
    candidate = replace(candidate, "        const int* guardTypes = g_creatureBankGuardTypes[bank];",
                        "        const TCreatureType* bankGuardTypes = guardTypes[bank];")
    candidate = replace(candidate, "guardTypes[slot] != -1", "bankGuardTypes[slot] != CREATURE_NONE")
    candidate = replace(candidate, "level->m_guards.m_armies[slot] = guardTypes[slot];",
                        "level->m_guards.m_armyTypes[slot] = bankGuardTypes[slot];")
    candidate = replace(candidate, "level->m_guards.m_armies[guard] = CREATURE_NONE;",
                        "level->m_guards.m_armyTypes[guard] = CREATURE_NONE;")
    candidate = replace(candidate, "            level->m_rewardCreature = creatureTypeFromInt(\n"
                        "                g_creatureBankRewardCreature[bank]);",
                        "            level->m_rewardCreature = rewardTypes[bank];")
    ai_source = (ROOT / "include/ai.h").read_text()
    ai_candidate = replace(ai_source, "    CREATURE_VAMPIRE = 0x3e,\n",
                           "    // CREATURE_VAMPIRE now belongs to TCreatureType: the bank's\n"
                           "    // native guard table and this sample gate share the same domain.\n")
    ai_candidate = replace(ai_candidate, "    CREATURE_SERPENT_FLY = 0x68,\n"
                           "    CREATURE_DRAGON_FLY = 0x69\n",
                           "    // CREATURE_DRAGON_FLY likewise moved to TCreatureType for the\n"
                           "    // native bank guard table; the on-attack evidence above still holds.\n"
                           "    CREATURE_SERPENT_FLY = 0x68\n")
    return dict(schema=1, source="src/creature_bank.cpp", units=consumers(), evidence=__doc__, axes=[
        dict(name="bank-table-type-owner", find=source, options=[
            dict(name="integer-file-static-control"),
            dict(name="native-function-static-tables", replace=candidate, extra_edits=[
                dict(source="include/armygrp.h", find="    CREATURE_WYVERN_MONARCH = 0x6d,\n",
                     replace="    CREATURE_WYVERN_MONARCH = 0x6d,\n" + ENUM_VALUES),
                dict(source="include/ai.h", find=ai_source, replace=ai_candidate)])])])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
