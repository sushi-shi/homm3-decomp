// Bitmap16.cpp - Bitmap16Bit and its colour helpers (Loki h3maped object 39).
//
// The Loki link order: the constructors and destructor, the four pixel
// format converters, Remap and the bitmap operations; g++ then emits the
// Bitmap16.h inline members (this unit defines the key function) and the
// inline file-static ftol.
#include "va.h"

#include <assert.h>
#include <limits>
#include <math.h>
#include <string.h>

#include <stdexcept>
#include "bitmap16.h"

#include "bitmap816.h"
#include "hsv.h"

// Loki keeps the helper but asserts on entry (Bitmap16.cpp:61): nothing in
// the port is meant to reach it.
DC_ADDRESS(0x050a9c, 0x62)
inline static long ftol(double d)
{
#line 61
    assert(0);
    const unsigned long magic = 0x59c00000;
    d += *(const float*)&magic;
    return *(long*)&d;
}

unsigned int Bitmap16Bit::red_mask;
unsigned int Bitmap16Bit::green_mask;
unsigned int Bitmap16Bit::blue_mask;

VA(0x0044df70, 0xA3)
DC_ADDRESS(0x050b00, 0x104)
MAC_ADDRESS(0x05be20, 0xc4)
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

VA(0x0044e050, 0xA5)
DC_ADDRESS(0x050c04, 0x104)
MAC_ADDRESS(0x05bee4, 0xc0)
Bitmap16Bit::Bitmap16Bit(const char* name, int w, int h)
    : resource(name, RESOURCE_TYPE_BITMAP16),
      m_imageSize(w * h * 2), m_width(w), m_height(h), m_pitch(w * 2),
      m_referenced(0)
{
#line 94
    assert(w >= 0);
    assert(h >= 0);
    m_dataSize = m_imageSize;
    if (w > 0 && h > 0)
        m_map = new unsigned short[m_dataSize / 2];
    else
        m_map = 0;
}

DC_ADDRESS(0x050d08, 0xfa)
Bitmap16Bit::Bitmap16Bit(const char* name, int w, int h,
                         const unsigned short* data, int size)
    : resource(name, RESOURCE_TYPE_BITMAP16),
      m_imageSize(w * h * 2), m_width(w), m_height(h), m_pitch(w * 2)
{
    m_dataSize = size ? size : m_imageSize;
    m_map = new unsigned short[m_dataSize / 2];
    if (m_map)
        memcpy(m_map, data, m_dataSize);
    m_referenced = 0;
}

// Loki builds the path in a PATH_MAX buffer.
DC_ADDRESS(0x050e04, 0xb8)
Bitmap16Bit::Bitmap16Bit(const char* name, const char* path)
    : resource(name, RESOURCE_TYPE_BITMAP16),
      m_dataSize(0), m_imageSize(0), m_width(0), m_height(0), m_pitch(0),
      m_map(0)
{
    char filename[4096];
    strcpy(filename, path);
    strcat(filename, name);
    importPCXFile(filename);
    m_referenced = 0;
}

VA(0x0044e100, 0x29)
DC_ADDRESS(0x050ebc, 0x76)
MAC_ADDRESS(0x05bfa4, 0x7c)
Bitmap16Bit::~Bitmap16Bit()
{
    if (m_map && !m_referenced)
        delete[] m_map;
}

DC_ADDRESS(0x050f34, 0x2c)
unsigned long color1555to8888(unsigned short color)
{
    unsigned long result = (unsigned short)(color & 0x8000) << 16;
    result |= (unsigned short)(color & 0x7c00) << 9;
    result |= (unsigned short)(color & 0x03e0) << 6;
    result |= (unsigned short)(color & 0x001f) << 3;
    return result;
}

DC_ADDRESS(0x050f60, 0x20)
unsigned long color0565to8888(unsigned short color)
{
    unsigned long result = (unsigned short)(color & 0xf800) << 8;
    result |= (unsigned short)(color & 0x07e0) << 5;
    result |= (unsigned short)(color & 0x001f) << 3;
    return result;
}

