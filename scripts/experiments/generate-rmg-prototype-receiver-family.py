#!/usr/bin/env python3
"""Prototype selector record reads and category lifetimes (retail 0x546040).

All 17 blocks and branch destinations match; only ESI/EDI range and terrain
roles differ. The nearby loader closes with direct record reads. Test that
source hypothesis here, crossing retained versus direct property/prototype
receivers, real category-value lifetimes and signed/unsigned scan counters.
Keep checked mask access, public push_back, ordered filtering and rand result.
No Dreamcast counterpart was found. No helper or declaration is changed.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
NAME = "type_random_map_generator::selectObjectPrototype"


def variants(source):
    helper = generator("generate-rmg-position-family.py")
    current = helper.definition(source, NAME)
    old = generator("generate-rmg-prototype-polish-hypotheses.py")
    original = next(old.lifetime_bodies())[1]
    if current != original:
        raise ValueError("review changed selector before constructing receiver frontier")
    declaration = "        TRmgObjectPropertiesRef* properties = m_objectPrototypes[objectType][index];\n"
    prototype = "        TObjectType* prototype = properties->m_prototype;\n"
    guard = "        if (prototype->m_subtype != subtype)\n            continue;\n"
    for receiver, category, signed in itertools.product(range(6), range(5), range(2)):
        body = original
        if category:
            typename = "const int&" if category == 2 else "int"
            capture = "        " + typename + " category = prototype->m_slotCategory;\n"
            if category == 4:
                body = body.replace(prototype, prototype + "        int category;\n")
                capture = "        category = prototype->m_slotCategory;\n"
            body = body.replace(guard, (capture + guard) if category == 3 else (guard + capture))
            body = body.replace("prototype->m_slotCategory ==", "category ==")
        if receiver == 1:
            body = body.replace(prototype, "").replace("prototype->", "properties->m_prototype->")
        elif receiver == 2:
            body = body.replace(declaration, "").replace("properties", "m_objectPrototypes[objectType][index]")
        elif receiver == 3:
            body = body.replace(prototype, "").replace(declaration, "")
            body = body.replace("prototype->", "m_objectPrototypes[objectType][index]->m_prototype->")
            body = body.replace("candidates.push_back(properties)", "candidates.push_back(m_objectPrototypes[objectType][index])")
        elif receiver == 4:
            body = body.replace("TRmgObjectPropertiesRef* properties =", "TRmgObjectPropertiesRef*& properties =")
        elif receiver == 5:
            body = body.replace(prototype, "        TObjectType& prototype = *properties->m_prototype;\n")
            body = body.replace("prototype->", "prototype.")
        if signed:
            body = body.replace("unsigned int index", "int index")
        yield dict(name="receiver_%d+category_%d+signed_%d" % (receiver, category, signed), replace=body)


def return_frontier(source, checkpoint_path):
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get("generation", 0) < 1 or len(checkpoint["elites"]) != 10:
        raise ValueError("unfinished ten-parent receiver population")
    payload, originals, axes = source_families.load_manifest(context / "input.json", HOMM3_DIR)
    if len(checkpoint["records"]) != 60 or any(not row["scores"] for row in checkpoint["records"]):
        raise ValueError("receiver population is not fully scored")
    for folder in ("src", "include"):
        frozen = context / "snapshot" / folder
        live = HOMM3_DIR / folder
        paths = [p.relative_to(frozen) for p in frozen.rglob("*") if p.is_file()]
        live_paths = [p.relative_to(live) for p in live.rglob("*") if p.is_file() and "build" not in p.relative_to(live).parts]
        if set(paths) != set(live_paths) or any((frozen / p).read_bytes() != (live / p).read_bytes() for p in paths):
            raise ValueError("changed receiver snapshot: " + folder)
    helper = generator("generate-rmg-position-family.py")
    current = helper.definition(source, NAME)
    options = [dict(name="unchanged", replace=current)]
    seen = {current}
    parents = []
    for elite in checkpoint["elites"]:
        rendered = source_families.render(originals, axes, tuple(elite["choices"]))
        repeated = context / "candidates" / elite["id"] / "repeat"
        result = json.loads((repeated / "result.json").read_text())
        for key in ("scores", "object_hash", "source_hashes", "choices"):
            if result[key] != elite[key]:
                raise ValueError("receiver parent did not reproduce " + key)
        for relative, text in rendered.items():
            if (repeated / "tree" / relative).read_text() != text or result["source_hashes"][relative] != source_families.digest(text.encode()):
                raise ValueError("changed reproduced receiver source")
        parent = helper.definition(rendered[SOURCE], NAME)
        parents.append((elite["id"], parent))
        if parent not in seen:
            seen.add(parent)
            options.append(dict(name=elite["id"] + "+parent", replace=parent))
    old = "    return candidates[rand() % candidates.size()];"
    returns = [
        "    TRmgObjectPropertiesRef* selected = candidates[rand() % candidates.size()];\n    return selected;",
        "    TRmgObjectPropertiesRef* const& selected = candidates[rand() % candidates.size()];\n    return selected;",
        "    unsigned int selectedIndex = rand() % candidates.size();\n    return candidates[selectedIndex];",
        "    int randomValue = rand();\n    return candidates[randomValue % candidates.size()];",
        "    unsigned int candidateCount = candidates.size();\n    return candidates[rand() % candidateCount];"]
    for form in range(5):
        for identity, parent in parents:
            if parent.count(old) != 1:
                raise ValueError("review changed selection result")
            body = parent.replace(old, returns[form])
            if body in seen:
                continue
            seen.add(body)
            options.append(dict(name=identity + "+return_%d" % form, replace=body))
            if len(options) == 60:
                return options
    return options


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--returns-from", type=Path, help="selected-value lifetimes from ten reproduced receiver parents")
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    options = return_frontier(source, args.returns_from) if args.returns_from else list(variants(source))
    args.output.write_text(json.dumps(dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="selector_receivers", source=SOURCE, find=options[0]["replace"], options=options)]), indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(options), "selector receiver states")


if __name__ == "__main__":
    main()
