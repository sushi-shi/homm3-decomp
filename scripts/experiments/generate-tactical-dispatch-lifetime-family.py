"""Bound the tactical dispatcher's remaining mass-arm register choices.

The canonical unfenced dispatcher already has retail's CFG and ordered call
stream. DC group-damage row 973 computes the target address before row 975's
damage call, and mass row 1132 carries the friendly-group/effect calls.
Test real target bindings, returned damage and mass-argument lifetimes without
changing the backwards scan, either helper's declaration, or the summon call.
These are C++ lifetime hypotheses, not a source-line or SH4 shape comparator.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index("\n}\n", start) + 2]


def option(name, original, replacement):
    result = dict(name=name)
    if replacement != original:
        result["replace"] = replacement
    return result


def make_manifest():
    source = (ROOT / "src/ai_tactical.cpp").read_text()
    group = function(source, "long type_AI_spellcaster::getGroupDamageValue(")
    loop = """    while (count--)
        total += getDamageValue(spell, baseDamage, targetHero,
                                  &g_combatManager->m_armies[group][count]);"""
    if group.count(loop) != 1:
        raise ValueError("Review canonical backwards group loop")
    loops = [loop,
             """    while (count--) {
        const army* target = &g_combatManager->m_armies[group][count];
        total += getDamageValue(spell, baseDamage, targetHero, target);
    }""",
             """    while (count--) {
        const army& target = g_combatManager->m_armies[group][count];
        total += getDamageValue(spell, baseDamage, targetHero, &target);
    }""",
             """    while (count--) {
        long damage = getDamageValue(spell, baseDamage, targetHero,
                                    &g_combatManager->m_armies[group][count]);
        total += damage;
    }"""]
    group_options = [option(label, group, group.replace(loop, replacement))
                     for label, replacement in zip(
                         ("direct-target-control", "named-target-pointer", "named-target-reference", "named-damage-result"), loops)]
    mass = function(source, "void type_AI_spellcaster::considerMassDamage(")
    friendly = """    long friendlyDamage = getGroupDamageValue(choice.m_spell, baseDamage,
                                                  m_side, m_ourHero);
    choice.m_value = getMassDamageEffect(enemyDamage, friendlyDamage);"""
    if mass.count(friendly) != 1:
        raise ValueError("Review ordered friendly/effect source boundary")
    direct = mass.replace(friendly, """    choice.m_value = getMassDamageEffect(enemyDamage,
        getGroupDamageValue(choice.m_spell, baseDamage, m_side, m_ourHero));""")
    declared_friend = mass.replace("    long enemyDamage = ", "    long friendlyDamage;\n    long enemyDamage = ")
    declared_friend = declared_friend.replace("    long friendlyDamage = ", "    friendlyDamage = ")
    declared_pair = mass.replace("    long baseDamage =\n", "    long enemyDamage;\n    long friendlyDamage;\n    long baseDamage =\n")
    declared_pair = declared_pair.replace("    long enemyDamage = ", "    enemyDamage = ")
    declared_pair = declared_pair.replace("    long friendlyDamage = ", "    friendlyDamage = ")
    mass_options = [option(label, mass, replacement) for label, replacement in (
        ("named-results-control", mass), ("direct-friendly-result", direct),
        ("friendly-declaration-first", declared_friend), ("result-declarations-at-entry", declared_pair))]
    return dict(schema=1, source="src/ai_tactical.cpp", units=["ai_tactical"], evidence=__doc__, axes=[
        dict(name="group-target-lifetime", find=group, options=group_options),
        dict(name="mass-result-lifetimes", find=mass, options=mass_options),
    ])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
