#!/usr/bin/env python3
"""Edge-factory pointer ownership and public insertion boundaries.

At retail 0x5fd390 allocation and paired initialization agree, but the
second insertion retains four vector::size calls instead of the candidate's
three; the twin pointer occupies a dead parameter home instead of a local.
Keep the ordinary paired constructor and owning vector, varying actual
pointer-result lifetimes, twin capture and public insertion operations.
No Dreamcast counterpart exists for this Complete-only Voronoi family.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants(original):
    prefix = original[:original.index("\n{")+2]
    for result, twin, timing, insertion in itertools.product(range(4), range(3), range(2), range(3)):
        expression = "new TRmgBoundaryVertex(first, firstZone, second, secondZone)"
        if result == 0:
            lines = [f"TRmgBoundaryVertex* edge = {expression};"]
        elif result == 1:
            lines = ["TRmgBoundaryVertex* edge;", f"edge = {expression};"]
        elif result == 2:
            lines = [f"TRmgBoundaryVertex* const edge = {expression};"]
        else:
            lines = [f"TRmgBoundaryVertex* const& edge = {expression};"]
        kind = ("TRmgBoundaryVertex*", "TRmgBoundaryVertex* const", "TRmgBoundaryVertex* const&")[twin]
        capture = f"{kind} twin = edge->m_twin;"
        first = "m_edges.push_back(edge);" if insertion != 2 else "m_edges.insert(m_edges.end(), edge);"
        second = "m_edges.push_back(twin);" if insertion != 1 else "m_edges.insert(m_edges.end(), twin);"
        lines += [first, capture] if not timing else [capture, first]
        lines += [second, "return edge;"]
        yield f"result_{result}+twin_{twin}+timing_{timing}+insert_{insertion}", prefix + "\n" + "\n".join(
            "    " + line for line in lines) + "\n}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition((HOMM3_DIR / "src/rmg_support.cpp").read_text(), "TRmgVoronoi::createEdge")
    forms = list(variants(original))
    if original != forms[0][1]:
        raise ValueError("review changed factory body")
    payload = dict(schema=1, units=["rmg_support"], evidence=__doc__, axes=[helper.axis(
        "edge_factory_ownership", "src/rmg_support.cpp", original, forms)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(forms), "edge factory states")


if __name__ == "__main__":
    main()
