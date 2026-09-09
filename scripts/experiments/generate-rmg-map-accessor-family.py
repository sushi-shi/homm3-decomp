#!/usr/bin/env python3
"""Canonical scalar map lookup: row/index lifetimes and operand order.

At commitTreasureGroup 0x5469b0, the verified /Z7 comparison first differs
inside the scalar lookup: retail loads width then multiplies by y at +0x16a,
while the candidate loads y then multiplies by width. All 45 blocks and all
three calls agree. No Dreamcast GetMapItem/commitTreasureGroup counterpart
was found. Keep the canonical inline signature, full row-major semantics,
and all caller boundaries; measure the seven consumers of the owning header.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

UNITS = ["rmg", "rmg_support", "rmg_terrain", "tiles",
         "singleselectionpopups", "singleselectionwindow", "scenarioinfo"]
SIGNATURE = "    inline TRmgMapItem* getMapItem(int x, int y, int z)"


def definition(header):
    start = header.index(SIGNATURE)
    end = header.index("\n    }", start) + len("\n    }")
    return header[start:end]


def variants(original):
    for lifetime, multiply, row_order, result in itertools.product(
            range(5), range(2), range(2), range(3)):
        row_expr = "z * m_mapHeight + y" if not row_order else "y + z * m_mapHeight"
        setup = []
        if lifetime == 0:
            row = "(" + row_expr + ")"
        elif lifetime == 1:
            setup = [f"const int row = {row_expr};"]
            row = "row"
        elif lifetime == 2:
            setup = ["int row;", f"row = {row_expr};"]
            row = "row"
        elif lifetime == 3:
            setup = (["int row = z * m_mapHeight;", "row += y;"]
                     if not row_order else ["int row = y;", "row += z * m_mapHeight;"])
            row = "row"
        else:
            setup = [f"const int row = {row_expr};"]
            row = "row"
        product = f"{row} * m_mapWidth" if not multiply else f"m_mapWidth * {row}"
        if lifetime == 4:
            setup += [f"const int index = {product} + x;"]
            index = "index"
        else:
            index = product + " + x"
        if result == 0:
            setup += [f"return m_mapItems + {index};"]
        elif result == 1:
            setup += [f"return &m_mapItems[{index}];"]
        else:
            setup += [f"TRmgMapItem* item = m_mapItems + {index};", "return item;"]
        body = SIGNATURE + "\n    {\n" + "\n".join("        " + line for line in setup) + "\n    }"
        yield f"row_{lifetime}+multiply_{multiply}+sum_{row_order}+result_{result}", body


def make_axes(header):
    original = definition(header)
    forms = list(variants(original))
    if forms[0][1] != original:
        raise ValueError("review changed canonical accessor before rebasing")
    return [generator("generate-rmg-position-family.py").axis(
        "canonical_map_lookup", "include/rmg.h", original, forms)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = dict(schema=1, units=UNITS, evidence=__doc__,
                   axes=make_axes((HOMM3_DIR / "include/rmg.h").read_text()))
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(payload["axes"][0]["options"]), "canonical accessor states")


if __name__ == "__main__":
    main()
