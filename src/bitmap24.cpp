// 15 functions in link order.
#include "va.h"

#include <limits>
#include <math.h>
#include <new>
#include <string.h>

#include "bitmap24.h"

#include "bitmap16.h"
#include "hsv.h"
#include "pcx.h"

// Dreamcast exposes these three source helpers as standalone bitmap24.cpp
// functions. Complete retains each call boundary in source but VC6 expands all
// of them into AdjustHSV, leaving no separate x86 bodies in this TU. The ftol
// dossier proves its double parameter and sole named local, const unsigned long
// magic; reusing d's representation is what gives retail its shared qword home.
// DC bitmap24.cpp:40 initializes magic, line 42 updates d, and line 43 reads
// its low word. Removing the unsupported forced-inline attribute produces
// the same code object: AdjustHSV stays 95.7470% and all seven exact siblings
// hold. The ordinary static helper expands naturally through HSVToRGB.
// Original: ftol; bitmap24.cpp:39
DC_ADDRESS(0x0525b4, 0x62)
static long ftol(double d)
{
    const unsigned long magic = 0x59c00000;
    d += *static_cast<const float*>(static_cast<const void*>(&magic));
    return *static_cast<long*>(static_cast<void*>(&d));
}

static void RGBToHSV(unsigned int r, unsigned int g, unsigned int b,
                     float* h, float* s, float* v);
static void HSVToRGB(float h, float s, float v,
                     unsigned int* r, unsigned int* g, unsigned int* b);

VA_COMPGEN(0x0044ed20, 0x21, SCALAR_DELETING_DTOR, Bitmap24Bit)

// Original: Bitmap24Bit::Bitmap24Bit; bitmap24.cpp:55
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

VA(0x0044ee00, 0xC0)
DC_ADDRESS(0x0526e8, 0x6c)
Bitmap24Bit::Bitmap24Bit(const char* name, const char* path)
    : resource(name, RESOURCE_TYPE_BITMAP24),
      m_dataSize(0), m_imageSize(0), m_width(0), m_height(0), m_data(0)
{
    char filename[261];
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

// Original: Bitmap24Bit::import; bitmap24.cpp:101
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

// Original: Bitmap24Bit::clear; bitmap24.cpp:123
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

VA(0x0044eef0, 0xDB)
DC_ADDRESS(0x05281c, 0xdc)
int Bitmap24Bit::importPCXFile(const char* filename)
{
    PcxData pdat;
    imgdes pcxfile;
    int error = pcxinfo(filename, &pdat);
    if (error)
        return 1;

    m_width = pdat.m_width;
    m_height = pdat.m_length;
    m_imageSize = m_width * m_height * 3;
    m_dataSize = m_imageSize;
    m_data = new unsigned char[m_imageSize];
    if (!m_data)
        return 2;

    allocimage(&pcxfile, pdat.m_width, pdat.m_length,
               pdat.m_bpPixel * pdat.m_nplanes);
    loadpcx(filename, &pcxfile);
    flipimage(&pcxfile, &pcxfile);

    for (int y = 0; y < m_height; ++y) {
        memcpy(m_data + y * GetPitch(),
               pcxfile.m_ibuff + y * pcxfile.m_buffwidth,
               GetPitch());
    }

    freeimage(&pcxfile);
    return 0;
}

VA(0x0044efd0, 0x37)
DC_ADDRESS(0x0528f8, 0x70)
MAC_ADDRESS(0x05cfd0, 0x3c)
void Bitmap24Bit::Draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                       int dx, int dy) const
{
    Draw(sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
         dst->GetWidth(), dst->GetHeight(), dst->GetPitch());
}

