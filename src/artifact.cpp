// artifact.cpp - E:\gamedcs\artifact.cpp (compiland artifact.obj)
// HAND-OWNED after admission. Retail Complete folds the Dreamcast helper,
// ownership wrappers and traits constructor into one surviving source body;
// the adjacent bitset bodies are Dinkumware COMDATs, not source claims.
#include <va.h>
#include <bitset>
#include <stdlib.h>
#include <string.h>

#include "artifact.h"
#include "ownership.h"
#include "resourcemanager.h"
#include "textresource.h"

namespace {

// Column 2 of artraits.txt names the final physical slot, while column 20
// names slot zero. The one exception is column 7, the two-handed weapon
// class (slot 18), which sits between slots 13 and 12 in the file.
DATA(0x0063b940)
static const int kArtifactSlotColumnBits[19] = {
    17, 16, 15, 14, 13, 18, 12, 11, 10, 9,
    8, 7, 6, 5, 4, 3, 2, 1, 0
};

DATA(0x0063b98c)
static const int kDisabledArtifacts[3] = { 141, 142, 143 };

DATA(0x0063b998)
static const int kSpellGivingArtifacts[9] = {
    1, 128, 123, 124, 86, 87, 88, 89, 135
};

// These two tables are initialized by artifact.obj's excluded cinit family
// at 0x44c700..0x44cd4f. Their storage and source initializers are a separate
// admission; declaring the byte-proven addresses here makes this function's
// data references authoritative without pretending the cinits are claims.
DATA(0x006939f8)
static TArtifactTraits aArtifactTraits[144];

DATA(0x00694bf8)
static TArtifactSlotTraits aArtifactSlotTraits[19];

} // namespace

DATA(0x00660b64)
const TArtifactSlotTraits* akArtifactSlotTraits = aArtifactSlotTraits;

DATA(0x00660b68)
const TArtifactTraits* akArtifactTraits = aArtifactTraits;

DATA(0x00660b6c)
const TCombinationArtifact* gCombinationArtifacts = aCombinationArtifacts;

