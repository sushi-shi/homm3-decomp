#!/usr/bin/env python3
"""Generate real control-flow and value-lifetime variants of the line selector.

Retail 0x4f9cb0 has ordered cardinal guards, a four-reflection corner search,
and optional end/corner patterns. Its vertical fallback jumps to a shared
zero-flip exit, while other arms return directly. Preserve all cases and the
three distinct output locals proved by its sole caller at 0x4f9f00; compare
real return/join structure, independent output stores and reflection iteration.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path
import re

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

SOURCE = "src/rmg_support.cpp"
SIGNATURE = "selectRmgLinePattern"


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_source_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def stores(pattern, x, y, order, indent, skip=""):
    values = dict(p=("pattern", pattern), x=("flipX", x), y=("flipY", y))
    return "".join(indent + values[key][0] + " = " + str(values[key][1]) + ";\n"
                   for key in order if key != skip)


def cardinal_group(vertical, style, order):
    opposite = ("NORTH", "SOUTH") if vertical else ("EAST", "WEST")
    branches = ("EAST", "WEST") if vertical else ("SOUTH", "NORTH")
    outputs = [(6, 0, 0), (6, 1, 0), (2, 0, 0)] if vertical else [(7, 0, 0), (7, 0, 1), (3, 0, 0)]
    constant = "y" if vertical else "x"
    result = (f"    if (neighbours[TILE_DIR_{opposite[0]}]"
              f" && neighbours[TILE_DIR_{opposite[1]}]) {{\n")
    if style == "join":
        for condition, values in zip(branches, outputs):
            result += f"        if (neighbours[TILE_DIR_{condition}]) {{\n"
            result += stores(*values, order, "            ") + "            return;\n        }\n"
        return result + "        pattern = 2;\n        goto unflipped;\n    }\n"
    for index, values in enumerate(outputs):
        if index == 0:
            result += f"        if (neighbours[TILE_DIR_{branches[0]}]) {{\n"
        elif index == 1:
            result += f"        }} else if (neighbours[TILE_DIR_{branches[1]}]) {{\n"
        else:
            result += "        } else {\n"
        result += stores(*values, order, "            ", constant if style == "constant" else "")
        if style == "early":
            result += "            return;\n"
    result += "        }\n"
    if style == "constant":
        result += "        " + ("flipY" if vertical else "flipX") + " = 0;\n"
    if style != "early":
        result += "        return;\n"
    return result + "    }\n"


def fallback(form, indent):
    test = "neighbours[TILE_DIR_WEST] || neighbours[TILE_DIR_EAST]"
    if form == "ternary":
        return indent + "pattern = " + test + " ? 3 : 2;\n"
    return (indent + "if (" + test + ")\n" + indent + "    pattern = 3;\n"
            + indent + "else\n" + indent + "    pattern = 2;\n")


def end_cases(form, indent):
    result = indent + "if (neighbours[TILE_DIR_WEST] || neighbours[TILE_DIR_EAST]) {\n"
    result += stores(1, "neighbours[TILE_DIR_WEST]", 0, "pxy", indent + "    ")
    result += indent + "} else {\n" + indent + "    pattern = 0;\n" + indent + "    flipX = 0;\n"
    if form == "ternary":
        result += indent + "    flipY = neighbours[TILE_DIR_SOUTH] ? 0 : 1;\n"
    elif form == "branch":
        result += (indent + "    if (neighbours[TILE_DIR_SOUTH])\n" + indent + "        flipY = 0;\n"
                   + indent + "    else\n" + indent + "        flipY = 1;\n")
    else:
        result += (indent + "    if (neighbours[TILE_DIR_SOUTH]) {\n" + indent + "        flipY = 0;\n"
                   + indent + "        return;\n" + indent + "    }\n" + indent + "    flipY = 1;\n")
    return result + indent + "}\n"


def exit_forms(refine=False):
    cross = ("    if (neighbours[TILE_DIR_NORTH] && neighbours[TILE_DIR_EAST]\n"
             "        && neighbours[TILE_DIR_SOUTH] && neighbours[TILE_DIR_WEST]) {\n"
             + stores(8, 0, 0, "pxy", "        ") + "        return;\n    }\n")
    for vertical, horizontal, vorder, horder, ending, straight in itertools.product(
            ("shared", "early", "join", "constant"), ("shared", "early", "constant"),
            ("pxy", "xpy"), ("pyx", "ypx"), ("ternary", "branch", "returns"),
            ("ternary", "branch")):
        if refine and (vorder != "pxy" or horder != "pyx" or ending == "ternary"):
            continue
        prefix = cross + cardinal_group(True, vertical, vorder) + cardinal_group(False, horizontal, horder)
        if vertical == "join":
            # The real corner-search locals end before the common exit.
            # A goto never bypasses a local's initialization in its own scope.
            prefix += "    {\n"
            tail = "    if (table->m_ranges[0].m_valueCount > 0) {\n"
            tail += end_cases(ending, "        ") + "        return;\n    }\n"
            tail += fallback(straight, "    ") + "    }\nunflipped:\n    flipX = 0;\n    flipY = 0;\n"
        else:
            tail = "    if (table->m_ranges[0].m_valueCount > 0) {\n"
            tail += end_cases(ending, "        ") + "    } else {\n"
            tail += fallback(straight, "        ") + "        flipX = 0;\n        flipY = 0;\n    }\n"
        yield "+".join((vertical, horizontal, vorder, horder, ending, straight)), prefix, tail


def corner_forms(refine=False):
    for kind, comparison, iteration, selection in itertools.product(
            ("unsigned char", "bool", "int"), ("> 0", "!= 0"),
            ("index", "reference", "pointer"), ("branch", "ternary")):
        if refine and (iteration != "index" or selection != "branch"):
            continue
        result = f"    {kind} hasCornerVariant = table->m_ranges[5].m_valueCount {comparison};\n"
        if iteration == "pointer":
            # VC6 misparses the pointer-to-array declarator in a for-init
            # clause; the same ordinary declaration before the loop is valid.
            result += ("    const unsigned char (*reflection)[2] = g_rmgLineReflections;\n"
                       "    for (; reflection < g_rmgLineReflections + 4; ++reflection) {\n")
            x, y = "(*reflection)[0]", "(*reflection)[1]"
        else:
            result += "    for (unsigned int reflection = 0; reflection < 4; ++reflection) {\n"
            x, y = "g_rmgLineReflections[reflection][0]", "g_rmgLineReflections[reflection][1]"
            if iteration == "reference":
                result += "        const unsigned char (&flip)[2] = g_rmgLineReflections[reflection];\n"
                x, y = "flip[0]", "flip[1]"
        result += f"        const int* order = g_rmgLineReflectedNeighbours\n            [{x}][{y}];\n"
        result += "        if (neighbours[order[2]] && neighbours[order[4]]) {\n"
        condition = "hasCornerVariant && (neighbours[order[1]] || neighbours[order[5]])"
        if selection == "branch":
            result += ("            if (" + condition + ")\n                pattern = 5;\n"
                       "            else\n                pattern = 4;\n")
        else:
            result += "            pattern = " + condition + " ? 5 : 4;\n"
        result += f"            flipX = {x};\n            flipY = {y};\n"
        result += "            return;\n        }\n    }\n"
        yield "+".join((kind, comparison, iteration, selection)), result


def make_axes(source, refine=False):
    helper = helpers()
    original = helper.definition(source, SIGNATURE)
    start = original.index("\n{\n") + 3
    flag = re.search(r"    (?:unsigned char|bool|int) hasCornerVariant = [^\n]+;\n", original)
    if flag is None:
        raise ValueError("review the selector's corner availability value")
    opening = original.index("{", original.index("    for (", flag.end()))
    depth, end = 1, opening + 1
    while depth:
        depth += (original[end] == "{") - (original[end] == "}")
        end += 1
    end += original[end:end + 1] == "\n"
    prefix, middle, tail = original[start:flag.start()], original[flag.start():end], original[end:-1]
    options = [dict(name="baseline", replace=prefix)]
    seen = {(prefix, tail)}
    for name, new_prefix, new_tail in exit_forms(refine):
        if (new_prefix, new_tail) in seen:
            continue
        seen.add((new_prefix, new_tail))
        options.append(dict(name=name, replace=new_prefix,
                            extra_edits=[dict(source=SOURCE, find=tail, replace=new_tail)]))
    return [dict(name="selector_exits", source=SOURCE, find=prefix, options=options),
            helper.axis("selector_corners", SOURCE, middle, corner_forms(refine))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--refine", action="store_true", help=(
        "exhaust the retail-ordered cardinal stores and branched end flips "
        "with the retained indexed corner loop"))
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / SOURCE).read_text(), args.refine)
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes, evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
