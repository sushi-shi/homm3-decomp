#include <stdlib.h>
#include <string.h>

#include "lodfile.h"

#include "va.h"

VA(0x004fa590, 0x77)  // dc 0xe908c
void LODFile::clear()
{
    if (m_opened) {
        m_subindex.clear();
        fclose(m_fileptr);
        delete m_dataBuffer;
        m_opened = 0;
    }
}

// E:\gamedcs\lodfile.cpp:72
// DC's getDataPtr is an ordinary helper called at pointAt line 431.
// The PC expansion seeks the archive stream before returning its handle.

void* LODFile::getDataPtr(const char* itemName)
{
    if (!m_opened)
        return 0;
    find(0, m_numEntries, itemName);
    if (m_matchindex >= 0) {
        fseek(m_fileptr, m_subindex[m_matchindex].m_offset, SEEK_SET);
        m_dataItemIndex = m_matchindex;
        return m_fileptr;
    }
    return 0;
}

VA(0x004fa610, 0x45)  // dc 0xe9154
LODEntry* LODFile::getItemIndex(const char* itemName)
{
    if (m_opened) {
        find(0, m_numEntries, itemName);
        if (m_matchindex >= 0)
            return &m_subindex[m_matchindex];
    }
    return 0;
}

// Original: LODFile::exist; lodfile.cpp:112, dc 0xe9198
unsigned char LODFile::exist(const char* itemName)
{
    find(0, m_numEntries, itemName);
    return m_matchindex >= 0;
}

VA(0x004fa660, 0x113)  // dc 0xe91c0
void LODFile::find(unsigned begin, unsigned end, const char* itemName)
{
    for (;;) {
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
                end = begin + half;
                continue;
            } else {
                for (unsigned i = begin; i < end; i++) {
                    if (_strcmpi(itemName, m_subindex[i].m_name) == 0) {
                        m_matchindex = i;
                        return;
                    }
                }
                m_matchindex = -1;
                return;
            }
        } else {
            if (end - begin > 4) {
                begin += half;
                continue;
            } else {
                for (unsigned j = begin; j < end; j++) {
                    if (_strcmpi(itemName, m_subindex[j].m_name) == 0) {
                        m_matchindex = j;
                        return;
                    }
                }
                m_matchindex = -1;
                return;
            }
        }
    }
}

// Original: LODFile::getErrorString; lodfile.cpp:189, dc 0xe92b4
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
LODHeader::LODHeader()
{
    strcpy(m_lodId, DATA_COMPGEN(0x0067fa58, lodSignature, "LOD"));
    m_version = 500;
    m_numEntries = 0;
    memset(m_reserved, 0, sizeof(m_reserved));
}

VA(0x004fa780, 0x7B)  // dc 0xe9330
LODFile::LODFile()
{
    m_fileptr = 0;
    m_opened = 0;
    m_dataBuffer = 0;
}

VA(0x004fa800, 0x97)  // dc 0xe9374
LODFile::~LODFile()
{
    clear();
}

VA(0x004fa8a0, 0x1C4)  // dc 0xe941c
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

// Original: compare; lodfile.cpp:393, dc 0xe9654
int __cdecl compare(const void* arg1, const void* arg2)
{
    return _strcmpi(static_cast<const LODEntry*>(arg1)->m_name,
                    static_cast<const LODEntry*>(arg2)->m_name);
}

// Original: LODFile::sort; lodfile.cpp:402, dc 0xe9668
void LODFile::sort()
{
    qsort(&m_subindex[0], m_numEntries, sizeof(LODEntry), compare);
}

VA(0x004faa70, 0xAB)  // dc 0xe9690
unsigned char LODFile::pointAt(const char* itemName)
{
    if (!getDataPtr(itemName)) {
        m_dataItemIndex = -1;
        m_dataPos = -1;
        return 0;
    }
    m_dataItemIndex = m_matchindex;
    m_dataPos = 0;
    delete m_dataBuffer;
    m_dataBuffer = 0;
    m_dataBufferSize = 0;
    return 1;
}

VA(0x004fab20, 0x114)  // dc 0xe96e0
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
