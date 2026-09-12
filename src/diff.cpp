// diff.cpp - E:\gamedcs\diff.cpp (compiland diff.obj)
#include <string.h>
#include <va.h>
#include "terrain.h"
#include "diff.h"
#include "includes.h"


// E:\gamedcs\diff.cpp:43, dc 0x825b8. CodeView type 0x54d4
// owns this record's single in-class constructor; only this TU uses it.
class CDiffHeader
{
public:
    int m_numBytes;
    int m_oldNumBytes;
    unsigned char m_copy;
    unsigned char m_tailPadding[3];

    CDiffHeader(int numBytes, unsigned char copy, int oldNumBytes)
        : m_numBytes(numBytes), m_oldNumBytes(oldNumBytes), m_copy(copy)
    {
    }
};

// E:\gamedcs\diff.cpp:52, dc 0x822e0
CDiffFile::CDiffFile()
{
}

// E:\gamedcs\diff.cpp:57, dc 0x822e4
unsigned char* CDiffFile::getData()
{
    // Payload follows the serialized size word in the same allocation.
    return static_cast<unsigned char*>(static_cast<void*>(this + 1));
}

// Exact with the payload pointer captured once immediately after allocation,
// as DC line63 emits GetData before the three offset initializers at65..67.
// The raw named result local is newSaveGame. Repeated GetData calls inside
// the loop score99.6429: three scale-one addresses choose the opposite SIB
// base/index order. The cached source lifetime fixes all three, without a
// qualifier or optimizer pin. Restoring the ordinary GetData body to its
// original source order is independently byte-neutral; both facts are kept.
// Four source-family controls reproduced: uncached99.6429, cached100 with
// either helper placement. Older cached-pointer/why-reg controls were flat
// in their then-current source state; they do not bound this reconstruction.
// E:\gamedcs\diff.cpp:62
VA(0x00490f60, 0xc5)  // linkorder + body: allocated output size and 12-byte copy/reference records, dc 0x822ec
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

// E:\gamedcs\diff.cpp:107
VA(0x00491030, 0x20)  // linkorder + 16-byte retail field layout, dc 0x82378
CDiffMaker::CDiffMaker(unsigned char* oldData, int oldSize,
                       unsigned char* newData, int newSize)
    : m_oldData(oldData), m_newData(newData),
      m_oldSize(oldSize), m_newSize(newSize)
{
}

// E:\gamedcs\diff.cpp:115, dc 0x8238c. Ordinary helper defined
// before MakeDiff; Complete's /Ob2 chooses its caller expansion.
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

// Dreamcast diff.cpp:142/144 has nested for-loop scopes; 146..150 has two
// separate early-failure checks before memcmp. Restoring both source facts
// closes 84.1667% to 100%: VC6 now emits the failure epilogue before success,
// exactly as retail does. Changing either alone leaves the old layout.
// The 24-state family produced 12 distinct objects and ten reproduced elites.
// CountSameBytes' ordinary TU placement and bounds-break body are independently
// byte-neutral in MakeDiff, so the canonical definition is retained above.
// Dreamcast's decorated _N return proves bool despite the dossier rendering
// that type as unsigned char. The byte-return alternatives are not adopted.
// E:\gamedcs\diff.cpp:133
VA(0x00491050, 0xed)  // linkorder + 64x64 search for a 16-byte synchronization run, dc 0x823d8
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
// DC diff.cpp:204/211, 222/226 and 237 use the whole output buffer
// plus diffOffset as their memcpy destinations (first pair 0x82500/0x82510).
// GetBase was an unattested header wrapper subtracting the size member
// from GetData's array address. The modeled payload array starts after the
// size prefix, so subtract that prefix from the whole-buffer offsets here.
// Apply keeps its separate payload-relative GetData accesses.
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
                memcpy(static_cast<unsigned char*>(static_cast<void*>(diff)) + diffOffset,
                       &header, sizeof(CDiffHeader));
                diffOffset += sizeof(CDiffHeader);
                memcpy(static_cast<unsigned char*>(static_cast<void*>(diff)) + diffOffset,
                       m_newData + newOffset, newCount);
                diffOffset += newCount;
                oldOffset += oldCount;
                newOffset += newCount;
            } else {
                CDiffHeader header(m_newSize - newOffset, 1, 0);
                memcpy(static_cast<unsigned char*>(static_cast<void*>(diff)) + diffOffset,
                       &header, sizeof(CDiffHeader));
                diffOffset += sizeof(CDiffHeader);
                memcpy(static_cast<unsigned char*>(static_cast<void*>(diff)) + diffOffset,
                       m_newData + newOffset, m_newSize - newOffset);
                diffOffset += m_newSize - newOffset;
                diffSize = diffOffset;
                diff->m_numBytes = m_newSize;
                return diff;
            }
        } else {
            CDiffHeader header(sameCount, 0, 0);
            memcpy(static_cast<unsigned char*>(static_cast<void*>(diff)) + diffOffset,
                   &header, sizeof(CDiffHeader));
            diffOffset += sizeof(CDiffHeader);
            oldOffset += sameCount;
            newOffset += sameCount;
        }
    }

}

// CDiffHeader's canonical source-local body is above at DC line 43.
