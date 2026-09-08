#!/usr/bin/env python3
"""Generate genuine proxy copy/return lifetimes for retail 0x4fa050/0x4f9f00.

The retained at() body writes a twelve-byte painter/coordinate value through
its hidden return pointer, while refresh expands that operation at entry and
retains it for neighbours. The previous 384-case family never changed the
proxy's implicit copy operation. Compare that operation with ordinary explicit
memberwise copies, returned-value and caller initialization forms, and the
assigned grid translation's named/compound return. Every source still calls
the canonical at() and compound-add helpers; no caller mass or inline pins.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import re

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


def helpers():
    return parent().helpers()


def constructor_definition(source, **kwargs):
    return parent().constructor_definition(source, **kwargs)


def tile_forms():
    return parent().tile_forms()


def copy_forms():
    yield "implicit", ""
    signature = "TRmgLinePainterTile::TRmgLinePainterTile(const TRmgLinePainterTile& other)"
    for name, initializer, body in (
            ("members_copy", " : m_painter(other.m_painter), m_point(other.m_point)", ""),
            ("members_coordinates", " : m_painter(other.m_painter), m_point(other.m_point.m_x, other.m_point.m_y)", ""),
            ("pointer_then_assignment", " : m_painter(other.m_painter)", "    m_point = other.m_point;\n"),
            ("point_then_pointer", " : m_point(other.m_point)", "    m_painter = other.m_painter;\n"),
            ("assign_pointer_point", "", "    m_painter = other.m_painter;\n    m_point = other.m_point;\n"),
            ("assign_point_pointer", "", "    m_point = other.m_point;\n    m_painter = other.m_painter;\n")):
        yield name, signature + ("\n   " + initializer if initializer else "") + "\n{\n" + body + "}"


def entry_forms():
    yield "copy_initialized", "    TRmgLinePainterTile tile = painter->at(point);"
    yield "direct_initialized", "    TRmgLinePainterTile tile(painter->at(point));"


def make_axes(header, source):
    original = constructor_definition(source, copy=True, required=False)
    if original not in dict(copy_forms()).values():
        raise ValueError("review the current proxy copy operation")
    value_declaration = parent().constructor_declaration(source) + "\n"
    copy_declaration = "    TRmgLinePainterTile(const TRmgLinePainterTile& other);\n"
    declaration = value_declaration + (copy_declaration if original else "")
    if header.count(declaration) != 1 or bool(copy_declaration in header) != bool(original):
        raise ValueError("review the proxy copy declaration and definition together")
    copies = [dict(name="baseline", replace=declaration)]
    for name, body in copy_forms():
        if body == original:
            continue
        edit = (dict(source=SOURCE, find=original, replace=body) if original else
                dict(source=SOURCE, insert_before="int TRmgLinePainterTile::getLand()",
                     text=body + "\n\n"))
        copies.append(dict(name=name, replace=value_declaration + (copy_declaration if body else ""),
                           extra_edits=[edit]))
    helper = helpers()
    refresh = helper.definition(source, "refreshRmgLinePoint")
    entry = next((body for _, body in entry_forms() if body in refresh), None)
    if entry is None:
        raise ValueError("review the refresh's real entry proxy initialization")
    header_start = header.index("    TRmgGridPoint operator+(const TPoint& offset) const")
    method = header[header_start:header.index("\n};", header_start)]
    statement = re.search(r"^        TRmgGridPoint result\b", method, re.M)
    if statement is None:
        raise ValueError("review the grid translation's assigned result")
    translation = method[statement.start():method.rindex("\n    }")]
    grid_forms = [(name, body) for name, body in parent().grid_forms() if name.startswith("assigned+")]
    if translation not in dict(grid_forms).values():
        raise ValueError("the proxy-copy follow-up requires the reviewed assigned grid translation")
    return [dict(name="proxy_copy", source=HEADER, find=declaration, options=copies),
            helper.axis("proxy_return", SOURCE, helper.definition(source, "TRmgLinePainterInterface::at"),
                        parent().return_forms()),
            helper.axis("entry_proxy", SOURCE, refresh,
                        ((name, refresh.replace(entry, body)) for name, body in entry_forms())),
            helper.axis("grid_translation", HEADER, translation, grid_forms)]


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
