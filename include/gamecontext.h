#ifndef HOMM3_GAMECONTEXT_H
#define HOMM3_GAMECONTEXT_H

#include <bitset>

// Retail-only installed-game context, backed by the initialized dword at 0x67f554.
extern int& g_gameContext;

// Provisional name for the four installed-game feature masks at 0x699240.
extern std::bitset<4> g_gameContextFeatures[4];

#endif
