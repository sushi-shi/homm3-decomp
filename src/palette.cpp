// 39 functions in link order.
#include "va.h"

#include <limits>
#include <math.h>
#include <string.h>

#include "palette.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "platform.h"

union TFloatLongBits {
    unsigned long m_bits;
    float m_value;
};

union TDoubleLongBits {
    double m_value;
    long m_words[2];
};

// Dreamcast exposes this original helper boundary and its sole named local.
// Retail has no out-of-line copy because VC6 /Ob2 expands it into HSVToRGB.
// Original: ftol; palette.cpp:45, dc 0x10a244.
static long ftol(double d)
{
    const unsigned long magic = 0x59c00000;
    TFloatLongBits magicValue;
    TDoubleLongBits result;
    result.m_value = d;
    magicValue.m_bits = magic;
    result.m_value += magicValue.m_value;
    return result.m_words[0];
}

VA(0x00522650, 0x16) MAC_ADDRESS(0x13b73c, 0x40)  // dc 0x10a2a8
TPalette16::TPalette16()
    : resource(0, RESOURCE_TYPE_NONE)
{
}

VA_COMPGEN(0x00522670, 0x21, SCALAR_DELETING_DTOR, TPalette16)

// The raw 16-bit table overload: 0x80 dwords straight into the payload at
// +0x1c, the same shape TPalette24's raw-data constructor has at 0x522e80.
VA(0x005226a0, 0x2D) MAC_ADDRESS(0x13b77c, 0x68)  // dc 0x10a2f0
TPalette16::TPalette16(const unsigned short* newData)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_data, newData, sizeof(m_data));
}

VA(0x005226d0, 0x9D) MAC_ADDRESS(0x13b7e4, 0x88)  // dc 0x10a338
TPalette16::TPalette16(const TPalette24& p24, int rbits, int rshift,
                       int gbits, int gshift, int bbits, int bshift)
    : resource(0, RESOURCE_TYPE_NONE)
{
    convert24to16(p24.m_palette, rbits, rshift, gbits, gshift,
                  bbits, bshift);
}

// Original: TPalette16::TPalette16; palette.cpp:73, dc 0x10a3ac.
TPalette16::TPalette16(const TRGBA* rgba, int rbits, int rshift,
                       int gbits, int gshift, int bbits, int bshift)
    : resource(0, RESOURCE_TYPE_NONE)
{
    convertRGBAto16(rgba, rbits, rshift, gbits, gshift, bbits, bshift);
}

// Original: TPalette16::TPalette16; palette.cpp:79, dc 0x10a41c.
TPalette16::TPalette16(const tagRGBQUAD* quad, int rbits, int rshift,
                       int gbits, int gshift, int bbits, int bshift)
    : resource(0, RESOURCE_TYPE_NONE)
{
    convertRGBQUADto16(quad, rbits, rshift, gbits, gshift, bbits, bshift);
}

VA(0x00522770, 0x9F) MAC_ADDRESS(0x13b86c, 0x84)  // dc 0x10a498
TPalette16::TPalette16(const char* name, const TPalette24& p24,
                       int rbits, int rshift, int gbits, int gshift,
                       int bbits, int bshift)
    : resource(name, RESOURCE_TYPE_PALETTE)
{
    convert24to16(p24.m_palette, rbits, rshift, gbits, gshift,
                  bbits, bshift);
}

VA(0x00522810, 0xC6) MAC_ADDRESS(0x13b8f0, 0x120)  // dc 0x10a508
TPalette16::TPalette16(const TPalette24& p24)
    : resource(0, RESOURCE_TYPE_NONE)
{
    unsigned short* dst = m_data;
    const unsigned int redScale = (s_redMask + s_redMask) & ~s_redMask;
    const unsigned int greenScale = (s_greenMask + s_greenMask) & ~s_greenMask;
    const unsigned int blueScale = (s_blueMask + s_blueMask) & ~s_blueMask;
    const unsigned char* src = p24.m_palette;
    for (int index = 0; index < 256; ++index) {
        unsigned int red = ((src[3 * index] * redScale) >> 8) & s_redMask;
        unsigned int green = ((src[3 * index + 1] * greenScale) >> 8) & s_greenMask;
        unsigned int blue = ((src[3 * index + 2] * blueScale) >> 8) & s_blueMask;
        *dst = static_cast<unsigned short>(red | green | blue);
        ++dst;
    }
}

