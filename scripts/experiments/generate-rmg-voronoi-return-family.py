#!/usr/bin/env python3
"""Canonical Voronoi return construction, retaining reproduced caller parents.

Retail 0x5fdb40 copies returned aggregates from seven retained calls. The five
ordinary operator bodies at 0x5fdcb0..0x5fdd40 return eight-byte aggregates.
Test named return construction and member assignment without changing their
interfaces, visibility, arithmetic, or the caller's canonical operation chain.
"""
import argparse
import hashlib
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6.test_rmg_families import generator
from homm3.vc6 import source_families


def operator_axes(source):
    helper = generator("generate-rmg-position-family.py")
    specs = [
        ("TRmgVector::operator+", None, "TRmgVector", "m_x + other.m_x", "m_y + other.m_y"),
        ("TRmgVector::operator*", None, "TRmgVector", "m_x * scale", "m_y * scale"),
        ("TRmgVector::operator/", None, "TRmgVector", "m_x / divisor", "m_y / divisor"),
        ("operator+", "TPoint point, TRmgVector offset", "TPoint", "point.m_x + offset.m_x", "point.m_y + offset.m_y"),
        ("operator-", "TPoint left, TPoint right", "TRmgVector", "left.m_x - right.m_x", "left.m_y - right.m_y"),
    ]
    axes = []
    for index, (name, params, kind, x, y) in enumerate(specs):
        original = helper.definition(source, name, **({"parameters": params} if params else {}))
        returned = f"    return {kind}({x}, {y});"
        forms = [
            ("original", returned),
            ("named_construct", f"    {kind} result({x}, {y});\n    return result;"),
            ("named_assign", f"    {kind} result;\n    result.m_x = {x};\n    result.m_y = {y};\n    return result;"),
        ]
        opening = original.index("\n{") + 2
        body = original[opening:original.rindex("}")]
        # Evidence comments may accompany an adopted form; source anchors still
        # include the exact authored definition, not a stale parent body.
        statements = "\n".join(line for line in body.strip("\n").splitlines()
                               if not line.lstrip().startswith("//"))
        if statements not in [value for _, value in forms]:
            raise ValueError("review changed operator " + name)
        axes.append(helper.axis("return_" + str(index), "src/rmg.cpp", original,
                                ((label, original[:opening] + "\n" + value + "\n}")
                                 for label, value in forms if value != statements)))
    return axes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--control", action="store_true",
                        help="isolate the reproduced caller and scale-result changes")
    args = parser.parse_args()
    context = args.checkpoint.parent
    previous = json.loads(args.checkpoint.read_text())
    if previous.get("generation", 0) < 1 or not previous.get("elites"):
        raise ValueError("parent checkpoint is unfinished")
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    snapshot = (context / "snapshot/src/rmg.cpp").read_text()
    if source != snapshot:
        raise ValueError("caller parent snapshot is stale")
    vertex = generator("generate-rmg-voronoi-vertices-family.py")
    caller = vertex.make_axes(source)[0]
    original_options = caller["options"]
    chosen = [original_options[0]]
    # Keep every reproduced elite; the runner observes all scores afresh.
    summary = json.loads((context / "generation-0001.json").read_text())
    for elite in summary["elites"]:
        tree = context / "candidates" / elite["id"] / "repeat/tree/src/rmg.cpp"
        if not tree.is_file():
            raise ValueError("parent has no reproduction: " + elite["id"])
        body = vertex.helpers().definition(tree.read_text(), vertex.FUNCTION)
        option = original_options[elite["choices"][0]]
        if body != option["replace"]:
            raise ValueError("parent manifest/candidate identity mismatch")
        if option not in chosen:
            chosen.append(option)
    caller["options"] = chosen
    axes = [caller] + operator_axes(source)
    if args.control:
        parent = next(option for option in chosen
                      if option["name"] == "expression_3+capture_3+order_1")
        caller["options"] = [chosen[0], parent]
        scale = axes[2]
        scale["options"] = [scale["options"][0], scale["options"][2]]
        axes = [caller, scale]
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
                   evidence=__doc__, parent_checkpoint=str(args.checkpoint.resolve()),
                   parent_source_sha256=hashlib.sha256(source.encode()).hexdigest(),
                   axes=axes)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("axis widths", [len(axis["options"]) for axis in axes])


if __name__ == "__main__":
    main()
