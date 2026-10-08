#include "va.h"

#include <limits>
#include <math.h>
#include <new>
#include <string.h>

#include "bitmap16.h"

#include "bitmap816.h"
#include "hsv.h"
#include "pcx.h"

// Convert using the low word of the biased double representation.
// Original: ftol; bitmap16.cpp:59
DC_ADDRESS(0x050a9c, 0x62)
static long ftol(double d)
{
    const unsigned long magic = 0x59c00000;
    d += *static_cast<const float*>(static_cast<const void*>(&magic));
    return *static_cast<long*>(static_cast<void*>(&d));
}

VA_COMPGEN(0x0044e020, 0x21, SCALAR_DELETING_DTOR, Bitmap16Bit)

// DC allocation uses BMCreateSurface then virtual Lock (slot +0x64),
// falling back to the same heap allocation below. Mac 0x5be20/0x5bee4
// and retail 0x44df70/0x44e050 retain only that heap path. Their class
// has no surface descriptor/owner; these calls are not expanded helpers.
VA(0x0044df70, 0xA3)
DC_ADDRESS(0x050b00, 0x104)
MAC_ADDRESS(0x05be20, 0xc4)
Bitmap16Bit::Bitmap16Bit(int w, int h)
    : resource(0, RESOURCE_TYPE_NONE),
      ImageSize(w * h * 2), Width(w), Height(h), Pitch(w * 2)
{
    DataSize = ImageSize;

    if (w && h) {
        map = new unsigned short[DataSize / 2];
        referenced = 0;
    } else {
        map = 0;
    }
}

VA(0x0044e050, 0xA5)
DC_ADDRESS(0x050c04, 0x104)
MAC_ADDRESS(0x05bee4, 0xc0)  // in-span, name/type base ctor + vftable 0x63b9c8
Bitmap16Bit::Bitmap16Bit(const char* name, int w, int h)
    : resource(name, RESOURCE_TYPE_BITMAP16),
      ImageSize(w * h * 2), Width(w), Height(h), Pitch(w * 2),
      referenced(0)
{
    DataSize = ImageSize;

    if (w > 0 && h > 0)
        map = new unsigned short[DataSize / 2];
    else
        map = 0;
}

// Original: Bitmap16Bit::Bitmap16Bit; bitmap16.cpp:152
// Complete's 0x38-byte bitmap keeps the heap-buffer arm of DC's allocation
// path; DDSURFACEDESC, BMCreateSurface and the locked-surface owner are absent.
DC_ADDRESS(0x050d08, 0xfa)
Bitmap16Bit::Bitmap16Bit(const char* name, int w, int h,
                         const unsigned short* data, int size)
    : resource(name, RESOURCE_TYPE_BITMAP16),
      ImageSize(w * h * 2), Width(w), Height(h), Pitch(w * 2)
{
    DataSize = size ? size : ImageSize;
    map = new unsigned short[DataSize / 2];
    referenced = 0;
    if (map)
        memcpy(map, data, DataSize);
}

// Original: Bitmap16Bit::Bitmap16Bit; bitmap16.cpp:187
DC_ADDRESS(0x050e04, 0xb8)
Bitmap16Bit::Bitmap16Bit(const char* name, const char* path)
    : resource(name, RESOURCE_TYPE_BITMAP16),
      DataSize(0), ImageSize(0), Width(0), Height(0), Pitch(0), map(0)
{
    char fileName[261];
    strcpy(fileName, path);
    strcat(fileName, name);
    importPCXFile(fileName);
    referenced = 0;
}

VA(0x0044e100, 0x29)
DC_ADDRESS(0x050ebc, 0x76)
MAC_ADDRESS(0x05bfa4, 0x7c)
Bitmap16Bit::~Bitmap16Bit()
{
    if (map && !referenced)
        delete[] map;
}

// E:\gamedcs\bitmap16.cpp:224/234/243/253. The four pixel-format converters
// the compiland's own Remap runs the surface through. NONE of them has a
// retail row: /Ob2 expands all four at Remap's two arms, which is the only
// place in the image that reaches them, so the shift/mask chains below are
// read straight out of that body.
DC_ADDRESS(0x050f34, 0x2c)
unsigned long color1555to8888(unsigned short color)
{
    return ((((((color & 0xffff8000) << 7 | (color & 0x7c00)) << 3)
              | (color & 0x3e0))
             << 3
             | (color & 0x1f))
            << 3);
}