// E:\gamedcs\artifact.cpp:56
// Retail requires 146 rows (two headers plus 144 artifacts), packs every
// name and description into one owned allocation, derives each artifact's
// 15-way allowable-slot class from its 19 spreadsheet columns, then applies
// Complete's disabled/spell/combo metadata and loads the 19 slot names.
// The two TResourcePtr instances account for the retail EH state and
// failure-only Dispose paths; the static array owners account for 0x44d340
// and 0x44d360.
// Residual (75.01%): all 81 retail blocks are semantically represented, but
// VC6 makes two linked choices differently: retail leaves the nested
// bitset<19>/bitset<144> operations out of line after folding its source
// helper, and gives three early failures distinct returns where this spelling
// tail-merges them. Direct set/operator[] spellings and stack-lifetime probes
// measured the plateau; the exact static destructors remain independent.
// 2026-08-14 triage: predict-inline's UNDER/OVER rows all NAME-PAIR (operator
// new <-> exe_new, the two bitset _Xran <-> the unnamed 0x4d380/0x4d4d0 rows,
// basic_string _Grow/assign/_Eos <-> the 0x4860/0x4a90 rows), so the inliner is
// NOT the wall despite diagnose routing there; base emits 14 out-of-line calls
// to retail's 13. The concrete unexplored delta is in the stringBytes loop:
// retail keeps `traitsSheet` itself in ESI and RELOADS `[esi+0x20]` (the row
// vector's begin) on every iteration while homing stringBytes at [ebp-0x20];
// our CL does the reverse - it LICM-hoists the begin pointer into a frame slot
// and keeps stringBytes in ESI. Retail consequently loads the row element twice
// per iteration (`mov ecx,[edx+eax-4]` / `mov eax,[edx+eax-4]`, one per GetRow
// call) where ours CSEs it to one load. Same enregistration-choice family that
// diff.cpp's CDiffFile::Apply turned out to be (68.96 -> 99.64 there).
VA(0x0044cd50, 0x5E8)  // anchor-strings/caller, dc 0x4fec0
unsigned char InitializeArtifactTraitsTable()
{
    {
        TResourcePtr<TSpreadsheetResource> traitsSheet(
            ResourceManager::GetSpreadsheet(
                DATA_COMPGEN(0x00660b80, artifactTraitsSpreadsheetName,
                             "artraits.txt")));
        if (!traitsSheet.get() || traitsSheet->GetNumberOfRows() < 146)
            return 0;

        unsigned stringBytes = 0;
        int row;
        for (row = 2; row < 146; ++row) {
            stringBytes += strlen(traitsSheet->GetRow(row)[0])
                + strlen(traitsSheet->GetRow(row)[22]) + 2;
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
                traitsSheet->GetRow(row);
            TArtifactTraits& traits = aArtifactTraits[row - 2];

            const char* source = values[0];
            unsigned length = strlen(source) + 1;
            memcpy(destination, source, length);
            traits.name = destination;
            destination += length;

            source = values[22];
            length = strlen(source) + 1;
            memcpy(destination, source, length);
            traits.description = destination;
            destination += length;

            traits.cost = atoi(values[1]);
            std::bitset<19> allowableSlots;
            int column;
            for (column = 2; column < 21; ++column) {
                int bit = kArtifactSlotColumnBits[column - 2];
                bool allowed = values[column][0] != 0
                    && values[column][0] != ' ';
                allowableSlots[bit] = allowed;
            }
            int mask = 0;
            while (allowableSlots != aArtifactSlotMasks[mask])
                ++mask;
            traits.allowableSlotMask = mask;

            char artifactClass = values[21][0];
            if (artifactClass == 'R')
                traits.artifactClass = 16;
            else if (artifactClass == 'J')
                traits.artifactClass = 8;
            else if (artifactClass == 'N')
                traits.artifactClass = 4;
            else if (artifactClass == 'T')
                traits.artifactClass = 2;
            else
                traits.artifactClass = 1;
            traits.disabled = 0;
            traits.comboType = -1;
            traits.targetCombo = -1;
            traits.givesSpells = 0;
        }
    }

    int artifactId;
    for (artifactId = 0; artifactId < 3; ++artifactId)
        aArtifactTraits[kDisabledArtifacts[artifactId]].disabled = 1;
    for (artifactId = 0; artifactId < 9; ++artifactId)
        aArtifactTraits[kSpellGivingArtifacts[artifactId]].givesSpells = 1;

    int combo;
    for (combo = 0; combo < 12; ++combo) {
        const TCombinationArtifact& combination =
            gCombinationArtifacts[combo];
        TArtifactTraits& assembled =
            aArtifactTraits[combination.artifactId];
        assembled.comboType = combo;
        assembled.cost = 0;
        for (artifactId = 0; artifactId < 144; ++artifactId) {
            if (combination.components[artifactId]) {
                aArtifactTraits[artifactId].targetCombo = combo;
                assembled.cost += aArtifactTraits[artifactId].cost;
            }
        }
    }

    {
        TResourcePtr<TSpreadsheetResource> slotsSheet(
            ResourceManager::GetSpreadsheet(
                DATA_COMPGEN(0x00660b70, artifactSlotsSpreadsheetName,
                             "artslots.txt")));
        if (!slotsSheet.get() || slotsSheet->GetNumberOfRows() < 19)
            return 0;

        unsigned stringBytes = 0;
        int slot;
        for (slot = 0; slot < 19; ++slot)
            stringBytes += strlen(slotsSheet->GetRow(slot)[0]) + 1;

        VA_COMPGEN(0x0044d340, 0x16, STATIC_DTOR, artifactSlotStrings)
        DATA(0x00694c98)
        static TAutoArrayPtr<char> artifactSlotStrings(new char[stringBytes]);
        if (!artifactSlotStrings.get())
            return 0;

        char* destination = artifactSlotStrings.get();
        for (slot = 0; slot < 19; ++slot) {
            const char* source = slotsSheet->GetRow(slot)[0];
            unsigned length = strlen(source) + 1;
            memcpy(destination, source, length);
            aArtifactSlotTraits[slot].name = destination;
            destination += length;

            int mask = 0;
            while (!aArtifactSlotMasks[mask].test(slot))
                ++mask;
            aArtifactSlotTraits[slot].type = mask;
        }
    }
    return 1;
}

#if 0  // @carcass: Dreamcast-only/out-of-line header and STL emissions

// E:\gamedcs\artifact.cpp:112
DC_ONLY(0x50058, 0x7BA)
void InitializeArtifactTraits(int id, const std::vector<char* resource)
{
    // @stub
}

// E:\gamedcs\TextResource.h:108
DC_ONLY(0x5088c, 0x18)
int TSpreadsheetResource::GetNumberOfRows()
{
    // @stub
}

// E:\gamedcs\TextResource.h:128
DC_ONLY(0x508a4, 0x18)
const std::vector<char* TSpreadsheetResource::GetRow(int r)
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
