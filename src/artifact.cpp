// Artifact traits, slot classes and combination recipes. The static traits
// helper and ownership wrappers expand into the table initializer. Adjacent
// bitset bodies are Dinkumware COMDATs, not authored game routines.
#include "va.h"

#include <algorithm>
#include <assert.h>
#include <bitset>
#include <functional>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "artifact.h"

#include "const_bitset_iterator.h"
#include "ownership.h"
#include "resourcemanager.h"
#include "textresource.h"

// Initial contents recovered from the pinned Complete image. One row per
// artifact: hero.cpp's combination walks read rows 0..143, and retail bounds
// that pointer walk at 0x63e998, exactly ARTIFACT_COUNT rows past the start.
DATA(0x0063e758) const signed char g_artifactPrimarySkillBonuses[ARTIFACT_COUNT][4] = {
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 2, 0, 0, 0 },
    { 3, 0, 0, 0 },
    { 4, 0, 0, 0 },
    { 5, 0, 0, 0 },
    { 6, 0, 0, 0 },
    { 12, -3, 0, 0 },
    { 0, 2, 0, 0 },
    { 0, 3, 0, 0 },
    { 0, 4, 0, 0 },
    { 0, 5, 0, 0 },
    { 0, 6, 0, 0 },
    { -3, 12, 0, 0 },
    { 0, 0, 0, 1 },
    { 0, 0, 0, 2 },
    { 0, 0, 0, 3 },
    { 0, 0, 0, 4 },
    { 0, 0, 0, 5 },
    { 0, 0, -2, 10 },
    { 0, 0, 1, 0 },
    { 0, 0, 2, 0 },
    { 0, 0, 3, 0 },
    { 0, 0, 4, 0 },
    { 0, 0, 5, 0 },
    { 0, 0, 10, -2 },
    { 1, 1, 1, 1 },
    { 2, 2, 2, 2 },
    { 3, 3, 3, 3 },
    { 4, 4, 4, 4 },
    { 5, 5, 5, 5 },
    { 6, 6, 6, 6 },
    { 1, 1, 0, 0 },
    { 2, 2, 0, 0 },
    { 3, 3, 0, 0 },
    { 4, 4, 0, 0 },
    { 0, 0, 1, 1 },
    { 0, 0, 2, 2 },
    { 0, 0, 3, 3 },
    { 0, 0, 4, 4 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 3, 3, 3, 6 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 6, 6, 6, 6 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 }
};

// Retail retains bitset<19>::_Tidy and set. The explicit class instantiation
// currently supplies set's retained body; real uses already emit _Tidy.
// Removing it leaves the initializer bytes unchanged but loses the exact
// retained set body after refreshing targets. Its original presence remains
// unproven; removal does not recover the proxy-to-set inline boundary below.
template class std::bitset<19>;
namespace {

// Column 2 of artraits.txt names the final physical slot, while column 20
// names slot zero. The one exception is column 7, the two-handed weapon
// class (slot 18), which sits between slots 13 and 12 in the file.
DATA(0x0063b940)
static const int g_artifactSlotColumnBits[kNumArtifactSlots + 1] = {
    17, 16, 15, 14, 13, 18, 12, 11, 10, 9,
    8, 7, 6, 5, 4, 3, 2, 1, 0
};

DATA(0x0063b98c)
static const int g_disabledArtifacts[3] = { 141, 142, 143 };

DATA(0x0063b998)
static const int g_spellGivingArtifacts[9] = {
    1, 128, 123, 124, 86, 87, 88, 89, 135
};

// Complete traits storage. These addresses and enlarged record counts are
// proved by the table parser and its consumers. DC's per-record bitset field
// becomes a compact slot-class index; its old constructor is not a Windows
// source claim. The two recipe/mask cinits below are separate table owners.
DATA(0x006939f8)
static TArtifactTraits g_artifactTraitsStorage[ARTIFACT_COUNT];

DATA(0x00694bf8)
static TArtifactSlotTraits g_artifactSlotTraitsStorage[kNumArtifactSlots + 1];

} // namespace

