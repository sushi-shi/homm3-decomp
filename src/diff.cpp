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

// Residual (68.9643%): exact 197-byte extent and three-branch/two-return CFG,
// but retail colors output/diff offsets in EBX/EAX/EDI while this CL colors the
// same lifetimes in EDI/stack/EAX. Tried and rejected: caching GetData(), a
// common for-loop increment, declaration permutations, and register hints.
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
            memcpy(result + resultOffset,
                   header->GetData(),
                   header->m_numBytes);
            diffOffset += sizeof(CDiffHeader) + header->m_numBytes;
            resultOffset += header->m_numBytes;
            oldOffset += header->m_oldNumBytes;
        } else {
            memcpy(result + resultOffset, oldSaveGame + oldOffset,
                   header->m_numBytes);
            diffOffset += sizeof(CDiffHeader);
            resultOffset += header->m_numBytes;
            oldOffset += header->m_numBytes;
        }
    }

    return result;
}

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

// Residual (83.2813%): the first 71 normalized instructions agree; retail
// places the success epilogue after the failure epilogue and chooses different
// result registers. Tried and rejected: nested-scope counters, direct returns,
// explicit success/failure labels, pointer-parameter spelling, and memcmp's
// opposite (semantically symmetric) operand order. The formal 2026-08-13
// diagnostics close the remaining source-level search: why-branch reports
// distance 0 (identical 11-block / 5-branch / 2-return shape), while why-reg's
// model finds no binding divergence in its source-addressable slice and its
// seven fallback mutations are all byte-neutral or worse.
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
    oldCount += oldDelta;
    newCount += newDelta;
    return 1;
}

// Residual (81.5174%): exact 447-byte extent and seven-branch/one-return CFG;
// the remaining delta is a cyclic ESI/EDI/EBX coloring of this, newOffset, and
// diffOffset. Tried and rejected: a ternary maximum, mutable-offset
// CountSameBytes, placement-new headers, a shared terminal tail, declaration
// permutations, register hints, and inert type-count probes. The retained
// indexed helper, reference-selecting max, scoped headers, reference
// signatures, and direct terminal return are all independently DC/retail
// evidenced and are the measured best shape.
VA(0x00491140, 0x1bf)  // linkorder + calls FindNextSame and emits 12-byte records, dc 0x82488
CDiffFile* CDiffMaker::MakeDiff(unsigned long& diffSize)
{
    diffSize = 0;
    CDiffFile* diff =
        static_cast<CDiffFile*>(static_cast<void*>(
            new unsigned char[max(m_oldSize, m_newSize) + 5000]));
    int oldOffset = 0;
    int newOffset = 0;
    int diffOffset = sizeof(unsigned int);

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
                int count = m_newSize - newOffset;
                CDiffHeader header(count, 1, 0);
                memcpy(diff->GetBase() + diffOffset, &header,
                       sizeof(CDiffHeader));
                diffOffset += sizeof(CDiffHeader);
                memcpy(diff->GetBase() + diffOffset,
                       m_newData + newOffset, count);
                diffOffset += count;
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
