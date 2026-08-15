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
// Residual (76.83%): all 81 retail blocks are semantically represented, but
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
// 2026-08-14 RESULT (75.01 -> 76.83): half of that diagnosis is now fixed by
// source. Evaluating the [22] strlen BEFORE the [0] strlen - which is the order
// both sides already EMIT them in - stops our CL common-subexpressioning the two
// GetRow(row) row-pointer loads, so we now emit retail's TWO `mov r,[edx+base]`
// loads, one per GetRow call. Retail's emission order is not the source order.
// What did NOT move, and is now measured rather than assumed:
//   - the LICM hoist itself. Retail keeps traitsSheet in ESI and reloads
//     [esi+0x20] per iteration; we spill the guard's already-computed _First to
//     [ebp-0x24] and reload it, which frees ESI for stringBytes and forces the
//     `jmp` peel into the loop (retail falls straight through). The zero pseudo
//     that the null checks and `stringBytes = 0` share is what wins ESI here;
//     retail gives that pseudo ECX and memory-homes the accumulator instead.
//   - EXHAUSTIVE NEGATIVES, all byte-flat at 76.8337: four loop forms (for /
//     while / do-while / `while (++row < 146)`); eight declaration orderings of
//     {stringBytes, row, destination, source, length, column, mask}; binding
//     traitsSheet.get() to a raw or `register` pointer local; `.get()->` at both
//     call sites; splitting the accumulate into two statements; folding the +2
//     as two +1s. Declaration order does not reach this allocator.
//   - MEASURED WORSE: `volatile unsigned stringBytes` 71.79 and a volatile-ref
//     `+=` 73.61 (they do memory-home the accumulator, but add a reload at the
//     `new char[stringBytes]` push that retail does not have); splitting the
//     guard into two `if`s 73.83; hoisting GetNumberOfRows() to a local 72.66;
//     declaring the bitset before the loop 74.15.
// The `push 0x8`/`push 0x0` prologue row is NOT a defect: it is the delinker
// symbol+addend split documented in initialize.cpp - we encode disp32=0 +
// reloc($L12234), the delinked target disp32=8 + reloc(...unwind03). Identical
// once linked, and PROVEN not to be scored: EH-bearing functions that already
// sit at 100.0 (button::button, ~button, ~textButton, ~type_func_button,
// armyGroup::SplitArmy, ~TSplitWindow) all carry the same 0x0-vs-0xb/0x8 split.
// Still open and structural: at the inlined artslots.txt GetSpreadsheet, retail
// expands basic_string::_Eos in line (`mov ecx,[ebp-0x3c]; mov [ebp-0x38],ebx;
// mov byte [ecx+ebx],0`) where we emit an out-of-line call - a genuine /Ob2
// budget divergence, and the reason retail has 3 rets to our 2.
//
// 2026-08-14 TWO-AXIS /Ob2 SWEEP - the "EXHAUSTIVE NEGATIVES, all byte-flat"
// verdict above is WRONG on the axis it never tested. Every negative listed
// there is a SPELLING at constant statement mass; `budget = 2 * cb(caller)` is
// the other axis and it is live here. Byte-inert pad statements at the head of
// the body x xx_nop sites before the last statement:
//     M     0..6      7..12     20..48
//     any k 76.8337   80.4733   79.1525
// - i.e. +3.64 sits at exactly seven to twelve statements of extra front-end
// mass, at every k (the divisor axis really is dead here, which is why the
// one-axis sweep found nothing). That is the missing budget for the `_Eos`
// expansion described above.
// The honest supply is NOT yet found, and the reason is worth recording: a
// named ALIAS local is not worth the same C1 mass as a pad dead store. Nine
// byte-inert alias locals ahead of the artslots block - a `traitsSheet.get()`
// pointer, the class cell, the combination component array, the slot-bit
// table, the combination table, the traits table, the slot-mask table, the
// spell-giver list and a row-vector reference - stack to 76.8733 and never
// reach the 80.4733 plateau, where seven dead stores do. Only the class cell
// is landed below (76.8337 -> 76.8733); the rest are inert noise and were
// reverted. What this body needs is seven statements that CARRY something,
// and the reconstruction has no room for them yet.
// Measured and byte-COSTLY while hunting (so not mass at any count): splitting
// the first loop's two strlens into named lengths 74.3842, a named reference
// per iteration in the disabled/spell-giver loops 74.7723, naming the bitset
// loop's cell 73.3683, naming the copy loop's source lengths 75.8257/75.9248,
// and naming the combination's component row 76.6792.
//
// 2026-08-14 THE SOURCE HELPER IS REAL, AND IT IS NOT THE SEVEN STATEMENTS.
// The DC roster row that this file's header dismisses as "folded" is
// `InitializeArtifactTraits` at dc 0x50058: **static**, 2 parameters, 1978 B,
// against InitializeArtifactTraitsTable's own 294 B. So on Dreamcast the table
// function is a shell and ALL the per-row work lives in a one-call-site static
// - which under /Ob2 inlines and vanishes, leaving retail's single 1512-byte
// body. Reintroducing it (`static void InitializeArtifactTraits(int id, const
// TSpreadsheetResource::TStringVector& values, char*& destination)` holding the
// whole per-row block, called once from the row loop) is the ONLY spelling
// found that reproduces retail's bitset emission EXACTLY: at
// `allowableSlots.set(bit, allowed)` plus two spare candidate sites, our obj
// calls BOTH `?set@?$bitset@$0BD@@` and `?test@?$bitset@$0JA@@` out of line -
// i.e. the two artifact.obj COMDATs the carve names game_4cd50_sub00_4d380 and
// game_4cd50_sub01_4d4d0, which no flat spelling of this body has ever produced
// together (flat gets one or the other, never both). `.set(bit, allowed)` is
// therefore retail's spelling, not `allowableSlots[bit] = allowed` - retail
// calls `set`, we call `??4reference@?$bitset@$0BD@@`.
// It still does not pay. Full mass x sites grid over the helper form
// (M 0..64 pad statements x k 0..14):
//     helper + `[bit] =`   74.8950 flat, ceiling 79.5723 at M=16..28, k=0
//     helper + `.set(...)` 74.4178 flat, ceiling 79.6000 at M=12..20, k=0
// against 76.8733 flat / 80.4733 titrated without it. Every cell of both grids
// is below the corresponding flat cell of the current spelling, so the helper
// is NOT landed: it is structurally right about the bitsets and wrong about
// everything the extra frame costs. Whatever the seven statements are, they are
// not "put the per-row block back in its own function".
// Also measured this round, and a real retail fact even though it does not pay:
// the disabled and spell-giver loops use an UNSIGNED induction variable in
// retail (`cmp eax,<end> / jb`, ours `cmp eax,<end> / jl`). Spelling them
// `unsigned artifactId` does emit retail's two `jb`s, but it also swaps the
// `xor edi,edi` / `mov ebx,<aArtifactTraits+4>` pair that opens the combination
// loop, and measures 76.8733 -> 75.8673 net. Same either way if the two flag
// loops get their own `unsigned` and the combination loop keeps its `int`, so
// the swap comes from the flag loops themselves.
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
            stringBytes += strlen(traitsSheet->GetRow(row)[22])
                + strlen(traitsSheet->GetRow(row)[0]) + 2;
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

            const char* classCell = values[21];
            char artifactClass = classCell[0];
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
