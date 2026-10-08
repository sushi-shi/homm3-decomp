// Player.h - the editor's players (Player.cpp; Loki h3maped object 24).
#ifndef HOMM3_EDITOR_PLAYER_H
#define HOMM3_EDITOR_PLAYER_H

#include <bitset>

// TTimedEvent proves TPlayer is an enum (passed by value, compared signed);
// GameMap.cpp's Loki asserts spell its "no player" value ePlayerNone. The
// players' own enumerator names are not yet recovered.
enum TPlayer {
    ePlayerNone = -1
};

// An enumerator, not a constant object: Player.cpp and its includers emit
// no data for it.
enum { kNumPlayers = 8 };

// One bit per player: Loki's TEditTimedEventGeneralPage constructor prints
// its parameter as "const TPlayerMask &" and mangles bitset<8, unsigned long>.
typedef std::bitset<kNumPlayers> TPlayerMask;

// One record per player: the display name (kPlayerNameFmtStr with the
// player's number and colour) and the colour name from "plcolors.txt".
struct TPlayerTraits {
    const char* m_pName;
    const char* m_pColorName;
};

extern const TPlayerTraits* akPlayerTraits;

bool InitializePlayerTraitsTable();

#endif  /* HOMM3_EDITOR_PLAYER_H */
