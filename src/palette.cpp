// Palette.cpp - TPalette16 and TPalette24 (Loki h3maped object 48).
//
// The Loki object has no __FILE__ string; its name follows the classes.
// Functions are defined in the Loki link order: the TPalette16 members,
// the TPalette24 members and the file-static RGBToHSV/HSVToRGB pair; g++
// then emits the inline TPalette16::SetPixelFormat and the inline ftol.
#include "va.h"

#include <limits>
#include <math.h>
#include <string.h>

#include "palette.h"

// The Loki port's own RGBQUAD: the Windows SDK's layout and tag, which the
// TPalette16/TPalette24 constructors' mangled names keep.
struct tagRGBQUAD {
    unsigned char rgbBlue;
    unsigned char rgbGreen;
    unsigned char rgbRed;
    unsigned char rgbReserved;
};

static void RGBToHSV(unsigned int r, unsigned int g, unsigned int b,
                     float* h, float* s, float* v);

// The hues of red, green and blue as fractions of the colour wheel. Loki's
// object keeps the three constants at the end of its .rodata, after the
// type names, as g++ 2.95 emits a namespace-scope const (names unproven).
const float kRedHue = 0.0f;
const float kGreenHue = 1.0f / 3.0f;
const float kBlueHue = 2.0f / 3.0f;
static void HSVToRGB(float h, float s, float v,
                     unsigned int* r, unsigned int* g, unsigned int* b);

// Dreamcast exposes this original helper boundary (palette.cpp:45). Loki
// emits it after every ordinary function: an inline file-static body that
// g++ 2.95 defers to the end of the unit.
// Loki file-static 0x081aa5a4, 0x30 bytes.
DC_ADDRESS(0x10a244, 0x62)
inline static long ftol(double d)
{
    const unsigned long magic = 0x59c00000;
    d += *(const float*)&magic;
    return *(long*)&d;
}

unsigned int TPalette16::red_mask;
unsigned int TPalette16::green_mask;
unsigned int TPalette16::blue_mask;

VA(0x00522650, 0x16)
DC_ADDRESS(0x10a2a8, 0x48)
MAC_ADDRESS(0x13b73c, 0x40)
TPalette16::TPalette16()
    : resource(0, RESOURCE_TYPE_NONE)
{
}

VA(0x005226a0, 0x2D)
DC_ADDRESS(0x10a2f0, 0x48)
MAC_ADDRESS(0x13b77c, 0x68)
TPalette16::TPalette16(const unsigned short* newData)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_data, newData, sizeof(m_data));
}

VA(0x005226d0, 0x9D)
DC_ADDRESS(0x10a338, 0x72)
MAC_ADDRESS(0x13b7e4, 0x88)
TPalette16::TPalette16(const TPalette24& p24, int rbits, int rshift,
                       int gbits, int gshift, int bbits, int bshift)
    : resource(0, RESOURCE_TYPE_NONE)
{
    Convert24to16(p24.m_palette, rbits, rshift, gbits, gshift, bbits, bshift);
}

DC_ADDRESS(0x10a3ac, 0x6e)
TPalette16::TPalette16(const TRGBA* rgba, int rbits, int rshift,
                       int gbits, int gshift, int bbits, int bshift)
    : resource(0, RESOURCE_TYPE_NONE)
{
    ConvertRGBAto16(rgba, rbits, rshift, gbits, gshift, bbits, bshift);
}

DC_ADDRESS(0x10a41c, 0x7c)
TPalette16::TPalette16(const tagRGBQUAD* quad, int rbits, int rshift,
                       int gbits, int gshift, int bbits, int bshift)
    : resource(0, RESOURCE_TYPE_NONE)
{
    ConvertRGBQUADto16(quad, rbits, rshift, gbits, gshift, bbits, bshift);
}

VA(0x00522770, 0x9F)
DC_ADDRESS(0x10a498, 0x70)
MAC_ADDRESS(0x13b86c, 0x84)
TPalette16::TPalette16(const char* name, const TPalette24& p24,
                       int rbits, int rshift, int gbits, int gshift,
                       int bbits, int bshift)
    : resource(name, RESOURCE_TYPE_PALETTE)
{
    Convert24to16(p24.m_palette, rbits, rshift, gbits, gshift, bbits, bshift);
}

