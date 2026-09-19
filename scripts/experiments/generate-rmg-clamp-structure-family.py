#!/usr/bin/env python3
"""Test one canonical reference clamp against its retained body and expansions.

DC includes.h:124 (dc:0x20d2c) records three const-reference inputs, a
const-reference return, two tests and three selected argument addresses.
Retail's retained 0x4e6750 body agrees with maximum < value; six expanded
RMG diagonal sites require the opposite comparison orientation in the current
source. Changing that comparison alone loses six existing exact consumers.
Test natural return/control-flow forms before assuming a second helper.
No interface, inline qualifier, helper body ownership or caller is changed.
"""
import argparse
import json
from pathlib import Path
import subprocess

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

SOURCE = "include/includes.h"
HEAD = "template <class T>\ninline const T& tLimit(const T& minimum, const T& value,\n                       const T& maximum)\n"


def definition(source):
    start = source.index(HEAD)
    end = source.index("\n}", start) + 2
    return source[start:end]


def variants():
    for upper in ("maximum < value", "value > maximum"):
        bodies = (
            """    if (value < minimum) {
        return minimum;
    } else if (UPPER) {
        return maximum;
    } else {
        return value;
    }""",
            """    if (value < minimum)
        return minimum;
    if (UPPER)
        return maximum;
    return value;""",
            """    return value < minimum ? minimum :
        (UPPER ? maximum : value);""",
            """    const T& selected = value < minimum ? minimum :
        (UPPER ? maximum : value);
    return selected;""",
            """    const T* selected;
    if (value < minimum)
        selected = &minimum;
    else if (UPPER)
        selected = &maximum;
    else
        selected = &value;
    return *selected;""",
            """    if (value < minimum) {
        return minimum;
    } else {
        const T& selected = UPPER ? maximum : value;
        return selected;
    }""",
        )
        for index, body in enumerate(bodies):
            yield dict(name=("maximum_left" if upper.startswith("maximum") else "value_left")
                       + f"+return_{index}", replace=HEAD + "{\n" + body.replace("UPPER", upper) + "\n}")


def consuming_units():
    # Use the actual VC6 dependency graph, including indirect header callers.
    deps = subprocess.check_output(["ninja", "-t", "deps"], cwd=HOMM3_DIR, text=True)
    units = []
    for block in deps.split("\n\n"):
        lines = block.splitlines()
        if lines and str(HOMM3_DIR / SOURCE) in [line.strip() for line in lines[1:]]:
            units.append(Path(lines[0].split(":", 1)[0]).stem)
    if "rmg_terrain" not in units or "hero" not in units:
        raise ValueError("fresh full VC6 dependency graph required")
    return sorted(units)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    original = definition((HOMM3_DIR / SOURCE).read_text())
    options = list(variants())
    assert original == options[0]["replace"]
    units = consuming_units()
    payload = dict(schema=1, units=units, evidence=__doc__, axes=[dict(
        name="canonical_clamp_structure", source=SOURCE, find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(f"12 canonical clamp structures across {len(units)} consuming units")


if __name__ == "__main__":
    main()
