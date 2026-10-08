// lodfile.cpp - Loki h3maped object 42: LOD archive access and editing.
// The port compares names with strcasecmp where Windows used _strcmpi.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "lodfile.h"

void LODFile::clear()
{
    if (!m_opened)
        return;
    m_subindex.clear();
    fclose(m_fileptr);
    m_opened = 0;
}

long LODFile::GetFileSize()
{
    if (!m_opened)
        return 0;
    int pos = ftell(m_fileptr);
    fseek(m_fileptr, 0, SEEK_END);
    int size = ftell(m_fileptr);
    fseek(m_fileptr, pos, SEEK_SET);
    return size;
}

int LODFile::create(const char* filename)
{
    if (m_opened)
        return LOD_NOT_OPEN;
    LODHeader header;
    strcpy(m_lodFileName, filename);
    m_fileptr = fopen(m_lodFileName, "wb+");
    if (!m_fileptr)
        return LOD_CHAPTER_NOT_FOUND;
    LODEntry* entries = new LODEntry[LOD_MAX_ENTRIES];
    memset(entries, 0, LOD_MAX_ENTRIES * sizeof(LODEntry));
    fwrite(&header, sizeof(LODHeader), 1, m_fileptr);
    fwrite(entries, sizeof(LODEntry), LOD_MAX_ENTRIES, m_fileptr);
    fclose(m_fileptr);
    m_fileptr = NULL;
    delete[] entries;
    return LOD_NO_ERROR;
}

void* LODFile::getDataPtr(const char* itemName)
{
    if (!m_opened)
        return NULL;
    Find(0, m_numEntries, itemName);
    if (m_matchindex >= 0) {
        fseek(m_fileptr, m_subindex[m_matchindex].m_offset, SEEK_SET);
        m_dataItemIndex = m_matchindex;
        return m_fileptr;
    }
    return NULL;
}

LODEntry* LODFile::getItemIndex(const char* itemName)
{
    if (!m_opened)
        return NULL;
    Find(0, m_numEntries, itemName);
    if (m_matchindex >= 0)
        return &m_subindex[m_matchindex];
    return NULL;
}

bool LODFile::exist(const char* itemName)
{
    Find(0, m_numEntries, itemName);
    if (m_matchindex >= 0)
        return true;
    return false;
}

void LODFile::Find(unsigned begin, unsigned end, const char* itemName)
{
    if (begin == end) {
        m_matchindex = -1;
        return;
    }

    unsigned half = (end - begin) / 2;
    unsigned i;
    int order = strcasecmp(itemName, m_subindex[begin + half].m_name);
    if (order == 0) {
        m_matchindex = begin + half;
        return;
    }
    if (order < 0) {
        if (end - begin > 4)
            Find(begin, begin + half, itemName);
        else {
            for (i = begin; i < end; i++) {
                if (strcasecmp(itemName, m_subindex[i].m_name) == 0) {
                    m_matchindex = i;
                    return;
                }
            }
            m_matchindex = -1;
        }
    } else {
        if (end - begin > 4)
            Find(begin + half, end, itemName);
        else {
            for (i = begin; i < end; i++) {
                if (strcasecmp(itemName, m_subindex[i].m_name) == 0) {
                    m_matchindex = i;
                    return;
                }
            }
            m_matchindex = -1;
        }
    }
}

char* LODFile::getErrorString(int lodError)
{
    switch (lodError) {
    case LOD_NO_ERROR:
        return "LOD File: No Error";
        break;
    case LOD_NOT_OPEN:
        return "LOD File: no LOD file has been opened";
        break;
    case LOD_ALREADY_EXISTS:
        return "LOD File: chapter or entry already exists";
        break;
    case LOD_CHAPTER_NOT_FOUND:
        return "LOD File: chapter not found";
        break;
    case LOD_ITEM_NOT_FOUND:
        return "LOD File: item not found";
        break;
    case LOD_NO_IO_BUFFER:
        return "LOD File: IO buffer not set";
        break;
    default:
        return "LOD File: Not a valid error code.";
    }
}

