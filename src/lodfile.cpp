#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "lodfile.h"

VA(0x004fa590, 0x77)
DC_ADDRESS(0x0e908c, 0x34)
MAC_ADDRESS(0x11b598, 0x5c)
void LODFile::clear()
{
    if (m_opened) {
        m_subindex.clear();
        fclose(m_fileptr);
        delete[] m_dataBuffer;
        m_opened = 0;
    }
}

// Project-inferred lookup shared by stream and directory-entry access.
// A closed archive leaves the previous match index untouched.
bool LODFile::findOpenEntry(const char* itemName)
{
    if (!m_opened)
        return false;
    find(0, m_numEntries, itemName);
    return m_matchindex >= 0;
}

// E:\gamedcs\lodfile.cpp:72
// DC's getDataPtr is an ordinary helper called at pointAt line 431.
// The PC expansion seeks the archive stream before returning its handle.
DC_ADDRESS(0x0e9100, 0x54)
MAC_ADDRESS(0x11b5f4, 0x8c)
void* LODFile::getDataPtr(const char* itemName)
{
    if (!findOpenEntry(itemName))
        return 0;
    fseek(m_fileptr, m_subindex[m_matchindex].m_offset, SEEK_SET);
    m_dataItemIndex = m_matchindex;
    return m_fileptr;
}

VA(0x004fa610, 0x45)
DC_ADDRESS(0x0e9154, 0x42)
MAC_ADDRESS(0x11b680, 0x70)
LODEntry* LODFile::getItemIndex(const char* itemName)
{
    if (findOpenEntry(itemName))
        return &m_subindex[m_matchindex];
    return 0;
}

// Original: LODFile::exist; lodfile.cpp:112
DC_ADDRESS(0x0e9198, 0x26)
unsigned char LODFile::exist(const char* itemName)
{
    find(0, m_numEntries, itemName);
    return m_matchindex >= 0;
}

// Project-inferred short-range scan shared by both binary-search branches.
void LODFile::findLinear(unsigned begin, unsigned end, const char* itemName)
{
    for (unsigned index = begin; index < end; ++index) {
        if (_strcmpi(itemName, m_subindex[index].m_name) == 0) {
            m_matchindex = index;
            return;
        }
    }
    m_matchindex = -1;
}

// Mac retains both recursive calls at 0x11b770 and 0x11b7d8. The Windows
// retail body at 0x4fa660 carries equivalent loops at the same two branches.
VA(0x004fa660, 0x113)
DC_ADDRESS(0x0e91c0, 0xf2)
MAC_ADDRESS(0x11b6f0, 0x148)
void LODFile::find(unsigned begin, unsigned end, const char* itemName)
{
    if (begin == end) {
        m_matchindex = -1;
        return;
    }

    unsigned half = (end - begin) / 2;
    int order = _strcmpi(itemName, m_subindex[begin + half].m_name);
    if (order == 0) {
        m_matchindex = begin + half;
        return;
    }
    if (order < 0) {
        if (end - begin > 4) {
            find(begin, begin + half, itemName);
            return;
        } else {
            findLinear(begin, end, itemName);
            return;
        }
    } else {
        if (end - begin > 4) {
            find(begin + half, end, itemName);
            return;
        } else {
            findLinear(begin, end, itemName);
            return;
        }
    }
}

// Original: LODFile::getErrorString; lodfile.cpp:189
DC_ADDRESS(0x0e92b4, 0x44)
char* LODFile::getErrorString(int lodError)
{
    switch (lodError) {
    case LOD_NO_ERROR: return "LOD File: No Error";
    case LOD_NOT_OPEN: return "LOD File: no LOD file has been opened";
    case LOD_ALREADY_EXISTS: return "LOD File: chapter or entry already exists";
    case LOD_CHAPTER_NOT_FOUND: return "LOD File: chapter not found";
    case LOD_ITEM_NOT_FOUND: return "LOD File: item not found";
    case LOD_NO_IO_BUFFER: return "LOD File: IO buffer not set";
    default: return "LOD File: Not a valid error code.";
    }
}

// E:\gamedcs\lodfile.cpp:226.  DC's seven source rows prove these five
// stores; retail repeats them exactly in open's vector-resize temporary.
DC_ADDRESS(0x0e92f8, 0x38)
MAC_ADDRESS(0x11b838, 0x1c)
LODEntry::LODEntry()
{
    m_name[0] = 0;
    m_offset = 0;
    m_size = 0;
    m_attrib = 0;
    m_csize = 0;
}

// E:\gamedcs\lodfile.cpp:266.  No retail row of its own: /Ob2 folds it
// into the one call site below, where the "LOD" strcpy expands to VC6's
// repne scasb + rep movs pair.
DC_ADDRESS(0x0e93bc, 0x60)
MAC_ADDRESS(0x11b9b0, 0x50)
LODHeader::LODHeader()
{
    strcpy(m_lodId, DATA_COMPGEN(0x0067fa58, lodSignature, "LOD"));
    m_version = 500;
    m_numEntries = 0;
    memset(m_reserved, 0, sizeof(m_reserved));
}

