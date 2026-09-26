// 17 functions in link order.
// <new> for its `void __cdecl operator delete(void*) _THROW0();` - the
// nothrow declaration is what retail's ~Bitmap816 unwind map proves this TU
// had (see the destructor's note below). NEW.H does NOT declare it.
#include "va.h"

#include <new>
#include <string.h>

#include "bitmap816.h"

#include "bitmap16.h"
#include "pcx.h"

VA_COMPGEN(0x0044f7d0, 0x21, SCALAR_DELETING_DTOR, Bitmap816)

// Original: Bitmap816::Bitmap816; bitmap816.cpp:36, dc 0x53854
// DC allocates a padded, locked DirectDraw surface when available. Complete
// removes that per-bitmap surface tail: retained constructor 0x44f800 owns
// the byte buffer directly, and destructor 0x44f9d0 releases it with delete[].
Bitmap816::Bitmap816(int w, int h)
    : resource(0, RESOURCE_TYPE_NONE),
      m_imageSize(w * h), m_width(w), m_height(h), m_pitch(w)
{
    m_dataSize = m_imageSize;
    if (w && h)
        m_map = new unsigned char[m_dataSize];
    else
        m_map = 0;
}

VA(0x0044f800, 0xCA)  // dc 0x53960
Bitmap816::Bitmap816(const char* name, int w, int h, unsigned char* data,
                     TPalette16* palette16, int dataSize)
    : resource(name, RESOURCE_TYPE_BITMAP),
      m_imageSize(w * h), m_width(w), m_height(h), m_pitch(w), m_p16(palette16)
{
    m_dataSize = dataSize ? dataSize : m_imageSize;
    m_map = new unsigned char[m_dataSize];
    if (m_map)
        memcpy(m_map, data, m_dataSize);
}

// Original: Bitmap816::Bitmap816; bitmap816.cpp:125, dc 0x53a68
Bitmap816::Bitmap816(const char* name, int rbits, int rshift,
                     int gbits, int gshift, int bbits, int bshift)
    : resource(name, RESOURCE_TYPE_BITMAP),
      m_dataSize(0), m_imageSize(0), m_width(0), m_height(0), m_pitch(0), m_map(0)
{
    importPCXFile(name, rbits, rshift, gbits, gshift, bbits, bshift);
}

VA(0x0044f8d0, 0xF8)  // dc 0x53b0c
Bitmap816::Bitmap816(const char* name, const char* path,
                     int rbits, int rshift, int gbits, int gshift,
                     int bbits, int bshift)
    : resource(name, RESOURCE_TYPE_BITMAP),
      m_dataSize(0), m_imageSize(0), m_width(0), m_height(0), m_pitch(0), m_map(0)
{
    char filename[264];

    strcpy(filename, path);
    strcat(filename, name);
    importPCXFile(filename, rbits, rshift, gbits, gshift, bbits, bshift);
}

VA(0x0044f9d0, 0x70)  // dc 0x53ba4
Bitmap816::~Bitmap816()
{
    if (m_map)
        delete[] m_map;
}
// The initial EH state (retail 1, ours was 2) was a CLEANUP-COUNT fact, not
// a spelling. Retail's unwind map for this body has exactly two entries and
// the funclets say which: 0x628600 is `mov ecx,[ebp-0x10]; jmp ~resource`
// and 0x628608 is `add ecx,0x34; jmp ~TPalette16`. There is no
// `add ecx,0x250; jmp ~TPalette24` funclet - although the Bitmap816
// CONSTRUCTOR group at 0x6285a0 has all three (8/0xb/0xe bytes). VC6 emits
// a cleanup chain only for a region that can THROW, so retail's two entries
// say (a) this body's delete cannot throw - the TU sees <new>'s
// `operator delete(void*) _THROW0()` rather than the implicit, throwing
// declaration - and (b) the ~TPalette24 call CAN, which is why that
// destructor carries no exception specification. Measured: empty body -> 1
// entry; <new> alone -> 1; <new> plus a throwing ~TPalette24 -> 2, retail.

// Original: Bitmap816::import; bitmap816.cpp:163, dc 0x53c5c
void Bitmap816::import(int w, int h, unsigned char* data,
                       TPalette16& p16, int size)
{
    clear();
    m_width = w;
    m_height = h;
    m_pitch = w;
    m_imageSize = w * h;
    m_dataSize = size ? size : m_imageSize;
    if (w && h)
        m_map = new unsigned char[m_dataSize];
    if (m_map)
        memcpy(m_map, data, m_dataSize);
    // DC copies through its reference-taking palette temporary. Complete
    // embeds the palette and uses the pointer-taking payload assignment
    // (0x522910), which preserves this bitmap palette's resource identity.
    m_p16 = &p16;
}

// Original: Bitmap816::clear; bitmap816.cpp:221, dc 0x53d60
void Bitmap816::clear()
{
    m_width = 0;
    m_height = 0;
    m_pitch = 0;
    m_dataSize = 0;
    m_imageSize = 0;
    if (m_map) {
        delete[] m_map;
        m_map = 0;
    }
}

