#!/usr/bin/env python3
"""Retail videoPlay declaration state from smackmgr's contiguous data order."""
import argparse
import json
import re
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()
prefix_end = source.index("VA(0x005971b0")
lines = source.splitlines(keepends=True)
data = []
offset = 0
for index, line in enumerate(lines):
    offset += len(line)
    if offset > prefix_end:
        break
    match = re.match(r"DATA\(0x([0-9A-Fa-f]+)\) ", line)
    if match:
        data.append((index, int(match.group(1), 16), line))


def reordered(name, predicate, key):
    chosen = [item for item in data if predicate(item[1], item[2])]
    assert len(chosen) >= 2, name
    ordered = sorted(chosen, key=key)
    changed = list(lines)
    first = min(item[0] for item in chosen)
    selected_indices = {item[0] for item in chosen}
    for index in selected_indices:
        changed[index] = ""
    changed[first] = "".join(item[2] for item in ordered)
    return {"name": name, "replace": "".join(changed)}


def move_after(name, symbol, predecessor):
    chosen = [item for item in data if symbol in item[2] or predecessor in item[2]]
    assert len(chosen) == 2
    ordered = sorted(chosen, key=lambda item: 0 if predecessor in item[2] else 1)
    changed = list(lines)
    first = min(item[0] for item in chosen)
    for index, _, _ in chosen:
        changed[index] = ""
    changed[first] = "".join(item[2] for item in ordered)
    return {"name": name, "replace": "".join(changed)}


ascending = lambda item: item[1]
descending = lambda item: -item[1]
options = [{"name": "control"}]
options.append(move_after("paused-after-loop", "g_smackPaused", "g_smackLoop"))
options.append(reordered(
    "smacker-core-retail-address-order",
    lambda address, line: 0x69fdec <= address <= 0x69fe20,
    ascending))
options.append(reordered(
    "whole-contiguous-retail-address-order",
    lambda address, line: 0x69fdd8 <= address <= 0x69fe5c,
    ascending))
options.append(reordered(
    "whole-contiguous-reverse-control",
    lambda address, line: 0x69fdd8 <= address <= 0x69fe5c,
    descending))
options.append(reordered(
    "sound-bss-retail-address-order",
    lambda address, line: 0x69d840 <= address <= 0x69e5b0,
    ascending))
options.append(reordered(
    "all-uninitialized-retail-address-order",
    lambda address, line: address >= 0x69d000,
    ascending))
options.append(reordered(
    "smacker-handles-and-state-retail-address-order",
    lambda address, line: (0x69fdf4 <= address <= 0x69fe20
                           or 0x69fe54 <= address <= 0x69fe5c),
    ascending))

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{
        "name": "pre-function-global-declaration-order",
        "find": source,
        "options": options,
    }],
    "evidence": [
        "Dreamcast identifies smackmgr.cpp and its file-static/global data, while retail proves a contiguous owner block at 0x69fdd8..0x69fe5c. MSVC allocates same-section data in an order that can preserve declaration order, although cross-section order remains unproven.",
        "The reconstruction currently defines g_smackPaused after the archive counts despite its retail address 0x69fe18 between g_smackLoop and g_smackAdvance. Several other definitions likewise depart from the contiguous retail order.",
        "videoPlay is sensitive only in C1/C2 coloring while every emitted operation agrees. Test the strongest address-order hypotheses plus reverse and narrow controls, requiring every exact smackmgr sibling to stay exact.",
        "These states move only real, uniquely owned definitions; no declaration, operation, helper, or compiler directive is invented.",
    ],
}, indent=2) + "\n")
