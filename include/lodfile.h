#ifndef HOMM3_LODFILE_H
#define HOMM3_LODFILE_H

#include "va.h"

#include <stdio.h>
#include <vector>
#include <zlib.h>

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
// Loki h3maped object 42 (RoE source, file name inferred from the class):
// the editor reads and writes LOD archives, so it keeps the editing members
// (create, addItem, deleteItem, pack, squishme) the game's VC6 link drops.
// SGI's 12-byte vector puts the size at 0x188.
class LODFile {
private:
    FILE* m_fileptr;               // +0x000
    char m_lodFileName[256];       // +0x004
    int m_opened;                  // +0x104
    unsigned char* m_dataBuffer;   // +0x108
    unsigned long m_dataBufferSize;  // +0x10c
    int m_dataItemIndex;           // +0x110
    int m_dataPos;                 // +0x114
    int m_matchindex;              // +0x118
    LODHeader m_header;            // +0x11c

    void Find(unsigned begin, unsigned end, const char* itemName);
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
    // create and pack write a fresh index of this many empty entries.
    enum { LOD_MAX_ENTRIES = 10000 };

    int m_numEntries;              // +0x178
    vector<LODEntry> m_subindex;   // +0x17c

    void clear();
    long GetFileSize();
    int create(const char* filename);
    LODEntry* getItemIndex(const char* itemName);
    bool exist(const char* itemName);
    char* getErrorString(int lodError);
    int addItem(LODEntry& entry, void* data, int noReplace, bool compressItem);
    int deleteItem(const char* itemName);

    LODFile();
    ~LODFile();

    int open(const char* filename, int flags);
    void sort();
    int pack();
    int squishme();
    bool pointAt(const char* itemName);
    int read(void* dest, int numBytes);
};

#endif  /* HOMM3_LODFILE_H */
