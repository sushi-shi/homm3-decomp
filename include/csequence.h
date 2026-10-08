#ifndef HOMM3_CSEQUENCE_H
#define HOMM3_CSEQUENCE_H

#include "cspriteframe.h"

class CSprite;

// CSequence of the Loki port (Loki object 46, CSequence.cpp): a frame count,
// the allocated capacity and the frame array, all owned by CSprite.
class CSequence {
private:
    int numFrames;
    int allocatedFrames;
    CSpriteFrame** f;

    friend class CSprite;
    CSequence();
    CSequence(int num);
    ~CSequence();
    int AddFrame(const char* name);
    int AddFrame(const char* name, int w, int h, unsigned char* data,
                 int csize, TEncodingMethod encoding);
    int AddFrame(const char* name, int w, int h, unsigned char* data,
                 int csize, TEncodingMethod encoding,
                 int croppedWidth, int croppedHeight, int croppedX, int croppedY);
    int AddFrame(CSpriteFrame* frame);
};

#endif  /* HOMM3_CSEQUENCE_H */
