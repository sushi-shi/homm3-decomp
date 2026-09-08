#!/usr/bin/env python3
"""60 real coordinate/vector source states for island fill at 0x53cf50.

Retail insetIslandZone ends with a retained call to this previously omitted
cardinal fill. The seed is unmarked, eligible neighbours are marked on push,
and a coordinate-vector stack owns cleanup. Preserve canonical coordinate
operations and public vector methods. No Dreamcast counterpart is claimed.
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
FUNCTION = "type_random_map_generator::fillIslandInterior"
BASELINE = """void type_random_map_generator::fillIslandInterior(TRmgZone* zone)
{
    std::vector<TRmgMapPosition> pending;
    int zoneIndex = zone->m_slot->m_zoneIndex;
    TRmgMapPosition position = zone->getLevelPosition();
    pending.push_back(position);
    while (pending.size()) {
        position = pending.back();
        pending.pop_back();
        for (int direction = 0; direction < 8; direction += 2) {
            TRmgMapPosition next = position;
            next += g_rmgDirections[direction];
            if (next.m_x < 0 || next.m_x >= m_map.m_mapWidth
                || next.m_y < 0 || next.m_y >= m_map.m_mapHeight)
                continue;
            TRmgMapItem* item = m_map.getMapItem(next.m_x, next.m_y, next.m_z);
            if (item->isZoneBoundary() || item->m_zoneState.m_zone != zoneIndex)
                continue;
            item->m_tileData.m_zoneBoundary = 1;
            pending.push_back(next);
        }
    }
}"""


def helpers():
    spec = importlib.util.spec_from_file_location("island_fill_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def forms():
    for coordinate, append, offset in itertools.product(range(5), range(4), range(3)):
        body = BASELINE
        copy = "            TRmgMapPosition next = position;"
        if coordinate == 1:
            body = body.replace(copy, "            TRmgMapPosition next;\n            next = position;")
        elif coordinate == 2:
            body = body.replace(copy, "            TRmgMapPosition next(position);")
        elif coordinate == 3:
            body = body.replace("    while (pending.size())", "    TRmgMapPosition next;\n    while (pending.size())")
            body = body.replace(copy, "            next = position;")
        elif coordinate == 4:
            body = body.replace(copy, "            TRmgMapPosition next;\n            next.m_x = position.m_x;\n            next.m_y = position.m_y;\n            next.m_z = position.m_z;")
        if append == 1:
            body = body.replace("pending.push_back(position);", "pending.insert(pending.end(), position);")
            body = body.replace("pending.push_back(next);", "pending.insert(pending.end(), next);")
        elif append == 2:
            body = body.replace("pending.push_back(next);", "pending.insert(pending.end(), 1, next);")
        elif append == 3:
            body = body.replace("pending.push_back(position);", "pending.insert(pending.end(), 1, position);")
        if offset:
            declaration = "            " + ("const TPoint&" if offset == 1 else "TPoint") + " offset = g_rmgDirections[direction];\n"
            body = body.replace("            next += g_rmgDirections[direction];", declaration + "            next += offset;")
        yield f"coordinate_{coordinate}+append_{append}+offset_{offset}", body


def make_axes(source):
    original = helpers().definition(source, FUNCTION)
    alternatives = list(forms())
    if original not in {body for _, body in alternatives}:
        if not any(original == body for _, seed in alternatives for _, body in refinements(seed)):
            raise ValueError("review current island-fill body")
        # Preserve an adopted component-lifetime winner as the control.
        # Historical parents still require their exact original snapshot.
        alternatives = alternatives[:59]
    axis = helpers().axis("island_fill", SOURCE, original, alternatives)
    if len(axis["options"]) != 60:
        raise ValueError("expected 60 distinct island-fill states")
    return [axis]


def component_form(body, order="xyz", defer_level=False):
    """Retain the canonical translation, change only real component lifetimes."""
    first = body.index("        for (int direction")
    translation = body.index("            next +=", first)
    prefix = body[first:translation]
    prefix = re.sub(r"            TRmgMapPosition next(?: = position|\(position\))?;\n", "", prefix)
    prefix = prefix.replace("            next = position;\n", "")
    prefix = re.sub(r"            next.m_[xyz] = position.m_[xyz];\n", "", prefix)
    split = prefix.index("\n") + 1
    declaration = "" if "    TRmgMapPosition next;\n    while" in body else "            TRmgMapPosition next;\n"
    fields = "".join("            next.m_" + field + " = position.m_" + field + ";\n" for field in order if not (defer_level and field == "z"))
    prefix = prefix[:split] + declaration + fields + prefix[split:]
    result = body[:first] + prefix + body[translation:]
    if defer_level:
        result = re.sub(r"(            next \+= [^\n]+;\n)", r"\1            next.m_z = position.m_z;\n", result)
    return result


def refinements(body):
    # Retail stores translated x, tests it, then stores y and the preserved z.
    # The first peak instead stores z/x/y. No operation between copying the
    # components and translating x/y reads or modifies the coordinate level.
    yield "defer_level", component_form(body, "xyz", True)
    yield "zxy", component_form(body, "zxy")
    yield "yxz", component_form(body, "yxz")
    yield "yx_defer_level", component_form(body, "yxz", True)
    yield "xzy", component_form(body, "xzy")
    yield "yzx", component_form(body, "yzx")
    yield "zyx", component_form(body, "zyx")
    for defer in (False, True):
        candidate = component_form(body, "xyz", defer)
        found = re.search(r"            (?:TPoint|const TPoint&) offset = g_rmgDirections\[direction\];\n", candidate)
        declaration = found[0] if found else "            TPoint offset = g_rmgDirections[direction];\n"
        if found:
            candidate = candidate.replace(declaration, "")
        candidate = candidate.replace("next += g_rmgDirections[direction];", "next += offset;")
        loop = "        for (int direction = 0; direction < 8; direction += 2) {\n"
        candidate = candidate.replace(loop, loop + declaration)
        yield "early_offset_defer_level" if defer else "early_offset", candidate


def load_parents(checkpoint_path, source):
    directory = checkpoint_path.parent
    payload = json.loads((directory / "input.json").read_text())
    if payload != dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=make_axes(source), evidence=__doc__):
        raise ValueError("review island-fill parent manifest")
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
        raise ValueError("expected 60 scored states and ten reproduced elites")
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


def make_parent_axes(source, parents):
    original = helpers().definition(source, FUNCTION)
    retained = [(label, helpers().definition(text, FUNCTION)) for label, text in parents]
    if len(retained) != 10 or len({text for _, text in retained}) != 10:
        raise ValueError("expected ten distinct reproduced parent sources")
    options = helpers().axis("island_fill_components", SOURCE, original, retained)["options"]
    seen = {row["replace"] for row in options}
    for entries in itertools.zip_longest(*(list(refinements(text)) for _, text in retained)):
        for (parent, _), entry in zip(retained, entries):
            if entry is None:
                continue
            label, body = entry
            if body not in seen:
                seen.add(body)
                options.append(dict(name=parent + "+" + label, replace=body))
            if len(options) == 60:
                return [dict(name="island_fill_components", source=SOURCE, find=original, options=options)]
    raise ValueError("expected sixty distinct frontier states")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=make_parent_axes(source, load_parents(args.parents_from, source)) if args.parents_from else make_axes(source), evidence=__doc__)
    if args.parents_from:
        payload["parent_checkpoint"] = str(args.parents_from.resolve())
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 island fill states ->", args.output)


if __name__ == "__main__":
    main()