VA(0x00522810, 0xC6)
DC_ADDRESS(0x10a508, 0xd8)
MAC_ADDRESS(0x13b8f0, 0x120)
TPalette16::TPalette16(const TPalette24& p24)
    : resource(0, RESOURCE_TYPE_NONE)
{
    unsigned short* p16 = m_data;
    const unsigned int redScale = (red_mask << 1) & ~red_mask;
    const unsigned int greenScale = (green_mask << 1) & ~green_mask;
    const unsigned int blueScale = (blue_mask << 1) & ~blue_mask;
    for (int i = 0; i < 256; i++) {
        const unsigned int red = ((p24.m_palette[i * 3] * redScale) / 256) & red_mask;
        const unsigned int green = ((p24.m_palette[i * 3 + 1] * greenScale) / 256) & green_mask;
        const unsigned int blue = ((p24.m_palette[i * 3 + 2] * blueScale) / 256) & blue_mask;
        *p16 = red | green | blue;
        p16++;
    }
}

DC_ADDRESS(0x10a5e0, 0xc2)
TPalette16::TPalette16(const TRGBA* rgba)
    : resource(0, RESOURCE_TYPE_NONE)
{
    unsigned short* p16 = m_data;
    const unsigned int redScale = (red_mask << 1) & ~red_mask;
    const unsigned int greenScale = (green_mask << 1) & ~green_mask;
    const unsigned int blueScale = (blue_mask << 1) & ~blue_mask;
    for (int i = 0; i < 256; i++) {
        const unsigned int red = ((rgba[i].m_red * redScale) / 256) & red_mask;
        const unsigned int green = ((rgba[i].m_green * greenScale) / 256) & green_mask;
        const unsigned int blue = ((rgba[i].m_blue * blueScale) / 256) & blue_mask;
        *p16 = red | green | blue;
        p16++;
    }
}

DC_ADDRESS(0x10a6a4, 0xd8)
TPalette16::TPalette16(const tagRGBQUAD* quad)
    : resource(0, RESOURCE_TYPE_NONE)
{
    unsigned short* p16 = m_data;
    const unsigned int redScale = (red_mask << 1) & ~red_mask;
    const unsigned int greenScale = (green_mask << 1) & ~green_mask;
    const unsigned int blueScale = (blue_mask << 1) & ~blue_mask;
    for (int i = 0; i < 256; i++) {
        const unsigned int red = ((quad[i].rgbRed * redScale) / 256) & red_mask;
        const unsigned int green = ((quad[i].rgbGreen * greenScale) / 256) & green_mask;
        const unsigned int blue = ((quad[i].rgbBlue * blueScale) / 256) & blue_mask;
        *p16 = red | green | blue;
        p16++;
    }
}

DC_ADDRESS(0x10a77c, 0xd6)
TPalette16::TPalette16(const char* name, const TPalette24& p24)
    : resource(name, RESOURCE_TYPE_PALETTE)
{
    unsigned short* p16 = m_data;
    const unsigned int redScale = (red_mask << 1) & ~red_mask;
    const unsigned int greenScale = (green_mask << 1) & ~green_mask;
    const unsigned int blueScale = (blue_mask << 1) & ~blue_mask;
    for (int i = 0; i < 256; i++) {
        const unsigned int red = ((p24.m_palette[i * 3] * redScale) / 256) & red_mask;
        const unsigned int green = ((p24.m_palette[i * 3 + 1] * greenScale) / 256) & green_mask;
        const unsigned int blue = ((p24.m_palette[i * 3 + 2] * blueScale) / 256) & blue_mask;
        *p16 = red | green | blue;
        p16++;
    }
}

VA(0x005228e0, 0x30)
DC_ADDRESS(0x10a854, 0x4a)
MAC_ADDRESS(0x13ba10, 0x68)
TPalette16::TPalette16(const TPalette16& copy)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_data, copy.m_data, sizeof(m_data));
}