// Original: TPalette16::TPalette16; palette.cpp:116, dc 0x10a5e0.
TPalette16::TPalette16(const TRGBA* rgba)
    : resource(0, RESOURCE_TYPE_NONE)
{
    const unsigned int redScale = (s_redMask + s_redMask) & ~s_redMask;
    const unsigned int greenScale = (s_greenMask + s_greenMask) & ~s_greenMask;
    const unsigned int blueScale = (s_blueMask + s_blueMask) & ~s_blueMask;
    unsigned short* destination = m_data;
    for (int index = 0; index < 256; ++index) {
        unsigned int red = ((rgba[index].m_red * redScale) >> 8) & s_redMask;
        unsigned int green = ((rgba[index].m_green * greenScale) >> 8) & s_greenMask;
        unsigned int blue = ((rgba[index].m_blue * blueScale) >> 8) & s_blueMask;
        *destination = static_cast<unsigned short>(red | green | blue);
        ++destination;
    }
}

// Original: TPalette16::TPalette16; palette.cpp:140, dc 0x10a6a4.
TPalette16::TPalette16(const tagRGBQUAD* quad)
    : resource(0, RESOURCE_TYPE_NONE)
{
    const unsigned int redScale = (s_redMask + s_redMask) & ~s_redMask;
    const unsigned int greenScale = (s_greenMask + s_greenMask) & ~s_greenMask;
    const unsigned int blueScale = (s_blueMask + s_blueMask) & ~s_blueMask;
    unsigned short* destination = m_data;
    for (int index = 0; index < 256; ++index) {
        unsigned int red = ((quad[index].rgbRed * redScale) >> 8) & s_redMask;
        unsigned int green = ((quad[index].rgbGreen * greenScale) >> 8) & s_greenMask;
        unsigned int blue = ((quad[index].rgbBlue * blueScale) >> 8) & s_blueMask;
        *destination = static_cast<unsigned short>(red | green | blue);
        ++destination;
    }
}

// Original: TPalette16::TPalette16; palette.cpp:165, dc 0x10a77c.
TPalette16::TPalette16(const char* name, const TPalette24& p24)
    : resource(name, RESOURCE_TYPE_PALETTE)
{
    const unsigned int redScale = (s_redMask + s_redMask) & ~s_redMask;
    const unsigned int greenScale = (s_greenMask + s_greenMask) & ~s_greenMask;
    const unsigned int blueScale = (s_blueMask + s_blueMask) & ~s_blueMask;
    unsigned short* destination = m_data;
    for (int index = 0; index < 256; ++index) {
        unsigned int red = ((p24.m_palette[3 * index] * redScale) >> 8) & s_redMask;
        unsigned int green = ((p24.m_palette[3 * index + 1] * greenScale) >> 8) & s_greenMask;
        unsigned int blue = ((p24.m_palette[3 * index + 2] * blueScale) >> 8) & s_blueMask;
        *destination = static_cast<unsigned short>(red | green | blue);
        ++destination;
    }
}

// The pointer-taking copy constructor, and the payload-only assignment behind
// it - both the TPalette24 shapes with the 0x200-byte table in place of the
// 0x300-byte one, and the assignment keeps the resource identity.
VA(0x005228e0, 0x30) MAC_ADDRESS(0x13ba10, 0x68)  // dc 0x10a854
TPalette16::TPalette16(const TPalette16* copy)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_data, copy->m_data, sizeof(m_data));
}

VA(0x00522910, 0x21) MAC_ADDRESS(0x13ba78, 0x48)  // dc 0x10a8a0
TPalette16* TPalette16::operator=(const TPalette16* from)
{
    if (this != from)
        memcpy(m_data, from->m_data, sizeof(m_data));
    return this;
}

VA(0x00522940, 0xB) MAC_ADDRESS(0x13bac0, 0x60)  // dc 0x10a8e0
TPalette16::~TPalette16()
{
}

