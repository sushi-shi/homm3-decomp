// diff.h - retail-proven layouts for diff.obj
#ifndef HOMM3_DIFF_H
#define HOMM3_DIFF_H

class CDiffHeader
{
public:
    int m_numBytes;
    int m_oldNumBytes;
    unsigned char m_copy;
    // Before normalization: _pad.
    // Dreamcast has two dwords and a byte; retail serializes a
    // 12-byte header. These three bytes align the header extent.
    unsigned char m_tailPadding[3];

    CDiffHeader(int numBytes, unsigned char copy, int oldNumBytes)
        : m_numBytes(numBytes), m_oldNumBytes(oldNumBytes), m_copy(copy)
    {
    }

    // Before normalization (function): CDiffHeader::GetData.
    unsigned char* getData()
    {
        // The payload follows the serialized 12-byte header, not a field
        // whose padding length happens to reach the same address.
        return static_cast<unsigned char*>(static_cast<void*>(this + 1));
    }
};

class CDiffFile
{
private:
    CDiffFile()
    {
    }

public:
    unsigned int m_numBytes;

    // Before normalization (function): CDiffFile::GetData.
    unsigned char* getData()
    {
        // DC diff.cpp:58 returns this + 4; MakeDiff allocates a byte
        // buffer with a size word followed by variable-length records.
        // There is no one-byte payload array to walk beyond.
        return getBase() + sizeof(m_numBytes);
    }

    // Before normalization (function): CDiffFile::GetBase.
    unsigned char* getBase()
    {
        // MakeDiff writes offsets from the allocation/header base (retail
        // 0x491140; DC diff.cpp:204/211), which is already this.
        return static_cast<unsigned char*>(static_cast<void*>(this));
    }

    // Before normalization (function): CDiffFile::Apply.
    void* apply(unsigned char* oldSaveGame, int oldSaveGameSize);
};

class CDiffMaker
{
public:
    unsigned char* m_oldData;
    unsigned char* m_newData;
    int m_oldSize;
    int m_newSize;

    CDiffMaker(unsigned char* oldData, int oldSize,
               unsigned char* newData, int newSize);

protected:
    // Before normalization (function): CDiffMaker::CountSameBytes.
    int countSameBytes(int oldOffset, int newOffset)
    {
        int count = 0;
        while (m_oldData[oldOffset + count] ==
                   m_newData[newOffset + count] &&
               oldOffset + count < m_oldSize &&
               newOffset + count < m_newSize) {
            ++count;
        }
        return count;
    }
    // Before normalization (function): CDiffMaker::FindNextSame.
    bool findNextSame(int oldOffset, int newOffset,
                      int& oldCount, int& newCount);

public:
    // Before normalization (function): CDiffMaker::MakeDiff.
    CDiffFile* makeDiff(unsigned long& diffSize);
};

#endif  /* HOMM3_DIFF_H */
