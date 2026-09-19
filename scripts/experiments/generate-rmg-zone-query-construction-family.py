#!/usr/bin/env python3
"""Recombine zone ownership parents with actual locate-point construction.

The byte-verified baseline trace gives Zone cleanup (2346-67)/13 = 175
nested budget: entrances consumes94, boundary also costs94 and is refused.
One fewer remaining real site predicts189, expanding boundary but not the
third vector. Compare ordinary default point construction and consumed x/y
assignments at each post-cleanup locate. Preserve all position rereads, parent
scopes, canonical helpers, EH order and final join. No dummy site is removed.
"""
import argparse
import hashlib
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

parent = generator("generate-rmg-zone-boundary-family.py")


def point(phase, which, order):
    if order is None:
        return phase
    assignments = "\n".join(f"            query.m_{axis} = position.m_{axis};" for axis in order)
    named = "                TPoint query(position.m_x, position.m_y);"
    if named in phase:
        return parent.replace(phase, named,
            "                TPoint query;\n" +
            "\n".join("    " + line for line in assignments.splitlines()))
    setup = "            TPoint query;\n" + assignments + "\n"
    if which == 0:
        old = "            TRmgBoundaryVertex* first = diagram.locate(TPoint(position.m_x, position.m_y));"
        new = setup + "            TRmgBoundaryVertex* first = diagram.locate(query);"
    else:
        old = "            fillZoneArea(current, diagram.locate(TPoint(position.m_x, position.m_y)));"
        new = setup + "            fillZoneArea(current, diagram.locate(query));"
    return parent.replace(phase, old, new)


def variant(body, mode):
    orders = (("xy", None), (None, "xy"), ("xy", "xy"), ("yx", None), (None, "yx"))
    prefix, suffix = body.split("    diagram.buildVertices();\n")
    phases = suffix.split("    for (zone = 0;")
    if len(phases) != 3 or phases[0]:
        raise ValueError("review the two post-cleanup zone traversals")
    return prefix + "    diagram.buildVertices();\n" + "".join(
        "    for (zone = 0;" + point(phase, i, order)
        for i, (phase, order) in enumerate(zip(phases[1:], orders[mode])))


def parents(checkpoint, expected_axis="zone_boundary_ownership_and_lifetimes", expected_count=10):
    root = checkpoint.parent
    source = (HOMM3_DIR / parent.SOURCE).read_text()
    if source != (root / "snapshot" / parent.SOURCE).read_text():
        raise ValueError("parent source changed; reproduce it first")
    for directory in ("include",):
        saved = root / "snapshot" / directory
        current = HOMM3_DIR / directory
        if ({p.relative_to(saved): p.read_bytes() for p in saved.rglob("*") if p.is_file()} !=
                {p.relative_to(current): p.read_bytes() for p in current.rglob("*") if p.is_file()}):
            raise ValueError("parent headers changed")
    manifest = json.loads((root / "input.json").read_text())
    axis, = manifest["axes"]
    if axis["name"] != expected_axis:
        raise ValueError("unexpected parent source family")
    rows = json.loads(checkpoint.read_text())["elites"]
    if len(rows) != expected_count:
        raise ValueError(f"{expected_count} reproduced parents required")
    for row in rows:
        trial = root / "candidates" / row["id"]
        repeated = json.loads((trial / "repeat/result.json").read_text())
        if any(repeated[k] != row[k] for k in ("choices", "scores", "object_hash", "source_hashes")):
            raise ValueError("parent did not reproduce")
        authored = (trial / "first/tree" / parent.SOURCE).read_text()
        rendered = source.replace(axis["find"], axis["options"][row["choices"][0]]["replace"])
        if authored != rendered or hashlib.sha256(authored.encode()).hexdigest() != row["source_hashes"][parent.SOURCE]:
            raise ValueError("parent manifest/source identity changed")
        yield row, parent.definition(authored)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    original = parent.definition((HOMM3_DIR / parent.SOURCE).read_text())
    retained = list(parents(args.checkpoint))
    options = [dict(name="unchanged", replace=original)]
    options += [dict(name=f"{row['id']}+point_{mode}", replace=variant(body, mode))
                for row, body in retained for mode in range(5)]
    if len({row["replace"] for row in options}) != 51:
        raise ValueError("expected 51 distinct meaningful states")
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__,
        parent_checkpoint=str(args.checkpoint.resolve()),
        reproduced_parents=[dict(id=r["id"], object_hash=r["object_hash"]) for r, _ in retained],
        axes=[dict(name="zone_query_construction", source=parent.SOURCE,
                   find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("51 zone-query construction states including unchanged control")


if __name__ == "__main__":
    main()
