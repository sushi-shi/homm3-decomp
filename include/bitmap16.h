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

class Bitmap16Bit : public resource {
public:
    // Original class statics red_mask/green_mask/blue_mask. Complete's
    // ResourceManager::setPixelFormat stores its arguments at 0x694d68,
    // 0x694d60 and 0x694d64 respectively. PCX bytes are BGR, so its first
    // byte uses s_blueMask; the earlier global names reversed red and blue.
    static unsigned int red_mask;
    static unsigned int green_mask;
    static unsigned int blue_mask;

    // Slot 0 is the scalar deleting destructor: heroWindow deletes its
    // background through [vptr]+flag 1. Slot 2 reports the resource's
    // total in-memory extent: the 0x38-byte object plus DataSize.
    virtual ~Bitmap16Bit();
    void clear();

private:
    int DataSize;
    int ImageSize;
    int Width;
    int Height;
    int Pitch;
    unsigned short* map;
    unsigned char referenced;

public:
    virtual unsigned int getSize() const;
    Bitmap16Bit(int w, int h);
    Bitmap16Bit(const char* name, int w, int h);
    Bitmap16Bit(const char* name, int w, int h,
                const unsigned short* data, int size);
    Bitmap16Bit(const char* name, const char* path);
    void import(int w, int h, const unsigned short* data, int size);
    void reference(int w, int h, int pitch, unsigned short* data);
    void Draw(int srcX, int srcY, int srcWidth, int srcHeight, unsigned short* dst, int dstX, int dstY, int dstWidth, int dstHeight, int dstPitch, bool flipped) const;
    void Grab(const unsigned short* src, int srcX, int srcY, int srcWidth, int srcHeight, int srcPitch);
    // Retail 0x44e4c0, thiscall (x, y, w, h, color). TWO independent
    // callers pin it: textWidget::Draw's back-colour fill, and
    // heroWindowManager::FadeToBlack (0x6030e0), whose five-argument push
    // run is the DC signature verbatim.
    void FillRect(int x, int y, int w, int h, unsigned short color);
    // Retail bodies 0x44e540 / 0x44e780, both reached from
    // coloredBorderFrame::Draw (0x4501e0): its five-argument push run is
    // the DC signature verbatim, and the truncating `mov dx, [ecx+0x30]`
    // load off an int member is what fixes the 16-bit colour parameter.
    void FrameRect(int x, int y, int w, int h, unsigned short color);
    void Darken(int x, int y, int w, int h);
    // DC bitmap16.cpp:778; UpdateGrid's seven pushes and retail target
    // 0x44e6a0 independently preserve this masked darken overload.
    void Darken(int x, int y, int w, int h, Bitmap816* mask,
                int sx, int sy);
    void Colorize(int x, int y, int width, int height, unsigned short color);
    // The float overload, DC bitmap16.cpp:873. advManager::ViewPuzzle is the
    // retail caller that proves the (hue, saturation) pair as raw dwords.
    void Colorize(int x, int y, int w, int h, float hue, float saturation);
    void Gray(int x, int y, int w, int h);
    void GrabAndBlur(const Bitmap16Bit* src, int sx, int sy);

    // Header accessors (DC Bitmap16.h:111-113, 150/156). They are kept
    // inline because Complete's ResourceManager expands them into its
    // bitmap-remap blit rather than calling the emitted DC copies.
    DC_ADDRESS(0x01f100, 0xc)
    int GetWidth() const { return Width; }

    DC_ADDRESS(0x01f10c, 0xc)
    int GetHeight() const { return Height; }

    DC_ADDRESS(0x01f118, 0xc)
    int GetPitch() const { return Pitch; }

    // Original: Bitmap16Bit::SetPixelFormat; Bitmap16.h:142
    DC_ADDRESS(0x122b8c, 0x1c)
    static void SetPixelFormat(unsigned int red, unsigned int green, unsigned int blue)
    {
        red_mask = red;
        green_mask = green;
        blue_mask = blue;
    }

    VA(0x004efff0, 0x19)  // COMDAT owner (kb.obj emits ?GetMap@Bitmap16Bit@@QAEPAGHH@Z), body in bitmap16.h
    DC_ADDRESS(0x01f124, 0x22)
    unsigned short* GetMap(int x, int y)
    {
        // Bitmap16.h:151 (dc 0x1f124): add the byte row stride, then
        // the pixel offset. Retail's retained body has the same order.
        return reinterpret_cast<unsigned short*>(
            reinterpret_cast<unsigned char*>(map) + y * Pitch
            + x * sizeof(*map));
    }

    // DC Bitmap16.h:156-157 has the same one-expression pixel address:
    // y advances bytes by pitch; x advances unsigned-short pixels.
    DC_ADDRESS(0x04ca7c, 0x10)
    const unsigned short* GetMap(int x, int y) const
    {
        return reinterpret_cast<const unsigned short*>(
            reinterpret_cast<const unsigned char*>(map) + y * Pitch
            + x * sizeof(*map));
    }

    VA(0x004f0010, 0x3B)  // COMDAT owner + anchor-callee the 0x44e2b0 raw Draw, body in bitmap16.h
    DC_ADDRESS(0x04ca8c, 0x90)
    void Draw(int srcX, int srcY, int srcWidth, int srcHeight,
              Bitmap16Bit* dst, int dstX, int dstY, bool flipped) const
    {
        Draw(srcX, srcY, srcWidth, srcHeight, dst->GetMap(0, 0),
             dstX, dstY, dst->GetWidth(), dst->GetHeight(), dst->GetPitch(),
             flipped);
    }

    // DC Bitmap16.h:168 retains this inline body. This header forwarding
    // overload calls GetMap/GetWidth/GetHeight/GetPitch and then the raw
    // six-argument Grab. Complete's ShootAnimatedMissile expands it into
    // the retained raw call at 0x0044e3f0.
    DC_ADDRESS(0x04cb1c, 0x74)
    void Grab(const Bitmap16Bit* src, int srcX, int srcY)
    {
        Grab(src->GetMap(0, 0), srcX, srcY, src->GetWidth(), src->GetHeight(),
             src->GetPitch());
    }
    void Remap(int oldGreenBits);

private:
    int importPCXFile(const char* filename);
};
SIZE(Bitmap16Bit, 0x38);

#endif  /* HOMM3_BITMAP16_H */
