// 7 functions in link order.
#include "va.h"

#include "csequence.h"

// Original: CSequence::CSequence; csequence.cpp:32
DC_ADDRESS(0x071f14, 0xc)
CSequence::CSequence()
    : m_numFrames(0), m_allocatedFrames(0), m_f(0)
{
}

DC_ADDRESS(0x071f20, 0x40)
VA(0x0047b840, 0x44) MAC_ADDRESS(0x08a014, 0x118)
CSequence::CSequence(int num)
{
    m_numFrames = 0;
    m_allocatedFrames = num;
    m_f = new CSpriteFrame*[num];
    MEMSET_LOCAL(m_f, 0, num * sizeof(m_f[0]), num, i);
}

DC_ADDRESS(0x071f60, 0x18)
VA(0x0047b890, 0x0F) MAC_ADDRESS(0x08a12c, 0x5c)
CSequence::~CSequence()
{
    if (m_f)
        delete[] m_f;
}

// Original: CSequence::AddFrame; csequence.cpp:68
DC_ADDRESS(0x071f78, 0x4e)
int CSequence::addFrame(const char* name)
{
    if (m_numFrames < m_allocatedFrames) {
        m_f[m_numFrames++] = new CSpriteFrame(name, 0);
        return m_numFrames;
    }
    return 0;
}

// Original: CSequence::AddFrame; csequence.cpp:79
DC_ADDRESS(0x071fc8, 0x72)
int CSequence::addFrame(const char* name, int w, int h, unsigned char* data,
                        int csize, TEncodingMethod encoding)
{
    if (m_numFrames < m_allocatedFrames) {
        m_f[m_numFrames++] = new CSpriteFrame(name, w, h, data, csize, encoding);
        return m_numFrames;
    }
    return 0;
}

// Original: CSequence::AddFrame; csequence.cpp:91
DC_ADDRESS(0x07203c, 0x8a)
int CSequence::addFrame(const char* name, int w, int h, unsigned char* data,
                        int csize, TEncodingMethod encoding,
                        int croppedWidth, int croppedHeight, int croppedX, int croppedY)
{
    if (m_numFrames < m_allocatedFrames) {
        m_f[m_numFrames++] = new CSpriteFrame(name, w, h, data, csize, encoding,
                                             croppedWidth, croppedHeight,
                                             croppedX, croppedY);
        return m_numFrames;
    }
    return 0;
}

DC_ADDRESS(0x0720c8, 0x38)
VA(0x0047b8a0, 0x26) MAC_ADDRESS(0x08a188, 0x34)
int CSequence::addFrame(CSpriteFrame* frame)
{
    if (m_numFrames < m_allocatedFrames) {
        m_f[m_numFrames] = frame;
        ++m_numFrames;
        return m_numFrames;
    }
    return 0;
}
