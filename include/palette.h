#ifndef HOMM3_PALETTE_H
#define HOMM3_PALETTE_H

#include "va.h"

#include "hsv.h"
#include "resource.h"

// Dreamcast CodeView type 0x184c; retail's TPalette24 constructor consumes
// this exact four-byte stride and copies the first three channels.
struct tagRGBQUAD;

struct TRGBA {
    unsigned char m_red;
    unsigned char m_green;
    unsigned char m_blue;
    unsigned char m_alpha;
};
SIZE(TRGBA, 4);

// Bootstrap VIEW of the 16-bit palette resource: the RGB555 table
// lives at +0x1c past the resource head (same shape CSprite::GetPalette
// exposes); Dispose is the shared resource slot 1.
// Raw palette records (distinct from the resource-derived TPalette*
// classes): the 16-bit record's 0x200-byte extent is byte-proven by
// Bitmap816's embedded pair at +0x50/+0x250
// (bitmapBorder::SetPlayerPaletteColors 0x450520). Storage only;
// names provisional.
class palette {
public:
    unsigned short m_data[256];
};

class paletteHiColor {
public:
    unsigned char m_data[256][3];
};

// Retail constructors at 0x522e80 and 0x522f00 copy 0x300 palette bytes
// to/from +0x1c; vtable slot 2 at 0x522f70 returns the resulting 0x31c
// total extent. This is the resource-owning 24-bit palette class, distinct
// from the same-sized paletteHiColor raw record embedded in Bitmap816.
class TPalette24 : public resource {
public:
    TPalette24();
    TPalette24(const unsigned char* data);
    TPalette24(const TRGBA* rgba);
    TPalette24(const tagRGBQUAD* quad);
    TPalette24(const TPalette24& copy);
    TPalette24& operator=(const TPalette24& from);
    // Loki's resource has only the virtual destructor (vtable 0x8427334).
    virtual ~TPalette24();
    void Cycle(int begin, int end, int step);
    void Colorize(float hue, float saturation);
    void Gray();
    void AdjustHSV(float hue, float hueAdjust, float saturationAdjust,
                   float valueAdjust);

    // DC LF_MEMBER Palette at +0x1c, type 0x1a26: unsigned char[768].
    unsigned char m_palette[768];
};
SIZE(TPalette24, 0x31c);

class TPalette16 : public resource {
public:
    TPalette16();
    TPalette16(const unsigned short* data);
    TPalette16(const TPalette24& p24, int rbits, int rshift,
               int gbits, int gshift, int bbits, int bshift);
    TPalette16(const TRGBA* rgba, int rbits, int rshift,
               int gbits, int gshift, int bbits, int bshift);
    TPalette16(const tagRGBQUAD* quad, int rbits, int rshift,
               int gbits, int gshift, int bbits, int bshift);
    TPalette16(const char* name, const TPalette24& p24,
               int rbits, int rshift, int gbits, int gshift,
               int bbits, int bshift);
    TPalette16(const TPalette24& p24);
    TPalette16(const TRGBA* rgba);
    TPalette16(const tagRGBQUAD* quad);
    TPalette16(const char* name, const TPalette24& p24);
    TPalette16(const TPalette16& copy);
    TPalette16& operator=(const TPalette16& from);
    // Loki's resource has only the virtual destructor (vtable 0x8427324).
    virtual ~TPalette16();

    void Convert24to16(const unsigned char* p24, int rbits, int rshift,
                       int gbits, int gshift, int bbits, int bshift);
    void ConvertRGBAto16(const TRGBA* rgba, int rbits, int rshift,
                         int gbits, int gshift, int bbits, int bshift);
    void ConvertRGBQUADto16(const tagRGBQUAD* quad, int rbits, int rshift,
                            int gbits, int gshift, int bbits, int bshift);
    void Cycle(int begin, int end, int step);
    void Colorize(float hue, float saturation);
    void AdjustHue(float hue, float amount);
    void AdjustSaturation(float amount);
    void AdjustValue(float amount);
    void AdjustHSV(float hue, float hueAdjust, float saturationAdjust,
                   float valueAdjust);
    void Gray();

    // Loki exports these statics as TPalette16::red_mask/green_mask/blue_mask.
    // DC Palette.h:137-140; Loki emits it after palette.cpp's own functions.
    DC_ADDRESS(0x122b08, 0x1c)
    static void SetPixelFormat(unsigned int red, unsigned int green,
                               unsigned int blue)
    {
        red_mask = red;
        green_mask = green;
        blue_mask = blue;
    }
    static unsigned int red_mask;
    static unsigned int green_mask;
    static unsigned int blue_mask;

    union {
        unsigned short m_data[256];
        palette m_colors;
    };
};

// The system palette pointer, re-declared here beside its type for
// consumers that need no townmgr surface (army::DrawToBuffer reads the
// highlight color out of it). The DATA claim stays on townmgr.h's
// declaration - this one is declaration only.
extern TPalette16* g_systemPalette;
// Dreamcast publishes these as gPlayerPalette/gPlayerPalette24. Retail
// oldmain stores the consecutive Players.pal results at 0x6aaca8/0x6aacac.
extern TPalette16* g_playerPalette;
extern TPalette24* g_playerPalette24;

// Dreamcast ?GetPalette@ResourceManager@@YAPAVTPalette16@@PBD_N@Z
// (retail body 0x55b3e0 takes just the name; called by button::Main).
namespace ResourceManager {
TPalette16* getPalette(const char* name);
}

#endif  /* HOMM3_PALETTE_H */