VA(0x0044c720, 0x10B)
MAC_ADDRESS(0x05a534, 0xdc)
static std::bitset<kNumArtifactSlots + 1> makeArtifactSlotMask(unsigned count, ...)
{
    std::bitset<kNumArtifactSlots + 1> mask;
    va_list slots;
    va_start(slots, count);
    try {
        while (count > 0) {
            mask[va_arg(slots, int)] = true;
            --count;
        }
    } catch (...) {
        va_end(slots);
        throw;
    }
    va_end(slots);
    return mask;
}

VA(0x0044c830, 0x122)
MAC_ADDRESS(0x05a610, 0xfc)
static std::bitset<ARTIFACT_COUNT> makeArtifactComponentMask(unsigned count, ...)
{
    std::bitset<ARTIFACT_COUNT> mask;
    va_list components;
    va_start(components, count);
    try {
        while (count > 0) {
            mask[va_arg(components, int)] = true;
            --count;
        }
    } catch (...) {
        va_end(components);
        throw;
    }
    va_end(components);
    return mask;
}

// A single ordinary function-template factory preserves both bodies but
// does not fix the slot cinit: its variadic calls still fail the non-EH
// caller-state gate (0x788 versus the ordinary bodies' 0x708). The empty
// mask's nested _Tidy still receives 742 units and expands. No template
// source spelling or convenience wrapper is established by that control.

// The twelve Shadow of Death combination artifacts, ids 129..140, each with
// the component set the assembled artifact consumes. Read straight out of
// the cinit at 0x44c960: the 24-byte record is built in a stack temporary
// (the id dword plus the builder's five-word result) and copied to
// 0x6938d8 + 24*i by a six-dword `rep movsd`, which is the record's own
// two-argument constructor inlined plus its implicit copy.
DATA(0x006938d8)
const TCombinationArtifact g_combinationArtifactTable[12] = {
    TCombinationArtifact(0x81,
        makeArtifactComponentMask(6, 0x24, 0x21, 0x23, 0x1f, 0x20, 0x22)),
    TCombinationArtifact(0x82, makeArtifactComponentMask(3, 0x36, 0x37, 0x38)),
    TCombinationArtifact(0x83, makeArtifactComponentMask(3, 0x5f, 0x60, 0x5e)),
    TCombinationArtifact(0x84,
        makeArtifactComponentMask(4, 0x14, 0x08, 0x1a, 0x0e)),
    TCombinationArtifact(0x85,
        makeArtifactComponentMask(5, 0x76, 0x77, 0x78, 0x79, 0x7a)),
    TCombinationArtifact(0x86,
        makeArtifactComponentMask(9, 0x2c, 0x2b, 0x2a, 0x26, 0x27,
                                  0x25, 0x2d, 0x29, 0x28)),
    TCombinationArtifact(0x87,
        makeArtifactComponentMask(4, 0x18, 0x0c, 0x1e, 0x12)),
    TCombinationArtifact(0x88, makeArtifactComponentMask(2, 0x7b, 0x47)),
    TCombinationArtifact(0x89, makeArtifactComponentMask(3, 0x3c, 0x3d, 0x3e)),
    TCombinationArtifact(0x8a, makeArtifactComponentMask(3, 0x49, 0x4a, 0x4b)),
    TCombinationArtifact(0x8b, makeArtifactComponentMask(3, 0x4c, 0x4e, 0x4d)),
    TCombinationArtifact(0x8c,
        makeArtifactComponentMask(4, 0x6f, 0x6d, 0x6e, 0x71)),
};

VA_COMPGEN(0x0044c960, 0x298, STATIC_CTOR, g_combinationArtifactTable)

