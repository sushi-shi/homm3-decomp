#include <va.h>
#include <limits>
#include <math.h>
#include <string.h>
#include "bitmap16.h"
#include "bitmap816.h"
#include "hsv.h"
#include "pcx.h"

// Retail's destructor is frameless under /GX, proving that this TU saw the
// deallocator as nothrow (the same header contract established by sample.obj).
__declspec(nothrow) void __cdecl operator delete(void* p);

// Convert using the low word of the biased double representation.
// Original: ftol; bitmap16.cpp:59, dc 0x50a9c
static long ftol(double d)
{
    const unsigned long magic = 0x59c00000;
    d += *static_cast<const float*>(static_cast<const void*>(&magic));
    return *static_cast<long*>(static_cast<void*>(&d));
}

#if 0  // @carcass

// E:\gamedcs\bitmap16.cpp:117
DC_ONLY(0x50c04, 0x104)
void Bitmap16Bit::Bitmap16Bit(const char* name, int w, int h)
{
    // @stub
}

// E:\gamedcs\bitmap16.cpp:208
#endif  // @carcass

VA_COMPGEN(0x0044e020, 0x21, SCALAR_DELETING_DTOR, Bitmap16Bit)

VA(0x0044df70, 0xA3)
Bitmap16Bit::Bitmap16Bit(int w, int h)
    : resource(0, RESOURCE_TYPE_NONE),
      m_imageSize(w * h * 2), m_width(w), m_height(h), m_pitch(w * 2)
{
    m_dataSize = m_imageSize;

    if (w && h) {
        m_map = new unsigned short[m_dataSize / 2];
        m_referenced = 0;
    } else {
        m_map = 0;
    }
}

VA(0x0044e050, 0xA5)  // in-span, name/type base ctor + vftable 0x63b9c8
Bitmap16Bit::Bitmap16Bit(const char* name, int w, int h)
    : resource(name, RESOURCE_TYPE_BITMAP16),
      m_imageSize(w * h * 2), m_width(w), m_height(h), m_pitch(w * 2),
      m_referenced(0)
{
    m_dataSize = m_imageSize;

    if (w > 0 && h > 0)
        m_map = new unsigned short[m_dataSize / 2];
    else
        m_map = 0;
}

// Original: Bitmap16Bit::Bitmap16Bit; bitmap16.cpp:152, dc 0x50d08
// Complete's 0x38-byte bitmap keeps the heap-buffer arm of DC's allocation
// path; DDSURFACEDESC, BMCreateSurface and the locked-surface owner are absent.
Bitmap16Bit::Bitmap16Bit(const char* name, int w, int h,
                         const unsigned short* data, int size)
    : resource(name, RESOURCE_TYPE_BITMAP16),
      m_imageSize(w * h * 2), m_width(w), m_height(h), m_pitch(w * 2)
{
    m_dataSize = size ? size : m_imageSize;
    m_map = new unsigned short[m_dataSize / 2];
    m_referenced = 0;
    if (m_map)
        memcpy(m_map, data, m_dataSize);
}

// Original: Bitmap16Bit::Bitmap16Bit; bitmap16.cpp:187, dc 0x50e04
Bitmap16Bit::Bitmap16Bit(const char* name, const char* path)
    : resource(name, RESOURCE_TYPE_BITMAP16),
      m_dataSize(0), m_imageSize(0), m_width(0), m_height(0), m_pitch(0), m_map(0)
{
    char fileName[261];
    strcpy(fileName, path);
    strcat(fileName, name);
    importPCXFile(fileName);
    m_referenced = 0;
}

VA(0x0044e100, 0x29)  // dc 0x50ebc
Bitmap16Bit::~Bitmap16Bit()
{
    if (m_map && !m_referenced)
        delete[] m_map;
}