DC_ADDRESS(0x050f80, 0x2e)
unsigned short color8888to1555(unsigned long color)
{
    unsigned long result = (color & 0x000000f8) >> 3;
    result |= (color & 0x0000f800) >> 6;
    result |= (color & 0x00f80000) >> 9;
    result |= (color & 0x80000000) >> 16;
    return result;
}

DC_ADDRESS(0x050fb0, 0x22)
unsigned short color8888to0565(unsigned long color)
{
    unsigned long result = (color & 0x000000f8) >> 3;
    result |= (color & 0x0000fc00) >> 5;
    result |= (color & 0x00f80000) >> 8;
    return result;
}

VA(0x0044e130, 0x110)
DC_ADDRESS(0x050fd4, 0xa4)
void Bitmap16Bit::Remap(int old_green_bits)
{
    if (old_green_bits == BITMAP_GREEN_BITS_565) {
#line 186
        assert(green_mask != 0x7e0);
    } else {
#line 190
        assert(green_mask == 0x7e0);
    }
    for (int x = 0; x < m_width; x++) {
        for (int y = 0; y < m_height; y++) {
            unsigned short* pixel = GetMap(x, y);
            if (old_green_bits == BITMAP_GREEN_BITS_565) {
                unsigned long color = color0565to8888(*pixel);
                *pixel = color8888to1555(color);
            } else {
                unsigned long color = color1555to8888(*pixel);
                *pixel = color8888to0565(color);
            }
        }
    }
}

DC_ADDRESS(0x051078, 0xda)
void Bitmap16Bit::import(int w, int h, const unsigned short* data, int size)
{
    clear();
    m_width = w;
    m_height = h;
    m_pitch = w * 2;
    m_imageSize = w * h * 2;
    m_dataSize = size ? size : m_imageSize;
    m_map = new unsigned short[m_dataSize / 2];
    if (m_map)
        memcpy(m_map, data, m_dataSize);
    m_referenced = 0;
}

VA(0x0044e250, 0x5D)
DC_ADDRESS(0x051154, 0x44)
MAC_ADDRESS(0x05c02c, 0x68)
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

DC_ADDRESS(0x051198, 0x90)
MAC_ADDRESS(0x05c094, 0x64)
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

// The Loki port has no PCX reader.
DC_ADDRESS(0x051228, 0x150)
int Bitmap16Bit::importPCXFile(const char* filename)
{
#line 448
    assert(0);
    return 2;
}

VA(0x0044e2b0, 0x139)
DC_ADDRESS(0x051378, 0xf0)
void Bitmap16Bit::Draw(int srcX, int srcY, int srcWidth, int srcHeight,
                       unsigned short* dst, int dstX, int dstY, int dstWidth,
                       int dstHeight, int dstPitch, bool flipped) const
{
    if (dstX < 0) {
        srcX -= dstX;
        srcWidth -= -dstX;
        dstX = 0;
    }
    if (dstY < 0) {
        srcY -= dstY;
        srcHeight -= -dstY;
        dstY = 0;
    }
    if (dstX + srcWidth > dstWidth)
        srcWidth = dstWidth - dstX;
    if (dstY + srcHeight > dstHeight)
        srcHeight = dstHeight - dstY;
    if (srcWidth <= 0 || srcHeight <= 0)
        return;

    const unsigned short* src = GetMap(srcX, srcY);
    dst = (unsigned short*)((unsigned char*)dst + dstY * dstPitch
                            + dstX * sizeof(unsigned short));
    if (flipped) {
        for (int row = 0; row < srcHeight; row++) {
            unsigned short* out = dst;
            const unsigned short* in = src;
            for (int col = 0; col < srcWidth; col++) {
                if (*in != flipped)
                    *out = *in;
                in++;
                out++;
            }
            src = (const unsigned short*)((const unsigned char*)src + m_pitch);
            dst = (unsigned short*)((unsigned char*)dst + dstPitch);
        }
    } else {
        for (int row = 0; row < srcHeight; row++) {
            memcpy(dst, src, srcWidth * sizeof(unsigned short));
            src = (const unsigned short*)((const unsigned char*)src + m_pitch);
            dst = (unsigned short*)((unsigned char*)dst + dstPitch);
        }
    }
}