VA(0x004fa780, 0x7B)
DC_ADDRESS(0x0e9330, 0x42)
MAC_ADDRESS(0x11b854, 0x7c)
LODFile::LODFile()
{
    m_fileptr = 0;
    m_opened = 0;
    clear();
    m_dataBuffer = 0;
}

VA(0x004fa800, 0x97)
DC_ADDRESS(0x0e9374, 0x48)
MAC_ADDRESS(0x11b934, 0x7c)
LODFile::~LODFile()
{
    clear();
}

VA(0x004fa8a0, 0x1C4)
DC_ADDRESS(0x0e941c, 0x140)
MAC_ADDRESS(0x11ba00, 0x174)
int LODFile::open(const char* filename, int flags)
{
    if (m_opened)
        clear();

    if (flags & 1)
        m_fileptr = fopen(filename,
            DATA_COMPGEN(0x00677d6c, lodReadMode, "rb"));
    else
        m_fileptr = fopen(filename,
            DATA_COMPGEN(0x0067fa5c, lodUpdateMode, "rb+"));
    if (!m_fileptr)
        return 1;

    strcpy(m_lodFileName, filename);
    fread(&m_header, sizeof(m_header), 1, m_fileptr);
    m_numEntries = m_header.m_numEntries;
    m_subindex.resize(m_numEntries);
    fread(&m_subindex[0], sizeof(LODEntry), m_numEntries, m_fileptr);
    fseek(m_fileptr, 0, SEEK_SET);
    m_opened = 1;
    return 0;
}

// DC set_filemap (lodfile.cpp:341, 0xe955c) owns hFile/hFileMap/
// pFileMem at +0x188/+0x18c/+0x190 and opens a wide WinCE mapping.
// GetFileSize (line53, 0xe90c0) uses the same backend: it saves DCftell,
// clears tempoff at +0x194, reads DCftell again and restores tempoff.
// Complete's LODFile is 0x18c bytes, with its Dinkumware vector occupying
// +0x17c..+0x18b and no mapping/tempoff tail. Its open/read bodies at
// 0x4fa8a0/0x4fab20 use fopen/fseek/fread and a decompression buffer.
// The two backend operations are reviewed in config/source/dc_only.tsv.

// Original: compare; lodfile.cpp:393
DC_ADDRESS(0x0e9654, 0x12)
int __cdecl compare(const void* arg1, const void* arg2)
{
    return _strcmpi(static_cast<const LODEntry*>(arg1)->m_name,
                    static_cast<const LODEntry*>(arg2)->m_name);
}

// Original: LODFile::sort; lodfile.cpp:402
DC_ADDRESS(0x0e9668, 0x28)
void LODFile::sort()
{
    qsort(&m_subindex[0], m_numEntries, sizeof(LODEntry), compare);
}

VA(0x004faa70, 0xAB)
DC_ADDRESS(0x0e9690, 0x4e)
MAC_ADDRESS(0x11bb74, 0x80)
unsigned char LODFile::pointAt(const char* itemName)
{
    if (!getDataPtr(itemName)) {
        m_dataItemIndex = -1;
        m_dataPos = -1;
        return 0;
    }
    m_dataItemIndex = m_matchindex;
    m_dataPos = 0;
    delete[] m_dataBuffer;
    m_dataBuffer = 0;
    m_dataBufferSize = 0;
    return 1;
}

VA(0x004fab20, 0x114)
DC_ADDRESS(0x0e96e0, 0x188)
MAC_ADDRESS(0x11bbf4, 0x174)
int LODFile::read(void* dest, int numBytes)
{
    if (!m_opened)
        return -1;
    if (m_dataItemIndex == -1)
        return -1;

    LODEntry entry = m_subindex[m_dataItemIndex];
    if (entry.m_csize) {
        if (m_dataBuffer == 0) {
            m_dataBuffer = new unsigned char[entry.m_size];
            m_dataBufferSize = entry.m_size;
            m_dataPos = 0;
            unsigned char* packed = new unsigned char[entry.m_csize];
            fread(packed, 1, entry.m_csize, m_fileptr);
            unsigned long destLen = m_dataBufferSize;
            uncompress(m_dataBuffer, &destLen, packed, entry.m_csize);
            delete packed;
        }
        memcpy(dest, m_dataBuffer + m_dataPos, numBytes);
        m_dataPos += numBytes;
    } else {
        fread(dest, 1, numBytes, m_fileptr);
    }
    return 0;
}

VA_COMPGEN(0x004fac40, 0x26B, VECTOR_INSERT, LODEntry)
