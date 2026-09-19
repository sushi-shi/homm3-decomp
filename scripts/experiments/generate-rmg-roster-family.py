#!/usr/bin/env python3
"""Natural registration lifetimes and reverse-loop initialization at 0x538b10.

Retail has the same unrolled allocation order and four repeated regions as
the authored initializer. The remaining differences include several adjacent
vector inline frontiers, a nested begin(), and the object-84 constructor.
RMG has no DC counterpart. Preserve constructors, canonical push_back calls,
version/size read locations, registration order, and all actual loop bodies.
"""
import argparse
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from experiments._support import generator

SOURCE = "src/rmg.cpp"
NAME = "type_random_map_generator::initializeObjectGenerators"
REGISTER = re.compile(r"(?m)^( +)m_objectGenerators\.push_back\(\s*new (\w+)\(([^()]*)\)\);")


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def replace(body, old, new, count=1):
    if body.count(old) != count:
        raise ValueError("review roster anchor: " + old)
    return body.replace(old, new)


def bound_forms(body, form):
    creature = "int creatureCount = m_mapVersion >= 1 ? 145 : 118;"
    dwelling = "int dwelling = 80;\n    if (m_mapVersion < 1)\n        dwelling = 58;"
    if form == 0:
        return body
    if form == 1:
        return replace(body, dwelling, "int dwelling = m_mapVersion >= 1 ? 80 : 58;")
    if form in (2, 3):
        if form == 2:
            new = "int creatureCount = 145;\n        if (m_mapVersion < 1)\n            creatureCount = 118;"
        else:
            new = "int creatureCount = 118;\n        if (m_mapVersion >= 1)\n            creatureCount = 145;"
            body = replace(body, dwelling,
                "int dwelling = 58;\n    if (m_mapVersion >= 1)\n        dwelling = 80;")
        return replace(body, creature, new, 2)
    body = replace(body, creature + "\n        for (int creature = creatureCount; creature--;)",
        "for (int creature = m_mapVersion >= 1 ? 145 : 118; creature--;)", 2)
    return replace(body, dwelling, "int dwelling = m_mapVersion >= 1 ? 80 : 58;")


def reverse_forms(body, form):
    if form == 0:
        return body
    if form == 1:
        for name, count in (("creature", 2), ("player", 1), ("dwelling", 1)):
            body = replace(body, name + "--;)", "--" + name + " >= 0;)", count)
        return body
    if form == 2:
        # Explicit last-index initialization, retaining the pre-loop player
        # count used by resize and the dwelling version selection.
        body = replace(body, "for (; player--;)", "for (--player; player >= 0; --player)")
        body = replace(body, "for (; dwelling--;)", "for (--dwelling; dwelling >= 0; --dwelling)")
        return re.sub(r"for \(int creature = ([^;]+); creature--;\)",
                      r"for (int creature = (\1) - 1; creature >= 0; --creature)", body)
    body = replace(body, "for (; player--;) {", "while (player != 0) {\n            --player;")
    body = replace(body, "for (; dwelling--;)\n        m_objectGenerators.push_back(new type_map_dwelling_def(dwelling));",
        "while (dwelling != 0) {\n        --dwelling;\n        m_objectGenerators.push_back(new type_map_dwelling_def(dwelling));\n    }")
    return re.sub(r"for \(int creature = ([^;]+); creature--;\) \{",
        r"int creature = \1;\n        while (creature != 0) {\n            --creature;", body)


def registration_forms(body, form):
    if form == 0:
        return body
    def registration(match):
        indent, kind, args = match.groups()
        local_type = "type_treasure_def" if form == 1 else kind
        return (indent + "{\n" + indent + "    " + local_type + "* objectGenerator = new "
                + kind + "(" + args + ");\n" + indent
                + "    m_objectGenerators.push_back(objectGenerator);\n" + indent + "}")
    return REGISTER.sub(registration, body)


def variants(original):
    for bound, reverse, registration in itertools.product(range(5), range(4), range(3)):
        body = registration_forms(reverse_forms(bound_forms(original, bound), reverse), registration)
        yield dict(name=f"bounds_{bound}+reverse_{reverse}+registration_{registration}", replace=body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    original = definition((HOMM3_DIR / SOURCE).read_text())
    options = list(variants(original))
    assert options[0]["replace"] == original
    assert len(options) == len({x["replace"] for x in options}) == 60
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="roster_construction", source=SOURCE, find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), "roster states")


if __name__ == "__main__":
    main()