VA(0x0044e3f0, 0xC9)
DC_ADDRESS(0x051468, 0xa4)
void Bitmap16Bit::Grab(const unsigned short* src, int srcX, int srcY,
                       int srcWidth, int srcHeight, int srcPitch)
{
    int dstX = 0;
    int dstY = 0;
    int w = m_width;
    int h = m_height;
    if (srcX < 0) {
        dstX -= srcX;
        w -= -srcX;
        srcX = 0;
    }
    if (srcY < 0) {
        dstY -= srcY;
        h -= -srcY;
        srcY = 0;
    }
    if (w > srcWidth - srcX)
        w = srcWidth - srcX;
    if (h > srcHeight - srcY)
        h = srcHeight - srcY;
    if (w <= 0 || h <= 0)
        return;

    unsigned short* dst = GetMap(dstX, dstY);
    const unsigned short* in = (const unsigned short*)
        ((const unsigned char*)src + srcY * srcPitch
         + srcX * sizeof(unsigned short));
    for (int row = 0; row < h; row++) {
        memcpy(dst, in, w * sizeof(unsigned short));
        dst = (unsigned short*)((unsigned char*)dst + m_pitch);
        in = (const unsigned short*)((const unsigned char*)in + srcPitch);
    }
}

VA(0x0044e4c0, 0x7D)
DC_ADDRESS(0x05150c, 0x70)
void Bitmap16Bit::FillRect(int x, int y, int w, int h, unsigned short color)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;
    if (!w || !h)
        return;

    unsigned short* row = GetMap(x, y);
    for (int iy = 0; iy < h; iy++) {
        unsigned short* pixel = row;
        for (int ix = 0; ix < w; ix++) {
            *pixel++ = color;
        }
        row = (unsigned short*)((unsigned char*)row + m_pitch);
    }
}

VA(0x0044e540, 0xA3)
DC_ADDRESS(0x05157c, 0x98)
void Bitmap16Bit::FrameRect(int x, int y, int w, int h, unsigned short color)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;
    if (!w || !h)
        return;

    unsigned short* row = GetMap(x, y);
    for (int iy = 0; iy < h; iy++) {
        unsigned short* pixel = row;
        if (iy == 0 || iy == h - 1) {
            for (int ix = 0; ix < w; ix++) {
                *pixel++ = color;
            }
        } else {
            *pixel = color;
            *(pixel + w - 1) = color;
        }
        row = (unsigned short*)((unsigned char*)row + m_pitch);
    }
}

VA(0x0044E5F0, 0xA4)
DC_ADDRESS(0x051614, 0x94)
void Bitmap16Bit::Darken(int x, int y, int w, int h)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;
    if (!w || !h)
        return;

    unsigned long mask = ((red_mask >> 1) & red_mask)
                       | ((green_mask >> 1) & green_mask)
                       | ((blue_mask >> 1) & blue_mask);
    unsigned short* row = GetMap(x, y);
    for (int iy = 0; iy < h; iy++) {
        unsigned short* pixel = row;
        for (int ix = 0; ix < w; ix++) {
            *pixel = (*pixel >> 1) & mask;
            pixel++;
        }
        row = (unsigned short*)((unsigned char*)row + m_pitch);
    }
}

VA(0x0044e6a0, 0xE0)
DC_ADDRESS(0x0516a8, 0xd4)
void Bitmap16Bit::Darken(int x, int y, int w, int h, Bitmap816* mask,
                         int sx, int sy)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;
    if (!w || !h)
        return;

    unsigned long shiftMask = ((red_mask >> 1) & red_mask)
                            | ((green_mask >> 1) & green_mask)
                            | ((blue_mask >> 1) & blue_mask);
    unsigned char* maskRow = mask->GetMap(sx, sy);
    unsigned short* row = GetMap(x, y);
    for (int iy = 0; iy < h; iy++) {
        unsigned char* maskPixel = maskRow;
        unsigned short* pixel = row;
        for (int ix = 0; ix < w; ix++) {
            if (*maskPixel)
                *pixel = (*pixel >> 1) & shiftMask;
            maskPixel++;
            pixel++;
        }
        maskRow += mask->GetPitch();
        row = (unsigned short*)((unsigned char*)row + m_pitch);
    }
}

