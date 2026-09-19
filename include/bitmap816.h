#ifndef HOMM3_BITMAP816_H
#define HOMM3_BITMAP816_H

#include "resource.h"
#include "palette.h"

class Bitmap16Bit;

// Partial model: the resource base is byte-proven by
// bitmapBorder::SetImage (name strcmp at +4, Dispose vcall); the
// embedded palette pair at +0x50/+0x250 by SetPlayerPaletteColors.
class Bitmap816 : public resource {
private:
    // DC names both dwords; retail vtable slot 2 reads DataSize directly
    // and adds the fixed 0x56c-byte object extent.
    int m_dataSize;  // +0x1c
    int m_imageSize;  // +0x20
    // Byte-proven 2026-08-08 by bitmapBorder::GetRealWidth /
    // GetRealHeight (0x4504a0 / 0x4504b0), which read +0x24 and +0x28
    // off this object; the names are the DC fieldlist's (Width@36,
    // Height@40 - the same offsets, since resource is 0x1c on both
    // builds). The rest of the head stays padded until a retail body
    // reads it.
    int m_width;  // +0x24
    int m_height;  // +0x28
    int m_pitch;  // +0x2c
    unsigned char* m_map;  // +0x30

public:
    TPalette16 m_p16;
    TPalette24 m_p24;
    Bitmap816(const char* name, int w, int h, unsigned char* data,
              TPalette16* palette16, int dataSize);
    Bitmap816(const char* name, const char* path,
              int rbits, int rshift, int gbits, int gshift,
              int bbits, int bshift);
    virtual ~Bitmap816();
    void zBufferDraw(int sx, int sy, int sw, int sh, unsigned short* dst,
        int dx, int dy, int dw, int dh, int dpitch, int id) const;
    void draw(int sx, int sy, int sw, int sh, unsigned short* dst, int dx,
        int dy, int dw, int dh, int dpitch, bool tblit) const;
    void draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx,
        int dy, bool tblit) const;
    void markPuzzle(unsigned char* visible, long destX, long destY);
    void setPalette(const unsigned short* pal);
    void setPalette(TPalette24* pal24);
    void resetPalette();
    // Bitmap816.h:70/71 header accessors. DrawBackground's Dreamcast xref
    // graph records both inlined uses; the retail body reads +0x24/+0x28.
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    // DC Bitmap816.h:71 (0x5256c) returns Width, while GetMap below
    // addresses rows through Pitch. Masked Darken's retail loads independently
    // confirm that distinction; do not replace this helper with m_pitch.
    int getPitch() const { return m_width; }
    // DC Bitmap816.h:98/99 (0x52570), expanded in masked Darken.
    unsigned char* getMap(int x, int y) { return m_map + m_pitch * y + x; }

private:
    int importPCXFile(const char* filename, int rbits, int rshift,
        int gbits, int gshift, int bbits, int bshift);

public:
    virtual unsigned int getSize() const;
    virtual void zBufferDraw(int sx, int sy, int sw, int sh,
        unsigned short* zBuffer, int dx, int dy, int id) const;
};
SIZE(Bitmap816, 0x56c);

#endif  /* HOMM3_BITMAP816_H */
