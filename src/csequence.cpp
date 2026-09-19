// csequence.cpp - E:\gamedcs\csequence.cpp (compiland csequence.obj)
// 7 functions in link order.
#include <va.h>
#include "csequence.h"

// Original: CSequence::CSequence; csequence.cpp:32, dc 0x71f14.
CSequence::CSequence()
    : m_numFrames(0), m_allocatedFrames(0), m_f(0)
{
}

VA(0x0047b840, 0x44)  // dc 0x71f20
CSequence::CSequence(int num)
{
    m_numFrames = 0;
    m_allocatedFrames = num;
    m_f = new CSpriteFrame*[num];
    for (int i = 0; i < num; ++i)
        m_f[i] = 0;
}

VA(0x0047b890, 0x0F)  // dc 0x71f60
CSequence::~CSequence()
{
    if (m_f)
        delete[] m_f;
}

// Original: CSequence::AddFrame; csequence.cpp:68, dc 0x71f78.
int CSequence::addFrame(const char* name)
{
    if (m_numFrames < m_allocatedFrames) {
        m_f[m_numFrames++] = new CSpriteFrame(name, 0);
        return m_numFrames;
    }
    return 0;
}

// Original: CSequence::AddFrame; csequence.cpp:79, dc 0x71fc8.
int CSequence::addFrame(const char* name, int w, int h, unsigned char* data,
                        int csize, TEncodingMethod encoding)
{
    if (m_numFrames < m_allocatedFrames) {
        m_f[m_numFrames++] = new CSpriteFrame(name, w, h, data, csize, encoding);
        return m_numFrames;
    }
    return 0;
}

// Original: CSequence::AddFrame; csequence.cpp:91, dc 0x7203c.
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

VA(0x0047b8a0, 0x26)  // dc 0x720c8
int CSequence::addFrame(CSpriteFrame* frame)
{
    if (m_numFrames < m_allocatedFrames) {
        m_f[m_numFrames] = frame;
        ++m_numFrames;
        return m_numFrames;
    }
    return 0;
}
