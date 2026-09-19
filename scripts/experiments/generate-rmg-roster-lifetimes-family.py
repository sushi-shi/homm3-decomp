#!/usr/bin/env python3
"""Recombine reproduced roster loops with real pointer and container lifetimes.

The first family established a better cap boundary with a named base pointer
but introduced two earlier base-constructor calls. Keep those parent controls,
then test const pointer initialization, separate assignment, and a consumed
function-scope pointer. A local reference to the actual registration vector
tests container ownership without changing its helper, member, or contents.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from experiments._support import generator


def options(parent):
    module = generator("generate-rmg-roster-family.py")
    source = (HOMM3_DIR / module.SOURCE).read_text()
    if (parent / "snapshot" / module.SOURCE).read_text() != source:
        raise ValueError("parent source identity changed")
    original = module.definition(source)
    expected = list(module.variants(original))
    manifest = json.loads((parent / "input.json").read_text())
    if manifest["axes"][0]["options"] != expected:
        raise ValueError("parent options changed")
    elites = json.loads((parent / "generation-0001.json").read_text())["elites"]
    result = [dict(name="unchanged", replace=original)]
    loops = []
    for elite in elites:
        repeated = json.loads((parent / "candidates" / elite["id"] / "repeat/result.json").read_text())
        if repeated["scores"] != elite["scores"] or repeated["object_hash"] != elite["object_hash"]:
            raise ValueError("parent reproduction mismatch")
        index = elite["choices"][0]
        result.append(dict(name="parent_" + elite["id"], replace=expected[index]["replace"]))
        bound, reverse = index // 12, (index % 12) // 3
        if (bound, reverse) not in loops:
            loops.append((bound, reverse))
    for bound, reverse in loops:
        direct = module.reverse_forms(module.bound_forms(original, bound), reverse)
        for lifetime in ("const_pointer", "assigned_pointer", "shared_pointer"):
            for container in ("member", "reference"):
                def registration(match):
                    indent, kind, args = match.groups()
                    new = "new " + kind + "(" + args + ")"
                    if lifetime == "const_pointer":
                        declaration = "type_treasure_def* const objectGenerator = " + new + ";"
                    elif lifetime == "assigned_pointer":
                        declaration = "type_treasure_def* objectGenerator;\n" + indent + "    objectGenerator = " + new + ";"
                    else:
                        declaration = "objectGenerator = " + new + ";"
                    return (indent + "{\n" + indent + "    " + declaration + "\n"
                            + indent + "    m_objectGenerators.push_back(objectGenerator);\n" + indent + "}")
                body = module.REGISTER.sub(registration, direct)
                declarations = []
                if lifetime == "shared_pointer":
                    declarations.append("    type_treasure_def* objectGenerator;")
                if container == "reference":
                    body = body.replace("m_objectGenerators.push_back", "objectGenerators.push_back")
                    declarations.append("    std::vector<type_treasure_def*>& objectGenerators = m_objectGenerators;")
                if declarations:
                    body = body.replace("{\n", "{\n" + "\n".join(declarations) + "\n", 1)
                result.append(dict(name=f"bounds_{bound}+reverse_{reverse}+{lifetime}+{container}", replace=body))
    unique = {}
    for row in result:
        unique.setdefault(row["replace"], row)
    return list(unique.values())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("parent", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    rows = options(args.parent)
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="roster_lifetimes", source="src/rmg.cpp", find=rows[0]["replace"], options=rows)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(rows), "roster lifetime states")


if __name__ == "__main__":
    main()
