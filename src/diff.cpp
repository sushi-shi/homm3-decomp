#include "va.h"
#include "includes.h"

#include <string.h>

#include "diff.h"

#include "terrain.h"

// E:\gamedcs\diff.cpp:43, dc 0x825b8. CodeView type 0x54d4
// owns this record's single in-class constructor; only this TU uses it.
class CDiffHeader
{
public:
    int m_numBytes;
    int m_oldNumBytes;
    unsigned char m_copy;

    CDiffHeader(int numBytes, bool copy, int oldNumBytes)
        : m_numBytes(numBytes), m_oldNumBytes(oldNumBytes), m_copy(copy)
    {
    }
};

// E:\gamedcs\diff.cpp:52, dc 0x822e0
CDiffFile::CDiffFile()
{
}

// E:\gamedcs\diff.cpp:57, dc 0x822e4
MAC_ADDRESS(0x0a25c8, 0x8)
unsigned char* CDiffFile::getData()
{
    // Payload follows the serialized size word in the same allocation.
    return static_cast<unsigned char*>(static_cast<void*>(this + 1));
}

VA(0x00490f60, 0xc5) MAC_ADDRESS(0x0a25d0, 0xcc)  // dc 0x822ec
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

VA(0x00491030, 0x20) MAC_ADDRESS(0x0a269c, 0x14)  // dc 0x82378
CDiffMaker::CDiffMaker(unsigned char* oldData, int oldSize,
                       unsigned char* newData, int newSize)
    : m_oldData(oldData), m_newData(newData),
      m_oldSize(oldSize), m_newSize(newSize)
{
}

// E:\gamedcs\diff.cpp:115, dc 0x8238c. Ordinary helper defined
// before MakeDiff; Complete's /Ob2 chooses its caller expansion.
MAC_ADDRESS(0x0a26b0, 0x5c)
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

VA(0x00491050, 0xed) MAC_ADDRESS(0x0a270c, 0x108)  // dc 0x823d8
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

// E:\gamedcs\diff.cpp:174
// DC lines 175 and 185 keep diffOffset as a zero-initialized local and then
// advance it past the serialized size word. Collapsing those statements into
// `int diffOffset = sizeof(unsigned int)` swaps retail's ESI/EDI roles and
// gives 83.9593%; the distinct lifetime below reproduces all 447 retail bytes.
// DC lines 204/211, 222/226 and 237 address the whole allocation plus
// diffOffset. Lines 228, 248 and 252 prove the terminal break and the final
// diffSize/m_numBytes stores outside the loop's lexical scopes. The changed
// arm advances newOffset before oldOffset (DC 214/215); the same-data arm uses
// the opposite order (DC 242/243).
VA(0x00491140, 0x1bf) MAC_ADDRESS(0x0a2814, 0x1c0)  // linkorder + calls FindNextSame and emits 12-byte records, dc 0x82488
CDiffFile* CDiffMaker::makeDiff(unsigned long& diffSize)
{
    diffSize = 0;
    int oldOffset = 0;
    int diffOffset = 0;
    CDiffFile* diff =
        static_cast<CDiffFile*>(static_cast<void*>(
            new unsigned char[max(m_oldSize, m_newSize) + 5000]));
    int newOffset = 0;
    diffOffset += sizeof(unsigned int);

    for (;;) {
        int sameCount = countSameBytes(oldOffset, newOffset);

        if (!sameCount) {
            int oldCount = 0;
            int newCount = 0;
            if (findNextSame(oldOffset, newOffset, oldCount, newCount)) {
                CDiffHeader diffHeader(newCount, 1, oldCount);
                memcpy(static_cast<unsigned char*>(static_cast<void*>(diff)) + diffOffset,
                       &diffHeader, sizeof(CDiffHeader));
                diffOffset += sizeof(CDiffHeader);
                memcpy(static_cast<unsigned char*>(static_cast<void*>(diff)) + diffOffset,
                       m_newData + newOffset, newCount);
                diffOffset += newCount;
                newOffset += newCount;
                oldOffset += oldCount;
            } else {
                CDiffHeader diffHeader(m_newSize - newOffset, 1, 0);
                memcpy(static_cast<unsigned char*>(static_cast<void*>(diff)) + diffOffset,
                       &diffHeader, sizeof(CDiffHeader));
                diffOffset += sizeof(CDiffHeader);
                memcpy(static_cast<unsigned char*>(static_cast<void*>(diff)) + diffOffset,
                       m_newData + newOffset, m_newSize - newOffset);
                diffOffset += m_newSize - newOffset;
                break;
            }
        } else {
            CDiffHeader diffHeader(sameCount, 0, 0);
            memcpy(static_cast<unsigned char*>(static_cast<void*>(diff)) + diffOffset,
                   &diffHeader, sizeof(CDiffHeader));
            diffOffset += sizeof(CDiffHeader);
            oldOffset += sameCount;
            newOffset += sameCount;
        }
    }

    diffSize = diffOffset;
    diff->m_numBytes = m_newSize;
    return diff;
}

// CDiffHeader's canonical source-local body is above at DC line 43.
