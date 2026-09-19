// bitmap16.h - bitmap16.cpp (compiland bitmap16.obj)
#ifndef HOMM3_BITMAP16_H
#define HOMM3_BITMAP16_H

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
    // Slot 0 is the scalar deleting destructor: heroWindow deletes its
    // background through [vptr]+flag 1. Slot 2 reports the resource's
    // total in-memory extent: the 0x38-byte object plus DataSize.
    virtual ~Bitmap16Bit();
    void clear();

private:
    int m_dataSize;
    int m_imageSize;
    int m_width;
    int m_height;
    int m_pitch;
    unsigned short* m_map;
    unsigned char m_referenced;

public:
    virtual unsigned int getSize() const;
    Bitmap16Bit(int w, int h);
    Bitmap16Bit(const char* name, int w, int h);
    void reference(int w, int h, int pitch, unsigned short* data);
    void draw(int srcX, int srcY, int srcWidth, int srcHeight, unsigned short* dst, int dstX, int dstY, int dstWidth, int dstHeight, int dstPitch, bool flipped) const;
    void grab(const unsigned short* src, int srcX, int srcY, int srcWidth, int srcHeight, int srcPitch);
    // Retail 0x44e4c0, thiscall (x, y, w, h, color). TWO independent
    // callers pin it: textWidget::Draw's back-colour fill, and
    // heroWindowManager::FadeToBlack (0x6030e0), whose five-argument push
    // run is the DC signature verbatim.
    void fillRect(int x, int y, int w, int h, unsigned short color);
    // Retail bodies 0x44e540 / 0x44e780, both reached from
    // coloredBorderFrame::Draw (0x4501e0): its five-argument push run is
    // the DC signature verbatim, and the truncating `mov dx, [ecx+0x30]`
    // load off an int member is what fixes the 16-bit colour parameter.
    void frameRect(int x, int y, int w, int h, unsigned short color);
    void darken(int x, int y, int w, int h);
    // DC bitmap16.cpp:778; UpdateGrid's seven pushes and retail target
    // 0x44e6a0 independently preserve this masked darken overload.
    void darken(int x, int y, int w, int h, Bitmap816* mask,
                int sx, int sy);
    void colorize(int x, int y, int width, int height, unsigned short color);
    // The float overload, DC bitmap16.cpp:873. advManager::ViewPuzzle is the
    // retail caller that proves the (hue, saturation) pair as raw dwords.
    void colorize(int x, int y, int w, int h, float hue, float saturation);
    // Header accessors (DC Bitmap16.h:111-113, 150/156). They are kept
    // inline because Complete's ResourceManager expands them into its
    // bitmap-remap blit rather than calling the emitted DC copies.
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getPitch() const { return m_pitch; }
    VA(0x004efff0, 0x19)  // COMDAT owner (kb.obj emits ?GetMap@Bitmap16Bit@@QAEPAGHH@Z), body in bitmap16.h
    unsigned short* getMap(int x, int y)
    {
        return static_cast<unsigned short*>(static_cast<void*>(
            static_cast<unsigned char*>(static_cast<void*>(m_map))
            + y * m_pitch)) + x;
    }
    const unsigned short* getMap(int x, int y) const
    {
        return static_cast<const unsigned short*>(static_cast<const void*>(
            static_cast<const unsigned char*>(static_cast<const void*>(m_map))
            + y * m_pitch)) + x;
    }
    VA(0x004f0010, 0x3B)  // COMDAT owner + anchor-callee the 0x44e2b0 raw Draw, body in bitmap16.h
    void draw(int srcX, int srcY, int srcWidth, int srcHeight,
              Bitmap16Bit* dst, int dstX, int dstY, bool flipped) const
    {
        draw(srcX, srcY, srcWidth, srcHeight, dst->getMap(0, 0),
             dstX, dstY, dst->getWidth(), dst->getHeight(), dst->getPitch(),
             flipped);
    }
    // DC Bitmap16.h:168, retained at dc 0x4cb1c. This header forwarding
    // overload calls GetMap/GetWidth/GetHeight/GetPitch and then the raw
    // six-argument Grab. Complete's ShootAnimatedMissile expands it into
    // the retained raw call at 0x0044e3f0.
    void grab(const Bitmap16Bit* src, int srcX, int srcY)
    {
        grab(src->getMap(0, 0), srcX, srcY, src->getWidth(), src->getHeight(),
             src->getPitch());
    }
    void remap(int oldGreenBits);
};
SIZE(Bitmap16Bit, 0x38);

// The live 16-bit pixel format, filled from the DirectDraw surface
// description and read by every bitmap16.obj blender (Darken, Colorize,
// the 24->16 importer) as well as winmgr's two fades.

// GREEN is PROVEN: wingraph 0x601bfe saves 0x694d60 across a mode
// change and feeds `mask == 0x7e0 ? 6 : 5` to Bitmap16Bit::Remap, whose
// parameter the DC roster names old_green_bits. RED/BLUE follow from
// term order, corroborated twice: Bitmap16Bit::Darken (0x44e5f0) and
// both fades emit their three `(px & m) >> n & m` terms in the order
// 0x694d64, 0x694d60, 0x694d68, and the 24->16 importer at 0x44f0f4
// pairs source byte 0 with 0x694d64, byte 1 (green in every 24-bit
// order) with 0x694d60 and byte 2 with 0x694d68 - i.e. R, G, B.
DATA(0x00694d60) extern unsigned long g_colorMaskGreen;
DATA(0x00694d64) extern unsigned long g_colorMaskRed;
DATA(0x00694d68) extern unsigned long g_colorMaskBlue;

#endif  /* HOMM3_BITMAP16_H */
