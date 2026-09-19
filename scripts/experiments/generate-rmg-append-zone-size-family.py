#!/usr/bin/env python3
"""Recombine coordinate lifetimes with the canonical zone-size accessor.

The existing getSize accessor restores a vector boundary in the adjacent
position filter. Its use for the initial radius and second-ring maximum is
therefore a concrete shared source model to test with accepted-point lifetimes.
Keep scalar max, canonical max, and single-evaluation size locals as distinct
natural ownership policies, preserving the queried zones and call timing.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def options(parent):
    module = generator("generate-rmg-append-zone-family.py")
    source = (HOMM3_DIR / module.SOURCE).read_text()
    if (parent / "snapshot" / module.SOURCE).read_text() != source:
        raise ValueError("parent source identity changed")
    original = module.definition(source)
    expected = list(module.variants(original))
    manifest = json.loads((parent / "input.json").read_text())
    if manifest["axes"][0]["options"] != expected:
        raise ValueError("parent manifest identity changed")
    elites = json.loads((parent / "generation-0001.json").read_text())["elites"]
    result = [dict(name="unchanged", replace=original)]
    initial = "int radius = center->m_slot->m_size + zone->m_slot->m_size;"
    maximum = "    radius = center->m_slot->m_size;\n    if (radius < zone->m_slot->m_size)\n        radius = zone->m_slot->m_size;"
    for elite in elites:
        repeated = json.loads((parent / "candidates" / elite["id"] / "repeat/result.json").read_text())
        if repeated["scores"] != elite["scores"] or repeated["object_hash"] != elite["object_hash"]:
            raise ValueError("parent reproduction mismatch")
        original_parent = expected[elite["choices"][0]]["replace"]
        for form in ("parent", "initial_accessor", "maximum_accessor", "both_accessors",
                     "canonical_max", "local_maximum"):
            body = original_parent
            if form not in ("parent", "maximum_accessor"):
                body = module.replace(body, initial, "int radius = center->getSize() + zone->getSize();")
            if form not in ("parent", "initial_accessor"):
                if form == "canonical_max":
                    new = "    radius = max(center->getSize(), zone->getSize());"
                elif form == "local_maximum":
                    new = ("    int centerSize = center->getSize();\n"
                           "    int zoneSize = zone->getSize();\n"
                           "    radius = centerSize;\n"
                           "    if (radius < zoneSize)\n"
                           "        radius = zoneSize;")
                else:
                    new = "    radius = center->getSize();\n    if (radius < zone->getSize())\n        radius = zone->getSize();"
                body = module.replace(body, maximum, new)
            result.append(dict(name=elite["id"] + "+" + form, replace=body))
    unique = {}
    for row in result:
        unique.setdefault(row["replace"], row)
    return list(unique.values())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("parent", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    rows = options(args.parent)
    payload = dict(schema=1, units=["rmg"], evidence=__doc__, axes=[dict(
        name="append_zone_size", source="src/rmg.cpp", find=rows[0]["replace"], options=rows)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print(len(rows), "append-zone accessor states")


if __name__ == "__main__":
    main()
