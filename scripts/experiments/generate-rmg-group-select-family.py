#!/usr/bin/env python3
"""Generate 60 real candidate-selection states for placeTreasureGroup, 0x5470d0.

Retail's 646-byte Complete-only body retains the placement filter, vector
count-insert, random selection and group commit. Its clear expansion calls
std::copy then vector::_Destroy; the 77.1261% control expands copy instead,
with a four-byte frame surplus and shifted coordinate homes. Cross five
entry-declaration orders, four centered-query value forms and three public
vector-emptying APIs. All snapshots stay before opaque placement calls;
preserve row-major enumeration, score re-reads, tie order and random draws.
Do not paste helper bodies or introduce false inline declarations or pins.
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
FUNCTION = "type_random_map_generator::placeTreasureGroup"
PARENT_EVIDENCE = (
    "Retain ten reproduced selection parents plus a current-source control "
    "and 49 new public-container/bounds-binding forms. The first 60 states "
    "reach 86.8529%, but resize(0) retains size/erase rather than retail's "
    "copy/_Destroy expansion, while clear/erase parents still expand copy. "
    "Test real iterator lifetimes, public append interfaces and group-bound "
    "copy/reference binding; every bound is consumed before the first opaque "
    "fit call, and score loads after that call must remain live re-reads.")
SHAPE_EVIDENCE = (
    "The second 60-state population reproduces 86.8529% without restoring "
    "retail's nested copy call. Preserve ten distinct finalists and inspect "
    "the centered lookup: retail loads center X, then scan X and center Y. "
    "Reverse the real point/vector operand roles, keep a named translated "
    "point, or spell the same three ordered cell filters as continue/nested "
    "guards. Also test the final selected coordinate as a real value or "
    "direct-copy assignment. Preserve all pre-call snapshots and post-call "
    "score re-reads, canonical helpers and random draw/tie order.")
LIFETIME_EVIDENCE = (
    "The third population reaches 87.0168% and restores the exact nine-call "
    "sequence: center-point compound translation plus public single insert "
    "allows clear to retain std::copy and _Destroy naturally. The frame is "
    "retail's 0x48, but coordinate/vector homes and the loop reload split "
    "still differ. Retest meaningful declaration orders under this new "
    "inline state, and hoist the already-used center, scan position or query "
    "point without moving any initializer, getter, field read or helper call.")


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_group_select_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


BASELINE = """unsigned char type_random_map_generator::placeTreasureGroup(TRmgTreasureGroup* group,
    TRmgZone* zone, int spacing)
{
    int zoneIndex = zone->m_slot->m_zoneIndex;
    std::vector<TRmgMapPosition> candidates;
    TRmgZoneBounds bounds = zone->m_bounds;
    TRmgZoneBounds groupBounds = group->m_bounds;
    bounds.m_minimumY -= groupBounds.m_minimumY;
    TRmgMapPosition position = zone->getLevelPosition();
    position.m_y = bounds.m_minimumY;
    bounds.m_maximumX += 1 - groupBounds.m_maximumX;
    bounds.m_maximumY += 1 - groupBounds.m_maximumY;
    bounds.m_minimumX -= groupBounds.m_minimumX;
    TPoint center;
    center.m_x = (groupBounds.m_minimumX + groupBounds.m_maximumX) / 2;
    center.m_y = (groupBounds.m_minimumY + groupBounds.m_maximumY) / 2;
    for (; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position.m_x + center.m_x,
                position.m_y + center.m_y, position.m_z);
            if (item->m_zoneState.m_zone == zoneIndex && item->m_zoneState.m_score >= spacing
                && canPlaceTreasureGroup(group, position, zone)) {
                if (item->m_zoneState.m_score > spacing) {
                    spacing = item->m_zoneState.m_score;
                    candidates.clear();
                }
                candidates.push_back(position);
            }
        }
    }
    if (candidates.size() == 0)
        return 0;
    position = candidates[rand() % candidates.size()];
    commitTreasureGroup(group, position);
    return 1;
}"""

ENTRY = (
    "    int zoneIndex = zone->m_slot->m_zoneIndex;\n",
    "    std::vector<TRmgMapPosition> candidates;\n",
    "    TRmgZoneBounds bounds = zone->m_bounds;\n",
    "    TRmgZoneBounds groupBounds = group->m_bounds;\n",
)
ENTRY_ORDERS = ((0, 1, 2, 3), (1, 0, 2, 3), (1, 2, 3, 0), (2, 3, 1, 0), (0, 1, 3, 2))
QUERIES = (
    ("scalar", "            TRmgMapItem* item = m_map.getMapItem(position.m_x + center.m_x,\n"
     "                position.m_y + center.m_y, position.m_z);\n"),
    ("position_sum", "            TRmgMapItem* item = m_map.getMapItem(position + center);\n"),
    ("named_position", "            TRmgMapPosition queryPosition = position;\n"
     "            queryPosition += center;\n"
     "            TRmgMapItem* item = m_map.getMapItem(queryPosition);\n"),
    ("point_sum", "            TPoint queryPoint = TPoint(position.m_x, position.m_y)\n"
     "                + TRmgVector(center.m_x, center.m_y);\n"
     "            TRmgMapItem* item = m_map.getMapItem(queryPoint.m_x, queryPoint.m_y, position.m_z);\n"),
)
EMPTY = (("clear", "candidates.clear();"),
         ("erase_range", "candidates.erase(candidates.begin(), candidates.end());"),
         ("resize_zero", "candidates.resize(0);"))


def forms():
    for order, (query_name, query), (empty_name, empty) in itertools.product(ENTRY_ORDERS, QUERIES, EMPTY):
        body = BASELINE.replace("".join(ENTRY), "".join(ENTRY[index] for index in order))
        body = body.replace(QUERIES[0][1], query).replace(EMPTY[0][1], empty)
        yield "entry_" + "".join(map(str, order)) + "+" + query_name + "+" + empty_name, body


def refinements(text):
    bounds = "    TRmgZoneBounds groupBounds = group->m_bounds;\n"
    if text.count(bounds) != 1:
        raise ValueError("review the group-bounds value boundary")
    for label, replacement in (
            ("bounds_reference", "    const TRmgZoneBounds& groupBounds = group->m_bounds;\n"),
            ("bounds_direct", "    TRmgZoneBounds groupBounds(group->m_bounds);\n"),
            ("bounds_assigned", "    TRmgZoneBounds groupBounds;\n    groupBounds = group->m_bounds;\n")):
        yield label, text.replace(bounds, replacement)
    found = [empty for _, empty in EMPTY if empty in text]
    if len(found) != 1:
        raise ValueError("review the current public vector emptying operation")
    before = "                    " + found[0] + "\n"
    first = "                    std::vector<TRmgMapPosition>::iterator first = candidates.begin();\n"
    last = "                    std::vector<TRmgMapPosition>::iterator last = candidates.end();\n"
    for label, replacement in (
            ("erase_named_range", first + last + "                    candidates.erase(first, last);\n"),
            ("erase_named_reverse", last + first + "                    candidates.erase(first, last);\n"),
            ("erase_named_last", last + "                    candidates.erase(candidates.begin(), last);\n"),
            ("erase_named_first", first + "                    candidates.erase(first, candidates.end());\n")):
        yield label, text.replace(before, replacement)
    append = "                candidates.push_back(position);\n"
    if text.count(append) != 1:
        raise ValueError("review the candidate append boundary")
    for label, replacement in (
            ("insert_single", "                candidates.insert(candidates.end(), position);\n"),
            ("insert_count", "                candidates.insert(candidates.end(), 1, position);\n")):
        yield label, text.replace(append, replacement)
    selected = "    position = candidates[rand() % candidates.size()];\n"
    if text.count(selected) != 1:
        raise ValueError("review the random selection value boundary")
    yield "selection_reference", text.replace(selected,
        "    const TRmgMapPosition& selected = candidates[rand() % candidates.size()];\n    position = selected;\n")


def shape_refinements(text):
    found = [query for _, query in QUERIES if query in text]
    if len(found) != 1:
        raise ValueError("review the centered map query")
    for label, query in (
            ("center_point_sum", "            TPoint queryPoint = center\n"
             "                + TRmgVector(position.m_x, position.m_y);\n"
             "            TRmgMapItem* item = m_map.getMapItem(queryPoint.m_x, queryPoint.m_y, position.m_z);\n"),
            ("center_point_compound", "            TPoint queryPoint = center;\n"
             "            queryPoint += TRmgVector(position.m_x, position.m_y);\n"
             "            TRmgMapItem* item = m_map.getMapItem(queryPoint.m_x, queryPoint.m_y, position.m_z);\n")):
        yield label, text.replace(found[0], query)
    condition = ("            if (item->m_zoneState.m_zone == zoneIndex && item->m_zoneState.m_score >= spacing\n"
                 "                && canPlaceTreasureGroup(group, position, zone)) {\n")
    start = text.index(condition)
    end = text.index("        }\n    }\n    if (candidates.size()", start)
    block = text[start:end]
    if not block.endswith("            }\n"):
        raise ValueError("review the three-filter selection body")
    inside = block[len(condition):-len("            }\n")]
    less = "".join(line[4:] if line.startswith("    ") else line for line in inside.splitlines(keepends=True))
    more = "".join("        " + line for line in inside.splitlines(keepends=True))
    for label, block in (
            ("continue_filters", "            if (item->m_zoneState.m_zone != zoneIndex)\n                continue;\n"
             "            if (item->m_zoneState.m_score < spacing)\n                continue;\n"
             "            if (!canPlaceTreasureGroup(group, position, zone))\n                continue;\n" + less),
            ("nested_filters", "            if (item->m_zoneState.m_zone == zoneIndex) {\n"
             "                if (item->m_zoneState.m_score >= spacing) {\n"
             "                    if (canPlaceTreasureGroup(group, position, zone)) {\n" + more
             + "                    }\n                }\n            }\n"),
            ("zone_guard_then_continue", "            if (item->m_zoneState.m_zone == zoneIndex) {\n"
             "                if (item->m_zoneState.m_score < spacing)\n                    continue;\n"
             "                if (!canPlaceTreasureGroup(group, position, zone))\n                    continue;\n"
             + inside + "            }\n")):
        yield label, text[:start] + block + text[end:]
    selected = "    position = candidates[rand() % candidates.size()];\n"
    reference = ("    const TRmgMapPosition& selected = candidates[rand() % candidates.size()];\n"
                 "    position = selected;\n")
    if reference in text:
        selection = reference
    elif selected in text:
        selection = selected
    else:
        raise ValueError("review the final coordinate selection")
    query = "candidates[rand() % candidates.size()]"
    yield "selected_value", text.replace(selection, "    TRmgMapPosition selected = " + query + ";\n").replace(
        "commitTreasureGroup(group, position);", "commitTreasureGroup(group, selected);")
    yield "selected_direct_copy", text.replace(selection, "    position = TRmgMapPosition(" + query + ");\n")


def lifetime_refinements(text):
    first = text.index("{\n") + 2
    last = text.index("    bounds.m_minimumY -= groupBounds.m_minimumY;\n", first)
    entry = text[first:last]
    group_forms = [ENTRY[3], "    const TRmgZoneBounds& groupBounds = group->m_bounds;\n",
                   "    TRmgZoneBounds groupBounds(group->m_bounds);\n",
                   "    TRmgZoneBounds groupBounds;\n    groupBounds = group->m_bounds;\n"]
    group = [item for item in group_forms if item in entry]
    if len(group) != 1:
        raise ValueError("review the group-bounds entry declaration")
    declarations = (*ENTRY[:3], group[0])
    if any(entry.count(item) != 1 for item in declarations) or "".join(
            sorted(declarations, key=entry.index)) != entry:
        raise ValueError("review the four entry-value lifetimes")
    for order in (*ENTRY_ORDERS, (0, 2, 3, 1)):
        changed = "".join(declarations[index] for index in order)
        if changed != entry:
            yield "entry_" + "".join(map(str, order)), text[:first] + changed + text[last:]
    center = "    TPoint center;\n"
    position = "    TRmgMapPosition position = zone->getLevelPosition();\n"
    if text.count(center) != 1 or text.count(position) != 1:
        raise ValueError("review the center/scan-position value declarations")
    for label, anchor in (("before_vector", ENTRY[1]), ("before_bounds", ENTRY[2])):
        yield "center_" + label, text.replace(center, "").replace(anchor, center + anchor, 1)
        changed = text.replace(position, "    position = zone->getLevelPosition();\n")
        yield "position_" + label, changed.replace(anchor, "    TRmgMapPosition position;\n" + anchor, 1)
    query = "            TPoint queryPoint = "
    if query in text:
        changed = text.replace(query, "            queryPoint = ", 1)
        anchor = "    for (; position.m_y < bounds.m_maximumY; ++position.m_y) {\n"
        if text.count(anchor) != 1:
            raise ValueError("review the row traversal before reusing query storage")
        yield "query_across_rows", changed.replace(anchor, "    TPoint queryPoint;\n" + anchor, 1)
        yield "query_within_row", changed.replace(anchor, anchor + "        TPoint queryPoint;\n", 1)


@functools.lru_cache(maxsize=1)
def admitted_bodies():
    result = set()
    for _, text in forms():
        result.add(text)
        result.update(candidate for _, candidate in refinements(text))
    for parent in list(result):
        result.update(candidate for _, candidate in shape_refinements(parent))
    return frozenset(result)


@functools.lru_cache(maxsize=None)
def is_admitted(text):
    if text in admitted_bodies():
        return True
    return any(text == candidate for parent in admitted_bodies()
               for _, candidate in lifetime_refinements(parent))


def make_axes(source):
    helper = helpers()
    original = helper.definition(source, FUNCTION)
    options = list(forms())
    if not is_admitted(original):
        raise ValueError("review the current treasure-position selection before rebasing")
    if original not in dict(options).values():
        options = options[:-1]
    return [helper.axis("group_select_boundaries", SOURCE, original, options)]


def load_parents(checkpoint_path, source, *, frontier=False, shape=False):
    directory = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    payload = json.loads((directory / "input.json").read_text())
    expected = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=make_axes(source), evidence=__doc__)
    if shape:
        prior = Path(payload["parent_checkpoint"])
        expected.update(axes=make_parent_axes(source, load_parents(prior, source, frontier=True), shape=True),
                        evidence=SHAPE_EVIDENCE, parent_checkpoint=str(prior.resolve()))
    elif frontier:
        prior = Path(payload["parent_checkpoint"])
        expected.update(axes=make_parent_axes(source, load_parents(prior, source)),
                        evidence=PARENT_EVIDENCE, parent_checkpoint=str(prior.resolve()))
    if payload != expected:
        raise ValueError("review the completed selection family")
    for relative in (SOURCE, "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
        if (directory / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("review selection parents against changed source: " + relative)
    saved, current = directory / "snapshot/include", HOMM3_DIR / "include"
    if {p.relative_to(saved) for p in saved.rglob("*") if p.is_file()} != {
            p.relative_to(current) for p in current.rglob("*") if p.is_file()}:
        raise ValueError("review selection parents against changed header population")
    for path in saved.rglob("*"):
        if path.is_file() and path.read_bytes() != (current / path.relative_to(saved)).read_bytes():
            raise ValueError("review selection parents against changed header: " + str(path.relative_to(saved)))
    records, elites = checkpoint["records"], checkpoint["elites"]
    if len(records) != 60 or any(not row.get("scores") for row in records) or len(elites) != 10:
        raise ValueError("expected all 60 scored states and ten reproduced elites")
    parents = []
    for row in elites:
        candidate = directory / "candidates" / row["id"]
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        for key in ("object_hash", "scores", "source_hashes", "choices"):
            if repeated.get(key) != row[key]:
                raise ValueError("selection parent did not reproduce: " + row["id"])
        text = (candidate / "first/tree" / SOURCE).read_text()
        if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("selection parent source hash changed: " + row["id"])
        parents.append((row["id"], text))
    return parents


def make_parent_axes(source, parents, *, shape=False, lifetime=False):
    if len(parents) != 10:
        raise ValueError("expected ten reproduced selection parents")
    helper = helpers()
    original = helper.definition(source, FUNCTION)
    retained = [(label, helper.definition(text, FUNCTION)) for label, text in parents]
    if len({text for _, text in retained}) != 10:
        raise ValueError("expected ten distinct selection parent sources")
    options = helper.axis("group_select_frontier", SOURCE, original, retained)["options"]
    seen = {option["replace"] for option in options}
    inputs = [("baseline", original), *retained]
    successors = []
    for index, (_, text) in enumerate(inputs):
        mutations = list((lifetime_refinements if lifetime else shape_refinements if shape else refinements)(text))
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
                return [dict(name="group_select_frontier", source=SOURCE, find=original, options=options)]
    raise ValueError("selection frontier does not supply 60 distinct states")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    parser.add_argument("--shape-parents-from", type=Path)
    parser.add_argument("--lifetime-parents-from", type=Path)
    args = parser.parse_args()
    if sum(bool(value) for value in (args.parents_from, args.shape_parents_from, args.lifetime_parents_from)) > 1:
        parser.error("select only one parent stage")
    source = (HOMM3_DIR / SOURCE).read_text()
    checkpoint = args.lifetime_parents_from or args.shape_parents_from or args.parents_from
    shape = bool(args.shape_parents_from)
    lifetime = bool(args.lifetime_parents_from)
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                   axes=make_parent_axes(source, load_parents(checkpoint, source, frontier=shape, shape=lifetime),
                                         shape=shape, lifetime=lifetime)
                        if checkpoint else make_axes(source),
                   evidence=LIFETIME_EVIDENCE if lifetime else SHAPE_EVIDENCE if shape
                            else PARENT_EVIDENCE if checkpoint else __doc__)
    if checkpoint:
        payload["parent_checkpoint"] = str(checkpoint.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 treasure-position selection states ->", args.output)


if __name__ == "__main__":
    main()
