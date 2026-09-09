"""Recover resurrection's staged scalar result through canonical min.

DC 0x29b94 lines 99..102 scales/truncates, divides by per-creature value,
calls the by-value min wrapper, then multiplies the returned scalar. Retail
0x423d00 loads the selected operand into EAX before imul EAX,EBX. The current
single return expression instead retains the selected address in ECX and
multiplies through it, although CFG and all named calls already agree.

Keep the canonical min owner and test only actual quotient/capped-value
lifetimes, including reuse of the existing long value. All variants retain
the truncation before integer division, cap, and final multiplication.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/ai_combat.cpp").read_text()
    old = """    return min(static_cast<long>(value * m_combatValuePerHit) / m_value,
                    m_originalNumber - m_number) * m_value;"""
    if source.count(old) != 1:
        raise ValueError("Review the canonical resurrection value expression")
    choices = [
        ("single-expression-control", old),
        ("named-capped-value", """    long resurrected = min(static_cast<long>(value * m_combatValuePerHit) / m_value,
                           m_originalNumber - m_number);
    return resurrected * m_value;"""),
        ("reuse-capped-value", """    value = min(static_cast<long>(value * m_combatValuePerHit) / m_value,
                m_originalNumber - m_number);
    return value * m_value;"""),
        ("reuse-quotient-and-cap", """    value = static_cast<long>(value * m_combatValuePerHit) / m_value;
    value = min(value, m_originalNumber - m_number);
    return value * m_value;"""),
        ("dc-ordered-separate-stages", """    value = static_cast<long>(value * m_combatValuePerHit);
    value /= m_value;
    value = min(value, m_originalNumber - m_number);
    return value * m_value;"""),
        ("named-quotient-and-cap", """    long resurrected = static_cast<long>(value * m_combatValuePerHit) / m_value;
    resurrected = min(resurrected, m_originalNumber - m_number);
    return resurrected * m_value;"""),
    ]
    options = [dict(name=label, **({"replace": value} if value != old else {})) for label, value in choices]
    return dict(schema=1, source="src/ai_combat.cpp", units=["ai_combat", "ai_player"],
                evidence=__doc__, axes=[dict(name="resurrection-value-lifetime", find=old, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
