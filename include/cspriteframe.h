#ifndef HOMM3_CSPRITEFRAME_H
#define HOMM3_CSPRITEFRAME_H

#include "va.h"

#include "resource.h"

class TPalette16;

// Retail layout is byte-proven by both constructors at 0x47c2b0/0x47c360
// and the destructor at 0x47c430.  The Dreamcast field roster agrees
// through map@0x44; retail omits that build's DirectDraw tail, leaving a
// 0x48-byte resource.  Retail vtable 0x63d6bc has the ordinary resource
// trio: scalar deleting destructor, Dispose, and the memory-size query.
// The blend masks live at retail .bss 0x6968a4 and 0x6968aa. Dreamcast
// CodeView proves their public static-member ownership, names, and older
// unsigned-short view. Complete preserves word writes and a word div4mask,
// while selected div2mask expressions use wider loads whose high bits
// are discarded. The next byte, 0x6968a6, is the separate RLE run code.

// Dreamcast CodeView enum 0x1772. Retail GetSprite passes the on-disk value
// through both CSpriteFrame constructor overloads, proving the same 32-bit
// domain on x86.
enum TEncodingMethod {
    eEncodeRaw = 0,
    eEncodeGeneralRLE = 1,
    eEncodeTilesetRLE = 2,
    eEncodeAdvObjRLE = 3,
    kNumEncodingMethods = 4
};

// General-RLE control values consumed by the specialized frame renderers.
// Their effects vary with the renderer and whether an outline color is live,
// so the names preserve the encoded domain rather than inventing one effect.
enum TRleControlCode {
    eRleControlShadow75 = 1,
    // Codes 2 and 3 are proven by DrawTileShadow's jump table alone: its
    // dispatch is `dec eax / cmp eax,3 / ja default / jmp [table+eax*4]`
    // over four entries at 0x47ef20, and the table pairs 1 with 2 onto the
    // three-quarter blend and 3 with 4 onto the half blend. Named for the
    // domain, not for an effect, like the outline codes below - no other
    // renderer in this compiland dispatches on either value.
    eRleControlShadow2 = 2,
    eRleControlShadow3 = 3,
    eRleControlShadow50 = 4,
    eRleControlOutline5 = 5,
    eRleControlOutline6 = 6,
    eRleControlOutline7 = 7
};

// The packed tile/adventure RLE stores a three-bit tag and a five-bit
// run length. Tag 7 introduces literal palette bytes; general RLE uses 255.
enum TPackedRleControlCode {
    ePackedRleLiteral = 7,
    ePackedRleMaxRunLength = 32
};

// Duff-loop entry selected by a raw row's width modulo eight. A zero
// remainder enters the full eight-pixel arm.
enum TRawRowUnrollEntry {
    eRawRowUnroll8 = 0,
    eRawRowUnroll1 = 1,
    eRawRowUnroll2 = 2,
    eRawRowUnroll3 = 3,
    eRawRowUnroll4 = 4,
    eRawRowUnroll5 = 5,
    eRawRowUnroll6 = 6,
    eRawRowUnroll7 = 7
};

class CSpriteFrame : public resource {
public:
    CSpriteFrame();
    CSpriteFrame(const char* name, unsigned char cropped);
    CSpriteFrame(const char* name, int w, int h, unsigned char* data,
                 int csize, TEncodingMethod encoding);
    CSpriteFrame(const char* name, int w, int h, unsigned char* data,
                 int csize, TEncodingMethod encoding,
                 int cw, int ch, int cx, int cy);
    virtual ~CSpriteFrame();
    void clear();
    unsigned char GetPixel(int x, int y) const;
    int Crop();
    void Encode(TEncodingMethod method);
    static unsigned short div2mask;
    static unsigned short div4mask;

private:
    int DataSize;
    int ImageSize;
    TEncodingMethod EncodingMethod;
    int Width;
    int Height;
    int CroppedWidth;
    int CroppedHeight;
    int CroppedX;
    int CroppedY;
    int Pitch;
    unsigned char* map;

public:
    virtual unsigned int getSize() const;
    // Original Draw/Draw* and private Clip* publics encode _N for flip,
    // transparency and alpha flags; repeated bools use mangling backreferences.
    // Keep that domain through the public wrappers and private implementations.
    void Draw(int sx, int sy, int sw, int sh, unsigned short* dst,
              int dx, int dy, int dw, int dh, int dpitch,
              TPalette16& pal, bool hflip,
              bool tblit) const;