// DC palette.cpp:210 places the ordinary Convert24to16 body after the
// destructor and before Cycle; its earlier constructors call this helper.
// Preserve that source order while matching the retail inline expansions.
// DC lines 211/224-226/228-229 retain the p16 destination pointer, indexed
// RGB reads, and three independent extu.w truncations before the final OR.
// The 72-state family distinguishes actual channel-value lifetimes from casts:
// named ushort channels give both callers 100%; inline casts alone leave
// 94.8548/94.9365. The destination-pointer form preserves the recorded p16.
MAC_ADDRESS(0x13bb20, 0xa0)
void TPalette16::convert24to16(const unsigned char* p24, int rbits, int rshift,
                               int gbits, int gshift, int bbits, int bshift)
{
    unsigned short* destination = m_data;
    for (int index = 0; index < 256; ++index) {
        unsigned short red = static_cast<unsigned short>(
            (p24[3 * index] >> (8 - rbits)) << rshift);
        unsigned short green = static_cast<unsigned short>(
            (p24[3 * index + 1] >> (8 - gbits)) << gshift);
        unsigned short blue = static_cast<unsigned short>(
            (p24[3 * index + 2] >> (8 - bbits)) << bshift);
        *destination = static_cast<unsigned short>(red | green | blue);
        ++destination;
    }
}

// Original: TPalette16::ConvertRGBAto16; palette.cpp:236, dc 0x10a998.
void TPalette16::convertRGBAto16(const TRGBA* rgba, int rbits, int rshift,
    int gbits, int gshift, int bbits, int bshift)
{
    unsigned short* destination = m_data;
    for (int index = 0; index < 256; ++index) {
        unsigned short red = static_cast<unsigned short>(
            (rgba[index].m_red >> (8 - rbits)) << rshift);
        unsigned short green = static_cast<unsigned short>(
            (rgba[index].m_green >> (8 - gbits)) << gshift);
        unsigned short blue = static_cast<unsigned short>(
            (rgba[index].m_blue >> (8 - bbits)) << bshift);
        *destination = static_cast<unsigned short>(red | green | blue);
        ++destination;
    }
}

// Original: TPalette16::ConvertRGBQUADto16; palette.cpp:262, dc 0x10aa18.
void TPalette16::convertRGBQUADto16(const tagRGBQUAD* quad, int rbits, int rshift,
    int gbits, int gshift, int bbits, int bshift)
{
    unsigned short* destination = m_data;
    for (int index = 0; index < 256; ++index) {
        unsigned short red = static_cast<unsigned short>(
            (quad[index].rgbRed >> (8 - rbits)) << rshift);
        unsigned short green = static_cast<unsigned short>(
            (quad[index].rgbGreen >> (8 - gbits)) << gshift);
        unsigned short blue = static_cast<unsigned short>(
            (quad[index].rgbBlue >> (8 - bbits)) << bshift);
        *destination = static_cast<unsigned short>(red | green | blue);
        ++destination;
    }
}

VA(0x00522950, 0xBE) MAC_ADDRESS(0x13bbc0, 0xe0)  // dc 0x10aa98
void TPalette16::cycle(int begin, int end, int step)
{
    if (step > 0) {
        for (int i = 0; i < step; ++i) {
            unsigned short saved = m_data[begin];
            memmove(&m_data[begin], &m_data[begin + 1],
                    (end - begin) * sizeof(m_data[0]));
            m_data[end] = saved;
        }
    } else {
        for (int i = 0; i < -step; ++i) {
            unsigned short saved = m_data[end];
            memmove(&m_data[begin + 1], &m_data[begin],
                    (end - begin) * sizeof(m_data[0]));
            m_data[begin] = saved;
        }
    }
}

// Original: TPalette16::Colorize; palette.cpp:315, dc 0x10ab44.
void TPalette16::colorize(float hue, float saturation)
{
    const unsigned int redNorm = std::numeric_limits<int>::max() / s_redMask;
    const unsigned int greenNorm = std::numeric_limits<int>::max() / s_greenMask;
    const unsigned int blueNorm = std::numeric_limits<int>::max() / s_blueMask;
    for (int i = 10; i < 256; ++i) {
        unsigned int r = (m_data[i] & s_redMask) * redNorm;
        unsigned int g = (m_data[i] & s_greenMask) * greenNorm;
        unsigned int b = (m_data[i] & s_blueMask) * blueNorm;
        const float value = static_cast<float>((r > g ? r : g) > b ? (r > g ? r : g) : b);
        const float f = static_cast<float>(fmod(hue * 6.0f, 1.0));
        const float p = value * (1.0f - saturation);
        const float q = value * (1.0f - saturation * f);
        const float t = value * (1.0f - saturation * (1.0f - f));
        switch (static_cast<int>(hue * 6.0f)) {
        case HSV_RED_SECTOR:     r = ftol(value); g = ftol(t); b = ftol(p); break;
        case HSV_YELLOW_SECTOR:  r = ftol(q); g = ftol(value); b = ftol(p); break;
        case HSV_GREEN_SECTOR:   r = ftol(p); g = ftol(value); b = ftol(t); break;
        case HSV_CYAN_SECTOR:    r = ftol(p); g = ftol(q); b = ftol(value); break;
        case HSV_BLUE_SECTOR:    r = ftol(t); g = ftol(p); b = ftol(value); break;
        case HSV_MAGENTA_SECTOR: r = ftol(value); g = ftol(p); b = ftol(q); break;
        }
        m_data[i] = static_cast<unsigned short>(
            ((r / redNorm) & s_redMask) |
            ((g / greenNorm) & s_greenMask) |
            ((b / blueNorm) & s_blueMask));
    }
}

