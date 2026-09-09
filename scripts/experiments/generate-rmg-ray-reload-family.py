#!/usr/bin/env python3
"""Ray-neighbour loop scopes and lookup receiver at retail 0x543c70.

All 23 CFG blocks currently agree; at +0x161 retail restores the persistent
error-accumulator register before reloading from.x, while the candidate reverses
those independent loads. Test ordinary loop/control and lookup lifetimes at
the inner-loop exit, preserving x-major traversal and the canonical lookup.
No Dreamcast procedure counterpart is known for this Complete-only function.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.test_rmg_families import generator
from homm3.vc6 import source_families


def variants(original):
    start = original.index("            TRmgMapPosition nearby;")
    end = original.index("\n        }", start)
    for shape, receiver, polarity in itertools.product(range(6), range(5), range(2)):
        lines = ["TRmgMapPosition nearby;", "nearby.m_z = level;"]
        if shape == 4:
            lines += ["int maximumX = from.m_x + 1;", "int maximumY = from.m_y + 1;"]
        xmax = "maximumX" if shape == 4 else "from.m_x + 1"
        ymax = "maximumY" if shape == 4 else "from.m_y + 1"
        if shape in (1, 3):
            lines += ["nearby.m_x = from.m_x - 1;", f"while (nearby.m_x <= {xmax}) {{"]
        else:
            lines += [f"for (nearby.m_x = from.m_x - 1; nearby.m_x <= {xmax}; ++nearby.m_x) {{"]
        if shape in (2, 3):
            lines += ["    nearby.m_y = from.m_y - 1;", f"    while (nearby.m_y <= {ymax}) {{"]
        else:
            lines += [f"    for (nearby.m_y = from.m_y - 1; nearby.m_y <= {ymax}; ++nearby.m_y) {{"]
        if shape == 5:
            lines += ["        TRmgMapPosition query(nearby.m_x, nearby.m_y, level);"]
        query = "query" if shape == 5 else "nearby"
        access = f"getMapItem({query})->hasSubterraneanGate()"
        declarations = [None, f"TRmgMapItem* item = getMapItem({query});",
                        f"const TRmgMapItem* item = getMapItem({query});",
                        f"TRmgMapItem& item = *getMapItem({query});",
                        f"const TRmgMapItem& item = *getMapItem({query});"]
        if receiver:
            lines += ["        " + declarations[receiver]]
            access = "item" + ("->" if receiver < 3 else ".") + "hasSubterraneanGate()"
        if polarity:
            lines += ["        bool blocked = " + access + ";", "        if (blocked)", "            return toward;"]
        else:
            lines += ["        if (" + access + ")", "            return toward;"]
        if shape in (2, 3):
            lines += ["        ++nearby.m_y;"]
        lines += ["    }"]
        if shape in (1, 3):
            lines += ["    ++nearby.m_x;"]
        lines += ["}"]
        replacement = "\n".join("            " + line for line in lines)
        yield f"loop_{shape}+receiver_{receiver}+predicate_{polarity}", original[:start] + replacement + original[end:]


def make_axes(source):
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, "type_random_map::traceBranchEnd")
    return [helper.axis("ray_reload", "src/rmg.cpp", original, variants(original))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__,
                   axes=make_axes((HOMM3_DIR / "src/rmg.cpp").read_text()))
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "ray reload states")


if __name__ == "__main__":
    main()
