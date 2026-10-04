// objnames.cpp - the Complete-only compiland between advmgr and advspells
// that owns the adventure-object trait rows, their five .rdata override
// tables and the objnames.txt name buffer.

// The whole span 0x41b250..0x41c2f0 is this compiland: besides the loader
// below it holds the Dinkumware string COMDATs retail calls from it
// (append at 0x41b340, runtime_error's string constructor at 0x41ba90)
// and the two TRuntimeError copy constructors at 0x41b7b0/0x41b920 whose
// `[src+0x1d]` byte copy exceptions.h already cites.
#include "va.h"

#include <string.h>

#include "objnames.h"
#include "autoarrayptr.h"

#include "exceptions.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "textresource.h"

// The rows themselves: retail .data 0x691698. The pointer cell at
// 0x660428 holds this address; readers and the loader share the same record.
DATA(0x00691698)
TAdvObjectTraits g_adventureObjectTraitRows[ADVENTURE_OBJECT_TRAIT_COUNT];
// Initial contents recovered from the pinned Complete image.
// Retail pointer cell used by the readers; the loader owns the rows.
DATA(0x00660428) const TAdvObjectTraits* g_adventureObjectTraits =
    g_adventureObjectTraitRows;


// The five .rdata override tables the loader replays over the zeroed
// rows, in the order it walks them. Each is a list of adventure-object
// ids; only the first carries a second column, the objnames.txt row that
// id reads its name from.
DATA(0x0063a6e4)
static const TAdvObjectNameRow g_adventureObjectNameRows[] =
#include "rmg_data/object_name_rows.inc"
;

DATA(0x0063a854)
static const int g_adventureObjectDecorationIds[] =
#include "rmg_data/object_decoration_ids.inc"
;

DATA(0x0063a9d0)
static const int g_adventureObjectClearedOnVisitIds[] =
#include "rmg_data/object_cleared_ids.inc"
;

DATA(0x0063aa58)
static const int g_adventureObjectLandBlockedIds[] =
#include "rmg_data/object_land_blocked_ids.inc"
;

DATA(0x0063ab00)
static const int g_adventureObjectEnterableFromNorthIds[] =
#include "rmg_data/object_north_ids.inc"
;

// It zeroes all 232 rows (pointing every name at the shared empty
// literal and seeding nameRow with the row's own index), replays five
// .rdata override tables over them, then loads objnames.txt, measures
// the total length of its first 232 lines, buys ONE buffer for all of
// them through a function-local TAutoArrayPtr<char> at 0x691688, and
// re-points each row's name into it.

// vftable - while keeping gzinflatebuf's 0x4d6b80 COMDAT. The shared header
// supplies body visibility to both callers; expansion does not establish
// the original inline qualifier. Moving the body to exceptions.h closed it (98.9899 -> 100.0000) and left gzinflatebuf's own
VA(0x0041b500, 0x28B)
MAC_ADDRESS(0x21d350, 0x6dc)
void initializeAdventureObjectNames()
{
    // Mac 0:0x21d918/0x21d95c retains array-owner assignment/cleanup.
    // Retail guards the static with bit 0 of 0x691690 and registers its
    // 22-byte destructor at 0x41b790 with _atexit.
    DATA_COMPGEN_GUARD(0x00691690, nameBufferGuard, nameBuffer)

    VA_COMPGEN(0x0041b790, 0x16, STATIC_DTOR, nameBuffer)
    DATA(0x00691688)
    static TAutoArrayPtr<char> nameBuffer;

    int i;
    TAdvObjectTraits* row = g_adventureObjectTraitRows;
    for (i = 0; i < ADVENTURE_OBJECT_TRAIT_COUNT; ++i, ++row) {
        row->m_enterableFromNorth = 0;
        row->m_clearedOnVisit = 0;
        row->m_blocksLanding = 0;
        row->m_isDecoration = 0;
        row->m_name = "";
        row->m_nameRow = i;
    }

    for (i = 0; i < sizeof(g_adventureObjectNameRows)
                        / sizeof(g_adventureObjectNameRows[0]); ++i) {
        g_adventureObjectTraitRows[g_adventureObjectNameRows[i].m_objectType]
            .m_nameRow = g_adventureObjectNameRows[i].m_nameRow;
    }
    for (i = 0; i < sizeof(g_adventureObjectDecorationIds)
                        / sizeof(g_adventureObjectDecorationIds[0]); ++i) {
        g_adventureObjectTraitRows[g_adventureObjectDecorationIds[i]].m_isDecoration = 1;
    }
    for (i = 0; i < sizeof(g_adventureObjectClearedOnVisitIds)
                        / sizeof(g_adventureObjectClearedOnVisitIds[0]); ++i) {
        g_adventureObjectTraitRows[g_adventureObjectClearedOnVisitIds[i]].m_clearedOnVisit = 1;
    }
    for (i = 0; i < sizeof(g_adventureObjectLandBlockedIds)
                        / sizeof(g_adventureObjectLandBlockedIds[0]); ++i) {
        g_adventureObjectTraitRows[g_adventureObjectLandBlockedIds[i]]
            .m_blocksLanding = 1;
    }
    for (i = 0; i < sizeof(g_adventureObjectEnterableFromNorthIds)
                        / sizeof(g_adventureObjectEnterableFromNorthIds[0]); ++i) {
        g_adventureObjectTraitRows[g_adventureObjectEnterableFromNorthIds[i]].m_enterableFromNorth = 1;
    }

    TTextResource* names = ResourceManager::getText(
        DATA_COMPGEN(0x006604b4, objectNamesFileName, "objnames.txt"));
    TResourcePtr<TTextResource> guard(names);
    if (names == 0)
        throw TRuntimeError();

    unsigned int total = 0;
    unsigned int line;
    for (line = 0; line < ADVENTURE_OBJECT_TRAIT_COUNT; ++line)
        total += strlen(names->getText(line)) + 1;

    nameBuffer = TAutoArrayPtr<char>(new char[total]);
    if (nameBuffer.get() == 0)
        throw TAllocationFailure();

    char* next = nameBuffer.get();
    for (line = 0; line < ADVENTURE_OBJECT_TRAIT_COUNT; ++line) {
        const char* text = names->getText(line);
        unsigned int size = strlen(text) + 1;
        memcpy(next, text, size);
        g_adventureObjectTraitRows[line].m_name = next;
        next += size;
    }
}