// E:\gamedcs\bitmap24.cpp:280. Dreamcast proves the clipped rectangle,
// source pointer, three channel-scale locals, nested row/pixel scopes and the
// two GetPitch boundaries. Retail independently fixes the 24-bit-to-16-bit
// channel conversion and brackets this 353-byte body immediately after the
// Bitmap16Bit wrapper above.
// Dreamcast line 342 advances both row pointers after the inner pixel loop;
// Complete retains the same post-row induction.
VA(0x0044f010, 0x161)
DC_ADDRESS(0x052968, 0x13e)
MAC_ADDRESS(0x05d00c, 0x210)  // source-order bracket + RGB mask/data flow
void Bitmap24Bit::Draw(int sx, int sy, int sw, int sh, unsigned short* dst,
                       int dx, int dy, int dw, int dh, int dpitch) const
{
    if (dx < 0) {
        sx -= dx;
        sw += dx;
        dx = 0;
    }
    if (dy < 0) {
        sy -= dy;
        sh += dy;
        dy = 0;
    }
    if (dx + sw > dw)
        sw = dw - dx;
    if (dy + sh > dh)
        sh = dh - dy;

    if (sw > 0 && sh > 0) {
        const unsigned char* src = m_data + sy * GetPitch() + sx * 3;
        dst = static_cast<unsigned short*>(static_cast<void*>(
            static_cast<unsigned char*>(static_cast<void*>(dst))
            + dy * dpitch + dx * 2));
        // Complete's x86 relocation and byte schedule require this later-
        // revision declaration order. It retains all three DC-named scale
        // locals while assigning bm1/rm1/gm1 to retail's ESI/stack/EBX roles.
        const unsigned int rm1 = (Bitmap16Bit::red_mask << 1) & ~Bitmap16Bit::red_mask;
        const unsigned int gm1 = (Bitmap16Bit::green_mask << 1) & ~Bitmap16Bit::green_mask;
        const unsigned int bm1 = (Bitmap16Bit::blue_mask << 1) & ~Bitmap16Bit::blue_mask;

        for (int y = 0; y < sh; ++y) {
            const unsigned char* in = src;
            unsigned short* out = dst;
            for (int x = 0; x < sw; ++x) {
                unsigned int blue =
                    ((in[0] * bm1) >> 8) & Bitmap16Bit::blue_mask;
                unsigned int green =
                    ((in[1] * gm1) >> 8) & Bitmap16Bit::green_mask;
                unsigned int red =
                    ((in[2] * rm1) >> 8) & Bitmap16Bit::red_mask;
                // The operands are commutative; this grouping is retail's
                // exact blue/red/green source-load schedule under VC6 C1.
                *out++ = static_cast<unsigned short>(blue | red | green);
                in += 3;
            }
            dst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dpitch));
            src += GetPitch();
        }
    }
}

VA(0x0044f180, 0x7)
MAC_ADDRESS(0x05d21c, 0xc)
unsigned int Bitmap24Bit::getSize() const
{
    return m_dataSize + sizeof(Bitmap24Bit);
}

// E:\gamedcs\bitmap24.cpp:349
VA(0x0044f190, 0x5F8)
DC_ADDRESS(0x052aa8, 0x424)
MAC_ADDRESS(0x05d228, 0x2f8)  // source-order bracket + inlined HSV helpers
void Bitmap24Bit::AdjustHSV(int x, int y, int w, int h, float hue,
                            float hueAdjust, float saturationAdjust,
                            float valueAdjust)
{
    const unsigned int redNorm =
        std::numeric_limits<int>::max() / 255;
    const unsigned int greenNorm =
        std::numeric_limits<int>::max() / 255;
    const unsigned int blueNorm =
        std::numeric_limits<int>::max() / 255;

    unsigned char* src = m_data + y * GetPitch() + x * 3;
    for (int row = 0; row < h; ++row) {
        unsigned char* pixel = src;
        for (int column = 0; column < w; ++column) {
            unsigned int r = pixel[2] * redNorm;
            unsigned int g = pixel[1] * greenNorm;
            unsigned int b = pixel[0] * blueNorm;

            float h;
            float s;
            float v;
            RGBToHSV(r, g, b, &h, &s, &v);

            if (hueAdjust >= 0.0f) {
                float delta = hue - h;
                h += delta * hueAdjust;
                if (fabs(delta) > 0.5) {
                    if (delta > 0.0) {
                        h += 1.0f - hueAdjust;
                    } else {
                        h += hueAdjust;
                    }
                    if (h >= 1.0)
                        h -= 1.0;
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

            HSVToRGB(h, s, v, &r, &g, &b);

            pixel[2] = static_cast<unsigned char>(r / redNorm);
            pixel[1] = static_cast<unsigned char>(g / greenNorm);
            pixel[0] = static_cast<unsigned char>(b / blueNorm);
            pixel += 3;
        }

        src += GetPitch();
    }
}

// Original: RGBToHSV; bitmap24.cpp:446
DC_ADDRESS(0x052ecc, 0x1b6)
MAC_ADDRESS(0x05d520, 0x1c8)
static void RGBToHSV(unsigned int r, unsigned int g,
                            unsigned int b, float* h, float* s, float* v)
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

        if (*h < 0.0f)
            *h += 1.0f;
    } else {
        *h = 0.0f;
    }
}

// Original: HSVToRGB; bitmap24.cpp:481
DC_ADDRESS(0x053084, 0x32c)
MAC_ADDRESS(0x05d6e8, 0x248)
static void HSVToRGB(float h, float s, float v,
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
