#!/usr/bin/env python3
"""Test pointer argument ownership at the two expanded quest constructors.

Creature-quest factory 0x534b90 matches all success-path instructions; its
allocation-failure arm exchanges the pending-hut and definition pointer loads.
The prison factory independently recovered its constructor's argument ownership
from a similar scheduling difference. Neither quest constructor has a retained
signature. Cross pointer values versus const references to those pointer values,
preserving pointee cv-qualification, argument expressions, allocations, canonical
base calls and reward stores. The pending-hut constructor has one pointer and
the wrapper four, giving 32 states. All three quest factories and every other
function in all seven header consumers are scored. No Dreamcast RMG body exists.
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
    seer_decl = "    rmgSeerHutObject(TRmgObjectPropertiesRef* properties);"
    seer_def = "rmgSeerHutObject::rmgSeerHutObject(TRmgObjectPropertiesRef* properties)"
    quest_decl = """    rmgQuestArtifactObject(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, rmgSeerHutObject* seerHut,
        type_treasure_def* definition);"""
    quest_def = """rmgQuestArtifactObject::rmgQuestArtifactObject(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, rmgSeerHutObject* seerHut,
    type_treasure_def* definition)"""
    axes = []
    for owner, declaration, definition, names in (
            ("seer", seer_decl, seer_def, ("properties",)),
            ("wrapper", quest_decl, quest_def, ("properties", "generator", "seerHut", "definition"))):
        options = []
        for refs in itertools.product((False, True), repeat=len(names)):
            header, source = declaration, definition
            for name, reference in zip(names, refs):
                if reference:
                    header = header.replace("* " + name, "* const& " + name)
                    source = source.replace("* " + name, "* const& " + name)
            options.append({"name": "ownership_" + "".join("r" if r else "v" for r in refs),
                            "replace": header, "extra_edits": [{"source": "src/rmg.cpp",
                            "find": definition, "replace": source}]})
        axes.append({"name": owner + "_arguments", "source": "include/rmg.h",
                     "find": declaration, "options": options})
    payload = {"schema": 1, "evidence": __doc__,
               "units": ["rmg", "rmg_support", "rmg_terrain", "tiles",
                         "singleselectionpopups", "singleselectionwindow", "scenarioinfo"],
               "axes": axes}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("32 quest-constructor pointer ownership states")


if __name__ == "__main__":
    main()
