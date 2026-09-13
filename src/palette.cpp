// palette.cpp - E:\gamedcs\palette.cpp (compiland palette.obj)
// 39 functions in link order.
#include <va.h>
#include <limits>
#include <math.h>
#include <string.h>
#include "palette.h"

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
static __forceinline long ftol(double d)
{
    const unsigned long magic = 0x59c00000;
    TFloatLongBits magicValue;
    TDoubleLongBits result;
    result.m_value = d;
    magicValue.m_bits = magic;
    result.m_value += magicValue.m_value;
    return result.m_words[0];
}

#if 0  // @carcass

// E:\gamedcs\palette.cpp:45
DC_ONLY(0x10a244, 0x62)
long ftol(double d)
{
    // @stub
}

// E:\gamedcs\palette.cpp:73
DC_ONLY(0x10a3ac, 0x6E)
void TPalette16::TPalette16(const TRGBA* rgba, int rbits, int rshift, int gbits, int gshift, int bbits, int bshift)
{
    // @stub
}

// E:\gamedcs\palette.cpp:79
DC_ONLY(0x10a41c, 0x7C)
void TPalette16::TPalette16(const tagRGBQUAD* quad, int rbits, int rshift, int gbits, int gshift, int bbits, int bshift)
{
    // @stub
}

// E:\gamedcs\palette.cpp:116
DC_ONLY(0x10a5e0, 0xC2)
void TPalette16::TPalette16(const TRGBA* rgba)
{
    // @stub
}

// E:\gamedcs\palette.cpp:140
DC_ONLY(0x10a6a4, 0xD8)
void TPalette16::TPalette16(const tagRGBQUAD* quad)
{
    // @stub
}

// E:\gamedcs\palette.cpp:165
DC_ONLY(0x10a77c, 0xD6)
void TPalette16::TPalette16(const char* name, const TPalette24& p24)
{
    // @stub
}

// E:\gamedcs\palette.cpp:204
#endif  // @carcass

VA(0x00522650, 0x16)  // dc 0x10a2a8
TPalette16::TPalette16()
    : resource(0, RESOURCE_TYPE_NONE)
{
}

VA_COMPGEN(0x00522670, 0x21, SCALAR_DELETING_DTOR, TPalette16)

// The raw 16-bit table overload: 0x80 dwords straight into the payload at
// +0x1c, the same shape TPalette24's raw-data constructor has at 0x522e80.
VA(0x005226a0, 0x2D)  // dc 0x10a2f0
TPalette16::TPalette16(const unsigned short* newData)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_data, newData, sizeof(m_data));
}

VA(0x005226d0, 0x9D)  // dc 0x10a338
TPalette16::TPalette16(const TPalette24& p24, int rbits, int rshift,
                       int gbits, int gshift, int bbits, int bshift)
    : resource(0, RESOURCE_TYPE_NONE)
{
    convert24to16(p24.m_palette, rbits, rshift, gbits, gshift,
                  bbits, bshift);
}

VA(0x00522770, 0x9F)  // dc 0x10a498
TPalette16::TPalette16(const char* name, const TPalette24& p24,
                       int rbits, int rshift, int gbits, int gshift,
                       int bbits, int bshift)
    : resource(name, RESOURCE_TYPE_PALETTE)
{
    convert24to16(p24.m_palette, rbits, rshift, gbits, gshift,
                  bbits, bshift);
}

VA(0x00522810, 0xC6)  // dc 0x10a508
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

// The pointer-taking copy constructor, and the payload-only assignment behind
// it - both the TPalette24 shapes with the 0x200-byte table in place of the
// 0x300-byte one, and the assignment keeps the resource identity.
VA(0x005228e0, 0x30)  // dc 0x10a854
TPalette16::TPalette16(const TPalette16* copy)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_data, copy->m_data, sizeof(m_data));
}

VA(0x00522910, 0x21)  // dc 0x10a8a0
TPalette16* TPalette16::operator=(const TPalette16* from)
{
    if (this != from)
        memcpy(m_data, from->m_data, sizeof(m_data));
    return this;
}

VA(0x00522940, 0xB)  // dc 0x10a8e0
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

VA(0x00522950, 0xBE)  // dc 0x10aa98
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

VA(0x00522a10, 0x122)  // dc 0x10b1ec
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

