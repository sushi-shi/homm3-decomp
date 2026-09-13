// sample.cpp - E:\gamedcs\sample.cpp (compiland sample.obj)
// 2 functions in link order.
#include "terrain.h"
#include <va.h>
#include <string.h>
#include "sample.h"

// The ai_combat lever, applied to a base-subobject unwind instead of a
// scope-exit one: under /GX the dtor's `delete data` is a throwing call
// with the ~resource base still to run, so VC6 wraps ~sample in a whole
// fs:[0] frame. Retail's ~sample (0x566e60) is frameless while its ctor
// (0x566da0) carries the frame the *new* demands - the two together
// prove /GX plus a nothrow-visible operator delete.
__declspec(nothrow) void __cdecl operator delete(void* p);

VA(0x00566da0, 0x8E)  // dc 0x129b3c
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

VA(0x00566e60, 0x29)  // dc 0x129b4c
sample::~sample()
{
    delete m_memSample.m_data;
    m_memSample.m_data = 0;
    m_memSample.m_size = 0;
    m_memSample.m_memVolume = 0;
}

// The third sample vtable entry at 0x6416d8 fixes this compact override;
// its constant is the complete 0x34-byte object followed by owned sample data.
VA(0x00566e90, 0x07)
unsigned int sample::getSize() const
{
    return sizeof(sample) + m_memSample.m_size;
}
