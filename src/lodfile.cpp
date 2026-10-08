#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "lodfile.h"

VA(0x004fa590, 0x77)
DC_ADDRESS(0x0e908c, 0x34)
MAC_ADDRESS(0x11b598, 0x5c)
void LODFile::clear()
{
    if (opened) {
        subindex.clear();
        fclose(fileptr);
        delete[] dataBuffer;
        opened = 0;
    }
}

// E:\gamedcs\lodfile.cpp:72
// DC's getDataPtr is an ordinary helper called at pointAt line 431.
// The PC expansion seeks the archive stream before returning its handle.
DC_ADDRESS(0x0e9100, 0x54)
MAC_ADDRESS(0x11b5f4, 0x8c)
void* LODFile::getDataPtr(const char* itemName)
{
    if (!opened)
        return 0;
    Find(0, numEntries, itemName);
    if (matchindex >= 0) {
        fseek(fileptr, subindex[matchindex].offset, SEEK_SET);
        dataItemIndex = matchindex;
        return fileptr;
    }
    return 0;
}

VA(0x004fa610, 0x45)
DC_ADDRESS(0x0e9154, 0x42)
MAC_ADDRESS(0x11b680, 0x70)
LODEntry* LODFile::getItemIndex(const char* itemName)
{
    if (!opened)
        return 0;
    Find(0, numEntries, itemName);
    if (matchindex >= 0)
        return &subindex[matchindex];
    return 0;
}

// Original: LODFile::exist; lodfile.cpp:112
DC_ADDRESS(0x0e9198, 0x26)
// Loki h3maped (exist__7LODFilePCc) returns bool through an explicit pair.
bool LODFile::exist(const char* itemName)
{
    Find(0, numEntries, itemName);
    if (matchindex >= 0)
        return true;
    return false;
}

// Mac retains both recursive calls at 0x11b770 and 0x11b7d8. The Windows
// retail body at 0x4fa660 carries equivalent loops at the same two branches.
VA(0x004fa660, 0x113)
DC_ADDRESS(0x0e91c0, 0xf2)
MAC_ADDRESS(0x11b6f0, 0x148)
void LODFile::Find(unsigned begin, unsigned end, const char* itemName)
{
    if (begin == end) {
        matchindex = -1;
        return;
    }

    unsigned half = (end - begin) / 2;
    unsigned i;
    int order = strcmpi(itemName, subindex[begin + half].name);
    if (order == 0) {
        matchindex = begin + half;
        return;
    }
    if (order < 0) {
        if (end - begin > 4) {
            Find(begin, begin + half, itemName);
        } else {
            for (i = begin; i < end; i++) {
                if (strcmpi(itemName, subindex[i].name) == 0) {
                    matchindex = i;
                    return;
                }
            }
            matchindex = -1;
        }
    } else {
        if (end - begin > 4) {
            Find(begin + half, end, itemName);
        } else {
            for (i = begin; i < end; i++) {
                if (strcmpi(itemName, subindex[i].name) == 0) {
                    matchindex = i;
                    return;
                }
            }
            matchindex = -1;
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
    name[0] = 0;
    offset = 0;
    size = 0;
    attrib = 0;
    csize = 0;
}

// E:\gamedcs\lodfile.cpp:266.  No retail row of its own: /Ob2 folds it
// into the one call site below, where the "LOD" strcpy expands to VC6's
// repne scasb + rep movs pair.
DC_ADDRESS(0x0e93bc, 0x60)
MAC_ADDRESS(0x11b9b0, 0x50)
LODHeader::LODHeader()
{
    strcpy(LOD_ID, DATA_COMPGEN(0x0067fa58, lodSignature, "LOD"));
    version = 500;
    numEntries = 0;
    memset(reserved, 0, sizeof(reserved));
}

VA(0x004fa780, 0x7B)
DC_ADDRESS(0x0e9330, 0x42)
MAC_ADDRESS(0x11b854, 0x7c)
LODFile::LODFile()
{
    fileptr = 0;
    opened = 0;
    clear();
    dataBuffer = 0;
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
    if (opened)
        clear();

    if (flags & 1)
        fileptr = fopen(filename,
            DATA_COMPGEN(0x00677d6c, lodReadMode, "rb"));
    else
        fileptr = fopen(filename,
            DATA_COMPGEN(0x0067fa5c, lodUpdateMode, "rb+"));
    if (!fileptr)
        return 1;

    strcpy(LODFileName, filename);
    fread(&header, sizeof(header), 1, fileptr);
    numEntries = header.numEntries;
    subindex.resize(numEntries);
    fread(&subindex[0], sizeof(LODEntry), numEntries, fileptr);
    fseek(fileptr, 0, SEEK_SET);
    opened = 1;
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
    return strcmpi(static_cast<const LODEntry*>(arg1)->name,
                    static_cast<const LODEntry*>(arg2)->name);
}

// Original: LODFile::sort; lodfile.cpp:402
DC_ADDRESS(0x0e9668, 0x28)
void LODFile::sort()
{
    qsort(&subindex[0], numEntries, sizeof(LODEntry), compare);
}

VA(0x004faa70, 0xAB)
DC_ADDRESS(0x0e9690, 0x4e)
MAC_ADDRESS(0x11bb74, 0x80)
// Original public0x593733 ?pointAt@LODFile@@QAA_NPBD@Z proves bool,
// despite byte lowering. Fresh VC6 emits QAE_NPBD rather than QAEEPBD;
// all17 lodfile and235 resourcemanager code sections remain byte-identical.
// The owner comparison requires refreshed target labels for the bool name.
bool LODFile::pointAt(const char* itemName)
{
    if (!getDataPtr(itemName)) {
        dataItemIndex = -1;
        dataPos = -1;
        return false;
    } else {
        dataItemIndex = matchindex;
        dataPos = 0;
        delete[] dataBuffer;
        dataBuffer = 0;
        dataBufferSize = 0;
        return true;
    }
}

VA(0x004fab20, 0x114)
DC_ADDRESS(0x0e96e0, 0x188)
MAC_ADDRESS(0x11bbf4, 0x174)
int LODFile::read(void* dest, int numBytes)
{
    if (!opened)
        return -1;
    if (dataItemIndex == -1)
        return -1;

    LODEntry entry = subindex[dataItemIndex];
    bool compressed = entry.csize != 0;
    if (compressed) {
        if (dataBuffer == 0) {
            dataBuffer = new unsigned char[entry.size];
            dataBufferSize = entry.size;
            dataPos = 0;
            unsigned char* packed = new unsigned char[entry.csize];
            fread(packed, 1, entry.csize, fileptr);
            unsigned long destLen = dataBufferSize;
            uncompress(dataBuffer, &destLen, packed, entry.csize);
            delete[] packed;
        }
        memcpy(dest, dataBuffer + dataPos, numBytes);
        dataPos += numBytes;
    } else {
        fread(dest, 1, numBytes, fileptr);
    }
    return 0;
}

VA_COMPGEN(0x004fac40, 0x26B, VECTOR_INSERT, LODEntry)
