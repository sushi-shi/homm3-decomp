#!/usr/bin/env python3
"""Object-distance flood public queue operations at retail 0x5402a0.

Retail retains single-element seed inserts, position erase, and the integer
erase's copy/destroy pair. Our 69.5855% body expands both erases and calls
count-insert for seeds, with a 12-byte frame surplus. Test public seed/pop
APIs and byte-valued parity, preserving the canonical position-first sorted
helper and ordinary base registration. No Dreamcast counterpart is mapped.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

NAME = "type_random_map_generator::addObject"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def variants(original):
    seed = "        positions.push_back(currentPosition);\n        costs.push_back(0);"
    seeds = [seed,
        "        positions.insert(positions.end(), currentPosition);\n        costs.insert(costs.end(), 0);",
        "        positions.push_back(currentPosition);\n        int seedCost = 0;\n        costs.push_back(seedCost);",
        "        std::vector<TRmgMapPosition>::iterator positionEnd = positions.end();\n        positions.insert(positionEnd, currentPosition);\n        costs.insert(costs.end(), 0);",
        "        positions.insert(positions.end(), currentPosition);\n        std::vector<int>::iterator costEnd = costs.end();\n        costs.insert(costEnd, 0);",
    ]
    if original.count(seed) != 1 or original.count("                if (direction & 1)") != 1:
        raise ValueError("review changed object-flood seed/parity")
    for initial, pop, parity in itertools.product(range(5), range(4), range(3)):
        body = original.replace(seed, seeds[initial])
        if pop & 1:
            body = body.replace("positions.erase(positions.end() - 1);", "positions.pop_back();")
        if pop & 2:
            body = body.replace("costs.erase(costs.end() - 1);", "costs.pop_back();")
        if parity == 1:
            body = body.replace("if (direction & 1)", "if (static_cast<unsigned char>(direction) & 1)")
        elif parity == 2:
            body = body.replace("                if (direction & 1)",
                "                unsigned char diagonal = direction & 1;\n                if (diagonal)")
        yield "seed_%d+pop_%d+parity_%d" % (initial, pop, parity), body


def coordinate_refinement(parent, form):
    old = """                TRmgMapPosition nextPosition;
                nextPosition.m_x = currentPosition.m_x + g_rmgDirections[direction].m_x;
                nextPosition.m_y = currentPosition.m_y + g_rmgDirections[direction].m_y;
                nextPosition.m_z = currentPosition.m_z;"""
    if parent.count(old) != 1:
        raise ValueError("review changed object-flood neighbor coordinate")
    if form in (0, 1):
        declaration = "TPoint offset = g_rmgDirections[direction];" if form == 0 else "const TPoint& offset = g_rmgDirections[direction];"
        replacement = "                " + declaration + "\n" + old.replace("g_rmgDirections[direction].", "offset.")
    elif form == 2:
        replacement = "                TRmgMapPosition nextPosition = currentPosition;\n                nextPosition += g_rmgDirections[direction];"
    elif form == 3:
        replacement = """                TRmgMapPosition nextPosition;
                nextPosition.m_x = currentPosition.m_x;
                nextPosition.m_y = currentPosition.m_y;
                nextPosition += g_rmgDirections[direction];
                nextPosition.m_z = currentPosition.m_z;"""
    elif form == 4:
        replacement = old.replace("currentPosition.m_x + g_rmgDirections[direction].m_x", "g_rmgDirections[direction].m_x + currentPosition.m_x")
        replacement = replacement.replace("currentPosition.m_y + g_rmgDirections[direction].m_y", "g_rmgDirections[direction].m_y + currentPosition.m_y")
    else:
        raise ValueError("unknown coordinate family")
    return parent.replace(old, replacement)


def seed_refinement(parent, form):
    trigger = "        TObjectType::TPoint trigger = prototype->m_triggerCell;\n"
    coordinate = """        TRmgMapPosition currentPosition;
        currentPosition.m_x = position.m_x - trigger.m_x;
        currentPosition.m_y = position.m_y - trigger.m_y;
        currentPosition.m_z = position.m_z;"""
    if parent.count(trigger) != 1 or parent.count(coordinate) != 1:
        raise ValueError("review changed seed coordinate lifetime")
    if form == 0:
        return parent.replace(trigger, "").replace(coordinate, trigger + coordinate)
    if form == 1:
        replacement = "        TRmgMapPosition currentPosition;\n        {\n" + trigger.replace("        ", "            ", 1)
        replacement += "\n".join("    " + line for line in coordinate.splitlines()[1:]) + "\n        }"
        return parent.replace(trigger, "").replace(coordinate, replacement)
    if form == 2:
        return parent.replace(trigger, trigger.replace("TObjectType::TPoint trigger", "const TObjectType::TPoint& trigger"))
    if form == 3:
        return parent.replace(trigger, "        int triggerX = prototype->m_triggerCell.m_x;\n        int triggerY = prototype->m_triggerCell.m_y;\n").replace("trigger.m_x", "triggerX").replace("trigger.m_y", "triggerY")
    if form == 4:
        return parent.replace(coordinate, """        TRmgMapPosition currentPosition = position;
        currentPosition.m_x -= trigger.m_x;
        currentPosition.m_y -= trigger.m_y;""")
    raise ValueError("unknown seed lifetime family")


def frontier(source, checkpoint_path, refine=coordinate_refinement):
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get("generation", 0) < 1 or len(checkpoint["elites"]) != 10:
        raise ValueError("unfinished object-flood parent population")
    _, originals, axes = load_manifest(context / "input.json", HOMM3_DIR)
    if len(checkpoint["records"]) != 60 or any(not row["scores"] for row in checkpoint["records"]):
        raise ValueError("object-flood population is not fully scored")
    for folder in ("src", "include"):
        frozen, live = context / "snapshot" / folder, HOMM3_DIR / folder
        paths = [p.relative_to(frozen) for p in frozen.rglob("*") if p.is_file()]
        live_paths = [p.relative_to(live) for p in live.rglob("*") if p.is_file() and "build" not in p.relative_to(live).parts]
        if set(paths) != set(live_paths) or any((frozen / p).read_bytes() != (live / p).read_bytes() for p in paths):
            raise ValueError("changed object-flood snapshot: " + folder)
    current = definition(source)
    forms, seen, parents = [("unchanged", current)], {current}, []
    for elite in checkpoint["elites"]:
        rendered = source_families.render(originals, axes, tuple(elite["choices"]))
        repeated = context / "candidates" / elite["id"] / "repeat"
        result = json.loads((repeated / "result.json").read_text())
        for key in ("scores", "object_hash", "source_hashes", "choices"):
            if result[key] != elite[key]:
                raise ValueError("object-flood parent did not reproduce " + key)
        for relative, text in rendered.items():
            if (repeated / "tree" / relative).read_text() != text or result["source_hashes"][relative] != source_families.digest(text.encode()):
                raise ValueError("changed reproduced object-flood source")
        parent = definition(rendered["src/rmg.cpp"])
        parents.append((elite["id"], parent))
        if parent not in seen:
            seen.add(parent)
            forms.append((elite["id"] + "+parent", parent))
    for form in range(5):
        for identity, parent in parents:
            body = refine(parent, form)
            if body not in seen:
                seen.add(body)
                forms.append((identity + "+" + refine.__name__ + "_%d" % form, body))
            if len(forms) == 60:
                return forms
    return forms


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--coordinates-from", type=Path)
    group.add_argument("--seeds-from", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    original = definition(source)
    if args.seeds_from:
        forms = frontier(source, args.seeds_from, seed_refinement)
    else:
        forms = frontier(source, args.coordinates_from) if args.coordinates_from else variants(original)
    helper = generator("generate-rmg-position-family.py")
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[
        helper.axis("object_distance_queue", "src/rmg.cpp", original, forms)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "object-distance queue states")


if __name__ == "__main__":
    main()
