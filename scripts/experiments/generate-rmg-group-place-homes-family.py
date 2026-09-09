#!/usr/bin/env python3
"""Six remaining coordinate displacements in canPlaceTreasureGroup 0x546c70.

The 99.9850% source has retail's frame, CFG and all helper decisions. Entrance
storage and neighboring-point X reuse the wrong earlier coordinate homes.
Vary declarations without moving any getter, constructor, snapshot or query.
All coordinates remain real consumed values; no padding or inline controls.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
FUNCTION = "type_random_map_generator::canPlaceTreasureGroup"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, FUNCTION)


def replace(body, old, new):
    if body.count(old) != 1:
        raise ValueError("review coordinate-home anchor: " + old)
    return body.replace(old, new)


def variant(original, entrance, neighbor, guard):
    body = original
    if entrance:
        body = replace(body, "    TRmgMapPosition entrance = lastObject->getPosition();",
                       "    entrance = lastObject->getPosition();")
        declaration = "    TRmgMapPosition entrance;\n"
        if entrance == 1:
            body = body.replace("{\n", "{\n" + declaration, 1)
        elif entrance == 2:
            anchor = "    if (group->m_hasGuard) {\n"
            body = replace(body, anchor, declaration + anchor)
        elif entrance == 3:
            anchor = "    int firstDirection = 0;\n"
            body = replace(body, anchor, declaration + anchor)
        else:
            anchor = "    TObjectType* prototype = lastObject->m_properties->m_prototype;\n"
            body = replace(body, anchor, declaration + anchor)
    if neighbor:
        start = body.index("    int direction;\n")
        end = body.index("    int allowEntrances;\n", start)
        scan = body[start:end]
        scan = replace(scan, "        TPoint point = ", "        point = ")
        scan = scan.replace("point.m_", "neighborPoint.m_").replace("        point = ", "        neighborPoint = ")
        body = body[:start] + scan + body[end:]
        declaration = "    TPoint neighborPoint;\n"
        if neighbor == 1:
            body = replace(body, "    int direction;\n", declaration + "    int direction;\n")
        else:
            body = body.replace("{\n", "{\n" + declaration, 1)
        if neighbor == 3:
            start = body.index("    TPoint point;\n")
            tail = body[start:]
            tail = replace(tail, "    TPoint point;\n", "")
            body = body[:start] + tail.replace("point.m_", "neighborPoint.m_")
    if guard:
        start = body.index("    if (group->m_hasGuard) {\n")
        end = body.index("    int firstDirection = 0;\n", start)
        scan = body[start:end]
        scan = replace(scan, "        TRmgMapPosition point;\n", "")
        scan = scan.replace("point.m_", "guardPoint.m_")
        if guard == 1:
            scan = scan.replace("{\n", "{\n        TRmgMapPosition guardPoint;\n", 1)
        body = body[:start] + scan + body[end:]
        if guard == 2:
            body = body.replace("{\n", "{\n    TRmgMapPosition guardPoint;\n", 1)
    return body


def axes(source):
    original = definition(source)
    options = [(f"entrance_{e}+neighbor_{n}+guard_{g}", variant(original, e, n, g))
               for e, n, g in itertools.product(range(5), range(4), range(3))]
    axis = generator("generate-rmg-position-family.py").axis("group_coordinate_homes", SOURCE, original, options)
    if len(axis["options"]) != 60:
        raise ValueError("expected sixty distinct coordinate-home forms")
    return [axis]


def parents(path, source):
    directory = path.parent
    expected = dict(schema=1, units=["rmg"], axes=axes(source), evidence=__doc__)
    if json.loads((directory / "input.json").read_text()) != expected:
        raise ValueError("review coordinate-home parent manifest")
    if (directory / "snapshot" / SOURCE).read_bytes() != (HOMM3_DIR / SOURCE).read_bytes():
        raise ValueError("coordinate-home source snapshot changed")
    saved, current = directory / "snapshot/include", HOMM3_DIR / "include"
    if {p.relative_to(saved) for p in saved.rglob("*") if p.is_file()} != {p.relative_to(current) for p in current.rglob("*") if p.is_file()}:
        raise ValueError("coordinate-home header population changed")
    for item in saved.rglob("*"):
        if item.is_file() and item.read_bytes() != (current / item.relative_to(saved)).read_bytes():
            raise ValueError("coordinate-home header changed: " + str(item))
    checkpoint = json.loads(path.read_text())
    rows = checkpoint["records"]
    if len(rows) != 60 or any(not row.get("scores") for row in rows):
        raise ValueError("expected sixty scored coordinate-home states")
    if len(checkpoint["elites"]) != min(10, len({row["object_hash"] for row in rows})):
        raise ValueError("expected the full reproduced coordinate-home frontier")
    result = []
    for row in checkpoint["elites"]:
        candidate = directory / "candidates" / row["id"]
        repeated = json.loads((candidate / "repeat/result.json").read_text())
        if any(repeated.get(key) != row[key] for key in ("choices", "scores", "object_hash", "source_hashes")):
            raise ValueError("coordinate-home parent did not reproduce")
        rendered = source.replace(expected["axes"][0]["find"], expected["axes"][0]["options"][row["choices"][0]]["replace"])
        actual = (candidate / "first/tree" / SOURCE).read_text()
        if actual != rendered or hashlib.sha256(actual.encode()).hexdigest() != row["source_hashes"][SOURCE]:
            raise ValueError("coordinate-home rendered parent changed")
        result.append((row["id"], definition(actual)))
    return result


def phase_variant(original, ownership, scopes):
    body = original
    getter = "lastObject->getPosition();"
    if ownership == 1:
        # A genuine reference to the still-live shared coordinate, not a
        # duplicated assignment or a compiler-only local.
        body = body.replace("    TRmgMapPosition entrance;\n", "")
        body = body.replace("    TRmgMapPosition entrance = " + getter, "    entrance = " + getter)
        body = replace(body, "    entrance = " + getter,
            "    TRmgMapPosition& entrance = workingPosition;\n    entrance = " + getter)
    elif ownership == 2:
        line = "    TRmgMapPosition entrance = " + getter
        if line in body:
            body = replace(body, line, "    const TRmgMapPosition& returnedPosition = " + getter +
                "\n    TRmgMapPosition entrance = returnedPosition;")
        else:
            body = replace(body, "    entrance = " + getter,
                "    const TRmgMapPosition& returnedPosition = " + getter + "\n    entrance = returnedPosition;")
    if scopes & 1:
        if ownership == 1:
            raise ValueError("shared working position must outlive the initial phase")
        body = replace(body, "    TRmgMapPosition workingPosition;\n", "")
        # A predeclared entrance may sit before the guard. Keep it outside
        # this shorter scope; its getter still stays after the guard checks.
        if "    TRmgMapPosition entrance;\n" in body:
            body = replace(body, "    TRmgMapPosition entrance;\n", "")
            body = body.replace("{\n", "{\n    TRmgMapPosition entrance;\n", 1)
        start = body.index("    for (unsigned int i = 0;")
        guard = body.index("    if (group->m_hasGuard) {\n", start)
        opening = body.index("{", guard)
        depth = 1
        end = opening + 1
        while depth:
            if body[end] == "{":
                depth += 1
            elif body[end] == "}":
                depth -= 1
            end += 1
        section = "    TRmgMapPosition workingPosition;\n" + body[start:end]
        body = body[:start] + "    {\n" + "\n".join("    " + line for line in section.splitlines()) + "\n    }" + body[end:]
    if scopes & 2:
        start = body.index("    int firstDirection = 0;\n")
        end = body.index("    int allowEntrances;\n", start)
        section = body[start:end]
        body = body[:start] + "    {\n" + "\n".join("    " + line for line in section.splitlines()) + "\n    }\n" + body[end:]
    return body


def phase_axes(source, retained):
    options = list(retained)
    refinements = []
    for _, body in retained:
        choices = []
        for ownership, scopes in itertools.product(range(3), range(4)):
            if (ownership, scopes) == (0, 0) or (ownership == 1 and scopes & 1):
                continue
            choices.append((f"ownership_{ownership}+scopes_{scopes}", phase_variant(body, ownership, scopes)))
        refinements.append(choices)
    for generation in range(9):
        for index, (label, _) in enumerate(retained):
            name, body = refinements[index][(generation + index) % len(refinements[index])]
            options.append((label + "+" + name, body))
    axis = generator("generate-rmg-position-family.py").axis("group_coordinate_phases", SOURCE, definition(source), options)
    axis["options"] = axis["options"][:60]
    # A small frontier may genuinely exhaust below sixty after equivalent
    # source forms are removed; never add dummy alternatives to fill it.
    return [axis]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--phases-from", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=["rmg"], axes=axes((HOMM3_DIR / SOURCE).read_text()), evidence=__doc__)
    if args.phases_from:
        source = (HOMM3_DIR / SOURCE).read_text()
        payload["axes"] = phase_axes(source, parents(args.phases_from, source))
        payload["parent_checkpoint"] = str(args.phases_from.resolve())
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("generated", len(payload["axes"][0]["options"]), "group coordinate-home forms ->", args.output)


if __name__ == "__main__":
    main()
