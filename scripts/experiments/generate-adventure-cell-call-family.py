"""Recover adventure drawing's canonical GetCell and boat accessor calls.

DC GetCell (0x14b90), advmgr.cpp:7027..7029, tests point validity and calls
NewfullMap::cell(0,0,0) or cell(point). Six drawing callers name that ordinary
member, not the three Draw*Cell copies currently in this TU. Boat drawing
also calls game::GetBoat and type_obscuring_object::get_location (5864/5869
and 5901/5906). Retail proves the same indexing/location semantics, but the
current hero lookup retains two separate helper calls where retail merges
one, and the boat invalid-point arm expands a call retail retains.

Cross these source boundaries with deletion of the existing CheckDimNextHeroBut
auto-inline fence. DC gives it no leading gap or local supporting invented
compiler work. No new helper, inline annotation, assertion or pragma is added.
All tracked advmgr functions are scored, including callers outside drawing.

Historical pre-adoption control: use the e4650642 source or the frozen
76b71ba668f31dcdfd1d snapshot. Changed source anchors deliberately fail.
"""

import argparse
import itertools
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = "src/advmgr.cpp"
COPIES_START = "// Before normalization (function): DrawHeroCell.\n"
COPIES_END = "// E:\\gamedcs\\advmgr.cpp:5688\n"
DRAW_END = "#if 0  // @carcass\n\n// E:\\gamedcs\\advmgr.cpp:7019\n"
GET_CELL = """NewmapCell* advManager::getCell(type_point point)
{
    if (!point.isValid())
        return m_fullMap->m_cellData;
    return &m_fullMap->m_cellData[(point.m_z * m_fullMap->m_size + point.m_y)
                              * m_fullMap->m_size + point.m_x];
}"""
CANONICAL_CELL = """NewmapCell* advManager::getCell(type_point point)
{
    if (!point.isValid())
        return m_fullMap->cell(0, 0, 0);
    return m_fullMap->cell(point);
}"""
DIM = """#pragma auto_inline(off)
void advManager::checkDimNextHeroBut()
{
    if (g_currentPlayer->isLocalHuman() && g_currentPlayer->hasMobileHero())
        m_advWindow->widgetClearStatus(11, widget::WIDGET_UPDATE | widget::WIDGET_DIMMED);
    else
        m_advWindow->widgetSetStatus(11, widget::WIDGET_UPDATE | widget::WIDGET_DIMMED);
}
#pragma auto_inline(on)"""


def replace(source, before, after, count=1):
    if source.count(before) != count:
        raise ValueError("Review changed source anchor: " + before)
    return source.replace(before, after)


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    for anchor in (COPIES_START, COPIES_END, DRAW_END, GET_CELL, DIM):
        if source.count(anchor) != 1:
            raise ValueError("Review changed source anchor: " + anchor)
    start = source.index(COPIES_START)
    copies = source[start:source.index(COPIES_END, start)]
    drawing = source[start:source.index(DRAW_END, start)]
    options = []
    for calls, boat, location in itertools.product(range(2), repeat=3):
        candidate = drawing
        if calls:
            candidate = replace(candidate, copies, "")
            candidate = replace(candidate, "drawHeroCell(this, ", "getCell(", 3)
            candidate = replace(candidate, "drawBoatCell(\n        this, ",
                                "getCell(\n        ", 2)
            candidate = replace(candidate, "drawGroundCell(\n        this, ",
                                "getCell(\n        ")
        if boat:
            candidate = replace(candidate, "&g_game->m_boats[boatParts.m_id]",
                                "g_game->getBoat(boatParts.m_id)", 2)
        if location:
            candidate = replace(candidate,
                                "type_point(currBoat->m_x, currBoat->m_y, currBoat->m_z)",
                                "currBoat->getLocation()", 2)
        option = dict(name=f"get-cell-{calls}-get-boat-{boat}-get-location-{location}")
        if candidate != drawing:
            option["replace"] = candidate
        options.append(option)
    return dict(schema=1, source=SOURCE, units=["advmgr"], evidence=__doc__, axes=[
        dict(name="get-cell-body", find=GET_CELL, options=[
            dict(name="unchanged"),
            dict(name="canonical-map-wrappers", replace=CANONICAL_CELL)]),
        dict(name="drawing-call-boundaries", find=drawing, options=options),
        dict(name="next-hero-inline-policy", find=DIM, options=[
            dict(name="unchanged"),
            dict(name="remove-existing-fence", replace=DIM.replace(
                "#pragma auto_inline(off)\n", "").replace(
                    "\n#pragma auto_inline(on)", ""))])])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