VA(0x00522910, 0x21)
DC_ADDRESS(0x10a8a0, 0x40)
MAC_ADDRESS(0x13ba78, 0x48)
TPalette16& TPalette16::operator=(const TPalette16& from)
{
    if (this != &from)
        memcpy(m_data, from.m_data, sizeof(m_data));
    return *this;
}

VA(0x00522940, 0xB)
DC_ADDRESS(0x10a8e0, 0x2e)
MAC_ADDRESS(0x13bac0, 0x60)
TPalette16::~TPalette16()
{
}

DC_ADDRESS(0x10a910, 0x88)
MAC_ADDRESS(0x13bb20, 0xa0)
void TPalette16::Convert24to16(const unsigned char* p24, int rbits, int rshift,
                               int gbits, int gshift, int bbits, int bshift)
{
    unsigned short* p16 = m_data;
    for (int i = 0; i < 256; i++) {
        const unsigned short red = (p24[i * 3] >> (8 - rbits)) << rshift;
        const unsigned short green = (p24[i * 3 + 1] >> (8 - gbits)) << gshift;
        const unsigned short blue = (p24[i * 3 + 2] >> (8 - bbits)) << bshift;
        *p16 = red | green | blue;
        p16++;
    }
}

DC_ADDRESS(0x10a998, 0x7e)
void TPalette16::ConvertRGBAto16(const TRGBA* rgba, int rbits, int rshift,
                                 int gbits, int gshift, int bbits, int bshift)
{
    unsigned short* p16 = m_data;
    for (int i = 0; i < 256; i++) {
        const unsigned short red = (rgba[i].m_red >> (8 - rbits)) << rshift;
        const unsigned short green = (rgba[i].m_green >> (8 - gbits)) << gshift;
        const unsigned short blue = (rgba[i].m_blue >> (8 - bbits)) << bshift;
        *p16 = red | green | blue;
        p16++;
    }
}

DC_ADDRESS(0x10aa18, 0x7e)
void TPalette16::ConvertRGBQUADto16(const tagRGBQUAD* quad, int rbits, int rshift,
                                    int gbits, int gshift, int bbits, int bshift)
{
    unsigned short* p16 = m_data;
    for (int i = 0; i < 256; i++) {
        const unsigned short red = (quad[i].rgbRed >> (8 - rbits)) << rshift;
        const unsigned short green = (quad[i].rgbGreen >> (8 - gbits)) << gshift;
        const unsigned short blue = (quad[i].rgbBlue >> (8 - bbits)) << bshift;
        *p16 = red | green | blue;
        p16++;
    }
}

VA(0x00522950, 0xBE)
DC_ADDRESS(0x10aa98, 0xac)
MAC_ADDRESS(0x13bbc0, 0xe0)
void TPalette16::Cycle(int begin, int end, int step)
{
    if (step > 0) {
        for (int i = step; i > 0; i--) {
            const unsigned short saved = m_data[begin];
            memmove(&m_data[begin], &m_data[begin + 1],
                    (end - begin) * sizeof(unsigned short));
            m_data[end] = saved;
        }
    } else {
        for (int i = -step; i > 0; i--) {
            const unsigned short saved = m_data[end];
            memmove(&m_data[begin + 1], &m_data[begin],
                    (end - begin) * sizeof(unsigned short));
            m_data[begin] = saved;
        }
    }
}

DC_ADDRESS(0x10ab44, 0x416)
void TPalette16::Colorize(float hue, float saturation)
{
    const unsigned int redNorm = numeric_limits<int>::max() / red_mask;
    const unsigned int greenNorm = numeric_limits<int>::max() / green_mask;
    const unsigned int blueNorm = numeric_limits<int>::max() / blue_mask;
    for (int i = 10; i < 256; i++) {
        unsigned int r = (m_data[i] & red_mask) * redNorm;
        unsigned int g = (m_data[i] & green_mask) * greenNorm;
        unsigned int b = (m_data[i] & blue_mask) * blueNorm;
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
        m_data[i] = (unsigned short)(((r / redNorm) & red_mask) |
                                     ((g / greenNorm) & green_mask) |
                                     ((b / blueNorm) & blue_mask));
    }
}

