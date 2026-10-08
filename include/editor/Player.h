// Player.h - the editor's player traits (Loki Player.cpp).
#ifndef HOMM3_EDITOR_PLAYER_H
#define HOMM3_EDITOR_PLAYER_H

const int kNumPlayers = 8;

// One record per player: the display name ("Player %d (%s)"-style, built
// from kPlayerNameFmtStr) and the colour name from "plcolors.txt".
struct TPlayerTraits {
    const char* m_pName;
    const char* m_pColorName;
};

extern const TPlayerTraits* const akPlayerTraits;

bool InitializePlayerTraitsTable();

#endif  /* HOMM3_EDITOR_PLAYER_H */
