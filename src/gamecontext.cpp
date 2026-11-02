// Provisional compiland name. Retail has a separate game-context data unit
// between inputmgr and kb; no Dreamcast source filename survives for it.
// inputmgr's final body ends at 0x4eccca and its locale-id guard is 0x4eccd0.
// This unit initializes the context selector at 0x4eccf0 and the feature
// table at 0x4ecd00, then emits bitset<4>::set at 0x4ecde0 and its own
// locale-id guard at 0x4ece40. kb's terrain initializers start at 0x4ece60.
#include "va.h"

#include "gamecontext.h"

// The context reference is bound once by the retail startup body at 0x4eccf0.
// Its backing int at 0x67f554 starts at Complete's context ordinal, 3.
// A named int reference emits exactly that eleven-byte store/return in VC6;
// a pointer initializer is static data, and a literal-bound const reference
// emits an additional backing-value store. All reviewed consumers load the
// binding before reading its int; none rebind it.
DATA(0x0067f554)
static int g_defaultGameContext = 3;

VA_COMPGEN(0x004eccf0, 0xb, STATIC_CTOR, g_videoGameState)
DATA(0x0069923c)
int& g_videoGameState = g_defaultGameContext;

// Four unsigned-long bitsets with successive masks 1, 3, 5 and 15. Game and
// single-selection readers index this table by the context selector and
// test individual feature bits. Each entry is a TGameContextFeatures built
// from an explicit bitset temporary, as is artifact's slot table; readers
// use each entry as the bitset. Only <bitset> is included, not <iostream>.
// Mac's table is built by a different compiland's initializer.
VA_COMPGEN(0x004ecd00, 0xd4, STATIC_CTOR, g_gameContextFeatures)
DATA(0x00699240)
TGameContextFeatures g_gameContextFeatures[4] = {
    TGameContextFeatures(std::bitset<4>(1ul)),
    TGameContextFeatures(std::bitset<4>(3ul)),
    TGameContextFeatures(std::bitset<4>(5ul)),
    TGameContextFeatures(std::bitset<4>(15ul))
};

VA_COMPGEN(0x004ecde0, 0x60, BITSET_SET, bitset4)
