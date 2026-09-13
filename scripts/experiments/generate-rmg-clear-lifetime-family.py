#!/usr/bin/env python3
"""Generate map-cell reset variants around the real vector clearing boundary.

Retail 0x530f10 expands the pointer-vector erase and then updates three
packed words, keeping EDI live across the copy guard. Compare public clear,
erase and zero-size resize calls and the lifetimes of the three real field
snapshots. The vector owns raw pointers, not pointed-to objects: clearing it
does not invoke game code or change the surrounding cell fields. Preserve
all named bitfield operations and the final member-store sequence.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

SNAPSHOTS = (("TRmgConnectionDecoration", "connection", "m_connection"),
             ("TRmgGroundTile", "tile", "m_tile"),
             ("TRmgGroundTileData", "tileData", "m_tileData"))
CALLS = (("erase", "    m_objects.erase(m_objects.begin(), m_objects.end());\n"),
         ("clear", "    m_objects.clear();\n"),
         ("resize", "    m_objects.resize(0);\n"))


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_source_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def variants(original):
    begin = original.index("\n{\n") + 3
    updates = original.index("    connection.m_present = 0;", begin)
    tail = original[updates:]
    for (api, call), before, order, form in itertools.product(
            CALLS, range(8), itertools.permutations(range(3)), ("copy", "direct", "assigned")):
        snapshots = []
        for kind, local, member in SNAPSHOTS:
            if form == "copy":
                text = f"    {kind} {local} = {member};\n"
            elif form == "direct":
                text = f"    {kind} {local}({member});\n"
            else:
                text = f"    {kind} {local};\n    {local} = {member};\n"
            snapshots.append(text)
        body = original[:begin]
        body += "".join(snapshots[index] for index in order if before & (1 << index))
        body += call
        body += "".join(snapshots[index] for index in order if not before & (1 << index))
        body += "\n" + tail
        label = api + "+" + str(before) + "+" + "".join(map(str, order)) + "+" + form
        yield label, body


def make_axes(source):
    helper = helpers()
    original = helper.definition(source, "TRmgMapItem::clear")
    return [helper.axis("clear_snapshot_lifetimes", "src/rmg.cpp", original, variants(original))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / "src/rmg.cpp").read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes, evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