// Original: TPalette16::AdjustHue; palette.cpp:360, dc 0x10af5c.
void TPalette16::adjustHue(float hue, float amount)
{
    const unsigned int redNorm = std::numeric_limits<int>::max() / s_redMask;
    const unsigned int greenNorm = std::numeric_limits<int>::max() / s_greenMask;
    const unsigned int blueNorm = std::numeric_limits<int>::max() / s_blueMask;
    for (int i = 10; i < 256; ++i) {
        unsigned int r = (m_data[i] & s_redMask) * redNorm;
        unsigned int g = (m_data[i] & s_greenMask) * greenNorm;
        unsigned int b = (m_data[i] & s_blueMask) * blueNorm;
        float h;
        float s;
        float v;
        rgbToHSV(r, g, b, &h, &s, &v);
        float delta = hue - h;
        h += delta * amount;
        if (fabs(delta) > 0.5) {
            if (delta > 0.0)
                h += 1.0f - amount;
            else
                h += amount;
            if (h >= 1.0)
                h -= 1.0;
        }
        hsvToRGB(h, s, v, &r, &g, &b);
        m_data[i] = static_cast<unsigned short>(
            ((r / redNorm) & s_redMask) |
            ((g / greenNorm) & s_greenMask) |
            ((b / blueNorm) & s_blueMask));
    }
}

VA(0x00522a10, 0x122) MAC_ADDRESS(0x13bca0, 0x160)  // dc 0x10b1ec
void TPalette16::adjustSaturation(float amount)
{
    const unsigned int redNorm =
        std::numeric_limits<int>::max() / s_redMask;
    const unsigned int greenNorm =
        std::numeric_limits<int>::max() / s_greenMask;
    const unsigned int blueNorm =
        std::numeric_limits<int>::max() / s_blueMask;

    for (int i = 10; i < 256; ++i) {
        unsigned int r = (m_data[i] & s_redMask) * redNorm;
        unsigned int g = (m_data[i] & s_greenMask) * greenNorm;
        unsigned int b = (m_data[i] & s_blueMask) * blueNorm;

        float h;
        float s;
        float v;
        rgbToHSV(r, g, b, &h, &s, &v);

        if (amount <= 1.0f) {
            s *= amount;
        } else {
            s = 1.0f - (1.0f - s) / amount;
        }

        hsvToRGB(h, s, v, &r, &g, &b);

        m_data[i] = static_cast<unsigned short>(
            ((r / redNorm) & s_redMask) |
            ((g / greenNorm) & s_greenMask) |
            ((b / blueNorm) & s_blueMask));
    }
}

#if 0  // @carcass

// E:\gamedcs\palette.cpp:288
// Retail body reconstructed above at 0x00522950; dc 0x10aa98.
void TPalette16::cycle(int begin, int end, int step)
{
    // @stub
}

// E:\gamedcs\palette.cpp:412
// Retail body reconstructed above at 0x00522a10; dc 0x10b1ec.
void TPalette16::adjustSaturation(float amount)
{
    // @stub
}

#endif  // @carcass

VA(0x00522b40, 0x6) MAC_ADDRESS(0x13be00, 0x8)  // TPalette16 vtable 0x640368 slot 2
unsigned int TPalette16::getSize() const
{
    return sizeof(*this);
}