DC_ADDRESS(0x050f60, 0x20)
unsigned long color0565to8888(unsigned short color)
{
    return ((((color & 0xf800) << 3 | (color & 0x7e0)) << 2 | (color & 0x1f))
            << 3);
}

DC_ADDRESS(0x050f80, 0x2e)
unsigned short color8888to1555(unsigned long color)
{
    return static_cast<unsigned short>(((color >> 9) & 0x7c00)
                                       | ((color >> 6) & 0x3e0)
                                       | ((color >> 3) & 0x1f)
                                       | ((color >> 16) & 0x8000));
}

DC_ADDRESS(0x050fb0, 0x22)
unsigned short color8888to0565(unsigned long color)
{
    return static_cast<unsigned short>(((color >> 8) & 0xf800)
                                       | ((color >> 5) & 0x7e0)
                                       | ((color >> 3) & 0x1f));
}

VA(0x0044e130, 0x110)
DC_ADDRESS(0x050fd4, 0xa4)
void Bitmap16Bit::Remap(int oldGreenBits)
{
    for (int col = 0; col < Width; col++) {
        for (int row = 0; row < Height; row++) {
            Bitmap16MapPointer pixel;
            pixel.m_pixels = GetMap(col, row);
            if (oldGreenBits == BITMAP_GREEN_BITS_565)
                *pixel.m_pixels = color8888to1555(color0565to8888(*pixel.m_pixels));
            else
                *pixel.m_pixels = color8888to0565(color1555to8888(*pixel.m_pixels));
        }
    }
}

VA(0x0044e240, 0x07)
MAC_ADDRESS(0x05c020, 0xc)  // vtable slot 2: fixed object extent + pixel bytes
unsigned int Bitmap16Bit::getSize() const
{
    return sizeof(*this) + DataSize;
}

// Original: Bitmap16Bit::import; bitmap16.cpp:294
DC_ADDRESS(0x051078, 0xda)
void Bitmap16Bit::import(int w, int h, const unsigned short* data, int size)
{
    clear();
    Width = w;
    Height = h;
    Pitch = w * 2;
    ImageSize = w * h * 2;
    DataSize = size ? size : ImageSize;
    map = new unsigned short[DataSize / 2];
    referenced = 0;
    if (map)
        memcpy(map, data, DataSize);
}

VA(0x0044e250, 0x5D)
DC_ADDRESS(0x051154, 0x44)
MAC_ADDRESS(0x05c02c, 0x68)
void Bitmap16Bit::reference(int w, int h, int pitch, unsigned short* data)
{
    clear();
    Width = w;
    Height = h;
    Pitch = pitch;
    ImageSize = w * h * 2;
    DataSize = ImageSize;
    map = data;
    referenced = 1;
}

// DC bitmap16.cpp:358 supplies the ordinary clear helper called by reference.
// Complete inlines its scalar resets and owned-buffer release; the DC-only
// surface-release arm has no corresponding field or operation in retail.
// DC 0x511d6/0x511e0 unlocks/releases the surface instead of deleting a
// heap buffer. Mac 0x5c094 and retail reference 0x44e250 retain only the
// heap/borrowed-buffer branch, including its null-pointer guard.
DC_ADDRESS(0x051198, 0x90)
MAC_ADDRESS(0x05c094, 0x64)
void Bitmap16Bit::clear()
{
    Width = 0;
    Height = 0;
    DataSize = 0;
    ImageSize = 0;
    if (map) {
        if (!referenced)
            delete[] map;
        map = 0;
        referenced = 0;
    }
}

