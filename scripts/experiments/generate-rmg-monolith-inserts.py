#!/usr/bin/env python3
"""Recover the monolith registry's single-element insertion overloads.

Retail 0x542ce0 passes two arguments to the selected first registry's insert
and to the second object's two-way registry insertion. The one-way branch
and final two exit registrations pass three arguments. Compare insert(end(),
object) with the current push_back independently at the first two sites.
Both APIs append the identical pointer; preserve every object/point lifetime,
allocation, guard and placement call. RMG has no Dreamcast counterpart.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    start = source.index("void type_random_map_generator::createMonolithConnection(")
    current = source[start:source.index("\n}", start) + 2]
    registry = "(exitProperties ? m_monolithsOneWay : m_monolithsTwoWay)"
    first = registry + ".push_back(object);"
    second = "if (!exitProperties)\n            m_monolithsTwoWay.push_back(object);"
    original = current.replace("(!exitProperties ? m_monolithsTwoWay : m_monolithsOneWay).push_back(object);", first)
    original = original.replace("if (!exitProperties)\n            m_monolithsTwoWay.insert(m_monolithsTwoWay.end(), object);", second)
    assert first in original and second in original
    options = [{"name": "source_control", "replace": current}]
    seen = {current}
    for one, two, inverted in itertools.product(("push_back", "insert", "reference", "pointer"), (False, True), (False, True)):
        body = original
        selection = "(!exitProperties ? m_monolithsTwoWay : m_monolithsOneWay)" if inverted else registry
        if one == "push_back":
            replacement = selection + ".push_back(object);"
        elif one == "insert":
            replacement = selection + ".insert(" + selection + ".end(), object);"
        elif one == "reference":
            replacement = "std::vector<type_object*>& monoliths = " + selection + ";\n        monoliths.insert(monoliths.end(), object);"
        else:
            replacement = "std::vector<type_object*>* monoliths = &" + selection + ";\n        monoliths->insert(monoliths->end(), object);"
        body = body.replace(first, replacement)
        if two:
            body = body.replace(second, "if (!exitProperties)\n            m_monolithsTwoWay.insert(m_monolithsTwoWay.end(), object);")
        if body not in seen:
            options.append({"name": "first_" + one + "+second_" + str(two) + "+inverted_" + str(inverted), "replace": body})
            seen.add(body)
    payload = {"schema": 1, "source": "src/rmg.cpp", "units": ["rmg"],
               "evidence": __doc__, "axes": [{"name": "registry_insert", "find": current, "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("16 monolith insertion-overload and selected-registry ownership states")


if __name__ == "__main__":
    main()