// Original: TPalette16::AdjustValue; palette.cpp:454, dc 0x10b320.
void TPalette16::adjustValue(float amount)
{
    const unsigned int redNorm = std::numeric_limits<int>::max() / s_redMask;
    const unsigned int greenNorm = std::numeric_limits<int>::max() / s_greenMask;
    const unsigned int blueNorm = std::numeric_limits<int>::max() / s_blueMask;
    for (int i = 10; i < 256; ++i) {
        unsigned int r = (m_data[i] & s_redMask) * redNorm;
        unsigned int g = (m_data[i] & s_greenMask) * greenNorm;
        unsigned int b = (m_data[i] & s_blueMask) * blueNorm;
        float h;
        float s;
        float v;
        rgbToHSV(r, g, b, &h, &s, &v);
        if (amount <= 1.0f)
            v *= amount;
        else
            v = 1.0f - (1.0f - v) / amount;
        hsvToRGB(h, s, v, &r, &g, &b);
        m_data[i] = static_cast<unsigned short>(
            ((r / redNorm) & s_redMask) |
            ((g / greenNorm) & s_greenMask) |
            ((b / blueNorm) & s_blueMask));
    }
}

VA(0x00522b50, 0x1F5) MAC_ADDRESS(0x13be08, 0x274)  // dc 0x10b484
void TPalette16::adjustHSV(float hue, float hueAdjust,
                           float saturationAdjust, float valueAdjust)
{
    const unsigned int redNorm =
        std::numeric_limits<int>::max() / s_redMask;
    const unsigned int greenNorm =
        std::numeric_limits<int>::max() / s_greenMask;
    const unsigned int blueNorm =
        std::numeric_limits<int>::max() / s_blueMask;

    for (int i = 10; i < 256; ++i) {
        unsigned int r = (m_data[i] & s_redMask) * redNorm;
        unsigned int g = (m_data[i] & s_greenMask) * greenNorm;
        unsigned int b = (m_data[i] & s_blueMask) * blueNorm;

        float h;
        float s;
        float v;
        rgbToHSV(r, g, b, &h, &s, &v);

        if (hueAdjust >= 0.0f) {
            float delta = hue - h;
            h += delta * hueAdjust;
            if (fabs(delta) > 0.5) {
                if (delta > 0.0) {
                    h += 1.0f - hueAdjust;
                } else {
                    h += hueAdjust;
                }
                if (h >= 1.0) {
                    h -= 1.0;
                }
            }
        }

        if (saturationAdjust >= 0.0f) {
            if (saturationAdjust <= 1.0f) {
                s *= saturationAdjust;
            } else {
                s = 1.0f - (1.0f - s) / saturationAdjust;
            }
        }

        if (valueAdjust >= 0.0) {
            if (valueAdjust <= 1.0f) {
                v *= valueAdjust;
            } else {
                v = 1.0f - (1.0f - v) / valueAdjust;
            }
        }

        hsvToRGB(h, s, v, &r, &g, &b);

        m_data[i] = static_cast<unsigned short>(
            ((r / redNorm) & s_redMask) |
            ((g / greenNorm) & s_greenMask) |
            ((b / blueNorm) & s_blueMask));
    }
}

VA(0x00522d50, 0xD6) MAC_ADDRESS(0x13c07c, 0x170)  // dc 0x10b7ac
void TPalette16::gray()
{
    const unsigned int redNorm =
        std::numeric_limits<int>::max() / s_redMask;
    const unsigned int greenNorm =
        std::numeric_limits<int>::max() / s_greenMask;
    const unsigned int blueNorm =
        std::numeric_limits<int>::max() / s_blueMask;

#define max(a, b) ((a) > (b) ? (a) : (b))
    for (int i = 10; i < 256; ++i) {
        unsigned int red = (m_data[i] & s_redMask) * redNorm;
        unsigned int green = (m_data[i] & s_greenMask) * greenNorm;
        unsigned int blue = (m_data[i] & s_blueMask) * blueNorm;

        unsigned int gray = max(max(red, green), blue);

        m_data[i] = static_cast<unsigned short>(
            ((gray / redNorm) & s_redMask) |
            ((gray / greenNorm) & s_greenMask) |
            ((gray / blueNorm) & s_blueMask));
    }
}
#undef max

VA(0x00522e30, 0x16) MAC_ADDRESS(0x13c1ec, 0x40)  // null-name resource ctor + TPalette24 vtable
TPalette24::TPalette24()
    : resource(0, RESOURCE_TYPE_NONE)
{
}

VA_COMPGEN(0x00522e50, 0x21, SCALAR_DELETING_DTOR, TPalette24)

VA(0x00522e80, 0x2D) MAC_ADDRESS(0x13c22c, 0x68)  // dc 0x10b904
TPalette24::TPalette24(const unsigned char* data)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_palette, data, sizeof(m_palette));
}