// Original: Bitmap16Bit::importPCXFile; bitmap16.cpp:472
DC_ADDRESS(0x051228, 0x150)
int Bitmap16Bit::importPCXFile(const char* filename)
{
    PcxData pdat;
    imgdes pcxfile;
    if (pcxinfo(filename, &pdat))
        return 1;

    Width = pdat.m_width;
    Height = pdat.m_length;
    Pitch = pdat.m_width * 2;
    ImageSize = Width * Height * 2;
    DataSize = ImageSize;
    map = new unsigned short[DataSize / 2];
    referenced = 0;
    if (!map)
        return 2;

    allocimage(&pcxfile, pdat.m_width, pdat.m_length,
               pdat.m_bpPixel * pdat.m_nplanes);
    loadpcx(filename, &pcxfile);
    flipimage(&pcxfile, &pcxfile);
    for (int y = 0; y < Height; ++y) {
        // DC 0x51310..0x5132a advances this destination by Width bytes,
        // although the copied row contains Width words. Preserve that old
        // importer's addressing; neither retained PCX importer calls it.
        Bitmap16MapPointer row;
        row.m_pixels = map;
        memcpy(row.m_bytes + y * Width,
               pcxfile.m_ibuff + y * pcxfile.m_buffwidth,
               Width * sizeof(unsigned short));
    }
    freeimage(&pcxfile);
    return 0;
}

// E:\gamedcs\bitmap16.cpp:541
#define BITMAP16_BYTE_OFFSET(pointer, offset)                              \
    static_cast<unsigned short*>(static_cast<void*>(                      \
        static_cast<unsigned char*>(static_cast<void*>(pointer)) + offset))
#define BITMAP16_CONST_BYTE_OFFSET(pointer, offset)                        \
    static_cast<const unsigned short*>(static_cast<const void*>(           \
        static_cast<const unsigned char*>(static_cast<const void*>(pointer)) \
        + offset))

// The Mac draw/grab/rectangle family expands the canonical Bitmap16.h
// dimension and byte-pitch reads (DC header lines 111-113). Examples:
// draw 0x5c1e8/0x5c224, grab 0x5c274/0x5c278/0x5c31c,
// fillRect 0x5c348/0x5c35c/0x5c41c, frameRect 0x5c438/0x5c44c/0x5c534,
// darken 0x5c55c/0x5c570/0x5c6d4, masked darken 0x5c6fc/0x5c710/0x5c7e8.
// Keep these operations as calls to the existing getters. The expanded
// one-word reads identify the operation; original source-call spelling is
// inferred. Map lookup is already represented by getMap in each caller.
VA(0x0044e2b0, 0x139)
DC_ADDRESS(0x051378, 0xf0)
MAC_ADDRESS(0x05c0f8, 0x158)  // MAC_ABSTRACTION_FROM(tokens1:172e459daefb,41.2791): canonical getPitch calls replace the two direct row-stride loads; Windows remains exact.
void Bitmap16Bit::Draw(int srcX, int srcY, int srcWidth, int srcHeight,
                       unsigned short* dst, int dstX, int dstY, int dstWidth,
                       int dstHeight, int dstPitch, bool flipped) const
{
    if (dstX < 0) {
        srcX -= dstX;
        srcWidth += dstX;
        dstX = 0;
    }
    if (dstY < 0) {
        srcY -= dstY;
        srcHeight += dstY;
        dstY = 0;
    }
    if (srcWidth + dstX > dstWidth)
        srcWidth = dstWidth - dstX;
    if (srcHeight + dstY > dstHeight)
        srcHeight = dstHeight - dstY;

    if (srcWidth > 0 && srcHeight > 0) {
        const unsigned short* src = GetMap(srcX, srcY);
        dst = BITMAP16_BYTE_OFFSET(
            dst, dstY * dstPitch + dstX * sizeof(unsigned short));

        if (flipped) {
            for (int row = 0; row < srcHeight; ++row) {
                const unsigned short* in = src;
                unsigned short* out = dst;
                for (int col = 0; col < srcWidth; ++col) {
                    if (*in != static_cast<unsigned short>(flipped))
                        *out = *in;
                    ++in;
                    ++out;
                }
                src = BITMAP16_CONST_BYTE_OFFSET(src, GetPitch());
                dst = BITMAP16_BYTE_OFFSET(dst, dstPitch);
            }
        } else {
            for (int row = 0; row < srcHeight; ++row) {
                memcpy(dst, src, srcWidth * sizeof(unsigned short));
                src = BITMAP16_CONST_BYTE_OFFSET(src, GetPitch());
                dst = BITMAP16_BYTE_OFFSET(dst, dstPitch);
            }
        }
    }
}

#undef BITMAP16_CONST_BYTE_OFFSET
#undef BITMAP16_BYTE_OFFSET

