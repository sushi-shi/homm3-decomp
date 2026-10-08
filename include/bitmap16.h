#ifndef HOMM3_BITMAP16_H
#define HOMM3_BITMAP16_H

#include "va.h"

#include "resource.h"

// Bitmap16Bit::map is DC-proven as unsigned short*, while Pitch is
// independently proven as a byte stride. This union names those two views
// without introducing reinterpret-cast debt or changing the stored pointer.
union Bitmap16MapPointer {
public:
    unsigned short* m_pixels;
    unsigned char* m_bytes;
};

union Bitmap16ConstMapPointer {
public:
    const unsigned short* m_pixels;
    const unsigned char* m_bytes;
};

// Bootstrap VIEW of Bitmap16Bit (resource lineage). Layout PROVEN at
// retail size 0x38: heroWindow::SaveBackground news it with
// `push 0x38`, and the DC roster (DataSize@0x1c, ImageSize@0x20,
// Width@0x24, Height@0x28, Pitch@0x2c, map@0x30, referenced@0x34)
// lands on the exact offsets heroWindow/CenterWindow read - retail
// dropped DC's per-bitmap DDSURFACEDESC/surface tail. pad_04 spans the
// unmodeled resource base fields. Method signatures are DC-attested
// (?Grab@Bitmap16Bit@@QAAXPBGHHHHH@Z, ?Draw@...PAGHHHHH_N@Z const);
// Darken's retail body is 0x44e5f0 (called by widget::Dim).
class Bitmap816;

// The two green-channel widths a 16-bit surface comes in. wingraph's
// mode-change path (0x601bfe) picks between them with
// `greenMask == 0x7e0 ? 6 : 5` and hands the answer to Remap, which is the
// only consumer; the DC roster names that parameter old_green_bits.
enum EBitmapGreenBits {
    BITMAP_GREEN_BITS_1555 = 5,
    BITMAP_GREEN_BITS_565 = 6
};

// Loki's display is 5:6:5 while the game's palettes are 5:5:5; Bitmap816 and
// the sprite drawers convert every palette entry they write (linkonce in
// Loki, owned by Bitmap816.cpp as the first user).
inline unsigned short convert555to565(unsigned short color)
{
    return ((color & 0x7fe0) << 1) | (color & 0x1f);
}

class Bitmap16Bit : public resource {
public:
    // Loki exports these as Bitmap16Bit::red_mask/green_mask/blue_mask.
    static unsigned int red_mask;
    static unsigned int green_mask;
    static unsigned int blue_mask;

    Bitmap16Bit(int w, int h);
    Bitmap16Bit(const char* name, int w, int h);
    Bitmap16Bit(const char* name, int w, int h,
                const unsigned short* data, int size);
    Bitmap16Bit(const char* name, const char* path);
    // Loki's vtable (0x8427250) holds only the destructor.
    virtual ~Bitmap16Bit();

    void Remap(int old_green_bits);
    void import(int w, int h, const unsigned short* data, int size);
    void reference(int w, int h, int pitch, unsigned short* data);
    void clear();
    int importPCXFile(const char* filename);
    void Draw(int srcX, int srcY, int srcWidth, int srcHeight,
              unsigned short* dst, int dstX, int dstY, int dstWidth,
              int dstHeight, int dstPitch, bool flipped) const;
    void Grab(const unsigned short* src, int srcX, int srcY, int srcWidth,
              int srcHeight, int srcPitch);
    void FillRect(int x, int y, int w, int h, unsigned short color);
    void FrameRect(int x, int y, int w, int h, unsigned short color);
    void Darken(int x, int y, int w, int h);
    void Darken(int x, int y, int w, int h, Bitmap816* mask, int sx, int sy);
    void Colorize(int x, int y, int w, int h, unsigned short color);
    void Colorize(int x, int y, int w, int h, float hue, float saturation);
    void Gray(int x, int y, int w, int h);
    void GrabAndBlur(const Bitmap16Bit* src, int sx, int sy);

    // DC Bitmap16.h:111-168; Loki emits these after Bitmap16.cpp's own
    // functions (the unit that defines the key function).
    int GetDataSize() const { return m_dataSize; }
    int GetImageSize() const { return m_imageSize; }
    DC_ADDRESS(0x01f100, 0xc)
    int GetWidth() const { return m_width; }
    DC_ADDRESS(0x01f10c, 0xc)
    int GetHeight() const { return m_height; }
    DC_ADDRESS(0x01f118, 0xc)
    int GetPitch() const { return m_pitch; }
    DC_ADDRESS(0x122b8c, 0x1c)
    static void SetPixelFormat(unsigned int red, unsigned int green,
                               unsigned int blue)
    {
        red_mask = red;
        green_mask = green;
        blue_mask = blue;
    }
    DC_ADDRESS(0x01f124, 0x22)
    unsigned short* GetMap(int x, int y)
    {
        return (unsigned short*)((unsigned char*)m_map + y * m_pitch
                                 + x * sizeof(unsigned short));
    }
    DC_ADDRESS(0x04ca7c, 0x10)
    const unsigned short* GetMap(int x, int y) const
    {
        return (const unsigned short*)((const unsigned char*)m_map
                                       + y * m_pitch
                                       + x * sizeof(unsigned short));
    }
    DC_ADDRESS(0x04ca8c, 0x90)
    void Draw(int srcX, int srcY, int w, int h, Bitmap16Bit* dst, int dstX,
              int dstY, bool flipped) const
    {
        Draw(srcX, srcY, w, h, dst->GetMap(0, 0), dstX, dstY, dst->GetWidth(),
             dst->GetHeight(), dst->GetPitch(), flipped);
    }
    DC_ADDRESS(0x04cb1c, 0x74)
    void Grab(const Bitmap16Bit* src, int srcX, int srcY)
    {
        Grab(src->GetMap(0, 0), srcX, srcY, src->GetWidth(),
             src->GetHeight(), src->GetPitch());
    }
    void Colorize(float hue, float saturation)
    {
        Colorize(0, 0, m_width, m_height, hue, saturation);
    }
    void Gray() { Gray(0, 0, m_width, m_height); }

private:
    int m_dataSize;
    int m_imageSize;
    int m_width;
    int m_height;
    int m_pitch;
    unsigned short* m_map;
    unsigned char m_referenced;
};
SIZE(Bitmap16Bit, 0x38);

#endif  /* HOMM3_BITMAP16_H */