DC_ADDRESS(0x10af5c, 0x28e)
void TPalette16::AdjustHue(float hue, float amount)
{
    const unsigned int redNorm = numeric_limits<int>::max() / red_mask;
    const unsigned int greenNorm = numeric_limits<int>::max() / green_mask;
    const unsigned int blueNorm = numeric_limits<int>::max() / blue_mask;
    for (int i = 10; i < 256; i++) {
        unsigned int r = (m_data[i] & red_mask) * redNorm;
        unsigned int g = (m_data[i] & green_mask) * greenNorm;
        unsigned int b = (m_data[i] & blue_mask) * blueNorm;
        float h, s, v;
        RGBToHSV(r, g, b, &h, &s, &v);
        float delta = hue - h;
        h += amount * delta;
        if (fabs(delta) > 0.5) {
            if (delta > 0.0)
                h += 1.0f - amount;
            else
                h += amount;
            if (h >= 1.0)
                h -= 1.0;
        }
        HSVToRGB(h, s, v, &r, &g, &b);
        m_data[i] = (unsigned short)(((r / redNorm) & red_mask) |
                                     ((g / greenNorm) & green_mask) |
                                     ((b / blueNorm) & blue_mask));
    }
}

VA(0x00522a10, 0x122)
DC_ADDRESS(0x10b1ec, 0x134)
MAC_ADDRESS(0x13bca0, 0x160)
void TPalette16::AdjustSaturation(float amount)
{
    const unsigned int redNorm = numeric_limits<int>::max() / red_mask;
    const unsigned int greenNorm = numeric_limits<int>::max() / green_mask;
    const unsigned int blueNorm = numeric_limits<int>::max() / blue_mask;
    for (int i = 10; i < 256; i++) {
        unsigned int r = (m_data[i] & red_mask) * redNorm;
        unsigned int g = (m_data[i] & green_mask) * greenNorm;
        unsigned int b = (m_data[i] & blue_mask) * blueNorm;
        float h, s, v;
        RGBToHSV(r, g, b, &h, &s, &v);
        if (amount <= 1.0f)
            s *= amount;
        else
            s = 1.0f - (1.0f - s) / amount;
        HSVToRGB(h, s, v, &r, &g, &b);
        m_data[i] = (unsigned short)(((r / redNorm) & red_mask) |
                                     ((g / greenNorm) & green_mask) |
                                     ((b / blueNorm) & blue_mask));
    }
}

DC_ADDRESS(0x10b320, 0x164)
void TPalette16::AdjustValue(float amount)
{
    const unsigned int redNorm = numeric_limits<int>::max() / red_mask;
    const unsigned int greenNorm = numeric_limits<int>::max() / green_mask;
    const unsigned int blueNorm = numeric_limits<int>::max() / blue_mask;
    for (int i = 10; i < 256; i++) {
        unsigned int r = (m_data[i] & red_mask) * redNorm;
        unsigned int g = (m_data[i] & green_mask) * greenNorm;
        unsigned int b = (m_data[i] & blue_mask) * blueNorm;
        float h, s, v;
        RGBToHSV(r, g, b, &h, &s, &v);
        if (amount <= 1.0f)
            v *= amount;
        else
            v = 1.0f - (1.0f - v) / amount;
        HSVToRGB(h, s, v, &r, &g, &b);
        m_data[i] = (unsigned short)(((r / redNorm) & red_mask) |
                                     ((g / greenNorm) & green_mask) |
                                     ((b / blueNorm) & blue_mask));
    }
}

