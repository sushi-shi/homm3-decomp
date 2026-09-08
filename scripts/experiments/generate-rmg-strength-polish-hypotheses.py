#!/usr/bin/env python3
"""Generate 60 point/query lifetime hypotheses for non-exact transition strength.

Retail 0x5b6fd0 keeps the west/north cache queries out of line and expands later
ones differently. Preserve west/north/east/south order, separate terrain/frame
queries, canonical point/cache helpers, dimension accessors and unsigned halving.
Vary actual point storage, binding the constant rule-table entry, and named query
results. No pasted helper, dummy work, false inline declaration or pragma pin.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source, hypotheses

FUNCTION = "?getTransitionStrength@rmgTerrainPainter@@QAEHABUTRmgGridPoint@@H@Z"
SIGNATURE = "int rmgTerrainPainter::getTransitionStrength(\n    const TRmgGridPoint& point, int terrain)"


def bodies():
    directions = (("point.m_x > 0", "x", "-", "point.m_x - 1", "point.m_y"),
                  ("point.m_y > 0", "y", "-", "point.m_x", "point.m_y - 1"),
                  ("point.m_x < getWidth() - 1", "x", "+", "point.m_x + 1", "point.m_y"),
                  ("point.m_y < getHeight() - 1", "y", "+", "point.m_x", "point.m_y + 1"))
    for storage, binding, query in itertools.product(
            ("direct", "assigned", "copy_offset", "shared_fields", "shared_copy"),
            ("pointer", "reference", "per_arm"),
            ("conjunction", "nested", "named_terrain", "named_frame")):
        lines = ["    unsigned int strength = m_transitionStrength;"]
        if binding == "pointer":
            lines.append("    TRmgTerrainRule* rule = g_rmgTerrainRules[terrain];")
        elif binding == "reference":
            lines.append("    TRmgTerrainRule& rule = *g_rmgTerrainRules[terrain];")
        if storage.startswith("shared"):
            lines.append("    TRmgGridPoint nearby;")
        invoke = "rule." if binding == "reference" else "rule->"
        for condition, axis, operation, x, y in directions:
            lines.append("    if (" + condition + ") {")
            if storage == "direct":
                lines.append(f"        TRmgGridPoint nearby({x}, {y});")
            elif storage in ("assigned", "shared_fields"):
                if storage == "assigned":
                    lines.append("        TRmgGridPoint nearby;")
                lines += [f"        nearby.m_x = {x};", f"        nearby.m_y = {y};"]
            else:
                lines.append("        " + ("TRmgGridPoint nearby(point);" if storage == "copy_offset" else "nearby = point;"))
                lines.append(f"        nearby.m_{axis} {operation}= 1;")
            if binding == "per_arm":
                lines.append("        TRmgTerrainRule* rule = g_rmgTerrainRules[terrain];")
            frame = "getPackedCell(nearby)->getFrame()"
            if query == "conjunction":
                lines += ["        if (getTerrain(nearby) == terrain",
                          f"            && {invoke}isSpecialFrame({frame}))", "            strength >>= 1;"]
            elif query == "named_terrain":
                lines += ["        int nearbyTerrain = getTerrain(nearby);",
                          "        if (nearbyTerrain == terrain",
                          f"            && {invoke}isSpecialFrame({frame}))", "            strength >>= 1;"]
            else:
                lines.append("        if (getTerrain(nearby) == terrain) {")
                if query == "named_frame":
                    lines.append("            int frame = " + frame + ";")
                    frame = "frame"
                lines += [f"            if ({invoke}isSpecialFrame({frame}))",
                          "                strength >>= 1;", "        }"]
            lines.append("    }")
        lines.append("    return strength;")
        yield "+".join((storage, binding, query)), SIGNATURE + "\n{\n" + "\n".join(lines) + "\n}"


def make_manifest(source):
    definitions = _source.find_definitions(source, FUNCTION)
    if len(definitions) != 1:
        raise ValueError("review the unique transition strength definition")
    definition = definitions[0]
    start = source.rfind("\n", 0, definition.head) + 1
    original = source[start:definition.body_close + 1]
    options = list(bodies())
    if original not in {body for _, body in options}:
        raise ValueError("review transition strength before rebasing the family")
    options.sort(key=lambda row: row[1] != original)
    return dict(schema=1, unit="rmg_terrain", function=FUNCTION, evidence=__doc__, axes=[dict(
        name="strength_point_queries", find=original,
        options=[dict(name=name, replace=body) for name, body in options])])


def helper_order_axis(source):
    def definition(name):
        found = _source.find_definitions(source, "rmgTerrainPainter::" + name)
        if len(found) != 1:
            raise ValueError("review the canonical helper " + name)
        item = found[0]
        return source.rfind("\n", 0, item.head) + 1, item.body_close + 1

    packed_start, packed_end = definition("getPackedCell")
    packed_start = source.rfind("VA(0x005B48D0,", 0, packed_start)
    terrain_start, terrain_end = definition("getTerrain")
    terrain_start = source.rfind("// Retail proves the shared accessor and its expanded uses,", 0, terrain_start)
    width_start, width_end = definition("getWidth")
    height_start, height_end = definition("getHeight")
    if min(packed_start, terrain_start) < 0 or source[width_end:height_start] != "\n\n":
        raise ValueError("review the helper evidence comments and paired dimension definitions")
    spans = dict(packed=(packed_start, packed_end), terrain=(terrain_start, terrain_end),
                 dimensions=(width_start, height_end))
    ordered = sorted(spans, key=lambda name: spans[name][0])
    for left, right in zip(ordered, ordered[1:]):
        if source[spans[left][1]:spans[right][0]] != "\n\n":
            raise ValueError("review statements between the canonical cache helpers")
    original = source[spans[ordered[0]][0]:spans[ordered[-1]][1]]
    groups = {name: source[start:end] for name, (start, end) in spans.items()}
    options = [dict(name="+".join(order), replace="\n\n".join(groups[name] for name in order))
               for order in itertools.permutations(groups)]
    options.sort(key=lambda row: row["replace"] != original)
    return dict(name="cache_helper_order", find=original, options=options)


def make_order_manifest(source, parent_labels):
    payload = make_manifest(source)
    shape = payload["axes"][0]
    choices = {row["name"]: row for row in shape["options"]}
    if len(parent_labels) != 10 or len(set(parent_labels)) != 10 or any(name not in choices for name in parent_labels):
        raise ValueError("review ten unique successful parent source hypotheses")
    selected = [choices[name] for name in parent_labels]
    if not any(row["replace"] == shape["find"] for row in selected):
        selected[-1] = shape["options"][0]
    selected.sort(key=lambda row: row["replace"] != shape["find"])
    shape["options"] = selected
    payload["axes"].append(helper_order_axis(source))
    payload["evidence"] += ("\nFollow-up: cross ten retained source parents with all six orders of the existing "
                            "packed-cell helper, terrain wrapper and paired dimension accessors. Move their "
                            "single canonical definitions and evidence annotations together, with no body changes. "
                            "This tests nested helper visibility; all parents are recompiled in the current context.")
    return payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--helper-order-from", type=Path, help="cross the top ten source parents from completed results.json with canonical helper order")
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
    if args.helper_order_from:
        parent = json.loads(args.helper_order_from.read_text())
        if (parent.get("schema") != 1 or parent.get("function") != FUNCTION
                or parent.get("source_sha256") != hashlib.sha256(source.encode()).hexdigest()):
            raise ValueError("review the parent batch against the current source")
        rows = [row for row in parent["results"] if not row["error"] and row["score"] is not None]
        payload = make_order_manifest(source, [row["labels"]["strength_point_queries"] for row in rows[:10]])
        payload["parent_results"] = str(args.helper_order_from.resolve())
    else:
        payload = make_manifest(source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    parsed = hypotheses.parse_manifest(args.output)
    print("generated", len(hypotheses.variants(parsed[4], parsed[5])), "unique source hypotheses ->", args.output)


if __name__ == "__main__":
    main()
