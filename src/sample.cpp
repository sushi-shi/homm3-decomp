// sample.cpp - Loki h3maped object 50 (file name inferred from the class):
// an in-memory sound effect resource.
#include <string.h>

#include "sample.h"

sample::sample(const char* name, char* src, int len, long channel, long volume, long loop)
    : resource(name, RESOURCE_TYPE_SFX)
{
    memSample.m_memCindex = channel;
    memSample.m_memVolume = volume;
    memSample.m_memLooping = loop;
    unsigned int size = len;
    memSample.m_data = new char[size];
    memSample.m_size = size;
    memcpy(memSample.m_data, src, size);
    memSample.m_memSampleHandle = 0;
}

sample::~sample()
{
    delete[] memSample.m_data;
    memSample.m_data = 0;
    memSample.m_size = 0;
    memSample.m_memVolume = 0;
}
