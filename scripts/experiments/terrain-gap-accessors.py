#!/usr/bin/env python3
"""Recover coordinate-value temporaries in the two canonical gap predicates.

Retail repairTerrainPoint 0x5b5440 has the same CFG and all 46 named calls.
Its expanded gap predicates retain the unaffected coordinate in ESI and copy
it into a fresh scalar before each retained TRmgGridPoint reference constructor.
Value-return coordinate accessors can account for that copy. Cross member vs
accessor reads independently for the changing and unaffected coordinates in
each ordinary predicate, preserving interfaces, short circuiting and helpers.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def definition(source, kind):
    head = (f"unsigned char rmgTerrainPainter::is{kind}Gap(\n"
            "    const TRmgGridPoint& point, int terrain)\n{\n")
    start = source.index(head)
    end = source.index("\n}", start) + 2
    return source[start:end]


def variants(original, kind):
    changing = "x" if kind == "Horizontal" else "y"
    fixed = "y" if changing == "x" else "x"
    for change_value, fixed_value in itertools.product((False, True), repeat=2):
        head, calls = original.split("        &&", 1)
        for axis, enabled in ((changing, change_value), (fixed, fixed_value)):
            if enabled:
                calls = calls.replace("point.m_" + axis, "point.get" + axis.upper() + "()")
        yield (f"changing-{'value' if change_value else 'member'}+"
               f"fixed-{'value' if fixed_value else 'member'}", head + "        &&" + calls)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
    axes = []
    for kind in ("Horizontal", "Vertical"):
        original = definition(source, kind)
        axes.append(dict(name=kind.lower(), find=original,
                         options=[dict(name=name, replace=body)
                                  for name, body in variants(original, kind)]))
    payload = dict(schema=1, source="src/rmg_terrain.cpp", units=["rmg_terrain"],
                   evidence=__doc__, axes=axes)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("16 coordinate accessor combinations ->", args.output)


if __name__ == "__main__":
    main()