// E:\gamedcs\palette.cpp:236
DC_ONLY(0x10a998, 0x7E)
void TPalette16::ConvertRGBAto16(const TRGBA* rgba, int rbits, int rshift, int gbits, int gshift, int bbits, int bshift)
{
    // @stub
}

// E:\gamedcs\palette.cpp:262
DC_ONLY(0x10aa18, 0x7E)
void TPalette16::ConvertRGBQUADto16(const tagRGBQUAD* quad, int rbits, int rshift, int gbits, int gshift, int bbits, int bshift)
{
    // @stub
}

// E:\gamedcs\palette.cpp:288
// Retail body reconstructed above at 0x00522950; dc 0x10aa98.
void TPalette16::cycle(int begin, int end, int step)
{
    // @stub
}

// E:\gamedcs\palette.cpp:315
DC_ONLY(0x10ab44, 0x416)
void TPalette16::colorize(float hue, float saturation)
{
    // @stub
}

// E:\gamedcs\palette.cpp:360
DC_ONLY(0x10af5c, 0x28E)
void TPalette16::AdjustHue(float hue, float amount)
{
    // @stub
}

// E:\gamedcs\palette.cpp:412
// Retail body reconstructed above at 0x00522a10; dc 0x10b1ec.
void TPalette16::adjustSaturation(float amount)
{
    // @stub
}

// E:\gamedcs\palette.cpp:454
// No retail body: this Dreamcast-only AdjustValue row is replaced at the
// corresponding x86 position by TPalette16's resource-size vtable slot.
DC_ONLY(0x10b320, 0x164)
void TPalette16::AdjustValue(float amount)
{
    // @stub
}

#endif  // @carcass

VA(0x00522b40, 0x6)  // TPalette16 vtable 0x640368 slot 2
unsigned int TPalette16::getSize() const
{
    return sizeof(*this);
}

VA(0x00522b50, 0x1F5)  // dc 0x10b484
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

VA(0x00522d50, 0xD6)  // dc 0x10b7ac
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

VA(0x00522e30, 0x16)  // null-name resource ctor + TPalette24 vtable
TPalette24::TPalette24()
    : resource(0, RESOURCE_TYPE_NONE)
{
}

VA_COMPGEN(0x00522e50, 0x21, SCALAR_DELETING_DTOR, TPalette24)

VA(0x00522e80, 0x2D)  // dc 0x10b904
TPalette24::TPalette24(const unsigned char* data)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_palette, data, sizeof(m_palette));
}

VA(0x00522eb0, 0x42)  // dc 0x10b94c
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

VA(0x00522f00, 0x30)  // dc 0x10ba3c
TPalette24::TPalette24(const TPalette24* copy)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_palette, copy->m_palette, sizeof(m_palette));
}

VA(0x00522f30, 0x21)  // payload-only assignment; resource identity retained
TPalette24& TPalette24::operator=(const TPalette24& from)
{
    if (this != &from)
        memcpy(m_palette, from.m_palette, sizeof(m_palette));
    return *this;
}

VA(0x00522f60, 0x0b)  // TPalette24 vtable 0x640374 + resource dtor tail
TPalette24::~TPalette24() throw()
{
}

VA(0x00522f70, 0x06)  // TPalette24 vtable 0x640374 slot 2
unsigned int TPalette24::getSize() const
{
    return sizeof(*this);
}

VA(0x00522f80, 0x20E)  // dc 0x10bfd4
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

VA(0x00523190, 0x160)  // dc 0x10c370
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

VA(0x005232f0, 0x2EC)  // dc 0x10c564
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

// E:\gamedcs\palette.cpp:622
// RETAIL_LOCATED(0x00522eb0, 0x42): not reconstructed; dc-bracket forced, dc 0x10b9c4
void TPalette24::TPalette24(const tagRGBQUAD* quad)
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

// E:\gamedcs\palette.cpp:655
DC_ONLY(0x10baf0, 0x102)
void TPalette24::cycle(int begin, int end, int step)
{
    // @stub
}

// E:\gamedcs\palette.cpp:685
DC_ONLY(0x10bbf4, 0x364)
void TPalette24::colorize(float hue, float saturation)
{
    // @stub
}

// E:\gamedcs\palette.cpp:723
DC_ONLY(0x10bf58, 0x7A)
void TPalette24::gray()
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

// E:\gamedcs\palette.cpp:57
DC_ONLY(0x10c8b0, 0x34)
void* TPalette16::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\palette.cpp:599
DC_ONLY(0x10c8e4, 0x34)
void* TPalette24::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass
