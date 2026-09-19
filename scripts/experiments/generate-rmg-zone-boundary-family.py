#!/usr/bin/env python3
"""Zone boundary construction, collection ownership and cleanup context.

Retail 0x53e050 expands the temporary Zone's entrances and boundary vector
destructors, then retains the distances destructor. After removal of the stale
user destructor declaration, the 96.3209% caller still retains boundary too.
The ordinary implicit destructor and all canonical vector operations remain
untouched. Retail's upper-bound integer spills occur inside the guarded arms;
its level checks and point creation independently consume position snapshots.

Finite family: three registry-owner forms x two allocated-zone lifetimes x
two signed-bound lifetimes x four independently scoped locate-query forms.
Every receiver, temporary and range is consumed by the real construction flow.

Native validation: test_rmg_zone_boundary.py accepts this manifest and extracts
its actual bodies. It imports point/slot declarations, constants and tables,
models opaque Zone/Voronoi behavior deterministically, and compares events with
an independent eight-direction radial-site model. The coordinator test only
mocks this procedure and is insufficient. The boundary oracle covers:
  * initial site collection, original-zone count and level filtering;
  * eight directions, x/y truncation and both signed/floating bounds tests;
  * rejected candidates produce no site/allocation/registration events;
  * each accepted surface candidate allocates its own slot and Zone, records
    the pre-append slot index, appends slot then Zone, and adds that new Zone;
  * accepted underground sites carry null and allocate/append nothing;
  * temporary Zone then slot cleanup precedes buildVertices; locate/trace,
    locate/fill and final join keep their order and original-zone snapshot.
It uses empty and mixed-level registries, zero/positive radii, narrow/non-square
maps, boundary equality and just-inside/outside cases, accepted/rejected site
patterns, surface water modes and underground levels, and append reallocation.
Constructor results are controlled explicitly: the current caller does not initialize
every slot scalar, so a fixture must not invent new gameplay initialization or
silently execute indeterminate native reads from a reduced constructor.
Meaningful failing controls: wrong level/direction step; wrong edge equality;
wrong slot index; missing slot registration; a null surface or nonnull underground
site owner; using the growing zone count instead of the original snapshot;
moving temporary cleanup after buildVertices; and wrong trace classification.
Native events cannot distinguish swapping two adjacent successful push_backs,
nor prove VC6 EH state, float rounding or inline boundaries:
review those against retail, including the retained third vector destructor.
"""
import argparse
import itertools
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator

SOURCE = "src/rmg.cpp"
NAME = "type_random_map_generator::buildZoneBoundaries"


def definition(source):
    return generator("generate-rmg-position-family.py").definition(source, NAME)


def replace(body, old, new):
    if body.count(old) != 1:
        raise ValueError("review zone-boundary source anchor: " + old)
    return body.replace(old, new)


def variant(original, registry, site_lifetime, bound_lifetime, query_scopes):
    body = original
    if registry:
        # The slot owner is used for both the assigned index and registration.
        body = replace(body, "                if (position.m_z == 0) {",
            "                if (position.m_z == 0) {\n"
            "                    std::vector<TRmgTownSlot*>& slots = mapTemplate->m_zones;")
        body = replace(body, "slot->m_zoneIndex = mapTemplate->m_zones.size();",
            "slot->m_zoneIndex = slots.size();")
        body = replace(body, "mapTemplate->m_zones.push_back(slot);",
            "slots.push_back(slot);")
    if registry == 2:
        # Keep the actual collection object, not a buffer invalidated by append.
        # Protect the separate template owner while rebinding generator uses.
        marker = "__ZONE_FAMILY_TEMPLATE_OWNER__"
        body = body.replace("mapTemplate->m_zones", marker)
        body = body.replace("m_zones", "zones")
        body = body.replace(marker, "mapTemplate->m_zones")
        body = replace(body, "    TRmgVoronoi diagram;",
            "    TRmgVoronoi diagram;\n"
            "    std::vector<TRmgZone*>& zones = m_zones;")
    if site_lifetime:
        body = replace(body, "        TRmgZone* addedZone = 0;\n", "")
        body = replace(body, "                if (position.m_z == 0) {",
            "                TRmgZone* addedZone = 0;\n"
            "                if (position.m_z == 0) {")
    if bound_lifetime:
        for coordinate, dimension, delta, local in (
                ("x", "Width", "dx", "maximumWidth"),
                ("y", "Height", "dy", "maximumHeight")):
            old = (f"                if (position.m_{coordinate} >= m_map.m_map{dimension}) {{\n"
                   f"                    if (position.m_{coordinate} >= m_map.m_map{dimension} + {delta})")
            new = (f"                if (position.m_{coordinate} >= m_map.m_map{dimension}) {{\n"
                   f"                    int {local} = m_map.m_map{dimension};\n"
                   f"                    if (position.m_{coordinate} >= {local} + {delta})")
            body = replace(body, old, new)
    if query_scopes & 1:
        body = replace(body,
            "            TRmgBoundaryVertex* first = diagram.locate(TPoint(position.m_x, position.m_y));",
            "            TRmgBoundaryVertex* first;\n"
            "            {\n"
            "                TPoint query(position.m_x, position.m_y);\n"
            "                first = diagram.locate(query);\n"
            "            }")
    if query_scopes & 2:
        body = replace(body,
            "            fillZoneArea(current, diagram.locate(TPoint(position.m_x, position.m_y)));",
            "            TRmgBoundaryVertex* first;\n"
            "            {\n"
            "                TPoint query(position.m_x, position.m_y);\n"
            "                first = diagram.locate(query);\n"
            "            }\n"
            "            fillZoneArea(current, first);")
    return body


def variants(original):
    for registry, site, bounds, queries in itertools.product(
            range(3), range(2), range(2), range(4)):
        yield dict(name=f"registry_{registry}+site_{site}+bounds_{bounds}+queries_{queries}",
                   replace=variant(original, registry, site, bounds, queries))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    # This family diagnoses the real implicit cleanup, not a missing definition.
    if "~TRmgZone();" in (HOMM3_DIR / "include/rmg.h").read_text():
        raise ValueError("establish the full implicit-Zone-destructor checkpoint first")
    original = definition((HOMM3_DIR / SOURCE).read_text())
    options = list(variants(original))
    assert len(options) == len({row["replace"] for row in options}) == 48
    assert options[0]["replace"] == original
    payload = dict(schema=1, units=["rmg", "rmg_support", "rmg_terrain"],
        evidence=__doc__, axes=[dict(name="zone_boundary_ownership_and_lifetimes",
            source=SOURCE, find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    source_families.load_manifest(args.output, HOMM3_DIR)
    print("48 Zone boundary ownership/lifetime states; validate with test_rmg_zone_boundary.py")


if __name__ == "__main__":
    main()
