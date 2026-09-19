#ifndef HOMM3_SAMPLE_H
#define HOMM3_SAMPLE_H

#include "resource.h"

class ds_memsample;

// Dreamcast sample owns MemorySampleStructure memSample (types 0x1c9a /
// 0x51ce). The DC structure has four words; the PC version adds data and
// size after the handle. NH3API core/resources/sounds.hpp supplies that
// 0x18-byte PC layout. Retail sample::sample 0x566da0 independently stores
// channel/volume/loop at +0x28/+0x2c/+0x30, allocates data at +0x20, stores
// its byte count at +0x24, and clears the handle at +0x1c.
struct MemorySampleStructure {
    // Miles handle assigned by soundManager::memorySample.
    ds_memsample* m_memSampleHandle;
    void* m_data;
    // NH3API size (size_t); retail getSize 0x566e90 adds this byte count
    // to sizeof(sample).
    unsigned int m_size;
    // Sound-channel range, volume, and AIL loop count for playback.
    int m_memCindex;
    int m_memVolume;
    int m_memLooping;
};
SIZE(MemorySampleStructure, 0x18);

class sample : public resource {
public:
    // Original Dreamcast/NH3API member memSample; PC resource base is 0x1c.
    MemorySampleStructure m_memSample;

    sample(const char* newName, const void* src, long len,
           long channel, long volume, long loop);
    // DC records an ordinary destructor. Complete resource's virtual
    // destructor requires this override; slot 0 of 0x6416d0 proves it.
    virtual ~sample();
    virtual unsigned int getSize() const;
};
SIZE(sample, 0x34);

#endif  /* HOMM3_SAMPLE_H */
