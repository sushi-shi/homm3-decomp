#!/usr/bin/env python3
"""Test Zone special-member ownership with derived-generator EH and callers.

Implicit Zone cleanup now emits after the derived generator destructor.
That caller retains an extra EH state store and five unwind actions absent
retail. The historical ordinary empty Zone destructor at its retail source
position made both retained bodies exact. Compare those two genuine ownership
models jointly with the newly exact boundary expansion and all header users.
No inline/exception specification, copied cleanup or dummy caller is added.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    claim = "VA_COMPGEN(0x00532B50, 0x76, IMPLICIT_DTOR, TRmgZone)"
    declaration = "    TRmgZone(TRmgTownSlot* slot);\n    void chooseTerrain();\n"
    assert (HOMM3_DIR / "src/rmg.cpp").read_text().count(claim) == 1
    assert (HOMM3_DIR / "include/rmg.h").read_text().count(declaration) == 1
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain", "scenarioinfo",
        "singleselectionpopups", "singleselectionwindow", "tiles"], evidence=__doc__,
        axes=[dict(name="zone_destructor_definition", source="src/rmg.cpp", find=claim, options=[
            dict(name="implicit", replace=claim),
            dict(name="ordinary_definition", replace="VA(0x00532B50, 0x76)\nTRmgZone::~TRmgZone()\n{\n}",
                 extra_edits=[dict(source="include/rmg.h", find=declaration,
                                   replace=declaration + "    ~TRmgZone();\n")])])])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("2 Zone special-member ownership states across seven header consumers")


if __name__ == "__main__":
    main()