VA(0x0044e780, 0x1BF)
DC_ADDRESS(0x05177c, 0x246)
void Bitmap16Bit::Colorize(int x, int y, int w, int h, unsigned short color)
{
    float r, g, b;
    float hue, saturation, value;
    float min, max;
    float delta;

    r = (float)(color & red_mask) / (float)red_mask;
    g = (float)(color & green_mask) / (float)green_mask;
    b = (float)(color & blue_mask) / (float)blue_mask;
    max = r > g ? r : g;
    if (max < b)
        max = b;
    min = r > g ? g : r;
    if (min > b)
        min = b;
    value = max;
    saturation = max != 0.0 ? (max - min) / max : 0.0f;
    if (saturation == 0.0) {
        hue = 0.0f;
    } else {
        delta = max - min;
        if (r == max)
            hue = (g - b) / delta;
        else if (g == max)
            hue = (b - r) / delta + 2.0f;
        else
            hue = (r - g) / delta + 4.0f;
        hue *= 60.0f;
        if (hue < 0.0)
            hue += 360.0f;
    }
    hue /= 360.0f;
    Colorize(x, y, w, h, hue, saturation);
}

DC_ADDRESS(0x0519c4, 0x47c)
void Bitmap16Bit::Colorize(int x, int y, int w, int h, float hue,
                           float saturation)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;
    if (!w || !h)
        return;

    const unsigned int redNorm = numeric_limits<int>::max() / red_mask;
    const unsigned int greenNorm = numeric_limits<int>::max() / green_mask;
    const unsigned int blueNorm = numeric_limits<int>::max() / blue_mask;
    unsigned short* row = GetMap(x, y);
    for (int iy = 0; iy < h; iy++) {
        unsigned short* pixel = row;
        for (int ix = 0; ix < w; ix++) {
            unsigned int r = (*pixel & red_mask) * redNorm;
            unsigned int g = (*pixel & green_mask) * greenNorm;
            unsigned int b = (*pixel & blue_mask) * blueNorm;
            const float v = (r > g ? r : g) > b ? (r > g ? r : g) : b;
            const float f = fmod(hue * 6.0f, 1.0);
            const float p = v * (1.0f - saturation);
            const float q = v * (1.0f - saturation * f);
            const float t = v * (1.0f - saturation * (1.0f - f));
            switch ((int)(hue * 6.0f)) {
            case HSV_RED_SECTOR:
                r = ftol(v); g = ftol(t); b = ftol(p);
                break;
            case HSV_YELLOW_SECTOR:
                r = ftol(q); g = ftol(v); b = ftol(p);
                break;
            case HSV_GREEN_SECTOR:
                r = ftol(p); g = ftol(v); b = ftol(t);
                break;
            case HSV_CYAN_SECTOR:
                r = ftol(p); g = ftol(q); b = ftol(v);
                break;
            case HSV_BLUE_SECTOR:
                r = ftol(t); g = ftol(p); b = ftol(v);
                break;
            case HSV_MAGENTA_SECTOR:
                r = ftol(v); g = ftol(p); b = ftol(q);
                break;
            }
            *pixel = (unsigned short)(((r / redNorm) & red_mask) |
                                      ((g / greenNorm) & green_mask) |
                                      ((b / blueNorm) & blue_mask));
            pixel++;
        }
        row = (unsigned short*)((unsigned char*)row + m_pitch);
    }
}