// The fifteen allowable-slot classes searched by the traits initializer,
// recovered from 0x44cc00. Class 0 is empty; classes 7 and 9 are the ring
// pair (6, 7) and misc/backpack group (9, 10, 11, 12, 18).
// Each entry is a TArtifactSlotMask built from its mask, as the combination
// table above builds TCombinationArtifact records. The empty mask's
// constructor is the first of sixteen depth-2 candidates (cb 363
// initializer, budget 637), so its nested bitset::_Tidy(0) gets 637/16 = 39
// and retail keeps the call at 0x44cc09; the entry constructors (cb 30)
// expand into the plain copies. This reproduces all 336 bytes. Failed
// controls, all of which expand _Tidy (budget 742 or 644) because the
// variadic builder is never a candidate (callee flags 0x708, or 0x8 without
// its try block): a plain bitset array (const or not, sized or not),
// explicit std::bitset<19>(...) element casts, zero-value construction of
// the empty entry, and moving or removing the explicit bitset<19>
// instantiation.
DATA(0x00693898)
const TArtifactSlotMask g_artifactSlotMasks[15] = {
    TArtifactSlotMask(std::bitset<kNumArtifactSlots + 1>()),
    TArtifactSlotMask(makeArtifactSlotMask(1, 0)),
    TArtifactSlotMask(makeArtifactSlotMask(1, 1)),
    TArtifactSlotMask(makeArtifactSlotMask(1, 2)),
    TArtifactSlotMask(makeArtifactSlotMask(1, 3)),
    TArtifactSlotMask(makeArtifactSlotMask(1, 4)),
    TArtifactSlotMask(makeArtifactSlotMask(1, 5)),
    TArtifactSlotMask(makeArtifactSlotMask(2, 6, 7)),
    TArtifactSlotMask(makeArtifactSlotMask(1, 8)),
    TArtifactSlotMask(makeArtifactSlotMask(5, 9, 10, 11, 12, 18)),
    TArtifactSlotMask(makeArtifactSlotMask(1, 13)),
    TArtifactSlotMask(makeArtifactSlotMask(1, 14)),
    TArtifactSlotMask(makeArtifactSlotMask(1, 15)),
    TArtifactSlotMask(makeArtifactSlotMask(1, 16)),
    TArtifactSlotMask(makeArtifactSlotMask(1, 17)),
};

VA_COMPGEN(0x0044cc00, 0x145, STATIC_CTOR, g_artifactSlotMasks)

DATA(0x00660b64)
const TArtifactSlotTraits (&akArtifactSlotTraits)[19] = g_artifactSlotTraitsStorage;

DATA(0x00660b68)
const TArtifactTraits (&akArtifactTraits)[ARTIFACT_COUNT] = g_artifactTraitsStorage;

DATA(0x00660b6c)
const TCombinationArtifact* g_combinationArtifacts = g_combinationArtifactTable;

static void InitializeArtifactTraits(int id,
    const TSpreadsheetResource::TStringVector& resource);

