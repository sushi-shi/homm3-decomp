"""Recover the single mass-damage helper and its real loop-carried value.

DC cast_mass_damage_spell, 0x2ac58, has one body and two calls from
cast_spell at lines 1062/1063. Its line 758 overwrites the running value
with take_damage's return before subtracting that value at line 759.
Retail 0x425bd0 corroborates this at both expansions: the first assigns
EAX to ESI after inline takeDamage; the second assigns EAX to EDI after
the retained call. The inherited two copies omit that assignment.

Retail also rebuilds the element address across getSpellDamage, matching
DC's separate subscript calls; retain the element-reference spelling only
as a control. Both builds seed the running value before getMasteryValue.

Cross these three data-flow/lifetime facts with cloned fenced/unfenced
controls and one canonical helper, with its inherited inline spelling or
ordinary declaration, retaining/removing only the existing first fence.
No extra pragma, invented helper, copied body in the caller or dummy work.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/ai_combat.cpp").read_text()
    header = (ROOT / "include/ai_combat.h").read_text()
    start = source.index("// E:\\gamedcs\\ai_combat.cpp:747\n")
    end = source.index("// E:\\gamedcs\\ai_combat.cpp:768\n", start)
    original = source[start:end]
    primary = original[:original.index("// Caller-specific copy")]
    initializers = """    long damage = choice.getMasteryValue()
                  + g_spellTraits[choice.m_spell].m_powerFactor * choice.m_power;
    long value = 0;"""
    clone_decl = """    // Before normalization (function): type_AI_combat_data::cast_mass_damage_spell_with_damage_call.
    void castMassDamageSpellWithDamageCall(
        // Before normalization (locals): casting_hero.
        type_spell_choice& choice, const hero* castingHero);
"""
    clone_call = "defender.castMassDamageSpellWithDamageCall("
    if (original.count(initializers) != 2 or header.count(clone_decl) != 1
            or source.count(clone_call) != 1):
        raise ValueError("Review the two current mass-damage bodies/declarations")
    models = ("cloned-fenced-control", "cloned-unfenced-negative",
              "canonical-inherited-inline-fenced", "canonical-inherited-inline-unfenced",
              "canonical-ordinary-fenced", "canonical-ordinary-unfenced")
    options = []
    for model, returned_value, subscript, early_zero in itertools.product(range(6), range(2), range(2), range(2)):
        candidate = original if model < 2 else primary
        if returned_value:
            candidate = candidate.replace("        m_totalCombatValue -= monster.takeDamage(value);",
                                          "        value = monster.takeDamage(value);\n"
                                          "        m_totalCombatValue -= value;")
        if subscript:
            candidate = candidate.replace("        type_monster_data& monster = m_creatures[i];\n", "")
            candidate = candidate.replace("monster.", "m_creatures[i].")
        if early_zero:
            candidate = candidate.replace(initializers,
                "    long value = 0;\n" + initializers.replace("\n    long value = 0;", ""))
        if model in (1, 3, 5):
            candidate = candidate.replace("#pragma inline_depth(0)\n", "").replace("#pragma inline_depth()\n", "")
        if model >= 4:
            candidate = candidate.replace("inline void type_AI_combat_data::castMassDamageSpell(",
                                          "void type_AI_combat_data::castMassDamageSpell(")
        option = dict(name=models[model] + ("-returned-value" if returned_value else "-uncapped-control")
                      + ("-resubscript" if subscript else "-element-reference")
                      + ("-early-zero" if early_zero else "-late-zero"))
        if candidate != original:
            option["replace"] = candidate
        if model >= 2:
            option["extra_edits"] = [
                dict(source="include/ai_combat.h", find=clone_decl, replace=""),
                dict(source="src/ai_combat.cpp", find=clone_call, replace="defender.castMassDamageSpell("),
            ]
        options.append(option)
    return dict(schema=1, source="src/ai_combat.cpp", units=["ai_combat", "ai_player"], evidence=__doc__,
                axes=[dict(name="mass-damage-source-boundary", find=original, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
