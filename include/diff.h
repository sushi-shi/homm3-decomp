// diff.h - retail-proven layouts for diff.obj
#ifndef HOMM3_DIFF_H
#define HOMM3_DIFF_H

class CDiffFile
{
private:
    CDiffFile();

public:
    unsigned int m_numBytes;
    unsigned char m_data[1];

    unsigned char* getData();

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
    int countSameBytes(int oldOffset, int newOffset);
    bool findNextSame(int oldOffset, int newOffset,
                      int& oldCount, int& newCount);

public:
    // Before normalization (function): CDiffMaker::MakeDiff.
    CDiffFile* makeDiff(unsigned long& diffSize);
};

#endif  /* HOMM3_DIFF_H */
