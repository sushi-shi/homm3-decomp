#!/usr/bin/env python3
"""Generate 60 retail-supported neighbour-ring gap enumeration candidates.

repairTerrainPoint (0x5b5440) has no Dreamcast counterpart. Retail +0x4de
enters one increment/mask/test header, with both matching-neighbour and
completed-gap backedges returning there. The current assignment-condition
while is rotated into entry and bottom checks. Cross six outer loop forms,
five gap receiver bindings and two diagonal local widths, leaving all
canonical helpers, preceding repair arms and following gap selection intact.
Every variant keeps the inner wraparound exit, gap order, field store order,
and cardinal/diagonal weights. No artificial work or compiler controls.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source, hypotheses

FUNCTION = "?repairTerrainPoint@rmgTerrainPainter@@QAEXABUTRmgGridPoint@@@Z"
START = "        unsigned int direction = first;\n"
END = "\n    gapsBuilt:"
INNER_LOOPS = ("do", "top_for", "top_while", "condition_while", "condition_for", "label")
EXIT_COMMENT = ("                        // Retail +0x52d leaves both loops directly. A\n"
                "                        // compound do condition followed by another test\n"
                "                        // duplicates this comparison and rotates the exit.\n")


def scans(inner="do"):
    if inner not in INNER_LOOPS:
        raise ValueError("review the inner gap loop")
    for loop, receiver, diagonal in itertools.product(
            ("condition_while", "top_for", "top_while", "label_header", "top_do", "increment_for"),
            ("index_post", "index_separate", "reference", "pointer", "last_subscript"),
            ("byte", "word")):
        lines = ["unsigned int direction = first;"]
        if loop == "condition_while":
            lines += ["while ((direction = (direction + 1) % TILE_DIR_COUNT) != first) {"]
        elif loop == "increment_for":
            lines += ["for (direction = (first + 1) % TILE_DIR_COUNT; direction != first;",
                      "     direction = (direction + 1) % TILE_DIR_COUNT) {"]
        else:
            if loop == "label_header":
                lines += ["nextDirection:", "{"]
            else:
                lines += [{"top_for": "for (;;) {", "top_while": "while (1) {", "top_do": "do {"}[loop]]
            lines += ["    direction = (direction + 1) % TILE_DIR_COUNT;",
                      "    if (direction == first)", "        goto gapsBuilt;"]
        lines += ["    if (!matches[direction]) {"]
        if receiver == "index_post":
            setup, prefix = ["unsigned int currentGap = gapCount++;"], "gaps[currentGap]."
        elif receiver == "index_separate":
            setup, prefix = ["unsigned int currentGap = gapCount;", "++gapCount;"], "gaps[currentGap]."
        elif receiver == "reference":
            setup, prefix = ["TRmgTerrainGap& currentGap = gaps[gapCount++];"], "currentGap."
        elif receiver == "pointer":
            setup, prefix = ["TRmgTerrainGap* currentGap = &gaps[gapCount++];"], "currentGap->"
        else:
            setup, prefix = ["++gapCount;"], "gaps[gapCount - 1]."
        lines += ["        " + line for line in setup]
        lines += ["        " + prefix + "m_weight = 0;",
                  "        " + prefix + "m_start = direction;",
                  "        " + prefix + "m_length = 0;"]
        if inner == "label":
            lines += ["    nextGapCell:", "        {"]
        else:
            lines += ["        " + {"do": "do {", "top_for": "for (;;) {", "top_while": "while (1) {",
                                     "condition_while": "while (!matches[direction]) {",
                                     "condition_for": "for (; !matches[direction];) {"}[inner]]
        lines += [
                  "            unsigned " + ("char" if diagonal == "byte" else "int") + " diagonal = direction & 1;",
                  "            " + prefix + "m_weight += diagonal ? 1 : 2;",
                  "            ++" + prefix + "m_length;",
                  "            direction = (direction + 1) % TILE_DIR_COUNT;",
                  "            if (direction == first)",
                  "                goto gapsBuilt;"]
        if inner in ("top_for", "top_while"):
            lines += ["            if (matches[direction])", "                break;"]
        elif inner == "label":
            lines += ["            if (!matches[direction])", "                goto nextGapCell;"]
        lines += ["        } while (!matches[direction]);" if inner == "do" else "        }", "    }"]
        if loop == "label_header":
            lines += ["    goto nextDirection;"]
        lines += ["} while (1);" if loop == "top_do" else "}"]
        body = "\n".join("        " + line for line in lines) + "\n"
        # Retain the existing evidence comment in the exact canonical option.
        if (loop, receiver, diagonal, inner) == ("condition_while", "index_post", "byte", "do"):
            body = body.replace("                        goto gapsBuilt;", EXIT_COMMENT + "                        goto gapsBuilt;")
        yield "+".join((loop, receiver, diagonal)), body


def make_manifest(source):
    definitions = _source.find_definitions(source, FUNCTION)
    if len(definitions) != 1:
        raise ValueError("review the unique canonical repairTerrainPoint")
    function = source[definitions[0].head:definitions[0].body_close + 1]
    if function.count(START) != 1 or function.count(END) != 1:
        raise ValueError("review the gap enumeration boundaries")
    start = function.index(START)
    end = function.index(END, start)
    original = function[start:end]
    options = list(scans())
    known = {body for inner in INNER_LOOPS for _, body in scans(inner)}
    # Main's verified outer-cycle break replaces only the outer goto. Keep
    # the inner multi-level exit and all historical source controls intact.
    outer_break = "\n            if (direction == first)\n                break;"
    outer_goto = "\n            if (direction == first)\n                goto gapsBuilt;"
    legacy = original.replace(outer_break, outer_goto, 1)
    if original not in known and legacy not in known:
        raise ValueError("review the gap enumeration before rebasing the family")
    if original not in known:
        options = [(name, original if body == legacy else body) for name, body in options]
    options.sort(key=lambda row: row[1] != original)
    return dict(schema=1, unit="rmg_terrain", function=FUNCTION, evidence=__doc__, axes=[dict(
        name="gap_scan", find=original,
        options=[dict(name=name, replace=body) for name, body in options])])


def make_inner_manifest(source, parents):
    payload = make_manifest(source)
    choices = {name for name, _ in scans()}
    if len(parents) != 10 or len(set(parents)) != 10 or any(name not in choices for name in parents):
        raise ValueError("review ten unique gap-scan parents")
    variants = {inner: dict(scans(inner)) for inner in INNER_LOOPS}
    payload["axes"][0]["name"] = "gap_scan_inner"
    payload["axes"][0]["options"] = [dict(name=parent + "+" + inner, replace=variants[inner][parent])
                                     for parent in parents for inner in INNER_LOOPS]
    payload["evidence"] += ("\nFollow-up: recompile ten highest-ranked gap scans against six inner-loop "
                            "forms, preserving the direct wraparound exit and all field updates. "
                            "The canonical baseline is separately compiled in the same context.")
    return payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--inner-from", type=Path, help="cross ten completed gap scans with six inner loop forms")
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
    if args.inner_from:
        parent = json.loads(args.inner_from.read_text())
        if (parent.get("schema") != 1 or parent.get("unit") != "rmg_terrain" or parent.get("function") != FUNCTION
                or parent.get("source_sha256") != hashlib.sha256(source.encode()).hexdigest()):
            raise ValueError("review the gap-scan parents against the current source")
        rows = [row for row in parent["results"] if not row["error"] and row["score"] is not None]
        payload = make_inner_manifest(source, [row["labels"]["gap_scan"] for row in rows[:10]])
        payload["parent_results"] = str(args.inner_from.resolve())
    else:
        payload = make_manifest(source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    parsed = hypotheses.parse_manifest(args.output)
    print("generated", len(hypotheses.variants(parsed[4], parsed[5])), "unique source hypotheses ->", args.output)


if __name__ == "__main__":
    main()
