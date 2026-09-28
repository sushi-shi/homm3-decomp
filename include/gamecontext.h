#ifndef HOMM3_GAMECONTEXT_H
#define HOMM3_GAMECONTEXT_H

#include <bitset>

// Active game/resource context; bound to the Complete default at startup.
extern int& g_videoGameState;

// Provisional name for the four installed-game feature masks at 0x699240.
extern std::bitset<4> g_gameContextFeatures[4];

#endif
