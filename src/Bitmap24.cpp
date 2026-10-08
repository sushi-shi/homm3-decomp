// Bitmap24.cpp - Bitmap24Bit (Loki h3maped object 40).
//
// The Loki link order: the constructors, destructor, import, clear, the
// stubbed PCX import, Draw, AdjustHSV and the file-static RGBToHSV/HSVToRGB
// pair; g++ then emits the bitmap24.h inline members and the inline ftol.
#include "va.h"

#include <assert.h>
#include <limits>
#include <math.h>
#include <string.h>

#include "exceptions.h"
#include "bitmap24.h"

#include "bitmap16.h"
#include "hsv.h"

// Loki keeps the helper but asserts on entry (Bitmap24.cpp:54).
DC_ADDRESS(0x0525b4, 0x62)
inline static long ftol(double d)
{
#line 54
    assert(0);
    const unsigned long magic = 0x59c00000;
    d += *(const float*)&magic;
    return *(long*)&d;
}

static void RGBToHSV(unsigned int r, unsigned int g, unsigned int b,
                     float* h, float* s, float* v);
static void HSVToRGB(float h, float s, float v,
                     unsigned int* r, unsigned int* g, unsigned int* b);

DC_ADDRESS(0x052618, 0x54)
Bitmap24Bit::Bitmap24Bit()
    : resource(0, RESOURCE_TYPE_NONE),
      m_dataSize(0), m_imageSize(0), m_width(0), m_height(0), m_data(0)
{
}

VA(0x0044ed50, 0xAA)
DC_ADDRESS(0x05266c, 0x7a)
MAC_ADDRESS(0x05ce9c, 0xc4)
Bitmap24Bit::Bitmap24Bit(const char* name, int w, int h,
                         const unsigned char* source, int size)
    : resource(name, RESOURCE_TYPE_BITMAP24),
      m_imageSize(w * h * 3), m_width(w), m_height(h)
{
    m_dataSize = size ? size : m_imageSize;
    m_data = new unsigned char[m_dataSize];
    if (m_data)
        memcpy(m_data, source, m_dataSize);
}

// Loki builds the path in a PATH_MAX buffer.
VA(0x0044ee00, 0xC0)
DC_ADDRESS(0x0526e8, 0x6c)
Bitmap24Bit::Bitmap24Bit(const char* name, const char* path)
    : resource(name, RESOURCE_TYPE_BITMAP24),
      m_dataSize(0), m_imageSize(0), m_width(0), m_height(0), m_data(0)
{
    char filename[4096];
    strcpy(filename, path);
    strcat(filename, name);
    importPCXFile(filename);
}

VA(0x0044eec0, 0x22)
DC_ADDRESS(0x052754, 0x3e)
MAC_ADDRESS(0x05cf60, 0x70)
Bitmap24Bit::~Bitmap24Bit()
{
    if (m_data)
        delete[] m_data;
}

DC_ADDRESS(0x052794, 0x5a)
void Bitmap24Bit::import(int w, int h, const unsigned char* data, int size)
{
    clear();
    m_width = w;
    m_height = h;
    m_imageSize = w * h * 3;
    m_dataSize = size ? size : m_imageSize;
    m_data = new unsigned char[m_dataSize];
    if (m_data)
        memcpy(m_data, data, m_dataSize);
}

DC_ADDRESS(0x0527f0, 0x2a)
void Bitmap24Bit::clear()
{
    m_width = 0;
    m_height = 0;
    m_dataSize = 0;
    m_imageSize = 0;
    if (m_data) {
        delete[] m_data;
        m_data = 0;
    }
}

// The Loki port has no PCX reader.
VA(0x0044eef0, 0xDB)
DC_ADDRESS(0x05281c, 0xdc)
int Bitmap24Bit::importPCXFile(const char* filename)
{
#line 326
    assert(0);
    return 2;
}