VA(0x00522b50, 0x1F5)
DC_ADDRESS(0x10b484, 0x328)
MAC_ADDRESS(0x13be08, 0x274)
void TPalette16::AdjustHSV(float hue, float hueAdjust,
                           float saturationAdjust, float valueAdjust)
{
    const unsigned int redNorm = numeric_limits<int>::max() / red_mask;
    const unsigned int greenNorm = numeric_limits<int>::max() / green_mask;
    const unsigned int blueNorm = numeric_limits<int>::max() / blue_mask;
    for (int i = 10; i < 256; i++) {
        unsigned int r = (m_data[i] & red_mask) * redNorm;
        unsigned int g = (m_data[i] & green_mask) * greenNorm;
        unsigned int b = (m_data[i] & blue_mask) * blueNorm;
        float h, s, v;
        RGBToHSV(r, g, b, &h, &s, &v);
        if (hueAdjust >= 0.0f) {
            float delta = hue - h;
            h += hueAdjust * delta;
            if (fabs(delta) > 0.5) {
                if (delta > 0.0)
                    h += 1.0f - hueAdjust;
                else
                    h += hueAdjust;
                if (h >= 1.0)
                    h -= 1.0;
            }
        }
        if (saturationAdjust >= 0.0f) {
            if (saturationAdjust <= 1.0f)
                s *= saturationAdjust;
            else
                s = 1.0f - (1.0f - s) / saturationAdjust;
        }
        if (valueAdjust >= 0.0) {
            if (valueAdjust <= 1.0f)
                v *= valueAdjust;
            else
                v = 1.0f - (1.0f - v) / valueAdjust;
        }
        HSVToRGB(h, s, v, &r, &g, &b);
        m_data[i] = (unsigned short)(((r / redNorm) & red_mask) |
                                     ((g / greenNorm) & green_mask) |
                                     ((b / blueNorm) & blue_mask));
    }
}

VA(0x00522d50, 0xD6)
DC_ADDRESS(0x10b7ac, 0xea)
MAC_ADDRESS(0x13c07c, 0x170)
void TPalette16::Gray()
{
    const unsigned int redNorm = numeric_limits<int>::max() / red_mask;
    const unsigned int greenNorm = numeric_limits<int>::max() / green_mask;
    const unsigned int blueNorm = numeric_limits<int>::max() / blue_mask;
    for (int i = 10; i < 256; i++) {
        const unsigned int r = (m_data[i] & red_mask) * redNorm;
        const unsigned int g = (m_data[i] & green_mask) * greenNorm;
        const unsigned int b = (m_data[i] & blue_mask) * blueNorm;
        const unsigned int gray = (r > g ? r : g) > b ? (r > g ? r : g) : b;
        m_data[i] = (unsigned short)(((gray / redNorm) & red_mask) |
                                     ((gray / greenNorm) & green_mask) |
                                     ((gray / blueNorm) & blue_mask));
    }
}

VA(0x00522e30, 0x16)
DC_ADDRESS(0x10b898, 0x6c)
MAC_ADDRESS(0x13c1ec, 0x40)
TPalette24::TPalette24()
    : resource(0, RESOURCE_TYPE_NONE)
{
}

VA(0x00522e80, 0x2D)
DC_ADDRESS(0x10b904, 0x48)
MAC_ADDRESS(0x13c22c, 0x68)
TPalette24::TPalette24(const unsigned char* data)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_palette, data, sizeof(m_palette));
}

VA(0x00522eb0, 0x42)
DC_ADDRESS(0x10b94c, 0x78)
MAC_ADDRESS(0x13c294, 0x124)
TPalette24::TPalette24(const TRGBA* rgba)
    : resource(0, RESOURCE_TYPE_NONE)
{
    for (int i = 0; i < 256; i++) {
        m_palette[i * 3] = rgba->m_red;
        m_palette[i * 3 + 1] = rgba->m_green;
        m_palette[i * 3 + 2] = rgba->m_blue;
        rgba++;
    }
}

DC_ADDRESS(0x10b9c4, 0x78)
TPalette24::TPalette24(const tagRGBQUAD* quad)
    : resource(0, RESOURCE_TYPE_NONE)
{
    for (int i = 0; i < 256; i++) {
        m_palette[i * 3] = quad->rgbRed;
        m_palette[i * 3 + 1] = quad->rgbGreen;
        m_palette[i * 3 + 2] = quad->rgbBlue;
        quad++;
    }
}

