#ifndef HOMM3_RESOURCEMANAGER_SOUND_H
#define HOMM3_RESOURCEMANAGER_SOUND_H

#include <memory>
#include <windows.h>

// Dreamcast names the 48-byte record; retail independently proves every
// field through GetSoundFile's filename comparison and file read.
struct SoundHeaderStruct {
    char filename[40];
    int offset;
    int size;
};
SIZE(SoundHeaderStruct, 0x30);

// Three retail descriptors at 0x69e500. Each points at one header array,
// its count, and the Windows file handle used for the positioned read.
struct TSoundHeaderDescriptor {
    SoundHeaderStruct** sounds;
    int* count;
    HANDLE* file;
};
SIZE(TSoundHeaderDescriptor, 0x0c);

extern TSoundHeaderDescriptor gSoundHeaderDescriptors[3];

namespace ResourceManager {
bool GetSoundFile(const char* localName, std::auto_ptr<char>& data, int* size);
}

#endif
