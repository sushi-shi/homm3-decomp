// palette.h - prototypes of palette.cpp (compiland palette.obj)
#ifndef HOMM3_PALETTE_H
#define HOMM3_PALETTE_H

#include "hsv.h"
#include "resource.h"

// Dreamcast CodeView type 0x184c; retail's TPalette24 constructor consumes
// this exact four-byte stride and copies the first three channels.
// Before normalization (type): TRGBA.
struct RGBA {
    unsigned char m_red;
    unsigned char m_green;
    unsigned char m_blue;
    unsigned char m_alpha;
};
SIZE(RGBA, 4);

void rgbToHSV(unsigned int r, unsigned int g, unsigned int b,
              float* h, float* s, float* v);
void hsvToRGB(float h, float s, float v,
              unsigned int* r, unsigned int* g, unsigned int* b);

// Bootstrap VIEW of the 16-bit palette resource: the RGB555 table
// lives at +0x1c past the resource head (same shape CSprite::GetPalette
// exposes); Dispose is the shared resource slot 1.
// Raw palette records (distinct from the resource-derived TPalette*
// classes): the 16-bit record's 0x200-byte extent is byte-proven by
// Bitmap816's embedded pair at +0x50/+0x250
// (bitmapBorder::SetPlayerPaletteColors 0x450520). Storage only;
// names provisional.
class Palette {
public:
    unsigned short m_data[256];
};

class PaletteHiColor {
public:
    unsigned char m_data[256][3];
};

// Retail constructors at 0x522e80 and 0x522f00 copy 0x300 palette bytes
// to/from +0x1c; vtable slot 2 at 0x522f70 returns the resulting 0x31c
// total extent. This is the resource-owning 24-bit palette class, distinct
// from the same-sized paletteHiColor raw record embedded in Bitmap816.
class Palette24 : public Resource {
public:
    Palette24();
    Palette24(const unsigned char* data);
    Palette24(const RGBA* rgba);
    // DC LF_MEMBER Palette at +0x1c, type 0x1a26: unsigned char[768].
    // Retail copies the same 0x300-byte payload; preserve the native array.
    unsigned char m_palette[768];
    Palette24(const Palette24* copy);
    Palette24& operator=(const Palette24& from);
    // NO exception specification: Bitmap816::~Bitmap816's retail unwind map
    // keeps a {p16, resource} cleanup chain across the ~TPalette24 call,
    // which VC6 only emits when that call is allowed to throw. A throw()
    // here erases that chain (the map collapses to one entry).
    virtual ~Palette24();
    virtual unsigned int getSize() const;
    void adjustHSV(float hue, float hueAdjust, float saturationAdjust,
                   float valueAdjust);
};
SIZE(Palette24, 0x31c);

class Palette16 : public Resource {
    // Dreamcast CodeView names these three class statics directly. Retail's
    // SetPixelFormat stores its red/green/blue arguments at the corresponding
    // three addresses, and every 16-bit palette transform reads them back.
private:
    static unsigned int s_redMask;
    static unsigned int s_greenMask;
    static unsigned int s_blueMask;

public:
    union {
        unsigned short m_data[256];
        Palette m_colors;
    };
    Palette16();
    Palette16(const unsigned short* data);
    Palette16(const Palette24& p24);
    Palette16(const Palette24& p24,
               int rbits, int rshift, int gbits, int gshift,
               int bbits, int bshift);
    Palette16(const RGBA* rgba,
               int rbits, int rshift, int gbits, int gshift,
               int bbits, int bshift);
    Palette16(const char* name, const Palette24& p24,
               int rbits, int rshift, int gbits, int gshift,
               int bbits, int bshift);

    // Retail 0x522650 (22 B): the resource base with an empty name and
    // type 0, plus the vptr store. Declared here because font embeds a
    // TPalette16 BY VALUE (DC LF_MEMBER `Palette`, offset 0x103c) and
    // its constructor 0x4b5070 runs this body on that subobject as a
    // member initializer. Declaration only - the body stays palette's.
    // DC Palette.h:137-140, dc 0x122b08. No receiver; three mask stores.
    // Retail ResourceManager::setPixelFormat expands this header helper.
    static void setPixelFormat(unsigned int red, unsigned int green, unsigned int blue)
    {
        s_redMask = red;
        s_greenMask = green;
        s_blueMask = blue;
    }
    Palette16(const Palette16* copy);

    // Retail 0x522940 reinstalls the TPalette16 vptr and tail-calls the
    // resource destructor. Keeping this out-of-line declaration is also
    // codegen-significant for owners of embedded palettes (font::~font).
    virtual ~Palette16();

    virtual unsigned int getSize() const;

