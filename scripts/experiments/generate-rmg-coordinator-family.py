#!/usr/bin/env python3
"""RMG generate 0x549930: slot-buffer initialization and player-map fill.

Retail clears each eight-byte slot array with two dword stores; partial
aggregate initialization currently emits several unaligned byte/word stores.
The nine-player mapping fill also schedules its destination differently.
Preserve selection, string assignment, slot ordering and all pipeline calls.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::generate"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, FUNCTION)


def replace(body, old, new):
    if body.count(old) != 1:
        raise ValueError("review coordinator anchor: " + old)
    return body.replace(old, new)


def variant(original, human, all_slots, mapping):
    body = original
    for name, form in (("humanSlots", human), ("allSlots", all_slots)):
        old = "    char " + name + "[8] = {0};"
        forms = (
            old,
            "    char " + name + "[8] = {0, 0, 0, 0, 0, 0, 0, 0};",
            "    char " + name + "[8];\n    memset(" + name + ", 0, sizeof(" + name + "));",
            "    char " + name + "[8];\n    std::fill(" + name + ", " + name + " + 8, 0);",
            "    char " + name + "[8];\n    for (int slotByte = 0; slotByte < 8; ++slotByte)\n        " + name + "[slotByte] = 0;",
        )
        found = [text for text in forms if text in body]
        if len(found) != 1:
            raise ValueError("review coordinator buffer form: " + name)
        body = replace(body, found[0], forms[form])
    old = "    memset(m_playerIndexMap, -1, sizeof(m_playerIndexMap));"
    forms = (old, "    std::fill(m_playerIndexMap, m_playerIndexMap + 9, -1);",
        "    for (int mapIndex = 0; mapIndex < 9; ++mapIndex)\n        m_playerIndexMap[mapIndex] = -1;")
    found = [text for text in forms if text in body]
    if len(found) != 1:
        raise ValueError("review coordinator mapping form")
    return replace(body, found[0], forms[mapping])


def axes(source):
    original = definition(source)
    options = [(f"human_{h}+all_{a}+mapping_{m}", variant(original, h, a, m))
               for h, a, m in itertools.product(range(5), range(4), range(3))]
    options.sort(key=lambda option: option[1] != original)
    axis = generator("generate-rmg-position-family.py").axis("coordinator_buffers", SOURCE, original, options)
    if len(axis["options"]) != 60:
        raise ValueError("expected sixty distinct coordinator forms")
    return [axis]


def follow_axes(source, parent):
    """Retain reproduced parents, vary selected-index type and sum evaluation.

    Retail uses EBX for the nonnegative selected index and loads human count
    before computer count at both sum sites. No pointer caches or call edits.
    """
    original = definition(source)
    report = json.loads((parent / "generation-0001.json").read_text())
    snapshot = (parent / "snapshot" / SOURCE).read_text()
    if snapshot != source:
        raise ValueError("parent source identity changed")
    manifest = json.loads((parent / "input.json").read_text())
    if manifest["axes"] != axes(source):
        raise ValueError("parent manifest identity changed")
    options = [("unchanged", original)]
    for elite in report["elites"]:
        repeated = json.loads((parent / "candidates" / elite["id"] / "repeat" / "result.json").read_text())
        if repeated["scores"] != elite["scores"] or repeated["object_hash"] != elite["object_hash"]:
            raise ValueError("parent reproduction changed")
        parent_body = manifest["axes"][0]["options"][elite["choices"][0]]["replace"]
        for signed, reverse in itertools.product(range(2), repeat=2):
            body = parent_body
            if signed:
                body = replace(body, "unsigned int selected =", "int selected =")
            if reverse:
                body = replace(body, "m_humanPlayerCount + m_computerPlayerCount", "m_computerPlayerCount + m_humanPlayerCount")
            options.append((f"parent_{elite['id']}+signed_{signed}+reverse_{reverse}", body))
    return [generator("generate-rmg-position-family.py").axis("coordinator_entry", SOURCE, original, options)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parent", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    choices = follow_axes(source, args.parent) if args.parent else axes(source)
    payload = dict(schema=1, units=["rmg"], axes=choices, evidence=__doc__)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated coordinator forms ->", args.output)


if __name__ == "__main__":
    main()