VA(0x00522eb0, 0x42) MAC_ADDRESS(0x13c294, 0x124)  // dc 0x10b94c
TPalette24::TPalette24(const TRGBA* rgba)
    : resource(0, RESOURCE_TYPE_NONE)
{
    for (int index = 0; index < 256; ++index) {
        m_palette[3 * index + 0] = rgba->m_red;
        m_palette[3 * index + 1] = rgba->m_green;
        m_palette[3 * index + 2] = rgba->m_blue;
        ++rgba;
    }
}

// Original: TPalette24::TPalette24; palette.cpp:622, dc 0x10b9c4.
TPalette24::TPalette24(const tagRGBQUAD* quad)
    : resource(0, RESOURCE_TYPE_NONE)
{
    for (int index = 0; index < 256; ++index) {
        m_palette[3 * index] = quad->rgbRed;
        m_palette[3 * index + 1] = quad->rgbGreen;
        m_palette[3 * index + 2] = quad->rgbBlue;
        ++quad;
    }
}

VA(0x00522f00, 0x30) MAC_ADDRESS(0x13c3b8, 0x68)  // dc 0x10ba3c
TPalette24::TPalette24(const TPalette24* copy)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_palette, copy->m_palette, sizeof(m_palette));
}

VA(0x00522f30, 0x21) MAC_ADDRESS(0x13c420, 0x48)  // payload-only assignment; resource identity retained
TPalette24& TPalette24::operator=(const TPalette24& from)
{
    if (this != &from)
        memcpy(m_palette, from.m_palette, sizeof(m_palette));
    return *this;
}

VA(0x00522f60, 0x0b) MAC_ADDRESS(0x13c468, 0x60)  // TPalette24 vtable 0x640374 + resource dtor tail
TPalette24::~TPalette24() throw()
{
}

VA(0x00522f70, 0x06) MAC_ADDRESS(0x13c4c8, 0x8)  // TPalette24 vtable 0x640374 slot 2
unsigned int TPalette24::getSize() const
{
    return sizeof(*this);
}

// Original: TPalette24::Cycle; palette.cpp:655, dc 0x10baf0.
void TPalette24::cycle(int begin, int end, int step)
{
    begin *= 3;
    end *= 3;
    if (step > 0) {
        for (int i = step; i > 0; --i) {
            int r = m_palette[begin];
            int g = m_palette[begin + 1];
            int b = m_palette[begin + 2];
            memmove(m_palette + begin, m_palette + begin + 3, end - begin);
            m_palette[end] = static_cast<unsigned char>(r);
            m_palette[end + 1] = static_cast<unsigned char>(g);
            m_palette[end + 2] = static_cast<unsigned char>(b);
        }
    } else {
        for (int i = -step; i > 0; --i) {
            int r = m_palette[end];
            int g = m_palette[end + 1];
            int b = m_palette[end + 2];
            memmove(m_palette + begin + 3, m_palette + begin, end - begin);
            m_palette[begin] = static_cast<unsigned char>(r);
            m_palette[begin + 1] = static_cast<unsigned char>(g);
            m_palette[begin + 2] = static_cast<unsigned char>(b);
        }
    }
}

// Original: TPalette24::Colorize; palette.cpp:685, dc 0x10bbf4.
void TPalette24::colorize(float hue, float saturation)
{
    for (int i = 10; i < 256; ++i) {
        unsigned int r = m_palette[3 * i];
        unsigned int g = m_palette[3 * i + 1];
        unsigned int b = m_palette[3 * i + 2];
        const float value = static_cast<float>((r > g ? r : g) > b ? (r > g ? r : g) : b);
        const int hextant = static_cast<int>(hue * 6.0f);
        const float f = static_cast<float>(fmod(hue * 6.0f, 1.0));
        const float p = value * (1.0f - saturation);
        const float q = value * (1.0f - saturation * f);
        const float t = value * (1.0f - saturation * (1.0f - f));
        switch (hextant) {
        case HSV_RED_SECTOR:     r = ftol(value); g = ftol(t); b = ftol(p); break;
        case HSV_YELLOW_SECTOR:  r = ftol(q); g = ftol(value); b = ftol(p); break;
        case HSV_GREEN_SECTOR:   r = ftol(p); g = ftol(value); b = ftol(t); break;
        case HSV_CYAN_SECTOR:    r = ftol(p); g = ftol(q); b = ftol(value); break;
        case HSV_BLUE_SECTOR:    r = ftol(t); g = ftol(p); b = ftol(value); break;
        case HSV_MAGENTA_SECTOR: r = ftol(value); g = ftol(p); b = ftol(q); break;
        }
        m_palette[3 * i] = static_cast<unsigned char>(r);
        m_palette[3 * i + 1] = static_cast<unsigned char>(g);
        m_palette[3 * i + 2] = static_cast<unsigned char>(b);
    }
}

