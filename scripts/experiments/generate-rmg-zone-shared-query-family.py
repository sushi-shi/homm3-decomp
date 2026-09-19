#!/usr/bin/env python3
"""Share the consumed locate-query value across the two zone traversals.

Two separate default TPoint constructors still count as two VC6 sites; the
prior family raises nested cleanup budget175 to181, below the second vector's
required94 after the first94. A single shared query has a meaningful lifetime
covering both traversals. It is passed by value, assigned before every use,
and has an empty constructor/trivial destructor. Compare after-buildVertices
and function-entry lifetimes, with real XY/YX store order. Preserve all zone
position rereads. Normalizing the replaced per-query scope axis leaves three
distinct reproduced ownership/bounds parents: twelve forms plus unchanged.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

previous = generator("generate-rmg-zone-query-construction-family.py")
parent = previous.parent


def variant(body, early, order):
    scoped = ("            TRmgBoundaryVertex* first;\n"
              "            {\n"
              "                TPoint query(position.m_x, position.m_y);\n"
              "                first = diagram.locate(query);\n"
              "            }")
    direct = "            TRmgBoundaryVertex* first = diagram.locate(TPoint(position.m_x, position.m_y));"
    fill = "            fillZoneArea(current, diagram.locate(TPoint(position.m_x, position.m_y)));"
    # These local query lifetimes are replaced by one real shared lifetime.
    body = body.replace(scoped, direct)
    body = body.replace(direct + "\n            fillZoneArea(current, first);", fill)
    if early:
        body = parent.replace(body, "    TRmgVoronoi diagram;",
                              "    TPoint query;\n    TRmgVoronoi diagram;")
    else:
        body = parent.replace(body, "    diagram.buildVertices();",
                              "    diagram.buildVertices();\n    TPoint query;")
    setup = "".join(f"            query.m_{axis} = position.m_{axis};\n" for axis in order)
    body = parent.replace(body, direct,
        setup + "            TRmgBoundaryVertex* first = diagram.locate(query);")
    return parent.replace(body, fill, setup + "            fillZoneArea(current, diagram.locate(query));")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    original = parent.definition((HOMM3_DIR / parent.SOURCE).read_text())
    retained = list(previous.parents(args.checkpoint))
    unique = {original: dict(name="unchanged", replace=original)}
    for row, body in retained:
        for early in (False, True):
            for order in ("xy", "yx"):
                form = variant(body, early, order)
                unique.setdefault(form, dict(name=f"{row['id']}+early_{int(early)}+{order}", replace=form))
    if len(unique) != 13:
        raise ValueError(f"expected thirteen distinct shared-query states, got {len(unique)}")
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"], evidence=__doc__,
        parent_checkpoint=str(args.checkpoint.resolve()),
        reproduced_parents=[dict(id=r["id"], object_hash=r["object_hash"]) for r, _ in retained],
        axes=[dict(name="zone_shared_query", source=parent.SOURCE,
                   find=original, options=list(unique.values()))])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("13 distinct shared-query states from ten reproduced parents")


if __name__ == "__main__":
    main()