// E:\gamedcs\bitmap16.cpp:625
VA(0x0044e3f0, 0xC9)
DC_ADDRESS(0x051468, 0xa4)
MAC_ADDRESS(0x05c250, 0xf8)  // order-map(DC bitmap16.obj, between Draw and FillRect)
void Bitmap16Bit::Grab(const unsigned short* src, int srcX, int srcY,
                       int srcWidth, int srcHeight, int srcPitch)
{
    int dstX = 0;
    int dstY = 0;
    int w = GetWidth();
    int h = GetHeight();

    if (srcX < 0) {
        dstX -= srcX;
        w += srcX;
        srcX = 0;
    }
    if (srcY < 0) {
        dstY -= srcY;
        h += srcY;
        srcY = 0;
    }
    if (w > srcWidth - srcX)
        w = srcWidth - srcX;
    if (h > srcHeight - srcY)
        h = srcHeight - srcY;

    if (w <= 0 || h <= 0)
        return;

    Bitmap16MapPointer dst;
    dst.m_pixels = GetMap(dstX, dstY);
    Bitmap16ConstMapPointer source;
    source.m_pixels = src;
    source.m_bytes += srcY * srcPitch + srcX * sizeof(unsigned short);
    for (int row = 0; row < h; ++row) {
        memcpy(dst.m_pixels, source.m_pixels, w * sizeof(unsigned short));
        dst.m_bytes += GetPitch();
        source.m_bytes += srcPitch;
    }
}

// E:\gamedcs\bitmap16.cpp:679. FrameRect's sibling, and the same clipped
// rectangle walk with the interior filled instead of outlined: VC6 turns
// the inner store loop into its word-fill idiom (duplicate the colour into
// a dword, `shr ecx,1 / rep stosd / adc ecx,ecx / rep stosw`).
// DC bitmap16.cpp:699 (0x51566..0x51568) and retail both advance the row
// cursor unconditionally. Retain that original source behavior for the exact
// reconstruction, including its unused out-of-range pointer after a bottom-edge
// rectangle with x > 0. For an owned 4x4 bitmap, fillRect(1,3,1,1) ends at
// pixel offset 17, beyond one-past 16. The allocation does not include padding
// that would make this valid portable C++. The earlier guarded repair scored
// 82.1964%; tested integral offsets and zero-column origins do not match.
VA(0x0044e4c0, 0x7D)
DC_ADDRESS(0x05150c, 0x70)
MAC_ADDRESS(0x05c348, 0xec)  // anchor-caller(textWidget::Draw, FadeToBlack) + order-map(DC bitmap16.obj)
void Bitmap16Bit::FillRect(int x, int y, int w, int h, unsigned short color)
{
    if (w > GetWidth() - x)
        w = GetWidth() - x;
    if (h > GetHeight() - y)
        h = GetHeight() - y;

    if (w && h) {
        Bitmap16MapPointer dst;
        dst.m_pixels = GetMap(x, y);
        for (int row = 0; row < h; ++row) {
            for (int col = 0; col < w; ++col)
                dst.m_pixels[col] = color;
            dst.m_bytes += GetPitch();
        }
    }
}

// E:\gamedcs\bitmap16.cpp:705. The Dreamcast dossier ()
// proves the clipped width/height, one GetMap call, row loop, full top/bottom
// rows, and two endpoint stores on interior rows. Retail independently fixes
// Pitch as a byte stride and preserves this 18-block source shape.
// Row-boundary residual (95.2113%): step only on a visited following row.
// The last-row guard scores 94.3662%, visited-row offsets 70.2113%; original
// 100% forms end+x at the bottom edge and fails the native pointer control.
VA(0x0044e540, 0xA3)
DC_ADDRESS(0x05157c, 0x98)
MAC_ADDRESS(0x05c434, 0x11c)
void Bitmap16Bit::FrameRect(int x, int y, int w, int h,
                            unsigned short color)
{
    if (w > GetWidth() - x)
        w = GetWidth() - x;
    if (h > GetHeight() - y)
        h = GetHeight() - y;

    if (w && h) {
        Bitmap16MapPointer dst;
        dst.m_pixels = GetMap(x, y);
        for (int row = 0; row < h; ++row) {
            if (row == 0 || row == h - 1) {
                for (int col = 0; col < w; ++col)
                    dst.m_pixels[col] = color;
            } else {
                dst.m_pixels[0] = color;
                dst.m_pixels[w - 1] = color;
            }
            dst.m_bytes += GetPitch();
        }
    }
}

