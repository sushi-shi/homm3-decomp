#!/usr/bin/env python3
"""Diagnostic parameter ownership for the canonical zone-position setter.

The current by-value boundary was inferred from three pre-store source loads.
appendZonePositions now matches every call decision but its middle setter has
two extra moves. Test whether a borrowed immutable coordinate explains that
copy while retaining the same ordinary helper at every caller. Existing
by-value evidence and all sibling callers must be reviewed before adoption;
this family does not establish reference ownership from a score alone.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

UNITS = ["rmg", "rmg_support", "rmg_terrain", "scenarioinfo",
         "singleselectionpopups", "singleselectionwindow", "tiles"]
NAME = "TRmgZone::setLevelPosition"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def variants(original):
    old_signature = "void TRmgZone::setLevelPosition(TRmgMapPosition position)"
    assert original == old_signature + "\n{\n    m_levelPosition = position;\n}"
    for parameter in ("TRmgMapPosition", "const TRmgMapPosition&", "const TRmgMapPosition"):
        for stores in ("whole", *map("".join, itertools.permutations("xyz"))):
            body = "    m_levelPosition = position;" if stores == "whole" else "\n".join(
                f"    m_levelPosition.m_{axis} = position.m_{axis};" for axis in stores)
            row = dict(name=parameter + "+" + stores,
                       replace=f"void TRmgZone::setLevelPosition({parameter} position)\n{{\n{body}\n}}")
            if parameter != "TRmgMapPosition":
                row["extra_edits"] = [dict(source="include/rmg.h",
                    find="    void setLevelPosition(TRmgMapPosition position);",
                    replace=f"    void setLevelPosition({parameter} position);")]
            yield row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    original = definition((HOMM3_DIR / "src/rmg.cpp").read_text())
    rows = list(variants(original))
    assert len(rows) == len({row["replace"] for row in rows}) == 21
    args.output.write_text(json.dumps(dict(schema=1, units=UNITS, evidence=__doc__, axes=[dict(
        name="zone_setter_ownership", source="src/rmg.cpp", find=original, options=rows)]), indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("21 canonical setter states")


if __name__ == "__main__":
    main()
