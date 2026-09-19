#include <va.h>
#include <string.h>
#include "lodfile.h"

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

#if 0  // @carcass

// E:\gamedcs\lodfile.cpp:53
DC_ONLY(0xe90c0, 0x3E)
int LODFile::GetFileSize()
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\lodfile.cpp:72
// DC's getDataPtr is an ordinary helper called at pointAt line 431.
// The PC expansion seeks the archive stream before returning its handle.
DC_ONLY(0xe9100, 0x54)
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

#if 0  // @carcass

// E:\gamedcs\lodfile.cpp:112
DC_ONLY(0xe9198, 0x26)
unsigned char LODFile::exist(const char* item_name)
{
    // @stub
}

// E:\gamedcs\lodfile.cpp:125 - promoted to a live claim above.

// E:\gamedcs\lodfile.cpp:189
DC_ONLY(0xe92b4, 0x44)
char* LODFile::getErrorString(int LODErr)
{
    // @stub
}

// E:\gamedcs\lodfile.cpp:226
// Promoted to a live definition below; retail inlines it into open's resize.

// E:\gamedcs\lodfile.cpp:240 / 252 / 266 - promoted to live claims below.

// E:\gamedcs\lodfile.cpp:277
// Promoted to a live claim below.

// E:\gamedcs\lodfile.cpp:341
DC_ONLY(0xe955c, 0xF6)
void LODFile::set_filemap(unsigned char on)
{
    // @stub
}

// E:\gamedcs\lodfile.cpp:393
DC_ONLY(0xe9654, 0x12)
int compare(const void* arg1, const void* arg2)
{
    // @stub
}

// E:\gamedcs\lodfile.cpp:402
DC_ONLY(0xe9668, 0x28)
void LODFile::sort()
{
    // @stub
}

// E:\gamedcs\lodfile.cpp:430 - promoted to a live claim below.

// E:\gamedcs\lodfile.cpp:452 - promoted to a live claim below.

#endif  // @carcass

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
