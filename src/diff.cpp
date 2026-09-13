// diff.cpp - E:\gamedcs\diff.cpp (compiland diff.obj)
#include <string.h>
#include <va.h>
#include "terrain.h"
#include "diff.h"

template <class _TYPE>
inline const _TYPE& cppMax(_TYPE x, _TYPE y)
{
    return (x < y ? y : x);
}

inline int max(int a, int b)
{
    return cppMax(a, b);
}

#if 0 // @carcass: trivial retail-dropped/inlined bodies
DC_ONLY(0x822e0, 0x4)
void CDiffFile::CDiffFile()
{
}

#endif

// DC diff.cpp:57/58: GetData is defined in this TU before Apply.
// The payload follows the four-byte size word in the serialized allocation.
DC_ONLY(0x822e4, 0x6)
unsigned char* CDiffFile::getData()
{
    return static_cast<unsigned char*>(static_cast<void*>(this + 1));
}

VA(0x00490f60, 0xc5)  // dc 0x822ec
void* CDiffFile::apply(unsigned char* oldSaveGame, int oldSaveGameSize)
{
    unsigned char* newSaveGame = new unsigned char[m_numBytes];
    unsigned char* diffData = getData();
    unsigned int resultOffset = 0;
    int oldOffset = 0;
    unsigned int diffOffset = 0;

    while (resultOffset < m_numBytes) {
        CDiffHeader* header =
            static_cast<CDiffHeader*>(
                static_cast<void*>(diffData + diffOffset));
        if (header->m_copy) {
            diffOffset += sizeof(CDiffHeader);
            memcpy(newSaveGame + resultOffset,
                   diffData + diffOffset,
                   header->m_numBytes);
            diffOffset += header->m_numBytes;
            resultOffset += header->m_numBytes;
            oldOffset += header->m_oldNumBytes;
        } else {
            diffOffset += sizeof(CDiffHeader);
            memcpy(newSaveGame + resultOffset, oldSaveGame + oldOffset,
                   header->m_numBytes);
            resultOffset += header->m_numBytes;
            oldOffset += header->m_numBytes;
        }
    }

    return newSaveGame;
}

VA(0x00491030, 0x20)  // dc 0x82378
CDiffMaker::CDiffMaker(unsigned char* oldData, int oldSize,
                       unsigned char* newData, int newSize)
    : m_oldData(oldData), m_newData(newData),
      m_oldSize(oldSize), m_newSize(newSize)
{
}

// DC diff.cpp:115, defined here before FindNextSame.
DC_ONLY(0x8238c, 0x4c)
int CDiffMaker::countSameBytes(int oldOffset, int newOffset)
{
    int count = 0;
    while (m_oldData[oldOffset + count] ==
           m_newData[newOffset + count]) {
        if (oldOffset + count >= m_oldSize)
            break;
        if (newOffset + count >= m_newSize)
            break;
        ++count;
    }
    return count;
}

VA(0x00491050, 0xed)  // dc 0x823d8
bool CDiffMaker::findNextSame(int oldOffset, int newOffset,
                              int& oldCount, int& newCount)
{
    oldCount = 1;
    newCount = 1;
    ++oldOffset;
    ++newOffset;

    for (;;) {
        for (int newDelta = 0; newDelta < 64; ++newDelta) {
            for (int oldDelta = 0; oldDelta < 64; ++oldDelta) {
                if (oldOffset + oldDelta + 16 >= m_oldSize)
                    return 0;
                if (newOffset + newDelta + 16 >= m_newSize)
                    return 0;

                if (memcmp(m_newData + newOffset + newDelta,
                           m_oldData + oldOffset + oldDelta, 16) == 0) {
                    newCount += newDelta;
                    oldCount += oldDelta;
                    return 1;
                }
            }
        }

        oldOffset += 64;
        newOffset += 64;
        oldCount += 64;
        newCount += 64;
    }
}

// Residual (current 83.9244%, banked MAX 83.9477%): exact 447-byte extent,
// exact 14-block retail CFG, and the exact 0x3c retail frame.  Dreamcast proves
// the max/CountSameBytes/FindNextSame helpers, three scoped CDiffHeader objects,
// and both payload memcpy arms.  Its lower-bound local inventory and tail line
// rows favor no separately named `count`; spelling m_newSize-newOffset directly
// restores retail's frame and moves the diagnostic from 214 mixed flow/register
// slots to 111 pure register-visible slots, so that coherent source shape is
// retained despite the small aggregate-score dip.

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
CDiffFile* CDiffMaker::makeDiff(unsigned long& diffSize)
{
    diffSize = 0;
    int oldOffset = 0;
    CDiffFile* diff =
        static_cast<CDiffFile*>(static_cast<void*>(
            new unsigned char[max(m_oldSize, m_newSize) + 5000]));
    int diffOffset = sizeof(unsigned int);
    int newOffset = 0;

    for (;;) {
        int sameCount = countSameBytes(oldOffset, newOffset);

        if (!sameCount) {
            int oldCount = 0;
            int newCount = 0;
            if (findNextSame(oldOffset, newOffset, oldCount, newCount)) {
                CDiffHeader header(newCount, 1, oldCount);
                memcpy(diff->getBase() + diffOffset, &header,
                       sizeof(CDiffHeader));
                diffOffset += sizeof(CDiffHeader);
                memcpy(diff->getBase() + diffOffset,
                       m_newData + newOffset, newCount);
                diffOffset += newCount;
                oldOffset += oldCount;
                newOffset += newCount;
            } else {
                CDiffHeader header(m_newSize - newOffset, 1, 0);
                memcpy(diff->getBase() + diffOffset, &header,
                       sizeof(CDiffHeader));
                diffOffset += sizeof(CDiffHeader);
                memcpy(diff->getBase() + diffOffset,
                       m_newData + newOffset, m_newSize - newOffset);
                diffOffset += m_newSize - newOffset;
                diffSize = diffOffset;
                diff->m_numBytes = m_newSize;
                return diff;
            }
        } else {
            CDiffHeader header(sameCount, 0, 0);
            memcpy(diff->getBase() + diffOffset, &header,
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