VA(0x0044efd0, 0x37)
DC_ADDRESS(0x0528f8, 0x70)
MAC_ADDRESS(0x05cfd0, 0x3c)
void Bitmap24Bit::Draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                       int dx, int dy) const
{
    Draw(sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy, dst->GetWidth(),
         dst->GetHeight(), dst->GetPitch());
}

VA(0x0044f010, 0x161)
DC_ADDRESS(0x052968, 0x13e)
MAC_ADDRESS(0x05d00c, 0x210)
void Bitmap24Bit::Draw(int sx, int sy, int sw, int sh, unsigned short* dst,
                       int dx, int dy, int dw, int dh, int dpitch) const
{
    if (dx < 0) {
        sx -= dx;
        sw -= -dx;
        dx = 0;
    }
    if (dy < 0) {
        sy -= dy;
        sh -= -dy;
        dy = 0;
    }
    if (dx + sw > dw)
        sw = dw - dx;
    if (dy + sh > dh)
        sh = dh - dy;
    if (sw <= 0 || sh <= 0)
        return;

    const unsigned char* src = m_data + sy * GetPitch() + sx * 3;
    dst = (unsigned short*)((unsigned char*)dst + dy * dpitch
                            + dx * sizeof(unsigned short));
    const unsigned int rs = (Bitmap16Bit::red_mask << 1) & ~Bitmap16Bit::red_mask;
    const unsigned int gs = (Bitmap16Bit::green_mask << 1) & ~Bitmap16Bit::green_mask;
    const unsigned int bs = (Bitmap16Bit::blue_mask << 1) & ~Bitmap16Bit::blue_mask;
    for (int y = 0; y < sh; y++) {
        unsigned short* out = dst;
        const unsigned char* in = src;
        for (int x = 0; x < sw; x++) {
            const unsigned int red = ((*(in + 2) * rs) / 256) & Bitmap16Bit::red_mask;
            const unsigned int green = ((*(in + 1) * gs) / 256) & Bitmap16Bit::green_mask;
            const unsigned int blue = ((*in * bs) / 256) & Bitmap16Bit::blue_mask;
            *out++ = red | green | blue;
            in += 3;
        }
        dst = (unsigned short*)((unsigned char*)dst + dpitch);
        src += GetPitch();
    }
}

VA(0x0044f190, 0x5F8)
DC_ADDRESS(0x052aa8, 0x424)
MAC_ADDRESS(0x05d228, 0x2f8)
void Bitmap24Bit::AdjustHSV(int x, int y, int w, int h, float hue,
                            float hueAdjust, float saturationAdjust,
                            float valueAdjust)
{
    const unsigned int redNorm = numeric_limits<int>::max() / 255;
    const unsigned int greenNorm = numeric_limits<int>::max() / 255;
    const unsigned int blueNorm = numeric_limits<int>::max() / 255;
    unsigned char* src = m_data + y * GetPitch() + x * 3;
    for (int row = 0; row < h; row++) {
        unsigned char* pixel = src;
        for (int column = 0; column < w; column++) {
            unsigned int r = *(pixel + 2) * redNorm;
            unsigned int g = *(pixel + 1) * greenNorm;
            unsigned int b = *pixel * blueNorm;
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
            *(pixel + 2) = r / redNorm;
            *(pixel + 1) = g / greenNorm;
            *pixel = b / blueNorm;
            pixel += 3;
        }
        src += GetPitch();
    }
}

DC_ADDRESS(0x052ecc, 0x1b6)
MAC_ADDRESS(0x05d520, 0x1c8)
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
            *h = (bc - gc) / 6.0f + 0.0f;
        else if (g == max)
            *h = (rc - bc) / 6.0f + 1.0f / 3.0f;
        else
            *h = (gc - rc) / 6.0f + 2.0f / 3.0f;
        if (*h < 0.0f)
            *h += 1.0f;
    } else {
        *h = 0.0f;
    }
}

DC_ADDRESS(0x053084, 0x32c)
MAC_ADDRESS(0x05d6e8, 0x248)
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