// Original: TPalette24::Gray; palette.cpp:723, dc 0x10bf58.
void TPalette24::gray()
{
    for (int i = 10; i < 256; ++i) {
        unsigned int r = m_palette[3 * i];
        unsigned int g = m_palette[3 * i + 1];
        unsigned int b = m_palette[3 * i + 2];
        unsigned int gray = (r > g ? r : g) > b ? (r > g ? r : g) : b;
        m_palette[3 * i] = static_cast<unsigned char>(gray);
        m_palette[3 * i + 1] = static_cast<unsigned char>(gray);
        m_palette[3 * i + 2] = static_cast<unsigned char>(gray);
    }
}

VA(0x00522f80, 0x20E) MAC_ADDRESS(0x13c4d0, 0x2c4)  // dc 0x10bfd4
void TPalette24::adjustHSV(float hue, float hueAdjust,
                           float saturationAdjust, float valueAdjust)
{
    const unsigned int redNorm =
        std::numeric_limits<int>::max() / 255;
    const unsigned int greenNorm =
        std::numeric_limits<int>::max() / 255;
    const unsigned int blueNorm =
        std::numeric_limits<int>::max() / 255;

    for (int i = 10; i < 256; ++i) {
        unsigned int r = m_palette[3 * i + 0] * redNorm;
        unsigned int g = m_palette[3 * i + 1] * greenNorm;
        unsigned int b = m_palette[3 * i + 2] * blueNorm;

        float h;
        float s;
        float v;
        rgbToHSV(r, g, b, &h, &s, &v);

        if (hueAdjust >= 0.0f) {
            float delta = hue - h;
            h += delta * hueAdjust;
            if (fabs(delta) > 0.5) {
                if (delta > 0.0) {
                    h += 1.0f - hueAdjust;
                } else {
                    h += hueAdjust;
                }
                if (h >= 1.0) {
                    h -= 1.0;
                }
            }
        }

        if (valueAdjust >= 0.0) {
            if (valueAdjust <= 1.0f) {
                v *= valueAdjust;
            } else {
                v = 1.0f - (1.0f - v) / valueAdjust;
            }
        }

        if (saturationAdjust >= 0.0f) {
            if (saturationAdjust <= 1.0f) {
                s *= saturationAdjust;
            } else if (v > 0.75 && s < 0.25) {
                s = (1.0f - v) * s * saturationAdjust * 4.0f;
            } else {
                s = 1.0f - (1.0f - s) / saturationAdjust;
            }
        }

        hsvToRGB(h, s, v, &r, &g, &b);

        m_palette[3 * i + 0] = static_cast<unsigned char>(r / redNorm);
        m_palette[3 * i + 1] = static_cast<unsigned char>(g / greenNorm);
        m_palette[3 * i + 2] = static_cast<unsigned char>(b / blueNorm);
    }
}

// Original: RGBToHSV; palette.cpp:827, dc 0x10c370.
VA(0x00523190, 0x160) MAC_ADDRESS(0x13c794, 0x1c8)  // dc 0x10c370
void rgbToHSV(unsigned int r, unsigned int g, unsigned int b,
              float* h, float* s, float* v)
{
    static const float redHue = 0.0f;

    const unsigned int max =
        (r > g ? r : g) > b ? (r > g ? r : g) : b;
    const unsigned int min =
        (r < g ? r : g) < b ? (r < g ? r : g) : b;

    *v = static_cast<float>(max) / std::numeric_limits<int>::max();

    *s = max ? static_cast<float>(max - min) / static_cast<float>(max)
             : 0.0f;

    if (max - min) {
        float rc = static_cast<float>(max - r) /
                   static_cast<float>(max - min);
        float gc = static_cast<float>(max - g) /
                   static_cast<float>(max - min);
        float bc = static_cast<float>(max - b) /
                   static_cast<float>(max - min);

        if (r == max) {
            *h = (bc - gc) / 6.0f + redHue;
        } else if (g == max) {
            *h = (rc - bc) / 6.0f + 1.0f / 3.0f;
        } else {
            *h = (gc - rc) / 6.0f + 2.0f / 3.0f;
        }

        if (*h < 0.0f) {
            *h += 1.0f;
        }
    } else {
        *h = 0.0f;
    }
}

