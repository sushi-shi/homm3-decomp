// Artifact traits, slot classes and combination recipes. The static traits
// helper and ownership wrappers expand into the table initializer. Adjacent
// bitset bodies are Dinkumware COMDATs, not authored game routines.
#include "va.h"

#include <algorithm>
#include <bitset>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "artifact.h"

#include "const_bitset_iterator.h"
#include "ownership.h"
#include "resourcemanager.h"
#include "textresource.h"

// Initial contents recovered from the pinned Complete image.
DATA(0x0063e758) const signed char g_artifactPrimarySkillBonuses[144][4] = {
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
// Removing it is flat for InitializeArtifactTraitsTable at 80.8218% but
// loses the exact set enrollment. It does not recover the proxy-to-set
// inline boundary below; the declaration's original presence is unproven.
template class std::bitset<19>;
namespace {

// Column 2 of artraits.txt names the final physical slot, while column 20
// names slot zero. The one exception is column 7, the two-handed weapon
// class (slot 18), which sits between slots 13 and 12 in the file.
DATA(0x0063b940)
static const int g_artifactSlotColumnBits[19] = {
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
static TArtifactTraits g_artifactTraitsStorage[144];

DATA(0x00694bf8)
static TArtifactSlotTraits g_artifactSlotTraitsStorage[19];

} // namespace

VA(0x0044c720, 0x10B)
static std::bitset<19> makeArtifactSlotMask(unsigned count, ...)
{
    std::bitset<19> mask;
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
static std::bitset<144> makeArtifactComponentMask(unsigned count, ...)
{
    std::bitset<144> mask;
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

// The fifteen allowable-slot classes searched by the traits initializer,
// recovered from 0x44cc00. Class 0 is empty; classes 7 and 9 are the ring
// pair (6, 7) and misc/backpack group (9, 10, 11, 12, 18).
// Retail retains _Tidy(0) for the first entry. Neither default construction
// nor explicit/implicit zero-value construction reproduces that boundary:
// all expand it and produce a 326-byte cinit versus retail's 325 bytes, with
// later copy-register changes.
// VC6's bitset copy/destructor are implicit. Retail reuses one four-byte
// temporary for all fifteen values, with no cleanup or additional owner;
// the register differences do not establish a missing copy helper.
// The neighboring combination cinit matches all 664 bytes / 24 relocations.
// Both cinits remain outside the ordinary function-score inventory.
DATA(0x00693898)
const std::bitset<19> g_artifactSlotMasks[15] = {
    std::bitset<19>(),
    makeArtifactSlotMask(1, 0),
    makeArtifactSlotMask(1, 1),
    makeArtifactSlotMask(1, 2),
    makeArtifactSlotMask(1, 3),
    makeArtifactSlotMask(1, 4),
    makeArtifactSlotMask(1, 5),
    makeArtifactSlotMask(2, 6, 7),
    makeArtifactSlotMask(1, 8),
    makeArtifactSlotMask(5, 9, 10, 11, 12, 18),
    makeArtifactSlotMask(1, 13),
    makeArtifactSlotMask(1, 14),
    makeArtifactSlotMask(1, 15),
    makeArtifactSlotMask(1, 16),
    makeArtifactSlotMask(1, 17),
};

DATA(0x00660b64)
const TArtifactSlotTraits (&g_artifactSlotTraits)[19] = g_artifactSlotTraitsStorage;

DATA(0x00660b68)
const TArtifactTraits (&g_artifactTraits)[144] = g_artifactTraitsStorage;

DATA(0x00660b6c)
const TCombinationArtifact* g_combinationArtifacts = g_combinationArtifactTable;

static void initializeArtifactTraits(int id,
    const TSpreadsheetResource::TStringVector& resource);

// E:\gamedcs\artifact.cpp:56, dc 0x4fec0. Complete requires 146 rows,
// packs names/descriptions into one owned buffer, derives the slot classes,
// applies disabled/spell/combination metadata, and loads 19 slot names.
// Keep the DC-proven static InitializeArtifactTraits boundary and its two
// parameters. Complete's pooled string copies belong to the caller; adding
// a char*& buffer parameter to the helper is a weaker retail hypothesis.
// The resource guards and static array owners reproduce retail cleanup.
// Restoring strcpy at the pooled copies makes VC6 scan each source twice:
// once for the cursor's length and again inside intrinsic strcpy. Retail
// scans once and reuses that count for copying, supporting memcpy here.
// DC uses Dispose and per-string TAutoStrPtr arrays. Complete's resources
// dispose through vtable+4; its two pooled owners have an ownership byte and
// pointer, proved by the retained 0x44d340/0x44d360 destructors. The canonical
// TResourcePtr/TAutoArrayPtr express those Windows lifetimes. The older set/get
// names do not imply additional Windows allocations or disposal calls.

// Raw spreadsheet queries retain the older source's pointer interface;
// the separate TResourcePtr guards own each Windows phase's real cleanup.
// Either raw-query phase gives 83.8990%; both keep that result. The 16-state
// pool-cursor reuse control gives 82.7881..83.4951% and changes no established
// source fact, so retain the owner queries at the pool check and cursor setup.
// Complete's added traits sizing pass uses the existing cell accessor.
// This recovers the retail row-base reload, both row loads and loop schedule:
// its 83 bytes differ only in two displacements for one length temporary.
// The four-state cell/row-accessor control gives 86.8713% for traits only,
// 85.5960% for both sizing passes and 83.8990% for slots only or neither.
// The original spelling is inferred; the canonical interfaces stay intact.
// Complete's retained bitset<19>::set call supports a direct set rather than
// DC's old proxy assignment. Retail also obtains resource[21] once and keeps
// its first byte in AL through the class chain; a named class-cell pointer
// preserves the four tests while reproducing that single vector subscript.
// Keeping the bitset declaration before the cost statement improves the
// retail schedule even though C2 emits atoi first. The Complete-only default
// fields stay in the caller immediately after the helper, preserving their
// emitted position while leaving the older helper's ownership intact.
// Residual (89.9743%): nested bitset<19> _Tidy and equality calls stay out of
// line where retail expands them. Late range-error construction reaches the
// retail _Grow call, but retains a string copy constructor where retail calls
// assign. Candidate/retail have 78/81 blocks and 16/13 retained calls.
// The combination loop now has retail's owner/offset end checks, set-bit
// search, returned-iterator copy and retained bitset<144>::test call.

// Controls: removing the old unsupported dead printf carrier alone gives
// 76.87327%. Restoring the helper with pooled copies and a char*& gives
// 74.41782%; unsigned flag loops give 73.65148%. The iterator search raises
// that to 80.19802%; moving pooled copies into the caller preserves the DC
// signature and gives 80.821785%. The find_if identity predicate restores
// test al,al rather than cmp al,1; naming the component traits restores the
// shared address (79.867325%). Loading both string pointers before strlen
// gives 81.376236%; reversing their strlen addends and removing unused stdio
// are byte-neutral. These source boundaries replace the fabricated carrier.
// Earlier named-row/declaration/volatile-accumulator probes did not resolve
// the first-loop hoist; the canonical cell accessor above does.
// DC public ?InitializeArtifactTraitsTable@@YA_NXZ proves bool; the SH4
// dossier renders its byte-sized procedure result as unsigned char. Complete
// returns only AL 0/1, and the sole kb caller tests that Boolean result.
VA(0x0044cd50, 0x5E8)  // anchor-strings/caller, dc 0x4fec0
bool initializeArtifactTraitsTable()
{
    {
        TSpreadsheetResource* traitsSheet = ResourceManager::getSpreadsheet(
            DATA_COMPGEN(0x00660b80, artifactTraitsSpreadsheetName,
                         "artraits.txt"));
        TResourcePtr<TSpreadsheetResource> traitsSheetGuard(traitsSheet);
        if (!traitsSheet || traitsSheet->getNumberOfRows() < 146) {
            return 0;
        }

        unsigned stringBytes = 0;
        int row;
        for (row = 2; row < 146; ++row) {
            const char* name = traitsSheet->getSpreadsheet(row, 0);
            const char* description = traitsSheet->getSpreadsheet(row, 22);
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
        for (row = 2; row < 146; ++row) {
            const TSpreadsheetResource::TStringVector& values =
                traitsSheet->getRow(row);
            TArtifactTraits& traits = g_artifactTraitsStorage[row - 2];
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

            initializeArtifactTraits(row - 2, values);
            traits.m_disabled = 0;
            traits.m_comboType = traits.m_targetCombo = -1;
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
        TConstBitsetIterator<144> current(combination.m_components, 0);
        TConstBitsetIterator<144> end(combination.m_components, 144);
        for (; (current = std::find_if(current, end, TBitIsSet())) != end;
             ++current) {
            int component = current.position();
            TArtifactTraits& componentTraits = g_artifactTraitsStorage[component];
            componentTraits.m_targetCombo = combo;
            assembled.m_cost += componentTraits.m_cost;
        }
    }

    {
        TSpreadsheetResource* slotsSheet = ResourceManager::getSpreadsheet(
            DATA_COMPGEN(0x00660b70, artifactSlotsSpreadsheetName,
                         "artslots.txt"));
        TResourcePtr<TSpreadsheetResource> slotsSheetGuard(slotsSheet);
        if (!slotsSheet || slotsSheet->getNumberOfRows() < 19) {
            return 0;
        }

        unsigned stringBytes = 0;
        int slot;
        for (slot = 0; slot < 19; ++slot)
            stringBytes += strlen(slotsSheet->getRow(slot)[0]) + 1;

        VA_COMPGEN(0x0044d340, 0x16, STATIC_DTOR, artifactSlotStrings)
        DATA(0x00694c98)
        static TAutoArrayPtr<char> artifactSlotStrings(new char[stringBytes]);
        if (!artifactSlotStrings.get())
            return 0;

        char* destination = artifactSlotStrings.get();
        for (slot = 0; slot < 19; ++slot) {
            const char* source = slotsSheet->getRow(slot)[0];
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

// DC's signature is void(int, const vector<char*>&). Retail replaces the
// individual name/description allocations with the caller's pooled copies.
// DC132..149 calls operator[], reference::operator=(bool), and its empty
// destructor for each of 18 slots. Complete replaces those statements with
// a 19-slot counted loop. Its retained call at 0x44cf32 is bitset::set; using
// that direct interface removes the unmatched proxy assignment and raises the
// current reconstruction from 86.8713% to 88.2178% before the later changes.
// Direct field subscripts instead of the traits reference give 79.619804%;
// copy-initializing the mask gives 81.19604%; spelling !(mask == other) is
// byte-neutral. Keep the default mask constructor and ordinary inequality.
// DC153/155/157/159 each performs a vector subscript before testing R/J/N/T.
// Complete retail instead has one subscript and one byte load. Repeating the
// tests through a named classCell reconciles the old statement pattern with
// the retail dataflow and raises 89.3248% to 89.3644%.
// Naming the consumed slot proxy in the earlier owner-query model gave
// 80.5782% and kept all three unwanted calls, failing its expansion prediction.
// Explicit successful-match breaks in either mask search leave all 1504
// initializer bytes unchanged, including both retained equality calls.
// Moving the four Complete-only defaults to the caller and chaining their two
// integer sentinels reaches 89.3861%. Declaring the bitset before the cost
// assignment then reaches 89.9743%. The old explicit bitset<19> instantiation
// was byte-flat here and unnecessary: real uses retain the exact set/_Tidy
// COMDATs, so no unsupported template-enrollment declaration remains.
static void initializeArtifactTraits(int id,
    const TSpreadsheetResource::TStringVector& resource)
{
    TArtifactTraits& traits = g_artifactTraitsStorage[id];
    std::bitset<19> allowableSlots;
    int column;
    int mask;
    traits.m_cost = atoi(resource[1]);
    for (column = 2; column < 21; ++column) {
        int bit = g_artifactSlotColumnBits[column - 2];
        allowableSlots.set(bit, resource[column][0] != 0
            && resource[column][0] != ' ');
    }
    mask = 0;
    while (allowableSlots != g_artifactSlotMasks[mask])
        ++mask;
    traits.m_allowableSlotMask = mask;

    const char* classCell = resource[21];
    if (classCell[0] == 'R')
        traits.m_artifactClass = 16;
    else if (classCell[0] == 'J')
        traits.m_artifactClass = 8;
    else if (classCell[0] == 'N')
        traits.m_artifactClass = 4;
    else if (classCell[0] == 'T')
        traits.m_artifactClass = 2;
    else
        traits.m_artifactClass = 1;
}

VA_COMPGEN(0x0044D380, 0x60, BITSET_SET, Bitset19)

VA_COMPGEN(0x0044d3e0, 0x17, BITSET_TIDY, Bitset19)

VA_COMPGEN(0x0044d400, 0xCB, BITSET_XRAN, Bitset19)
