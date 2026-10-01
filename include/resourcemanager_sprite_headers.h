// resourcemanager_sprite_headers.h - on-disk DEF records consumed by GetSprite
#ifndef HOMM3_RESOURCEMANAGER_SPRITE_HEADERS_H
#define HOMM3_RESOURCEMANAGER_SPRITE_HEADERS_H

#include "cspriteframe.h"

// Dreamcast function-local records (types 0x28ae and 0x55f6), with every
// x86 field and stride independently visible in GetSprite's retail copies.
struct SpriteDefHeader {
    EResourceType m_type;
    int m_width;
    int m_height;
    int m_numSequences;
    // Native0x28ac is char[768], distinct from the palette's unsigned bytes.
    char m_palette[768];
};
SIZE(SpriteDefHeader, 0x310);

struct TSpriteDataHeader {
    int m_sequenceNumber;
    int m_numFrames;
    // Native0x55f6 fname points through0x55f4 to char[13] (LF_ARRAY0x184e).
    char (*m_frameNames)[13];
    int* m_frameOffsets;
};
SIZE(TSpriteDataHeader, 0x10);

// The two frame-header formats are retail-byte views. The compact form's
// second dword is copied but never consumed; retail instead reads the
// encoding slot of the cropped-header local in both constructor arms.
struct TCompactSpriteFrameHeader {
    int m_dataSize;
    int m_encoding;
    int m_width;
    int m_height;
};
SIZE(TCompactSpriteFrameHeader, 0x10);

struct TCroppedSpriteFrameHeader {
    int m_dataSize;
    TEncodingMethod m_encoding;
    int m_width;
    int m_height;
    int m_croppedWidth;
    int m_croppedHeight;
    int m_croppedX;
    int m_croppedY;
};
SIZE(TCroppedSpriteFrameHeader, 0x20);

#endif  /* HOMM3_RESOURCEMANAGER_SPRITE_HEADERS_H */
