#!/usr/bin/env python3
"""Test whether the map's three signed dimensions form a coordinate member.

The map owns width, height and level count as consecutive signed dwords at
0xc, 0x10 and 0x14. Its exact constructor writes those coordinates before the
array allocation. A three-coordinate member is a plausible source ownership
model using the existing TRmgMapPosition type, whose constructor now lives in
its evidenced owning TU. Compare direct fields with that member and genuine
body-store, member-initializer and assigned-value construction at the owning
and buffer-view constructors. Keep every extent expression, cell access,
layout, parameter ABI and ordinary helper definition. Score all seven header
consumers. Contiguity alone does not establish the aggregate relationship.
"""
import argparse
import itertools
import json
import re
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest


def make_constructor(text, signature, level, style):
    start = text.index(signature)
    opening = text.index("{", start)
    indent = "        " if signature.lstrip().startswith("inline") else "    "
    close = text.index("\n    }" if indent == "        " else "\n}", opening)
    end = close + (6 if indent == "        " else 2)
    original = text[start:end]
    body = original
    values = (("x", "width"), ("y", "height"), ("z", level))
    if style != "fields":
        for field, value in values:
            old = indent + "m_size.m_" + field + " = " + value + ";\n"
            assert old in body
            new = ""
            if style == "assignment" and field == "x":
                new = indent + "m_size = TRmgMapPosition(width, height, " + level + ");\n"
            body = body.replace(old, new)
    if style == "initializer":
        body = body.replace(signature, signature + "\n" + indent + ": m_size(width, height, " + level + ")", 1)
    return text.replace(original, body, 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    header = (HOMM3_DIR / "include/rmg.h").read_text()
    first = header.index("    // Before normalization: mapWidth, mapHeight, numberLevels.")
    last = header.index("\n\n    // Owning constructor", first)
    aggregate = header[first:last]
    fields = ("    int m_mapWidth;\n    int m_mapHeight;\n    int m_numberLevels;")
    direct_header = header.replace(aggregate, fields, 1)
    direct_source = source
    # Undo only this model's member paths; painter m_size members are unrelated.
    for old, new in (("m_mapWidth", "m_size.m_x"), ("m_mapHeight", "m_size.m_y"), ("m_numberLevels", "m_size.m_z")):
        direct_header = direct_header.replace(new, old)
        direct_source = direct_source.replace(new, old)
    binding = "    const TRmgMapPosition& dimensions = m_map.m_size;\n"
    assert direct_source.count(binding) == 1
    direct_source = direct_source.replace(binding, "")
    for coordinate, field in (("x", "m_mapWidth"), ("y", "m_mapHeight"), ("z", "m_numberLevels")):
        direct_source = direct_source.replace("dimensions.m_" + coordinate, "m_map." + field)
    mapped_header = direct_header.replace(fields, aggregate, 1)
    mapped_source = direct_source
    for old, new in (("m_mapWidth", "m_size.m_x"), ("m_mapHeight", "m_size.m_y"), ("m_numberLevels", "m_size.m_z")):
        mapped_header = re.sub(r"\b" + old + r"\b", new, mapped_header)
        mapped_source = re.sub(r"\b" + old + r"\b", new, mapped_source)
    owned_signature = "type_random_map::type_random_map(int width, int height, int levels)"
    view_signature = "    inline type_random_map(TRmgMapItem* items, int width, int height)"
    options = [{"name": "adopted_dimension_reference_control", "replace": source},
               {"name": "direct_dimensions_control", "replace": direct_source,
                "extra_edits": [{"source": "include/rmg.h", "find": header, "replace": direct_header}]}]
    for owned, view in itertools.product(("fields", "initializer", "assignment"), repeat=2):
        body = make_constructor(mapped_source, owned_signature, "levels", owned)
        declaration = make_constructor(mapped_header, view_signature, "1", view)
        options.append({"name": "dimension_value+owned_" + owned + "+view_" + view, "replace": body,
                        "extra_edits": [{"source": "include/rmg.h", "find": header, "replace": declaration}]})
    payload = {"schema": 1, "source": "src/rmg.cpp", "evidence": __doc__,
               "units": ["rmg", "rmg_support", "rmg_terrain", "tiles", "singleselectionpopups",
                         "singleselectionwindow", "scenarioinfo"],
               "axes": [{"name": "map_dimensions", "find": source, "options": options}]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("11 map-dimension ownership/construction states, including adopted reference control")


if __name__ == "__main__":
    main()
