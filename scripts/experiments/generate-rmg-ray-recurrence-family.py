#!/usr/bin/env python3
"""Error-recurrence lifetime at the ray's inner-loop reload (retail +0x161).

The 60 loop/lookup-receiver forms preserve 99.8683% (nine emitted objects).
Retail reloads the error accumulator before from.x after the neighbour scan.
Move the actual recurrence update between the step body and for-clause, or
give its declaration a wider lifetime; preserve the same error at every step.
Counter and previous-point snapshots are real state, with no dummy accesses.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.test_rmg_families import generator
from homm3.vc6 import source_families


def variants(original):
    for recurrence, counter, snapshot in itertools.product(range(5), range(3), range(4)):
        body = original
        if recurrence == 1:
            body = body.replace("int error = major / 2;", "int error = major / 2 + minor;")
            body = body.replace("for (;;) {", "for (;; error += minor) {")
            body = body.replace("        error += minor;\n", "")
        elif recurrence == 2:
            body = body.replace("int error = major / 2;", "int error = major / 2 + minor;")
            body = body.replace("        error += minor;\n", "")
            end = body.rindex("\n    }")
            body = body[:end] + "\n        error += minor;" + body[end:]
        elif recurrence == 3:
            body = body.replace("    int major;", "    int error;\n    int major;")
            body = body.replace("    int error = major / 2;", "    error = major / 2;")
        elif recurrence == 4:
            body = body.replace("    int error = major / 2;\n    int steps = 0;", "    int steps = 0;\n    int error = major / 2;")
        if counter == 1:
            body = body.replace("    int steps = 0;", "    int steps = 1;")
            body = body.replace("        ++steps;\n", "")
            end = body.rindex("\n    }")
            body = body[:end] + "\n        ++steps;" + body[end:]
        elif counter == 2:
            body = body.replace("        ++steps;\n", "")
            body = body.replace("if (steps > 2)", "if (++steps > 2)")
        if snapshot == 1:
            body = body.replace("        toward = from;", "        toward = TPoint(from.m_x, from.m_y);")
        elif snapshot == 2:
            body = body.replace("        toward = from;", "        TPoint previous = from;")
            body = body.replace("return toward;", "return previous;")
        elif snapshot == 3:
            body = body.replace("        toward = from;", "        toward.m_x = from.m_x;\n        toward.m_y = from.m_y;")
        yield f"recurrence_{recurrence}+counter_{counter}+snapshot_{snapshot}", body


def make_axes(source):
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, "type_random_map::traceBranchEnd")
    return [helper.axis("ray_recurrence", "src/rmg.cpp", original, variants(original))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__,
                   axes=make_axes((HOMM3_DIR / "src/rmg.cpp").read_text()))
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "ray recurrence states")


if __name__ == "__main__":
    main()
