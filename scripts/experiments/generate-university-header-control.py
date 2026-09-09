"""Isolate shared-header collateral from the university ownership correction.

Run on the adopted POD record. Compare its ordinary initializer declaration
with the old automatic constructor, in consumers which do not call either.
This is a negative control, not a proposal to restore the misattribution.
Full-build inspection found two small tracked losses outside the five
focused initializer units; check those consumers against one frozen target
generation. Other raw object differences are data placement or anonymous
header identities, not altered function bytes. The seven-unit initial
control stops at reproduction on anonymous header identities and data-symbol
placement: the driver deliberately normalizes only .cpp anonymous names,
not the .h spellings or changing symbol offsets in five table/data units.
It is not counted as a successfully searched family.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest():
    header = (ROOT / "include/game.h").read_text()
    declaration = "type_university* initializeMagicSkills();"
    if header.count(declaration) != 1:
        raise ValueError("Review the adopted university declaration")
    return dict(
        schema=1, source="include/game.h", evidence=__doc__,
        units=["army", "singleselectionwindow"],
        axes=[dict(name="generic-university-type", find=declaration,
                   options=[dict(name="adopted-aggregate"),
                            dict(name="old-constructor-control",
                                 replace="type_university();")])])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
