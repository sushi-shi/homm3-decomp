#!/usr/bin/env python3
"""Generate 60 point-lifetime candidates for the canonical gap predicates.

repairTerrainPoint 0x5b5440 now has the retail CFG and all but one of its
named calls. Retail +0x449 retains the coordinate-reference TRmgGridPoint
constructor in the final vertical-gap expansion. Earlier expanded gap reads
also preserve more coordinate homes than the candidate. Keep the exact
ordinary horizontal/vertical predicate interfaces and canonical grid type;
cross five real point lifetimes, three short-circuit scopes, and four sites.
All dimension tests and terrain reads keep their original order. No copied
helper, extra declaration, forced inline qualifier, pragma or dummy work.
This Complete-only RMG cluster has no Dreamcast source counterpart.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import _source, hypotheses

FUNCTION = "?repairTerrainPoint@rmgTerrainPainter@@QAEXABUTRmgGridPoint@@@Z"
KINDS = ("Horizontal", "Vertical")
BINDINGS = ("direct", "const_value", "copy", "assigned", "const_reference")
FLOWS = ("guards", "scoped_guards", "nested")
SITES = ("horizontal", "vertical", "both", "second_only")


def signature(kind):
    return ("unsigned char rmgTerrainPainter::is" + kind + "Gap(\n"
            "    const TRmgGridPoint& point, int terrain)")


def expressions(kind):
    horizontal = kind == "Horizontal"
    axis, dimension = ("x", "getWidth()") if horizontal else ("y", "getHeight()")
    bound = "point.m_" + axis + " > 0 && point.m_" + axis + " < " + dimension + " - 1"
    points = [("point.m_x " + sign + " 1, point.m_y") if horizontal else
              ("point.m_x, point.m_y " + sign + " 1") for sign in ("-", "+")]
    return bound, points


def canonical(kind):
    bound, points = expressions(kind)
    return (signature(kind) + "\n{\n    return " + bound + "\n"
            + "\n".join("        && getTerrain(TRmgGridPoint(" + value + ")) != terrain" +
                         (";" if index else "") for index, value in enumerate(points)) + "\n}")


def query(value, name, binding, named):
    if not named:
        return [], "getTerrain(TRmgGridPoint(" + value + "))"
    if binding == "direct":
        setup = ["TRmgGridPoint " + name + "(" + value + ");"]
    elif binding == "const_value":
        setup = ["const TRmgGridPoint " + name + "(" + value + ");"]
    elif binding == "copy":
        setup = ["TRmgGridPoint " + name + " = TRmgGridPoint(" + value + ");"]
    elif binding == "assigned":
        setup = ["TRmgGridPoint " + name + ";", name + " = TRmgGridPoint(" + value + ");"]
    else:
        setup = ["const TRmgGridPoint& " + name + " = TRmgGridPoint(" + value + ");"]
    return setup, "getTerrain(" + name + ")"


def body(kind, binding, flow, site):
    if site in ("horizontal", "vertical") and site != kind.lower():
        return canonical(kind)
    bound, points = expressions(kind)
    first, first_query = query(points[0], "first", binding, site != "second_only")
    second, second_query = query(points[1], "second", binding, True)
    if flow in ("guards", "scoped_guards"):
        lines = ["if (!(" + bound + "))", "    return 0;"]
        if flow == "scoped_guards":
            lines += ["{"] + ["    " + line for line in first]
            lines += ["    if (" + first_query + " == terrain)", "        return 0;", "}"]
        else:
            lines += first + ["if (" + first_query + " == terrain)", "    return 0;"]
        lines += second + ["return " + second_query + " != terrain;"]
    else:
        lines = ["if (" + bound + ") {"] + ["    " + line for line in first]
        lines += ["    if (" + first_query + " != terrain) {"]
        lines += ["        " + line for line in second]
        lines += ["        return " + second_query + " != terrain;", "    }", "}", "return 0;"]
    return signature(kind) + "\n{\n" + "\n".join("    " + line for line in lines) + "\n}"


def pairs():
    for binding, flow, site in itertools.product(BINDINGS, FLOWS, SITES):
        yield "+".join((binding, flow, site)), tuple(body(kind, binding, flow, site) for kind in KINDS)


def make_manifest(source):
    options = list(pairs())
    originals = []
    for index, kind in enumerate(KINDS):
        # The definition locator intentionally does not infer overload arity
        # from a mangled spelling. Select this proven two-argument signature.
        found = [item for item in _source.find_definitions(source, "rmgTerrainPainter::is" + kind + "Gap")
                 if source[source.rfind("\n", 0, item.head) + 1:item.body_open].strip() == signature(kind)]
        if len(found) != 1:
            raise ValueError("review the unique two-argument " + kind + " gap predicate")
        item = found[0]
        start = source.rfind("\n", 0, item.head) + 1
        original = source[start:item.body_close + 1]
        if original not in {canonical(kind), *(pair[index] for _, pair in options)}:
            raise ValueError("review the " + kind + " gap predicate before rebasing")
        originals.append(original)
    return dict(schema=1, unit="rmg_terrain", function=FUNCTION, evidence=__doc__, axes=[dict(
        name="gap_points", find=originals[0], options=[dict(
            name=name, replace=pair[0], extra_edits=[dict(find=originals[1], replace=pair[1])])
            for name, pair in options])])


def make_order_manifest(source, parents):
    payload = make_manifest(source)
    choices = dict(pairs())
    if len(parents) != 10 or len(set(parents)) != 10 or any(name not in choices for name in parents):
        raise ValueError("review ten unique gap-point parents")
    spans, blocks = {}, {}
    for key, marker, selector in (
            ("transitions", "// The unsigned grid/ref-argument correction", "rmgTerrainPainter::paintTransitions"),
            ("horizontal", "// These ordinary gap predicates", "rmgTerrainPainter::isHorizontalGap"),
            ("vertical", "VA(0x005B6430,", "rmgTerrainPainter::isVerticalGap")):
        found = _source.find_definitions(source, selector)
        if key != "transitions":
            found = [item for item in found
                     if source[source.rfind("\n", 0, item.head) + 1:item.body_open].strip() == signature(key.title())]
        if len(found) != 1:
            raise ValueError("review the ordered definition " + key)
        item = found[0]
        start = source.rfind(marker, 0, item.head)
        if start < 0:
            raise ValueError("review the definition evidence comment " + key)
        spans[key] = (start, item.body_close + 1)
        blocks[key] = source[start:item.body_close + 1]
    ordered = sorted(spans, key=lambda key: spans[key][0])
    for left, right in zip(ordered, ordered[1:]):
        if source[spans[left][1]:spans[right][0]].strip():
            raise ValueError("review the contiguous transition/gap definitions")
    original = source[spans[ordered[0]][0]:spans[ordered[-1]][1]]
    axis = payload["axes"][0]
    old_horizontal, old_vertical = axis["find"], axis["options"][0]["extra_edits"][0]["find"]
    options = []
    for parent in parents:
        horizontal, vertical = choices[parent]
        edited = dict(blocks)
        edited["horizontal"] = edited["horizontal"].replace(old_horizontal, horizontal)
        edited["vertical"] = edited["vertical"].replace(old_vertical, vertical)
        for order in itertools.permutations(blocks):
            options.append(dict(name=parent + "+order:" + ",".join(order),
                                replace="\n\n".join(edited[key] for key in order)))
    payload["axes"] = [dict(name="gap_points_order", find=original, options=options)]
    payload["evidence"] += ("\nFollow-up: cross ten completed point-lifetime parents with six orders of the "
                            "canonical horizontal/vertical predicates and adjacent transition painter. "
                            "Move whole definitions with their evidence comments, retaining each exactly once. "
                            "Recompile every parent and a separate canonical baseline.")
    return payload


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--order-from", type=Path, help="cross ten completed parents with six definition orders")
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg_terrain.cpp").read_text()
    if args.order_from:
        parent = json.loads(args.order_from.read_text())
        if (parent.get("schema") != 1 or parent.get("unit") != "rmg_terrain" or parent.get("function") != FUNCTION
                or parent.get("source_sha256") != hashlib.sha256(source.encode()).hexdigest()):
            raise ValueError("review the gap-point parents against the current source")
        rows = [row for row in parent["results"] if not row["error"] and row["score"] is not None]
        payload = make_order_manifest(source, [row["labels"]["gap_points"] for row in rows[:10]])
        payload["parent_results"] = str(args.order_from.resolve())
    else:
        payload = make_manifest(source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    parsed = hypotheses.parse_manifest(args.output)
    print("generated", len(hypotheses.variants(parsed[4], parsed[5])), "unique source hypotheses ->", args.output)


if __name__ == "__main__":
    main()
