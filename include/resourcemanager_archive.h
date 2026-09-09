// Resource archive selection and disk-header layouts shared by the resource readers.
#ifndef HOMM3_RESOURCEMANAGER_ARCHIVE_H
#define HOMM3_RESOURCEMANAGER_ARCHIVE_H

#include <windows.h>
#include <va.h>

struct SoundHeaderStruct;

// The four state-selected rows at retail 0x69e538 contain three
// (count, LOD-index-list) pairs. The first serves sprites, the second
// bitmaps, and the final pair serves the sound-header archives.
struct TResourceArchiveList {
    // Before normalization: count.
    int m_count;
    // Before normalization: indices.
    int* m_indices;
};

struct TResourceArchiveContext {
    // Before normalization: sprites.
    TResourceArchiveList m_sprites;
    // Before normalization: bitmaps.
    TResourceArchiveList m_bitmaps;
    // Before normalization: sounds.
    TResourceArchiveList m_sounds;
};
SIZE(TResourceArchiveContext, 0x18);

// Dreamcast CodeView's function-local GetBitmap16 record (type 0x289c),
// independently byte-proven by retail's three archive-header reads.
struct TBitmapResourceHeader {
    // Before normalization: DataSize.
    int m_dataSize;
    // Before normalization: Width.
    int m_width;
    // Before normalization: Height.
    int m_height;
};
SIZE(TBitmapResourceHeader, 0x0c);

struct TResourceLODSlot;
// Before normalization: gResourceLODSlots.
extern TResourceLODSlot g_resourceLodSlots[];

// Before normalization: gResourceArchiveContexts.
extern TResourceArchiveContext g_resourceArchiveContexts[4];

// Three retail descriptors at 0x69e500. Each points at one header array,
// its count, and the Windows file handle used for the positioned read.
struct TSoundHeaderDescriptor {
    // Before normalization: sounds.
    SoundHeaderStruct** m_sounds;
    // Before normalization: count.
    int* m_count;
    // Before normalization: file.
    HANDLE* m_file;
};
SIZE(TSoundHeaderDescriptor, 0x0c);

#endif
