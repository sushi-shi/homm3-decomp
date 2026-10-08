#ifndef HOMM3_BITMAP8_H
#define HOMM3_BITMAP8_H

#include "palette.h"
#include "resource.h"

// Loki h3maped Bitmap8.cpp (object 41): an 8-bit paletted bitmap resource.
// Layout from the Loki bodies: the resource head (vptr at +0x18), DataSize
// +0x1c, ImageSize +0x20, Width +0x24, Height +0x28, the pixel map +0x2c
// and an embedded TPalette24 at +0x30. The pitch is the width. The class
// inline members are emitted after Bitmap8.cpp's own functions, global, as
// the 2.95.2 release does in the unit that defines the key function.
class Bitmap8Bit : public resource {
public:
    Bitmap8Bit();
    Bitmap8Bit(const char* name, int w, int h, unsigned char* data,
               const TPalette24& palette, int size);
    Bitmap8Bit(const char* name, const char* path);
    virtual ~Bitmap8Bit();

    void import(int w, int h, unsigned char* data, const TPalette24& palette,
                int size);
    void clear();
    int exportPCXFile(const char* filename);
    int importPCXFile(const char* filename);

    int GetDataSize() const { return m_dataSize; }
    int GetImageSize() const { return m_imageSize; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetPitch() const { return m_width; }
    unsigned char* GetPalette() { return m_palette.m_palette; }
    unsigned char* GetMap(int x, int y) { return m_map + y * m_width + x; }

private:
    int m_dataSize;
    int m_imageSize;
    int m_width;
    int m_height;
    unsigned char* m_map;
    TPalette24 m_palette;
};

#endif  /* HOMM3_BITMAP8_H */
