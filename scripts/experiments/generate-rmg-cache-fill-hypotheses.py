#!/usr/bin/env python3
"""Generate 60 canonical cache-fill hypotheses, scoring the rectangle caller.

Retail 0x5b3dd0 reads one tile from the adapter before acquiring the packed
cell, copies terrain/frame/flips and sets validity last, preserving bits 14/15.
Its retained body is exact, but rectangle 0x5b4960 still calls this helper
where retail expands it. Cross actual returned-tile lifetimes, receiver
bindings, and the packed cell's existing setters. Preserve one ordinary
canonical helper, the adapter call, field order and all unrelated cell bits.
No pasted helper, artificial work, false inline qualifier or pragma pin.
This Complete-only cluster has no Dreamcast source counterpart.
"""
import argparse
import hashlib
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source, hypotheses

FUNCTION = "?paintRectangle@rmgTerrainPainter@@QAEXIIII@Z"
HELPER = "?initializePackedCell@rmgTerrainPainter@@QAEXABUTRmgGridPoint@@I@Z"
SIGNATURE = ("void rmgTerrainPainter::initializePackedCell(\n"
             "    const TRmgGridPoint& point, unsigned int index)")


def bodies():
    for tile, receiver, access in itertools.product(
            ("copy", "direct", "const_value", "const_reference", "assigned"),
            ("reference", "pointer", "subscript"),
            ("members", "setters", "payload_setters", "validity_setter")):
        query = "m_adapter->getTile(point)"
        if tile == "copy":
            lines = ["rmgTerrainTile tile = " + query + ";"]
        elif tile == "direct":
            lines = ["rmgTerrainTile tile(" + query + ");"]
        elif tile == "const_value":
            lines = ["const rmgTerrainTile tile = " + query + ";"]
        elif tile == "const_reference":
            lines = ["const rmgTerrainTile& tile = " + query + ";"]
        else:
            lines = ["rmgTerrainTile tile;", "tile = " + query + ";"]
        if receiver == "reference":
            lines.append("TRmgPackedTerrainCell& packed = m_packedCells[index];")
            prefix = "packed."
        elif receiver == "pointer":
            lines.append("TRmgPackedTerrainCell* packed = &m_packedCells[index];")
            prefix = "packed->"
        else:
            prefix = "m_packedCells[index]."
        for field in ("terrain", "frame", "flipX", "flipY"):
            value = "tile.m_" + field
            if access in ("setters", "payload_setters"):
                lines.append(prefix + "set" + field[0].upper() + field[1:] + "(" + value + ");")
            else:
                lines.append(prefix + "m_" + field + " = " + value + ";")
        lines.append(prefix + ("setInitialized();" if access in ("setters", "validity_setter")
                               else "m_initialized = 1;"))
        yield "+".join((tile, receiver, access)), SIGNATURE + "\n{\n" + "\n".join(
            "    " + line for line in lines) + "\n}"


def make_manifest(source):
    found = _source.find_definitions(source, HELPER)
    if len(found) != 1:
        raise ValueError("review the unique canonical cache fill")
    item = found[0]
    start = source.rfind("\n", 0, item.head) + 1
    original = source[start:item.body_close + 1]
    options = list(bodies())
    if original not in {body for _, body in options}:
        raise ValueError("review the cache fill before rebasing the family")
    options.sort(key=lambda row: row[1] != original)
    return dict(schema=1, unit="rmg_terrain", function=FUNCTION, evidence=__doc__, axes=[dict(
        name="cache_fill_lifetimes", find=original,
        options=[dict(name=name, replace=body) for name, body in options])])


def make_order_manifest(source, parent_labels):
    payload = make_manifest(source)
    axis = payload["axes"][0]
    choices = {row["name"]: row for row in axis["options"]}
    if len(parent_labels) != 10 or len(set(parent_labels)) != 10 or any(name not in choices for name in parent_labels):
        raise ValueError("review ten unique cache-fill parents")
    axis["options"] = [choices[name] for name in parent_labels]
    # Reuse the reviewed order axis: move the existing packed-cell query,
    # terrain wrapper and dimension definitions with their evidence comments.
    path = Path(__file__).with_name("generate-rmg-strength-polish-hypotheses.py")
    spec = importlib.util.spec_from_file_location("rmg_strength_order", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    payload["axes"].append(module.helper_order_axis(source))
    payload["evidence"] += ("\nFollow-up: cross ten retained canonical cache-fill parents with all six orders "
                            "of the existing packed-cell query, terrain wrapper and dimension definitions. "
                            "Keep each helper exactly once and recompile all parents plus a separate baseline.")
    return payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--helper-order-from", type=Path, help="cross ten completed parents with six canonical helper orders")
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
    if args.helper_order_from:
        parent = json.loads(args.helper_order_from.read_text())
        if (parent.get("schema") != 1 or parent.get("unit") != "rmg_terrain" or parent.get("function") != FUNCTION
                or parent.get("source_sha256") != hashlib.sha256(source.encode()).hexdigest()):
            raise ValueError("review the cache-fill parents against the current source")
        rows = [row for row in parent["results"] if not row["error"] and row["score"] is not None]
        payload = make_order_manifest(source, [row["labels"]["cache_fill_lifetimes"] for row in rows[:10]])
        payload["parent_results"] = str(args.helper_order_from.resolve())
    else:
        payload = make_manifest(source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    parsed = hypotheses.parse_manifest(args.output)
    print("generated", len(hypotheses.variants(parsed[4], parsed[5])), "unique source hypotheses ->", args.output)


if __name__ == "__main__":
    main()
