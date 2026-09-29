#ifndef HOMM3_GAMECONTEXT_H
#define HOMM3_GAMECONTEXT_H

#include <bitset>

// Active game/resource context; bound to the Complete default at startup.
extern int& g_videoGameState;

// One installed-game context's feature mask. The table cinit at 0x4ecd00
// proves an entry type built from each mask by a one-argument constructor,
// and the readers use each entry as the bitset itself: see gamecontext.cpp.
// The type and table names are provisional.
struct TGameContextFeatures : public std::bitset<4> {
    TGameContextFeatures(const std::bitset<4>& features)
        : std::bitset<4>(features) {}
};

// The four installed-game feature masks at 0x699240.
extern TGameContextFeatures g_gameContextFeatures[4];

#endif
