// artifact.cpp - E:\gamedcs\artifact.cpp (compiland artifact.obj)
// helper and ownership wrappers into the table initializer. The adjacent
// bitset bodies are Dinkumware COMDATs, not source claims.
#include <va.h>
#include <bitset>
#include <algorithm>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "artifact.h"
#include "const_bitset_iterator.h"
#include "ownership.h"
#include "resourcemanager.h"
#include "textresource.h"

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

// These two tables are initialized by artifact.obj's excluded cinit family
// at 0x44c700..0x44cd4f. Their storage and source initializers are a separate
// admission; declaring the byte-proven addresses here makes this function's
// data references authoritative without pretending the cinits are claims.
DATA(0x006939f8)
static ArtifactTraits g_artifactTraitsStorage[144];

DATA(0x00694bf8)
static ArtifactSlotTraits g_artifactSlotTraitsStorage[19];

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

// The twelve Shadow of Death combination artifacts, ids 129..140, each with
// the component set the assembled artifact consumes. Read straight out of
// the cinit at 0x44c960: the 24-byte record is built in a stack temporary
// (the id dword plus the builder's five-word result) and copied to
// 0x6938d8 + 24*i by a six-dword `rep movsd`, which is the record's own
// two-argument constructor inlined plus its implicit copy.
DATA(0x006938d8)
const CombinationArtifact g_combinationArtifactTable[12] = {
    CombinationArtifact(0x81,
        makeArtifactComponentMask(6, 0x24, 0x21, 0x23, 0x1f, 0x20, 0x22)),
    CombinationArtifact(0x82, makeArtifactComponentMask(3, 0x36, 0x37, 0x38)),
    CombinationArtifact(0x83, makeArtifactComponentMask(3, 0x5f, 0x60, 0x5e)),
    CombinationArtifact(0x84,
        makeArtifactComponentMask(4, 0x14, 0x08, 0x1a, 0x0e)),
    CombinationArtifact(0x85,
        makeArtifactComponentMask(5, 0x76, 0x77, 0x78, 0x79, 0x7a)),
    CombinationArtifact(0x86,
        makeArtifactComponentMask(9, 0x2c, 0x2b, 0x2a, 0x26, 0x27,
                                  0x25, 0x2d, 0x29, 0x28)),
    CombinationArtifact(0x87,
        makeArtifactComponentMask(4, 0x18, 0x0c, 0x1e, 0x12)),
    CombinationArtifact(0x88, makeArtifactComponentMask(2, 0x7b, 0x47)),
    CombinationArtifact(0x89, makeArtifactComponentMask(3, 0x3c, 0x3d, 0x3e)),
    CombinationArtifact(0x8a, makeArtifactComponentMask(3, 0x49, 0x4a, 0x4b)),
    CombinationArtifact(0x8b, makeArtifactComponentMask(3, 0x4c, 0x4e, 0x4d)),
    CombinationArtifact(0x8c,
        makeArtifactComponentMask(4, 0x6f, 0x6d, 0x6e, 0x71)),
};

// The fifteen allowable-slot classes InitializeArtifactTraitsTable searches
// linearly, read out of the cinit at 0x44cc00. Class 0 is the empty set and
// is the one entry retail builds with the plain default constructor, which
// is why `bitset<19>::_Tidy` survives out of line at 0x44d3e0; the two
// multi-slot classes are the ring pair (6, 7) and the misc/backpack group
// (9, 10, 11, 12, 18).
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
const ArtifactSlotTraits* g_artifactSlotTraits = g_artifactSlotTraitsStorage;

DATA(0x00660b68)
const ArtifactTraits* g_artifactTraits = g_artifactTraitsStorage;

DATA(0x00660b6c)
const CombinationArtifact* g_combinationArtifacts = g_combinationArtifactTable;

static void initializeArtifactTraits(int id,
    const SpreadsheetResource::TStringVector& resource);

// E:\gamedcs\artifact.cpp:56, dc 0x4fec0. Complete requires 146 rows,
// packs names/descriptions into one owned buffer, derives the slot classes,
// applies disabled/spell/combination metadata, and loads 19 slot names.
// Keep the DC-proven static InitializeArtifactTraits boundary and its two
// parameters. Complete's pooled string copies belong to the caller; adding
// a char*& buffer parameter to the helper is a weaker retail hypothesis.
// The resource guards and static array owners reproduce retail cleanup.

