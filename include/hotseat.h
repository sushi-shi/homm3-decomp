// hotseat.h - the local hot-seat player-name roster shared by the
// multiplayer setup dialog and the game-selection window.
#ifndef HOMM3_HOTSEAT_H
#define HOMM3_HOTSEAT_H

#include <string.h>
#include "va.h"

// DC's nested char[21][8] type gives this class its complete 0xac-byte
// layout. Retail OnOK independently proves the same 21-byte stride and the
// eight-player bound while inlining the constructor and AddPlayer.
class CHotSeatMan {
public:
    enum {
        MAX_PLAYERS = 8,
        PLAYER_NAME_SIZE = 21
    };

    int m_playerCount;
    char m_names[MAX_PLAYERS][PLAYER_NAME_SIZE];

    CHotSeatMan() : m_playerCount(0) {}
    void clear() { m_playerCount = 0; }
    void addPlayer(const char* name)
    {
        if (m_playerCount < MAX_PLAYERS) {
            strcpy(m_names[m_playerCount], name);
            ++m_playerCount;
        }
    }
    char* getName(int player);
};
SIZE(CHotSeatMan, 0xac);

DATA(0x0069ca50) extern CHotSeatMan* g_hotSeatMan;

#endif /* HOMM3_HOTSEAT_H */
