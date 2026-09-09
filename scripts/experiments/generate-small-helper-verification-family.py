"""Bounded, function-specific release-verification controls for four helpers.

Each entry was inspected with DC show/asm/inline-clues and retail sema
summary/structure/source before this family was authored. Gaps permit the
invariant hypotheses below; they do not recover assertion text. Compare an
unchanged pin, removal alone, and actual pointer/index preconditions. No
dummy work or new override is generated. These are separate TU families,
not a Cartesian search across unrelated helpers.
"""
import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CASES = {
    "cursor": dict(unit="advmgr", address="0x0040e280",
        retained="    HOMM3_RELEASE_VERIFY(currCell != 0);\n",
        evidence="getNormalCursor, called by processHover/getGarrisonCursor; "
                 "DC advmgr.cpp:4531..4532 gap before the cell access",
        checks=[("nonnull", ["currCell != 0"])]),
    "purchase": dict(unit="ai_player", address="0x0042d690",
        retained="    HOMM3_RELEASE_VERIFY(newArmy != 0);\n"
                 "    HOMM3_RELEASE_VERIFY(newFunds != 0);\n",
        evidence="doPurchase, called by markTown/buyCreatures/valueOfHiring; "
                 "DC ai_player.cpp:2627..2628 gap before input stores; "
                 "newAdjacentArmy is optional and is deliberately not asserted",
        checks=[("army-nonnull", ["newArmy != 0"]),
                ("both-nonnull", ["newArmy != 0 && newFunds != 0"]),
                ("split-nonnull", ["newArmy != 0", "newFunds != 0"])]),
    "compress": dict(unit="remote", address="0x005532b0",
        retained="    HOMM3_RELEASE_VERIFY(netMsg != 0 && netMsg->m_size >= sizeof(CNetMsg));\n",
        evidence="compressMsg, called by transmitRemoteDataDPID; "
                 "DC remote.cpp:426 gap before the message size read; "
                 "retail subtracts its 20-byte header from the unsigned size",
        checks=[("nonnull", ["netMsg != 0"]),
                ("header-size", ["netMsg != 0 && netMsg->m_size >= sizeof(CNetMsg)"]),
                ("split-header-size", ["netMsg != 0", "netMsg->m_size >= sizeof(CNetMsg)"])]),
    "spellbonus": dict(unit="hero", address="0x004e5ff0",
        retained="    HOMM3_RELEASE_VERIFY(m_id >= 0);\n"
                 "    HOMM3_RELEASE_VERIFY(m_id < sizeof(g_heroSpecificAbilities)\n"
                 "        / sizeof(g_heroSpecificAbilities[0]));\n",
        evidence="getHeroSpellBonus, called by modifySpellDamage; "
                 "DC hero.cpp:6429..6430 gap before the bonus initialization; "
                 "retail indexes the hero ability array on every path",
        checks=[("hero-index", ["m_id >= 0 && m_id < "
                 "sizeof(g_heroSpecificAbilities) / sizeof(g_heroSpecificAbilities[0])"]),
                ("split-hero-index", ["m_id >= 0", "m_id < "
                 "sizeof(g_heroSpecificAbilities) / sizeof(g_heroSpecificAbilities[0])"])]),
}


def make_manifest(name):
    case = CASES[name]
    source = "src/" + case["unit"] + ".cpp"
    text = (ROOT / source).read_text()
    marker = "VA(" + case["address"] + ","
    if text.count(marker) != 1:
        raise ValueError("Review changed function claim: " + name)
    claim = text.index(marker)
    start = claim
    end = text.index("\n}", claim) + len("\n}")
    # Accept the inspected historical pin or the adopted verification, and
    # refuse a third boundary state. Do not swallow a neighbouring function.
    if text[end:].startswith("\n#pragma auto_inline(on)"):
        end += len("\n#pragma auto_inline(on)")
        if case["unit"] != "ai_player":
            start = text.rfind("#pragma auto_inline(off)", 0, claim)
    body = text[start:end]
    pinned = body.count("#pragma auto_inline(off)") == 1
    verified = body.count(case["retained"]) == 1
    if pinned == verified or body.count("VA(") != 1:
        raise ValueError("Review changed override boundary: " + name)
    unpinned = body.replace("#pragma auto_inline(off)\n", "").replace(
        "\n#pragma auto_inline(on)", "")
    if verified:
        unpinned = unpinned.replace(case["retained"], "")
    options = [dict(name="unchanged"), dict(name="remove-only", replace=unpinned)]
    for label, expressions in case["checks"]:
        comment = "    // Probe: " + case["evidence"] + ".\n"
        comment += "    // Removal-only is the negative control; no assertion text is claimed.\n"
        check = "".join("    HOMM3_RELEASE_VERIFY(" + expression + ");\n" for expression in expressions)
        replacement = unpinned.replace("\n{\n", "\n{\n" + comment + check, 1)
        options.append(dict(name=label, replace=replacement))
    return dict(schema=1, source=source, units=[case["unit"]],
                evidence=__doc__ + "\n" + case["evidence"],
                axes=[dict(name=name + "-verification", find=body, options=options)])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("case", choices=CASES)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(args.case), indent=2) + "\n")
