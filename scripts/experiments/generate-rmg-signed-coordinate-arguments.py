#!/usr/bin/env python3
"""Test argument ownership at the two expanded signed-coordinate constructors.

Retail insetIslandZone 0x53d1c0 expands TRmgVector construction and its
arithmetic, but the scalar captures around the retained length call differ.
The unsigned grid constructor independently proves coordinate-reference
parameters in the RMG geometry surface. Neither signed constructor has a
retained symbol proving value parameters; test that hypothesis without
changing the retained arithmetic signatures, field order or caller source.
The two coordinates of TPoint and TRmgVector each admit value or const-reference
ownership. These sixteen states score all seven header consumers. RMG has no
Dreamcast compiland, so this is a retail hypothesis, not a recovered signature.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = []
    for owner in ("TPoint", "TRmgVector"):
        original = "    " + owner + "(int newX, int newY) : m_x(newX), m_y(newY) {}"
        options = []
        for refs in itertools.product((False, True), repeat=2):
            replacement = original
            for name, reference in zip(("newX", "newY"), refs):
                if reference:
                    replacement = replacement.replace("int " + name, "const int& " + name)
            options.append({"name": "ownership_" + "".join("r" if r else "v" for r in refs),
                            "replace": replacement})
        axes.append({"name": owner + "_arguments", "find": original, "options": options})
    payload = {"schema": 1, "source": "include/rmg.h", "evidence": __doc__,
               "units": ["rmg", "rmg_support", "rmg_terrain", "tiles",
                         "singleselectionpopups", "singleselectionwindow", "scenarioinfo"],
               "axes": axes}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("16 signed-coordinate argument ownership states")


if __name__ == "__main__":
    main()
