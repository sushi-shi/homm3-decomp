#!/usr/bin/env python3
"""Generate 60 retail-backed treasure-outline scan states.

Complete-only TRmgTreasureGroup::traceOutline (0x535ee0) has no Dreamcast
counterpart. Retail keeps a row-major search followed by the canonical point
perimeter walk. Its inner back-edge is a forward jge plus a backward jmp;
the current nested for emits a backward jl. Explore five loop structures,
six actual point initialization lifetimes and two dimension-read lifetimes.
Preserve the cached-outline guard, byte predicate helpers, level-zero map
queries, x==width termination (including zero-height behavior), and the walk.
No helper flattening, new qualifiers, artificial operations or layout edits.
"""
import argparse
import hashlib
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families

SOURCE = "src/rmg.cpp"
FUNCTION = "TRmgTreasureGroup::traceOutline"
WALK_START = "    --position.m_y;\n    TPoint start = position;\n    int direction = 2;\n"
PREDICATE = ("item->isRoadEntrance() || !item->m_tileData.m_roadPassable\n"
             "                || item->m_tile.m_landType == eTerrainRock || !item->hasSubterraneanGate()")


def helpers():
    spec = importlib.util.spec_from_file_location(
        "rmg_treasure_outline_helpers", Path(__file__).with_name("generate-rmg-position-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def scan_body(loop, initialization, dimensions):
    setups = (
        ("    TPoint position;\n    position.m_x = 0;\n", "position.m_y = 0"),
        ("    TPoint position;\n    position.m_x = 0;\n    position.m_y = 0;\n", ""),
        ("    TPoint position;\n    position.m_y = 0;\n    position.m_x = 0;\n", ""),
        ("    TPoint position(0, 0);\n", ""),
        ("    TPoint position = TPoint(0, 0);\n", ""),
        ("    TPoint position;\n    position = TPoint(0, 0);\n", ""),
    )
    result, y_initialize = setups[initialization]
    height, width = ("height", "width") if dimensions == 0 else ("m_map.m_mapHeight", "m_map.m_mapWidth")
    if dimensions == 0:
        result += "    int height = m_map.m_mapHeight;\n"
    row_width = "        int width = m_map.m_mapWidth;\n" if dimensions == 0 else ""
    lookup = "            TRmgMapItem* item = m_map.getMapItem(position.m_x, position.m_y, 0);\n"
    test = "            if (" + PREDICATE + ")\n"
    if loop == 1:
        if y_initialize:
            result += "    for (" + y_initialize + "; position.m_y < " + height + "; ) {\n"
        else:
            result += "    while (position.m_y < " + height + ") {\n"
        result += row_width
        result += "        position.m_x = 0;\n        while (position.m_x < " + width + ") {\n"
        result += lookup + test + "                break;\n            ++position.m_x;\n        }\n"
        result += "        if (position.m_x < " + width + ")\n            break;\n        ++position.m_y;\n    }\n"
        return result
    result += "    for (" + y_initialize + "; position.m_y < " + height + "; ++position.m_y) {\n" + row_width
    if loop == 0:
        result += "        for (position.m_x = 0; position.m_x < " + width + "; ++position.m_x) {\n"
        result += lookup + test + "                break;\n        }\n"
    elif loop == 2:
        result += "        position.m_x = 0;\n        if (position.m_x < " + width + ") {\n"
        result += "        scanColumn:\n" + lookup + test + "                goto scanRowDone;\n"
        result += "            ++position.m_x;\n            if (position.m_x >= " + width + ")\n                goto scanRowDone;\n"
        result += "            goto scanColumn;\n        }\n    scanRowDone:\n"
    elif loop == 3:
        result += "        position.m_x = 0;\n        if (position.m_x < " + width + ") {\n            do {\n"
        result += "    " + lookup + "    " + test.replace("\n                ||", "\n                    ||")
        result += "                    break;\n            } while (++position.m_x < " + width + ");\n        }\n"
    elif loop == 4:
        result += "        for (position.m_x = 0; ; ++position.m_x) {\n"
        result += "            if (position.m_x >= " + width + ")\n                break;\n"
        result += lookup + test + "                break;\n        }\n"
    else:
        raise ValueError("unknown scan structure")
    return result + "        if (position.m_x < " + width + ")\n            break;\n    }\n"


def scan_forms():
    loops = ("nested_for", "manual_increment", "inner_goto", "guarded_do", "top_break")
    initializers = ("for_y", "fields_xy", "fields_yx", "constructed", "copy_initialized", "assigned")
    for loop, initializer, dimensions in itertools.product(range(5), range(6), range(2)):
        yield (loops[loop] + "+" + initializers[initializer] + "+" + ("cached" if dimensions == 0 else "live"),
               scan_body(loop, initializer, dimensions))


def make_axes(source):
    helper = helpers()
    original = helper.definition(source, FUNCTION)
    start = original.index("    TPoint position")
    end = original.index("    if (position.m_x == m_map.m_mapWidth)\n")
    scan = original[start:end]
    forms = list(scan_forms())
    if scan not in dict(forms).values():
        raise ValueError("review the current treasure-outline scan before rebasing")
    return [helper.axis("treasure_outline_scan", SOURCE, original,
                        [(label, original[:start] + text + original[end:]) for label, text in forms])]


def walk_starts():
    yield "copy_initialized", WALK_START
    yield "direct_copy", "    --position.m_y;\n    TPoint start(position);\n    int direction = 2;\n"
    yield "assigned", "    --position.m_y;\n    TPoint start;\n    start = position;\n    int direction = 2;\n"
    yield "direction_first", "    --position.m_y;\n    int direction = 2;\n    TPoint start = position;\n"
    yield "predeclared_start", "    TPoint start;\n    --position.m_y;\n    start = position;\n    int direction = 2;\n"
    yield "coordinate_copy", "    --position.m_y;\n    TPoint start(position.m_x, position.m_y);\n    int direction = 2;\n"


def load_parents(checkpoint_path, source):
    directory = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    payload = json.loads((directory / "input.json").read_text())
    if payload != dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                       axes=make_axes(source), evidence=__doc__):
        raise ValueError("review the completed treasure-outline scan family")
    for relative in (SOURCE, "src/rmg_support.cpp", "src/rmg_terrain.cpp"):
        if (directory / "snapshot" / relative).read_bytes() != (HOMM3_DIR / relative).read_bytes():
            raise ValueError("review parents against the current source: " + relative)
    snapshot_headers = directory / "snapshot/include"
    if {path.relative_to(snapshot_headers) for path in snapshot_headers.rglob("*") if path.is_file()} != {
            path.relative_to(HOMM3_DIR / "include") for path in (HOMM3_DIR / "include").rglob("*") if path.is_file()}:
        raise ValueError("review parents against the current header population")
    for path in snapshot_headers.rglob("*"):
        if path.is_file() and path.read_bytes() != (HOMM3_DIR / "include" / path.relative_to(snapshot_headers)).read_bytes():
            raise ValueError("review parents against changed header: " + str(path.relative_to(snapshot_headers)))
    records, elites = checkpoint["records"], checkpoint["elites"]
    if len(records) != 60 or any(not row.get("scores") for row in records) or len(elites) != 10:
        raise ValueError("expected all 60 scored states and ten reproduced elites")
    baseline = next(row for row in records if not any(row["choices"]))
    equivalents = [row for row in elites if row["object_hash"] == baseline["object_hash"]]
    if len(equivalents) != 1:
        raise ValueError("review the baseline-equivalent elite")
    parents = [("baseline", source)]
    for row in elites:
        candidate = directory / "candidates" / row["id"]
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        for key in ("object_hash", "scores", "source_hashes", "choices"):
            if repeated.get(key) != row[key]:
                raise ValueError("parent did not reproduce: " + row["id"])
        text = (candidate / "first/tree" / SOURCE).read_text()
        if hashlib.sha256(text.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("parent source hash changed: " + row["id"])
        if row is not equivalents[0]:
            parents.append((row["id"], text))
    return parents


def make_parent_axes(source, parents):
    helper = helpers()
    original = helper.definition(source, FUNCTION)

    def scan(text):
        method = helper.definition(text, FUNCTION)
        return method[method.index("    TPoint position"):method.index("    if (position.m_x == m_map.m_mapWidth)\n")]

    if original.count(WALK_START) != 1 or len(parents) != 10:
        raise ValueError("review ten scan parents and the perimeter start")
    return [helper.axis("scan_parent", SOURCE, scan(source), [(label, scan(text)) for label, text in parents]),
            helper.axis("perimeter_start_lifetime", SOURCE, WALK_START, list(walk_starts()))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--parents-from", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / SOURCE).read_text()
    axes = (make_parent_axes(source, load_parents(args.parents_from, source)) if args.parents_from
            else make_axes(source))
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                   axes=axes, evidence=__doc__)
    if args.parents_from:
        payload["parent_checkpoint"] = str(args.parents_from.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated 60 treasure-outline states ->", args.output)


if __name__ == "__main__":
    main()