// E:\gamedcs\bitmap16.cpp:224/234/243/253. The four pixel-format converters
// the compiland's own Remap runs the surface through. NONE of them has a
// retail row: /Ob2 expands all four at Remap's two arms, which is the only
// place in the image that reaches them, so the shift/mask chains below are
// read straight out of that body.
unsigned long color1555to8888(unsigned short color)
{
    return ((((((color & 0xffff8000) << 7 | (color & 0x7c00)) << 3)
              | (color & 0x3e0))
             << 3
             | (color & 0x1f))
            << 3);
}

unsigned long color0565to8888(unsigned short color)
{
    return ((((color & 0xf800) << 3 | (color & 0x7e0)) << 2 | (color & 0x1f))
            << 3);
}

unsigned short color8888to1555(unsigned long color)
{
    return static_cast<unsigned short>(((color >> 9) & 0x7c00)
                                       | ((color >> 6) & 0x3e0)
                                       | ((color >> 3) & 0x1f)
                                       | ((color >> 16) & 0x8000));
}

unsigned short color8888to0565(unsigned long color)
{
    return static_cast<unsigned short>(((color >> 8) & 0xf800)
                                       | ((color >> 5) & 0x7e0)
                                       | ((color >> 3) & 0x1f));
}

VA(0x0044e130, 0x110)  // dc 0x50fd4
void Bitmap16Bit::remap(int oldGreenBits)
{
    for (int col = 0; col < m_width; col++) {
        for (int row = 0; row < m_height; row++) {
            Bitmap16MapPointer pixel;
            pixel.m_pixels = m_map;
            pixel.m_bytes += row * m_pitch + col * sizeof(unsigned short);
            if (oldGreenBits == BITMAP_GREEN_BITS_565)
                *pixel.m_pixels = color8888to1555(color0565to8888(*pixel.m_pixels));
            else
                *pixel.m_pixels = color8888to0565(color1555to8888(*pixel.m_pixels));
        }
    }
}

VA(0x0044e240, 0x07)  // vtable slot 2: fixed object extent + pixel bytes
unsigned int Bitmap16Bit::getSize() const
{
    return sizeof(*this) + m_dataSize;
}

// Original: Bitmap16Bit::import; bitmap16.cpp:294, dc 0x51078
void Bitmap16Bit::import(int w, int h, const unsigned short* data, int size)
{
    clear();
    m_width = w;
    m_height = h;
    m_pitch = w * 2;
    m_imageSize = w * h * 2;
    m_dataSize = size ? size : m_imageSize;
    m_map = new unsigned short[m_dataSize / 2];
    m_referenced = 0;
    if (m_map)
        memcpy(m_map, data, m_dataSize);
}

VA(0x0044e250, 0x5D)  // dc 0x51154
void Bitmap16Bit::reference(int w, int h, int pitch, unsigned short* data)
{
    clear();
    m_width = w;
    m_height = h;
    m_pitch = pitch;
    m_imageSize = w * h * 2;
    m_dataSize = m_imageSize;
    m_map = data;
    m_referenced = 1;
}

// DC bitmap16.cpp:358 supplies the ordinary clear helper called by reference.
// Complete inlines its scalar resets and borrowed-buffer release; the DC-only
// surface-release arm has no corresponding field or operation in retail.
DC_ONLY(0x51198, 0x90)
void Bitmap16Bit::clear()
{
    m_width = 0;
    m_height = 0;
    m_dataSize = 0;
    m_imageSize = 0;
    if (m_map) {
        if (!m_referenced)
            delete[] m_map;
        m_map = 0;
        m_referenced = 0;
    }
}

