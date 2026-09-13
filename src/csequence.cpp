// csequence.cpp - E:\gamedcs\csequence.cpp (compiland csequence.obj)
// 7 functions in link order.
#include <va.h>
#include "csequence.h"

#if 0  // @carcass: no distinct retail body located

// E:\gamedcs\csequence.cpp:32
DC_ONLY(0x71f14, 0xC)
void CSequence::CSequence()
{
    // @stub
}

#endif  // @carcass

VA(0x0047b840, 0x44)  // dc 0x71f20
CSequence::CSequence(const int num)
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

#if 0  // @carcass: overloads inlined or absent from retail

// E:\gamedcs\csequence.cpp:68
DC_ONLY(0x71f78, 0x4E)
int CSequence::addFrame(const char* name)
{
    // @stub
}

// E:\gamedcs\csequence.cpp:79
DC_ONLY(0x71fc8, 0x72)
int CSequence::addFrame(const char* name, int w, int h, unsigned char* data, int csize, TEncodingMethod encoding)
{
    // @stub
}

// E:\gamedcs\csequence.cpp:91
DC_ONLY(0x7203c, 0x8A)
int CSequence::addFrame(const char* name, int w, int h, unsigned char* data, int csize, TEncodingMethod encoding, int m_croppedWidth, int m_croppedHeight, int m_croppedX, int m_croppedY)
{
    // @stub
}

#endif  // @carcass

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
