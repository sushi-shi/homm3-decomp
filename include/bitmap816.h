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
    // Before normalization (Dreamcast): Palette.
    TPalette16 m_p16;
    // Before normalization (Dreamcast): Palette24.
    TPalette24 m_p24;

public:
    Bitmap816(int w, int h);
    Bitmap816(const char* name, int w, int h, unsigned char* data,
              const TPalette16& palette16, int dataSize);
    Bitmap816(const char* name, int rbits, int rshift,
              int gbits, int gshift, int bbits, int bshift);
    Bitmap816(const char* name, const char* path,
              int rbits, int rshift, int gbits, int gshift,
              int bbits, int bshift);
    virtual ~Bitmap816();
    void import(int w, int h, unsigned char* data, const TPalette16& p16,
                int size);
    void clear();
    int importPCXFile(const char* filename, int rbits, int rshift,
                      int gbits, int gshift, int bbits, int bshift);
    void zBufferDraw(int sx, int sy, int sw, int sh, unsigned short* dst,
                     int dx, int dy, int dw, int dh, int dpitch, int id) const;
    void Draw(int sx, int sy, int sw, int sh, unsigned short* dst, int dx,
              int dy, int dw, int dh, int dpitch, bool tblit) const;
    void Draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx,
              int dy, bool tblit) const;
    // Loki vtable 0x842723c: the destructor, then this overload.
    virtual void zBufferDraw(int sx, int sy, int sw, int sh,
                             unsigned short* zBuffer, int dx, int dy,
                             int id) const;
    void SetPalette(const unsigned short* pal);
    void SetPalette(const TPalette24& pal24);
    void ResetPalette();

    // DC Bitmap816.h:70-104; Loki emits these after bitmap816.cpp's own
    // functions (the unit that defines the key function).
    int GetDataSize() const { return m_dataSize; }
    int GetImageSize() const { return m_imageSize; }
    DC_ADDRESS(0x020164, 0xc)
    int GetWidth() const { return m_width; }
    DC_ADDRESS(0x020170, 0xc)
    int GetHeight() const { return m_height; }
    // DC Bitmap816.h:71 returns Width, while GetMap addresses rows through
    // Pitch.
    DC_ADDRESS(0x05256c, 0x4)
    int GetPitch() const { return m_width; }
    DC_ADDRESS(0x02017c, 0x10)
    TPalette16& GetPalette() { return m_p16; }
    DC_ADDRESS(0x19c5e8, 0x8)
    const TPalette16& GetPalette() const { return m_p16; }
    DC_ADDRESS(0x054300, 0x8)
    TPalette24& GetPalette24() { return m_p24; }
    const TPalette24& GetPalette24() const { return m_p24; }
    DC_ADDRESS(0x052570, 0xe)
    unsigned char* GetMap(int x, int y) { return m_map + y * m_pitch + x; }
    DC_ADDRESS(0x19c5f0, 0xe)
    const unsigned char* GetMap(int x, int y) const
    {
        return m_map + y * m_pitch + x;
    }
};
SIZE(Bitmap816, 0x56c);

#endif  /* HOMM3_BITMAP816_H */