// Residual (81.376236%): the nested bitset<19> _Tidy and equality calls stay
// out of line where retail expands them. The first size loop still hoists
// the sheet's row-vector base, and late range-error construction differs.
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
// the first-loop hoist; retain the direct row accesses and ordinary locals.
VA(0x0044cd50, 0x5E8)  // anchor-strings/caller, dc 0x4fec0
unsigned char initializeArtifactTraitsTable()
{
    {
        ResourcePtr<SpreadsheetResource> traitsSheet(
            ResourceManager::getSpreadsheet(
                DATA_COMPGEN(0x00660b80, artifactTraitsSpreadsheetName,
                             "artraits.txt")));
        if (!traitsSheet.get() || traitsSheet->getNumberOfRows() < 146) {
            return 0;
        }

        unsigned stringBytes = 0;
        int row;
        for (row = 2; row < 146; ++row) {
            const char* name = traitsSheet->getRow(row)[0];
            const char* description = traitsSheet->getRow(row)[22];
            stringBytes += strlen(description) + strlen(name) + 2;
        }

        DATA_COMPGEN_GUARD(0x006938d4, artifactStringsGuard, artifactStrings)
        VA_COMPGEN(0x0044d360, 0x16, STATIC_DTOR, artifactStrings)
        DATA(0x00694c90)
        static AutoArrayPtr<char> artifactStrings(new char[stringBytes]);
        if (!artifactStrings.get())
            return 0;

        char* destination = artifactStrings.get();
        for (row = 2; row < 146; ++row) {
            const SpreadsheetResource::TStringVector& values =
                traitsSheet->getRow(row);
            ArtifactTraits& traits = g_artifactTraitsStorage[row - 2];
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
        }
    }

    unsigned artifactId;
    for (artifactId = 0; artifactId < 3; ++artifactId)
        g_artifactTraitsStorage[g_disabledArtifacts[artifactId]].m_disabled = 1;
    for (artifactId = 0; artifactId < 9; ++artifactId)
        g_artifactTraitsStorage[g_spellGivingArtifacts[artifactId]].m_givesSpells = 1;

    int combo;
    for (combo = 0; combo < 12; ++combo) {
        const CombinationArtifact& combination =
            g_combinationArtifacts[combo];
        ArtifactTraits& assembled =
            g_artifactTraitsStorage[combination.m_artifactId];
        assembled.m_comboType = combo;
        assembled.m_cost = 0;
        ConstBitsetIterator<144> current(combination.m_components, 0);
        ConstBitsetIterator<144> end(combination.m_components, 144);
        for (; (current = std::find_if(current, end, BitIsSet())) != end;
             ++current) {
            int component = current.position();
            ArtifactTraits& componentTraits = g_artifactTraitsStorage[component];
            componentTraits.m_targetCombo = combo;
            assembled.m_cost += componentTraits.m_cost;
        }
    }

    {
        ResourcePtr<SpreadsheetResource> slotsSheet(
            ResourceManager::getSpreadsheet(
                DATA_COMPGEN(0x00660b70, artifactSlotsSpreadsheetName,
                             "artslots.txt")));
        if (!slotsSheet.get() || slotsSheet->getNumberOfRows() < 19) {
            return 0;
        }

        unsigned stringBytes = 0;
        int slot;
        for (slot = 0; slot < 19; ++slot)
            stringBytes += strlen(slotsSheet->getRow(slot)[0]) + 1;

        VA_COMPGEN(0x0044d340, 0x16, STATIC_DTOR, artifactSlotStrings)
        DATA(0x00694c98)
        static AutoArrayPtr<char> artifactSlotStrings(new char[stringBytes]);
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
// destructor for each of 18 slots. Retail's 19-slot counted loop is a port
// difference, but its 0x44cf32 set call does not disprove proxy assignment:
// Dinkumware's reference assignment delegates to that canonical set body.
// Restore the proxy expression. Three reproduced source forms emit three
// objects: direct set 81.3762%, proxy with named bool 80.8594%, and proxy
// expression 81.9921%, with all seven exact siblings preserved. The proxy
// call still needs to expand naturally to recover retail's named call stream.
// Direct field subscripts instead of the traits reference give 79.619804%;
// copy-initializing the mask gives 81.19604%; spelling !(mask == other) is
// byte-neutral. Keep the default mask constructor and proxy interface.
// DC153/155/157/159 each calls vector::operator[] and tests the first
// character against R/J/N/T. Preserve four resource[21][0] expressions;
// the port used column 20 before Complete added the nineteenth slot column.
// Caching the character erased these source calls and gave 81.9921%.
// The two-form control reproduces 80.8218% with the recovered calls and
// preserves all seven exact siblings. A lower score does not contradict
// the positive source-call evidence; the cached form remains a failed lead.
// Passive VC6 traces keep the outer caller at cb=1056/budget=2112. Restoring
// the reads changes this helper from cb=330 to 351 and its child budget from
// 65 to 64. The proxy assignment still exceeds its remaining budget (43 vs
// 23), while _Tidy and equality remain rejected at the next nesting level.
// The residual is not explained by the removed cache alone.
static void initializeArtifactTraits(int id,
    const SpreadsheetResource::TStringVector& resource)
{
    ArtifactTraits& traits = g_artifactTraitsStorage[id];

    traits.m_cost = atoi(resource[1]);
    std::bitset<19> allowableSlots;
    int column;
    for (column = 2; column < 21; ++column) {
        int bit = g_artifactSlotColumnBits[column - 2];
        allowableSlots[bit] = resource[column][0] != 0
            && resource[column][0] != ' ';
    }
    int mask = 0;
    while (allowableSlots != g_artifactSlotMasks[mask])
        ++mask;
    traits.m_allowableSlotMask = mask;

    if (resource[21][0] == 'R')
        traits.m_artifactClass = 16;
    else if (resource[21][0] == 'J')
        traits.m_artifactClass = 8;
    else if (resource[21][0] == 'N')
        traits.m_artifactClass = 4;
    else if (resource[21][0] == 'T')
        traits.m_artifactClass = 2;
    else
        traits.m_artifactClass = 1;
    traits.m_disabled = 0;
    traits.m_comboType = -1;
    traits.m_targetCombo = -1;
    traits.m_givesSpells = 0;
}

#if 0  // @carcass: Dreamcast-only/out-of-line header and STL emissions

// E:\gamedcs\TextResource.h:108
DC_ONLY(0x5088c, 0x18)
int SpreadsheetResource::getNumberOfRows()
{
    // @stub
}

// E:\gamedcs\TextResource.h:128
DC_ONLY(0x508a4, 0x18)
const std::vector<char* SpreadsheetResource::getRow(int r)
{
    // @stub
}

// E:\gamedcs\artifact.cpp:33
DC_ONLY(0x508bc, 0x8)
void `anonymous namespace'::AutoStrPtr::AutoStrPtr()
{
    // @stub
}

// E:\gamedcs\artifact.cpp:34
DC_ONLY(0x508c4, 0x18)
void `anonymous namespace'::AutoStrPtr::~AutoStrPtr()
{
    // @stub
}

// E:\gamedcs\artifact.cpp:36
DC_ONLY(0x508dc, 0x4)
void `anonymous namespace'::AutoStrPtr::set(char* pStr)
{
    // @stub
}

// E:\gamedcs\artifact.cpp:38
DC_ONLY(0x508e0, 0x4)
char* `anonymous namespace'::AutoStrPtr::get()
{
    // @stub
}

// E:\gamedcs\artifact.cpp:49
DC_ONLY(0x508e4, 0x20)
void ArtifactTraits::ArtifactTraits()
{
    // @stub
}

// ..\stlport\stl_bitset.h:414
DC_ONLY(0x50904, 0x20)
void std::bitset<18,unsigned long>::bitset<18,unsigned long>()
{
    // @stub
}

// ..\stlport\stl_bitset.h:564
DC_ONLY(0x50924, 0x20)
std::bitset<18,unsigned std::bitset<18,unsigned long>::operator[](__$ReturnUdt, unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_bitset.h:376
DC_ONLY(0x50944, 0x4)
void std::bitset<18,unsigned long>::reference::~reference()
{
    // @stub
}

// ..\stlport\stl_bitset.h:379
DC_ONLY(0x50948, 0x3C)
std::bitset<18,unsigned* std::bitset<18,unsigned long>::reference::operator=(unsigned char __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x50984, 0xC)
unsigned std::vector<std::vector<char *,std::allocator<char *> > *,std::allocator<std::vector<char *,std::allocator<char *> > *> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:204
DC_ONLY(0x50990, 0x20)
std::vector<char** std::vector<std::vector<char *,std::allocator<char *> > *,std::allocator<std::vector<char *,std::allocator<char *> > *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_bitset.h:107
DC_ONLY(0x509b0, 0x1C)
void std::_Base_bitset<1,unsigned long>::_Base_bitset<1,unsigned long>()
{
    // @stub
}

// ..\stlport\stl_bitset.h:370
DC_ONLY(0x509cc, 0x34)
void std::bitset<18,unsigned long>::reference::reference(std::bitset<18,unsigned* __b, unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_vector.h:180
DC_ONLY(0x50a00, 0x4)
std::vector<char** std::vector<std::vector<char *,std::allocator<char *> > *,std::allocator<std::vector<char *,std::allocator<char *> > *> >::begin()
{
    // @stub
}

// ..\stlport\stl_bitset.h:120
DC_ONLY(0x50a04, 0x1C)
unsigned long std::_Base_bitset<18,unsigned long>::_S_maskbit(unsigned __pos)
{
    // @stub
}

// ..\stlport\stl_bitset.h:117
DC_ONLY(0x50a20, 0x8)
unsigned std::_Base_bitset<18,unsigned long>::_S_whichbit(unsigned __pos)
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x0044D380, 0x60, BITSET_SET, Bitset19)

VA_COMPGEN(0x0044d3e0, 0x17, BITSET_TIDY, Bitset19)

VA_COMPGEN(0x0044d400, 0xCB, BITSET_XRAN, Bitset19)
