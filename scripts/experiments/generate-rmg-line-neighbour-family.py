#!/usr/bin/env python3
"""Generate real border-coordinate and neighbouring-proxy source lifetimes.

Clear 0x4fa080 computes the lower y before its cached upper bound but stores
that value into the shared point afterward. Compare early/for-clause setup
with a real lower-bound local, and factory versus constructor border proxies.
Point painting 0x4fa3c0 retains compound addition in its first neighbour pass,
then consumes the returned coordinate through the line proxy. Explore real
translated values/references and local compound-add forms; retain canonical
helpers, north-first query order and the complete snapshot before refreshes.
The unretained operator+ wrapper is not a separately proven source call here;
direct compound-add alternatives still call the existing operator+= body.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

SOURCE = "src/rmg_terrain.cpp"
HEADER = "include/rmg.h"
LOWER = "rectangle.m_origin.m_y > 0 ? rectangle.m_origin.m_y - 1 : 0"


def parent():
    spec = importlib.util.spec_from_file_location(
        "rmg_line_entry", Path(__file__).with_name("generate-rmg-line-entry-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def lower_forms():
    yield "early", f"        point.m_y = {LOWER};\n", "        for (; point.m_y < end; ++point.m_y) {"
    yield "loop", "", f"        for (point.m_y = {LOWER}; point.m_y < end; ++point.m_y) {{"
    yield "cached", f"        unsigned int first = {LOWER};\n", "        for (point.m_y = first; point.m_y < end; ++point.m_y) {"


def border_forms():
    tail = "                refreshRmgLinePoint(painter, point);"
    yield "factory_temporary", "            if (painter->at(point).getLand())\n" + tail
    yield "factory_named", ("            TRmgLinePainterTile border = painter->at(point);\n"
                            "            if (border.getLand())\n" + tail)
    yield "constructor_temporary", "            if (TRmgLinePainterTile(painter, point).getLand())\n" + tail
    yield "constructor_named", ("            TRmgLinePainterTile border(painter, point);\n"
                                "            if (border.getLand())\n" + tail)


def clear_forms(original):
    lower = next((row for row in lower_forms() if original.count(row[2]) == 2
                  and (not row[1] or original.count(row[1]) == 2)), None)
    border = next((body for _, body in border_forms() if original.count(body) == 4), None)
    if lower is None or border is None:
        raise ValueError("review both vertical bounds and all four border queries")
    canonical = original.replace(lower[2], list(lower_forms())[0][2])
    if lower[1]:
        canonical = canonical.replace(lower[1], "")
    anchor = "        unsigned int end = rectangle.m_origin.m_y + rectangle.m_size.m_y"
    if canonical.count(anchor) != 2:
        raise ValueError("review the two upper-bound snapshots")
    for (name, prefix, loop), (query, body) in itertools.product(lower_forms(), border_forms()):
        result = canonical.replace(anchor, prefix + anchor)
        result = result.replace(list(lower_forms())[0][2], loop).replace(border, body)
        yield name + "+" + query, result


def queries(expression):
    yield "factory", f"matches[direction] = painter->at({expression}).getLand() == riverType;"
    yield "constructor", f"matches[direction] = TRmgLinePainterTile(painter, {expression}).getLand() == riverType;"


def neighbour_forms():
    offset = "g_tileDirections[direction]"
    sum_value = f"point + {offset}"
    yield "factory_sum", [dict(queries(sum_value))["factory"]]
    yield "named_sum", [f"TRmgGridPoint nearby = {sum_value};", dict(queries("nearby"))["factory"]]
    yield "named_sum_reference", [f"const TRmgGridPoint& nearby = {sum_value};", dict(queries("nearby"))["factory"]]
    yield "named_proxy", [f"TRmgLinePainterTile nearby = painter->at({sum_value});",
                          "matches[direction] = nearby.getLand() == riverType;"]
    yield "named_sum_factory_named", [f"TRmgGridPoint nearby = {sum_value};",
                                      "TRmgLinePainterTile neighbour = painter->at(nearby);",
                                      "matches[direction] = neighbour.getLand() == riverType;"]
    yield "named_sum_constructor_named", [f"TRmgGridPoint nearby = {sum_value};",
                                          "TRmgLinePainterTile neighbour(painter, nearby);",
                                          "matches[direction] = neighbour.getLand() == riverType;"]
    yield "named_sum_constructor_temporary", [f"TRmgGridPoint nearby = {sum_value};",
                                              dict(queries("nearby"))["constructor"]]
    yield "direct_initialized_sum", [f"TRmgGridPoint nearby({sum_value});", dict(queries("nearby"))["factory"]]
    for (construction, statements), use in itertools.product((
            ("copy", ["TRmgGridPoint nearby(point);"]),
            ("copy_initialized", ["TRmgGridPoint nearby = point;"]),
            ("assigned", ["TRmgGridPoint nearby;", "nearby = point;"]),
            ("coordinates", ["TRmgGridPoint nearby(point.m_x, point.m_y);"])), ("separate", "operand")):
        for query, statement in queries("nearby" if use == "separate" else f"nearby += {offset}"):
            body = [*statements]
            if use == "separate":
                body.append(f"nearby += {offset};")
            yield construction + "+" + use + "+" + query, [*body, statement]


def point_forms(original):
    anchor = "    for (direction = 0; direction < TILE_DIR_COUNT; ++direction) {\n"
    if original.count(anchor) != 2:
        raise ValueError("review the two ordered neighbour passes")
    first = original.index(anchor)
    second = original.index(anchor, first + len(anchor))
    previous = original[first:second]
    original_loop = (anchor + "        if (available[direction])\n"
                     "            matches[direction] = painter->at(point + g_tileDirections[direction]).getLand() == riverType;\n"
                     "        else\n            matches[direction] = 0;\n    }\n")
    forms = []
    for name, statements in neighbour_forms():
        body = (anchor + "        if (available[direction]) {\n"
                + "".join("            " + statement + "\n" for statement in statements)
                + "        } else {\n            matches[direction] = 0;\n        }\n    }\n")
        forms.append((name, body))
    if previous not in [original_loop, *(body for _, body in forms)]:
        raise ValueError("review the complete neighbour snapshot loop")
    for name, body in forms:
        yield name, original.replace(previous, body)


def make_axes(header, source):
    helper = parent().parent().helpers()
    clear = helper.definition(source, "clearRmgLineRectangle")
    point = helper.definition(source, "TRmgLineWalker::paintPoint")
    return [helper.axis("border_lifetime", SOURCE, clear, clear_forms(clear)),
            helper.axis("neighbour_value", SOURCE, point, point_forms(point))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / HEADER).read_text(), (HOMM3_DIR / SOURCE).read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes, evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
