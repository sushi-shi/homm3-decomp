"""Recover DC-proven lobby player helpers and reprice three existing pins.

DC HandleNetMsg line 6488 calls GetThisPlayer twice with short circuiting;
line 6511 calls ordinary bool OnPlayerDroppedMsg, defined at line 6937.
Complete retains the two inner GetPlayer calls at +0x141/+0x160 and expands
the drop helper at +0x21e..+0x2ae. Complete also recomputes common version
after deleting the player and uses no-argument Update (no DC message local).
Cross both source recoveries with the remaining HeaderRequested auto-inline
override; all eight corners and every header consumer are measured.
"""

import argparse
import json
from pathlib import Path
import runpy

from homm3.core.common import HOMM3_DIR


SOURCE = "src/singleselectionwindow.cpp"
TRANSFER = """        // DC line 6488 evaluates the ordinary GetThisPlayer helper twice.
        if (getThisPlayer() && getThisPlayer()->m_playerPos == -1) {
            destroyMsg(netMsg);
            remoteCleanup();
            cancel = true;
            normalDialog(g_generalText->getText(525), 1, -1, -1,
                         -1, 0, -1, 0, -1, 0, -1, 0);
            return 1;
        }
"""
DROP = """    case RS_PLAYER_DROPPED:
        onPlayerDroppedMsg(netMsg);
        break;
"""
BODY = """#endif  // @carcass

// DC HandleNetMsg calls this ordinary bool helper; its pPlayer local is
// CNetPlayerInfo*, not the derived handler record. Complete's expansion at
// HandleNetMsg +0x21e recomputes the version after DeletePlayer and calls
// no-argument Update: the older DC message junk local does not survive.
// Before normalization: OnPlayerDroppedMsg, pNetMsg, pPlayer.
// E:\\gamedcs\\singleselectionwindow.cpp:6937
DC_ONLY(0x140c88, 0xC6)
bool TSingleSelectionWindow::onPlayerDroppedMsg(CNetMsg* netMsg)
{
    CNetPlayerInfo* player = m_players.getPlayer(netMsg->m_dpidFrom);
    m_players.deletePlayer(netMsg->m_dpidFrom);
    m_commonGameVersion = getCommonGameVersion();
    m_newPlayerUpdateMan->playerDropped(netMsg->m_dpidFrom);
    if (player)
        playerDropMsg(&g_chatMan, g_generalText->getText(527), player->m_name);
    updateNameLists();
    displayChat();
    drawWindow(0, 0xffff0001, 0xffff);
    this->update();
    return true;
}
"""
DECL = """    // DC ordinary OnPlayerDroppedMsg, line 6937; QAA_N return.
    bool onPlayerDroppedMsg(CNetMsg* netMsg);
"""


def make_manifest():
    source = (HOMM3_DIR / SOURCE).read_text()
    transfer_start = source.index("        // GetThisPlayer open-coded twice:")
    transfer_end = source.index("        if (onGameTransmitInitMsg(netMsg))", transfer_start)
    drop_start = source.index("    case RS_PLAYER_DROPPED: {")
    drop_end = source.index("    case RS_SET_AS_HOST:", drop_start)
    stub_start = source.index("// E:\\gamedcs\\singleselectionwindow.cpp:6937")
    stub_end = source.index("#endif  // @carcass", stub_start) + len("#endif  // @carcass\n")
    pin_start = source.index("// auto_inline(off): retail CALLS this from HandleNetMsg's request arm;")
    pin_end = source.index("#pragma auto_inline(on)", pin_start) + len("#pragma auto_inline(on)")
    pin = source[pin_start:pin_end]
    natural = pin[pin.index("// E:\\gamedcs\\singleselectionwindow.cpp:1492"):]
    natural = natural.replace("\n#pragma auto_inline(on)", "")
    consumers = runpy.run_path(str(Path(__file__).with_name("generate-lobby-map-header-family.py")))["consumers"]
    return {"schema": 1, "source": SOURCE, "units": consumers(), "evidence": __doc__, "axes": [
        {"name": "transfer-player-helper", "find": source[transfer_start:transfer_end], "options": [
            {"name": "flattened-two-pin-control"},
            {"name": "ordinary-twice-queried-helper", "replace": TRANSFER},
        ]},
        {"name": "dropped-player-helper", "find": source[drop_start:drop_end], "options": [
            {"name": "flattened-four-call-pin-control"},
            {"name": "ordinary-drop-helper", "replace": DROP, "extra_edits": [
                {"source": SOURCE, "find": source[stub_start:stub_end], "replace": BODY},
                {"source": "include/singleselectionwindow.h", "insert_before":
                 "    // DC ordinary OnNewMapHeaderInfo, source line 6968; QAA_N return.", "text": DECL},
            ]},
        ]},
        {"name": "header-requested-override", "find": pin, "options": [
            {"name": "current-auto-inline-control"},
            {"name": "natural-header-requested", "replace": natural},
        ]},
    ]}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    payload = make_manifest()
    args.output.write_text(json.dumps(payload, indent=2) + "\n")
    print("8 source states;", len(payload["units"]), "header consumers:", ", ".join(payload["units"]))
