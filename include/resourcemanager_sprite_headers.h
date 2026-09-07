// resourcemanager_sprite_headers.h - on-disk DEF records consumed by GetSprite
#ifndef HOMM3_RESOURCEMANAGER_SPRITE_HEADERS_H
#define HOMM3_RESOURCEMANAGER_SPRITE_HEADERS_H

#include "cspriteframe.h"

// Dreamcast function-local records (types 0x28ae and 0x55f6), with every
// x86 field and stride independently visible in GetSprite's retail copies.
struct TSpriteDefHeader {
    // Before normalization: type.
    EResourceType m_type;
    // Before normalization: width.
    int m_width;
    // Before normalization: height.
    int m_height;
    // Before normalization: numSequences.
    int m_numSequences;
    // Before normalization: palette.
    unsigned char m_palette[768];
};
SIZE(TSpriteDefHeader, 0x310);

struct TSpriteDataHeader {
    // Before normalization: sequenceNumber.
    int m_sequenceNumber;
    // Before normalization: numFrames.
    int m_numFrames;
    // Before normalization: frameNames.
    char* m_frameNames;
    // Before normalization: frameOffsets.
    int* m_frameOffsets;
};
SIZE(TSpriteDataHeader, 0x10);

// The two frame-header formats are retail-byte views. The compact form's
// second dword is copied but never consumed; retail instead reads the
// encoding slot of the cropped-header local in both constructor arms.
struct TCompactSpriteFrameHeader {
    // Before normalization: dataSize.
    int m_dataSize;
    // Before normalization: encoding.
    int m_encoding;
    // Before normalization: width.
    int m_width;
    // Before normalization: height.
    int m_height;
};
SIZE(TCompactSpriteFrameHeader, 0x10);

struct TCroppedSpriteFrameHeader {
    // Before normalization: dataSize.
    int m_dataSize;
    // Before normalization: encoding.
    TEncodingMethod m_encoding;
    // Before normalization: width.
    int m_width;
    // Before normalization: height.
    int m_height;
    // Before normalization: croppedWidth.
    int m_croppedWidth;
    // Before normalization: croppedHeight.
    int m_croppedHeight;
    // Before normalization: croppedX.
    int m_croppedX;
    // Before normalization: croppedY.
    int m_croppedY;
};
SIZE(TCroppedSpriteFrameHeader, 0x20);

#endif  /* HOMM3_RESOURCEMANAGER_SPRITE_HEADERS_H */