int LODFile::addItem(LODEntry& entry, void* data, int noReplace, bool compressItem)
{
    if (!m_opened)
        return LOD_NOT_OPEN;

    char* buffer;
    uLongf length = entry.m_size;
    if (compressItem) {
        length += 12 + length / 16;
        buffer = new char[length];
        compress((Bytef*)buffer, &length, (Bytef*)data, entry.m_size);
        entry.m_csize = length;
        if (length > entry.m_size) {
            length = entry.m_size;
            memcpy(buffer, data, length);
            entry.m_csize = 0;
        }
    } else {
        length = entry.m_size;
        buffer = new char[length];
        memcpy(buffer, data, length);
        entry.m_csize = 0;
    }

    unsigned long offset = 0;
    int inPlace = 0;
    if (exist(entry.m_name)) {
        if (!noReplace) {
            Find(0, m_numEntries, entry.m_name);
            if (m_matchindex >= 0) {
                if (m_subindex[m_matchindex].m_csize > 0) {
                    if (length <= m_subindex[m_matchindex].m_csize)
                        inPlace = 1;
                } else {
                    if (length <= m_subindex[m_matchindex].m_size)
                        inPlace = 1;
                }
            }
        } else
            return LOD_ITEM_NOT_FOUND;

        if (inPlace) {
            fseek(m_fileptr, m_subindex[m_matchindex].m_offset, SEEK_SET);
        } else {
            fseek(m_fileptr, 0, SEEK_END);
            m_subindex[m_matchindex].m_offset = ftell(m_fileptr);
        }
        offset = ftell(m_fileptr);
        fwrite(buffer, length, 1, m_fileptr);
        m_subindex[m_matchindex].m_size = entry.m_size;
        m_subindex[m_matchindex].m_csize = entry.m_csize;
        m_subindex[m_matchindex].m_attrib = entry.m_attrib;
    } else {
        fseek(m_fileptr, 0, SEEK_END);
        entry.m_offset = ftell(m_fileptr);
        offset = ftell(m_fileptr);
        fwrite(buffer, length, 1, m_fileptr);
        m_subindex.push_back(entry);
        m_header.m_numEntries = m_numEntries = m_subindex.size();
        sort();
    }

    fseek(m_fileptr, 0, SEEK_SET);
    fwrite(&m_header, sizeof(LODHeader), 1, m_fileptr);
    fwrite(&m_subindex[0], sizeof(LODEntry), m_numEntries, m_fileptr);
    printf("%s added at file offset: %lu\n", entry.m_name, offset);
    delete[] buffer;
    return LOD_NO_ERROR;
}

int LODFile::deleteItem(const char* itemName)
{
    if (!m_opened)
        return LOD_NOT_OPEN;

    long offset = 0;
    int inPlace = 0;
    if (exist(itemName)) {
        Find(0, m_numEntries, itemName);
        if (m_matchindex < 0)
            return LOD_ITEM_NOT_FOUND;
    }
    m_subindex.erase(m_subindex.begin() + m_matchindex);
    m_header.m_numEntries = m_numEntries = m_subindex.size();

    fseek(m_fileptr, 0, SEEK_SET);
    fwrite(&m_header, sizeof(LODHeader), 1, m_fileptr);
    fwrite(&m_subindex[0], sizeof(LODEntry), m_numEntries, m_fileptr);
    LODEntry empty;
    fwrite(&empty, sizeof(LODEntry), 1, m_fileptr);
    printf("%s deleted\n", itemName);
    return LOD_NO_ERROR;
}

LODEntry::LODEntry()
{
    m_name[0] = 0;
    m_offset = 0;
    m_size = 0;
    m_attrib = 0;
    m_csize = 0;
}

LODFile::LODFile()
{
    m_fileptr = NULL;
    m_opened = 0;
    clear();
}

LODFile::~LODFile()
{
    if (!m_opened)
        return;
    fclose(m_fileptr);
    m_subindex.clear();
    delete[] m_dataBuffer;
}

LODHeader::LODHeader()
{
    strcpy(m_lodId, "LOD");
    m_version = 500;
    m_numEntries = 0;
    memset(m_reserved, 0, sizeof(m_reserved));
}

int LODFile::open(const char* filename, int flags)
{
    if (m_opened)
        clear();

    if (flags & 1)
        m_fileptr = fopen(filename, "rb");
    else
        m_fileptr = fopen(filename, "rb+");
    if (!m_fileptr)
        return LOD_NOT_OPEN;

    strcpy(m_lodFileName, filename);
    fread(&m_header, sizeof(LODHeader), 1, m_fileptr);
    m_numEntries = m_header.m_numEntries;
    m_subindex.resize(m_numEntries);
    fread(&m_subindex[0], sizeof(LODEntry), m_numEntries, m_fileptr);
    fseek(m_fileptr, 0, SEEK_SET);
    m_opened = 1;
    return LOD_NO_ERROR;
}

