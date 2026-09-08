#!/usr/bin/env python3
"""Generate real line-proxy construction and border-coordinate lifetimes.

Clear 0x4fa080 and walker paintPoint 0x4fa3c0 construct their entry proxy in
place; the current at()-return spellings retain an extra proxy copy. Compare
returned-proxy initialization with direct calls to the same ordinary value
constructor, not pasted member stores. These two entry sites have no retained
at() call proving a factory boundary. Refresh keeps its canonical at() calls.
Clear also compares initializing its border y in the for clause with setting
it before the cached upper bound. Keep the asymmetric retail bounds and query
order. Score all three TUs for each candidate.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

SOURCE = "src/rmg_terrain.cpp"
HEADER = "include/rmg.h"


def parent():
    spec = importlib.util.spec_from_file_location(
        "rmg_line_refresh", Path(__file__).with_name("generate-rmg-line-refresh-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def entries(painter):
    yield "factory_copy", f"TRmgLinePainterTile tile = {painter}->at(point);"
    yield "factory_direct", f"TRmgLinePainterTile tile({painter}->at(point));"
    yield "constructor_direct", f"TRmgLinePainterTile tile({painter}, point);"
    yield "constructor_copy", f"TRmgLinePainterTile tile = TRmgLinePainterTile({painter}, point);"


def replace_entry(body, painter, replacement):
    original = next((text for _, text in entries(painter) if text in body), None)
    if original is None or body.count(original) != 1:
        raise ValueError("review the real entry proxy construction")
    return body.replace(original, replacement)


def border_y(body, form):
    initial = "rectangle.m_origin.m_y > 0 ? rectangle.m_origin.m_y - 1 : 0"
    prefix = f"        point.m_y = {initial};\n"
    early_loop = "        for (; point.m_y < end; ++point.m_y) {"
    local_loop = f"        for (point.m_y = {initial}; point.m_y < end; ++point.m_y) {{"
    cached_prefix = f"        unsigned int first = {initial};\n"
    cached_loop = "        for (point.m_y = first; point.m_y < end; ++point.m_y) {"
    if body.count(cached_prefix) == 2 and body.count(cached_loop) == 2:
        body = body.replace(cached_prefix, prefix).replace(cached_loop, early_loop)
    if body.count(prefix) == 2 and body.count(early_loop) == 2:
        if form == "early":
            return body
        return body.replace(prefix, "").replace(early_loop, local_loop)
    if body.count(local_loop) == 2 and prefix not in body:
        if form == "loop":
            return body
        body = body.replace(local_loop, early_loop)
        anchor = "        unsigned int end = rectangle.m_origin.m_y + rectangle.m_size.m_y"
        if body.count(anchor) != 2:
            raise ValueError("review the two cached vertical-border ends")
        return body.replace(anchor, prefix + anchor)
    raise ValueError("review the clear's border-coordinate lifetimes")


def make_axes(header, source):
    module = parent()
    helper = module.helpers()
    constructor = module.constructor_definition(source)
    clear = helper.definition(source, "clearRmgLineRectangle")
    refresh = helper.definition(source, "refreshRmgLinePoint")
    point = helper.definition(source, "TRmgLineWalker::paintPoint")
    clear_forms = ((entry + "+" + lifetime, border_y(replace_entry(clear, "painter", text), lifetime))
                   for (entry, text), lifetime in itertools.product(entries("painter"), ("early", "loop")))
    return [helper.axis("proxy_constructor", SOURCE, constructor,
                        module.constructor_forms(module.constructor_parameter(source))),
            helper.axis("clear_entry", SOURCE, clear, clear_forms),
            helper.axis("refresh_entry", SOURCE, refresh,
                        ((name, replace_entry(refresh, "painter", text))
                         for name, text in list(entries("painter"))[:2])),
            helper.axis("point_entry", SOURCE, point,
                        ((name, replace_entry(point, "m_painter", text)) for name, text in entries("m_painter")))]


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