// Original: Bitmap16Bit::importPCXFile; bitmap16.cpp:472, dc 0x51228
int Bitmap16Bit::importPCXFile(const char* filename)
{
    PcxData pdat;
    imgdes pcxfile;
    if (pcxinfo(filename, &pdat))
        return 1;

    m_width = pdat.m_width;
    m_height = pdat.m_length;
    m_pitch = pdat.m_width * 2;
    m_imageSize = m_width * m_height * 2;
    m_dataSize = m_imageSize;
    m_map = new unsigned short[m_dataSize / 2];
    m_referenced = 0;
    if (!m_map)
        return 2;

    allocimage(&pcxfile, pdat.m_width, pdat.m_length,
               pdat.m_bpPixel * pdat.m_nplanes);
    loadpcx(filename, &pcxfile);
    flipimage(&pcxfile, &pcxfile);
    for (int y = 0; y < m_height; ++y) {
        // DC 0x51310..0x5132a advances this destination by Width bytes,
        // although the copied row contains Width words. Preserve that old
        // importer's addressing; neither retained PCX importer calls it.
        Bitmap16MapPointer row;
        row.m_pixels = m_map;
        memcpy(row.m_bytes + y * m_width,
               pcxfile.m_ibuff + y * pcxfile.m_buffwidth,
               m_width * sizeof(unsigned short));
    }
    freeimage(&pcxfile);
    return 0;
}

// E:\gamedcs\bitmap16.cpp:541
VA(0x0044e2b0, 0x139)  // order-map(DC bitmap16.obj, immediately before Grab), dc 0x51378
void Bitmap16Bit::draw(int srcX, int srcY, int srcWidth, int srcHeight,
                       unsigned short* dst, int dstX, int dstY, int dstWidth,
                       int dstHeight, int dstPitch, bool flipped) const
{
    int w = srcWidth;

    if (dstX < 0) {
        srcX -= dstX;
        w += dstX;
        dstX = 0;
    }
    if (dstY < 0) {
        srcY -= dstY;
        srcHeight += dstY;
        dstY = 0;
    }
    if (w + dstX > dstWidth)
        w = dstWidth - dstX;
    if (srcHeight + dstY > dstHeight)
        srcHeight = dstHeight - dstY;

    if (w > 0 && srcHeight > 0) {
        Bitmap16ConstMapPointer source;
        source.m_pixels = getMap(0, srcY);
        Bitmap16MapPointer target;
        target.m_pixels = dst;
        target.m_bytes += dstY * dstPitch;

        if (flipped) {
            for (int row = 0; row < srcHeight; ++row) {
                for (int col = 0; col < w; ++col) {
                    // BOUND BY `const unsigned short&`: retail loads the
                    // source pixel twice - once for the key compare and once
                    // for the store - rather than keeping it in a register.
                    // 81.3095 -> 83.5794.
                    const unsigned short& pixel = source.m_pixels[srcX + col];
                    if (pixel != static_cast<unsigned short>(flipped))
                        target.m_pixels[dstX + col] = pixel;
                }
                source.m_bytes += m_pitch;
                target.m_bytes += dstPitch;
            }
        } else {
            for (int row = 0; row < srcHeight; ++row) {
                memcpy(target.m_pixels + dstX, source.m_pixels + srcX,
                       w * sizeof(unsigned short));
                source.m_bytes += m_pitch;
                target.m_bytes += dstPitch;
            }
        }
    }
}

// E:\gamedcs\bitmap16.cpp:625
VA(0x0044e3f0, 0xC9)  // order-map(DC bitmap16.obj, between Draw and FillRect), dc 0x51468
void Bitmap16Bit::grab(const unsigned short* src, int srcX, int srcY,
                       int srcWidth, int srcHeight, int srcPitch)
{
    int dstX = 0;
    int dstY = 0;
    int w = m_width;
    int h = m_height;

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

    if (w > 0 && h > 0) {
        Bitmap16MapPointer dst;
        dst.m_pixels = getMap(dstX, dstY);
        Bitmap16ConstMapPointer source;
        source.m_pixels = src;
        source.m_bytes += srcY * srcPitch + srcX * sizeof(unsigned short);
        unsigned char* dstRowBase = dst.m_bytes;
        int dstRowOffset = 0;
        const unsigned char* sourceRowBase = source.m_bytes;
        int sourceRowOffset = 0;
        for (int row = 0; row < h;
             dstRowOffset += m_pitch, sourceRowOffset += srcPitch, ++row) {
            dst.m_bytes = dstRowBase + dstRowOffset;
            source.m_bytes = sourceRowBase + sourceRowOffset;
            memcpy(dst.m_pixels, source.m_pixels, w * sizeof(unsigned short));
        }
    }
}

