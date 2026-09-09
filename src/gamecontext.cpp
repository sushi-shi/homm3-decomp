// Provisional compiland name. Retail has a separate game-context data unit
// between inputmgr and kb; no Dreamcast source filename survives for it.
// inputmgr's final body ends at 0x4eccca and its locale-id guard is 0x4eccd0.
// This unit initializes the context selector at 0x4eccf0 and the feature
// table at 0x4ecd00, then emits bitset<4>::set at 0x4ecde0 and its own
// locale-id guard at 0x4ece40. kb's terrain initializers start at 0x4ece60.
#include <va.h>
#include "gamecontext.h"

// The 212-byte retail initializer constructs four unsigned-long bitsets in
// one reused stack temporary, then copies them into 0x699240..0x69924f.
// Its successive masks are 1, 3, 5 and 15. Game and single-selection readers
// index this table by the context selector and test individual feature bits.
// The existing provisional name was gGameContextFeatures before normalization;
// no surviving symbol supplies an original semantic spelling.
//
// A plain four-element initializer naturally retains set for its last two
// elements and reproduces the table's construction and stores. Retail retains
// set for all four elements. That initializer expansion remains a separate
// compiler-state question; the retained setter itself is byte-exact.
// Adding <iostream> is a negative control: it introduces unmatched narrow and
// wide stream initialization, while <bitset> alone already emits the retail
// 32-byte locale-id guard. No extra stream include or emission caller is needed.
// Unsuffixed integer literals are byte-flat. An implicit scalar initializer
// list constructs directly into the array and loses retail's stack temporary,
// so explicit bitset temporaries are retained. The adjacent selector still
// needs its own source recovery: binding a const int reference directly to 3
// adds a ten-byte runtime backing-value store absent from its retail thunk.
DATA(0x00699240)
std::bitset<4> g_gameContextFeatures[4] = {
    std::bitset<4>(1ul), std::bitset<4>(3ul),
    std::bitset<4>(5ul), std::bitset<4>(15ul)
};

// The table initializer is the sole retail caller. The four-bit bound,
// Boolean set/reset arms and reference-return ABI match Dinkumware's body.
VA_COMPGEN(0x004ecde0, 0x60, BITSET_SET, bitset4)
