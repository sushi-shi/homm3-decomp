// diff.cpp - E:\gamedcs\diff.cpp (compiland diff.obj)
#include <string.h>
#include <va.h>
#include "terrain.h"
#include "diff.h"

template <class _TYPE>
inline const _TYPE& _cpp_max(_TYPE _X, _TYPE _Y)
{
    return (_X < _Y ? _Y : _X);
}

inline int max(int a, int b)
{
    return _cpp_max(a, b);
}

#if 0 // @carcass: trivial retail-dropped/inlined bodies
DC_ONLY(0x822e0, 0x4)
void CDiffFile::CDiffFile()
{
}

DC_ONLY(0x822e4, 0x6)
unsigned char* CDiffFile::GetData()
{
    return 0;
}
#endif

// Residual (99.6429%): B18 commutative scale-1 SIB base/index swap on the three
// `this + diffOffset` addresses - retail encodes base=EAX(diffOffset),
// index=ESI(this) (SIB 0x30), our CL the reverse (SIB 0x06). Everything else is
// byte-exact. Tried and rejected (all byte-identical): `diffOffset + GetData()`,
// `&GetData()[diffOffset]`. Same class as hero.cpp:2162.
// The Dreamcast dossier corroborates the seven-block loop, its two memcpy
// arms, and the single newSaveGame pointer local.  A fresh why-reg catalog
// sweep leaves six masked slots (the three reciprocal EAX/ESI SIB pairs):
// zero-hoisting and oldOffset/diffOffset declaration swaps are flat, while
// every other naming/order/volatile probe is worse.  This remains a measured
// B1 encoding wall, not a missing source statement.
// 2026-08-14 two-axis /Ob2 re-test (the campaign rule that a one-axis "flat"
// verdict is not a verdict): HELD. Pad statements ahead of `resultOffset` x
// xx_nop sites before the return are 99.6429 in all twelve cells of
// M in {0,2,4,8} x k in {0,1,2}. Four further spellings measured byte-identical
// as well: `int diffOffset`, a hoisted `GetData()` pointer local, a named
// `diffCursor` for the header cast, and an extra unused local. The SIB
// base/index choice is not source-reachable here.
// The 68.96% plateau was structural, not register coloring: retail advances
// diffOffset PAST the header before the payload memcpy and re-derives the source
// as GetData() + diffOffset, which is what keeps diffOffset in a register and
// homes resultOffset instead.
// E:\gamedcs\diff.cpp:62
VA(0x00490f60, 0xc5)  // linkorder + body: allocated output size and 12-byte copy/reference records, dc 0x822ec
void* CDiffFile::Apply(unsigned char* oldSaveGame, int oldSaveGameSize)
{
    unsigned char* result = new unsigned char[m_numBytes];
    unsigned int resultOffset = 0;
    int oldOffset = 0;
    unsigned int diffOffset = 0;

    while (resultOffset < m_numBytes) {
        CDiffHeader* header =
            static_cast<CDiffHeader*>(
                static_cast<void*>(GetData() + diffOffset));
        if (header->m_copy) {
            diffOffset += sizeof(CDiffHeader);
            memcpy(result + resultOffset,
                   GetData() + diffOffset,
                   header->m_numBytes);
            diffOffset += header->m_numBytes;
            resultOffset += header->m_numBytes;
            oldOffset += header->m_oldNumBytes;
        } else {
            diffOffset += sizeof(CDiffHeader);
            memcpy(result + resultOffset, oldSaveGame + oldOffset,
                   header->m_numBytes);
            resultOffset += header->m_numBytes;
            oldOffset += header->m_numBytes;
        }
    }

    return result;
}

// E:\gamedcs\diff.cpp:107
VA(0x00491030, 0x20)  // linkorder + 16-byte retail field layout, dc 0x82378
CDiffMaker::CDiffMaker(unsigned char* oldData, int oldSize,
                       unsigned char* newData, int newSize)
    : m_oldData(oldData), m_newData(newData),
      m_oldSize(oldSize), m_newSize(newSize)
{
}

#if 0 // @carcass: retail inlined into MakeDiff
DC_ONLY(0x8238c, 0x4c)
int CDiffMaker::CountSameBytes(int oldOffset, int newOffset)
{
    return 0;
}
#endif

