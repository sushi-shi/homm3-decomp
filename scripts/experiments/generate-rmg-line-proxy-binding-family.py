#!/usr/bin/env python3
"""Generate real proxy constructor parameter and returned-value alternatives.

Retail 0x4fa050 proves at(const grid&) through its ret 8 hidden-result ABI;
that boundary stays fixed. The in-place proxy constructor is expanded and has
no separately proved parameter ABI. Its coordinate may be copied by value or
bound by const reference before copying into the owned member. Compare those
two genuine interfaces and six member initialization forms, with canonical at
return, refresh entry initialization and assigned grid return alternatives.
Declarations and definitions change together. No duplicated overloads, empty
destructors, false inline declarations or synthetic caller mass are generated.
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
        "rmg_line_proxy_copy", Path(__file__).with_name("generate-rmg-line-proxy-copy-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def helpers():
    return parent().helpers()


def constructor_definition(source, **kwargs):
    return parent().constructor_definition(source, **kwargs)


def tile_forms():
    return parent().tile_forms()


def make_axes(header, source):
    refresh = parent().parent()
    original = constructor_definition(source)
    declaration = refresh.constructor_declaration(source)
    if header.count(declaration) != 1:
        raise ValueError("review the proxy constructor declaration and definition together")
    options = [dict(name="baseline", replace=declaration)]
    for binding, parameter in (("reference", "const TRmgGridPoint&"), ("value", "TRmgGridPoint")):
        for name, body in refresh.constructor_forms(parameter):
            if body != original:
                prototype = f"    TRmgLinePainterTile(TRmgLinePainterInterface* painter, {parameter} point);"
                options.append(dict(name=binding + "+" + name, replace=prototype,
                                    extra_edits=[dict(source=SOURCE, find=original, replace=body)]))
    if len(options) != 12:
        raise ValueError("review the current constructor before expanding its binding family")
    # Keep proxy copying fixed while testing how the original coordinate enters
    # its value constructor. The preceding family remains available separately.
    rest = parent().make_axes(header, source)[1:]
    return [dict(name="constructor_binding", source=HEADER, find=declaration, options=options), *rest]


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
