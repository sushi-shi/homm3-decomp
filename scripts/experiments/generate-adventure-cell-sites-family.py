"""Separate the six drawing callers while preserving canonical cell helpers.

The first cell-call family restored all three drawing groups at once. It
made the boat twins exact but stopped emitting the retained scalar cell.
Its const-signature follow-up was byte-flat. Distinguish hero/underlay,
boat, and ground caller groups now, each with its own retail call/expansion
decision. Every restored group calls the same ordinary GetCell definition;
no new copy, inline annotation, pragma or release verification is introduced.

Historical control: frozen context bc78bc047916d70eaef7 has the corrected
bool-const validity signature, but predates adopting the boat/ground calls.
"""

import argparse
import importlib.util
import itertools
import json
from pathlib import Path


def make_manifest():
    parent = Path(__file__).with_name("generate-adventure-cell-call-family.py")
    spec = importlib.util.spec_from_file_location("cell_calls", parent)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    result = module.make_manifest()
    result["evidence"] = __doc__
    drawing_axis = result["axes"][1]
    drawing = drawing_axis["find"]
    boundaries = [module.COPIES_START,
                  "// Before normalization (function): DrawBoatCell.\n",
                  "// Before normalization (function): DrawGroundCell.\n",
                  module.COPIES_END]
    copies = [drawing[drawing.index(a):drawing.index(b)]
              for a, b in zip(boundaries, boundaries[1:])]
    options = []
    for hero, boat, ground, accessors in itertools.product(range(2), repeat=4):
        candidate = drawing
        for enabled, copy in zip((hero, boat, ground), copies):
            if enabled:
                candidate = module.replace(candidate, copy, "")
        if hero:
            candidate = module.replace(candidate, "drawHeroCell(this, ", "getCell(", 3)
        if boat:
            candidate = module.replace(candidate, "drawBoatCell(\n        this, ", "getCell(\n        ", 2)
        if ground:
            candidate = module.replace(candidate, "drawGroundCell(\n        this, ", "getCell(\n        ")
        if accessors:
            candidate = module.replace(candidate, "&g_game->m_boats[boatParts.m_id]",
                                       "g_game->getBoat(boatParts.m_id)", 2)
            candidate = module.replace(candidate,
                "type_point(currBoat->m_x, currBoat->m_y, currBoat->m_z)",
                "currBoat->getLocation()", 2)
        option = dict(name=f"hero-{hero}-boat-{boat}-ground-{ground}-accessors-{accessors}")
        if candidate != drawing:
            option["replace"] = candidate
        options.append(option)
    drawing_axis["options"] = options
    result["axes"] = result["axes"][:2]
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