// E:\gamedcs\bitmap16.cpp:679. FrameRect's sibling, and the same clipped
// rectangle walk with the interior filled instead of outlined: VC6 turns
// the inner store loop into its word-fill idiom (duplicate the colour into
// a dword, `shr ecx,1 / rep stosd / adc ecx,ecx / rep stosw`).
// Bound the row cursor: advance only when another row is actually visited,
// avoiding retail's unused end+x pointer after a bottom-edge rectangle.
// DC bitmap16.cpp:679..703 supplies the one GetMap and nested pixel loops.
// Residual (82.1964%): deliberate row-boundary check absent in retail.
// Ten source forms / nine distinct objects: GetMap per row 53.8929%, byte
// offset 73.7857%, product 64.2143%, final-row guard 69.625%, allocation-row
// base 63.625%, break-before-step 68.7857%; unchecked control remains 100%.
// Actual-body native tests check output and every pointer step before it is
// formed, including padding/nonzero origins/bottom edges and the old defect
// as a failing control. No integer address or oversized allocation is used.
VA(0x0044e4c0, 0x7D)  // anchor-caller(textWidget::Draw, FadeToBlack) + order-map(DC bitmap16.obj), dc 0x5150c
void Bitmap16Bit::fillRect(int x, int y, int w, int h, unsigned short color)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;

    if (w && h) {
        Bitmap16MapPointer dst;
        dst.m_pixels = getMap(x, y);
        for (int row = 0; row < h; ++row) {
            if (row)
                dst.m_bytes += m_pitch;
            for (int col = 0; col < w; ++col)
                dst.m_pixels[col] = color;
        }
    }
}

// E:\gamedcs\bitmap16.cpp:705. The Dreamcast dossier (dc 0x5157c)
// proves the clipped width/height, one GetMap call, row loop, full top/bottom
// rows, and two endpoint stores on interior rows. Retail independently fixes
// Pitch as a byte stride and preserves this 18-block source shape.
// Row-boundary residual (95.2113%): step only on a visited following row.
// The last-row guard scores 94.3662%, visited-row offsets 70.2113%; original
// 100% forms end+x at the bottom edge and fails the native pointer control.
VA(0x0044e540, 0xA3)
void Bitmap16Bit::frameRect(int x, int y, int w, int h,
                            unsigned short color)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;

    if (w && h) {
        Bitmap16MapPointer dst;
        dst.m_pixels = getMap(x, y);
        for (int row = 0; row < h; ++row) {
            if (row == 0 || row == h - 1) {
                for (int col = 0; col < w; ++col)
                    dst.m_pixels[col] = color;
            } else {
                dst.m_pixels[0] = color;
                dst.m_pixels[w - 1] = color;
            }
            dst.m_bytes += m_pitch;
        }
    }
}

