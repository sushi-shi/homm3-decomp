// csequence.h - prototypes of csequence.cpp (compiland csequence.obj)
#ifndef HOMM3_CSEQUENCE_H
#define HOMM3_CSEQUENCE_H

#include "va.h"

class CSprite;
class CSpriteFrame;

// DC's complete three-member fieldlist transfers without adjustment, and
// retail's int constructor writes all three fields at +0/+4/+8. button::Draw
// independently reads numFrames through CSprite::s[0].
class CSequence {
private:
    int m_numFrames;
    int m_allocatedFrames;
    CSpriteFrame** m_f;

    friend class CSprite;
    int addFrame(CSpriteFrame* frame);
    CSequence(int num);
    ~CSequence();
};
SIZE(CSequence, 0x0c);

// --- CSequence ---
// CODEVIEW(E:\gamedcs\csequence.cpp:32, dc 0x71f14) void CSequence::CSequence();
// CODEVIEW(E:\gamedcs\csequence.cpp:68, dc 0x71f78) int CSequence::AddFrame(const char* name);
// CODEVIEW(E:\gamedcs\csequence.cpp:79, dc 0x71fc8) int CSequence::AddFrame(const char* name, int w, int h, unsigned char* data, int csize, TEncodingMethod encoding);
// CODEVIEW(E:\gamedcs\csequence.cpp:91, dc 0x7203c) int CSequence::AddFrame(const char* name, int w, int h, unsigned char* data, int csize, TEncodingMethod encoding, int CroppedWidth, int CroppedHeight, int CroppedX, int CroppedY);

#endif  /* HOMM3_CSEQUENCE_H */
