#!/usr/bin/env python3
"""Cross 60 real source states for treasure-group commit at 0x5469b0.

The retail-only body transfers object positions, clips a two-dimensional map
overlap, snapshots destination gate/border bits, updates both maps, then invokes
each object's mutable virtual query. Its 99.8047% baseline has the same CFG and
calls; source-cell multiplication operands and an outer-loop reload differ.
Cross three scan-coordinate lifetimes, five construction/assignment forms for
the canonical destination position, and four snapshot/source-query orders.
Preserve live object-vector bounds, all helpers, the retained position
constructor, byte predicates and pre-write snapshots. No body flattening,
qualifier gates, layout variants, pragmas or dummy compiler-budget operations.
"""
import argparse
import functools
import hashlib
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::commitTreasureGroup"
PARENT_EVIDENCE = """Retain ten reproduced treasure-group commit parents plus the unchanged
control, then round-robin fresh source-map bindings and coordinate snapshots.
The first 60-state frontier reaches 99.9141%: the outer-loop reload agrees;
only source width/y multiply operands differ. Preserve the canonical scalar
lookup, destination constructor, live virtual-call loops and all map policies.
Each successor also passes the independent map/alias/call behavioral oracle.
"""


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_group_commit_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


HEAD = """void type_random_map_generator::commitTreasureGroup(TRmgTreasureGroup* group,
    TRmgMapPosition position)
{
    group->m_position = position;
    for (unsigned int i = 0; i < group->m_objects.size(); ++i) {
        type_object* object = group->m_objects[i];
        TRmgMapPosition objectPosition = object->getPosition();
        objectPosition.m_x += position.m_x;
        objectPosition.m_y += position.m_y;
        objectPosition.m_z = position.m_z;
        addObject(object, objectPosition);
    }
    TRmgZoneBounds bounds;
    bounds.m_minimumX = std::_cpp_max<long>(0, -position.m_x);
    bounds.m_minimumY = std::_cpp_max<long>(0, -position.m_y);
    bounds.m_maximumX = std::_cpp_min<long>(group->m_map.m_mapWidth, m_map.m_mapWidth - position.m_x);
    bounds.m_maximumY = std::_cpp_min<long>(group->m_map.m_mapHeight, m_map.m_mapHeight - position.m_y);
"""
POLICY = """            if (destination->m_tile.m_landType != eTerrainWater
                && !source->hasSubterraneanGate() && source->m_tileData.m_roadPassable
                && source->m_tile.m_landType != eTerrainRock && !source->isRoadEntrance()
                && destination->m_tileData.m_roadPassable
                && destination->m_tile.m_landType != eTerrainRock && !destination->isRoadEntrance()) {
                if (!destination->m_connection.m_present)
                    destination->m_tileData.m_subterraneanGate = 0;
                if (source->hasBorderObject() && !destination->m_connection.m_present) {
                    destination->m_tileData.m_subterraneanGate = 0;
                    destination->m_tileData.m_borderObject = 1;
                }
            }
            if (!source->m_connection.m_present) {
                source->m_tileData.m_borderObject = border;
                if (border)
                    source->m_tileData.m_subterraneanGate = 0;
            }
            if (!source->m_connection.m_present) {
                source->m_tileData.m_subterraneanGate = gate;
                if (gate)
                    source->m_tileData.m_borderObject = 0;
            }
        }
    }
    for (unsigned int objectIndex = 0; objectIndex < group->m_objects.size(); ++objectIndex)
        group->m_objects[objectIndex]->isWritable();
}"""


def body(scan, construction, snapshots):
    scans = (
        "    TPoint point;\n"
        "    for (point.m_y = bounds.m_minimumY; point.m_y < bounds.m_maximumY; ++point.m_y) {\n"
        "        for (point.m_x = bounds.m_minimumX; point.m_x < bounds.m_maximumX; ++point.m_x) {\n",
        "    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {\n"
        "        for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {\n",
        "    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {\n"
        "        TPoint point(bounds.m_minimumX, y);\n"
        "        for (; point.m_x < bounds.m_maximumX; ++point.m_x) {\n",
    )
    x, y = ("x", "y") if scan == 1 else ("point.m_x", "point.m_y")
    arguments = x + " + position.m_x, " + y + " + position.m_y, position.m_z"
    value = "TRmgMapPosition(" + arguments + ")"
    forms = (
        "            TRmgMapItem* destination = m_map.getMapItem(\n                " + value + ");\n",
        "            TRmgMapPosition destinationPosition = " + value + ";\n",
        "            TRmgMapPosition destinationPosition(" + arguments + ");\n",
        "            TRmgMapPosition destinationPosition;\n            destinationPosition = " + value + ";\n",
        "            const TRmgMapPosition& destinationPosition = " + value + ";\n",
    )
    destination = forms[construction]
    if construction:
        destination += "            TRmgMapItem* destination = m_map.getMapItem(destinationPosition);\n"
    gate = "            unsigned char gate = destination->hasSubterraneanGate();\n"
    border = "            unsigned char border = destination->hasBorderObject();\n"
    source = "            TRmgMapItem* source = group->m_map.getMapItem(" + x + ", " + y + ", 0);\n"
    orders = (gate + border + source, gate + source + border, source + gate + border, border + gate + source)
    return HEAD + scans[scan] + destination + orders[snapshots] + POLICY


def forms():
    for scan, construction, snapshots in itertools.product(range(3), range(5), range(4)):
        label = ("point", "scalars", "per_row_point")[scan] + "+" + (
            "temporary", "copy_initialized", "direct_initialized", "assigned", "const_reference")[construction]
        label += "+" + ("gate_border_source", "gate_source_border", "source_gate_border", "border_gate_source")[snapshots]
        yield label, body(scan, construction, snapshots)


