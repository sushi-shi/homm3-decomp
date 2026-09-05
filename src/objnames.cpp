// objnames.cpp - the Complete-only compiland between advmgr and advspells
// that owns the adventure-object trait rows, their five .rdata override
// tables and the objnames.txt name buffer.
//
// The whole span 0x41b250..0x41c2f0 is this compiland: besides the loader
// below it holds the Dinkumware string COMDATs retail calls from it
// (append at 0x41b340, runtime_error's string constructor at 0x41ba90)
// and the two TRuntimeError copy constructors at 0x41b7b0/0x41b920 whose
// `[src+0x1d]` byte copy exceptions.h already cites.
#include <string.h>

#include <memory>

#include <va.h>

#include "exceptions.h"
#include "objnames.h"
#include "resourcemanager.h"
#include "textresource.h"

// The rows themselves: retail .data 0x691698, and the `unsigned char
// (*)[16]` pointer at 0x660428 that every reader goes through holds
// exactly this address.
DATA(0x00691698)
TAdvObjectTraits gAdventureObjectTraitRows[ADVENTURE_OBJECT_TRAIT_COUNT];

// The five .rdata override tables the loader replays over the zeroed
// rows, in the order it walks them. Each is a list of adventure-object
// ids; only the first carries a second column, the objnames.txt row that
// id reads its name from.
DATA(0x0063a6e4)
static const TAdvObjectNameRow kAdventureObjectNameRows[] = {
    {165, 114}, {166, 115}, {167, 116}, {168, 117}, {169, 118}, {170,
    119}, {171, 120}, {172, 121}, {173, 122}, {174, 123}, {175, 124},
    {176, 125}, {177, 126}, {178, 127}, {179, 128}, {180, 129}, {181,
    130}, {182, 131}, {183, 132}, {184, 133}, {185, 134}, {186, 135},
    {187, 136}, {188, 137}, {189, 138}, {190, 143}, {191, 147}, {192,
    148}, {193, 149}, {194, 150}, {195, 151}, {196, 152}, {197, 153},
    {198, 154}, {199, 155}, {200, 156}, {201, 157}, {202, 158}, {203,
    159}, {204, 160}, {205, 161}, {219, 33}, {220, 53}, {221, 99}, {223,
    21}, {230, 46}
};

DATA(0x0063a854)
static const int kAdventureObjectTrait3Ids[] = {
    114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
    127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
    140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152,
    153, 154, 155, 156, 157, 158, 159, 160, 161, 165, 166, 167, 168,
    169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181,
    182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211
};

DATA(0x0063a9d0)
static const int kAdventureObjectTrait2Ids[] = {
    5, 6, 9, 12, 26, 29, 34, 54, 59, 62, 65, 66, 67, 68, 69, 70, 71, 72,
    73, 74, 75, 162, 163, 164, 76, 79, 81, 82, 86, 93, 101, 212, 214,
    215
};

DATA(0x0063aa58)
static const int kAdventureObjectLandBlockedIds[] = {
    3, 5, 6, 8, 9, 11, 12, 22, 26, 29, 34, 36, 52, 54, 59, 62, 65, 66,
    67, 68, 69, 70, 71, 72, 73, 74, 75, 162, 163, 164, 76, 79, 81, 82,
    85, 86, 92, 93, 95, 101, 214, 215
};

DATA(0x0063ab00)
static const int kAdventureObjectTrait1Ids[] = {
    3, 5, 6, 8, 9, 11, 12, 22, 26, 29, 33, 34, 36, 54, 59, 65, 66, 67,
    68, 69, 70, 71, 72, 73, 74, 75, 162, 163, 164, 76, 79, 81, 82, 85,
    86, 93, 101, 111, 212, 214, 215, 219
};

