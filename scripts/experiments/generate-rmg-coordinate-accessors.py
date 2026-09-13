#!/usr/bin/env python3
"""Review coordinate projections in the canonical three-dimensional map lookup.

The ordinary position lookup retained at 0x5378e0 delegates to the canonical
scalar formula. Inside monolith's placeGuard it expands at cost 41 against
budgets 70/88, while retail retains that overload. Coordinate accessors are
already evidenced for the adjacent TPoint and grid-point models; test their
same value/borrowed projection interface on the three-coordinate type, without
changing its layout, constructor, special members or the map overload ABIs.
Cross point-argument and map-dimension projection uses, retaining a declaration-
only control. The position declaration is Complete-only, so the API name and
return ownership remain hypotheses. No wrapper, repeated read or dummy work
is introduced; both retained lookup bodies and all seven consumers must agree.
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
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition(source, "type_random_map::getMapItem", parameters="TRmgMapPosition point")
    position_start = header.index("struct TRmgMapPosition {")
    position_end = header.index("\n};", position_start)
    scalar = generator("generate-rmg-map-accessor-family.py").definition(header)
    options = [dict(name="current_control", replace=original)]
    for borrowed in (False, True):
        returned = "const int&" if borrowed else "int"
        accessors = "\n" + "\n".join("    " + returned + " get" + axis.upper() + "() const { return m_" + axis + "; }" for axis in "xyz")
        declaration = header[:position_end] + accessors + header[position_end:]
        for projections in ("none", "point", "dimensions", "both"):
            body = original
            scalar_body = scalar
            if projections in ("point", "both"):
                for axis in "xyz":
                    body = body.replace("point.m_" + axis, "point.get" + axis.upper() + "()")
            if projections in ("dimensions", "both"):
                for axis in "xy":
                    scalar_body = scalar_body.replace("m_size.m_" + axis, "m_size.get" + axis.upper() + "()")
            changed_header = declaration.replace(scalar, scalar_body, 1)
            options.append(dict(name=("borrowed" if borrowed else "value") + "+" + projections,
                                replace=body, extra_edits=[dict(source="include/rmg.h", find=header, replace=changed_header)]))
    payload = dict(schema=1, source="src/rmg.cpp", evidence=__doc__,
                   units=["rmg", "rmg_support", "rmg_terrain", "tiles", "singleselectionpopups", "singleselectionwindow", "scenarioinfo"],
                   axes=[dict(name="coordinate_projections", find=original, options=options)])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print("9 coordinate projection ownership/use controls")


if __name__ == "__main__":
    main()
