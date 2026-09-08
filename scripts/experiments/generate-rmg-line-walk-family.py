#!/usr/bin/env python3
"""Generate source alternatives for the shared river/road two-axis walk.

Retail 0x4fa2b0 selects pointers to two three-dword axis records and walks
from the destination back toward the stored position. Keep that algorithm,
unsigned comparisons, all three point-painting sites and their visit order.
Vary real axis construction, reference/value parameters, major/minor selection,
loop form and point lifetime. All three RMG TUs are compiled and scored.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_source_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def axis_constructors():
    for parameter, comparison, stores, position in itertools.product(
            ("value", "reference"), ("le", "gt"), ("distance_first", "step_first"),
            ("initializer", "body_first", "body_last")):
        kind = "unsigned int" if parameter == "value" else "const unsigned int&"
        lines = [f"    TRmgLineWalkAxis({kind} destination, {kind} previous)\n"]
        if position == "initializer":
            lines.append("        : m_position(destination)\n")
        lines.append("    {\n")
        if position == "body_first":
            lines.append("        m_position = destination;\n")
        arms = [("previous - destination", "1"), ("destination - previous", "-1")]
        if comparison == "gt":
            arms.reverse()
        lines.append(f"        if (destination {'<=' if comparison == 'le' else '>'} previous) {{\n")
        for index, (distance, step) in enumerate(arms):
            statements = [f"            m_distance = {distance};\n",
                          f"            m_step = {step};\n"]
            if stores == "step_first":
                statements.reverse()
            lines.extend(statements)
            if index == 0:
                lines.append("        } else {\n")
        lines.append("        }\n")
        if position == "body_last":
            lines.append("        m_position = destination;\n")
        lines.append("    }")
        yield "+".join((parameter, comparison, stores, position)), "".join(lines)


def walk_setups():
    axes = dict(x="    TRmgLineWalkAxis x(destination.m_x, m_position.m_x);\n",
                y="    TRmgLineWalkAxis y(destination.m_y, m_position.m_y);\n")
    for order, selection in itertools.product(("xy", "yx"), ("ge", "lt", "ternary", "swapped")):
        lines = [axes[key] for key in order]
        if selection == "ternary":
            lines.extend([
                "    TRmgLineWalkAxis* major = x.m_distance >= y.m_distance ? &x : &y;\n",
                "    TRmgLineWalkAxis* minor = x.m_distance >= y.m_distance ? &y : &x;\n"])
        elif selection == "swapped":
            lines.extend([
                "    TRmgLineWalkAxis* major = &x;\n",
                "    TRmgLineWalkAxis* minor = &y;\n",
                "    if (x.m_distance < y.m_distance) {\n",
                "        major = &y;\n",
                "        minor = &x;\n",
                "    }\n"])
        else:
            first, second = ("x", "y") if selection == "ge" else ("y", "x")
            op = ">=" if selection == "ge" else "<"
            lines.extend([
                "    TRmgLineWalkAxis* major;\n",
                "    TRmgLineWalkAxis* minor;\n",
                f"    if (x.m_distance {op} y.m_distance) {{\n",
                f"        major = &{first};\n",
                f"        minor = &{second};\n",
                "    } else {\n",
                f"        major = &{second};\n",
                f"        minor = &{first};\n",
                "    }\n"])
        yield order + "+" + selection, "".join(lines)


def point_call(form, indent):
    constructor = "TRmgGridPoint(x.m_position, y.m_position)"
    if form == "temporary":
        return indent + "paintPoint(" + constructor + ");\n"
    statements = {
        "named": ["TRmgGridPoint point(x.m_position, y.m_position);"],
        "reference": ["const TRmgGridPoint& point = " + constructor + ";"],
        "assigned": ["TRmgGridPoint point;", "point = " + constructor + ";"],
    }[form]
    # Each point belongs to exactly one paint call, just like the temporary.
    return (indent + "{\n" + "".join(indent + "    " + line + "\n"
                                      for line in statements + ["paintPoint(point);"])
            + indent + "}\n")


def walk_loops():
    for loop, point, order in itertools.product(
            ("down_for", "down_while", "up", "up_cached"),
            ("temporary", "named", "reference", "assigned"), ("position_first", "error_first")):
        lines = ["    unsigned int error = 0;\n"]
        if loop == "down_for":
            lines.append("    for (unsigned int remaining = major->m_distance; remaining; --remaining) {\n")
        elif loop == "down_while":
            lines.extend(["    unsigned int remaining = major->m_distance;\n",
                          "    while (remaining) {\n"])
        else:
            bound = "major->m_distance"
            if loop == "up_cached":
                lines.append("    unsigned int distance = major->m_distance;\n")
                bound = "distance"
            lines.append(f"    for (unsigned int index = 0; index < {bound}; ++index) {{\n")
        lines.append(point_call(point, "        "))
        lines.extend(["        error += minor->m_distance;\n",
                      "        if (error >= major->m_distance) {\n"])
        statements = ["            minor->m_position += minor->m_step;\n",
                      "            error -= major->m_distance;\n"]
        if order == "error_first":
            statements.reverse()
        lines.extend(statements)
        lines.append(point_call(point, "            "))
        lines.extend(["        }\n", "        major->m_position += major->m_step;\n"])
        if loop == "down_while":
            lines.append("        --remaining;\n")
        lines.extend(["    }\n", "    if (error + minor->m_distance >= major->m_distance)"])
        # Named/reference variants provide their own compound statement.
        lines.extend(["\n", point_call(point, "        ")])
        lines.append("    m_position = destination;\n")
        yield "+".join((loop, point, order)), "".join(lines)


def make_axes(header, source, *, refine=False):
    helper = helpers()
    axis = helper.definition(header, "TRmgLineWalkAxis")
    body = helper.definition(source, "TRmgLineWalker::drawTo")
    start = body.index("\n{\n") + 3
    split = body.index("    unsigned int error = 0;", start)
    setup, loop = body[start:split], body[split:-1]
    constructors, setups, loops = list(axis_constructors()), list(walk_setups()), list(walk_loops())
    for current, choices in ((axis, constructors), (setup, setups), (loop, loops)):
        if current not in dict(choices).values():
            raise ValueError("review the current two-axis walk before generating")
    axes = [helper.axis("walk_axis", "include/rmg.h", axis, constructors),
            helper.axis("walk_setup", "src/rmg_terrain.cpp", setup, setups),
            helper.axis("walk_loop", "src/rmg_terrain.cpp", loop, loops)]
    if refine:
        # The broad winner keeps every instruction except the major/minor
        # arm polarity. Cross the reproduced reference-argument parents with
        # both arm orders and the cached-count loop, retaining the authored
        # source as the zero control. This finite follow-up is exhausted.
        keep = [
            {"baseline", "reference+le+distance_first+initializer",
             "reference+le+distance_first+body_first", "reference+le+step_first+initializer"},
            {"baseline", "xy+ge", "xy+lt"},
            {"baseline"}
            | {"down_for+" + point + "+error_first"
               for point in ("temporary", "named", "reference", "assigned")}
            | {"up_cached+" + point + "+" + order
               for point, order in itertools.product(
                   ("temporary", "named", "reference", "assigned"),
                   ("position_first", "error_first"))},
        ]
        for item, names in zip(axes, keep):
            item["options"] = [option for option in item["options"] if option["name"] in names]
    return axes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--refine", action="store_true",
                        help="exhaust the reference-axis/cached-count winner's nearby source combinations")
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / "include/rmg.h").read_text(),
                     (HOMM3_DIR / "src/rmg_terrain.cpp").read_text(), refine=args.refine)
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes, evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