// E:\gamedcs\bitmap16.cpp:742. Dreamcast (dc 0x51614) proves the clipped
// rectangle, one GetMap expression, RGB shift-mask construction, and nested
// row/pixel loops. Complete inlines GetMap and independently fixes Pitch as
// a byte stride; the earlier unchecked 0xA4-byte body was exact.
// Row-boundary residual (82.6377%): an integral byte displacement advances
// after each row, but the pointer is formed only on a visit. Next-row guards
// score 77.4203%, last-row guards 64.5072%; the DC pixel/mask work is retained.
VA(0x0044E5F0, 0xA4)
void Bitmap16Bit::darken(int x, int y, int w, int h)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;

    if (w && h) {
        unsigned long shiftMask =
            ((Bitmap16Bit::s_blueMask >> 1) & Bitmap16Bit::s_blueMask)
            | ((Bitmap16Bit::s_greenMask >> 1) & Bitmap16Bit::s_greenMask)
            | ((Bitmap16Bit::s_redMask >> 1) & Bitmap16Bit::s_redMask);
        Bitmap16MapPointer row;
        row.m_pixels = getMap(x, y);

        unsigned char* rowRowBase = row.m_bytes;
        int rowRowOffset = 0;
        for (int iy = 0; iy < h; ++iy) {
            row.m_bytes = rowRowBase + rowRowOffset;
            Bitmap16MapPointer pixel = row;
            for (int ix = 0; ix < w; ++ix) {
                *pixel.m_pixels = static_cast<unsigned short>(
                    (*pixel.m_pixels >> 1) & shiftMask);
                ++pixel.m_pixels;
            }

            rowRowOffset += m_pitch;
        }
    }
}

// E:\gamedcs\bitmap16.cpp:778. The masked Darken overload: the same halve-
// and-mask pass as the plain one, applied only where the 8-bit companion
// bitmap has a non-zero byte. The mask row stride is its WIDTH, not its
// Pitch - retail adds [mask+0x24] at the foot of every row - while the
// starting row is still taken through Pitch.
// Row-boundary residual (86.8791%): both map and mask step only when another
// row exists; next-row guards score 67.6374%. Keep DC GetMap/GetPitch and
// their different pitch meanings (dc 0x52570/0x5256c), not a width->pitch fix.
// Native actual-body tests cover output, independent pitches and the old
// final-row defect; changing GetPitch to the storage pitch fails the oracle.
VA(0x0044e6a0, 0xE0)  // anchor-caller(UpdateGrid, seven pushes) + order-map(DC bitmap16.obj), dc 0x516a8
void Bitmap16Bit::darken(int x, int y, int w, int h, Bitmap816* mask,
                         int sx, int sy)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;

    if (w && h) {
        unsigned int shiftMask =
            ((Bitmap16Bit::s_blueMask >> 1) & Bitmap16Bit::s_blueMask)
            | ((Bitmap16Bit::s_greenMask >> 1) & Bitmap16Bit::s_greenMask)
            | ((Bitmap16Bit::s_redMask >> 1) & Bitmap16Bit::s_redMask);
        unsigned char* maskRow = mask->getMap(sx, sy);
        Bitmap16MapPointer row;
        row.m_pixels = getMap(x, y);

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
            if (iy + 1 < h) {
                maskRow += mask->getPitch();
                row.m_bytes += m_pitch;
            }
        }
    }
}

