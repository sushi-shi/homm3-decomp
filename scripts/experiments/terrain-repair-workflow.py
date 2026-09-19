#!/usr/bin/env python3
"""Cross ten reproduced coordinate parents with six natural repair workflows.

The retail repair routine has the same 71-block CFG and 46 named calls as the
current source. Its four expanded gap predicates retain a coordinate across
terrain reads; accessor parents recover its fresh constructor-argument copy.
Test whether the surrounding developer-level choice/point lifetimes explain
the remaining register homes. Preserve ordinary predicate APIs, short-circuit
query order and the complete neighbour-ring phase. No helper body is pasted,
new helper declared, or inline directive added.
"""
import argparse
import importlib.util
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

spec = importlib.util.spec_from_file_location("accessors", Path(__file__).with_name("terrain-gap-accessors.py"))
accessors = importlib.util.module_from_spec(spec)
spec.loader.exec_module(accessors)


def prefix(source):
    start = source.index("void rmgTerrainPainter::repairTerrainPoint(")
    end = source.index("\n    if (!g_rmgTerrainRules", start)
    return source[start:end]


def phase(horizontal, form):
    outer, cross = ("Horizontal", "Vertical") if horizontal else ("Vertical", "Horizontal")
    low = "TRmgGridPoint(point.m_x - 1, point.m_y)" if horizontal else "TRmgGridPoint(point.m_x, point.m_y - 1)"
    high = "TRmgGridPoint(point.m_x + 1, point.m_y)" if horizontal else "TRmgGridPoint(point.m_x, point.m_y + 1)"
    lines = [f"    if (is{outer}Gap(point)) {{"]
    if form in ("point_pair", "const_point_pair"):
        const = "const " if form == "const_point_pair" else ""
        lines += [f"        {const}TRmgGridPoint lower = {low};", f"        {const}TRmgGridPoint upper = {high};"]
        low, high = "lower", "upper"
    forward = (f"!needsTerrainRepair({low}) &&\n"
               f"            (needsTerrainRepair({high}) ||\n"
               f"             (is{cross}Gap({low}, getPaintTerrain()) &&\n"
               f"              !is{cross}Gap({high}, getPaintTerrain())))")
    if form == "negative_guard":
        condition = (f"needsTerrainRepair({low}) ||\n"
                     f"            (!needsTerrainRepair({high}) &&\n"
                     f"             (!is{cross}Gap({low}, getPaintTerrain()) ||\n"
                     f"              is{cross}Gap({high}, getPaintTerrain())))")
        lines += ["        if (" + condition + ")", f"            paintPoint({low});", "        else", f"            paintPoint({high});"]
    elif form == "selected_point":
        lines += ["        paintPoint(" + forward + "\n", f"            ? {high} : {low});"]
    else:
        if form == "side_byte":
            lines += ["        unsigned char forward = " + forward + ";"]
            condition = "forward"
        else:
            condition = forward
        lines += ["        if (" + condition + ")", f"            paintPoint({high});", "        else", f"            paintPoint({low});"]
    return "\n".join(lines + ["    }"])


def workflows(original):
    yield "baseline", original
    head = original[:original.index("{\n") + 2]
    for form in ("negative_guard", "selected_point", "point_pair", "const_point_pair", "side_byte"):
        yield form, head + phase(False, form) + "\n" + phase(True, form) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("parents", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    checkpoint = json.loads(args.parents.read_text())
    root = args.parents.parent
    prior = json.loads((root / "input.json").read_text())
    source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
    parents = checkpoint["elites"]
    assert len(parents) == 10
    parents.sort(key=lambda row: row["choices"] != [0, 0])
    options = []
    evidence = []
    for row in parents:
        candidate = root / "candidates" / row["id"]
        results = [json.loads(path.read_text()) for path in candidate.glob("*/result.json")]
        assert len(results) >= 2 and all(r["scores"] == row["scores"] and r["object_hash"] == row["object_hash"] for r in results)
        tree = (candidate / "first/tree/src/rmg_terrain.cpp").read_text()
        edits = []
        for index, kind in enumerate(("Horizontal", "Vertical")):
            original = accessors.definition(source, kind)
            assert original == prior["axes"][index]["find"], "parent helper source changed"
            replacement = prior["axes"][index]["options"][row["choices"][index]]["replace"]
            assert accessors.definition(tree, kind) == replacement
            edits.append(dict(find=original, replace=replacement))
        options.append(dict(name=row["id"], replace=edits[0]["replace"], extra_edits=[edits[1]]))
        evidence.append(dict(id=row["id"], choices=row["choices"], object_hash=row["object_hash"]))
    payload = dict(schema=1, source="src/rmg_terrain.cpp", units=["rmg_terrain"],
        evidence=__doc__, parent_context=root.name, reproduced_parents=evidence,
        axes=[dict(name="coordinate_parent", find=accessors.definition(source, "Horizontal"), options=options),
              dict(name="workflow", find=prefix(source), options=[dict(name=name, replace=body) for name, body in workflows(prefix(source))])])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("60 parent/workflow combinations ->", args.output)


if __name__ == "__main__":
    main()
