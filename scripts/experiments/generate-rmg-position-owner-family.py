#!/usr/bin/env python3
"""Revalidate an earlier positive map-position constructor ownership recovery.

Commit 8acabff0 places the retained 0x5355c0 constructor immediately before
canFitObject in rmg.cpp, exposing its ordinary body to its expanding callers.
The current support-TU split hides that body. Cross the two ownership forms
with original and historical appendZonePositions; do not copy unrelated prior
changes or add inline declarations. Cross-TU claim migration is not scored by
the frozen targets: verify the retained constructor raw, then full-delink any
adoption before reporting its score.
"""
import argparse
import json
import subprocess
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    module = generator("generate-rmg-append-zone-family.py")
    main_source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    support = (HOMM3_DIR / "src/rmg_support.cpp").read_text()
    original = module.definition(main_source)
    historical_source = subprocess.check_output(
        ["git", "show", "8acabff0:src/rmg.cpp"], cwd=HOMM3_DIR, text=True)
    historical = module.definition(historical_source)
    assert historical.count("m_map.m_size.m_z") == 1
    historical = historical.replace("m_map.m_size.m_z", "m_map.m_numberLevels")
    extract = generator("generate-rmg-position-family.py").definition
    constructor = extract(support, "TRmgMapPosition::TRmgMapPosition")
    assert constructor == extract(historical_source, "TRmgMapPosition::TRmgMapPosition")
    claimed = "VA(0x005355C0, 0x1A)\n" + constructor + "\n\n"
    assert support.count(claimed) == 1
    anchor = "// Complete-only group fit predicate, recovered on decomp-complete-4.0 in\n"
    assert main_source.count(anchor) == 1
    options = []
    for ownership in ("support", "rmg"):
        for caller, body in (("original", original), ("historical", historical)):
            row = dict(name=ownership + "+" + caller, replace=body)
            if ownership == "rmg":
                row["extra_edits"] = [
                    dict(source="src/rmg_support.cpp", find=claimed, replace=""),
                    dict(source="src/rmg.cpp", find=anchor, replace=claimed + anchor),
                ]
            options.append(row)
    payload = dict(schema=1, units=["rmg", "rmg_support"], evidence=__doc__, axes=[dict(
        name="position_constructor_owner", source="src/rmg.cpp", find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("4 constructor-owner/caller states")


if __name__ == "__main__":
    main()