    // CSpriteFrame.h:87-90.  DC emits standalone copies, while retail's
    // consumers expand these one-field accessors in place.
    DC_ADDRESS(0x04cb90, 0x4)
    int GetCroppedWidth() const { return CroppedWidth; }

    DC_ADDRESS(0x08734c, 0x4)
    int GetCroppedHeight() const { return CroppedHeight; }

    DC_ADDRESS(0x04cb94, 0x4)
    int GetCroppedX() const { return CroppedX; }

    DC_ADDRESS(0x04cb98, 0x4)
    int GetCroppedY() const { return CroppedY; }

    // CSpriteFrame.h:147-148. Dreamcast emits this header wrapper as a
    // standalone function; retail inlines its fixed zero-alpha forwarding.
    DC_ADDRESS(0x074068, 0x68)
    void DrawCreature(int sx, int sy, int sw, int sh,
                      unsigned short* dst, int dx, int dy, int dw, int dh,
                      int dpitch, TPalette16& pal, bool hflip,
                      unsigned short outcolor) const
    {
        DrawCreatureImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                         pal, hflip, outcolor, 0);
    }

    // Original: CSpriteFrame::DrawCreatureAlpha; CSpriteFrame.h:152
    DC_ADDRESS(0x0740d0, 0x68)
    void DrawCreatureAlpha(int sx, int sy, int sw, int sh,
                           unsigned short* dst, int dx, int dy, int dw, int dh,
                           int dpitch, TPalette16& pal, bool hflip,
                           unsigned short outcolor) const
    {
        DrawCreatureImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                         pal, hflip, outcolor, 1);
    }

    // DC CSpriteFrame.h:157..179 records each public forwarding boundary.
    // Retail CSprite 0x47bdc0..0x47c0d0 expands them and calls the private impls.
    DC_ADDRESS(0x074138, 0x60)
    void DrawAdvObj(int sx, int sy, int sw, int sh, unsigned short* dst,
                 int dx, int dy, int dw, int dh, int dpitch, TPalette16& pal, bool hflip) const
    {
        DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 0);
    }

    DC_ADDRESS(0x074198, 0x64)
    void DrawAdvObjWithFlag(int sx, int sy, int sw, int sh, unsigned short* dst,
                 int dx, int dy, int dw, int dh, int dpitch, TPalette16& pal, unsigned short flagcolor, bool hflip) const
    {
        DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, flagcolor);
    }
    void DrawAdvObjWithFlagAlpha(int sx, int sy, int sw, int sh,
                                 unsigned short* dst, int dx, int dy,
                                 int dw, int dh, int dpitch, TPalette16& pal,
                                 unsigned short flagcolor,
                                 bool hflip) const;

    DC_ADDRESS(0x0741fc, 0x5c)
    void DrawAdvObjShadow(int sx, int sy, int sw, int sh, unsigned short* dst,
                 int dx, int dy, int dw, int dh, int dpitch, TPalette16& pal, bool hflip) const
    {
        DrawAdvObjShadowImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip);
    }
    void DrawTile(int sx, int sy, int sw, int sh, unsigned short* dst,
                  int dx, int dy, int dw, int dh, int dpitch,
                  TPalette16& pal, bool hflip,
                  bool vflip) const;
    void DrawTileShadow(int sx, int sy, int sw, int sh, unsigned short* dst,
                        int dx, int dy, int dw, int dh, int dpitch,
                        TPalette16& pal, bool hflip,
                        bool vflip) const;

    DC_ADDRESS(0x074258, 0x60)
    void DrawHero(int sx, int sy, int sw, int sh, unsigned short* dst,
                 int dx, int dy, int dw, int dh, int dpitch, TPalette16& pal, bool hflip) const
    {
        DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 0);
    }

    DC_ADDRESS(0x0742b8, 0x5c)
    void DrawHeroShadow(int sx, int sy, int sw, int sh, unsigned short* dst,
                 int dx, int dy, int dw, int dh, int dpitch, TPalette16& pal, bool hflip) const
    {
        DrawAdvObjShadowImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip);
    }
    void DrawSpellEffect(int sx, int sy, int sw, int sh,
                         unsigned short* dst, int dx, int dy, int dw, int dh,
                         int dpitch, TPalette16& pal, bool hflip,
                         bool alpha) const;

    // Original: CSpriteFrame::DrawPointer; CSpriteFrame.h:182
    // Complete expands this whole-frame transparent draw in CSprite::DrawPointer.
    DC_ADDRESS(0x074314, 0x68)
    void DrawPointer(unsigned short* dst, int dx, int dy, int dw, int dh,
                     int dpitch, TPalette16& pal, bool hflip) const
    {
        Draw(0, 0, Width, Height, dst, dx, dy, dw, dh, dpitch,
             pal, hflip, 1);
    }

    // Original: CSpriteFrame::DrawInterface; CSpriteFrame.h:187
    DC_ADDRESS(0x07437c, 0x60)
    void DrawInterface(int sx, int sy, int sw, int sh, unsigned short* dst,
                       int dx, int dy, int dw, int dh, int dpitch,
                       TPalette16& pal, bool hflip) const
    {
        Draw(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 1);
    }

    // Original: CSpriteFrame::DrawShroudTile; CSpriteFrame.h:192
    // The original public ends in _N2: both flip parameters are bool.
    // Its palette/frame parameter lifetimes span both retained retail calls.
    DC_ADDRESS(0x0743dc, 0xa8)
    void DrawShroudTile(int sx, int sy, int sw, int sh, unsigned short* dst,
                        int dx, int dy, int dw, int dh, int dpitch,
                        TPalette16& pal, bool hflip,
                        bool vflip) const
    {
        DrawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, vflip);
        DrawTileShadow(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                       pal, hflip, vflip);
    }
    // DC CSpriteFrame field list records these public const accessors;
    // CSprite::DrawPointer expands the corresponding retail width/height loads.
    // Width loads in retail CSprite::DrawPointer identify this field.
    // The DC declaration survives, but no body source location does.
    // Header ownership is provisional; no source order is claimed.
    // @dc-declaration-only: 0x1799
    int GetWidth() const { return Width; }
    // Height loads in retail CSprite::DrawPointer identify this field.
    // The DC declaration survives, but no body source location does.
    // Header ownership is provisional; no source order is claimed.
    // @dc-declaration-only: 0x1799
    int GetHeight() const { return Height; }

    // DC CSpriteFrame.h:198..200: canonical zero-flag wrapper.
    // DrawSpellEffect calls it at DC line 3792; retail expands the wrapper
    // and calls DrawAdvObjWithFlagAlpha with flagcolor=0 at 0x47efca.
    DC_ADDRESS(0x074484, 0x60)
    void DrawHeroAlpha(int sx, int sy, int sw, int sh, unsigned short* dst,
                       int dx, int dy, int dw, int dh, int dpitch,
                       TPalette16& pal, bool hflip) const
    {
        DrawAdvObjWithFlagAlpha(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                                pal, 0, hflip);
    }
    void DrawAdvObjWithFlagScaled50(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal, unsigned short flagcolor) const;
    void DrawAdvObjWithFlagScaled25(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal, unsigned short flagcolor) const;
    void DrawAdvObjShadowScaled50(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal) const;
    void DrawAdvObjShadowScaled25(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal) const;
    void DrawTileScaled50(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal, bool hflip, bool vflip) const;
    void DrawTileScaled25(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal, bool hflip, bool vflip) const;
    static void SetPixelFormat(unsigned rmask, unsigned gmask,
                               unsigned bmask);

