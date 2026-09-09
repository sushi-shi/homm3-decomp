"""Recover canonical AI min/max ownership and ordinary valuation helpers.

The ai_combat DC calls name includes.h's int min/max (0x2da4, 0x1ef28),
which return a value after calling the const-reference _cpp_min/_cpp_max
selectors (0x3b88, 0x20d04). Those wrappers already live in homm3_minmax.h.
Retail getFastestSpeed 0x4249a1 copies both arguments and selects their
addresses, corroborating the two layers, not a reference to a dead parameter.
The current local by-value/reference-returning templates conflate them.

Cross that recovered interface with ordinary getMassDamageValue (DC
0x2ac18) and one-side getEnchantmentValue (0x2ace4) definitions. Their
inherited inline keywords were justified by retail expansion, not source
evidence. Keep each canonical body and its calls, including the corrected
mass-damage loop. No pins, copied caller code or dummy compiler-state work.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/ai_combat.cpp").read_text()
    start = source.index("// VC6's own <xutility> reference-returning min/max")
    end = source.index("// The mutually exclusive AI-dispatch family", start)
    old = source[start:end]
    if old.count("inline const _TYPE&") != 2:
        raise ValueError("Review the inherited local min/max signatures")
    calls = [
        "cppMin(static_cast<long>(value * m_combatValuePerHit) / m_value,",
        "cppMin(static_cast<long>(damage * m_combatValuePerHit), m_totalValue)",
        "cppMin(attack, enemyDefense)",
        "cppMin(defense, enemyAttack)",
        "cppMax(fastest, m_creatures[i].m_speed)",
    ]
    for call in calls:
        if source.count(call) != 1:
            raise ValueError("Review min/max source call: " + call)
    axes = [dict(name="minmax-owner", find=old, options=[
        dict(name="invalid-local-reference-return-control"),
        dict(name="canonical-value-wrappers", replace='#include "homm3_minmax.h"\n\n', extra_edits=[
            dict(find=call, replace=call.replace("cppMin(", "min(").replace("cppMax(", "max("))
            for call in calls
        ]),
    ])]
    for label, signature in (
        ("mass-valuation", "inline void type_AI_combat_data::getMassDamageValue(\n"),
        ("enchantment-valuation", "inline void type_AI_combat_data::getEnchantmentValue(type_spell_choice& choice, const hero* castingHero) const\n"),
    ):
        if source.count(signature) != 1:
            raise ValueError("Review canonical helper: " + signature)
        axes.append(dict(name=label, find=signature, options=[
            dict(name="inherited-inline-control"),
            dict(name="ordinary-source-helper", replace=signature.removeprefix("inline ")),
        ]))
    return dict(schema=1, source="src/ai_combat.cpp", units=["ai_combat", "ai_player"],
                evidence=__doc__, axes=axes)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
