// bitmap24.cpp - E:\gamedcs\bitmap24.cpp (compiland bitmap24.obj)
// 15 functions in link order.
#include <va.h>
#include <limits>
#include <math.h>
#include <string.h>
#include "bitmap24.h"
#include "bitmap16.h"
#include "hsv.h"
#include "pcx.h"

// The retail destructor is frameless under /GX, so this compiland saw the
// same nothrow deallocator contract as bitmap16.obj and sample.obj.
__declspec(nothrow) void __cdecl operator delete(void* value);

// Dreamcast exposes these three source helpers as standalone bitmap24.cpp
// functions. Complete retains each call boundary in source but VC6 expands all
// of them into AdjustHSV, leaving no separate x86 bodies in this TU. The ftol
// dossier proves its double parameter and sole named local, const unsigned long
// magic; reusing d's representation is what gives retail its shared qword home.
// DC bitmap24.cpp:40 initializes magic, line 42 updates d, and line 43 reads
// its low word. Removing the unsupported __forceinline attribute produces
// the same code object: AdjustHSV stays 95.7470% and all seven exact siblings
// hold. The ordinary static helper expands naturally through HSVToRGB.
// Original: ftol; bitmap24.cpp:39, dc 0x525b4
static long ftol(double d)
{
    const unsigned long magic = 0x59c00000;
    d += *static_cast<const float*>(static_cast<const void*>(&magic));
    return *static_cast<long*>(static_cast<void*>(&d));
}

static void rgbToHSV(unsigned int r, unsigned int g, unsigned int b,
                     float* h, float* s, float* v);
static void hsvToRGB(float h, float s, float v,
                     unsigned int* r, unsigned int* g, unsigned int* b);

#if 0  // @carcass: remaining Bitmap24Bit bodies are not reconstructed yet

// E:\gamedcs\bitmap24.cpp:64
DC_ONLY(0x5266c, 0x7A)
void Bitmap24Bit::Bitmap24Bit(const char* name, int w, int h, const unsigned char* data, int size)
{
    // @stub
}

// E:\gamedcs\bitmap24.cpp:80
DC_ONLY(0x526e8, 0x6C)
void Bitmap24Bit::Bitmap24Bit(const char* name, const char* path)
{
    // @stub
}

// E:\gamedcs\bitmap24.cpp:94
DC_ONLY(0x52754, 0x3E)
void Bitmap24Bit::~Bitmap24Bit()
{
    // @stub
}

// E:\gamedcs\bitmap24.cpp:274
DC_ONLY(0x528f8, 0x70)
void Bitmap24Bit::draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx, int dy)
{
    // @stub
}

// E:\gamedcs\bitmap24.cpp:349
DC_ONLY(0x52aa8, 0x424)
void Bitmap24Bit::adjustHSV(int x, int y, int w, int h, float hue, float hue_adjust, float saturation_adjust, float value_adjust)
{
    // @stub
}

// E:\gamedcs\bitmap24.cpp:446
DC_ONLY(0x52ecc, 0x1B6)
void rgbToHSV(unsigned r, unsigned g, unsigned b, float* h, float* s, float* v)
{
    // @stub
}

// E:\gamedcs\bitmap24.cpp:481
DC_ONLY(0x53084, 0x32C)
void hsvToRGB(float h, float s, float v, unsigned* r, unsigned* g, unsigned* b)
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x0044ed20, 0x21, SCALAR_DELETING_DTOR, Bitmap24Bit)

// Original: Bitmap24Bit::Bitmap24Bit; bitmap24.cpp:55, dc 0x52618
Bitmap24Bit::Bitmap24Bit()
    : resource(0, RESOURCE_TYPE_NONE),
      m_dataSize(0), m_imageSize(0), m_width(0), m_height(0), m_data(0)
{
}

VA(0x0044ed50, 0xAA)
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
Bitmap24Bit::~Bitmap24Bit()
{
    if (m_data)
        delete[] m_data;
}

// Original: Bitmap24Bit::import; bitmap24.cpp:101, dc 0x52794
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

// Original: Bitmap24Bit::clear; bitmap24.cpp:123, dc 0x527f0
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

VA(0x0044eef0, 0xDB)  // dc 0x5281c
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
        memcpy(m_data + y * m_width * 3,
               pcxfile.m_ibuff + y * pcxfile.m_buffwidth,
               m_width * 3);
    }

    freeimage(&pcxfile);
    return 0;
}

VA(0x0044efd0, 0x37)
void Bitmap24Bit::draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                       int dx, int dy) const
{
    draw(sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
         dst->getWidth(), dst->getHeight(), dst->getPitch());
}