private:
    int importPCXFile(const char* filename);
    int importCroppedPCXFile(const char* filename);
    void EncodeGeneral();
    void EncodeTileset();
    void EncodeAdvObj();
    void ClipScaled25(int& sx, int& sy, int& sw, int& sh, int& dx, int& dy,
        int dw, int dh, bool hflip, bool vflip) const;
    void ClipScaled50(int& sx, int& sy, int& sw, int& sh, int& dx, int& dy,
        int dw, int dh, bool hflip, bool vflip) const;
    void DrawCreatureImpl(int sx, int sy, int sw, int sh,
                          unsigned short* dst, int dx, int dy, int dw,
                          int dh, int dpitch, TPalette16& pal,
                          bool hflip, unsigned short outcolor,
                          bool alpha) const;
    void DrawAdvObjImpl(int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh,
                        int dpitch, TPalette16& pal, bool hflip,
                        unsigned short flagcolor) const;
    void DrawAdvObjShadowImpl(int sx, int sy, int sw, int sh,
                              unsigned short* dst, int dx, int dy, int dw,
                              int dh, int dpitch, TPalette16& pal,
                              bool hflip) const;
    void Clip(int& sx, int& sy, int& sw, int& sh, int& dx, int& dy,
              int dw, int dh, bool hflip,
              bool vflip) const;
};
SIZE(CSpriteFrame, 0x48);

#endif  /* HOMM3_CSPRITEFRAME_H */
