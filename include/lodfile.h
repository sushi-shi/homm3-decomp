#ifndef HOMM3_LODFILE_H
#define HOMM3_LODFILE_H

#include <stdio.h>
#include <vector>
#include <zlib.h>
#include "va.h"

// The 32-byte archive-directory row. Retail Find's indexing uses a five-bit
// shift, while open reads these same five fields from the on-disk table.
struct LODEntry {
    char m_name[16];
    int m_offset;
    int m_size;
    int m_attrib;
    int m_csize;

    LODEntry();
};
SIZE(LODEntry, 0x20);

// Retail's inlined header constructor writes "LOD" at +0, version 500 at
// +4, and clears the remaining 84 bytes.
struct LODHeader {
    char m_lodId[4];
    int m_version;
    int m_numEntries;
    // Original Dreamcast LODHeader::reserved is char[80] at +12,
    // exactly matching the retail 0x5c-byte header and constructor clear.
    // This is documented reserved storage, not an unresolved field.
    char m_reserved[80];

    // No retail row of its own - LODFile's constructor 0x4fa780 carries
    // it inline, in this order: the "LOD" strcpy into this+0x11c, the
    // 500 at +4, the zero at +8, then the twenty-dword rep stosd over
    // reserved.  Defined in lodfile.cpp beside its one call site.
    LODHeader();
};
SIZE(LODHeader, 0x5c);

// Canonical retail layout. The constructor and clear/open/read bodies account
// for every field and DoNewGame's static storage proves the total 0x18c size.
class LODFile {
private:
    FILE* m_fileptr;
    char m_lodFileName[256];
    int m_opened;
    unsigned char* m_dataBuffer;
    unsigned long m_dataBufferSize;
    int m_dataItemIndex;
    int m_dataPos;
    int m_matchindex;
    LODHeader m_header;

    void find(unsigned begin, unsigned end, const char* itemName);
    void* getDataPtr(const char* itemName);

public:
    enum EError {
        LOD_NO_ERROR = 0,
        LOD_NOT_OPEN = 1,
        LOD_ALREADY_EXISTS = 2,
        LOD_CHAPTER_NOT_FOUND = 3,
        LOD_ITEM_NOT_FOUND = 4,
        LOD_NO_IO_BUFFER = 5
    };
    int m_numEntries;
    std::vector<LODEntry> m_subindex;
    unsigned char exist(const char* itemName);
    char* getErrorString(int lodError);
    void sort();
    void clear();
    unsigned char pointAt(const char* itemName);
    int read(void* dest, int numBytes);
    LODEntry* getItemIndex(const char* itemName);
    int open(const char* filename, int flags);

    LODFile();
    ~LODFile();
};
SIZE(LODFile, 0x18c);

#endif  /* HOMM3_LODFILE_H */