// Retail 0x41b500, the startup pass that builds the adventure-object
// trait rows. Called once, from 0x4ed80d.
//
// It zeroes all 232 rows (pointing every name at the shared empty
// literal and seeding nameRow with the row's own index), replays five
// .rdata override tables over them, then loads objnames.txt, measures
// the total length of its first 232 lines, buys ONE buffer for all of
// them through a function-local std::auto_ptr<char> at 0x691688, and
// re-points each row's name into it.
//
// The text resource lives in a scope guard whose destructor the state-0
// unwind funclet calls, so both throws below release it; the raw pointer
// beside it is what the two row loops read `Text` through.
// EXACT since 2026-09-05. The last residual was the second throw: retail
// EXPANDS TAllocationFailure's constructor here - the literal, the
// out-of-line TRuntimeError(const char*) at 0x49a0c0, then the 0x63aba8
// vftable - while keeping gzinflatebuf's 0x4d6b80 COMDAT, which only an
// inline definition can produce. Moving the body from gzinflatebuf.cpp into
// exceptions.h closed it (98.9899 -> 100.0000) and left gzinflatebuf's own
// COMDAT row at 100 with its three throw sites unmoved.
//
// Three levers got the rest: an explicit row pointer in the zeroing loop
// (95.53 -> 97.17, because retail keeps the COUNTER compare and walks a
// separate pointer), its store order (-> 98.38), and UNSIGNED counters
// for the two text loops where the zeroing loop's is signed (-> 98.99).
VA(0x0041b500, 0x28B)  // anchor-global 0x691698 trait rows + 0x660428 pointer; sole caller 0x4ed80d; retail-only
void InitializeAdventureObjectNames()
{
    static std::auto_ptr<char> nameBuffer;

    int i;
    TAdvObjectTraits* row = gAdventureObjectTraitRows;
    for (i = 0; i < ADVENTURE_OBJECT_TRAIT_COUNT; ++i, ++row) {
        row->trait1 = 0;
        row->trait2 = 0;
        row->blocksLanding = 0;
        row->trait3 = 0;
        row->name = "";
        row->nameRow = i;
    }

    for (i = 0; i < sizeof(kAdventureObjectNameRows)
                        / sizeof(kAdventureObjectNameRows[0]); ++i) {
        gAdventureObjectTraitRows[kAdventureObjectNameRows[i].objectType]
            .nameRow = kAdventureObjectNameRows[i].nameRow;
    }
    for (i = 0; i < sizeof(kAdventureObjectTrait3Ids)
                        / sizeof(kAdventureObjectTrait3Ids[0]); ++i) {
        gAdventureObjectTraitRows[kAdventureObjectTrait3Ids[i]].trait3 = 1;
    }
    for (i = 0; i < sizeof(kAdventureObjectTrait2Ids)
                        / sizeof(kAdventureObjectTrait2Ids[0]); ++i) {
        gAdventureObjectTraitRows[kAdventureObjectTrait2Ids[i]].trait2 = 1;
    }
    for (i = 0; i < sizeof(kAdventureObjectLandBlockedIds)
                        / sizeof(kAdventureObjectLandBlockedIds[0]); ++i) {
        gAdventureObjectTraitRows[kAdventureObjectLandBlockedIds[i]]
            .blocksLanding = 1;
    }
    for (i = 0; i < sizeof(kAdventureObjectTrait1Ids)
                        / sizeof(kAdventureObjectTrait1Ids[0]); ++i) {
        gAdventureObjectTraitRows[kAdventureObjectTrait1Ids[i]].trait1 = 1;
    }

    TTextResource* names = ResourceManager::GetText(
        DATA_COMPGEN(0x006604b4, objectNamesFileName, "objnames.txt"));
    TTextResourceGuard guard(names);
    if (names == 0)
        throw TRuntimeError();

    unsigned int total = 0;
    unsigned int line;
    for (line = 0; line < ADVENTURE_OBJECT_TRAIT_COUNT; ++line)
        total += strlen(names->GetText(line)) + 1;

    nameBuffer = std::auto_ptr<char>(new char[total]);
    if (nameBuffer.get() == 0)
        throw TAllocationFailure();

    char* next = nameBuffer.get();
    for (line = 0; line < ADVENTURE_OBJECT_TRAIT_COUNT; ++line) {
        const char* text = names->GetText(line);
        unsigned int size = strlen(text) + 1;
        memcpy(next, text, size);
        gAdventureObjectTraitRows[line].name = next;
        next += size;
    }
}

// Retail 0x41bd90, reached from the loader's state-0 unwind funclet and
// expanded again at its normal exit.
VA(0x0041bd90, 0x12)  // anchor-eh 0x627890 unwind funclet for 0x41b500 state 0, retail-only
TTextResourceGuard::~TTextResourceGuard()
{
    if (have && text != 0)
        text->Dispose();
}
