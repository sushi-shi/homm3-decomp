// soundheader.h - one entry of a sound archive's directory.
// Dreamcast SoundHeaderStruct proves filename[40], signed int offset/size
// at +40/+44 and size 48; Loki's ResourceManager::GetSoundFile walks the
// global SoundHeader/SoundHeaderCD tables with the same layout.
#ifndef HOMM3_SOUNDHEADER_H
#define HOMM3_SOUNDHEADER_H

struct SoundHeaderStruct {
    char m_filename[40];
    int m_offset;
    int m_size;
};

// Loki h3maped's exported sound directories (the sound module fills them).
extern SoundHeaderStruct* SoundHeader;
extern int numSound;
extern SoundHeaderStruct* SoundHeaderCD;
extern int numSoundCD;

#endif  /* HOMM3_SOUNDHEADER_H */