// Residual (84.1667%): everything up to the two epilogues is now byte-identical
// (retail's success block updates newCount BEFORE oldCount - the reverse of the
// obvious source order - which this body now does). The sole remaining delta is
// EPILOGUE EMISSION ORDER: retail lays the failure return (`xor al,al`) first
// and the success return second; our CL always emits success first. Proven not
// source-addressable - five exit shapes (goto/goto, direct `return 0` guard,
// inlined success return, both inlined, and swapped label order) ALL compile to
// the identical 84.1667 layout. Merged-return / block-layout generation family.
// Earlier rejects: nested-scope counters, pointer-parameter spelling, memcmp's
// symmetric operand order; why-branch distance 0.  The Dreamcast dossier
// corroborates the nested 64x64 search, 16-byte memcmp, loop-local `i` scopes,
// and the two early-failure exits.  A fresh why-reg sweep leaves 14 schedule slots:
// zero-hoisting, delta declaration swaps, and one chained assignment are flat;
// the reverse chain/store order and volatile deltas are worse.
// E:\gamedcs\diff.cpp:133
VA(0x00491050, 0xed)  // linkorder + 64x64 search for a 16-byte synchronization run, dc 0x823d8
bool CDiffMaker::FindNextSame(int oldOffset, int newOffset,
                              int& oldCount, int& newCount)
{
    oldCount = 1;
    newCount = 1;
    ++oldOffset;
    ++newOffset;

    int oldDelta;
    int newDelta;
    for (;;) {
        newDelta = 0;
        while (newDelta < 64) {
            oldDelta = 0;
            while (oldDelta < 64) {
                if (oldOffset + oldDelta + 16 >= m_oldSize ||
                    newOffset + newDelta + 16 >= m_newSize)
                    goto notFound;

                if (memcmp(m_newData + newOffset + newDelta,
                           m_oldData + oldOffset + oldDelta, 16) == 0) {
                    goto found;
                }
                ++oldDelta;
            }
            ++newDelta;
        }

        oldOffset += 64;
        newOffset += 64;
        oldCount += 64;
        newCount += 64;
    }

notFound:
    return 0;

found:
    newCount += newDelta;
    oldCount += oldDelta;
    return 1;
}

// Residual (current 83.9244%, banked MAX 83.9477%): exact 447-byte extent,
// exact 14-block retail CFG, and the exact 0x3c retail frame.  Dreamcast proves
// the max/CountSameBytes/FindNextSame helpers, three scoped CDiffHeader objects,
// and both payload memcpy arms.  Its lower-bound local inventory and tail line
// rows favor no separately named `count`; spelling m_newSize-newOffset directly
// restores retail's frame and moves the diagnostic from 214 mixed flow/register
// slots to 111 pure register-visible slots, so that coherent source shape is
// retained despite the small aggregate-score dip.
//
// The remaining delta is a callee-saved role swap: retail binds ESI=diffOffset,
// EDI=this, EBX=newOffset; our CL binds EDI=diffOffset, ESI=this, EBX=newOffset.
// The allocator-model pass confirms equal pseudo definition slots/order but a
// different front-end processing order: `this` is always the first-created
// call-crossing pseudo in this compile.  Its three model-filtered edits
// (diffOffset/newOffset declaration swap, oldCount/newCount swap, and terminal
// store swap) are flat or worse.  Earlier exhaustive controls also reject all
// 120 prologue orderings, ternary max, mutable-offset CountSameBytes,
// placement-new headers, a shared terminal tail, function-scope sameCount,
// register hints, and inert type-count probes.  This is a measured C1
// front-end-handle wall, not license to invent an alias local absent from DC.
// E:\gamedcs\diff.cpp:174
VA(0x00491140, 0x1bf)  // linkorder + calls FindNextSame and emits 12-byte records, dc 0x82488
CDiffFile* CDiffMaker::MakeDiff(unsigned long& diffSize)
{
    diffSize = 0;
    int oldOffset = 0;
    CDiffFile* diff =
        static_cast<CDiffFile*>(static_cast<void*>(
            new unsigned char[max(m_oldSize, m_newSize) + 5000]));
    int diffOffset = sizeof(unsigned int);
    int newOffset = 0;

    for (;;) {
        int sameCount = CountSameBytes(oldOffset, newOffset);

        if (!sameCount) {
            int oldCount = 0;
            int newCount = 0;
            if (FindNextSame(oldOffset, newOffset, oldCount, newCount)) {
                CDiffHeader header(newCount, 1, oldCount);
                memcpy(diff->GetBase() + diffOffset, &header,
                       sizeof(CDiffHeader));
                diffOffset += sizeof(CDiffHeader);
                memcpy(diff->GetBase() + diffOffset,
                       m_newData + newOffset, newCount);
                diffOffset += newCount;
                oldOffset += oldCount;
                newOffset += newCount;
            } else {
                CDiffHeader header(m_newSize - newOffset, 1, 0);
                memcpy(diff->GetBase() + diffOffset, &header,
                       sizeof(CDiffHeader));
                diffOffset += sizeof(CDiffHeader);
                memcpy(diff->GetBase() + diffOffset,
                       m_newData + newOffset, m_newSize - newOffset);
                diffOffset += m_newSize - newOffset;
                diffSize = diffOffset;
                diff->m_numBytes = m_newSize;
                return diff;
            }
        } else {
            CDiffHeader header(sameCount, 0, 0);
            memcpy(diff->GetBase() + diffOffset, &header,
                   sizeof(CDiffHeader));
            diffOffset += sizeof(CDiffHeader);
            oldOffset += sameCount;
            newOffset += sameCount;
        }
    }

}

#if 0 // @carcass: retail inlined into MakeDiff
DC_ONLY(0x825b8, 0xe)
void CDiffHeader::CDiffHeader(int numBytes, unsigned char copy,
                             int oldNumBytes)
{
}
#endif
