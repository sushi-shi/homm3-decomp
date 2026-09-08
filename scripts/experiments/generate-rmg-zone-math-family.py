#!/usr/bin/env python3
"""Generate zone-distance lifetimes and prototype-selection result families.

Retail 0x532bd0 and 0x53ad60 square signed coordinate differences, convert
the integer sum to double, call sqrt and truncate through _ftol. Preserve
those operations and the complete position-copy/helper boundaries. Vary
only the real arithmetic intermediates and independent evaluation order.
0x546040 retains its candidate vector until after selecting one pointer;
keep the filtering loop and vary the result/count lifetimes around rand.
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


def distance_forms(left, right, indent):
    for order, subtraction, squares in itertools.product(
            ("xy", "yx"), ("expression", "split", "grouped"),
            ("expression", "reverse", "named_xy", "named_yx",
             "sum_xy", "sum_yx", "inplace_xy", "inplace_yx")):
        lines = []
        for field in order:
            lhs, rhs = left + "m_" + field, right + "m_" + field
            if subtraction == "expression":
                lines.append(f"int d{field} = {lhs} - {rhs};")
            else:
                lines.append(f"int d{field} = {lhs};")
                if subtraction == "split":
                    lines.append(f"d{field} -= {rhs};")
        if subtraction == "grouped":
            lines += [f"d{field} -= {right}m_{field};" for field in order]
        expression = "dx * dx + dy * dy"
        if squares == "reverse":
            expression = "dy * dy + dx * dx"
        elif squares.startswith("named_"):
            lines += [f"int d{field}Squared = d{field} * d{field};" for field in squares[-2:]]
            expression = "dxSquared + dySquared"
        elif squares.startswith("sum_"):
            first, second = squares[-2:]
            lines += [f"int squaredDistance = d{first} * d{first};",
                      f"squaredDistance += d{second} * d{second};"]
            expression = "squaredDistance"
        elif squares.startswith("inplace_"):
            lines += [f"d{field} *= d{field};" for field in squares[-2:]]
            expression = "dx + dy"
        lines.append(f"int distance = static_cast<int>(sqrt(static_cast<double>({expression})));")
        yield "+".join((order, subtraction, squares)), "".join(indent + line.rstrip() + "\n" for line in lines)


def connection_tail():
    minimum = ("        int minimumSize = thisSize;\n"
               "        if (otherSize < minimumSize)\n"
               "            minimumSize = otherSize;\n")
    returning = "        return combinedSize - distance > minimumSize / 2;\n"
    final = "    return 11 * combinedSize >= 10 * distance;\n}"
    # These alternatives keep the proven branch-local value-select minimum.
    for sum_form, difference, comparison in itertools.product(
            ("expression", "this_first", "other_first"),
            ("expression", "named", "compound"), ("expression", "named", "byte")):
        body = "    int otherSize = other->m_slot->m_size;\n    int thisSize = m_slot->m_size;\n"
        if sum_form == "expression":
            body += "    int combinedSize = thisSize + otherSize;\n"
        else:
            first, second = ("thisSize", "otherSize") if sum_form == "this_first" else ("otherSize", "thisSize")
            body += f"    int combinedSize = {first};\n    combinedSize += {second};\n"
        body += ("    if (other->m_levelPosition.m_z != m_levelPosition.m_z) {\n"
                 "        if (combinedSize < distance)\n            return 0;\n" + minimum)
        if difference == "expression":
            body += returning
        elif difference == "named":
            body += ("        int clearance = combinedSize - distance;\n"
                     "        return clearance > minimumSize / 2;\n")
        else:
            body += "        combinedSize -= distance;\n        return combinedSize > minimumSize / 2;\n"
        body += "    }\n"
        if comparison == "expression":
            body += final
        elif comparison == "named":
            body += ("    int permittedDistance = 11 * combinedSize;\n"
                     "    int measuredDistance = 10 * distance;\n"
                     "    return permittedDistance >= measuredDistance;\n}")
        else:
            body += "    unsigned char connected = 11 * combinedSize >= 10 * distance;\n    return connected;\n}"
        yield "+".join((sum_form, difference, comparison)), body


def selector_results():
    for count, selection, result in itertools.product(
            ("member", "named", "const"), ("expression", "index", "random"), ("expression", "named")):
        lines = []
        size = "candidates.size()"
        if count != "member":
            const = "const " if count == "const" else ""
            lines.append(f"    {const}unsigned int count = candidates.size();")
            size = "count"
        lines += [f"    if (!{size})", "        return 0;"]
        index = f"rand() % {size}"
        if selection == "index":
            lines.append(f"    unsigned int selected = {index};")
            index = "selected"
        elif selection == "random":
            lines.append("    int random = rand();")
            index = f"random % {size}"
        expression = f"candidates[{index}]"
        if result == "named":
            lines.append(f"    TRmgObjectPropertiesRef* selectedProperties = {expression};")
            expression = "selectedProperties"
        lines += [f"    return {expression};", "}"]
        yield "+".join((count, selection, result)), "\n".join(lines)


def make_axes(source):
    helper = helpers()
    connected = helper.definition(source, "TRmgZone::canConnect")
    begin = connected.index("\n{\n") + 3
    end = connected.index("    int otherSize =", begin)
    placed = helper.definition(source, "type_random_map_generator::canPlaceZone")
    marker = "        TRmgMapPosition otherPosition = otherZone->getLevelPosition();\n"
    placed_begin = placed.index(marker) + len(marker)
    placed_end = placed.index("        if (10 * distance", placed_begin)
    selector = helper.definition(source, "type_random_map_generator::selectObjectPrototype")
    selector_end = selector.index("    }\n", selector.index("        candidates.push_back(properties);")) + 6
    axes = [helper.axis("connection_distance", "src/rmg.cpp", connected[begin:end],
                        distance_forms("m_levelPosition.", "other->m_levelPosition.", "    ")),
            helper.axis("connection_sizes", "src/rmg.cpp", connected[end:], connection_tail()),
            helper.axis("placement_distance", "src/rmg.cpp", placed[placed_begin:placed_end],
                        distance_forms("otherPosition.", "position.", "        ")),
            helper.axis("prototype_result", "src/rmg.cpp", selector[selector_end:], selector_results())]
    return axes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / "src/rmg.cpp").read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes, evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
