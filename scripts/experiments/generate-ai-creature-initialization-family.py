"""Recover initialization order without reviving the false vector owner.

DC initialize_creatures (0x29f58) names unit, speed_bonus, hit_points,
force_modifier, archery_modifier and hit_bonus in the outer scope. Its first
body statement (line 222) copies base_modifier to force_modifier, before the
tactics stores; retail 0x424120 likewise copies both argument dwords in the
prologue. The current assignment after tactics is the negative control.

At DC line 325 the actual vector begin/end feed std::sort. Retail expands
sort but retains its nested _Unguarded_partition, which the current native
vector reconstruction over-expands. Vary only real argument lifetimes:
direct member calls, a reference to that same vector, or named iterators
constructed immediately before sort. Preserve all six evidenced outer
locals, the real vendor interfaces, and the canonical combat helpers.
No private-field adapter, helper clone, pragma, dummy work or false inline.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/ai_combat.cpp").read_text()
    start = source.index("void type_AI_combat_data::initializeCreatures(")
    end = source.index("    if (m_myHero) {\n        long attack", start)
    original = source[start:end]
    if original.count("    double forceModifier;\n") != 1 or original.count("    forceModifier = baseModifier;\n") != 1:
        raise ValueError("Review initializeCreatures' force-modifier lifetime")
    early = original.replace("    forceModifier = baseModifier;\n", "")
    assignment = early.replace("    m_tacticsAdvantage = 0;", "    forceModifier = baseModifier;\n    m_tacticsAdvantage = 0;")
    initializer = early.replace("    double forceModifier;", "    double forceModifier = baseModifier;")
    sort = "    std::sort(m_creatures.begin(), m_creatures.end());"
    if source.count(sort) != 1:
        raise ValueError("Review the canonical vector sort call")
    return dict(schema=1, source="src/ai_combat.cpp", units=["ai_combat"], evidence=__doc__, axes=[
        dict(name="force-modifier-initialization", find=original, options=[
            dict(name="after-tactics-control"),
            dict(name="before-tactics-assignment", replace=assignment),
            dict(name="declaration-initializer", replace=initializer),
        ]),
        dict(name="sort-argument-lifetimes", find=sort, options=[
            dict(name="direct-vendor-calls"),
            dict(name="vector-reference", replace="    std::vector<type_monster_data>& creatures = m_creatures;\n"
                 "    std::sort(creatures.begin(), creatures.end());"),
            dict(name="named-iterators", replace="    std::vector<type_monster_data>::iterator first = m_creatures.begin();\n"
                 "    std::vector<type_monster_data>::iterator last = m_creatures.end();\n"
                 "    std::sort(first, last);"),
        ]),
    ])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
