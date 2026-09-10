#!/usr/bin/env python3
"""Reply construction, lookup-local lifetime, and Complete's boolean sort flag.

DC reply constructor rows 676/678/679 prove base/position/face order, but
do not independently settle member-initializer versus body-assignment syntax.
Keep that order in all three forms. GetThisPlayer's named pPlayer result is
used only after its early local-game return (7324/7326, lookup 7329, return
7333): test its declaration at entry versus at that query. Complete narrows
sortDirection to a byte and normalizes the incoming int to truth; test a bool
field with explicit/implicit conversion, never uchar truncation of the int.
This fresh family follows the adopted access/pin search and subsequent
chat-member and host-query interface corrections; its own controls bind the
current full-build checkpoint rather than reusing stale parent objects.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

CPP = "src/singleselectionwindow.cpp"
HDR = "include/singleselectionwindow.h"
PRIV = "include/singleselectionwindow_priv.h"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    constructor = """    CRequestHeroFaceReplyMsg(int pos, int face)
        : CNetMsg(RS_REQUEST_HERO_FACE_REPLY,
                  sizeof(CRequestHeroFaceReplyMsg))
    {
        m_pos = pos;
        m_face = face;
    }"""
    both_initializers = """    CRequestHeroFaceReplyMsg(int pos, int face)
        : CNetMsg(RS_REQUEST_HERO_FACE_REPLY,
                  sizeof(CRequestHeroFaceReplyMsg)),
          m_pos(pos), m_face(face)
    {
    }"""
    position_initializer = """    CRequestHeroFaceReplyMsg(int pos, int face)
        : CNetMsg(RS_REQUEST_HERO_FACE_REPLY,
                  sizeof(CRequestHeroFaceReplyMsg)), m_pos(pos)
    {
        m_face = face;
    }"""
    lookup = """    // Before normalization (locals): pPlayer.
    CNetPlayerHandlerPlayer* player;
    if (g_unnamed6989f0 == WINDOW_MODE_6989F0_3)
        return &m_players.m_humanPlayers[0];
    player = m_players.getPlayer(g_thisNetPlayerInfo.m_dpid);
    return player;"""
    query_local = """    // Before normalization (locals): pPlayer.
    if (g_unnamed6989f0 == WINDOW_MODE_6989F0_3)
        return &m_players.m_humanPlayers[0];
    CNetPlayerHandlerPlayer* player =
        m_players.getPlayer(g_thisNetPlayerInfo.m_dpid);
    return player;"""
    query_assignment = query_local.replace(
        "CNetPlayerHandlerPlayer* player =\n        ",
        "CNetPlayerHandlerPlayer* player;\n    player = ")
    direction = "    unsigned char m_sortDirection;       // 0x36c (DC sortDirection, a byte here)"
    boolean = "    bool m_sortDirection;                // 0x36c (DC sortDirection; retail normalizes truth)"
    conversion = "        m_sortDirection = msg->m_direction != 0;"
    manifest = dict(schema=1, source=CPP,
        units=["singleselectionwindow", "advmgr", "scenarioinfo", "kb"],
        evidence=__doc__, axes=[
            dict(name="reply_field_construction", source=PRIV, find=constructor, options=[
                dict(name="body_assignments_control"),
                dict(name="both_member_initializers", replace=both_initializers),
                dict(name="position_initializer_then_face_assignment", replace=position_initializer)]),
            dict(name="lookup_result_lifetime", find=lookup, options=[
                dict(name="entry_declaration_control"),
                dict(name="query_local_initialization", replace=query_local),
                dict(name="query_local_assignment", replace=query_assignment)]),
            dict(name="sort_direction_type", source=HDR, find=direction, options=[
                dict(name="byte_explicit_truth_control"),
                dict(name="bool_explicit_truth", replace=boolean),
                dict(name="bool_implicit_truth", replace=boolean, extra_edits=[
                    dict(source=CPP, find=conversion,
                         replace="        m_sortDirection = msg->m_direction;")])]),
        ])
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(f"{args.output}: 27 source states")


if __name__ == "__main__":
    main()
