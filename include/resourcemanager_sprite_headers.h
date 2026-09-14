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
    unsigned char m_palette[768];
};
SIZE(SpriteDefHeader, 0x310);

// Before normalization (type): TSpriteDataHeader.
struct SpriteDataHeader {
    int m_sequenceNumber;
    int m_numFrames;
    char* m_frameNames;
    int* m_frameOffsets;
};
SIZE(SpriteDataHeader, 0x10);

// The two frame-header formats are retail-byte views. The compact form's
// second dword is copied but never consumed; retail instead reads the
// encoding slot of the cropped-header local in both constructor arms.
// Before normalization (type): TCompactSpriteFrameHeader.
struct CompactSpriteFrameHeader {
    int m_dataSize;
    int m_encoding;
    int m_width;
    int m_height;
};
SIZE(CompactSpriteFrameHeader, 0x10);

// Before normalization (type): TCroppedSpriteFrameHeader.
struct CroppedSpriteFrameHeader {
    int m_dataSize;
    EncodingMethod m_encoding;
    int m_width;
    int m_height;
    int m_croppedWidth;
    int m_croppedHeight;
    int m_croppedX;
    int m_croppedY;
};
SIZE(CroppedSpriteFrameHeader, 0x20);

#endif  /* HOMM3_RESOURCEMANAGER_SPRITE_HEADERS_H */
