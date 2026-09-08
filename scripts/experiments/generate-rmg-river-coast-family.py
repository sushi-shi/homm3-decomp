#!/usr/bin/env python3
"""60 retail-grounded coastal target lifetime states (0x548a40).

Retail keeps direction in EBX and recycles its argument home for all three
loop counters. Its two canonical coordinate constructor calls already agree.
Cross five counter lifetimes, four coordinate initialization forms and three
step lifetimes without changing helper definitions or the asymmetric bounds.
No Dreamcast counterpart was found; these are retail-only hypotheses.
"""
import argparse
import hashlib
import itertools
import json
import re
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::markRiverCoastTarget"


def baseline():
    bounds = """        if (point.m_x < 0 || point.m_x > m_map.m_mapWidth
            || point.m_y < 0 || point.m_y >= m_map.m_mapHeight)
            return;
"""
    return """void type_random_map_generator::markRiverCoastTarget(TRmgMapPosition position, int direction)
{
    TRmgMapPosition point = position + g_rmgDirections[(direction + 2) & 7];
    TPoint step = g_rmgDirections[(direction - 2) & 7];
    for (int count = 0; count < 3; ++count) {
""" + bounds + """        if (m_map.getMapItem(point.m_x, point.m_y, point.m_z)->m_tile.m_landType != eTerrainWater)
            return;
        point += step;
    }
    point = position + g_rmgDirections[(direction + 1) & 7];
    for (count = 0; count < 3; ++count) {
""" + bounds + """        TRmgMapItem* item = m_map.getMapItem(point.m_x, point.m_y, point.m_z);
        if (item->m_tile.m_landType == eTerrainWater || item->isRoadEntrance())
            return;
        point += step;
    }
    point = position;
    point += g_rmgDirections[direction];
    TRmgMapItem* item;
    for (count = 0; count < 4; ++count) {
""" + bounds + """        item = m_map.getMapItem(point.m_x, point.m_y, point.m_z);
        if (item->m_tile.m_landType == eTerrainWater || item->isRoadEntrance())
            return;
        point += g_rmgDirections[direction];
    }
    item->m_tileData.m_blockedDirections |= 1 << (((direction - 4) >> 1) & 3);
    item->m_tileData.m_riverTarget = 1;
}"""


def forms():
    for counter, coordinate, step in itertools.product(range(5), range(4), range(3)):
        body = baseline()
        init = "    TRmgMapPosition point = position + g_rmgDirections[(direction + 2) & 7];"
        if coordinate == 1:
            body = body.replace(init, "    TRmgMapPosition point(position + g_rmgDirections[(direction + 2) & 7]);")
        elif coordinate == 2:
            body = body.replace(init, "    TRmgMapPosition point;\n    point = position + g_rmgDirections[(direction + 2) & 7];")
        elif coordinate == 3:
            body = body.replace("    point = position;\n    point += g_rmgDirections[direction];", "    TRmgMapPosition inland = position;\n    inland += g_rmgDirections[direction];")
            start = body.index("    TRmgMapPosition inland")
            body = body[:start] + body[start:].replace("point.", "inland.").replace("point +=", "inland +=")
        if step == 1:
            body = body.replace("    TPoint step = g_rmgDirections[(direction - 2) & 7];", "    TPoint step;\n    step = g_rmgDirections[(direction - 2) & 7];")
        elif step == 2:
            declaration = "    TPoint step = g_rmgDirections[(direction - 2) & 7];\n"
            body = body.replace(declaration, "")
            body = body.replace("{\n", "{\n" + declaration, 1)
        if counter == 1:
            body = body.replace("for (int count = 0;", "for (count = 0;")
            body = body.replace("{\n", "{\n    int count;\n", 1)
        elif counter == 2:
            body = body.replace("for (int count = 0;", "for (count = 0;")
            body = body.replace("    for (count = 0;", "    int count;\n    for (count = 0;", 1)
        elif counter == 3:
            body = body.replace("for (count = 0;", "for (int count = 0;")
            # Explicit lexical scopes remain distinct under VC6's legacy
            # for-scope; the final item must survive its counter's scope.
            body = body.replace("    for (int count", "    {\n    for (int count")
            body = body.replace("        point += step;\n    }", "        point += step;\n    }\n    }")
            body = body.replace("    item->m_tileData.m_blockedDirections", "    }\n    item->m_tileData.m_blockedDirections")
        elif counter == 4:
            # Three independent names, but no extra brace lifetime boundary.
            chunks = body.split("    for (")
            for i in range(1, 4):
                chunks[i] = chunks[i].replace("count", ("waterCount", "dryCount", "inlandCount")[i - 1])
                if not chunks[i].startswith("int "):
                    chunks[i] = "int " + chunks[i]
            body = "    for (".join(chunks)
        yield f"counter_{counter}+coordinate_{coordinate}+step_{step}", body


