// artifact.cpp - E:\gamedcs\artifact.cpp (compiland artifact.obj)
// HAND-OWNED after admission. Retail Complete inlines the static traits
// helper and ownership wrappers into the table initializer. The adjacent
// bitset bodies are Dinkumware COMDATs, not source claims.
#include <va.h>
#include <bitset>
#include <algorithm>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "artifact.h"
#include "ownership.h"
#include "resourcemanager.h"
#include "textresource.h"

// Retail retains bitset<19>::_Tidy for the first slot-mask initializer.  An
// explicit class instantiation before this TU's first bitset use makes VC6
// emit that canonical Dinkumware COMDAT; the linker may discard the other
// unreferenced members independently.
template class std::bitset<19>;
namespace {

// Column 2 of artraits.txt names the final physical slot, while column 20
// names slot zero. The one exception is column 7, the two-handed weapon
// class (slot 18), which sits between slots 13 and 12 in the file.
// Before normalization: kArtifactSlotColumnBits.
DATA(0x0063b940)
static const int g_artifactSlotColumnBits[19] = {
    17, 16, 15, 14, 13, 18, 12, 11, 10, 9,
    8, 7, 6, 5, 4, 3, 2, 1, 0
};

// Before normalization: kDisabledArtifacts.
DATA(0x0063b98c)
static const int g_disabledArtifacts[3] = { 141, 142, 143 };

// Before normalization: kSpellGivingArtifacts.
DATA(0x0063b998)
static const int g_spellGivingArtifacts[9] = {
    1, 128, 123, 124, 86, 87, 88, 89, 135
};

// These two tables are initialized by artifact.obj's excluded cinit family
// at 0x44c700..0x44cd4f. Their storage and source initializers are a separate
// admission; declaring the byte-proven addresses here makes this function's
// data references authoritative without pretending the cinits are claims.
// Before normalization: aArtifactTraits.
DATA(0x006939f8)
static TArtifactTraits g_artifactTraitsStorage[144];

// Before normalization: aArtifactSlotTraits.
DATA(0x00694bf8)
static TArtifactSlotTraits g_artifactSlotTraitsStorage[19];

} // namespace

// The two table builders. They are retail rows of their own - file statics
// with no name of their own in the image, kept OUT of this file's unnamed
// namespace because VC6 mangles that one with the absolute source path -
// and the variadic signature is what fixes them: `count` gets a memory home
// because `va_start` takes its
// address, which is why retail writes the decremented counter back to
// `[ebp+0xc]` every iteration and never strength-reduces the walk. The bit
// default-constructed local is the inline `_Tidy` (one zero store for the
// 19-bit width, the five-word downward fill for the 144-bit one).
//
// Retail FuncInfo 0x649538 / 0x6495a0 each has one catch-all handler
// covering states 0..2. The handlers at 0x44c7e7 / 0x44c907 rethrow;
// va_end is a no-op on Win32 but belongs on both exit paths. Restoring
// that try/catch makes the bitset range throw expand into the loop.
// Subscript assignment then retains basic_string::assign at the right
// depth inside that throw. Both builders now match at 100%.
//
// Negative controls: without the catch, both set(i) and [i] = true stay
// at 22.16% / 32.15%; adding an explicit zero constructor or making the
// slot builder a function template is byte-flat. With the catch restored,
// set(i) reaches 67.91% for 19 bits but expands basic_string::assign too
// far. Earlier explicit-throw and union-word-view probes duplicated the
// bounds check or sank the throw after the epilogue. The old diagnosis
// that the EH frame was solely an inliner consequence was wrong: the
// retail try-block map independently proves the missing catch scope.
// Before normalization (function): MakeArtifactSlotMask.
VA(0x0044c720, 0x10B)  // anchor-callee the aArtifactSlotMasks cinit's 14 calls, retail-only file static
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

// Before normalization (function): MakeArtifactComponentMask.
VA(0x0044c830, 0x122)  // anchor-callee the aCombinationArtifacts cinit's 12 calls, retail-only file static
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
const TArtifactSlotTraits* g_artifactSlotTraits = g_artifactSlotTraitsStorage;

DATA(0x00660b68)
const TArtifactTraits* g_artifactTraits = g_artifactTraitsStorage;

DATA(0x00660b6c)
const TCombinationArtifact* g_combinationArtifacts = g_combinationArtifactTable;

// Retail 0x44d063..0x44d0bf compares both owner and offset with an end
// iterator, searches through bitset::test, copies the found offset, then
// checks the end again. This is a const traversal twin of bitset_iterator;
// its original source name is unknown. The combination pass is Complete-only.
template<size_t N>
class TConstBitsetIterator {
public:
    TConstBitsetIterator(const std::bitset<N>& bits, size_t position)
        : m_bits(&bits), m_position(position) {}
    bool operator*() const { return m_bits->test(m_position); }
    TConstBitsetIterator& operator++()
    {
        ++m_position;
        return *this;
    }
    bool operator!=(const TConstBitsetIterator& other) const
    {
        return m_bits != other.m_bits || m_position != other.m_position;
    }
    size_t position() const { return m_position; }
private:
    const std::bitset<N>* m_bits;
    size_t m_position;
};

