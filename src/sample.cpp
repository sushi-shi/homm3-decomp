// 2 functions in link order.
#include "va.h"

#include <string.h>

#include "sample.h"

#include "terrain.h"

// Mac embeds a native sound owner: 0x276464 decodes RIFF/WAVE or AIFF/AIFC
// before playback; Windows Miles receives the copied file bytes directly.
VA(0x00566da0, 0x8E) MAC_ADDRESS(0x15da8c, 0x84)  // dc 0x129b3c
sample::sample(const char* newName, const void* src, long len,
               long channel, long volume, long loop)
    : resource(newName, RESOURCE_TYPE_SFX)
{
    m_memSample.m_memCindex = channel;
    m_memSample.m_memVolume = volume;
    m_memSample.m_memLooping = loop;
    m_memSample.m_data = new char[len];
    m_memSample.m_size = len;
    memcpy(m_memSample.m_data, src, len);
    m_memSample.m_memSampleHandle = 0;
}

VA_COMPGEN(0x00566e30, 0x21, SCALAR_DELETING_DTOR, sample)

VA(0x00566e60, 0x29) MAC_ADDRESS(0x15db68, 0x8c)  // dc 0x129b4c
sample::~sample()
{
    delete m_memSample.m_data;
    m_memSample.m_data = 0;
    m_memSample.m_size = 0;
    m_memSample.m_memVolume = 0;
}

// The third sample vtable entry at 0x6416d8 fixes this compact override;
// its constant is the complete 0x34-byte object followed by owned sample data.
VA(0x00566e90, 0x07) MAC_ADDRESS(0x15dbf4, 0xc)
unsigned int sample::getSize() const
{
    return sizeof(sample) + m_memSample.m_size;
}
