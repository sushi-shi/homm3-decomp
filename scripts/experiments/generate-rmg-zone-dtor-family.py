#!/usr/bin/env python3
"""Check the compiler-owned TRmgZone destructor and all header consumers.

Retail 0x532b50 destroys entrances, boundary and distances in reverse member
order, with no user cleanup. The IMPLICIT_DTOR claim replaced the empty written
definition, but the header retained a declaration that prevents implicit
definition. This finite family compares the current declaration with its
removal; it adds no special-member body or inlining directive.
"""
import argparse
import json
from pathlib import Path
import subprocess

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    relative = "include/rmg.h"
    if "VA_COMPGEN(0x00532B50, 0x76, IMPLICIT_DTOR, TRmgZone)" not in (HOMM3_DIR / "src/rmg.cpp").read_text():
        raise ValueError("this historical family requires the implicit-destructor claim baseline")
    anchor = "    TRmgZone(TRmgTownSlot* slot);\n    void chooseTerrain();\n    ~TRmgZone();\n"
    if (HOMM3_DIR / relative).read_text().count(anchor) != 1:
        raise ValueError("review zone declaration anchor")
    deps = subprocess.check_output(["ninja", "-t", "deps"], cwd=HOMM3_DIR, text=True)
    units = []
    for block in deps.split("\n\n"):
        lines = block.splitlines()
        if lines and str(HOMM3_DIR / relative) in [line.strip() for line in lines[1:]]:
            units.append(Path(lines[0].split(":", 1)[0]).stem)
    if "rmg" not in units:
        raise ValueError("fresh VC6 dependency graph required")
    payload = dict(schema=1, units=sorted(units), evidence=__doc__, axes=[dict(
        name="zone_destructor_ownership", source=relative, find=anchor, options=[
            dict(name="stale_user_declaration", replace=anchor),
            dict(name="compiler_owned", replace=anchor.replace("    ~TRmgZone();\n", ""))])])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(f"2 destructor ownership states across {len(units)} consuming units")


if __name__ == "__main__":
    main()
