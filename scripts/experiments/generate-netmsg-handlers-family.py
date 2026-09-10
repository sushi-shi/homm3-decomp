#!/usr/bin/env python3
"""Recover DC-attested ordinary handlers in HandleNetMsg, not inline-budget padding.

Six independent groups exhaust 64 combinations. DC lines 6537/6541,
6567..6574, 6583, 6592..6603 and 6606 prove the source calls. Dossiers and
SH4 source groups prove early returns and local types; retail supplies the
Complete-specific message layouts, no-argument Update and town draw flag.
The old flattened bodies are negative controls, not preferred source models.
"""
import argparse
import json
from pathlib import Path

from homm3.core.common import HOMM3_DIR
from homm3.vc6.source_families import load_manifest

CPP = "src/singleselectionwindow.cpp"
HDR = "include/singleselectionwindow.h"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = (HOMM3_DIR / CPP).read_text()
    dispatcher = source[source.index("bool TSingleSelectionWindow::handleNetMsg("):]
    dispatcher = dispatcher[:dispatcher.index("\n}\n") + 2]
    dispatcher = dispatcher[dispatcher.index("\n    switch (netMsg->m_subType) {"):]
    axes = []

    def edit(find, replace, path=CPP):
        return dict(source=path, find=find, replace=replace)

    def arm(first, after):
        start = dispatcher.index("    case " + first)
        return dispatcher[start:dispatcher.index("    case " + after, start)]

    def definition(signature):
        # ReceiveChat has an earlier disabled carcass declaration. Its active
        # VA-owned definition is later; never replace the disabled stub.
        start = source.rindex(signature)
        return source[start:source.index("\n}", start) + 2]

    def recover_stub(old, name, body):
        stub = definition("unsigned char TSingleSelectionWindow::" + old + "(")
        return edit(stub, "#endif  // @carcass\n\n"
                    "// DC source call and local lifetimes recovered in HandleNetMsg.\n"
                    "// Before normalization: " + old + ", pNetMsg, pMsg.\n"
                    "bool TSingleSelectionWindow::" + name + "(CNetMsg* netMsg)\n{\n"
                    + body + "\n    return true;\n}\n\n#if 0  // @carcass")

    def declare(name, anchor):
        return edit(anchor, "    bool " + name + "(CNetMsg* netMsg);\n" + anchor, HDR)

    def add(name, find, replace, extras):
        axes.append(dict(name=name, find=find, options=[dict(name="flattened_control"),
                    dict(name="recovered_helpers", replace=replace, extra_edits=extras)]))

    add("header_end", arm("RS_GAME_HEADER_INFO_END:", "RS_SCROLL:"),
        "    case RS_GAME_HEADER_INFO_END:\n        onGameHeaderInfoEndMsg(netMsg);\n        break;\n", [
        recover_stub("OnGameHeaderInfoEndMsg", "onGameHeaderInfoEndMsg", """    m_receivedMaps = true;
    m_currentIndex = m_currentMap = 0;
    m_receivingMaps = false;
    if (m_chatShowing)
        getWidget(179)->show();
    m_fileSlider->setResolution(getMapCount() - g_unnamed69fdc8 + 1);
    if (m_selectionHeaders.size() > 0)
        updateGameVars();
    drawWindow(0, 0xffff0001, 0xffff);
    this->update();"""),
        declare("onGameHeaderInfoEndMsg", "    unsigned char checkMissingHeaders(unsigned long dpidHost);")])

    # SH4 line 7123 explicitly reloads the window's current index, not pMsg.
    add("scroll", arm("RS_SCROLL:", "RS_GAME_HEADER_INFO_INIT:"),
        "    case RS_SCROLL:\n        onScrollMsg(netMsg);\n        break;\n", [
        recover_stub("OnScrollMsg", "onScrollMsg", """    CScrollMsg* msg = static_cast<CScrollMsg*>(netMsg);
    m_currentIndex = msg->m_index;
    m_fileSlider->setState(m_currentIndex);
    setCurrentMap(msg->m_map, 1);"""),
        declare("onScrollMsg", "    void setFilter(int size);")])

    request = """void TSingleSelectionWindow::onRequestHeroFaceMsg(
        // Before normalization: OnRequestHeroFaceMsg, pNetMsg, pMsg, pPlayer, msgReply.
        CNetMsg* netMsg, unsigned char inPopup)
{
    // DC 7299/7300, 7306/7307 and 7310/7311 are three early returns.
    if (!isHost())
        return;
    CRequestHeroFaceMsg* msg = static_cast<CRequestHeroFaceMsg*>(netMsg);
    CNetPlayerHandlerPlayer* player = m_players.getPlayer(netMsg->m_dpidFrom);
    if (!player)
        return;
    if (player->m_playerPos == -1)
        return;
    getHeroFace(msg->m_which, player);
    CRequestHeroFaceReplyMsg reply(player->m_playerPos, player->m_heroIndex);
    transmitRemoteDataDPID(&reply, 0, false, true);
    onRequestHeroFaceReplyMsg(&reply, inPopup);
}"""
    agr = """void TSingleSelectionWindow::onSetAGRMsg(
        // Before normalization: OnSetAGRMsg, pNetMsg, pMsg, pPlayer.
        CNetMsg* netMsg, unsigned char inPopup)
{
    CSetAGRMsg* msg = static_cast<CSetAGRMsg*>(netMsg);
    CNetPlayerHandlerPlayer* player = m_players.getPlayerInPos(msg->m_gamePos);
    if (!player)
        player = m_players.getCompPlayerInPos(msg->m_gamePos);
    // DC 7364/7365 returns before the assignment, rather than nesting it.
    if (!player)
        return;
    player->m_startBonusIndex = msg->m_agr;
    if (!inPopup)
        drawHeroAdvancedOption(msg->m_gamePos, 1, -1);
}"""
    add("hero_handlers", arm("RS_REQUEST_HERO_FACE:", "RS_NEW_HOST:"), """    case RS_REQUEST_HERO_FACE:
        onRequestHeroFaceMsg(netMsg, 0);
        break;
    case RS_REQUEST_HERO_FACE_REPLY:
        onRequestHeroFaceReplyMsg(netMsg, 0);
        break;
    case RS_SETAGR:
        onSetAGRMsg(netMsg, 0);
        break;
""", [edit(definition("inline void TSingleSelectionWindow::onRequestHeroFaceMsg("), request),
          edit(definition("inline void TSingleSelectionWindow::onSetAGRMsg("), agr)])

    chat = """void TSingleSelectionWindow::receiveChat(
    // Before normalization: ReceiveChat, cChat, pPlayer.
    unsigned long dpid, char* chat, unsigned char inPopup)
{
    CNetPlayerInfo* player = m_players.getPlayer(dpid);
    // DC 7216/7217 is an early return with a CNetPlayerInfo* local.
    if (!player)
        return;
    addChat(&g_chatMan,
            DATA_COMPGEN(0x00682ab4, chatPlayerLineFormat, "%s: %s"),
            player->m_name, chat);
    if (!inPopup)
        displayChat();
}"""
    add("chat", arm("RS_CHAT_MSG:", "RS_MAP_FILE_NAME:"), """    case RS_CHAT_MSG: {
        CChatMsg* msg = static_cast<CChatMsg*>(netMsg);
        receiveChat(msg->m_dpidFrom, msg->m_text, 0);
        break;
    }
""", [edit(definition("void TSingleSelectionWindow::receiveChat("), chat)])

    add("header_requests", arm("RS_HEADER_CONFIRM:", "RS_CLICK:"), """    case RS_HEADER_CONFIRM:
        onHeaderConfirmMsg(netMsg);
        break;
    case RS_REQ_HEADER_CONFIRM:
        onReqHeaderConfirmMsg(netMsg);
        break;
    case RS_MAP_HEADER_REQUEST:
        onMapHeaderRequestMsg(netMsg);
        break;
""", [
        recover_stub("OnHeaderConfirmMsg", "onHeaderConfirmMsg", """    m_newPlayerUpdateMan->headerConfirmed(netMsg->m_dpidFrom);
    g_logFile.log(DATA_COMPGEN(0x006838c0, headerConfirmedLog,
                    "Header confirmed [%d]"), netMsg->m_dpidFrom);"""),
        recover_stub("OnReqHeaderConfirmMsg", "onReqHeaderConfirmMsg", """    if (!checkMissingHeaders(netMsg->m_dpidFrom)) {
        g_logFile.log(DATA_COMPGEN(0x006838ac, noMissingHeadersLog,
                        "No Missing Headers"));
        // DC 6715 constructs and sends the temporary in one expression.
        transmitRemoteDataDPID(&CHeaderConfirmMsg(), netMsg->m_dpidFrom,
                               false, true);
    }"""),
        recover_stub("OnMapHeaderRequestMsg", "onMapHeaderRequestMsg", """    CMapHeaderRequestMsg* msg = static_cast<CMapHeaderRequestMsg*>(netMsg);
    m_newPlayerUpdateMan->headerRequested(netMsg->m_dpidFrom,
                                         msg->m_flag, msg->m_number);
    g_logFile.log(DATA_COMPGEN(0x0068388c, headerRequestedLog,
                    "Header requested [%d] [%d] [%d]"),
                 netMsg->m_dpidFrom, msg->m_flag, msg->m_number);"""),
        edit("    void displayChat();", """    bool onHeaderConfirmMsg(CNetMsg* netMsg);
    bool onReqHeaderConfirmMsg(CNetMsg* netMsg);
    bool onMapHeaderRequestMsg(CNetMsg* netMsg);
    void displayChat();""", HDR)])

    add("click_town", arm("RS_CLICK:", "RS_LAUNCHING_GAME:"), """    case RS_CLICK:
        onClickMsg(netMsg);
        break;
    case RS_TOWN_UPDATE:
        onTownUpdateMsg(netMsg, 0);
        break;
""", [recover_stub("OnClickMsg", "onClickMsg", """    CClickMsg* clickMsg = static_cast<CClickMsg*>(netMsg);
    lobby_message msg;
    unsigned char exitFlag;
    msg.m_codeY = clickMsg->m_widgetId;
    onWidgetDeselect(&msg, &exitFlag, 1);"""),
        declare("onClickMsg", "    void makeHeroFilter();"),
        edit("inline void TSingleSelectionWindow::onTownUpdateMsg(",
             "void TSingleSelectionWindow::onTownUpdateMsg(")])

    manifest = dict(schema=1, source=CPP,
                    units=["advmgr", "kb", "scenarioinfo", "singleselectionwindow"], axes=axes)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
    load_manifest(args.output, HOMM3_DIR)
    print(f"{args.output}: six binary groups, 64 source states")


if __name__ == "__main__":
    main()
