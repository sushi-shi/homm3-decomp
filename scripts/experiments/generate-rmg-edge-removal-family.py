#!/usr/bin/env python3
"""Unsigned edge-search lifetimes around the canonical detach boundary.

Retail 0x5fd5b0 and candidate have the same 18 CFG blocks and ten branches.
The entry differs because detach's second splice is retained only by retail;
all following loop/copy structure agrees with an unsigned index, not a pointer
find. Preserve both index searches, erase APIs, twin reload and deletion order.
Vary loop structure, shared/separate indices and actual erase-argument values.
No Dreamcast counterpart exists for the Voronoi family.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def search(kind, index, target):
    condition = f"{index} < m_edges.size() && m_edges[{index}] != {target}"
    if kind == 0:
        return [f"while ({condition})", f"    ++{index};"]
    if kind == 1:
        return [f"for (; {condition}; ++{index})", "    ;"]
    if kind == 2:
        return [f"for (; {index} < m_edges.size(); ++{index}) {{",
                f"    if (m_edges[{index}] == {target})", "        break;", "}"]
    return ["for (;;) {", f"    if ({index} >= m_edges.size() || m_edges[{index}] == {target})",
            "        break;", f"    ++{index};", "}"]


def variants():
    for first, second, separate, argument in itertools.product(range(4), range(4), range(2), range(2)):
        lines = ["edge->detach();", "unsigned int index = 0;"]
        lines += search(first, "index", "edge")
        if argument:
            lines += ["{", "    std::vector<TRmgBoundaryVertex*>::iterator position = m_edges.begin() + index;",
                      "    m_edges.erase(position);", "}"]
        else:
            lines += ["m_edges.erase(m_edges.begin() + index);"]
        lines += ["TRmgBoundaryVertex* twin = edge->m_twin;"]
        index = "twinIndex" if separate else "index"
        lines += [f"{'unsigned int ' if separate else ''}{index} = 0;"]
        lines += search(second, index, "twin")
        if argument:
            lines += ["{", f"    std::vector<TRmgBoundaryVertex*>::iterator position = m_edges.begin() + {index};",
                      "    m_edges.erase(position);", "}"]
        else:
            lines += [f"m_edges.erase(m_edges.begin() + {index});"]
        lines += ["delete edge;", "delete twin;"]
        yield f"first_{first}+second_{second}+separate_{separate}+argument_{argument}", (
            "void TRmgVoronoi::removeEdge(TRmgBoundaryVertex* edge)\n{\n"
            + "\n".join("    " + line for line in lines) + "\n}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    helper = generator("generate-rmg-position-family.py")
    original = helper.definition((HOMM3_DIR / "src/rmg_support.cpp").read_text(), "TRmgVoronoi::removeEdge")
    forms = list(variants())
    if original != forms[0][1]:
        raise ValueError("review changed edge removal")
    args.output.write_text(json.dumps(dict(schema=1, units=["rmg_support"], evidence=__doc__, axes=[helper.axis(
        "edge_removal_lifetimes", "src/rmg_support.cpp", original, forms)]), indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(forms), "edge removal states")


if __name__ == "__main__":
    main()
