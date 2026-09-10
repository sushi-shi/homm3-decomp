#!/usr/bin/env python3
"""Retail videoPlay's adjacent global coordinates as one POINT owner."""
import argparse
import json
from pathlib import Path


p = argparse.ArgumentParser(description=__doc__)
p.add_argument("output", type=Path)
args = p.parse_args()

root = Path(__file__).resolve().parents[2]
source = (root / "src/smackmgr.cpp").read_text()

x_definition = "DATA(0x0069fe00) int g_smackX;                      // blit origin on the screen bitmap"
y_definition = "DATA(0x0069fe04) int g_smackY;"
assert source.count(x_definition) == 1
assert source.count(y_definition) == 1

coordinates = """            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackX = pos.x;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackY = pos.y;"""
assert source.count(coordinates) == 1


def point_source(replacement):
    changed = source.replace(
        x_definition,
        "DATA(0x0069fe00) POINT g_smackPosition;                   // blit origin on the screen bitmap")
    changed = changed.replace(y_definition, "")
    changed = changed.replace("g_smackX", "g_smackPosition.x")
    changed = changed.replace("g_smackY", "g_smackPosition.y")
    point_coordinates = coordinates.replace("g_smackX", "g_smackPosition.x").replace(
        "g_smackY", "g_smackPosition.y")
    assert changed.count(point_coordinates) == 1
    return changed.replace(point_coordinates, replacement)


forms = [
    ("separate-global-control", source),
    ("point-local-first", point_source(coordinates.replace(
        "g_smackX", "g_smackPosition.x").replace("g_smackY", "g_smackPosition.y"))),
    ("point-global-first-members", point_source("""            g_smackPosition.x = x + (vw - g_smackVideo->m_width) / 2;
            pos.x = g_smackPosition.x;
            g_smackPosition.y = y + (vh - g_smackVideo->m_height) / 2;
            pos.y = g_smackPosition.y;""")),
    ("point-both-globals-then-members", point_source("""            g_smackPosition.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackPosition.y = y + (vh - g_smackVideo->m_height) / 2;
            pos.x = g_smackPosition.x;
            pos.y = g_smackPosition.y;""")),
    ("point-both-globals-then-aggregate", point_source("""            g_smackPosition.x = x + (vw - g_smackVideo->m_width) / 2;
            g_smackPosition.y = y + (vh - g_smackVideo->m_height) / 2;
            pos = g_smackPosition;""")),
    ("point-both-locals-then-aggregate", point_source("""            pos.x = x + (vw - g_smackVideo->m_width) / 2;
            pos.y = y + (vh - g_smackVideo->m_height) / 2;
            g_smackPosition = pos;""")),
    ("point-chained-local-global", point_source("""            pos.x = g_smackPosition.x = x + (vw - g_smackVideo->m_width) / 2;
            pos.y = g_smackPosition.y = y + (vh - g_smackVideo->m_height) / 2;""")),
]

options = []
for name, changed in forms:
    option = {"name": name}
    if changed != source:
        option["replace"] = changed
    options.append(option)

args.output.write_text(json.dumps({
    "schema": 1,
    "unit": "smackmgr",
    "function": "?videoPlay@@YIHHHHHH@Z",
    "axes": [{"name": "global-coordinate-owner", "find": source, "options": options}],
    "evidence": [
        "Dreamcast's platform-specific body and globals do not identify Complete's coordinate owner; retail is authoritative.",
        "Retail places the two coordinate dwords contiguously at 0x69fe00 and 0x69fe04, consistent with one POINT as well as two ints.",
        "Retail writes both globals, keeps their computed values in EAX/ECX, and then homes those values at ebp-c/ebp-8 before SmackToBuffer.",
        "Test the natural aggregate-copy boundary and memberwise controls while preserving all values, calls, branches, and SDK interfaces.",
    ],
}, indent=2) + "\n")
