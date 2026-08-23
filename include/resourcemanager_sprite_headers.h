// resourcemanager_sprite_headers.h - on-disk DEF records consumed by GetSprite
#ifndef HOMM3_RESOURCEMANAGER_SPRITE_HEADERS_H
#define HOMM3_RESOURCEMANAGER_SPRITE_HEADERS_H

#include "cspriteframe.h"

// Dreamcast function-local records (types 0x28ae and 0x55f6), with every
// x86 field and stride independently visible in GetSprite's retail copies.
struct TSpriteDefHeader {
    EResourceType type;
    int width;
    int height;
    int numSequences;
    unsigned char palette[768];
};
SIZE(TSpriteDefHeader, 0x310);

struct TSpriteDataHeader {
    int sequenceNumber;
    int numFrames;
    char* frameNames;
    int* frameOffsets;
};
SIZE(TSpriteDataHeader, 0x10);

// The two frame-header formats are retail-byte views. The compact form's
// second dword is copied but never consumed; retail instead reads the
// encoding slot of the cropped-header local in both constructor arms.
struct TCompactSpriteFrameHeader {
    int dataSize;
    int encoding;
    int width;
    int height;
};
SIZE(TCompactSpriteFrameHeader, 0x10);

struct TCroppedSpriteFrameHeader {
    int dataSize;
    TEncodingMethod encoding;
    int width;
    int height;
    int croppedWidth;
    int croppedHeight;
    int croppedX;
    int croppedY;
};
SIZE(TCroppedSpriteFrameHeader, 0x20);

#endif  /* HOMM3_RESOURCEMANAGER_SPRITE_HEADERS_H */
