#!/usr/bin/env python3
"""Test stable coordinate ownership inside gap predicates and their caller.

Retail carries the unaffected coordinate across the first cache query and
materializes a new reference argument for the second constructor. Five
reproduced accessor parents are crossed with original/scoped-fixed/scoped-pair
coordinate lifetimes and the repair caller's two coordinate accessors.
The ordinary APIs, short-circuit query ordering and painting workflow remain.
"""
import argparse
import importlib.util
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

spec = importlib.util.spec_from_file_location("workflow", Path(__file__).with_name("terrain-repair-workflow.py"))
workflow = importlib.util.module_from_spec(spec)
spec.loader.exec_module(workflow)
accessors = workflow.accessors


def ownership(body, kind, mode):
    if mode == "original":
        return body
    head = body[:body.index("{\n") + 2]
    condition, calls = body.split("    return ", 1)[1].rsplit("\n}", 1)[0].split("        &&", 1)
    condition = condition.strip()
    calls = calls.strip()
    fixed = "y" if kind == "Horizontal" else "x"
    coords = (fixed,) if mode == "fixed" else ("x", "y")
    setup = []
    for axis in coords:
        getter = "point.get" + axis.upper() + "()"
        member = "point.m_" + axis
        expression = getter if getter in calls else member
        setup.append(f"        const unsigned int {axis} = {expression};")
        calls = calls.replace(getter, axis).replace(member, axis)
    return head + "    if (" + condition + ") {\n" + "\n".join(setup) + "\n        return " + calls + "\n    }\n    return 0;\n}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("parents", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    root = args.parents.parent
    checkpoint = json.loads(args.parents.read_text())
    prior = json.loads((root / "input.json").read_text())
    source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
    picked = ([0, 0], [0, 1], [1, 0], [2, 3], [2, 2])
    options, evidence = [], []
    for choices in picked:
        row = next(r for r in checkpoint["elites"] if r["choices"] == choices)
        candidate = root / "candidates" / row["id"]
        results = [json.loads(p.read_text()) for p in candidate.glob("*/result.json")]
        assert len(results) >= 2 and all(r["scores"] == row["scores"] and r["object_hash"] == row["object_hash"] for r in results)
        tree = (candidate / "first/tree/src/rmg_terrain.cpp").read_text()
        parents = []
        for index, kind in enumerate(("Horizontal", "Vertical")):
            original = accessors.definition(source, kind)
            assert original == prior["axes"][index]["find"]
            body = prior["axes"][index]["options"][choices[index]]["replace"]
            assert body == accessors.definition(tree, kind)
            parents.append(body)
        for mode in ("original", "fixed", "pair"):
            bodies = [ownership(body, kind, mode) for body, kind in zip(parents, ("Horizontal", "Vertical"))]
            options.append(dict(name=row["id"] + "+" + mode, replace=bodies[0],
                extra_edits=[dict(find=accessors.definition(source, "Vertical"), replace=bodies[1])]))
        evidence.append(dict(id=row["id"], choices=choices, object_hash=row["object_hash"]))
    original = workflow.prefix(source)
    callers = []
    for axes in ("", "x", "y", "xy"):
        body = original
        for axis in axes:
            body = body.replace("point.m_" + axis, "point.get" + axis.upper() + "()")
        callers.append(dict(name=axes or "members", replace=body))
    payload = dict(schema=1, source="src/rmg_terrain.cpp", units=["rmg_terrain"], evidence=__doc__,
        parent_context=root.name, reproduced_parents=evidence,
        axes=[dict(name="gap_coordinate_ownership", find=accessors.definition(source, "Horizontal"), options=options),
              dict(name="caller_coordinates", find=original, options=callers)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("60 coordinate ownership/caller combinations ->", args.output)


if __name__ == "__main__":
    main()