// E:\gamedcs\artifact.cpp:56 Complete requires 146 rows,
// packs names/descriptions into one owned buffer, derives the slot classes,
// applies disabled/spell/combination metadata, and loads 19 slot names.
//
// Keep the ordinary row-parser boundary proved by DC and represented by a
// recognizable expansion in Complete Mac. The current reconstructed helper
// exceeds the editor profile's C2 candidate-size threshold; that unresolved
// compiler difference does not establish absence of the original helper.
// With the Complete owner/counter/default-field model below, the ordinary
// helper scores game 90.20%, h3maped 68.35%, h3ccmped 77.41%; flattening its
// body into this caller scores 91.88%, 95.37%, 89.77% respectively, but loses
// the evidenced source boundary. Those flattened peaks remain HIST only.
//
// The spreadsheet is held only through its TResourcePtr: retail and Mac
// keep the pointer in the guard slot and reload it for every query
// (h3maped 90.57%, game 91.32%). Both loops count the artifact id from 0
// to 144 and add 2 for the spreadsheet row, as Mac's r25/r20 counters do
// (h3maped 95.23%, game 91.88%, h3ccmped 89.77%).
// Restoring strcpy at the pooled copies makes VC6 scan each source twice:
// once for the cursor's length and again inside intrinsic strcpy. Retail
// scans once and reuses that count for copying, supporting memcpy here.
// DC uses Dispose and per-string TAutoStrPtr arrays. Complete's resources
// dispose through vtable+4; its two pooled owners have an ownership byte and
// pointer, proved by the retained 0x44d340/0x44d360 destructors.
// Mac 0x5a9a0 stages the bitset pointer and index before calling set,
// preserving operator[]'s reference proxy; a direct set(bit, value) call
// lets the game expand set (76.95%). DC153/155/157/159 each subscript the
// row before testing R/J/N/T; retail has one subscript and one byte load,
// which the named classCell reproduces. Mac and the h3maped /O1 schedule
// store the four Complete-only defaults as combo, target, disabled, spells
// (h3maped +1.3%; the game's /O2 schedule loses 0.24%). Mac stores the
// combination's type before its cost; the reverse order is 0.02% lower.
// Earlier-model controls: removing the dead printf carrier, pooled copies in
// the caller, the find_if identity predicate (test al,al rather than
// cmp al,1) and naming the component traits each moved toward retail.
// Residual: the slot-mask search keeps its counter in a register and the
// equality word index in memory where h3maped does the reverse; the
// editors' data labels for the traits tables are not yet claimed.
// DC public ?InitializeArtifactTraitsTable@@YA_NXZ proves bool; the SH4
// dossier renders its byte-sized procedure result as unsigned char. Complete
// returns only AL 0/1, and the sole kb caller tests that Boolean result.
VA_AT(h3maped, 0x004036a8, 0x457)
VA_AT(h3ccmped, 0x004028d0, 0x5f7)
VA(0x0044cd50, 0x5E8)
DC_ADDRESS(0x04fec0, 0x126)
MAC_ADDRESS(0x05a70c, 0x7ec)  // anchor-strings/caller
bool InitializeArtifactTraitsTable()
{
    {
        TResourcePtr<TSpreadsheetResource> traitsSheet(
            ResourceManager::GetSpreadsheet(
                DATA_COMPGEN(0x00660b80, artifactTraitsSpreadsheetName,
                             "artraits.txt")));
        if (!traitsSheet.get() || traitsSheet->GetNumberOfRows() < 146) {
            return 0;
        }

        unsigned stringBytes = 0;
        int id;
        for (id = 0; id < ARTIFACT_COUNT; ++id) {
            const char* name = traitsSheet->GetSpreadsheet(id + 2, 0);
            const char* description = traitsSheet->GetSpreadsheet(id + 2, 22);
            stringBytes += strlen(description) + strlen(name) + 2;
        }

        // DC artifact.cpp:33..38 used an anonymous four-byte TAutoStrPtr:
        // null construction, a direct pointer set/get, and unconditional
        // destruction. Complete replaces it with TAutoArrayPtr<char>.
        // The retained destructor 0x44d360 tests ownership at 0x694c90
        // before deleting the pointer at 0x694c94; the second buffer uses
        // the same eight-byte layout and conditional destructor 0x44d340.
        DATA_COMPGEN_GUARD(0x006938d4, artifactStringsGuard, artifactStrings)

        VA_COMPGEN(0x0044d360, 0x16, STATIC_DTOR, artifactStrings)
        DATA(0x00694c90)
        static TAutoArrayPtr<char> artifactStrings(new char[stringBytes]);
        if (!artifactStrings.get())
            return 0;

        char* destination = artifactStrings.get();
        for (id = 0; id < ARTIFACT_COUNT; ++id) {
            const TSpreadsheetResource::TStringVector& values =
                traitsSheet->GetRow(id + 2);
            TArtifactTraits& traits = g_artifactTraitsStorage[id];
            const char* source = values[0];
            unsigned length = strlen(source) + 1;
            memcpy(destination, source, length);
            traits.m_name = destination;
            destination += length;

            source = values[22];
            length = strlen(source) + 1;
            memcpy(destination, source, length);
            traits.m_description = destination;
            destination += length;

            InitializeArtifactTraits(id, values);
            traits.m_comboType = -1;
            traits.m_targetCombo = -1;
            traits.m_disabled = 0;
            traits.m_givesSpells = 0;
        }
    }

    unsigned artifactId;
    for (artifactId = 0; artifactId < 3; ++artifactId)
        g_artifactTraitsStorage[g_disabledArtifacts[artifactId]].m_disabled = 1;
    for (artifactId = 0; artifactId < 9; ++artifactId)
        g_artifactTraitsStorage[g_spellGivingArtifacts[artifactId]].m_givesSpells = 1;

    int combo;
    for (combo = 0; combo < 12; ++combo) {
        const TCombinationArtifact& combination =
            g_combinationArtifacts[combo];
        TArtifactTraits& assembled =
            g_artifactTraitsStorage[combination.m_artifactId];
        assembled.m_comboType = combo;
        assembled.m_cost = 0;
        TConstBitsetIterator<ARTIFACT_COUNT> current(combination.m_components, 0);
        TConstBitsetIterator<ARTIFACT_COUNT> end(combination.m_components, ARTIFACT_COUNT);
        for (; (current = std::find_if(current, end,
                    std::bind2nd(std::not_equal_to<bool>(), false))) != end;
             ++current) {
            int component = current.position();
            TArtifactTraits& componentTraits = g_artifactTraitsStorage[component];
            componentTraits.m_targetCombo = combo;
            assembled.m_cost += componentTraits.m_cost;
        }
    }

    {
        TResourcePtr<TSpreadsheetResource> slotsSheet(
            ResourceManager::GetSpreadsheet(
                DATA_COMPGEN(0x00660b70, artifactSlotsSpreadsheetName,
                             "artslots.txt")));
        if (!slotsSheet.get() || slotsSheet->GetNumberOfRows() < kNumArtifactSlots + 1) {
            return 0;
        }

        unsigned stringBytes = 0;
        int slot;
        for (slot = 0; slot < kNumArtifactSlots + 1; ++slot)
            stringBytes += strlen(slotsSheet->GetRow(slot)[0]) + 1;

        VA_COMPGEN(0x0044d340, 0x16, STATIC_DTOR, artifactSlotStrings)
        DATA(0x00694c98)
        static TAutoArrayPtr<char> artifactSlotStrings(new char[stringBytes]);
        if (!artifactSlotStrings.get())
            return 0;

        char* destination = artifactSlotStrings.get();
        for (slot = 0; slot < kNumArtifactSlots + 1; ++slot) {
            const char* source = slotsSheet->GetRow(slot)[0];
            unsigned length = strlen(source) + 1;
            memcpy(destination, source, length);
            g_artifactSlotTraitsStorage[slot].m_name = destination;
            destination += length;

            int mask = 0;
            while (!g_artifactSlotMasks[mask].test(slot))
                ++mask;
            g_artifactSlotTraitsStorage[slot].m_type = mask;
        }
    }
    return 1;
}

