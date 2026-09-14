#!/usr/bin/env python3
"""Test quest-factory result and creature-count ownership after allocation.

Retail 0x534b90 differs only in the order of its allocation-failure pointer
reloads. The returned wrapper is used solely through the factory's type_object
result. Test that public base, the artifact base and the concrete wrapper as
the source local's type, consistently in all three factories. Independently
test value/reference ownership of the creature-count field captured after
allocation; no reward store, constructor call or evaluation phase is moved.
No Dreamcast counterpart or retained constructor signature exists.
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
    bodies = []
    for owner in ("type_quest_creature_def", "type_quest_experience_def", "type_quest_gold_def"):
        start = source.index("type_object* " + owner + "::generate(")
        bodies.append(source[start:source.index("\n}", start) + 2])
    options = []
    for result, count in itertools.product(("rmgQuestArtifactObject", "type_object", "rmgArtifactObject"),
                                            ("int", "const int", "const int&", "int&")):
        replacements = [body.replace("rmgQuestArtifactObject* object =", result + "* object =") for body in bodies]
        replacements[0] = replacements[0].replace("int count = m_adjustedValue;", count + " count = m_adjustedValue;")
        options.append({"name": result + "+" + count, "replace": replacements[0],
                        "extra_edits": [{"find": old, "replace": new} for old, new in zip(bodies[1:], replacements[1:])]})
    payload = {"schema": 1, "source": "src/rmg.cpp", "units": ["rmg"], "evidence": __doc__,
               "axes": [{"name": "quest_result", "find": bodies[0], "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("12 quest result/count ownership states")


if __name__ == "__main__":
    main()
