#!/usr/bin/env python3
"""Preserve the position lookup's canonical delegation with projected locals.

Retail 0x5378e0 keeps a 39-byte wrapper; monolith retains that wrapper in
both guard placements. Field/getter projection controls preserve this body,
but their direct expressions still expand it into the scalar overload.
Test real scalar projection lifetimes and a named result at this wrapper,
using fields, value accessors or borrowed accessors consistently. The three
input coordinates are used once, there is no added coordinate copy, and both
map overload interfaces and the single scalar indexing formula are unchanged.
No Dreamcast declaration survives for this Complete-only type. Getter names
and local bindings are source hypotheses; all seven consumers are scored.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest
from homm3.vc6.test_rmg_families import generator


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    header = (HOMM3_DIR / "include/rmg.h").read_text()
    source = (HOMM3_DIR / "src/rmg.cpp").read_text()
    original = generator("generate-rmg-position-family.py").definition(
        source, "type_random_map::getMapItem", parameters="TRmgMapPosition point")
    signature = original[:original.index("\n{")]
    end = header.index("\n};", header.index("struct TRmgMapPosition {"))
    options = []
    for projection in ("fields", "values", "references"):
        declaration = header
        if projection != "fields":
            returned = "int" if projection == "values" else "const int&"
            methods = "\n" + "\n".join("    " + returned + " get" + axis.upper() + "() const { return m_" + axis + "; }" for axis in "xyz")
            declaration = header[:end] + methods + header[end:]
        for binding in ("expression", "scalars", "constants", "references"):
            for result in ("direct", "named"):
                projected = ["point.m_" + axis if projection == "fields" else "point.get" + axis.upper() + "()" for axis in "xyz"]
                lines = []
                if binding != "expression":
                    returned = {"scalars": "int", "constants": "const int", "references": "const int&"}[binding]
                    lines = ["    " + returned + " " + axis + " = " + value + ";" for axis, value in zip("xyz", projected)]
                    projected = list("xyz")
                call = "getMapItem(" + ", ".join(projected) + ")"
                lines += (["    return " + call + ";"] if result == "direct" else
                          ["    TRmgMapItem* item = " + call + ";", "    return item;"])
                body = signature + "\n{\n" + "\n".join(lines) + "\n}"
                option = dict(name=projection+"+"+binding+"+"+result, replace=body)
                if declaration != header:
                    option["extra_edits"] = [dict(source="include/rmg.h", find=header, replace=declaration)]
                options.append(option)
    assert options[0]["replace"] == original
    payload = dict(schema=1, source="src/rmg.cpp", evidence=__doc__,
                   units=["rmg", "rmg_support", "rmg_terrain", "tiles", "singleselectionpopups", "singleselectionwindow", "scenarioinfo"],
                   axes=[dict(name="position_projection", find=original, options=options)])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2)+"\n")
    load_manifest(args.output,HOMM3_DIR)
    print("24 canonical lookup projection/lifetime controls")


if __name__ == "__main__":
    main()
