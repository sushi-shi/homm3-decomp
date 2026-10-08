// Player.h - the editor's player traits (Loki Player.cpp).
#ifndef HOMM3_EDITOR_PLAYER_H
#define HOMM3_EDITOR_PLAYER_H

#include <bitset>

// TTimedEvent proves TPlayer is an enum (passed by value, compared signed,
// and SGI's global relops instantiate operator!=<TPlayer>); GameMap.cpp's
// asserts spell its "no player" value ePlayerNone. The players' own
// enumerator names are not yet recovered.
enum TPlayer {
    ePlayerNone = -1
};

const int kNumPlayers = 8;

// One bit per player: TEditTimedEventGeneralPage's constructor prints its
// parameter as "const TPlayerMask &" and mangles bitset<8, unsigned long>.
typedef bitset<kNumPlayers> TPlayerMask;

// One record per player: the display name ("Player %d (%s)"-style, built
// from kPlayerNameFmtStr) and the colour name from "plcolors.txt".
struct TPlayerTraits {
    const char* m_pName;
    const char* m_pColorName;
};

extern const TPlayerTraits* akPlayerTraits;

bool InitializePlayerTraitsTable();

#endif  /* HOMM3_EDITOR_PLAYER_H */
