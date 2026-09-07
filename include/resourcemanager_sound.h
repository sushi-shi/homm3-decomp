#ifndef HOMM3_RESOURCEMANAGER_SOUND_H
#define HOMM3_RESOURCEMANAGER_SOUND_H

#include <memory>
#include <windows.h>

// Dreamcast names the 48-byte record; retail independently proves every
// field through GetSoundFile's filename comparison and file read.
struct SoundHeaderStruct {
    // Before normalization: filename.
    char m_filename[40];
    int m_offset;
    int m_size;
};
SIZE(SoundHeaderStruct, 0x30);

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

// Before normalization: gSoundHeaderDescriptors.
extern TSoundHeaderDescriptor g_soundHeaderDescriptors[3];

namespace ResourceManager {
// Before normalization (function): ResourceManager::GetSoundFile.
bool getSoundFile(const char* localName, std::auto_ptr<char>& data, int* size);
}

#endif