// Identity predicate for the set-bit search. Retail tests the returned bool
// at 0x44d077; std::find(..., true) instead emits cmp al,1 / je.
struct TBitIsSet {
    bool operator()(bool value) const { return value; }
};

static void initializeArtifactTraits(int id,
    const TSpreadsheetResource::TStringVector& resource);

// E:\gamedcs\artifact.cpp:56, dc 0x4fec0. Complete requires 146 rows,
// packs names/descriptions into one owned buffer, derives the slot classes,
// applies disabled/spell/combination metadata, and loads 19 slot names.
// Keep the DC-proven static InitializeArtifactTraits boundary and its two
// parameters. Complete's pooled string copies belong to the caller; adding
// a char*& buffer parameter to the helper is a weaker retail hypothesis.
// The resource guards and static array owners reproduce retail cleanup.
//
// Residual (81.376236%): the nested bitset<19> _Tidy and equality calls stay
// out of line where retail expands them. The first size loop still hoists
// the sheet's row-vector base, and late range-error construction differs.
// The combination loop now has retail's owner/offset end checks, set-bit
// search, returned-iterator copy and retained bitset<144>::test call.
//
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
        TResourcePtr<TSpreadsheetResource> traitsSheet(
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
        TResourcePtr<TSpreadsheetResource> slotsSheet(
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

// Before normalization: InitializeArtifactTraits, static, dc 0x50058.
// DC's signature is void(int, const vector<char*>&). Retail replaces the
// individual name/description allocations with the caller's pooled copies,
// and its 0x44cf32 call has the checked bitset::set(size_t,bool) body.
// Direct field subscripts instead of the traits reference give 79.619804%;
// copy-initializing the mask gives 81.19604%; spelling !(mask == other) is
// byte-neutral. Keep the default mask constructor and canonical set call.
static void initializeArtifactTraits(int id,
    const TSpreadsheetResource::TStringVector& resource)
{
    TArtifactTraits& traits = g_artifactTraitsStorage[id];

    traits.m_cost = atoi(resource[1]);
    std::bitset<19> allowableSlots;
    int column;
    for (column = 2; column < 21; ++column) {
        int bit = g_artifactSlotColumnBits[column - 2];
        bool allowed = resource[column][0] != 0
            && resource[column][0] != ' ';
        allowableSlots.set(bit, allowed);
    }
    int mask = 0;
    while (allowableSlots != g_artifactSlotMasks[mask])
        ++mask;
    traits.m_allowableSlotMask = mask;

    const char* classCell = resource[21];
    char artifactClass = classCell[0];
    if (artifactClass == 'R')
        traits.m_artifactClass = 16;
    else if (artifactClass == 'J')
        traits.m_artifactClass = 8;
    else if (artifactClass == 'N')
        traits.m_artifactClass = 4;
    else if (artifactClass == 'T')
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
int TSpreadsheetResource::getNumberOfRows()
{
    // @stub
}

// E:\gamedcs\TextResource.h:128
DC_ONLY(0x508a4, 0x18)
const std::vector<char* TSpreadsheetResource::getRow(int r)
{
    // @stub
}

// E:\gamedcs\artifact.cpp:33
DC_ONLY(0x508bc, 0x8)
void `anonymous namespace'::TAutoStrPtr::TAutoStrPtr()
{
    // @stub
}

// E:\gamedcs\artifact.cpp:34
DC_ONLY(0x508c4, 0x18)
void `anonymous namespace'::TAutoStrPtr::~TAutoStrPtr()
{
    // @stub
}

// E:\gamedcs\artifact.cpp:36
DC_ONLY(0x508dc, 0x4)
void `anonymous namespace'::TAutoStrPtr::set(char* pStr)
{
    // @stub
}

// E:\gamedcs\artifact.cpp:38
DC_ONLY(0x508e0, 0x4)
char* `anonymous namespace'::TAutoStrPtr::get()
{
    // @stub
}

// E:\gamedcs\artifact.cpp:49
DC_ONLY(0x508e4, 0x20)
void TArtifactTraits::TArtifactTraits()
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

// COMDAT pairing: bitset<19>::_Xran - `cmp <reg>, 0x13` guards the call at
// 0x4d380 and at hero's HeroFn_004E2550, and artifact.obj is the only object
// that emits `?_Xran@?$bitset@$0BD@@`. The nineteen artifact slots.
// Artifact's slot-mask setup naturally emits this checked bit setter. The
// 19-bit bound and its retained _Xran call identify all 96 retail bytes.
VA_COMPGEN(0x0044D380, 0x60, BITSET_SET, Bitset19)

// COMDAT pairing: bitset<19>'s default `_Tidy`, the one out-of-line copy in
// this object. Its sole caller is the aArtifactSlotMasks cinit's class-0
// entry (`push 0 / lea ecx,[ebp-4] / call`); every other default
// construction in the unit is inlined.
VA_COMPGEN(0x0044d3e0, 0x17, BITSET_TIDY, Bitset19)

VA_COMPGEN(0x0044d400, 0xCB, BITSET_XRAN, Bitset19)
