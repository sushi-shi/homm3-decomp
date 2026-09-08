#ifndef HOMM3_GAMECONTEXT_H
#define HOMM3_GAMECONTEXT_H

#include <bitset>

// Provisional name for the four installed-game feature masks at 0x699240.
// Before normalization: gGameContextFeatures. Definition: gamecontext.cpp.
extern std::bitset<4> g_gameContextFeatures[4];

#endif