VA(0x0044e780, 0x1BF)  // dc 0x5177c
void Bitmap16Bit::colorize(int x, int y, int width, int height,
                           unsigned short color)
{
    float redLevel = static_cast<float>(color & Bitmap16Bit::s_redMask)
                       / static_cast<float>(Bitmap16Bit::s_redMask);
    float greenLevel = static_cast<float>(color & Bitmap16Bit::s_greenMask)
                        / static_cast<float>(Bitmap16Bit::s_greenMask);
    float blueLevel = static_cast<float>(color & Bitmap16Bit::s_blueMask)
                      / static_cast<float>(Bitmap16Bit::s_blueMask);

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
    colorize(x, y, width, height, hue, saturation);
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
VA(0x0044e940, 0x3B8)  // anchor-caller(the 16-bit Colorize tail call) + order-map(DC bitmap16.obj), dc 0x519c4
void Bitmap16Bit::colorize(int x, int y, int w, int h, float hue,
                           float saturation)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;

    if (!w || !h)
        return;

    const unsigned int redNorm =
        std::numeric_limits<int>::max() / Bitmap16Bit::s_redMask;
    const unsigned int greenNorm =
        std::numeric_limits<int>::max() / Bitmap16Bit::s_greenMask;
    const unsigned int blueNorm =
        std::numeric_limits<int>::max() / Bitmap16Bit::s_blueMask;

    Bitmap16MapPointer row;
    row.m_pixels = getMap(x, y);
    unsigned char* rowRowBase = row.m_bytes;
    int rowRowOffset = 0;

    for (int iy = 0; iy < h; ++iy) {
        row.m_bytes = rowRowBase + rowRowOffset;
        Bitmap16MapPointer pixel = row;
        for (int ix = 0; ix < w; ++ix) {
            unsigned int r =
                (*pixel.m_pixels & Bitmap16Bit::s_redMask) * redNorm;
            unsigned int g =
                (*pixel.m_pixels & Bitmap16Bit::s_greenMask) * greenNorm;
            unsigned int b =
                (*pixel.m_pixels & Bitmap16Bit::s_blueMask) * blueNorm;

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
                ((b / blueNorm) & Bitmap16Bit::s_blueMask)
                | ((g / greenNorm) & Bitmap16Bit::s_greenMask)
                | ((r / redNorm) & Bitmap16Bit::s_redMask));
            ++pixel.m_pixels;
        }
        rowRowOffset += m_pitch;
    }
}

// Original: Bitmap16Bit::Gray; bitmap16.cpp:934, dc 0x51e40
void Bitmap16Bit::gray(int x, int y, int w, int h)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;
    if (!w || !h)
        return;

    const unsigned int redNorm = std::numeric_limits<int>::max() / Bitmap16Bit::s_redMask;
    const unsigned int greenNorm = std::numeric_limits<int>::max() / Bitmap16Bit::s_greenMask;
    const unsigned int blueNorm = std::numeric_limits<int>::max() / Bitmap16Bit::s_blueMask;
    unsigned short* row = getMap(x, y);
    for (int rowIndex = 0; rowIndex < h; ++rowIndex) {
        unsigned short* pixel = row;
        for (int column = 0; column < w; ++column) {
            unsigned int r = (*pixel & Bitmap16Bit::s_redMask) * redNorm;
            unsigned int g = (*pixel & Bitmap16Bit::s_greenMask) * greenNorm;
            unsigned int b = (*pixel & Bitmap16Bit::s_blueMask) * blueNorm;
            unsigned int gray = (r > g ? r : g) > b ? (r > g ? r : g) : b;
            *pixel = static_cast<unsigned short>(
                ((gray / redNorm) & Bitmap16Bit::s_redMask) |
                ((gray / greenNorm) & Bitmap16Bit::s_greenMask) |
                ((gray / blueNorm) & Bitmap16Bit::s_blueMask));
            ++pixel;
        }
        Bitmap16MapPointer nextRow;
        nextRow.m_pixels = row;
        nextRow.m_bytes += m_pitch;
        row = nextRow.m_pixels;
    }
}

