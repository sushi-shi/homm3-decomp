#!/usr/bin/env python3
"""Generate 60 lifetime hypotheses for the existing 98.0374% zone predicate.

Retail 0x53ad60 agrees through both zone-position reads and differs only in the
signed subtraction/square block. Keep dy-then-dx evaluation, the full position
accessors/copies, integer squares, double sqrt and truncation. Earlier algebraic
forms plateaued; cross real declaration order with position-copy and square
lifetimes. Do not replace the distance with a squared threshold comparison.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source, hypotheses

FUNCTION = "?canPlaceZone@type_random_map_generator@@QAEEPAUTRmgZone@@@Z"


def fragments():
    for copying, declaration, squares in itertools.product(
            ("copy", "direct", "assigned"),
            ("inline", "xy_after", "yx_after", "xy_before", "x_before"),
            ("expression", "named_xy", "named_yx", "inplace")):
        lines = []
        if declaration in ("xy_before", "x_before"):
            lines.append("int dx;")
        if declaration == "xy_before":
            lines.append("int dy;")
        if copying == "copy":
            lines.append("TRmgMapPosition otherPosition = otherZone->getLevelPosition();")
        elif copying == "direct":
            lines.append("TRmgMapPosition otherPosition(otherZone->getLevelPosition());")
        else:
            lines += ["TRmgMapPosition otherPosition;", "otherPosition = otherZone->getLevelPosition();"]
        if declaration.endswith("after"):
            lines += ["int d" + axis + ";" for axis in declaration[:2]]
        lines += [("int " if declaration in ("inline", "x_before") else "")
                  + "dy = otherPosition.m_y - position.m_y;",
                  ("int " if declaration == "inline" else "")
                  + "dx = otherPosition.m_x - position.m_x;"]
        expression = "dx * dx + dy * dy"
        if squares.startswith("named"):
            lines += [f"int d{axis}Squared = d{axis} * d{axis};" for axis in squares[-2:]]
            expression = "dxSquared + dySquared"
        elif squares == "inplace":
            lines += ["dx *= dx;", "dy *= dy;"]
            expression = "dx + dy"
        lines.append(f"int distance = static_cast<int>(sqrt(static_cast<double>({expression})));")
        yield "+".join((copying, declaration, squares)), "".join("        " + line + "\n" for line in lines)


def make_manifest(source):
    definitions = _source.find_definitions(source, FUNCTION)
    if len(definitions) != 1:
        raise ValueError("review the unique zone placement predicate")
    definition = definitions[0]
    body = source[definition.body_open:definition.body_close + 1]
    options = list(fragments())
    matches = [fragment for _, fragment in options if fragment in body]
    if len(matches) != 1:
        raise ValueError("review the zone-distance block before rebasing the family")
    original = matches[0]
    options.sort(key=lambda row: row[1] != original)
    return dict(schema=1, unit="rmg", function=FUNCTION, evidence=__doc__, axes=[dict(
        name="zone_distance_lifetimes", find=original,
        options=[dict(name=name, replace=fragment) for name, fragment in options])])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = make_manifest((HOMM3_DIR / "src/rmg.cpp").read_text())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    parsed = hypotheses.parse_manifest(args.output)
    print("generated", len(hypotheses.variants(parsed[4], parsed[5])), "unique source hypotheses ->", args.output)


if __name__ == "__main__":
    main()