// E:\gamedcs\bitmap24.cpp:280. Dreamcast proves the clipped rectangle,
// source pointer, three channel-scale locals, nested row/pixel scopes and the
// two GetPitch boundaries. Retail independently fixes the 24-bit-to-16-bit
// channel conversion and brackets this 353-byte body immediately after the
// Bitmap16Bit wrapper above.
// Row-boundary residual (85.0312%): advance source/destination only before
// a following row; final-row guards score 72.2734%, unchecked control 100%.
// DC's const source bytes and channel scales, separate GetPitch/dpitch
// boundaries and channel work are retained. Native tests
// use different pitches and clipped origins at the last allocation row.
VA(0x0044f010, 0x161)  // source-order bracket + RGB mask/data flow, dc 0x52968
void Bitmap24Bit::draw(int sx, int sy, int sw, int sh, unsigned short* dst,
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
        const unsigned char* src = m_data + sy * getPitch() + sx * 3;
        dst = static_cast<unsigned short*>(static_cast<void*>(
            static_cast<unsigned char*>(static_cast<void*>(dst))
            + dy * dpitch + dx * 2));
        // Complete's x86 relocation and byte schedule require this later-
        // revision declaration order. It retains all three DC-named scale
        // locals while assigning bm1/rm1/gm1 to retail's ESI/stack/EBX roles.
        const unsigned int rm1 = (Bitmap16Bit::s_redMask << 1) & ~Bitmap16Bit::s_redMask;
        const unsigned int gm1 = (Bitmap16Bit::s_greenMask << 1) & ~Bitmap16Bit::s_greenMask;
        const unsigned int bm1 = (Bitmap16Bit::s_blueMask << 1) & ~Bitmap16Bit::s_blueMask;

        for (int y = 0; y < sh; ++y) {
            if (y) {
                dst = static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst))
                    + dpitch));
                src += getPitch();
            }
            const unsigned char* in = src;
            unsigned short* out = dst;
            for (int x = 0; x < sw; ++x) {
                unsigned int blue =
                    ((in[0] * bm1) >> 8) & Bitmap16Bit::s_blueMask;
                unsigned int green =
                    ((in[1] * gm1) >> 8) & Bitmap16Bit::s_greenMask;
                unsigned int red =
                    ((in[2] * rm1) >> 8) & Bitmap16Bit::s_redMask;
                // The operands are commutative; this grouping is retail's
                // exact blue/red/green source-load schedule under VC6 C1.
                *out++ = static_cast<unsigned short>(blue | red | green);
                in += 3;
            }
        }
    }
}

VA(0x0044f180, 0x7)
unsigned int Bitmap24Bit::getSize() const
{
    return m_dataSize + sizeof(Bitmap24Bit);
}

// E:\gamedcs\bitmap24.cpp:349
VA(0x0044f190, 0x5F8)  // source-order bracket + inlined HSV helpers, dc 0x52aa8
void Bitmap24Bit::adjustHSV(int x, int y, int w, int h, float hue,
                            float hueAdjust, float saturationAdjust,
                            float valueAdjust)
{
    const unsigned int redNorm =
        std::numeric_limits<int>::max() / 255;
    const unsigned int greenNorm =
        std::numeric_limits<int>::max() / 255;
    const unsigned int blueNorm =
        std::numeric_limits<int>::max() / 255;

    unsigned char* src = m_data + y * getPitch() + x * 3;
    unsigned char* srcRowBase = src;
    int srcRowOffset = 0;
    for (int row = 0; row < h; ++row) {
        src = srcRowBase + srcRowOffset;
        unsigned char* pixel = src;
        for (int column = 0; column < w; ++column) {
            unsigned int r = pixel[2] * redNorm;
            unsigned int g = pixel[1] * greenNorm;
            unsigned int b = pixel[0] * blueNorm;

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

            hsvToRGB(h, s, v, &r, &g, &b);

            pixel[2] = static_cast<unsigned char>(r / redNorm);
            pixel[1] = static_cast<unsigned char>(g / greenNorm);
            pixel[0] = static_cast<unsigned char>(b / blueNorm);
            pixel += 3;
        }

        srcRowOffset += getPitch();
    }
}

// Original: RGBToHSV; bitmap24.cpp:446, dc 0x52ecc.
static void rgbToHSV(unsigned int r, unsigned int g,
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

// Original: HSVToRGB; bitmap24.cpp:481, dc 0x53084.
static void hsvToRGB(float h, float s, float v,
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

// E:\gamedcs\bitmap24.cpp:58
DC_ONLY(0x533bc, 0x34)
void* Bitmap24Bit::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

#endif  // @carcass