// Original: Bitmap16Bit::GrabAndBlur; bitmap16.cpp:979, dc 0x51f88
// The recorded 1017..1103 interior path explicitly sums the four neighboring
// pixels on each axis, excluding the center; 1109..1241 checks those same
// sixteen samples individually at the image edges and divides by their count.
void Bitmap16Bit::grabAndBlur(const Bitmap16Bit* src, int sx, int sy)
{
    int w = m_width;
    int h = m_height;
    int sw = src->getWidth();
    int sh = src->getHeight();
    int sp = src->getPitch();
    if (w > sw - sx)
        w = sw - sx;
    if (h > sh - sy)
        h = sh - sy;

    unsigned short* dstRow = m_map;
    const unsigned short* srcRow = src->getMap(sx, sy);
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
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[-3];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[-2];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[-1];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[1];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[2];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[3];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[4];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[-4 * spp];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[-3 * spp];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[-2 * spp];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[-1 * spp];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[1 * spp];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[2 * spp];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[3 * spp];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                color = s[4 * spp];
                r += color & Bitmap16Bit::s_redMask;
                g += color & Bitmap16Bit::s_greenMask;
                b += color & Bitmap16Bit::s_blueMask;

                r = (r / (blurRadius * 4)) & Bitmap16Bit::s_redMask;
                g = (g / (blurRadius * 4)) & Bitmap16Bit::s_greenMask;
                b = (b / (blurRadius * 4)) & Bitmap16Bit::s_blueMask;
            } else {
                int count = 0;
                if (sx + x - 4 >= 0) {
                    color = s[-4];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sx + x - 3 >= 0) {
                    color = s[-3];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sx + x - 2 >= 0) {
                    color = s[-2];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sx + x - 1 >= 0) {
                    color = s[-1];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sx + x + 1 < sw) {
                    color = s[1];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sx + x + 2 < sw) {
                    color = s[2];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sx + x + 3 < sw) {
                    color = s[3];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sx + x + 4 < sw) {
                    color = s[4];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sy + y - 4 >= 0) {
                    color = s[-4 * spp];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sy + y - 3 >= 0) {
                    color = s[-3 * spp];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sy + y - 2 >= 0) {
                    color = s[-2 * spp];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sy + y - 1 >= 0) {
                    color = s[-1 * spp];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sy + y + 1 < sh) {
                    color = s[1 * spp];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sy + y + 2 < sh) {
                    color = s[2 * spp];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sy + y + 3 < sh) {
                    color = s[3 * spp];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                if (sy + y + 4 < sh) {
                    color = s[4 * spp];
                    r += color & Bitmap16Bit::s_redMask;
                    g += color & Bitmap16Bit::s_greenMask;
                    b += color & Bitmap16Bit::s_blueMask;
                    ++count;
                }
                r = (r / count) & Bitmap16Bit::s_redMask;
                g = (g / count) & Bitmap16Bit::s_greenMask;
                b = (b / count) & Bitmap16Bit::s_blueMask;
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
        nextTarget.m_bytes += m_pitch;
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
DC_ONLY(0x51378, 0x139)
void Bitmap16Bit::draw(int sx, int sy, int sw, int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch, unsigned char alpha)
{
    // @stub
}

// E:\gamedcs\bitmap16.cpp:625
// RETAIL_LOCATED(0x0044e3f0, 0xC9): anchor-bracket, not reconstructed.
DC_ONLY(0x51468, 0xC9)
void Bitmap16Bit::grab(const unsigned short* src, int sx, int sy, int sw, int sh, int spitch)
{
    // @stub
}

// E:\gamedcs\bitmap16.cpp:679
// RETAIL_LOCATED(0x0044e4c0, 0x7D): anchor-global, not reconstructed.
DC_ONLY(0x5150c, 0x7D)
void Bitmap16Bit::fillRect(int x, int y, int w, int h, unsigned short color)
{
    // @stub
}

// E:\gamedcs\bitmap16.cpp:778
DC_ONLY(0x516a8, 0xD4)
void Bitmap16Bit::darken(int x, int y, int w, int h, Bitmap816* mask, int sx, int sy)
{
    // @stub
}

// E:\gamedcs\bitmap16.cpp:815
DC_ONLY(0x5177c, 0x246)
void Bitmap16Bit::colorize(int x, int y, int width, int height, unsigned short color)
{
    // @stub
}

// E:\gamedcs\bitmap16.cpp:873
// Retail body reconstructed above at 0x0044e940; dc 0x519c4.
void Bitmap16Bit::colorize(int x, int y, int w, int h, float hue, float saturation)
{
    // @stub
}



// E:\gamedcs\bitmap16.cpp:107
DC_ONLY(0x52580, 0x34)
void* Bitmap16Bit::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass
