#!/usr/bin/env python3
"""Independent rectangle lifetimes for expandObstacleClearance (0x53f880).

Retail carries outer x+2 at entry, against the candidate's x-1 induction.
The already-exact neighboring repairWaterZoneBorders uses scoped TPoint lower
corners, an original row capture and dimension values at the upper clamps.
Single uniform spelling controls were previously neutral here. This family
instead crosses all three phases independently, testing whether their coupled
lifetimes select that induction and the last scan's bound reload. It preserves
one bounds aggregate, the real point/map helpers, all scans and both distinct
connection-policy checks. Complete's RMG TU has no Dreamcast counterpart.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

NAME = "type_random_map_generator::expandObstacleClearance"


def baseline_definition(source):
    current = generator("generate-rmg-position-family.py").definition(source, NAME)
    upper = upper_block(1, 2, 1)
    if upper not in current:
        return current
    start = current.rindex(upper)
    baseline = current[:start] + clamp_block(1, 2, 0) + current[start + len(upper):]
    if current != list(upper_variants(baseline))[1][1]:
        raise ValueError("review changed clearance implementation before rebasing")
    return baseline


def clamp_block(lower_offset, upper_offset, form):
    x = "position.m_x" + (f" - {lower_offset}" if lower_offset else "")
    y = "position.m_y" + (f" - {lower_offset}" if lower_offset else "")
    upper_y = f"position.m_y + {upper_offset}"
    upper_x = f"position.m_x + {upper_offset}"
    lines = []
    if form:
        if form == 3:
            lines.append("int row = position.m_y;")
            y = "row" + (f" - {lower_offset}" if lower_offset else "")
            upper_y = f"row + {upper_offset}"
        if form == 2:
            lines.extend(["TPoint lower;", f"lower.m_x = {x};", f"lower.m_y = {y};"])
        else:
            lines.append(f"TPoint lower({x}, {y});")
        x, y = "lower.m_x", "lower.m_y"
    lines.extend([f"bounds.m_minimumY = max({y}, 0);", f"bounds.m_minimumX = max({x}, 0);"])
    if form:
        lines.extend(["int height = m_map.m_mapHeight;", f"bounds.m_maximumY = min({upper_y}, height);",
                      "int width = m_map.m_mapWidth;", f"bounds.m_maximumX = min({upper_x}, width);"])
    else:
        lines.extend([f"bounds.m_maximumY = min({upper_y}, m_map.m_mapHeight);",
                      f"bounds.m_maximumX = min({upper_x}, m_map.m_mapWidth);"])
    return "                {\n" + "\n".join("                    " + line for line in lines) + "\n                }"


def variants(original):
    spans = []
    cursor = 0
    for offsets in ((1, 2), (0, 1), (1, 2)):
        anchor = clamp_block(*offsets, 0)
        start = original.index(anchor, cursor)
        cursor = start + len(anchor)
        spans.append((start, cursor, offsets))
    for choices in itertools.product(range(4), repeat=3):
        body = original
        for (start, end, offsets), choice in reversed(list(zip(spans, choices))):
            body = body[:start] + clamp_block(*offsets, choice) + body[end:]
        yield "rectangles_" + "_".join(map(str, choices)), body


def upper_block(lower_offset, upper_offset, form):
    if not form:
        return clamp_block(lower_offset, upper_offset, 0)
    x = "position.m_x" + (f" - {lower_offset}" if lower_offset else "")
    y = "position.m_y" + (f" - {lower_offset}" if lower_offset else "")
    ux, uy = f"position.m_x + {upper_offset}", f"position.m_y + {upper_offset}"
    if form == 3:
        lines = [f"TRmgZoneBounds rectangle = {{{x}, {y}, {ux}, {uy}}};"]
        x, y, ux, uy = ("rectangle.m_minimumX", "rectangle.m_minimumY",
                        "rectangle.m_maximumX", "rectangle.m_maximumY")
    else:
        lines = [f"TPoint upper({ux}, {uy});"]
        ux, uy = "upper.m_x", "upper.m_y"
        if form == 2:
            lines.append(f"TPoint lower({x}, {y});")
            x, y = "lower.m_x", "lower.m_y"
    lines.extend([f"bounds.m_minimumY = max({y}, 0);",
                  f"bounds.m_minimumX = max({x}, 0);",
                  f"bounds.m_maximumY = min({uy}, m_map.m_mapHeight);",
                  f"bounds.m_maximumX = min({ux}, m_map.m_mapWidth);"])
    return "                {\n" + "\n".join("                    " + line for line in lines) + "\n                }"


def upper_variants(original):
    """Real upper corners can carry the retail x+2 induction without a fake counter."""
    spans, cursor = [], 0
    for offsets in ((1, 2), (0, 1), (1, 2)):
        anchor = clamp_block(*offsets, 0)
        start = original.index(anchor, cursor)
        cursor = start + len(anchor)
        spans.append((start, cursor, offsets))
    for choices in itertools.product(range(4), repeat=3):
        body = original
        for (start, end, offsets), choice in reversed(list(zip(spans, choices))):
            body = body[:start] + upper_block(*offsets, choice) + body[end:]
        yield "upper_corners_" + "_".join(map(str, choices)), body


def refine(parent, form):
    spans, cursor = [], 0
    for offsets in ((1, 2), (0, 1), (1, 2)):
        known = set(clamp_block(*offsets, choice) for choice in range(4))
        known.update(upper_block(*offsets, choice) for choice in range(4))
        matches = [(parent.find(text, cursor), text) for text in known
                   if parent.find(text, cursor) >= 0]
        if not matches:
            raise ValueError("unrecognized parent rectangle")
        start, text = min(matches)
        cursor = start + len(text)
        spans.append((start, cursor, offsets, text))
    edits = []
    if form < 4:
        phase, shape = ((0, 1), (0, 3), (1, 1), (1, 3))[form]
        start, end, offsets, text = spans[phase]
        edits.append((start, end, clamp_block(*offsets, shape)))
    elif form == 4:
        start, end, offsets, text = spans[2]
        text = text.replace("                    bounds.m_maximumY = min(",
                            "                    int height = m_map.m_mapHeight;\n                    bounds.m_maximumY = min(")
        text = text.replace(", m_map.m_mapHeight);", ", height);")
        text = text.replace("                    bounds.m_maximumX = min(",
                            "                    int width = m_map.m_mapWidth;\n                    bounds.m_maximumX = min(")
        text = text.replace(", m_map.m_mapWidth);", ", width);")
        edits.append((start, end, text))
    else:
        for phase, shape in ((0, 1 + (form - 5) % 3), (1, 1 + (form - 5) // 3)):
            start, end, offsets, text = spans[phase]
            edits.append((start, end, clamp_block(*offsets, shape)))
    for start, end, text in sorted(edits, reverse=True):
        parent = parent[:start] + text + parent[end:]
    return parent


def corner_frontier(source, checkpoint_path):
    context = checkpoint_path.parent
    checkpoint = json.loads(checkpoint_path.read_text())
    if checkpoint.get("generation", 0) != 2 or len(checkpoint["elites"]) != 10:
        raise ValueError("unfinished ten-parent upper-corner population")
    payload, originals, axes = source_families.load_manifest(context / "input.json", HOMM3_DIR)
    if len(checkpoint["records"]) != 64 or any(not row["scores"] for row in checkpoint["records"]):
        raise ValueError("upper-corner population is not fully scored")
    for folder in ("src", "include"):
        frozen, live = context / "snapshot" / folder, HOMM3_DIR / folder
        paths = [p.relative_to(frozen) for p in frozen.rglob("*") if p.is_file()]
        live_paths = [p.relative_to(live) for p in live.rglob("*")
                      if p.is_file() and "build" not in p.relative_to(live).parts]
        if set(paths) != set(live_paths) or any((frozen / p).read_bytes() != (live / p).read_bytes() for p in paths):
            raise ValueError("changed clearance snapshot: " + folder)
    definition = generator("generate-rmg-position-family.py").definition
    current = definition(source, NAME)
    forms, seen, parents = [("unchanged", current)], {current}, []
    for elite in checkpoint["elites"]:
        rendered = source_families.render(originals, axes, tuple(elite["choices"]))
        repeated = context / "candidates" / elite["id"] / "repeat"
        result = json.loads((repeated / "result.json").read_text())
        for key in ("scores", "object_hash", "source_hashes", "choices"):
            if result[key] != elite[key]:
                raise ValueError("clearance parent did not reproduce " + key)
        for relative, text in rendered.items():
            if (repeated / "tree" / relative).read_text() != text or result["source_hashes"][relative] != source_families.digest(text.encode()):
                raise ValueError("changed reproduced clearance source")
        parent = definition(rendered["src/rmg.cpp"], NAME)
        parents.append((elite["id"], parent))
        if parent not in seen:
            seen.add(parent)
            forms.append((elite["id"] + "+parent", parent))
    for form in range(14):
        for identity, parent in parents:
            body = refine(parent, form)
            if body in seen:
                continue
            seen.add(body)
            forms.append((identity + "+corner_%d" % form, body))
            if len(forms) == 60:
                return forms
    return forms


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--upper-corners", action="store_true")
    parser.add_argument("--corners-from", type=Path)
    args = parser.parse_args()
    helper = generator("generate-rmg-position-family.py")
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    original = helper.definition(source, NAME)
    baseline = baseline_definition(source)
    forms = upper_variants(baseline) if args.upper_corners else variants(baseline)
    if args.corners_from:
        forms = corner_frontier(source, args.corners_from)
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[
        helper.axis("clearance_rectangles", "src/rmg.cpp", original, forms)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "clearance-rectangle states")


if __name__ == "__main__":
    main()