def make_axes(source):
    helper = helpers()
    original = helper.definition(source, FUNCTION)
    options = list(forms())
    if original not in admitted_bodies():
        raise ValueError("review the current group-commit operations before rebasing")
    axis = helper.axis("group_commit_lifetimes", SOURCE, original, options)
    axis["options"] = axis["options"][:60]
    return [axis]


def refinements(original):
    if original not in dict(forms()).values():
        raise ValueError("review the parent source before refining its lookup")
    x, y = ("x", "y") if "for (int x =" in original else ("point.m_x", "point.m_y")
    query = "            TRmgMapItem* source = group->m_map.getMapItem(" + x + ", " + y + ", 0);\n"
    if original.count(query) != 1:
        raise ValueError("review the source-cell lookup")
    alternatives = (
        ("map_reference", "            type_random_map& sourceMap = group->m_map;\n"
         "            TRmgMapItem* source = sourceMap.getMapItem(" + x + ", " + y + ", 0);\n"),
        ("map_pointer", "            type_random_map* sourceMap = &group->m_map;\n"
         "            TRmgMapItem* source = sourceMap->getMapItem(" + x + ", " + y + ", 0);\n"),
        ("point_copy", "            TPoint sourcePoint(" + x + ", " + y + ");\n"
         "            TRmgMapItem* source = group->m_map.getMapItem(sourcePoint.m_x, sourcePoint.m_y, 0);\n"),
        ("coordinates_xy", "            int sourceX = " + x + ";\n            int sourceY = " + y + ";\n"
         "            TRmgMapItem* source = group->m_map.getMapItem(sourceX, sourceY, 0);\n"),
        ("coordinates_yx", "            int sourceY = " + y + ";\n            int sourceX = " + x + ";\n"
         "            TRmgMapItem* source = group->m_map.getMapItem(sourceX, sourceY, 0);\n"),
        ("point_assigned", "            TPoint sourcePoint;\n            sourcePoint.m_x = " + x + ";\n"
         "            sourcePoint.m_y = " + y + ";\n"
         "            TRmgMapItem* source = group->m_map.getMapItem(sourcePoint.m_x, sourcePoint.m_y, 0);\n"),
    )
    for name, replacement in alternatives:
        yield name, original.replace(query, replacement)


def semantic_forms():
    for label, text in forms():
        yield label, text
        for suffix, candidate in refinements(text):
            yield label + "+" + suffix, candidate


@functools.lru_cache(maxsize=1)
def admitted_bodies():
    return frozenset(text for _, text in semantic_forms())


def load_parents(checkpoint_path, source):
    directory = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    payload = json.loads((directory / "input.json").read_text())
    expected = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                    axes=make_axes(source), evidence=__doc__)
    if payload != expected:
        raise ValueError("review the completed group-commit family")
    for relative in (SOURCE, "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
        if (directory / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("review parents against changed source: " + relative)
    saved, current = directory / "snapshot/include", HOMM3_DIR / "include"
    if {p.relative_to(saved) for p in saved.rglob("*") if p.is_file()} != {
            p.relative_to(current) for p in current.rglob("*") if p.is_file()}:
        raise ValueError("review parents against changed header population")
    for path in saved.rglob("*"):
        if path.is_file() and path.read_bytes() != (current / path.relative_to(saved)).read_bytes():
            raise ValueError("review parents against changed header: " + str(path.relative_to(saved)))
    records, elites = checkpoint["records"], checkpoint["elites"]
    if len(records) != 60 or any(not row.get("scores") for row in records) or len(elites) != 10:
        raise ValueError("expected all 60 scored states and ten reproduced elites")
    parents = []
    for row in elites:
        candidate = directory / "candidates" / row["id"]
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        for key in ("object_hash", "scores", "source_hashes", "choices"):
            if repeated.get(key) != row[key]:
                raise ValueError("parent did not reproduce: " + row["id"])
        text = (candidate / "first/tree" / SOURCE).read_text()
        if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("parent source hash changed: " + row["id"])
        parents.append((row["id"], text))
    return parents


def make_parent_axes(source, parents):
    if len(parents) != 10:
        raise ValueError("expected ten reproduced parents")
    helper = helpers()
    original = helper.definition(source, FUNCTION)
    retained = [(label, helper.definition(text, FUNCTION)) for label, text in parents]
    if len({text for _, text in retained}) != 10:
        raise ValueError("expected ten distinct parent sources")
    options = helper.axis("group_commit_frontier", SOURCE, original, retained)["options"]
    seen = {option["replace"] for option in options}
    inputs = [("baseline", original), *retained]
    successors = []
    for index, (_, text) in enumerate(inputs):
        mutations = list(refinements(text))
        rotation = index % len(mutations)
        successors.append(mutations[rotation:] + mutations[:rotation])
    for step in itertools.zip_longest(*successors):
        for (parent, _), entry in zip(inputs, step):
            if entry is None:
                continue
            label, text = entry
            if text not in seen:
                options.append(dict(name=parent + "+" + label, replace=text))
                seen.add(text)
            if len(options) == 60:
                return [dict(name="group_commit_frontier", source=SOURCE, find=original, options=options)]
    raise ValueError("frontier does not supply 60 distinct states")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                   axes=(make_parent_axes(source, load_parents(args.parents_from, source))
                         if args.parents_from else make_axes(source)),
                   evidence=PARENT_EVIDENCE if args.parents_from else __doc__)
    if args.parents_from:
        payload["parent_checkpoint"] = str(args.parents_from.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 treasure-group commit states ->", args.output)


if __name__ == "__main__":
    main()
