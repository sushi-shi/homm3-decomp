#!/usr/bin/env python3
"""Refine the line selector's actual end-case decision and output lifetimes.

Retail 0x4f9e33 reads/tests the south neighbour before writing pattern and X,
then returns through separate Y=0/Y=1 arms. Writing pattern/X before the
decision lets VC6 replace even explicit returns with SETE (92.329% control).
Keep the reproduced cardinal and indexed-corner parents; generate complete
end-case arms, independent output order and equivalent guard structure.
The sole caller at 0x4f9f00 supplies distinct mask and output locals.
"""
import argparse
import importlib.util
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def parent():
    spec = importlib.util.spec_from_file_location(
        "rmg_line_selector", Path(__file__).with_name("generate-rmg-line-selector-family.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def end_forms(module):
    for order, south_first, horizontal_first, early in itertools.product(
            itertools.permutations("pxy"), (True, False), (True, False), (False, True)):
        order = "".join(order)
        indent = "            "
        vertical = indent + "if (" + ("" if south_first else "!") + "neighbours[TILE_DIR_SOUTH]) {\n"
        vertical += module.stores(0, 0, 0 if south_first else 1, order, indent + "    ")
        vertical += indent + "} else {\n"
        vertical += module.stores(0, 0, 1 if south_first else 0, order, indent + "    ")
        vertical += indent + "}\n"
        horizontal = module.stores(1, "neighbours[TILE_DIR_WEST]", 0, "pxy", indent)
        if horizontal_first:
            condition = "neighbours[TILE_DIR_WEST] || neighbours[TILE_DIR_EAST]"
            first, second = horizontal, vertical
        else:
            condition = "!neighbours[TILE_DIR_WEST] && !neighbours[TILE_DIR_EAST]"
            first, second = vertical, horizontal
        tail = "    if (table->m_ranges[0].m_valueCount > 0) {\n"
        tail += "        if (" + condition + ") {\n" + first + "        } else {\n" + second + "        }\n"
        if early:
            tail += "        return;\n    }\n"
            tail += module.fallback("ternary", "    ") + "    flipX = 0;\n    flipY = 0;\n"
        else:
            tail += "    } else {\n" + module.fallback("ternary", "        ")
            tail += "        flipX = 0;\n        flipY = 0;\n    }\n"
        label = "+".join((order, "south" if south_first else "not_south",
                          "horizontal" if horizontal_first else "vertical",
                          "guard" if early else "else"))
        yield label, tail


def make_axes(source):
    module = parent()
    axes = module.make_axes(source, refine=True)
    original_prefix = axes[0]["find"]
    original_tail = axes[0]["options"][1]["extra_edits"][0]["find"]
    prefix = next(prefix for label, prefix, _ in module.exit_forms(refine=True)
                  if label == "constant+constant+pxy+pyx+branch+ternary")
    options = [dict(name="baseline", replace=original_prefix)]
    seen = {(original_prefix, original_tail)}
    for label, tail in end_forms(module):
        if (prefix, tail) not in seen:
            seen.add((prefix, tail))
            options.append(dict(name=label, replace=prefix, extra_edits=[dict(
                source=module.SOURCE, find=original_tail, replace=tail)]))
    axes[0] = dict(name="selector_end_flow", source=module.SOURCE,
                   find=original_prefix, options=options)
    return axes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    module = parent()
    axes = make_axes((HOMM3_DIR / module.SOURCE).read_text())
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], axes=axes, evidence=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("generated", " x ".join(str(len(item["options"])) for item in axes), "->", args.output)


if __name__ == "__main__":
    main()
