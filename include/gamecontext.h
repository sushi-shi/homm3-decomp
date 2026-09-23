#ifndef HOMM3_GAMECONTEXT_H
#define HOMM3_GAMECONTEXT_H

#include <bitset>

// Installed-game context shared by resource selection and networking.
extern int& g_gameContext;

// Provisional name for the four installed-game feature masks at 0x699240.
extern std::bitset<4> g_gameContextFeatures[4];

#endif
