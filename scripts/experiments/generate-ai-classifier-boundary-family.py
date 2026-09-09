"""Extend the reproduced initialization family with getCatagory's interface.

DC ai_combat.cpp:381, 0x2a52c proves a const receiver and places the real
get_catagory body after check_wall_archery_penalty. Retail expands that
read-only body at initializeCreatures' one call site. The inherited inline
keyword was justified solely by the missing retail standalone body, which
does not establish a source inline declaration. Measure its removal, the
const interface and original source order together and independently.

Keep all nine reproduced initialization/iterator parents. No new helper,
private vector interface, forced inlining or filler statements. The header
also reaches ai_player, so score both complete TUs. If a formerly absent
direct-symbol COMDAT reappears, inspect its actual symbol and rebuild/delink
before judging its own row: the parent's target may still have a placeholder.
"""

import argparse
import importlib.util
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def make_manifest(parent):
    checkpoint = json.loads((parent / "checkpoint.json").read_text())
    if len(checkpoint["records"]) != 9 or len(checkpoint["seen"]) != 9:
        raise ValueError("The nine-state initialization parent must be exhausted")
    for relative in ("src/ai_combat.cpp", "src/ai_player.cpp", "include/ai_combat.h"):
        if (ROOT / relative).read_bytes() != (parent / "snapshot" / relative).read_bytes():
            raise ValueError("Current source differs from verified parent: " + relative)
    spec = importlib.util.spec_from_file_location("initialization_family", Path(__file__).with_name(
        "generate-ai-creature-initialization-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    manifest = module.make_manifest()
    if manifest != json.loads((parent / "input.json").read_text()):
        raise ValueError("The initialization manifest no longer matches its parent")
    for row in checkpoint["elites"]:
        repeat = json.loads((parent / "candidates" / row["id"] / "repeat/result.json").read_text())
        if repeat["scores"] != row["scores"] or repeat["object_hash"] != row["object_hash"]:
            raise ValueError("Parent elite did not reproduce: " + row["id"])

    source = (ROOT / "src/ai_combat.cpp").read_text()
    header = (ROOT / "include/ai_combat.h").read_text()
    start = source.index("// E:\\gamedcs\\ai_combat.cpp:381\n// Retail expands every use")
    end = source.index("\n}\n", start) + 3
    original = source[start:end]
    declaration = "    type_speed_catagory getCatagory(TCreatureType creature, long speed);"
    stub = r"""#if 0  // @carcass

// E:\gamedcs\ai_combat.cpp:381
DC_ONLY(0x2a52c, 0x5C)
type_speed_catagory type_AI_combat_data::getCatagory(TCreatureType creature, long speed)
{
    // @stub
}

#endif  // @carcass"""
    if header.count(declaration) != 1 or source.count(stub) != 1:
        raise ValueError("Review the classifier declaration and old source position")
    options = []
    for ordinary, const, original_order in itertools.product(range(2), repeat=3):
        option = dict(name=("ordinary" if ordinary else "inherited-inline")
                      + ("-const" if const else "-mutable-receiver")
                      + ("-original-source-order" if original_order else "-before-caller"))
        candidate = original
        edits = []
        if ordinary:
            candidate = candidate.replace("inline type_speed_catagory", "type_speed_catagory")
        if const:
            candidate = candidate.replace("    long speed)\n", "    long speed) const\n")
            edits.append(dict(source="include/ai_combat.h", find=declaration,
                              replace=declaration.replace(");", ") const;")))
        if original_order:
            option["replace"] = ""
            edits.append(dict(source="src/ai_combat.cpp", find=stub, replace=candidate.rstrip()))
        elif candidate != original:
            option["replace"] = candidate
        if edits:
            option["extra_edits"] = edits
        options.append(option)
    manifest["axes"].append(dict(name="classifier-interface-and-visibility", find=original, options=options))
    manifest["units"] = ["ai_combat", "ai_player"]
    manifest["evidence"] = __doc__
    manifest["parent_context"] = parent.name
    return manifest


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("parent", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(args.parent), indent=2) + "\n")
