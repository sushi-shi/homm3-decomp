#ifndef HOMM3_CSEQUENCE_H
#define HOMM3_CSEQUENCE_H

#include "va.h"

#include "cspriteframe.h"

class CSprite;
class CSpriteFrame;

// DC's complete three-member fieldlist transfers without adjustment, and
// retail's int constructor writes all three fields at +0/+4/+8. button::Draw
// independently reads numFrames through CSprite::s[0].
class CSequence {
private:
    int numFrames;
    int allocatedFrames;
    CSpriteFrame** f;

    friend class CSprite;
    int AddFrame(const char* name);
    int AddFrame(const char* name, int w, int h, unsigned char* data,
                 int csize, TEncodingMethod encoding);
    int AddFrame(const char* name, int w, int h, unsigned char* data,
                 int csize, TEncodingMethod encoding,
                 int croppedWidth, int croppedHeight, int croppedX, int croppedY);
    int AddFrame(CSpriteFrame* frame);
    CSequence();
    CSequence(int num);
    ~CSequence();
};
SIZE(CSequence, 0x0c);

#endif  /* HOMM3_CSEQUENCE_H */
