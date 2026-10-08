#ifndef HOMM3_BITMAP816_H
#define HOMM3_BITMAP816_H

#include "va.h"

#include "palette.h"
#include "resource.h"

class Bitmap16Bit;

// Partial model: the resource base is byte-proven by
// bitmapBorder::SetImage (name strcmp at +4, Dispose vcall); the
// embedded palette pair at +0x50/+0x250 by SetPlayerPaletteColors.
class Bitmap816 : public resource {
private:
    // DC names both dwords; retail vtable slot 2 reads DataSize directly
    // and adds the fixed 0x56c-byte object extent.
    int DataSize;  // +0x1c
    int ImageSize;  // +0x20
    // Byte-proven 2026-08-08 by bitmapBorder::GetRealWidth /
    // GetRealHeight (0x4504a0 / 0x4504b0), which read +0x24 and +0x28
    // off this object; the names are the DC fieldlist's (Width@36,
    // Height@40 - the same offsets, since resource is 0x1c on both
    // builds). The rest of the head stays padded until a retail body
    // reads it.
    int Width;  // +0x24
    int Height;  // +0x28
    int Pitch;  // +0x2c
    unsigned char* map;  // +0x30

public:
    // Before normalization (Dreamcast): Palette.
    TPalette16 m_p16;
    // Before normalization (Dreamcast): Palette24.
    TPalette24 m_p24;

public:
    Bitmap816(int w, int h);
    Bitmap816(const char* name, int w, int h, unsigned char* data,
              TPalette16* palette16, int dataSize);
    Bitmap816(const char* name, int rbits, int rshift,
              int gbits, int gshift, int bbits, int bshift);
    Bitmap816(const char* name, const char* path,
              int rbits, int rshift, int gbits, int gshift,
              int bbits, int bshift);
    virtual ~Bitmap816();
    void import(int w, int h, unsigned char* data, TPalette16& p16, int size);
    void clear();
    void zBufferDraw(int sx, int sy, int sw, int sh, unsigned short* dst,
        int dx, int dy, int dw, int dh, int dpitch, int id) const;
    void Draw(int sx, int sy, int sw, int sh, unsigned short* dst, int dx,
        int dy, int dw, int dh, int dpitch, bool tblit) const;
    void Draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx,
        int dy, bool tblit) const;
    void mark_puzzle(unsigned char* visible, long destX, long destY);
    void SetPalette(const unsigned short* pal);
    void SetPalette(TPalette24* pal24);
    void ResetPalette();

    // Bitmap816.h:70/71 header accessors. DrawBackground's Dreamcast xref
    // graph records both inlined uses; the retail body reads +0x24/+0x28.
    DC_ADDRESS(0x020164, 0xc)
    int GetWidth() const { return Width; }

    DC_ADDRESS(0x020170, 0xc)
    int GetHeight() const { return Height; }

    // DC Bitmap816.h:71 returns Width, while GetMap below
    // addresses rows through Pitch. Masked Darken's retail loads independently
    // confirm that distinction; do not replace this helper with m_pitch.
    DC_ADDRESS(0x05256c, 0x4)
    int GetPitch() const { return Width; }

    // Original: Bitmap816::GetPalette; Bitmap816.h:72
    DC_ADDRESS(0x02017c, 0x10)
    TPalette16& GetPalette() { return m_p16; }

    // Original: Bitmap816::GetPalette; Bitmap816.h:73
    DC_ADDRESS(0x19c5e8, 0x8)
    const TPalette16& GetPalette() const { return m_p16; }

    // Original: Bitmap816::GetPalette24; Bitmap816.h:74
    // Complete embeds both palettes; DC stored pointers to the same types.
    DC_ADDRESS(0x054300, 0x8)
    TPalette24& GetPalette24() { return m_p24; }

    // DC Bitmap816.h:98/99, expanded in masked Darken.
    DC_ADDRESS(0x052570, 0xe)
    unsigned char* GetMap(int x, int y) { return map + Pitch * y + x; }

    // Original: Bitmap816::GetMap; Bitmap816.h:104
    DC_ADDRESS(0x19c5f0, 0xe)
    const unsigned char* GetMap(int x, int y) const
    {
        return map + Pitch * y + x;
    }

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
