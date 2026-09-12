#!/usr/bin/env python3
"""Test scalar ownership at the prison object's expanded constructor boundary.

Retail 0x5348d0 reads the definition's experience after capturing the properties
argument, whereas the current value-parameter reconstruction hoists that load.
The constructor is only present as an expansion, so its scalar parameter
ownership is not fixed by a standalone mangled symbol. Compare value and const
reference parameters for the three existing integers. Keep the allocation,
hero reservation, conditional object-id increment, base constructor, member
stores and caller expression intact. There is no Dreamcast RMG compiland.
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
    declaration = """    rmgHeroObject(TRmgObjectPropertiesRef* properties,
        type_random_map_generator* generator, int objectId, int heroIndex,
        int experience);"""
    definition = """rmgHeroObject::rmgHeroObject(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, int objectId, int heroIndex,
    int experience)"""
    options = []
    for references in itertools.product((False, True), repeat=3):
        header, source = declaration, definition
        for name, reference in zip(("objectId", "heroIndex", "experience"), references):
            if reference:
                header = header.replace("int " + name, "const int& " + name)
                source = source.replace("int " + name, "const int& " + name)
        options.append({"name": "ownership_" + "".join("r" if r else "v" for r in references),
                        "replace": header,
                        "extra_edits": [{"source": "src/rmg.cpp", "find": definition,
                                         "replace": source}]})
    payload = {"schema": 1,
               "units": ["rmg", "rmg_support", "rmg_terrain", "tiles",
                         "singleselectionpopups", "singleselectionwindow", "scenarioinfo"],
               "evidence": __doc__,
               "axes": [{"name": "prison_argument_ownership", "source": "include/rmg.h",
                         "find": declaration, "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("8 constructor argument-ownership states")


if __name__ == "__main__":
    main()
