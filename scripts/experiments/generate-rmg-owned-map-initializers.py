#!/usr/bin/env python3
"""Recover map-constructor expansion through ordinary member initialization.

Retail retains the exact owned-map constructor at 0x530fb0 and calls it from
TRmgGeneratorBase 0x536070. With both definitions visible in their retail
source order, current VC6 expands the callee. Compare body assignment with
member initialization for the five real owned-map fields, and independently
for the base generator's progress/version fields. Preserve allocation size,
the canonical array constructor/destructor, progress calls, time and RNG order,
all declarations, member layout and source visibility. No dummy statements,
new helpers, inline annotations or exception-policy changes are introduced.
The retained owned-map body must be reviewed along with every affected caller;
RMG has no Dreamcast counterpart.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def definition(source, signature):
    start = source.index(signature)
    return source[start:source.index("\n}", start) + 2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    signature = "type_random_map::type_random_map(int width, int height, int levels)"
    original = definition(source, signature)
    # Keep declaration order in the initializer list and the recovered store
    # order in the body. The allocation expression is identical in each form.
    fields = [("m_ownsMapItems", "1"), ("m_mapItems", "new TRmgMapItem[width * height * levels]"),
              ("m_mapWidth", "width"), ("m_mapHeight", "height"), ("m_numberLevels", "levels")]
    options = []
    for mask in range(32):
        initialized = [(name, value) for index, (name, value) in enumerate(fields) if mask & (1 << index)]
        body = original[original.index("\n{"):]
        for name, value in initialized:
            statement = "    " + name + " = " + value + ";\n"
            assert statement in body
            body = body.replace(statement, "")
        prefix = signature
        if initialized:
            prefix += "\n    : " + ", ".join(name + "(" + value + ")" for name, value in initialized)
        options.append({"name": "initialize_" + ("_".join(name[2:] for name, _ in initialized) or "none"),
                        "replace": prefix + body})
    base_signature = "TRmgGeneratorBase::TRmgGeneratorBase(int width, int height, int levels,"
    base = definition(source, base_signature)
    base_options = []
    for mask in range(4):
        body = base
        initialized = []
        for index, (name, value) in enumerate((("m_progress", "progress"), ("m_mapVersion", "version"))):
            if mask & (1 << index):
                body = body.replace("    " + name + " = " + value + ";\n", "")
                initialized.append(name + "(" + value + ")")
        if initialized:
            ordered = (["m_mapVersion(version)"] if mask & 2 else []) + ["m_map(width, height, levels)"]
            if mask & 1:
                ordered.append("m_progress(progress)")
            body = body.replace(": m_map(width, height, levels)",
                                ": " + ", ".join(ordered))
        base_options.append({"name": "base_initializers_" + str(mask), "replace": body})
    payload = {"schema": 1, "source": "src/rmg.cpp", "units": ["rmg"], "evidence": __doc__,
               "axes": [{"name": "owned_map_members", "find": original, "options": options},
                        {"name": "generator_base_members", "find": base, "options": base_options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("32 owned-map x 4 generator-base initializer states")


if __name__ == "__main__":
    main()
