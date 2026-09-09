"""Recover the tactical spell dispatcher's canonical mass/summon boundaries.

DC ai_tactical.cpp:3129 and 3167 call consider_mass_damage (0x3de90)
and consider_summon (0x41e5c), both const members taking a writable choice
reference. The former calls get_group_damage_value (0x3dabc) twice and
get_mass_damage_effect (0x3db2c); its second group/effect calls share line
1132. Summon's line 3098 calls get_mastery_value (0x3d56c), and line 3107
writes cast_now after the kills-only split. Retail 0x43bb20 expands these
boundaries but retains the mass-effect call. Its current forced-inline
mass helper and effect pin are controls, not recovered source declarations.

Cross inherited/ordinary mass declarations, the group's inherited inline
versus ordinary definition, the existing fence versus deletion, and pasted
versus canonical summon source.
Every choice preserves enemy-before-friendly evaluation, backwards group
scans, canonical helper calls, both summon guards and the final choice store.
No new inline keyword, pragma, duplicate helper or dummy operation is added.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index("\n}\n", start) + 2]


def make_manifest():
    source = (ROOT / "src/ai_tactical.cpp").read_text()
    mass = function(source, "__forceinline void type_AI_spellcaster::considerMassDamage(\n")
    friendly = """    long friendlyDamage = getGroupDamageValue(choice.m_spell, baseDamage,
                                                  m_side, m_ourHero);
"""
    assignment = "    choice.m_value = getMassDamageEffect(enemyDamage, friendlyDamage);"
    if mass.count(friendly) != 1 or mass.count(assignment) != 1:
        raise ValueError("Review mass damage's ordered group calls")
    # Keep the fence in its own non-overlapping axis: changing the mass
    # declaration never changes the effect call or its ordered arguments.
    pin = "#pragma inline_depth(0)\n" + assignment + "\n#pragma inline_depth()"
    prefix = mass[:mass.index("#pragma inline_depth(0)")]
    options = []
    for spelling, declaration in (("forced-control", "__forceinline "),
                                  ("ordinary-helper", "")):
        replacement = prefix.replace("__forceinline ", declaration, 1)
        option = dict(name=spelling)
        if replacement != prefix:
            option["replace"] = replacement
        options.append(option)
    pins = [dict(name="existing-effect-fence"),
            dict(name="natural-effect-call", replace=assignment)]

    group = "inline long type_AI_spellcaster::getGroupDamageValue("
    summon_start = source.index("#if 0  // @carcass\n\n// E:\\gamedcs\\ai_tactical.cpp:3093")
    summon_end = source.index("#endif  // @carcass", summon_start) + len("#endif  // @carcass")
    summon_stub = source[summon_start:summon_end]
    arm_start = source.index("    case SPELL_SUMMON_AIR_ELEMENTAL: {", source.index(
        "void type_AI_spellcaster::considerSpell("))
    arm_end = source.index("    case SPELL_TELEPORT:", arm_start)
    arm = source[arm_start:arm_end]
    table = """g_spellTraits[choice->m_spell].m_masteryBonus[choice->m_mastery]
                     * choice->m_power"""
    if arm.count(table) != 1 or arm.count("choice->m_castNow = 1;") != 1:
        raise ValueError("Review summon mastery and post-split choice write")
    mastery_arm = arm.replace(table, "choice->getMasteryValue() * choice->m_power")
    body_start = mastery_arm.index("        if (m_winLikely)")
    body_end = mastery_arm.rindex("        return;\n    }\n")
    body = mastery_arm[body_start:body_end]
    body = "\n".join(line[4:] if line.startswith("    ") else line
                     for line in body.splitlines()).replace("choice->", "choice.")
    helper = """// E:\\gamedcs\\ai_tactical.cpp:3093
// DC 0x41e5c and dispatcher line 3167 prove this ordinary helper boundary.
// Line 3098 calls get_mastery_value; line 3107 writes cast_now after the split.
DC_ONLY(0x41e5c, 0x78)
void type_AI_spellcaster::considerSummon(type_spell_choice& choice) const
{
""" + body + "\n}"
    declaration = "    void considerSummon(type_spell_choice* choice);"
    summoning = [dict(name="pasted-table-control"),
                 dict(name="pasted-mastery-call", replace=mastery_arm),
                 dict(name="ordinary-summon-helper", replace=(
                     "    case SPELL_SUMMON_AIR_ELEMENTAL:\n"
                     "        considerSummon(*choice);\n        return;\n"), extra_edits=[
                         dict(find=summon_stub, replace=helper),
                         dict(source="include/ai_tactical.h", find=declaration, replace=(
                             "    void considerSummon(type_spell_choice& choice) const;")),
                     ])]
    return dict(schema=1, source="src/ai_tactical.cpp",
                units=["ai_tactical", "ai", "drawing", "ai_combat", "ai_player"],
                evidence=__doc__, axes=[
                    dict(name="mass-helper-declaration", find=prefix, options=options),
                    dict(name="effect-boundary", find=pin, options=pins),
                    dict(name="group-helper-declaration", find=group, options=[
                        dict(name="inherited-inline-control"),
                        dict(name="ordinary-group-helper", replace=group.removeprefix("inline "))]),
                    dict(name="summon-boundary", find=arm, options=summoning),
                ])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