def helpers():
    return generator("generate-rmg-position-family.py")


def make_axes(source):
    original = helpers().definition(source, FUNCTION)
    alternatives = list(forms())
    if original not in {body for _, body in alternatives}:
        raise ValueError("review the current coastal target body")
    result = helpers().axis("river_coast", SOURCE, original, alternatives)
    if len(result["options"]) != 60:
        raise ValueError("expected 60 distinct coastal states")
    return [result]


def refinements(body):
    """Address formation and live value ownership visible in retail B0/B5."""
    water = "        if (m_map.getMapItem(point.m_x, point.m_y, point.m_z)->m_tile.m_landType != eTerrainWater)"
    bound = "        TRmgMapItem* waterItem = m_map.getMapItem(point.m_x, point.m_y, point.m_z);\n        if (waterItem->m_tile.m_landType != eTerrainWater)"
    yield "water_item", body.replace(water, bound)
    yield "constant_step", body.replace("    TPoint step =", "    const TPoint step =") if "    TPoint step =" in body else body.replace("    TPoint step;\n    step =", "    const TPoint step =")
    renamed = re.sub(r"\bdirection\b", "coastDirection", body)
    renamed = renamed.replace("int coastDirection)", "int direction)")
    yield "direction_value", renamed.replace("{\n", "{\n    const int coastDirection = direction;\n", 1)
    shared = body.replace("        TRmgMapItem* item =", "        item =").replace("    TRmgMapItem* item;\n", "")
    yield "shared_item", shared.replace("{\n", "{\n    TRmgMapItem* item;\n", 1)
    mapped = body.replace("m_map.", "map.")
    yield "map_reference", mapped.replace("{\n", "{\n    type_random_map& map = m_map;\n", 1)
    yield "water_and_direction", renamed.replace(water, bound).replace("{\n", "{\n    const int coastDirection = direction;\n", 1)


def load_parents(checkpoint_path, source):
    directory = checkpoint_path.parent
    payload = json.loads((directory / "input.json").read_text())
    expected = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=make_axes(source), evidence=__doc__)
    if payload != expected:
        raise ValueError("review coastal parent manifest")
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
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        for key in ("object_hash", "scores", "source_hashes", "choices"):
            if repeated.get(key) != row[key]:
                raise ValueError("parent failed reproduction")
        text = (candidate / "first/tree" / SOURCE).read_text()
        if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("parent source changed")
        parents.append((row["id"], text))
    return parents


def make_parent_axes(source, parents):
    original = helpers().definition(source, FUNCTION)
    retained = [(label, helpers().definition(text, FUNCTION)) for label, text in parents]
    if len(retained) != 10 or len({text for _, text in retained}) != 10:
        raise ValueError("expected ten distinct reproduced parents")
    options = helpers().axis("river_coast_frontier", SOURCE, original, retained)["options"]
    seen = {row["replace"] for row in options}
    mutations = [list(refinements(text)) for _, text in retained]
    for entries in itertools.zip_longest(*mutations):
        for (parent, _), entry in zip(retained, entries):
            if entry is None:
                continue
            label, body = entry
            if body not in seen:
                seen.add(body)
                options.append(dict(name=parent + "+" + label, replace=body))
            if len(options) == 60:
                return [dict(name="river_coast_frontier", source=SOURCE, find=original, options=options)]
    raise ValueError("expected sixty distinct frontier states")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
        axes=make_parent_axes(source, load_parents(args.parents_from, source)) if args.parents_from else make_axes(source), evidence=__doc__)
    if args.parents_from:
        payload["parent_checkpoint"] = str(args.parents_from.resolve())
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 coastal states ->", args.output)


if __name__ == "__main__":
    main()
