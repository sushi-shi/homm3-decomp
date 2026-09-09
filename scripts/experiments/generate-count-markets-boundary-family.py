"""Recover ordinary CountMarkets and its canonical HasBuilding call.

DC tradpost.cpp:618 identifies the file-local helper; row 621 calls
game::GetTown then town::HasBuilding(14, true). The ordinary declaration
is tested at that original source position without a false inline keyword.
Retail expands that active-market test in DoArtifactMerchants, the town
Freelancer's Guild entry, and DoMarketplace. Keep the existing canonical
GetTown and public HasBuilding interface, and measure every tradpost row.
The complete four-state family includes the old inline/field-read controls.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    source = (ROOT / "src/tradpost.cpp").read_text()
    declaration = "static inline void countMarkets()"
    condition = "if (currentTown->m_active & g_bitNumber[MARKETPLACE_ID])"
    if source.count(declaration) != 1 or source.count(condition) != 1:
        raise ValueError("Review CountMarkets boundary anchors")
    return dict(schema=1, source="src/tradpost.cpp", units=["tradpost"], evidence=__doc__,
                axes=[
                    dict(name="ordinary-helper", find=declaration, options=[
                        dict(name="inline-control"),
                        dict(name="ordinary", replace="static void countMarkets()")]),
                    dict(name="building-query", find=condition, options=[
                        dict(name="flattened-control"),
                        dict(name="canonical-call", replace="if (currentTown->hasBuilding(MARKETPLACE_ID, true))")])])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