DC_ADDRESS(0x051e40, 0x148)
void Bitmap16Bit::Gray(int x, int y, int w, int h)
{
    if (w > m_width - x)
        w = m_width - x;
    if (h > m_height - y)
        h = m_height - y;
    if (!w || !h)
        return;

    const unsigned int redNorm = numeric_limits<int>::max() / red_mask;
    const unsigned int greenNorm = numeric_limits<int>::max() / green_mask;
    const unsigned int blueNorm = numeric_limits<int>::max() / blue_mask;
    unsigned short* row = GetMap(x, y);
    for (int iy = 0; iy < h; iy++) {
        unsigned short* pixel = row;
        for (int ix = 0; ix < w; ix++) {
            unsigned int r = (*pixel & red_mask) * redNorm;
            unsigned int g = (*pixel & green_mask) * greenNorm;
            unsigned int b = (*pixel & blue_mask) * blueNorm;
            unsigned int gray = (r > g ? r : g) > b ? (r > g ? r : g) : b;
            *pixel = (unsigned short)(((gray / redNorm) & red_mask) |
                                      ((gray / greenNorm) & green_mask) |
                                      ((gray / blueNorm) & blue_mask));
            pixel++;
        }
        row = (unsigned short*)((unsigned char*)row + m_pitch);
    }
}

DC_ADDRESS(0x051f88, 0x5e4)
void Bitmap16Bit::GrabAndBlur(const Bitmap16Bit* src, int sx, int sy)
{
    int w = m_width;
    int h = m_height;
    int sw = src->GetWidth();
    int sh = src->GetHeight();
    int sp = src->GetPitch();
    if (w > sw - sx)
        w = sw - sx;
    if (h > sh - sy)
        h = sh - sy;

    unsigned short* dstRow = m_map;
    const unsigned short* srcRow = src->GetMap(sx, sy);
    for (int y = 0; y < h; y++) {
        unsigned short* d = dstRow;
        const unsigned short* s = srcRow;
        for (int x = 0; x < w; x++) {
            unsigned int r = 0;
            unsigned int g = 0;
            unsigned int b = 0;
            int spp = sp / 2;
            unsigned short color;
            if (sx + x >= 4 && sx + x < sw - 4 && sy + y >= 4 && sy + y < sh - 4) {
                color = *(s - 4);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s - 3);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s - 2);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s - 1);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s + 1);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s + 2);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s + 3);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s + 4);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s - 4 * spp);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s - 3 * spp);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s - 2 * spp);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s - spp);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s + spp);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s + 2 * spp);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s + 3 * spp);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                color = *(s + 4 * spp);
                r += color & red_mask;
                g += color & green_mask;
                b += color & blue_mask;

                r = (r / 16) & red_mask;
                g = (g / 16) & green_mask;
                b = (b / 16) & blue_mask;
            } else {
                int count = 0;
                if (sx + x - 4 >= 0) {
                    color = *(s - 4);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sx + x - 3 >= 0) {
                    color = *(s - 3);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sx + x - 2 >= 0) {
                    color = *(s - 2);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sx + x - 1 >= 0) {
                    color = *(s - 1);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sx + x + 1 < sw) {
                    color = *(s + 1);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sx + x + 2 < sw) {
                    color = *(s + 2);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sx + x + 3 < sw) {
                    color = *(s + 3);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sx + x + 4 < sw) {
                    color = *(s + 4);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sy + y - 4 >= 0) {
                    color = *(s - 4 * spp);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sy + y - 3 >= 0) {
                    color = *(s - 3 * spp);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sy + y - 2 >= 0) {
                    color = *(s - 2 * spp);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sy + y - 1 >= 0) {
                    color = *(s - spp);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sy + y + 1 < sh) {
                    color = *(s + spp);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sy + y + 2 < sh) {
                    color = *(s + 2 * spp);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sy + y + 3 < sh) {
                    color = *(s + 3 * spp);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                if (sy + y + 4 < sh) {
                    color = *(s + 4 * spp);
                    r += color & red_mask;
                    g += color & green_mask;
                    b += color & blue_mask;
                    count++;
                }
                r = (r / count) & red_mask;
                g = (g / count) & green_mask;
                b = (b / count) & blue_mask;
            }
            *d++ = r | g | b;
            s++;
        }
        srcRow = (const unsigned short*)((const unsigned char*)srcRow + sp);
        dstRow = (unsigned short*)((unsigned char*)dstRow + m_pitch);
    }
}