// E:\gamedcs\bitmap16.cpp:742. Dreamcast () proves the clipped
// rectangle, one GetMap expression, RGB shift-mask construction, and nested
// row/pixel loops. Complete inlines GetMap and independently fixes Pitch as
// a byte stride; the earlier unchecked 0xA4-byte body was exact.
// Row-boundary residual (82.6377%): an integral byte displacement advances
// after each row, but the pointer is formed only on a visit. Next-row guards
// score 77.4203%, last-row guards 64.5072%; the DC pixel/mask work is retained.
VA(0x0044E5F0, 0xA4)
DC_ADDRESS(0x051614, 0x94)
MAC_ADDRESS(0x05c550, 0x1a8)
void Bitmap16Bit::Darken(int x, int y, int w, int h)
{
    if (w > GetWidth() - x)
        w = GetWidth() - x;
    if (h > GetHeight() - y)
        h = GetHeight() - y;

    if (w && h) {
        unsigned long shiftMask =
            ((Bitmap16Bit::red_mask >> 1) & Bitmap16Bit::red_mask)
            | ((Bitmap16Bit::green_mask >> 1) & Bitmap16Bit::green_mask)
            | ((Bitmap16Bit::blue_mask >> 1) & Bitmap16Bit::blue_mask);
        Bitmap16MapPointer row;
        row.m_pixels = GetMap(x, y);

        for (int iy = 0; iy < h; ++iy) {
            Bitmap16MapPointer pixel = row;
            for (int ix = 0; ix < w; ++ix) {
                *pixel.m_pixels = static_cast<unsigned short>(
                    (*pixel.m_pixels >> 1) & shiftMask);
                ++pixel.m_pixels;
            }

            row.m_bytes += GetPitch();
        }
    }
}

// E:\gamedcs\bitmap16.cpp:778. The masked Darken overload: the same halve-
// and-mask pass as the plain one, applied only where the 8-bit companion
// bitmap has a non-zero byte. The mask row stride is its WIDTH, not its
// Pitch - retail adds [mask+0x24] at the foot of every row - while the
// starting row is still taken through Pitch.
// Dreamcast line 808 and retail both advance the mask and bitmap row pointers
// after the inner pixel loop. Keep DC GetMap/GetPitch and their different
// pitch meanings (dc 0x52570/0x5256c), not a width-to-pitch substitution.
VA(0x0044e6a0, 0xE0)
DC_ADDRESS(0x0516a8, 0xd4)
MAC_ADDRESS(0x05c6f8, 0x10c)  // MAC_ABSTRACTION_FROM(tokens1:ad3fb507f76d,31.1644): canonical dimension/pitch getters replace the expanded field reads; Windows remains exact.
void Bitmap16Bit::Darken(int x, int y, int w, int h, Bitmap816* mask,
                         int sx, int sy)
{
    if (w > GetWidth() - x)
        w = GetWidth() - x;
    if (h > GetHeight() - y)
        h = GetHeight() - y;

    if (w && h) {
        unsigned int shiftMask =
            ((Bitmap16Bit::red_mask >> 1) & Bitmap16Bit::red_mask)
            | ((Bitmap16Bit::green_mask >> 1) & Bitmap16Bit::green_mask)
            | ((Bitmap16Bit::blue_mask >> 1) & Bitmap16Bit::blue_mask);
        unsigned char* maskRow = mask->GetMap(sx, sy);
        Bitmap16MapPointer row;
        row.m_pixels = GetMap(x, y);

        for (int iy = 0; iy < h; ++iy) {
            unsigned char* maskPixel = maskRow;
            Bitmap16MapPointer pixel = row;
            for (int ix = 0; ix < w; ++ix) {
                if (*maskPixel) {
                    *pixel.m_pixels = static_cast<unsigned short>(
                        (*pixel.m_pixels >> 1) & shiftMask);
                }
                ++maskPixel;
                ++pixel.m_pixels;
            }
            maskRow += mask->GetPitch();
            row.m_bytes += GetPitch();
        }
    }
}

VA(0x0044e780, 0x1BF)
DC_ADDRESS(0x05177c, 0x246)
MAC_ADDRESS(0x05c804, 0x1b0)
void Bitmap16Bit::Colorize(int x, int y, int width, int height,
                           unsigned short color)
{
    float redLevel = static_cast<float>(color & Bitmap16Bit::red_mask)
                       / static_cast<float>(Bitmap16Bit::red_mask);
    float greenLevel = static_cast<float>(color & Bitmap16Bit::green_mask)
                        / static_cast<float>(Bitmap16Bit::green_mask);
    float blueLevel = static_cast<float>(color & Bitmap16Bit::blue_mask)
                      / static_cast<float>(Bitmap16Bit::blue_mask);

    float top = redLevel > greenLevel ? redLevel : greenLevel;
    if (top < blueLevel)
        top = blueLevel;

    float bottom = redLevel > greenLevel ? greenLevel : redLevel;
    if (bottom > blueLevel)
        bottom = blueLevel;

    float saturation = top != 0.0 ? (top - bottom) / top : 0.0f;

    float hue;
    if (saturation == 0.0) {
        hue = 0.0f;
    } else {
        float span = top - bottom;
        if (redLevel == top)
            hue = (greenLevel - blueLevel) / span;
        else if (greenLevel == top)
            hue = (blueLevel - redLevel) / span + 2.0f;
        else
            hue = (redLevel - greenLevel) / span + 4.0f;
        hue *= 60.0f;
        if (hue < 0.0)
            hue += 360.0f;
    }
    hue /= 360.0f;
    Colorize(x, y, width, height, hue, saturation);
}

// E:\gamedcs\bitmap16.cpp:873. The float Colorize, tail-called by the
// 16-bit-colour overload above. Each pixel's three channels are prescaled to
// a full 31-bit range by INT_MAX/mask, their maximum becomes the HSV value,
// and the hue's sextant selects which of v/p/q/t each channel takes back.
// The integer and float overloads use the same class-owned RGB masks.
// The sextant, `hue * 6`, `1.0f - saturation` and the fmod argument's
// double conversion are all loop-invariant and land in the inner loop's
// preheader; only the fmod call itself stays per-pixel, because VC6 will not
// hoist an opaque call.
// Row-boundary residual (96.5654%): integral relative byte offsets avoid the
// final end+x pointer. Next/last guards score 94.6013/94.5490%; unchecked
// row walks are not safe. Native one-row differential tests cover all six
// hue sectors.
// EXACT: the helper's by-value parameter owns the double scratch storage.
// Sixteen source states / eight reproduced objects isolate this from the
// caller's early-return scope (DC 884/885), plain ushort row/pixel pointers,
// and the helper's ordinary declaration; those source restorations are flat.
VA(0x0044e940, 0x3B8)
DC_ADDRESS(0x0519c4, 0x47c)
MAC_ADDRESS(0x05c9b4, 0x328)  // anchor-caller(the 16-bit Colorize tail call) + order-map(DC bitmap16.obj)
void Bitmap16Bit::Colorize(int x, int y, int w, int h, float hue,
                           float saturation)
{
    if (w > Width - x)
        w = Width - x;
    if (h > Height - y)
        h = Height - y;

    if (!w || !h)
        return;

    const unsigned int redNorm =
        std::numeric_limits<int>::max() / Bitmap16Bit::red_mask;
    const unsigned int greenNorm =
        std::numeric_limits<int>::max() / Bitmap16Bit::green_mask;
    const unsigned int blueNorm =
        std::numeric_limits<int>::max() / Bitmap16Bit::blue_mask;

    Bitmap16MapPointer row;
    row.m_pixels = GetMap(x, y);

    for (int iy = 0; iy < h; ++iy) {
        Bitmap16MapPointer pixel = row;
        for (int ix = 0; ix < w; ++ix) {
            unsigned int r =
                (*pixel.m_pixels & Bitmap16Bit::red_mask) * redNorm;
            unsigned int g =
                (*pixel.m_pixels & Bitmap16Bit::green_mask) * greenNorm;
            unsigned int b =
                (*pixel.m_pixels & Bitmap16Bit::blue_mask) * blueNorm;

            const unsigned int max =
                (r > g ? r : g) > b ? (r > g ? r : g) : b;
            const float v = static_cast<float>(max);
            const float f =
                static_cast<float>(fmod(hue * 6.0f, 1.0));
            const float p = v * (1.0f - saturation);
            const float q = v * (1.0f - saturation * f);
            const float t = v * (1.0f - saturation * (1.0f - f));

            switch (static_cast<int>(hue * 6.0f)) {
            case HSV_RED_SECTOR:
                r = ftol(v);
                g = ftol(t);
                b = ftol(p);
                break;
            case HSV_YELLOW_SECTOR:
                r = ftol(q);
                g = ftol(v);
                b = ftol(p);
                break;
            case HSV_GREEN_SECTOR:
                r = ftol(p);
                g = ftol(v);
                b = ftol(t);
                break;
            case HSV_CYAN_SECTOR:
                r = ftol(p);
                g = ftol(q);
                b = ftol(v);
                break;
            case HSV_BLUE_SECTOR:
                r = ftol(t);
                g = ftol(p);
                b = ftol(v);
                break;
            case HSV_MAGENTA_SECTOR:
                r = ftol(v);
                g = ftol(p);
                b = ftol(q);
                break;
            }

            *pixel.m_pixels = static_cast<unsigned short>(
                ((b / blueNorm) & Bitmap16Bit::blue_mask)
                | ((g / greenNorm) & Bitmap16Bit::green_mask)
                | ((r / redNorm) & Bitmap16Bit::red_mask));
            ++pixel.m_pixels;
        }
        row.m_bytes += Pitch;
    }
}

// Original: Bitmap16Bit::Gray; bitmap16.cpp:934
DC_ADDRESS(0x051e40, 0x148)
void Bitmap16Bit::Gray(int x, int y, int w, int h)
{
    if (w > Width - x)
        w = Width - x;
    if (h > Height - y)
        h = Height - y;
    if (!w || !h)
        return;

    const unsigned int redNorm = std::numeric_limits<int>::max() / Bitmap16Bit::red_mask;
    const unsigned int greenNorm = std::numeric_limits<int>::max() / Bitmap16Bit::green_mask;
    const unsigned int blueNorm = std::numeric_limits<int>::max() / Bitmap16Bit::blue_mask;
    unsigned short* row = GetMap(x, y);
    for (int rowIndex = 0; rowIndex < h; ++rowIndex) {
        unsigned short* pixel = row;
        for (int column = 0; column < w; ++column) {
            unsigned int r = (*pixel & Bitmap16Bit::red_mask) * redNorm;
            unsigned int g = (*pixel & Bitmap16Bit::green_mask) * greenNorm;
            unsigned int b = (*pixel & Bitmap16Bit::blue_mask) * blueNorm;
            unsigned int gray = (r > g ? r : g) > b ? (r > g ? r : g) : b;
            *pixel = static_cast<unsigned short>(
                ((gray / redNorm) & Bitmap16Bit::red_mask) |
                ((gray / greenNorm) & Bitmap16Bit::green_mask) |
                ((gray / blueNorm) & Bitmap16Bit::blue_mask));
            ++pixel;
        }
        Bitmap16MapPointer nextRow;
        nextRow.m_pixels = row;
        nextRow.m_bytes += Pitch;
        row = nextRow.m_pixels;
    }
}

// Original: Bitmap16Bit::GrabAndBlur; bitmap16.cpp:979
// The recorded 1017..1103 interior path explicitly sums the four neighboring
// pixels on each axis, excluding the center; 1109..1241 checks those same
// sixteen samples individually at the image edges and divides by their count.
DC_ADDRESS(0x051f88, 0x5e4)
void Bitmap16Bit::GrabAndBlur(const Bitmap16Bit* src, int sx, int sy)
{
    int w = Width;
    int h = Height;
    int sw = src->GetWidth();
    int sh = src->GetHeight();
    int sp = src->GetPitch();
    if (w > sw - sx)
        w = sw - sx;
    if (h > sh - sy)
        h = sh - sy;

    unsigned short* dstRow = map;
    const unsigned short* srcRow = src->GetMap(sx, sy);
    for (int y = 0; y < h; ++y) {
        unsigned short* d = dstRow;
        const unsigned short* s = srcRow;
        for (int x = 0; x < w; ++x) {
            int spp = sp / 2;
            unsigned int r = 0;
            unsigned int g = 0;
            unsigned int b = 0;
            unsigned int color;
            const int blurRadius = 4;
            if (sx + x >= blurRadius && sx + x < sw - blurRadius &&
                sy + y >= blurRadius && sy + y < sh - blurRadius) {
                color = s[-4];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[-3];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[-2];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[-1];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[1];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[2];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[3];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[4];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[-4 * spp];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[-3 * spp];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[-2 * spp];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[-1 * spp];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[1 * spp];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[2 * spp];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[3 * spp];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                color = s[4 * spp];
                r += color & Bitmap16Bit::red_mask;
                g += color & Bitmap16Bit::green_mask;
                b += color & Bitmap16Bit::blue_mask;

                r = (r / (blurRadius * 4)) & Bitmap16Bit::red_mask;
                g = (g / (blurRadius * 4)) & Bitmap16Bit::green_mask;
                b = (b / (blurRadius * 4)) & Bitmap16Bit::blue_mask;
            } else {
                int count = 0;
                if (sx + x - 4 >= 0) {
                    color = s[-4];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sx + x - 3 >= 0) {
                    color = s[-3];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sx + x - 2 >= 0) {
                    color = s[-2];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sx + x - 1 >= 0) {
                    color = s[-1];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sx + x + 1 < sw) {
                    color = s[1];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sx + x + 2 < sw) {
                    color = s[2];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sx + x + 3 < sw) {
                    color = s[3];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sx + x + 4 < sw) {
                    color = s[4];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sy + y - 4 >= 0) {
                    color = s[-4 * spp];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sy + y - 3 >= 0) {
                    color = s[-3 * spp];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sy + y - 2 >= 0) {
                    color = s[-2 * spp];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sy + y - 1 >= 0) {
                    color = s[-1 * spp];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sy + y + 1 < sh) {
                    color = s[1 * spp];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sy + y + 2 < sh) {
                    color = s[2 * spp];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sy + y + 3 < sh) {
                    color = s[3 * spp];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                if (sy + y + 4 < sh) {
                    color = s[4 * spp];
                    r += color & Bitmap16Bit::red_mask;
                    g += color & Bitmap16Bit::green_mask;
                    b += color & Bitmap16Bit::blue_mask;
                    ++count;
                }
                r = (r / count) & Bitmap16Bit::red_mask;
                g = (g / count) & Bitmap16Bit::green_mask;
                b = (b / count) & Bitmap16Bit::blue_mask;
            }
            *d++ = static_cast<unsigned short>(r | g | b);
            ++s;
        }
        Bitmap16ConstMapPointer nextSource;
        nextSource.m_pixels = srcRow;
        nextSource.m_bytes += sp;
        srcRow = nextSource.m_pixels;
        Bitmap16MapPointer nextTarget;
        nextTarget.m_pixels = dstRow;
        nextTarget.m_bytes += Pitch;
        dstRow = nextTarget.m_pixels;
    }
}

#if 0  // @carcass

// E:\gamedcs\bitmap16.cpp:224
// E:\gamedcs\bitmap16.cpp:234
// E:\gamedcs\bitmap16.cpp:243
// E:\gamedcs\bitmap16.cpp:253
// E:\gamedcs\bitmap16.cpp:262
// E:\gamedcs\bitmap16.cpp:335
// E:\gamedcs\bitmap16.cpp:358
// E:\gamedcs\bitmap16.cpp:541
// RETAIL_LOCATED(0x0044e2b0, 0x139): anchor-bracket, not reconstructed.

// E:\gamedcs\bitmap16.cpp:625
// RETAIL_LOCATED(0x0044e3f0, 0xC9): anchor-bracket, not reconstructed.

// E:\gamedcs\bitmap16.cpp:679
// RETAIL_LOCATED(0x0044e4c0, 0x7D): anchor-global, not reconstructed.

// E:\gamedcs\bitmap16.cpp:873
// Retail body reconstructed above at 0x0044e940; dc 0x519c4.
void Bitmap16Bit::colorize(int x, int y, int w, int h, float hue, float saturation)
{
    // @stub
}

#endif  // @carcass
