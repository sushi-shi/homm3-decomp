// The turn-start network broadcast shared by game.obj and ai_player.obj.
#ifndef HOMM3_TURN_UPDATE_MSG_H
#define HOMM3_TURN_UPDATE_MSG_H

#include "netmsg.h"

// Dreamcast supplies the class/member names and the 24-byte size. Retail's
// constructor stores subtype 0x3f8 (RS_TURN_UPDATE) and the game position at
// +20 in both NextPlayer and advManager::StartLocalPlayerTurn.
class CTurnUpdateMsg : public CNetMsg {
public:
    int m_gamePos;

    CTurnUpdateMsg(int gamePos)
        : CNetMsg(0x3f8, sizeof(CTurnUpdateMsg)), m_gamePos(gamePos) {}
};
SIZE(CTurnUpdateMsg, 24);

#endif