VA(0x00522f00, 0x30)
DC_ADDRESS(0x10ba3c, 0x4a)
MAC_ADDRESS(0x13c3b8, 0x68)
TPalette24::TPalette24(const TPalette24& copy)
    : resource(0, RESOURCE_TYPE_NONE)
{
    memcpy(m_palette, copy.m_palette, sizeof(m_palette));
}

VA(0x00522f30, 0x21)
DC_ADDRESS(0x10ba88, 0x22)
MAC_ADDRESS(0x13c420, 0x48)
TPalette24& TPalette24::operator=(const TPalette24& from)
{
    if (this != &from)
        memcpy(m_palette, from.m_palette, sizeof(m_palette));
    return *this;
}

VA(0x00522f60, 0x0b)
DC_ADDRESS(0x10baac, 0x44)
MAC_ADDRESS(0x13c468, 0x60)
TPalette24::~TPalette24()
{
}

DC_ADDRESS(0x10baf0, 0x102)
void TPalette24::Cycle(int begin, int end, int step)
{
    begin *= 3;
    end *= 3;
    if (step > 0) {
        for (int i = step; i > 0; i--) {
            int r = m_palette[begin];
            int g = m_palette[begin + 1];
            int b = m_palette[begin + 2];
            memmove(&m_palette[begin], &m_palette[begin + 3], end - begin);
            m_palette[end] = r;
            m_palette[end + 1] = g;
            m_palette[end + 2] = b;
        }
    } else {
        for (int i = -step; i > 0; i--) {
            int r = m_palette[end];
            int g = m_palette[end + 1];
            int b = m_palette[end + 2];
            memmove(&m_palette[begin + 3], &m_palette[begin], end - begin);
            m_palette[begin] = r;
            m_palette[begin + 1] = g;
            m_palette[begin + 2] = b;
        }
    }
}

