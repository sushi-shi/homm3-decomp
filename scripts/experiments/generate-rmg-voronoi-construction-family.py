#!/usr/bin/env python3
"""Voronoi perimeter and diagonal construction lifetimes.

Retail 0x5fd010 expands four createEdge calls, retains the fifth, and
retains only the final splice. The candidate expands all five factories,
adding an allocation arm and twelve frame bytes. Fresh retail CFG, verified
/Z7 and named call streams confirm the boundary; no Dreamcast Voronoi exists.
Preserve all canonical factories/splices, allocations and their order while
varying actual corner ownership, edge declaration lifetimes and diagonal
endpoint bindings. No artificial inline declarations or budget padding.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

FUNCTION = "TRmgVoronoi::TRmgVoronoi"


def variants():
    names = ("first", "second", "third", "fourth")
    coords = ("-200, -200", "400, -200", "400, 400", "-200, 400")
    for corners, diagonal, declarations in itertools.product(range(6), range(5), range(2)):
        lines = []
        values = list(names)
        if corners == 0:
            lines += [f"TPoint {name}({coord});" for name, coord in zip(names, coords)]
        elif corners == 1:
            values = [f"TPoint({coord})" for coord in coords]
        elif corners == 2:
            lines += [f"const TPoint& {name} = TPoint({coord});" for name, coord in zip(names, coords)]
        elif corners == 3:
            lines += [f"TPoint {name};" for name in names]
            lines += [f"{name} = TPoint({coord});" for name, coord in zip(names, coords)]
        elif corners == 4:
            lines += ["TPoint corners[4] = {" + ", ".join(f"TPoint({coord})" for coord in coords) + "};"]
            values = [f"corners[{index}]" for index in range(4)]
        if declarations:
            lines += [f"TRmgBoundaryVertex* {name}Edge;" for name in names]
        for index, name in enumerate(names):
            if corners == 5:
                if index == 0:
                    lines += [f"TPoint first({coords[0]});", f"TPoint second({coords[1]});"]
                elif index < 3:
                    lines += [f"TPoint {names[index+1]}({coords[index+1]});"]
            kind = "" if declarations else "TRmgBoundaryVertex* "
            lines += [f"{kind}{name}Edge = createEdge({values[index]}, 0, {values[(index+1)%4]}, 0);"]
        lines += [f"{names[index]}Edge->m_twin->splice({names[(index+1)%4]}Edge);" for index in range(4)]
        if diagonal == 0:
            lines += ["TRmgBoundaryVertex* diagonal = createEdge(fourthEdge->m_twin->m_sitePosition,",
                      "    fourthEdge->m_twin->m_zone, thirdEdge->m_sitePosition, thirdEdge->m_zone);"]
        elif diagonal in (1, 2):
            kind = "const TPoint&" if diagonal == 1 else "TPoint"
            lines += [f"{kind} firstSite = fourthEdge->m_twin->m_sitePosition;",
                      f"{kind} secondSite = thirdEdge->m_sitePosition;",
                      "TRmgBoundaryVertex* diagonal = createEdge(firstSite, fourthEdge->m_twin->m_zone,",
                      "    secondSite, thirdEdge->m_zone);"]
        elif diagonal == 3:
            lines += ["TRmgBoundaryVertex* fourthTwin = fourthEdge->m_twin;",
                      "TRmgBoundaryVertex* diagonal = createEdge(fourthTwin->m_sitePosition,",
                      "    fourthTwin->m_zone, thirdEdge->m_sitePosition, thirdEdge->m_zone);"]
        else:
            lines += ["const TRmgBoundaryVertex& firstSite = *fourthEdge->m_twin;",
                      "const TRmgBoundaryVertex& secondSite = *thirdEdge;",
                      "TRmgBoundaryVertex* diagonal = createEdge(firstSite.m_sitePosition,",
                      "    firstSite.m_zone, secondSite.m_sitePosition, secondSite.m_zone);"]
        lines += ["diagonal->splice(fourthEdge->m_twin->m_previous);",
                  "diagonal->m_twin->splice(thirdEdge);", "m_root = firstEdge;"]
        yield f"corners_{corners}+diagonal_{diagonal}+declarations_{declarations}", (
            "TRmgVoronoi::TRmgVoronoi()\n{\n" + "\n".join("    " + line for line in lines) + "\n}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition((HOMM3_DIR / "src/rmg_support.cpp").read_text(), FUNCTION)
    forms = list(variants())
    if original != forms[0][1]:
        raise ValueError("review changed Voronoi constructor")
    axes = [helper.axis("voronoi_construction", "src/rmg_support.cpp", original, forms)]
    args.output.write_text(json.dumps(dict(schema=1, units=["rmg_support"],
                                         evidence=__doc__, axes=axes), indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(forms), "Voronoi construction states")


if __name__ == "__main__":
    main()