VA(0x0044fa40, 0x155)  // dc 0x53df0
int Bitmap816::importPCXFile(const char* filename, int rbits, int rshift,
                             int gbits, int gshift, int bbits, int bshift)
{
    PcxData pdat;
    imgdes pcxfile;
    int error = pcxinfo(filename, &pdat);
    if (error)
        return 1;

    m_width = pdat.m_width;
    m_height = pdat.m_length;
    m_pitch = m_width;
    m_imageSize = m_width * m_height;
    m_dataSize = m_imageSize;
    m_map = new unsigned char[m_imageSize];
    if (!m_map)
        return 2;

    allocimage(&pcxfile, pdat.m_width, pdat.m_length,
               pdat.m_bpPixel * pdat.m_nplanes);
    loadpcx(filename, &pcxfile);
    flipimage(&pcxfile, &pcxfile);

    for (int y = 0; y < m_height; ++y) {
        memcpy(m_map + y * m_pitch,
               pcxfile.m_ibuff + y * pcxfile.m_buffwidth,
               m_width);
    }

    for (int i = 0; i < 256; ++i) {
        m_p16.m_data[i] =
            static_cast<unsigned short>(
                ((pcxfile.m_palette[i].rgbRed >> (8 - rbits)) << rshift) |
                ((pcxfile.m_palette[i].rgbGreen >> (8 - gbits)) << gshift) |
                ((pcxfile.m_palette[i].rgbBlue >> (8 - bbits)) << bshift));
    }

    freeimage(&pcxfile);
    return 0;
}

VA(0x0044fba0, 0xCB)  // dc 0x53fe4
void Bitmap816::zBufferDraw(int sx, int sy, int sw, int sh,
                            unsigned short* dst, int dx, int dy,
                            int dw, int dh, int dpitch, int id) const
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
        unsigned char* src = m_map + sy * m_pitch + sx;
        dst = static_cast<unsigned short*>(static_cast<void*>(
            static_cast<unsigned char*>(static_cast<void*>(dst))
            + dy * dpitch + dx * 2));
        for (int y = 0; y < sh; ++y) {
            unsigned short* out = dst;
            unsigned char* in = src;
            for (int x = 0; x < sw; ++x) {
                unsigned char pixel = *in++;
                if (pixel)
                    *out = static_cast<unsigned short>(id);
                ++out;
            }
            dst = static_cast<unsigned short*>(static_cast<void*>(
                static_cast<unsigned char*>(static_cast<void*>(dst))
                + dpitch));
            src += m_pitch;
        }
    }
}

VA(0x0044fc70, 0x136)  // dc 0x540a8
void Bitmap816::draw(int sx, int sy, int sw, int sh, unsigned short* dst,
                     int dx, int dy, int dw, int dh, int dpitch,
                     bool tblit) const
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
        unsigned char* src = m_map + sy * m_pitch + sx;
        dst = static_cast<unsigned short*>(static_cast<void*>(
            static_cast<unsigned char*>(static_cast<void*>(dst))
            + dy * dpitch + dx * 2));
        if (tblit) {
            for (int y = 0; y < sh; ++y) {
                unsigned short* out = dst;
                unsigned char* in = src;
                for (int x = 0; x < sw; ++x) {
                    unsigned char pixel = *in++;
                    if (pixel)
                        *out = m_p16.m_data[pixel];
                    ++out;
                }
                dst = static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst))
                    + dpitch));
                src += m_pitch;
            }
        } else {
            for (int y = 0; y < sh; ++y) {
                unsigned short* out = dst;
                unsigned char* in = src;
                for (int x = 0; x < sw; ++x) {
                    *out++ = m_p16.m_data[*in++];
                }
                dst = static_cast<unsigned short*>(static_cast<void*>(
                    static_cast<unsigned char*>(static_cast<void*>(dst))
                    + dpitch));
                src += m_pitch;
            }
        }
    }
}

VA(0x0044fdb0, 0x3B)  // dc 0x541b0
void Bitmap816::draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                     int dx, int dy, bool tblit) const
{
    draw(sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
         dst->getWidth(), dst->getHeight(), dst->getPitch(), tblit);
}

VA(0x0044fdf0, 0x3B)  // dc 0x5422c
void Bitmap816::zBufferDraw(int sx, int sy, int sw, int sh,
                            unsigned short* zBuffer, int dx, int dy,
                            int id) const
{
    zBufferDraw(sx, sy, sw, sh, zBuffer, dx, dy, 800, 600, 1600, id);
}

// Retail vtable 0x63ba14 slot 2. Complete adds this resource virtual;
// DC Bitmap816 type 0x105e / fields 0x244e has no GetSize member.
VA(0x0044fe30, 0x09)
unsigned int Bitmap816::getSize() const
{
    return m_dataSize + sizeof(*this);
}

VA(0x0044fe40, 0x18)
void Bitmap816::setPalette(const unsigned short* pal)
{
    memcpy(m_p16.m_data, pal, sizeof(m_p16.m_data));
}

VA(0x0044fe60, 0x16)  // dc 0x54294
void Bitmap816::setPalette(TPalette24* pal24)
{
    m_p24 = *pal24;
}

VA(0x0044fe80, 0x40)  // dc 0x5429c
void Bitmap816::resetPalette()
{
    TPalette16 converted(m_p24);
    setPalette(converted.m_data);
}
