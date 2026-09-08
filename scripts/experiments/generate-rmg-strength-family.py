#!/usr/bin/env python3
"""Generate a retail-evidenced RMG family on top of the previous top N.

This emits ordinary C++ alternatives, not pragma pins or TU noise. Preserve
the four cardinal visits, short-circuit query order, unsigned halving, and
canonical point/cache helpers. A frame accessor is a source hypothesis for
the repeated frame extraction; its flattened form is the negative control.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.match import status
from homm3.vc6 import _source
from homm3.vc6.source_families import code_identity, digest, load_manifest, select_elites


def strength_body(construction, dimensions, conditions, rule_kind, frame):
    lines = ["int rmgTerrainPainter::getTransitionStrength(",
             "    const TRmgGridPoint& point, int terrain)", "{",
             "    unsigned int strength = m_transitionStrength;"]
    if rule_kind == "pointer":
        lines.append("    TRmgTerrainRule* rule = g_rmgTerrainRules[terrain];")
        invoke = "rule->"
    else:
        lines.append("    TRmgTerrainRule& rule = *g_rmgTerrainRules[terrain];")
        invoke = "rule."
    width = "m_width" if dimensions == "members" else "getWidth()"
    height = "m_height" if dimensions == "members" else "getHeight()"
    directions = [
        ("point.m_x > 0", "point.m_x - 1", "point.m_y"),
        ("point.m_y > 0", "point.m_x", "point.m_y - 1"),
        (f"point.m_x < {width} - 1", "point.m_x + 1", "point.m_y"),
        (f"point.m_y < {height} - 1", "point.m_x", "point.m_y + 1"),
    ]
    frame_query = "getPackedCell(nearby)->getFrame()" if frame == "direct" else "getFrame(nearby)"
    for condition, x, y in directions:
        lines.append(f"    if ({condition}) {{")
        if construction == "direct":
            lines.append(f"        TRmgGridPoint nearby({x}, {y});")
        elif construction == "copy_init":
            lines.append(f"        TRmgGridPoint nearby = TRmgGridPoint({x}, {y});")
        else:
            lines.append(f"        const TRmgGridPoint& nearby = TRmgGridPoint({x}, {y});")
        if conditions == "conjunction":
            lines.extend(["        if (getTerrain(nearby) == terrain",
                          f"            && {invoke}isSpecialFrame({frame_query}))",
                          "            strength >>= 1;"])
        else:
            lines.extend(["        if (getTerrain(nearby) == terrain) {",
                          f"            if ({invoke}isSpecialFrame({frame_query}))",
                          "                strength >>= 1;", "        }"])
        lines.append("    }")
    return "\n".join(lines + ["    return strength;", "}"])


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("checkpoint", type=Path)
    ap.add_argument("output", type=Path)
    ap.add_argument("--keep", type=int, default=8)
    args = ap.parse_args()
    original_family = Path(__file__).with_name("rmg-grid-source-family.json")
    payload, originals, axes = load_manifest(original_family, HOMM3_DIR)
    previous = json.loads(args.checkpoint.read_text())
    # Re-index old checkpoints too: v1 included VC6's anonymous-scope nonce
    # in whole-object identities. Only emitted code/semantic relocations
    # distinguish parents. Never reuse the old score as a new observation.
    for row in previous["records"]:
        if not row.get("scores"):
            continue
        trial = args.checkpoint.parent / "candidates" / row["id"] / "first"
        row["object_hash"] = digest(json.dumps([
            code_identity((trial / unit / "candidate.c.obj").read_bytes())
            for unit in payload["units"]]).encode())
    rows = status.load_baseline()
    parents = select_elites(previous["records"], args.keep,
                            {"|".join(key): row.cur or 0 for key, row in rows.items()})
    choices = [(0,) * len(axes)]
    for row in parents:
        choice = tuple(next(index for index, option in enumerate(axis.options)
                            if option.name == row["labels"][axis.name]) for axis in axes)
        if choice not in choices:
            choices.append(choice)
    options = []
    for choice in choices:
        name = "+".join(axis.options[value].name for axis, value in zip(axes, choice))
        edits = []
        for axis, value in zip(axes, choice):
            for edit in axis.options[value].edits:
                edits.append({"source": edit.source,
                              "find": originals[edit.source][edit.start:edit.end],
                              "replace": edit.replacement})
        options.append(dict(name=name, replace=edits[0]["replace"], extra_edits=edits[1:]))
    grid_axis = dict(name="grid_parent", source=payload["source"],
                     find=payload["axes"][0]["find"], options=options)
    source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
    definitions = _source.find_definitions(source, "rmgTerrainPainter::getTransitionStrength")
    if len(definitions) != 1:
        raise ValueError("expected one reconstructed getTransitionStrength definition")
    definition = definitions[0]
    # Definition.head starts at the qualified name, not its return type.
    # Include the complete declaration line or a replacement creates int int.
    start = source.rfind("\n", 0, definition.head) + 1
    original = source[start:definition.body_close + 1]
    if not original.startswith("int rmgTerrainPainter::getTransitionStrength("):
        raise ValueError("unexpected transition-strength declaration")
    options = [dict(name="baseline", replace=original)]
    shape_axes = [("direct", "copy_init", "const_reference"),
                  ("members", "accessors"), ("conjunction", "nested"),
                  ("pointer", "reference"), ("direct", "helper")]
    for choices in itertools.product(*shape_axes):
        name = "+".join(choices)
        replacement = strength_body(*choices)
        option = dict(name=name, replace=replacement)
        if choices[-1] == "helper":
            option["extra_edits"] = [
                dict(source="include/rmg_terrain.h", find="    int getTerrain(const TRmgGridPoint& point);",
                     replace="    int getTerrain(const TRmgGridPoint& point);\n    int getFrame(const TRmgGridPoint& point);"),
                dict(source="src/rmg_terrain.cpp", insert_before="unsigned int rmgTerrainPainter::getWidth() const",
                     text="// Provisional frame accessor: repeated extraction at retail 0x5b6fd0.\n"
                          "// Its direct getPackedCell()->getFrame() form is the family control.\n"
                          "int rmgTerrainPainter::getFrame(const TRmgGridPoint& point)\n"
                          "{\n    return getPackedCell(point)->getFrame();\n}\n\n"),
            ]
        options.append(option)
    output = dict(schema=1, units=payload["units"], source="src/rmg_terrain.cpp",
                  evidence="Retail 0x5b6fd0: west/north/east/south; separate terrain/frame queries; logical halving. Semantics, argument ABI and helper signatures stay fixed. Source-family alternatives: point temporary lifetime, dimension accessors, condition spelling, rule reference/pointer and inferred frame accessor. Eight aggregate/specialist grid parents come from the completed prior sweep; old scores are not reused.",
                  parent_checkpoint=str(args.checkpoint.resolve()),
                  axes=[grid_axis, dict(name="strength_shape", find=original, options=options)])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(f"generated {len(grid_axis['options'])} grid parents x {len(options)} strength forms -> {args.output}")


if __name__ == "__main__":
    main()