DC_ADDRESS(0x10bbf4, 0x364)
void TPalette24::Colorize(float hue, float saturation)
{
    for (int i = 10; i < 256; i++) {
        unsigned int r = m_palette[i * 3];
        unsigned int g = m_palette[i * 3 + 1];
        unsigned int b = m_palette[i * 3 + 2];
        const float v = (r > g ? r : g) > b ? (r > g ? r : g) : b;
        const int hextant = (int)(hue * 6.0f);
        const float f = fmod(hue * 6.0f, 1.0);
        const float p = v * (1.0f - saturation);
        const float q = v * (1.0f - saturation * f);
        const float t = v * (1.0f - saturation * (1.0f - f));
        switch (hextant) {
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
        m_palette[i * 3] = r;
        m_palette[i * 3 + 1] = g;
        m_palette[i * 3 + 2] = b;
    }
}

DC_ADDRESS(0x10bf58, 0x7a)
void TPalette24::Gray()
{
    for (int i = 10; i < 256; i++) {
        const unsigned int r = m_palette[i * 3];
        const unsigned int g = m_palette[i * 3 + 1];
        const unsigned int b = m_palette[i * 3 + 2];
        const unsigned int gray = (r > g ? r : g) > b ? (r > g ? r : g) : b;
        m_palette[i * 3] = gray;
        m_palette[i * 3 + 1] = gray;
        m_palette[i * 3 + 2] = gray;
    }
}

VA(0x00522f80, 0x20E)
DC_ADDRESS(0x10bfd4, 0x39c)
MAC_ADDRESS(0x13c4d0, 0x2c4)
void TPalette24::AdjustHSV(float hue, float hueAdjust,
                           float saturationAdjust, float valueAdjust)
{
    const unsigned int redNorm = numeric_limits<int>::max() / 255;
    const unsigned int greenNorm = numeric_limits<int>::max() / 255;
    const unsigned int blueNorm = numeric_limits<int>::max() / 255;
    for (int i = 10; i < 256; i++) {
        unsigned int r = m_palette[i * 3] * redNorm;
        unsigned int g = m_palette[i * 3 + 1] * greenNorm;
        unsigned int b = m_palette[i * 3 + 2] * blueNorm;
        float h, s, v;
        RGBToHSV(r, g, b, &h, &s, &v);
        if (hueAdjust >= 0.0f) {
            float delta = hue - h;
            h += hueAdjust * delta;
            if (fabs(delta) > 0.5) {
                if (delta > 0.0)
                    h += 1.0f - hueAdjust;
                else
                    h += hueAdjust;
                if (h >= 1.0)
                    h -= 1.0;
            }
        }
        if (valueAdjust >= 0.0) {
            if (valueAdjust <= 1.0f)
                v *= valueAdjust;
            else
                v = 1.0f - (1.0f - v) / valueAdjust;
        }
        if (saturationAdjust >= 0.0f) {
            if (saturationAdjust <= 1.0f)
                s *= saturationAdjust;
            else if (v > 0.75 && s < 0.25)
                s *= (1.0f - v) * saturationAdjust * 4.0f;
            else
                s = 1.0f - (1.0f - s) / saturationAdjust;
        }
        HSVToRGB(h, s, v, &r, &g, &b);
        m_palette[i * 3] = r / redNorm;
        m_palette[i * 3 + 1] = g / greenNorm;
        m_palette[i * 3 + 2] = b / blueNorm;
    }
}

// Original: RGBToHSV; palette.cpp:827
// Loki file-static 0x081aa08c, 0x1cd bytes.
VA(0x00523190, 0x160)
DC_ADDRESS(0x10c370, 0x1f2)
MAC_ADDRESS(0x13c794, 0x1c8)
static void RGBToHSV(unsigned int r, unsigned int g, unsigned int b,
                     float* h, float* s, float* v)
{
    const unsigned int max = (r > g ? r : g) > b ? (r > g ? r : g) : b;
    const unsigned int min = (r < g ? r : g) < b ? (r < g ? r : g) : b;
    *v = (float)max / numeric_limits<int>::max();
    *s = max ? (float)(max - min) / (float)max : 0.0f;
    if (max != min) {
        const float delta = max - min;
        const float rc = (max - r) / delta;
        const float gc = (max - g) / delta;
        const float bc = (max - b) / delta;
        if (r == max)
            *h = (bc - gc) / 6.0f + kRedHue;
        else if (g == max)
            *h = (rc - bc) / 6.0f + kGreenHue;
        else
            *h = (gc - rc) / 6.0f + kBlueHue;
        if (*h < 0.0f)
            *h += 1.0f;
    } else {
        *h = 0.0f;
    }
}

// Original: HSVToRGB; palette.cpp:862
// Loki file-static 0x081aa25c, 0x31d bytes.
VA(0x005232f0, 0x2EC)
DC_ADDRESS(0x10c564, 0x34c)
MAC_ADDRESS(0x13c95c, 0x248)
static void HSVToRGB(float h, float s, float v,
                     unsigned int* r, unsigned int* g, unsigned int* b)
{
    if (s != 0.0f) {
        const float f = fmod(h * 6.0f, 1.0);
        v *= numeric_limits<int>::max();
        const float p = v * (1.0f - s);
        const float q = v * (1.0f - s * f);
        const float t = v * (1.0f - s * (1.0f - f));
        switch ((int)(h * 6.0f)) {
        case HSV_RED_SECTOR:
            *r = ftol(v); *g = ftol(t); *b = ftol(p);
            break;
        case HSV_YELLOW_SECTOR:
            *r = ftol(q); *g = ftol(v); *b = ftol(p);
            break;
        case HSV_GREEN_SECTOR:
            *r = ftol(p); *g = ftol(v); *b = ftol(t);
            break;
        case HSV_CYAN_SECTOR:
            *r = ftol(p); *g = ftol(q); *b = ftol(v);
            break;
        case HSV_BLUE_SECTOR:
            *r = ftol(t); *g = ftol(p); *b = ftol(v);
            break;
        case HSV_MAGENTA_SECTOR:
            *r = ftol(v); *g = ftol(p); *b = ftol(q);
            break;
        }
    } else {
        *r = *g = *b = ftol(numeric_limits<int>::max() * v);
    }
}
