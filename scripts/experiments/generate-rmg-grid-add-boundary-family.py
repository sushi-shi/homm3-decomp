#!/usr/bin/env python3
"""Generate canonical grid-add definition placement and real value lifetimes.

Retail line paintPoint 0x4fa3c0 retains the two-dword compound-add body at
0x4fa540 in its first neighbour pass and expands it in the second. Its current
in-class definition is not a recovered inline declaration: RMG is Complete-only.
Compare that control with one ordinary definition in the painting TU, before
the cluster or after paintPoint (the retained retail order). Keep the exact
signature and canonical implementation, and cross this with genuine translated
value lifetimes in operator+ and the caller. Never suppress inlining or create
an undefined helper to force a call. All three TUs remain part of every score.
"""
import argparse
import importlib.util
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

SOURCE = "src/rmg_terrain.cpp"
HEADER = "include/rmg.h"
DECLARATION = "    TRmgGridPoint& operator+=(const TPoint& offset);"
CLASS_BODY = """    TRmgGridPoint& operator+=(const TPoint& offset)
    {
        m_x += offset.m_x;
        m_y += offset.m_y;
        return *this;
    }"""
TU_BODY = """TRmgGridPoint& TRmgGridPoint::operator+=(const TPoint& offset)
{
    m_x += offset.m_x;
    m_y += offset.m_y;
    return *this;
}"""
BEFORE = "TRmgLinePainterTile::TRmgLinePainterTile(\n    TRmgLinePainterInterface* painter,"
AFTER = "// Vtable 0x642c98 slot 1 tests the count for pattern value 1."


def parent():
    spec = importlib.util.spec_from_file_location(
        "rmg_line_neighbour", Path(__file__).with_name("generate-rmg-line-neighbour-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def placement_axis(header, source):
    in_class = header.count(CLASS_BODY) == 1
    ordinary = header.count(DECLARATION) == 1 and source.count(TU_BODY) == 1
    if in_class == ordinary or (in_class and TU_BODY in source):
        raise ValueError("review the single canonical grid compound-add definition")
    if source.count(BEFORE) != 1 or source.count(AFTER) != 1:
        raise ValueError("review grid-add placement around the line-painting cluster")
    original = CLASS_BODY if in_class else DECLARATION
    options = [dict(name="baseline", replace=original)]
    removed = [] if in_class else [dict(source=SOURCE, find=TU_BODY, replace="")]
    if ordinary:
        options.append(dict(name="in_class", replace=CLASS_BODY, extra_edits=removed))
    for name, anchor in (("ordinary_before", BEFORE), ("ordinary_after", AFTER)):
        # If already at this boundary, the baseline represents it exactly.
        if ordinary and TU_BODY + "\n\n" + anchor in source:
            continue
        options.append(dict(name=name, replace=DECLARATION, extra_edits=[
            *removed, dict(source=SOURCE, insert_before=anchor, text=TU_BODY + "\n\n")]))
    return dict(name="grid_add_definition", source=HEADER, find=original, options=options)


def make_axes(header, source):
    neighbour = parent()
    refresh = neighbour.parent().parent()
    helper = refresh.helpers()
    point = helper.definition(source, "TRmgLineWalker::paintPoint")
    chosen = {"named_sum", "named_sum_reference", "named_proxy",
              "copy+separate+factory", "assigned+operand+factory", "coordinates+separate+constructor"}
    point_options = [(name, body) for name, body in neighbour.point_forms(point) if name in chosen]
    if len(point_options) != len(chosen):
        raise ValueError("review the representative neighbour constructions")
    return [placement_axis(header, source), refresh.make_axes(header, source)[-1],
            helper.axis("neighbour_value", SOURCE, point, point_options)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    axes = make_axes((HOMM3_DIR / HEADER).read_text(), (HOMM3_DIR / SOURCE).read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes, evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
