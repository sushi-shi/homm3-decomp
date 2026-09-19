// lodfile.h - prototypes of lodfile.cpp (compiland lodfile.obj)
#ifndef HOMM3_LODFILE_H
#define HOMM3_LODFILE_H

#include <stdio.h>
#include <vector>
#include "va.h"

// zlib's uncompress, which LODFile::read calls on every packed entry
// (retail 0x606be0, decorated @uncompress@16 - the vendored zlib TUs
// compile /Gr, so it is fastcall like everything else here). The vendor
// include directory is deliberately kept off the compiler's INCLUDE
// path, so the single prototype lodfile.obj needs is spelled here
// rather than by pulling zlib.h into the game headers.
extern "C" int uncompress(unsigned char* dest, unsigned long* destLen,
                          const unsigned char* source,
                          unsigned long sourceLen);

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
    int m_numEntries;
    std::vector<LODEntry> m_subindex;
    void clear();
    unsigned char pointAt(const char* itemName);
    int read(void* dest, int numBytes);
    LODEntry* getItemIndex(const char* itemName);
    int open(const char* filename, int flags);

    LODFile();
    ~LODFile();
};
SIZE(LODFile, 0x18c);

// --- globals ---
// CODEVIEW(E:\gamedcs\lodfile.cpp:393, dc 0xe9654) int compare(const void* arg1, const void* arg2);

// --- LODEntry ---
// CODEVIEW(E:\gamedcs\lodfile.cpp:226, dc 0xe92f8) void LODEntry::LODEntry();

// --- LODFile ---
// CODEVIEW(E:\gamedcs\lodfile.cpp:53, dc 0xe90c0) int LODFile::GetFileSize();
// CODEVIEW(E:\gamedcs\lodfile.cpp:72, dc 0xe9100) void* LODFile::getDataPtr(const char* item_name);
// CODEVIEW(E:\gamedcs\lodfile.cpp:112, dc 0xe9198) unsigned char LODFile::exist(const char* item_name);
// CODEVIEW(E:\gamedcs\lodfile.cpp:189, dc 0xe92b4) char* LODFile::getErrorString(int LODErr);
// CODEVIEW(E:\gamedcs\lodfile.cpp:341, dc 0xe955c) void LODFile::set_filemap(unsigned char on);
// CODEVIEW(E:\gamedcs\lodfile.cpp:402, dc 0xe9668) void LODFile::sort();

// --- LODHeader ---
// CODEVIEW(E:\gamedcs\lodfile.cpp:266, dc 0xe93bc) void LODHeader::LODHeader();

#endif  /* HOMM3_LODFILE_H */