    Palette16* operator=(const Palette16* from);
    void cycle(int begin, int end, int step);
    void gray();
    void adjustSaturation(float amount);
    void adjustHSV(float hue, float hueAdjust, float saturationAdjust,
                   float valueAdjust);

private:
    // DC palette.cpp:210 (dc 0x10a910). Retail keeps NO out-of-line copy -
    // /Ob2 expanded it into each of its constructor call sites - but the
    // boundary is the DC roster's own, not an invention.
    void convert24to16(const unsigned char* p24, int rbits, int rshift,
                       int gbits, int gshift, int bbits, int bshift);
};

// The system palette pointer, re-declared here beside its type for
// consumers that need no townmgr surface (army::DrawToBuffer reads the
// highlight color out of it). The DATA claim stays on townmgr.h's
// declaration - this one is declaration only.
extern Palette16* g_systemPalette;
// Dreamcast publishes these as gPlayerPalette/gPlayerPalette24. Retail
// oldmain stores the consecutive Players.pal results at 0x6aaca8/0x6aacac.
extern Palette16* g_playerPalette;
extern Palette24* g_playerPalette24;

// Dreamcast ?GetPalette@ResourceManager@@YAPAVTPalette16@@PBD_N@Z
// (retail body 0x55b3e0 takes just the name; called by button::Main).
namespace ResourceManager {
Palette16* getPalette(const char* name);
}

// --- globals ---
// CODEVIEW(E:\gamedcs\palette.cpp:45, dc 0x10a244) long ftol(double d);

// --- TPalette16 ---
// CODEVIEW(E:\gamedcs\palette.cpp:73, dc 0x10a3ac) void TPalette16::TPalette16(const TRGBA* rgba, int rbits, int rshift, int gbits, int gshift, int bbits, int bshift);
// CODEVIEW(E:\gamedcs\palette.cpp:79, dc 0x10a41c) void TPalette16::TPalette16(const tagRGBQUAD* quad, int rbits, int rshift, int gbits, int gshift, int bbits, int bshift);
// CODEVIEW(E:\gamedcs\palette.cpp:116, dc 0x10a5e0) void TPalette16::TPalette16(const TRGBA* rgba);
// CODEVIEW(E:\gamedcs\palette.cpp:140, dc 0x10a6a4) void TPalette16::TPalette16(const tagRGBQUAD* quad);
// CODEVIEW(E:\gamedcs\palette.cpp:165, dc 0x10a77c) void TPalette16::TPalette16(const char* name, const TPalette24& p24);
// CODEVIEW(E:\gamedcs\palette.cpp:210, dc 0x10a910) void TPalette16::Convert24to16(const unsigned char* p24, int rbits, int rshift, int gbits, int gshift, int bbits, int bshift);
// CODEVIEW(E:\gamedcs\palette.cpp:236, dc 0x10a998) void TPalette16::ConvertRGBAto16(const TRGBA* rgba, int rbits, int rshift, int gbits, int gshift, int bbits, int bshift);
// CODEVIEW(E:\gamedcs\palette.cpp:262, dc 0x10aa18) void TPalette16::ConvertRGBQUADto16(const tagRGBQUAD* quad, int rbits, int rshift, int gbits, int gshift, int bbits, int bshift);
// CODEVIEW(E:\gamedcs\palette.cpp:315, dc 0x10ab44) void TPalette16::Colorize(float hue, float saturation);
// CODEVIEW(E:\gamedcs\palette.cpp:360, dc 0x10af5c) void TPalette16::AdjustHue(float hue, float amount);
// CODEVIEW(E:\gamedcs\palette.cpp:454, dc 0x10b320) void TPalette16::AdjustValue(float amount);
// CODEVIEW(E:\gamedcs\palette.cpp:57, dc 0x10c8b0) void* TPalette16::`scalar deleting destructor'(unsigned __flags);

// --- TPalette24 ---
// CODEVIEW(E:\gamedcs\palette.cpp:598, dc 0x10b898) void TPalette24::TPalette24();
// CODEVIEW(E:\gamedcs\palette.cpp:622, dc 0x10b9c4) void TPalette24::TPalette24(const tagRGBQUAD* quad);
// CODEVIEW(E:\gamedcs\palette.cpp:640, dc 0x10ba88) TPalette24* TPalette24::operator=(const TPalette24* from);
// CODEVIEW(E:\gamedcs\palette.cpp:650, dc 0x10baac) void TPalette24::~TPalette24();
// CODEVIEW(E:\gamedcs\palette.cpp:655, dc 0x10baf0) void TPalette24::Cycle(int begin, int end, int step);
// CODEVIEW(E:\gamedcs\palette.cpp:685, dc 0x10bbf4) void TPalette24::Colorize(float hue, float saturation);
// CODEVIEW(E:\gamedcs\palette.cpp:723, dc 0x10bf58) void TPalette24::Gray();
// CODEVIEW(E:\gamedcs\palette.cpp:599, dc 0x10c8e4) void* TPalette24::`scalar deleting destructor'(unsigned __flags);

#endif  /* HOMM3_PALETTE_H */
