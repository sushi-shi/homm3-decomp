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

// Each entry is a TGameContextFeatures built from an explicit bitset
// temporary. Its constructors (cb 30) are inline candidate sites after
// each bitset<4>(unsigned long) constructor, so the depth-2 site
// list is eight long: each constructor's nested budget leaves bitset::_Tidy
// expanded but refuses set (budgets 29, 35, 46 and 82 against its cb 91),
// and all 224 bytes, including the four retained set calls, match retail.
// The same per-mask construction reproduces artifact's slot table.
// Readers use each entry as the bitset: a bitset member reproduces this
// initializer too, but its added access cost changes updateMainWindow's
// expansion and drops onSetAsHostMsg from 100% to 25.5976%.
// Failed controls: a plain bitset array (set budgets 137, 121, 88 and 82 -
// the first two expand), with flag-constant masks, unsuffixed literals,
// nested temporaries or a returning file-static helper; an entry type whose
// constructor takes the unsigned long (272 bytes); and implicit scalar
// initializers, which construct directly into the array and lose retail's
// stack temporary. Adding <iostream> introduces unmatched stream
// initialization, while <bitset> alone emits the retail 32-byte locale-id
// guard. Mac's table is built by a different compiland's initializer
// (0x221ee4, which also runs <iostream> and terrain.h initialization) from
// plain bitset temporaries four bytes apart; this type would reserve a
// second temporary per entry there, so that Mac spelling is not this unit's.
DATA(0x00699240)
TGameContextFeatures g_gameContextFeatures[4] = {
    TGameContextFeatures(std::bitset<4>(1ul)),
    TGameContextFeatures(std::bitset<4>(3ul)),
    TGameContextFeatures(std::bitset<4>(5ul)),
    TGameContextFeatures(std::bitset<4>(15ul))
};

VA_COMPGEN(0x004ecde0, 0x60, BITSET_SET, bitset4)
