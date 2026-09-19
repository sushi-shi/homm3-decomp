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

// --- globals ---

// --- LODEntry ---
// CODEVIEW(E:\gamedcs\lodfile.cpp:226, dc 0xe92f8) void LODEntry::LODEntry();

// --- LODFile ---
// CODEVIEW(E:\gamedcs\lodfile.cpp:53, dc 0xe90c0) int LODFile::GetFileSize();
// CODEVIEW(E:\gamedcs\lodfile.cpp:72, dc 0xe9100) void* LODFile::getDataPtr(const char* item_name);
// CODEVIEW(E:\gamedcs\lodfile.cpp:341, dc 0xe955c) void LODFile::set_filemap(unsigned char on);

// --- LODHeader ---
// CODEVIEW(E:\gamedcs\lodfile.cpp:266, dc 0xe93bc) void LODHeader::LODHeader();

// --- std ---
// CODEVIEW(..\stlport\stl_vector.h:203, dc 0xe9868) LODEntry* std::vector<LODEntry,std::allocator<LODEntry> >::operator[](unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:218, dc 0xe9888) void std::vector<LODEntry,std::allocator<LODEntry> >::vector<LODEntry,std::allocator<LODEntry> >(const std::allocator<LODEntry>* __a);
// CODEVIEW(..\stlport\stl_vector.h:288, dc 0xe98a4) void std::vector<LODEntry,std::allocator<LODEntry> >::~vector<LODEntry,std::allocator<LODEntry> >();
// CODEVIEW(..\stlport\stl_vector.h:505, dc 0xe98cc) void std::vector<LODEntry,std::allocator<LODEntry> >::resize(unsigned __new_size);
// CODEVIEW(..\stlport\stl_vector.h:506, dc 0xe98fc) void std::vector<LODEntry,std::allocator<LODEntry> >::clear();
// CODEVIEW(..\stlport\stl_alloc.h:527, dc 0xe9934) void std::allocator<LODEntry>::allocator<LODEntry>();
// CODEVIEW(..\stlport\stl_alloc.h:537, dc 0xe9938) void std::allocator<LODEntry>::~allocator<LODEntry>();
// CODEVIEW(..\stlport\stl_vector.h:179, dc 0xe993c) LODEntry* std::vector<LODEntry,std::allocator<LODEntry> >::begin();
// CODEVIEW(..\stlport\stl_vector.h:181, dc 0xe9940) LODEntry* std::vector<LODEntry,std::allocator<LODEntry> >::end();
// CODEVIEW(..\stlport\stl_vector.h:490, dc 0xe9944) LODEntry* std::vector<LODEntry,std::allocator<LODEntry> >::erase(LODEntry* __first, LODEntry* __last);
// CODEVIEW(..\stlport\stl_vector.h:499, dc 0xe9980) void std::vector<LODEntry,std::allocator<LODEntry> >::resize(unsigned __new_size, const LODEntry* __x);
// CODEVIEW(..\stlport\stl_vector.h:89, dc 0xe99fc) void std::_Vector_base<LODEntry,std::allocator<LODEntry> >::_Vector_base<LODEntry,std::allocator<LODEntry> >(const std::allocator<LODEntry>* __a);
// CODEVIEW(..\stlport\stl_vector.h:101, dc 0xe9a28) void std::_Vector_base<LODEntry,std::allocator<LODEntry> >::~_Vector_base<LODEntry,std::allocator<LODEntry> >();
// CODEVIEW(..\stlport\stl_bvector.h:101, dc 0xe9a58) void std::_STL_alloc_proxy<LODEntry *,LODEntry,std::allocator<LODEntry> >::~_STL_alloc_proxy<LODEntry *,LODEntry,std::allocator<LODEntry> >();
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0xe9a70) unsigned std::vector<LODEntry,std::allocator<LODEntry> >::size();
// CODEVIEW(..\stlport\stl_vector.h:472, dc 0xe9a7c) void std::vector<LODEntry,std::allocator<LODEntry> >::insert(LODEntry* __pos, unsigned __n, const LODEntry* __x);
// CODEVIEW(..\stlport\stl_alloc.h:1004, dc 0xe9a94) void std::_STL_alloc_proxy<LODEntry *,LODEntry,std::allocator<LODEntry> >::_STL_alloc_proxy<LODEntry *,LODEntry,std::allocator<LODEntry> >(const std::allocator<LODEntry>* __a, LODEntry** __p);
// CODEVIEW(..\stlport\stl_alloc.h:1025, dc 0xe9aa0) void std::_STL_alloc_proxy<LODEntry *,LODEntry,std::allocator<LODEntry> >::deallocate(LODEntry* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:552, dc 0xe9acc) void std::allocator<LODEntry>::deallocate(LODEntry* __p, unsigned __n);
// CODEVIEW(..\stlport\stl_vector.c:283, dc 0xe9ae8) void std::vector<LODEntry,std::allocator<LODEntry> >::_M_fill_insert(LODEntry* __position, unsigned __n, const LODEntry* __x);
// CODEVIEW(..\stlport\stl_construct.h:128, dc 0xe9bd8) void std::destroy(LODEntry* __first, LODEntry* __last);
// CODEVIEW(..\stlport\stl_algobase.h:322, dc 0xe9c08) LODEntry* std::copy(LODEntry* __first, LODEntry* __last, LODEntry* __result);
// CODEVIEW(..\stlport\stl_alloc.h:968, dc 0xe9c58) std::allocator<LODEntry>* std::__stl_alloc_rebind(std::allocator<LODEntry>* __a, const LODEntry* __formal);
// CODEVIEW(..\stlport\stl_vector.c:248, dc 0xe9c5c) void std::vector<LODEntry,std::allocator<LODEntry> >::_M_insert_overflow(LODEntry* __position, const LODEntry* __x, unsigned __fill_len);
// CODEVIEW(..\stlport\stl_uninitialized.h:97, dc 0xe9d2c) LODEntry* std::uninitialized_copy(LODEntry* __first, LODEntry* __last, LODEntry* __result);
// CODEVIEW(..\stlport\stl_algobase.h:442, dc 0xe9d64) LODEntry* std::copy_backward(LODEntry* __first, LODEntry* __last, LODEntry* __result);
// CODEVIEW(..\stlport\stl_algobase.h:495, dc 0xe9db4) void std::fill(LODEntry* __first, LODEntry* __last, const LODEntry* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:263, dc 0xe9df4) LODEntry* std::uninitialized_fill_n(LODEntry* __first, unsigned __n, const LODEntry* __x);
// CODEVIEW(..\stlport\stl_iterator_base.h:262, dc 0xe9e2c) LODEntry* std::value_type(const LODEntry* __formal);
// CODEVIEW(..\stlport\stl_construct.h:121, dc 0xe9e30) void std::__destroy(LODEntry* __first, LODEntry* __last, LODEntry* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:243, dc 0xe9e4c) std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const LODEntry* __formal);
// CODEVIEW(..\stlport\stl_iterator_base.h:291, dc 0xe9e58) int* std::distance_type(const LODEntry* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:209, dc 0xe9e5c) LODEntry* std::__copy(LODEntry* __first, LODEntry* __last, LODEntry* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_alloc.h:1022, dc 0xe9ea8) LODEntry* std::_STL_alloc_proxy<LODEntry *,LODEntry,std::allocator<LODEntry> >::allocate(unsigned __n);
// CODEVIEW(..\stlport\stl_alloc.h:547, dc 0xe9ed0) LODEntry* std::allocator<LODEntry>::allocate(unsigned __n, const void* __formal);
// CODEVIEW(..\stlport\stl_construct.h:85, dc 0xe9ef8) void std::construct(LODEntry* __p, const LODEntry* __value);
// CODEVIEW(..\stlport\stl_uninitialized.h:88, dc 0xe9f3c) LODEntry* std::__uninitialized_copy(LODEntry* __first, LODEntry* __last, LODEntry* __result, LODEntry* __formal);
// CODEVIEW(..\stlport\stl_algobase.h:382, dc 0xe9f58) LODEntry* std::__copy_backward(LODEntry* __first, LODEntry* __last, LODEntry* __result, std::random_access_iterator_tag __formal, int* __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:255, dc 0xe9fa8) LODEntry* std::__uninitialized_fill_n(LODEntry* __first, unsigned __n, const LODEntry* __x, LODEntry* __formal);
// CODEVIEW(..\stlport\stl_construct.h:110, dc 0xe9fc4) void std::__destroy_aux(LODEntry* __first, LODEntry* __last, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:70, dc 0xe9ff4) LODEntry* std::__uninitialized_copy_aux(LODEntry* __first, LODEntry* __last, LODEntry* __result, __false_type __formal);
// CODEVIEW(..\stlport\stl_uninitialized.h:239, dc 0xea030) LODEntry* std::__uninitialized_fill_n_aux(LODEntry* __first, unsigned __n, const LODEntry* __x, __false_type __formal);
// CODEVIEW(..\stlport\stl_construct.h:59, dc 0xea06c) void std::destroy(LODEntry* __pointer);
// CODEVIEW(..\stlport\stl_construct.h:53, dc 0xea088) void std::__destroy_aux();

#endif  /* HOMM3_LODFILE_H */