// Original: HSVToRGB; palette.cpp:862, dc 0x10c564.
VA(0x005232f0, 0x2EC) MAC_ADDRESS(0x13c95c, 0x248)  // dc 0x10c564
void hsvToRGB(float h, float s, float v,
              unsigned int* r, unsigned int* g, unsigned int* b)
{
    if (s != 0.0f) {
        const float f = static_cast<float>(fmod(h * 6.0f, 1.0));

        v *= static_cast<float>(std::numeric_limits<int>::max());
        const float p = v * (1.0f - s);
        const float q = v * (1.0f - s * f);
        const float t = v * (1.0f - s * (1.0f - f));

        switch (static_cast<int>(h * 6.0f)) {
        case HSV_RED_SECTOR:
            *r = ftol(v);
            *g = ftol(t);
            *b = ftol(p);
            break;
        case HSV_YELLOW_SECTOR:
            *r = ftol(q);
            *g = ftol(v);
            *b = ftol(p);
            break;
        case HSV_GREEN_SECTOR:
            *r = ftol(p);
            *g = ftol(v);
            *b = ftol(t);
            break;
        case HSV_CYAN_SECTOR:
            *r = ftol(p);
            *g = ftol(q);
            *b = ftol(v);
            break;
        case HSV_BLUE_SECTOR:
            *r = ftol(t);
            *g = ftol(p);
            *b = ftol(v);
            break;
        case HSV_MAGENTA_SECTOR:
            *r = ftol(v);
            *g = ftol(p);
            *b = ftol(q);
            break;
        }
    } else {
        *r = *g = *b = ftol(
            v * static_cast<float>(std::numeric_limits<int>::max()));
    }
}

#if 0  // @carcass

// E:\gamedcs\palette.cpp:496
// Retail body reconstructed above at 0x00522b50; dc 0x10b484.
void TPalette16::adjustHSV(float hue, float hue_adjust, float saturation_adjust, float value_adjust)
{
    // @stub
}

// E:\gamedcs\palette.cpp:571
// Retail body reconstructed above at 0x00522d50; dc 0x10b7ac.
void TPalette16::gray()
{
    // @stub
}

// E:\gamedcs\palette.cpp:598
// Retail body reconstructed above; dc 0x10b898.
void TPalette24::TPalette24()
{
    // @stub
}

// E:\gamedcs\palette.cpp:603
// Retail body reconstructed above at 0x00522e80; dc 0x10b904.
void TPalette24::TPalette24(const unsigned char* data)
{
    // @stub
}

// E:\gamedcs\palette.cpp:609
// Retail body reconstructed above at 0x00522eb0; dc 0x10b94c.
void TPalette24::TPalette24(const TRGBA* rgba)
{
    // @stub
}

// E:\gamedcs\palette.cpp:635
// RETAIL_LOCATED(0x00522f00, 0x30): not reconstructed; dc-bracket forced, dc 0x10ba3c
void TPalette24::TPalette24(const TPalette24* copy)
{
    // @stub
}

// E:\gamedcs\palette.cpp:640
// RETAIL_LOCATED(0x00522f30, 0x21): not reconstructed; dc-bracket forced, dc 0x10ba88
TPalette24* TPalette24::operator=(const TPalette24* from)
{
    // @stub
}

// E:\gamedcs\palette.cpp:650
// Retail body reconstructed above; dc 0x10baac.
void TPalette24::~TPalette24()
{
    // @stub
}

// E:\gamedcs\palette.cpp:740
// Retail body reconstructed above at 0x00522f80; dc 0x10bfd4.
void TPalette24::adjustHSV(float hue, float hue_adjust, float saturation_adjust, float value_adjust)
{
    // @stub
}

// E:\gamedcs\palette.cpp:827
// Retail body reconstructed above at 0x00523190; dc 0x10c370.
void rgbToHSV(unsigned r, unsigned g, unsigned b, float* h, float* s, float* v)
{
    // @stub
}

// E:\gamedcs\palette.cpp:862
// Retail body reconstructed above at 0x005232f0; dc 0x10c564.
void hsvToRGB(float h, float s, float v, unsigned* r, unsigned* g, unsigned* b)
{
    // @stub
}

#endif  // @carcass
