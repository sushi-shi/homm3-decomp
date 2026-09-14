// Resource archive selection and disk-header layouts shared by the resource readers.
#ifndef HOMM3_RESOURCEMANAGER_ARCHIVE_H
#define HOMM3_RESOURCEMANAGER_ARCHIVE_H

#include <windows.h>
#include <va.h>

struct SoundHeaderStruct;

// The four state-selected rows at retail 0x69e538 contain three
// (count, LOD-index-list) pairs. The first serves sprites, the second
// bitmaps, and the final pair serves the sound-header archives.
// Before normalization (type): TResourceArchiveList.
#ifndef ResourceArchiveList
#define ResourceArchiveList TResourceArchiveList
#endif
struct ResourceArchiveList {
public:
    int m_count;
    int* m_indices;
};

// Before normalization (type): TResourceArchiveContext.
#ifndef ResourceArchiveContext
#define ResourceArchiveContext TResourceArchiveContext
#endif
struct ResourceArchiveContext {
public:
    ResourceArchiveList m_sprites;
    ResourceArchiveList m_bitmaps;
    ResourceArchiveList m_sounds;
};
SIZE(ResourceArchiveContext, 0x18);

// Dreamcast CodeView's function-local GetBitmap16 record (type 0x289c),
// independently byte-proven by retail's three archive-header reads.
// Before normalization (type): TBitmapResourceHeader.
#ifndef BitmapResourceHeader
#define BitmapResourceHeader TBitmapResourceHeader
#endif
struct BitmapResourceHeader {
public:
    int m_dataSize;
    int m_width;
    int m_height;
};
SIZE(BitmapResourceHeader, 0x0c);

// Before normalization (type): TResourceLODSlot.
#ifndef ResourceLODSlot
#define ResourceLODSlot TResourceLODSlot
#endif
struct ResourceLODSlot;
extern ResourceLODSlot g_resourceLodSlots[];
extern ResourceArchiveContext g_resourceArchiveContexts[4];

// Three retail descriptors at 0x69e500. Each points at one header array,
// its count, and the Windows file handle used for the positioned read.
// Before normalization (type): TSoundHeaderDescriptor.
#ifndef SoundHeaderDescriptor
#define SoundHeaderDescriptor TSoundHeaderDescriptor
#endif
struct SoundHeaderDescriptor {
public:
    SoundHeaderStruct** m_sounds;
    int* m_count;
    HANDLE* m_file;
};
SIZE(SoundHeaderDescriptor, 0x0c);

#endif
