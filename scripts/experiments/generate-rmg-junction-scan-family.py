#!/usr/bin/env python3
"""Junction scan back edge and reset-snapshot scope after constructor repair.

Retail 0x5446a0 reloads the minimum X at the outer header; our 77.5209%
body has an extra bound-reload block and a jump into that header. Its row
home also occupies the former bounds minimum Y. Compare meaningful scan
ownership, guarded loops and reset-value declaration scope. Keep the three
matched calls and the canonical reset helper. No Dreamcast mapping exists.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def indent(text, count):
    return "\n".join(" " * count + line if line else line for line in text.splitlines())


def variants(original):
    start = original.index("    for (position.m_y = bounds.m_minimumY;")
    end = original.index("    if (!zone->m_entrances.size())", start)
    kernel_start = original.index("            TRmgMapItem* item", start)
    kernel_end = original.index("\n        }\n    }\n", kernel_start)
    kernel = "\n".join(line[12:] for line in original[kernel_start:kernel_end].splitlines())
    for coordinates, loop, snapshot in itertools.product(range(4), range(5), range(3)):
        prefix = ""
        x, y = "position.m_x", "position.m_y"
        if coordinates == 1:
            prefix = "TPoint scan;\n"
            x, y = "scan.m_x", "scan.m_y"
        elif coordinates == 2:
            prefix = "int x;\nint y;\n"
            x, y = "x", "y"
        elif coordinates == 3:
            prefix = "int x;\n"
            x, y = "x", "bounds.m_minimumY"
        code = kernel.replace("getMapItem(position.m_x, position.m_y, level)",
                              "getMapItem(" + x + ", " + y + ", level)")
        if snapshot:
            declaration = "    TRmgMapPosition previous;\n"
            if code.count(declaration) != 1:
                raise ValueError("changed reset predecessor declaration")
            code = code.replace(declaration, "")
            if snapshot == 1:
                prefix = "TRmgMapPosition previous;\n" + prefix
            else:
                code = "TRmgMapPosition previous;\n" + code
        init_y = "" if coordinates == 3 else y + " = bounds.m_minimumY"
        init_x = x + " = bounds.m_minimumX"
        y_test, x_test = y + " < bounds.m_maximumY", x + " < bounds.m_maximumX"
        if loop == 0:
            scan = "for (" + init_y + "; " + y_test + "; ++" + y + ") {\n"
            scan += "    for (" + init_x + "; " + x_test + "; ++" + x + ") {\n" + indent(code, 8) + "\n    }\n}"
        elif loop == 1:
            scan = (init_y + ";\n" if init_y else "") + "if (" + y_test + ") do {\n"
            scan += "    " + init_x + ";\n    if (" + x_test + ") do {\n" + indent(code, 8)
            scan += "\n    } while (++" + x + " < bounds.m_maximumX);\n} while (++" + y + " < bounds.m_maximumY);"
        elif loop == 2:
            scan = (init_y + ";\n" if init_y else "") + "while (" + y_test + ") {\n"
            scan += "    " + init_x + ";\n    while (" + x_test + ") {\n" + indent(code, 8)
            scan += "\n        ++" + x + ";\n    }\n    ++" + y + ";\n}"
        elif loop == 3:
            scan = (init_y + ";\n" if init_y else "") + "nextRow:\nif (" + y + " >= bounds.m_maximumY) goto scanDone;\n"
            scan += init_x + ";\nnextColumn:\nif (" + x + " >= bounds.m_maximumX) goto rowDone;\n{\n" + indent(code, 4)
            scan += "\n}\n++" + x + ";\ngoto nextColumn;\nrowDone:\n++" + y + ";\ngoto nextRow;\nscanDone:;"
        else:
            scan = "for (" + init_y + "; " + y_test + "; ++" + y + ") {\n"
            scan += "    " + init_x + ";\n    if (" + x_test + ") do {\n" + indent(code, 8)
            scan += "\n    } while (++" + x + " < bounds.m_maximumX);\n}"
        replacement = indent(prefix + scan, 4) + "\n"
        yield "coordinates_%d+loop_%d+snapshot_%d" % (coordinates, loop, snapshot), original[:start] + replacement + original[end:]


def receiver_refinement(parent, form):
    """Retail forms a distinct map receiver at the seed, preserving its owner."""
    if form in (3, 4):
        old = "    TRmgZoneBounds bounds = zone->m_bounds;"
        if parent.count(old) != 1:
            raise ValueError("changed junction bounds snapshot")
        parent = parent.replace(old, "    TRmgZoneBounds bounds;\n    bounds = zone->m_bounds;")
        if form == 3:
            return parent
    if form == 2:
        split = parent.index("{\n") + 2
    else:
        split = parent.index("    TRmgMapPosition first;")
    head, tail = parent[:split], parent[split:]
    if form == 1:
        return head + "    type_random_map* map = &m_map;\n" + tail.replace("m_map.", "map->")
    return head + "    type_random_map& map = m_map;\n" + tail.replace("m_map.", "map.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--receivers-from", type=Path)
    args = parser.parse_args()
    module = generator("generate-rmg-junction-prepare-family.py")
    original = module.authored_definition((HOMM3_DIR / "src/rmg.cpp").read_text())
    if args.receivers_from:
        forms = module.frontier((HOMM3_DIR / "src/rmg.cpp").read_text(), args.receivers_from, receiver_refinement)
    else:
        forms = list(variants(original))
        if forms[0][1] != original:
            raise ValueError("review changed scan before rebasing")
    helper = generator("generate-rmg-position-family.py")
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[
        helper.axis("junction_scan", "src/rmg.cpp", original, forms)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(len(forms), "junction scan states")


if __name__ == "__main__":
    main()
