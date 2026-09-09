"""Historical pre-adoption family for SavedGameHeader::reset's boundaries.

Run against the ab315284 source. Changed anchors are rejected deliberately;
the adopted source does not restore the dummy helper, unions or fences just
to replay this measurement. Its frozen snapshot preserves the old control.

The current SCampaign and NewSMapHeader declarations already have their
retail-proven implicit assignments. The old reset body pasted both member
walks, fenced three regions, and added an empty resetAssignmentSurface call
to compensate for a formerly hidden declaration surface. Test the real
assignments, normal bool-to-int flag stores, and removal of that empty call
independently. DC Reset (0xbcf00) predates these Complete copies and the
human-player loop; this family does not infer an assertion from its lines.
"""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = "src/game.cpp"
CAMPAIGN_START = "    SCampaign& savedCampaign = m_campaign;\n"
MAP_START = "    NewSMapHeader& savedMapHeader = m_mapHeader;\n"
MAP_END = "    savedMapHeader.m_availableHeroes = gameMapHeader.m_availableHeroes;\n"
HUMAN = """    int* human = m_humanPlayer;
    union {
        // Before normalization: byte.
        unsigned char m_byte;
        // Before normalization: value.
        unsigned int m_value;
    } isHuman;
    for (int i = 0; i < 8; ++i) {
        isHuman.m_byte = g_game->m_players[i].isHuman();
        *human++ = isHuman.m_value & 0xff;
    }
"""
HUMAN_BOOL = """    int* human = m_humanPlayer;
    bool isHuman;
    for (int i = 0; i < 8; ++i) {
        isHuman = g_game->m_players[i].isHuman();
        *human++ = isHuman;
    }
"""
HUMAN_DIRECT = """    int* human = m_humanPlayer;
    for (int i = 0; i < 8; ++i)
        *human++ = g_game->m_players[i].isHuman();
"""
DUMMY = """static void resetAssignmentSurface()
{
}
"""
DUMMY_CALL = "    resetAssignmentSurface();\n"


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    for anchor in (CAMPAIGN_START, MAP_START, MAP_END, HUMAN, DUMMY, DUMMY_CALL):
        if source.count(anchor) != 1:
            raise ValueError("Review changed reset source: " + anchor)
    campaign_start = source.index(CAMPAIGN_START)
    map_start = source.index(MAP_START)
    map_end = source.index(MAP_END) + len(MAP_END)
    campaign = source[campaign_start:map_start]
    map_header = source[map_start:map_end]
    return dict(schema=1, source=SOURCE, units=["game"], evidence=__doc__, axes=[
        dict(name="campaign-assignment", find=campaign, options=[
            dict(name="unchanged"),
            dict(name="canonical-references", replace=CAMPAIGN_START
                 + "    const SCampaign& gameCampaign = g_game->m_campaign;\n"
                 + "    savedCampaign = gameCampaign;\n\n"),
            dict(name="canonical-direct", replace="    m_campaign = g_game->m_campaign;\n\n")]),
        dict(name="map-header-assignment", find=map_header, options=[
            dict(name="unchanged"),
            dict(name="canonical-references", replace=MAP_START
                 + "    const NewSMapHeader& gameMapHeader = g_game->m_mapHeader;\n"
                 + "    savedMapHeader = gameMapHeader;\n"),
            dict(name="canonical-direct", replace="    m_mapHeader = g_game->m_mapHeader;\n")]),
        dict(name="human-player-value", find=HUMAN, options=[
            dict(name="unchanged"),
            dict(name="bool-local", replace=HUMAN_BOOL),
            dict(name="direct-value", replace=HUMAN_DIRECT)]),
        dict(name="empty-compiler-helper", find=DUMMY_CALL, options=[
            dict(name="unchanged"),
            dict(name="remove-empty-helper", replace="", extra_edits=[
                dict(source=SOURCE, find=DUMMY, replace="")])])])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + "\n")
