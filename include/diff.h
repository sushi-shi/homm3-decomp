// diff.h - retail-proven layouts for diff.obj
#ifndef HOMM3_DIFF_H
#define HOMM3_DIFF_H

class CDiffHeader
{
public:
    int m_numBytes;
    int m_oldNumBytes;
    unsigned char m_copy;
    unsigned char _pad[3];

    CDiffHeader(int numBytes, unsigned char copy, int oldNumBytes)
        : m_numBytes(numBytes), m_oldNumBytes(oldNumBytes), m_copy(copy)
    {
    }

    unsigned char* GetData()
    {
        return _pad + 3;
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
    unsigned char m_data[1];

    unsigned char* GetData()
    {
        return m_data;
    }

    unsigned char* GetBase()
    {
        return m_data - sizeof(m_numBytes);
    }

    void* Apply(unsigned char* oldSaveGame, int oldSaveGameSize);
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
    int CountSameBytes(int oldOffset, int newOffset)
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
    bool FindNextSame(int oldOffset, int newOffset,
                      int& oldCount, int& newCount);

public:
    CDiffFile* MakeDiff(unsigned long& diffSize);
};

#endif  /* HOMM3_DIFF_H */
