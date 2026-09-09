"""Recover natural nested size decisions in the canonical inline bank helper.

DC value_of_bank 0x110808 / philai.cpp:2054 proves the bank reference lookup,
empty flag, ordered combat/resource/reward appraisal and public vector size.
Retail 0x529920 expands size in the retained helper, while the first two bank
arms of aiValueOfEvent (0x528040) call it; the final bank arm calls valueOfBank.
The old shared size fence reproduces those two expanded arms but incorrectly
de-inlines size in the retained helper (92.2043%; removal alone reaches 100%).

The six-view evidence pass and ordered named-call review precede this family.
Keep one canonical inline helper and all three source calls. Vary only actual
bank receiver binding, consumed artifact size/receiver lifetime, combat-value
declaration lifetime and removal of the existing fence. None adds a helper,
new override, invented assertion, repeated read or dummy compiler mass. These
are C++ lifetime hypotheses, not claims of unrecorded original local spellings.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/philai.cpp").read_text()
    start = source.index("inline long valueOfBank(")
    end = source.index("\n}", start) + 2
    original = source[start:end]
    binding = "    type_creature_bank& bank = info->getCreatureBank();"
    tail = """#pragma inline_depth(0)
    value = bank.m_artifacts.size()
        * g_currentPlayer->m_ai.m_turnValueOfAvgArtifact + value;
#pragma inline_depth()"""
    for anchor in (binding, tail, "    long value;\n", "    value = aiValueOfCombat("):
        if original.count(anchor) != 1:
            raise ValueError("Review the bank helper parent anchor: " + anchor)
    tails = [
        tail,
        """#pragma inline_depth(0)
    unsigned int artifactCount = bank.m_artifacts.size();
    value = artifactCount
        * g_currentPlayer->m_ai.m_turnValueOfAvgArtifact + value;
#pragma inline_depth()""",
        """#pragma inline_depth(0)
    const unsigned int& artifactCount = bank.m_artifacts.size();
    value = artifactCount
        * g_currentPlayer->m_ai.m_turnValueOfAvgArtifact + value;
#pragma inline_depth()""",
        """    const std::vector<TArtifact>& artifacts = bank.m_artifacts;
#pragma inline_depth(0)
    value = artifacts.size()
        * g_currentPlayer->m_ai.m_turnValueOfAvgArtifact + value;
#pragma inline_depth()""",
        """    const std::vector<TArtifact>* artifacts = &bank.m_artifacts;
#pragma inline_depth(0)
    value = artifacts->size()
        * g_currentPlayer->m_ai.m_turnValueOfAvgArtifact + value;
#pragma inline_depth()""",
    ]
    options = []
    for bank, lifetime, value, unfenced in itertools.product(range(3), range(5), range(2), range(2)):
        candidate = original.replace(tail, tails[lifetime])
        if bank == 1:
            candidate = candidate.replace(binding, binding.replace("type_creature_bank&", "const type_creature_bank&"))
        elif bank == 2:
            candidate = candidate.replace(binding, "    type_creature_bank* bank = &info->getCreatureBank();")
            candidate = candidate.replace("bank.", "bank->")
        if value:
            candidate = candidate.replace("    long value;\n\n", "")
            candidate = candidate.replace("    value = aiValueOfCombat(", "    long value = aiValueOfCombat(")
        if unfenced:
            candidate = candidate.replace("#pragma inline_depth(0)\n", "").replace("\n#pragma inline_depth()", "")
        option = dict(name="-".join((
            ("bank-reference", "const-bank-reference", "bank-pointer")[bank],
            ("direct-size", "size-value", "size-reference", "vector-reference", "vector-pointer")[lifetime],
            ("entry-value", "initialized-value")[value],
            ("existing-fence", "unfenced")[unfenced],
        )))
        if candidate != original:
            option["replace"] = candidate
        options.append(option)
    return dict(schema=1, source="src/philai.cpp", units=["philai"], evidence=__doc__,
                axes=[dict(name="bank-value-lifetimes", find=original, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
