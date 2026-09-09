#!/usr/bin/env python3
"""Key-tent guard owner and result lifetimes at retail 0x54b8c0.

Retail retains the outline's two-coordinate lookup and the failure reset,
while our caller expands both. Its guard remains in EDI rather than a stack
home. Test actual origin/outline bindings and fill/add result lifetimes;
preserve canonical helpers and automatic treasure-group ownership. There is
no mapped Dreamcast counterpart. All rmg siblings are scored.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator

NAME = "type_random_map_generator::placeKeyTentGuard"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def variants(original):
    origin = "    TRmgZone* origin = m_zones[m_map.getMapItem(object->m_position)->m_zoneState.m_zone];"
    outline = """        for (unsigned int i = 0; i < group.m_outline.size(); ++i)
            group.m_map.getMapItem(group.m_outline[i].m_x,
                group.m_outline[i].m_y)->m_tileData.m_placementOutline = 1;"""
    guard = "    if (fillTreasureGroup(origin, &group, 0, maxValue) && group.addGuard(guard)) {"
    for anchor in (origin, outline, guard):
        if original.count(anchor) != 1:
            raise ValueError("review changed key-tent source anchor: " + anchor)
    origins = [origin,
        "    TRmgMapItem* originItem = m_map.getMapItem(object->m_position);\n    TRmgZone* origin = m_zones[originItem->m_zoneState.m_zone];",
        "    TRmgMapItem& originItem = *m_map.getMapItem(object->m_position);\n    TRmgZone* origin = m_zones[originItem.m_zoneState.m_zone];",
        "    TRmgMapPosition originPosition = object->m_position;\n" + origin.replace("object->m_position", "originPosition"),
        "    const TRmgMapPosition& originPosition = object->m_position;\n" + origin.replace("object->m_position", "originPosition"),
    ]
    outlines = [outline]
    for binding in ("TPoint point =", "const TPoint& point ="):
        outlines.append("""        for (unsigned int i = 0; i < group.m_outline.size(); ++i) {
            %s group.m_outline[i];
            group.m_map.getMapItem(point.m_x, point.m_y)->m_tileData.m_placementOutline = 1;
        }""" % binding)
    outlines.append("""        for (unsigned int i = 0; i < group.m_outline.size(); ++i) {
            TRmgMapItem* item = group.m_map.getMapItem(group.m_outline[i].m_x,
                group.m_outline[i].m_y);
            item->m_tileData.m_placementOutline = 1;
        }""")
    guards = [guard,
        "    int filled = fillTreasureGroup(origin, &group, 0, maxValue);\n    if (filled && group.addGuard(guard)) {",
        "    unsigned char added = 0;\n    if (fillTreasureGroup(origin, &group, 0, maxValue))\n        added = group.addGuard(guard);\n    if (added) {",
    ]
    for a, b, c in itertools.product(range(5), range(4), range(3)):
        yield "origin_%d+outline_%d+result_%d" % (a, b, c), original.replace(origin, origins[a]).replace(outline, outlines[b]).replace(guard, guards[c])


def receiver_refinement(parent, form):
    if form == 0:
        anchor = "        for (unsigned int i = 0; i < group.m_outline.size(); ++i)"
        if parent.count(anchor) != 1:
            raise ValueError("review outline scan")
        return parent.replace(anchor, "        type_random_map& outlineMap = group.m_map;\n" + anchor).replace("group.m_map.getMapItem", "outlineMap.getMapItem")
    if form == 1:
        return parent.replace("    group.reset();", "    TRmgTreasureGroup& resetGroup = group;\n    resetGroup.reset();")
    if form == 2:
        anchor = "    unsigned int index = 0;"
        return parent.replace(anchor, "    std::vector<unsigned char>& disabled = m_disabledKeyTents;\n" + anchor).replace("m_disabledKeyTents[", "disabled[").replace("m_disabledKeyTents.size()", "disabled.size()")
    if form == 3:
        return parent.replace("for (unsigned int i = 0; i < group.m_outline.size(); ++i)", "for (index = 0; index < group.m_outline.size(); ++index)").replace("group.m_outline[i]", "group.m_outline[index]").replace("for (unsigned int i = 0; i < group.m_objects.size(); ++i)", "for (index = 0; index < group.m_objects.size(); ++index)").replace("group.m_objects[i]", "group.m_objects[index]")
    if form == 4:
        anchor = "    if (index == m_objectPrototypes[BORDER_GUARD].size())\n        return 0;"
        if parent.count(anchor) != 1:
            raise ValueError("review origin entry")
        return parent.replace(anchor, anchor + "\n    type_random_map& originMap = m_map;").replace("m_map.getMapItem(object->m_position)", "originMap.getMapItem(object->m_position)").replace("m_map.getMapItem(originPosition)", "originMap.getMapItem(originPosition)")
    raise ValueError("unknown receiver family")


def parents(source, checkpoint_path, extract=definition):
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get("generation", 0) < 1 or len(checkpoint["elites"]) != 10:
        raise ValueError("unfinished key-tent parent population")
    _, originals, axes = load_manifest(context / "input.json", HOMM3_DIR)
    finite = set(itertools.product(*(range(len(axis.options)) for axis in axes)))
    recorded = {tuple(row["choices"]) for row in checkpoint["records"]}
    if len(checkpoint["records"]) != min(60, len(finite)) or len(recorded) != len(checkpoint["records"]) or not recorded <= finite or (len(finite) <= 60 and recorded != finite) or any(not row["scores"] for row in checkpoint["records"]):
        raise ValueError("key-tent population is not fully scored")
    for folder in ("src", "include"):
        frozen, live = context / "snapshot" / folder, HOMM3_DIR / folder
        paths = [p.relative_to(frozen) for p in frozen.rglob("*") if p.is_file()]
        live_paths = [p.relative_to(live) for p in live.rglob("*") if p.is_file() and "build" not in p.relative_to(live).parts]
        if set(paths) != set(live_paths) or any((frozen / p).read_bytes() != (live / p).read_bytes() for p in paths):
            raise ValueError("changed key-tent snapshot: " + folder)
    retained = []
    for elite in checkpoint["elites"]:
        rendered = source_families.render(originals, axes, tuple(elite["choices"]))
        repeated = context / "candidates" / elite["id"] / "repeat"
        result = json.loads((repeated / "result.json").read_text())
        for key in ("scores", "object_hash", "source_hashes", "choices"):
            if result[key] != elite[key]:
                raise ValueError("key-tent parent did not reproduce " + key)
        for relative, text in rendered.items():
            if (repeated / "tree" / relative).read_text() != text or result["source_hashes"][relative] != source_families.digest(text.encode()):
                raise ValueError("changed reproduced key-tent source")
        parent = extract(rendered["src/rmg.cpp"])
        retained.append((elite["id"], parent))
    return retained


def result_refinement(parent, form):
    anchors = [
        "    if (fillTreasureGroup(origin, &group, 0, maxValue) && group.addGuard(guard)) {",
        "    int filled = fillTreasureGroup(origin, &group, 0, maxValue);\n    if (filled && group.addGuard(guard)) {",
        "    unsigned char added = 0;\n    if (fillTreasureGroup(origin, &group, 0, maxValue))\n        added = group.addGuard(guard);\n    if (added) {",
    ]
    found = [anchor for anchor in anchors if parent.count(anchor) == 1]
    if len(found) != 1:
        raise ValueError("review key-tent result ownership")
    expression = "fillTreasureGroup(origin, &group, 0, maxValue) && group.addGuard(guard)"
    replacements = [
        "    unsigned char accepted = " + expression + ";\n    if (accepted) {",
        "    bool accepted = " + expression + ";\n    if (accepted) {",
        "    unsigned char accepted;\n    if (fillTreasureGroup(origin, &group, 0, maxValue))\n        accepted = group.addGuard(guard);\n    else\n        accepted = 0;\n    if (accepted) {",
        "    int filled = fillTreasureGroup(origin, &group, 0, maxValue);\n    unsigned char accepted = 0;\n    if (filled)\n        accepted = group.addGuard(guard);\n    if (accepted) {",
        "    if (!fillTreasureGroup(origin, &group, 0, maxValue) || !group.addGuard(guard)) {\n        delete guard;\n    } else {",
    ]
    child = parent.replace(found[0], replacements[form])
    if form == 4:
        tail = "    } else {\n        delete guard;\n    }\n    for ("
        if child.count(tail) != 1:
            raise ValueError("review shared guard-delete branch")
        child = child.replace(tail, "    }\n    for (")
    return child


def frontier(source, checkpoint_path, refine=receiver_refinement):
    current = definition(source)
    forms, seen = [("unchanged", current)], {current}
    retained = parents(source, checkpoint_path)
    for identity, parent in retained:
        if parent not in seen:
            seen.add(parent)
            forms.append((identity + "+parent", parent))
    for form in range(5):
        for identity, parent in retained:
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
    group.add_argument("--receivers-from", type=Path)
    group.add_argument("--results-from", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    original = definition(source)
    if args.results_from:
        forms = frontier(source, args.results_from, result_refinement)
    else:
        forms = frontier(source, args.receivers_from) if args.receivers_from else variants(original)
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[
        generator("generate-rmg-position-family.py").axis("key_tent_lifetimes", "src/rmg.cpp", original, forms)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "key-tent source states")


if __name__ == "__main__":
    main()
