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

DATA(0x0069923c)
int& g_videoGameState = g_defaultGameContext;

// The 212-byte retail initializer constructs four unsigned-long bitsets in
// one reused stack temporary, then copies them into 0x699240..0x69924f.
// Its successive masks are 1, 3, 5 and 15. Game and single-selection readers
// index this table by the context selector and test individual feature bits.

// A plain four-element initializer naturally retains set for its last two
// elements and reproduces the table's construction and stores. Retail retains
// set for all four elements. That initializer expansion remains a separate
// compiler-state question; the retained setter itself is byte-exact.
// Adding <iostream> is a negative control: it introduces unmatched narrow and
// wide stream initialization, while <bitset> alone already emits the retail
// 32-byte locale-id guard. No extra stream include or emission caller is needed.
// Unsuffixed integer literals are byte-flat. An implicit scalar initializer
// list constructs directly into the array and loses retail's stack temporary,
// so explicit bitset temporaries are retained.
// `homm3 vc6 predict-inline --trace` on the initializer: the $E wrapper has
// cb 16 (budget 1000), the initializer cb 68, each bitset<4>(unsigned long)
// cb 95 at depth 2 (remaining 4), then _Tidy cb 72 and set cb 91 at depth 3.
// set's budgets are 137, 121, 88 and 82, so the first two expand. Refusing
// all four needs a first budget below 91: an initializer cb of at least 254,
// or at least six depth-2 candidates. Flag-constant masks (FA | FB ...), a
// returning file-static helper, nested temporaries and unsuffixed literals
// all still expand the first two, so the retail spelling stays unknown.
DATA(0x00699240)
std::bitset<4> g_gameContextFeatures[4] = {
    std::bitset<4>(1ul), std::bitset<4>(3ul),
    std::bitset<4>(5ul), std::bitset<4>(15ul)
};

VA_COMPGEN(0x004ecde0, 0x60, BITSET_SET, bitset4)
