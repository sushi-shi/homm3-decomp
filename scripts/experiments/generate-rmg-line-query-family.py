#!/usr/bin/env python3
"""Generate real proxy-query receiver/argument lifetimes and construction.

At 0x4fa135/0x4fa19d retail forms the embedded coordinate address before
finishing its y store, then loads the virtual receiver. The lower-bound
snapshot has recovered the preceding blocks. Compare meaningful pointer and
reference bindings in the canonical ordinary getLand helper, crossed with
the real proxy constructor and named/temporary border proxies. Preserve the
owned coordinate, virtual slot, argument alias and query count/order. No
alternate declaration, false inline, pasted helper body or synthetic work.
"""
import argparse
import importlib.util
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

SOURCE = "src/rmg_terrain.cpp"
HEADER = "include/rmg.h"


def parent():
    spec = importlib.util.spec_from_file_location(
        "rmg_line_neighbour", Path(__file__).with_name("generate-rmg-line-neighbour-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def query_forms():
    receiver = "    TRmgLinePainterInterface* painter = m_painter;\n"
    coordinate = "    const TRmgGridPoint& point = m_point;\n"
    for name, body in (
            ("members", "    return m_painter->getLand(m_point);\n"),
            ("receiver_pointer", receiver + "    return painter->getLand(m_point);\n"),
            ("receiver_reference", "    TRmgLinePainterInterface& painter = *m_painter;\n"
                                   "    return painter.getLand(m_point);\n"),
            ("coordinate_reference", coordinate + "    return m_painter->getLand(point);\n"),
            ("receiver_then_coordinate", receiver + coordinate + "    return painter->getLand(point);\n"),
            ("coordinate_then_receiver", coordinate + receiver + "    return painter->getLand(point);\n"),
            ("named_result", "    int land = m_painter->getLand(m_point);\n    return land;\n")):
        yield name, "int TRmgLinePainterTile::getLand()\n{\n" + body + "}"


def make_axes(header, source):
    neighbour = parent()
    refresh = neighbour.parent().parent()
    helper = refresh.helpers()
    constructor = refresh.constructor_definition(source)
    query = helper.definition(source, "TRmgLinePainterTile::getLand")
    if query not in [body for _, body in query_forms()]:
        raise ValueError("review the canonical proxy query before generating alternatives")
    clear = helper.definition(source, "clearRmgLineRectangle")
    border = next((body for _, body in neighbour.border_forms() if clear.count(body) == 4), None)
    if border is None:
        raise ValueError("review all four border query sites")
    return [helper.axis("query_lifetime", SOURCE, query, query_forms()),
            helper.axis("proxy_constructor", SOURCE, constructor,
                        refresh.constructor_forms(refresh.constructor_parameter(source))),
            helper.axis("border_proxy", SOURCE, clear,
                        ((name, clear.replace(border, body)) for name, body in neighbour.border_forms()))]


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
