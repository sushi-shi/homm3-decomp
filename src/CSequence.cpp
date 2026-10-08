// CSequence.cpp of the Loki port (Loki object 46), in Loki's function order.
#include "csequence.h"

CSequence::CSequence()
    : numFrames(0), allocatedFrames(0), f(0)
{
}

CSequence::CSequence(int num)
{
    numFrames = 0;
    allocatedFrames = num;
    f = new CSpriteFrame*[num];
    for (int i = 0; i < num; i++)
        f[i] = 0;
}

CSequence::~CSequence()
{
    if (f) {
        for (int i = 0; i < allocatedFrames; i++)
            if (f[i])
                delete f[i];
        delete[] f;
    }
}

int CSequence::AddFrame(const char* name)
{
    if (numFrames < allocatedFrames) {
        f[numFrames++] = new CSpriteFrame(name, false);
        return numFrames;
    }
    return 0;
}

int CSequence::AddFrame(const char* name, int w, int h, unsigned char* data,
                        int csize, TEncodingMethod encoding)
{
    if (numFrames < allocatedFrames) {
        f[numFrames++] = new CSpriteFrame(name, w, h, data, csize, encoding);
        return numFrames;
    }
    return 0;
}

int CSequence::AddFrame(const char* name, int w, int h, unsigned char* data,
                        int csize, TEncodingMethod encoding,
                        int croppedWidth, int croppedHeight, int croppedX, int croppedY)
{
    if (numFrames < allocatedFrames) {
        f[numFrames++] = new CSpriteFrame(name, w, h, data, csize, encoding,
                                          croppedWidth, croppedHeight,
                                          croppedX, croppedY);
        return numFrames;
    }
    return 0;
}

int CSequence::AddFrame(CSpriteFrame* frame)
{
    if (numFrames < allocatedFrames) {
        f[numFrames++] = frame;
        return numFrames;
    }
    return 0;
}
