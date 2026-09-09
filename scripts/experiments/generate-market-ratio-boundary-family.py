"""Test evidenced accessor, value-lifetime and setup boundaries of market ratios.

DC TSellArtifactWindow::ComputeTradeRatios (dc 0x18afd4, rows 2232..2247)
constructs a type_artifact, assigns hero::get_artifact/get_backpack results,
scales the price, obtains the resource value on row 2240, and divides on
2241. The explicit right-value local is a lifetime hypothesis, not a claim
that a missing CodeView local proves its exact spelling. WindowHandler's
resource selection reaches SetupNewTrade (rows 2961/2990, shared DC tail);
retail retains ComputeTradeRatios at that arm while expanding other copies.

Enumerate the entire 2 x 2 x 3 family. Direct member reads, the flattened
setup and the existing fence are negative controls. No invented assertion,
alternate declaration, false inline or new suppression is introduced.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/tradpost.cpp").read_text()
    branch = """        artifact = g_marketHero->m_equipped[inLeftResource];
    else
        artifact = g_marketHero->m_backpack[inLeftResource - 18];"""
    values = """    float result =
        leftValue / static_cast<float>(g_marketValues[inRightResource]);"""
    setup = """#pragma inline_depth(0)
                    computeTradeRatios(g_selectedArtifact, g_leftResource,
                        &g_giveQuantity, &g_ratioInverted, &g_maxTradeUnits);
#pragma inline_depth()
                    g_rightAmount = 1;"""
    for anchor in (branch, values, setup):
        if source.count(anchor) != 1:
            raise ValueError("Review ratio-boundary anchor: " + anchor)
    return dict(schema=1, source="src/tradpost.cpp", units=["tradpost"], evidence=__doc__,
                axes=[
                    dict(name="artifact-accessors", find=branch, options=[
                        dict(name="flattened-control"),
                        dict(name="canonical-calls", replace=branch.replace(
                            "m_equipped[inLeftResource]", "getArtifact(inLeftResource)").replace(
                            "m_backpack[inLeftResource - 18]", "getBackpack(inLeftResource - 18)"))]),
                    dict(name="resource-value-lifetime", find=values, options=[
                        dict(name="direct-operand"),
                        dict(name="named-float-value", replace=(
                            "    float rightValue = static_cast<float>(g_marketValues[inRightResource]);\n"
                            "    float result = leftValue / rightValue;"))]),
                    dict(name="resource-selection-boundary", find=setup, options=[
                        dict(name="fenced-flattened-control"),
                        dict(name="unfenced-flattened-control", replace=setup.replace(
                            "#pragma inline_depth(0)\n", "").replace("#pragma inline_depth()\n", "")),
                        dict(name="canonical-setup-call", replace="                    setupNewTrade();")])])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