// DC 0x050058 proves the ordinary static two-argument row parser. Complete
// Mac 0x5a92c..0x5aa58 expands its cost/slot-mask/class operation; the pooled
// string copies and added default fields remain owned by the caller.
DC_ADDRESS(0x050058, 0x7ba)
static void InitializeArtifactTraits(int id,
    const TSpreadsheetResource::TStringVector& resource)
{
    assert(id >= 0 && id < ARTIFACT_COUNT);
    assert(resource.size() >= 23);
    TArtifactTraits& traits = g_artifactTraitsStorage[id];
    std::bitset<kNumArtifactSlots + 1> allowableSlots;
    traits.m_cost = atoi(resource[1]);
    int column;
    for (column = 0; column < kNumArtifactSlots + 1; ++column) {
        int bit = g_artifactSlotColumnBits[column];
        allowableSlots[bit] = resource[column + 2][0] != 0
            && resource[column + 2][0] != ' ';
    }
    int mask = 0;
    while (allowableSlots != g_artifactSlotMasks[mask])
        ++mask;
    traits.m_allowableSlotMask = mask;

    const char* classCell = resource[21];
    if (classCell[0] == 'R')
        traits.m_class = 16;
    else if (classCell[0] == 'J')
        traits.m_class = 8;
    else if (classCell[0] == 'N')
        traits.m_class = 4;
    else if (classCell[0] == 'T')
        traits.m_class = 2;
    else {
        assert(classCell[0] == 'S');
        traits.m_class = 1;
    }
}

VA_COMPGEN(0x0044D380, 0x60, BITSET_SET, Bitset19)

VA_COMPGEN(0x0044d3e0, 0x17, BITSET_TIDY, Bitset19)

VA_COMPGEN(0x0044d400, 0xCB, BITSET_XRAN, Bitset19)
