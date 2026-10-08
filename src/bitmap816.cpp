// Bitmap816.cpp - Bitmap816 (Loki h3maped object 38; the object has no
// __FILE__ string, so the file name follows the class).
//
// The Loki link order: the constructors, destructor, import, clear, the
// stubbed PCX import, the draw family and the palette setters; g++ then
// emits the Bitmap816.h inline members.
#include "va.h"

#include <string.h>

#include "bitmap816.h"

#include "bitmap16.h"

DC_ADDRESS(0x053854, 0x10c)
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

VA(0x0044f800, 0xCA)
DC_ADDRESS(0x053960, 0x106)
MAC_ADDRESS(0x05daf0, 0xc4)
Bitmap816::Bitmap816(const char* name, int w, int h, unsigned char* data,
                     const TPalette16& palette16, int dataSize)
    : resource(name, RESOURCE_TYPE_BITMAP),
      m_imageSize(w * h), m_width(w), m_height(h), m_pitch(w),
      m_p16(palette16)
{
    m_dataSize = dataSize ? dataSize : m_imageSize;
    m_map = new unsigned char[m_dataSize];
    if (m_map)
        memcpy(m_map, data, m_dataSize);
}

DC_ADDRESS(0x053a68, 0xa4)
Bitmap816::Bitmap816(const char* name, int rbits, int rshift,
                     int gbits, int gshift, int bbits, int bshift)
    : resource(name, RESOURCE_TYPE_BITMAP),
      m_dataSize(0), m_imageSize(0), m_width(0), m_height(0), m_pitch(0),
      m_map(0)
{
    importPCXFile(name, rbits, rshift, gbits, gshift, bbits, bshift);
}

VA(0x0044f8d0, 0xF8)
DC_ADDRESS(0x053b0c, 0x96)
Bitmap816::Bitmap816(const char* name, const char* path,
                     int rbits, int rshift, int gbits, int gshift,
                     int bbits, int bshift)
    : resource(name, RESOURCE_TYPE_BITMAP),
      m_dataSize(0), m_imageSize(0), m_width(0), m_height(0), m_pitch(0),
      m_map(0)
{
    char filename[1025];
    strcpy(filename, path);
    strcat(filename, name);
    importPCXFile(filename, rbits, rshift, gbits, gshift, bbits, bshift);
}

VA(0x0044f9d0, 0x70)
DC_ADDRESS(0x053ba4, 0xb8)
MAC_ADDRESS(0x05dbb4, 0x88)
Bitmap816::~Bitmap816()
{
    if (m_map)
        delete[] m_map;
}

DC_ADDRESS(0x053c5c, 0x104)
void Bitmap816::import(int w, int h, unsigned char* data,
                       const TPalette16& p16, int size)
{
    clear();
    m_width = w;
    m_height = h;
    m_pitch = w;
    m_imageSize = w * h;
    m_dataSize = size ? size : m_imageSize;
    m_map = new unsigned char[m_dataSize];
    if (m_map)
        memcpy(m_map, data, m_dataSize);
    m_p16 = TPalette16(p16);
}

DC_ADDRESS(0x053d60, 0x90)
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

// The Loki port has no PCX reader.
VA(0x0044fa40, 0x155)
DC_ADDRESS(0x053df0, 0x1f2)
int Bitmap816::importPCXFile(const char* filename, int rbits, int rshift,
                             int gbits, int gshift, int bbits, int bshift)
{
    return 2;
}

VA(0x0044fba0, 0xCB)
DC_ADDRESS(0x053fe4, 0xc4)
MAC_ADDRESS(0x05dc3c, 0xf8)
void Bitmap816::zBufferDraw(int sx, int sy, int sw, int sh,
                            unsigned short* dst, int dx, int dy,
                            int dw, int dh, int dpitch, int id) const
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

    const unsigned char* src = m_map + sy * m_pitch + sx;
    dst = (unsigned short*)((unsigned char*)dst + dy * dpitch
                            + dx * sizeof(unsigned short));
    const unsigned short* palette = m_p16.m_data;
    for (int y = 0; y < sh; y++) {
        unsigned short* out = dst;
        const unsigned char* in = src;
        for (int x = 0; x < sw; x++) {
            unsigned char pixel = *in++;
            if (pixel)
                *out = id;
            out++;
        }
        dst = (unsigned short*)((unsigned char*)dst + dpitch);
        src += m_pitch;
    }
}

// Loki draws the 5:5:5 palette onto a 5:6:5 display.
VA(0x0044fc70, 0x136)
DC_ADDRESS(0x0540a8, 0x108)
MAC_ADDRESS(0x05dd34, 0x21c)
void Bitmap816::Draw(int sx, int sy, int sw, int sh, unsigned short* dst,
                     int dx, int dy, int dw, int dh, int dpitch,
                     bool tblit) const
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

    const unsigned char* src = m_map + sy * m_pitch + sx;
    dst = (unsigned short*)((unsigned char*)dst + dy * dpitch
                            + dx * sizeof(unsigned short));
    const unsigned short* palette = m_p16.m_data;
    if (tblit) {
        for (int y = 0; y < sh; y++) {
            unsigned short* out = dst;
            const unsigned char* in = src;
            for (int x = 0; x < sw; x++) {
                unsigned char pixel = *in++;
                if (pixel)
                    *out = convert555to565(palette[pixel]);
                out++;
            }
            dst = (unsigned short*)((unsigned char*)dst + dpitch);
            src += m_pitch;
        }
    } else {
        for (int y = 0; y < sh; y++) {
            unsigned short* out = dst;
            const unsigned char* in = src;
            for (int x = 0; x < sw; x++)
                *out++ = convert555to565(palette[*in++]);
            dst = (unsigned short*)((unsigned char*)dst + dpitch);
            src += m_pitch;
        }
    }
}

VA(0x0044fdb0, 0x3B)
DC_ADDRESS(0x0541b0, 0x7a)
MAC_ADDRESS(0x05df50, 0x44)
void Bitmap816::Draw(int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                     int dx, int dy, bool tblit) const
{
    Draw(sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy, dst->GetWidth(),
         dst->GetHeight(), dst->GetPitch(), tblit);
}

VA(0x0044fdf0, 0x3B)
DC_ADDRESS(0x05422c, 0x4e)
MAC_ADDRESS(0x05df94, 0x40)
void Bitmap816::zBufferDraw(int sx, int sy, int sw, int sh,
                            unsigned short* zBuffer, int dx, int dy,
                            int id) const
{
    zBufferDraw(sx, sy, sw, sh, zBuffer, dx, dy, 800, 600, 1600, id);
}

VA(0x0044fe40, 0x18)
DC_ADDRESS(0x05427c, 0x18)
MAC_ADDRESS(0x05dfe0, 0x34)
void Bitmap816::SetPalette(const unsigned short* pal)
{
    memcpy(m_p16.m_data, pal, sizeof(m_p16.m_data));
}

VA(0x0044fe60, 0x16)
DC_ADDRESS(0x054294, 0x6)
MAC_ADDRESS(0x05e014, 0x24)
void Bitmap816::SetPalette(const TPalette24& pal24)
{
    m_p24 = pal24;
}

VA(0x0044fe80, 0x40)
DC_ADDRESS(0x05429c, 0x64)
MAC_ADDRESS(0x05e038, 0x4c)
void Bitmap816::ResetPalette()
{
    TPalette16 converted(GetPalette24());
    SetPalette(converted.m_data);
}