int compare(const void* arg1, const void* arg2)
{
    const LODEntry* entry1 = (const LODEntry*)arg1;
    const LODEntry* entry2 = (const LODEntry*)arg2;
    return strcasecmp(entry1->m_name, entry2->m_name);
}

void LODFile::sort()
{
    qsort(&m_subindex[0], m_numEntries, sizeof(LODEntry), compare);
}

int LODFile::pack()
{
    if (!m_opened)
        return LOD_NOT_OPEN;

    int i;
    FILE* packed;
    LODHeader header;
    packed = fopen("packed.lod", "wb+");
    if (!packed)
        return LOD_NOT_OPEN;
    LODEntry* entries = new LODEntry[LOD_MAX_ENTRIES];
    memset(entries, 0, LOD_MAX_ENTRIES * sizeof(LODEntry));
    fwrite(&header, sizeof(LODHeader), 1, packed);
    fwrite(entries, sizeof(LODEntry), LOD_MAX_ENTRIES, packed);
    delete[] entries;
    entries = NULL;

    int bufferSize = 0x20000;
    char* buffer = new char[bufferSize];
    for (i = 0; i < m_numEntries; i++) {
        fseek(m_fileptr, m_subindex[i].m_offset, SEEK_SET);
        int remaining = m_subindex[i].m_csize > 0 ? m_subindex[i].m_csize : m_subindex[i].m_size;
        int offset = ftell(packed);
        while (remaining > 0) {
            int chunk = remaining > bufferSize ? bufferSize : remaining;
            fread(buffer, 1, chunk, m_fileptr);
            fwrite(buffer, 1, chunk, packed);
            remaining -= chunk;
        }
        m_subindex[i].m_offset = offset;
    }
    delete[] buffer;

    i = 0;
    int oldCount = m_numEntries;
    while (i < m_numEntries) {
        if (strlen(m_subindex[i].m_name) == 0 || m_subindex[i].m_size == 0) {
            m_subindex.erase(m_subindex.begin() + i);
            m_numEntries = m_header.m_numEntries = m_subindex.size();
        } else
            i++;
    }

    fseek(packed, 0, SEEK_SET);
    fwrite(&m_header, sizeof(LODHeader), 1, packed);
    fwrite(&m_subindex[0], sizeof(LODEntry), m_numEntries, packed);
    LODEntry empty;
    for (i = 0; i < oldCount - m_numEntries; i++)
        fwrite(&empty, sizeof(LODEntry), 1, packed);
    fclose(packed);
    return LOD_NO_ERROR;
}

int LODFile::squishme()
{
    if (!m_opened)
        return LOD_NOT_OPEN;

    int i;
    char* buffer = NULL;
    LODEntry entry;
    for (i = 0; i < m_numEntries; i++) {
        entry = m_subindex[i];
        buffer = (char*)realloc(buffer, entry.m_size);
        pointAt(entry.m_name);
        read(buffer, entry.m_size);
        addItem(entry, buffer, 0, true);
    }
    delete[] buffer;
    return LOD_NO_ERROR;
}

bool LODFile::pointAt(const char* itemName)
{
    if (!getDataPtr(itemName)) {
        m_dataItemIndex = -1;
        m_dataPos = -1;
        return false;
    } else {
        m_dataItemIndex = m_matchindex;
        m_dataPos = 0;
        delete[] m_dataBuffer;
        m_dataBuffer = NULL;
        m_dataBufferSize = 0;
        return true;
    }
}

int LODFile::read(void* dest, int numBytes)
{
    if (!m_opened)
        return -1;
    if (m_dataItemIndex == -1)
        return -1;

    LODEntry entry = m_subindex[m_dataItemIndex];
    bool compressed = entry.m_csize != 0;
    if (compressed) {
        if (m_dataBuffer == NULL) {
            m_dataBuffer = new unsigned char[entry.m_size];
            m_dataBufferSize = entry.m_size;
            m_dataPos = 0;
            unsigned char* packed = new unsigned char[entry.m_csize];
            fread(packed, 1, entry.m_csize, m_fileptr);
            uLongf length = m_dataBufferSize;
            uncompress(m_dataBuffer, &length, packed, entry.m_csize);
            delete[] packed;
        }
        memcpy(dest, m_dataBuffer + m_dataPos, numBytes);
        m_dataPos += numBytes;
    } else {
        fread(dest, 1, numBytes, m_fileptr);
    }
    return 0;
}
