#!/usr/bin/env python3
"""60 coordinate-copy and centroid lifetime hypotheses for retail 0x53d0d0.

Both accessors expand and all eleven CFG blocks already agree structurally.
Retail preserves a full coordinate copy with an extra four-byte frame slot;
the sums and center-copy scheduling motivate real coordinate lifetimes.
No Dreamcast counterpart is claimed. Keep the actual accessors/constructors,
signed zone comparisons, bounded y/x scan and zero-count guard.
"""
import argparse
import hashlib
import importlib.util
import itertools
import json
import re
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::recenterZone"


def helpers():
    spec = importlib.util.spec_from_file_location("recenter_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def forms():
    for capture, accumulator, order in itertools.product(range(5), range(4), range(3)):
        bounds = "    TRmgZoneBounds bounds = zone->m_bounds;\n"
        index = "    int zoneIndex = zone->m_slot->m_zoneIndex;\n"
        position = (
            "    TRmgMapPosition position = zone->getLevelPosition();\n",
            "    TRmgMapPosition position(zone->getLevelPosition());\n",
            "    TRmgMapPosition position;\n    position = zone->getLevelPosition();\n",
            "    const TRmgMapPosition& original = zone->getLevelPosition();\n    TRmgMapPosition position = original;\n",
            "    const TRmgMapPosition& position = zone->getLevelPosition();\n",
        )[capture]
        declarations = (bounds + index + position, bounds + position + index, index + bounds + position)[order]
        total = (
            "    TPoint total(0, 0);\n",
            "    TRmgVector total(0, 0);\n",
            "    TPoint total;\n    total.m_x = 0;\n    total.m_y = 0;\n",
            "    TRmgMapPosition total(0, 0, position.m_z);\n",
        )[accumulator]
        body = "void type_random_map_generator::recenterZone(TRmgZone* zone)\n{\n" + declarations + "    int count = 0;\n" + total + """    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
        for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
            if (m_map.getMapItem(x, y, position.m_z)->m_zoneState.m_zone == zoneIndex) {
                ++count;
                total.m_x += x;
                total.m_y += y;
            }
        }
    }
    if (count) {
"""
        if accumulator == 3:
            body += "        total.m_x /= count;\n        total.m_y /= count;\n        zone->setLevelPosition(total);\n"
        elif capture == 4:
            body += "        TRmgMapPosition centered = position;\n        centered.m_x = total.m_x / count;\n        centered.m_y = total.m_y / count;\n        zone->setLevelPosition(centered);\n"
        else:
            body += "        position.m_x = total.m_x / count;\n        position.m_y = total.m_y / count;\n        zone->setLevelPosition(position);\n"
        body += "    }\n}"
        yield f"capture_{capture}+accumulator_{accumulator}+order_{order}", body


def make_axes(source):
    original = helpers().definition(source, FUNCTION)
    alternatives = list(forms())
    if original not in {body for _, body in alternatives}:
        if not any(original == body for _, seed in alternatives for _, body in refinements(seed)):
            raise ValueError("review current recenterZone body")
        # After adopting a proven child, retain it as the unchanged-source
        # control and sample 59 original alternatives. Old checkpoints still
        # require their exact pre-adoption source/header snapshot.
        alternatives = alternatives[:59]
    result = helpers().axis("recenter", SOURCE, original, alternatives)
    if len(result["options"]) != 60:
        raise ValueError("expected 60 distinct recenter states")
    return [result]


def level_sum(body):
    """A three-coordinate centroid retains the original level as real data."""
    body, count = re.subn(r"    (?:TPoint|TRmgVector) total\(0, 0\);|    TPoint total;\n    total.m_x = 0;\n    total.m_y = 0;|    TRmgMapPosition total\(0, 0, position.m_z\);|    TRmgMapPosition total;\n    total.m_x = 0;\n    total.m_y = 0;\n    total.m_z = position.m_z;",
        "    TRmgMapPosition total;\n    total.m_x = 0;\n    total.m_y = 0;\n    total.m_z = position.m_z;", body)
    if count != 1:
        raise ValueError("review centroid accumulator")
    start = body.index("    if (count) {")
    return body[:start] + "    if (count) {\n        total.m_x /= count;\n        total.m_y /= count;\n        zone->setLevelPosition(total);\n    }\n}"


def refinements(body):
    yield "level_sum", level_sum(body)
    no_count = body.replace("    int count = 0;\n", "")
    yield "count_first", no_count.replace("{\n", "{\n    int count = 0;\n", 1)
    yield "count_after_bounds", no_count.replace("    TRmgZoneBounds bounds = zone->m_bounds;\n", "    TRmgZoneBounds bounds = zone->m_bounds;\n    int count = 0;\n")
    yield "count_after_total", no_count.replace("    for (int y", "    int count = 0;\n    for (int y", 1)
    yield "split_count", body.replace("    int count = 0;", "    int count;\n    count = 0;")
    mapped = body.replace("m_map.", "map.")
    yield "map_reference", mapped.replace("{\n", "{\n    type_random_map& map = m_map;\n", 1)
    yield "level_sum_count_first", level_sum(no_count.replace("{\n", "{\n    int count = 0;\n", 1))


def scheduling_refinements(body):
    body = level_sum(body)
    old = "    TRmgMapPosition total;\n    total.m_x = 0;\n    total.m_y = 0;\n    total.m_z = position.m_z;"
    initializers = [
        ("copy", "    TRmgMapPosition total = position;\n    total.m_x = 0;\n    total.m_y = 0;"),
        ("direct_copy", "    TRmgMapPosition total(position);\n    total.m_x = 0;\n    total.m_y = 0;"),
        ("assigned_copy", "    TRmgMapPosition total;\n    total = position;\n    total.m_x = 0;\n    total.m_y = 0;"),
    ]
    for order in itertools.permutations("xyz"):
        initializers.append(("fields_" + "".join(order), "    TRmgMapPosition total;\n" + "\n".join(
            "    total.m_" + field + " = " + ("position.m_z" if field == "z" else "0") + ";" for field in order)))
    for label, initializer in initializers:
        candidate = body.replace(old, initializer)
        yield label, candidate
        without_count = re.sub(r"    int count(?: = 0;|;\n    count = 0;)[\n]", "", candidate)
        yield label + "_count_first", without_count.replace("{\n", "{\n    int count = 0;\n", 1)
        yield label + "_count_last", without_count.replace("    for (int y", "    int count = 0;\n    for (int y", 1)


def load_parents(checkpoint_path, source, *, frontier=False):
    directory = checkpoint_path.parent
    payload = json.loads((directory / "input.json").read_text())
    expected = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=make_axes(source), evidence=__doc__)
    if frontier:
        prior = Path(payload["parent_checkpoint"])
        expected.update(axes=make_parent_axes(source, load_parents(prior, source)), parent_checkpoint=str(prior.resolve()))
    if payload != expected:
        raise ValueError("review recenter parent manifest")
    for relative in (SOURCE, "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
        if (directory / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("review changed parent source " + relative)
    saved, current = directory / "snapshot/include", HOMM3_DIR / "include"
    if {p.relative_to(saved) for p in saved.rglob("*") if p.is_file()} != {p.relative_to(current) for p in current.rglob("*") if p.is_file()}:
        raise ValueError("review changed header population")
    for path in saved.rglob("*"):
        if path.is_file() and path.read_bytes() != (current / path.relative_to(saved)).read_bytes():
            raise ValueError("review changed header " + str(path))
    checkpoint = json.loads(checkpoint_path.read_text())
    if len(checkpoint["records"]) != 60 or any(not row.get("scores") for row in checkpoint["records"]) or len(checkpoint["elites"]) != 10:
        raise ValueError("expected 60 successful states and ten reproduced elites")
    parents = []
    for row in checkpoint["elites"]:
        candidate = directory / "candidates" / row["id"]
        repeat = json.loads((candidate / "repeat/result.json").read_text())
        if any(repeat.get(key) != row[key] for key in ("object_hash", "scores", "source_hashes", "choices")):
            raise ValueError("parent did not reproduce")
        text = (candidate / "first/tree" / SOURCE).read_text()
        if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("parent source changed")
        parents.append((row["id"], text))
    return parents


def make_parent_axes(source, parents, *, scheduling=False, balanced=False):
    original = helpers().definition(source, FUNCTION)
    retained = [(label, helpers().definition(text, FUNCTION)) for label, text in parents]
    if len(retained) != 10 or len({text for _, text in retained}) != 10:
        raise ValueError("expected ten distinct reproduced parent sources")
    options = helpers().axis("recenter_frontier", SOURCE, original, retained)["options"]
    seen = {row["replace"] for row in options}
    refine = scheduling_refinements if scheduling else refinements
    mutations = []
    for index, (_, text) in enumerate(retained):
        variants = list(refine(text))
        # Three adjacent entries share one initializer. Rotate by initializer,
        # not spelling, so a finite 60-state frontier covers all nine families
        # instead of exhausting its width on just the first two copy forms.
        offset = index * 3 % len(variants) if balanced else 0
        mutations.append(variants[offset:] + variants[:offset])
    for entries in itertools.zip_longest(*mutations):
        for (parent, _), entry in zip(retained, entries):
            if entry is None:
                continue
            label, body = entry
            if body not in seen:
                seen.add(body)
                options.append(dict(name=parent + "+" + label, replace=body))
            if len(options) == 60:
                return [dict(name="recenter_frontier", source=SOURCE, find=original, options=options)]
    raise ValueError("expected sixty distinct frontier states")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    parser.add_argument("--centroid-parents-from", type=Path)
    parser.add_argument("--centroid-balanced-parents-from", type=Path)
    args = parser.parse_args()
    if sum(bool(value) for value in (args.parents_from, args.centroid_parents_from, args.centroid_balanced_parents_from)) > 1:
        parser.error("select only one parent stage")
    centroid = args.centroid_balanced_parents_from or args.centroid_parents_from
    checkpoint = centroid or args.parents_from
    source = (HOMM3_DIR / SOURCE).read_text()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
        axes=make_parent_axes(source, load_parents(checkpoint, source, frontier=bool(centroid)), scheduling=bool(centroid), balanced=bool(args.centroid_balanced_parents_from)) if checkpoint else make_axes(source), evidence=__doc__)
    if checkpoint:
        payload["parent_checkpoint"] = str(checkpoint.resolve())
    if centroid:
        payload["stage"] = "centroid_balanced" if args.centroid_balanced_parents_from else "centroid_scheduling"
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 recenter states ->", args.output)


if __name__ == "__main__":
    main()
