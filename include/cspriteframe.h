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
    unsigned char getPixel(int x, int y) const;
    int crop();
    void encode(TEncodingMethod method);
    static unsigned short s_div2mask;
    static unsigned short s_div4mask;

private:
    int m_dataSize;
    int m_imageSize;
    TEncodingMethod m_encodingMethod;
    int m_width;
    int m_height;
    int m_croppedWidth;
    int m_croppedHeight;
    int m_croppedX;
    int m_croppedY;
    int m_pitch;
    unsigned char* m_map;

public:
    virtual unsigned int getSize() const;
    void draw(int sx, int sy, int sw, int sh, unsigned short* dst,
              int dx, int dy, int dw, int dh, int dpitch,
              TPalette16& pal, unsigned char hflip,
              unsigned char tblit) const;

    // CSpriteFrame.h:87-90.  DC emits standalone copies, while retail's
    // consumers expand these one-field accessors in place.
    DC_ADDRESS(0x04cb90, 0x4)
    int getCroppedWidth() const { return m_croppedWidth; }

    DC_ADDRESS(0x08734c, 0x4)
    int getCroppedHeight() const { return m_croppedHeight; }

    DC_ADDRESS(0x04cb94, 0x4)
    int getCroppedX() const { return m_croppedX; }

    DC_ADDRESS(0x04cb98, 0x4)
    int getCroppedY() const { return m_croppedY; }

    // CSpriteFrame.h:147-148. Dreamcast emits this header wrapper as a
    // standalone function; retail inlines its fixed zero-alpha forwarding.
    DC_ADDRESS(0x074068, 0x68)
    void drawCreature(int sx, int sy, int sw, int sh,
                      unsigned short* dst, int dx, int dy, int dw, int dh,
                      int dpitch, TPalette16& pal, unsigned char hflip,
                      unsigned short outcolor) const
    {
        drawCreatureImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                         pal, hflip, outcolor, 0);
    }

    // Original: CSpriteFrame::DrawCreatureAlpha; CSpriteFrame.h:152
    DC_ADDRESS(0x0740d0, 0x68)
    void drawCreatureAlpha(int sx, int sy, int sw, int sh,
                           unsigned short* dst, int dx, int dy, int dw, int dh,
                           int dpitch, TPalette16& pal, unsigned char hflip,
                           unsigned short outcolor) const
    {
        drawCreatureImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                         pal, hflip, outcolor, 1);
    }

    // DC CSpriteFrame.h:157..179 records each public forwarding boundary.
    // Retail CSprite 0x47bdc0..0x47c0d0 expands them and calls the private impls.
    DC_ADDRESS(0x074138, 0x60)
    void drawAdvObj(int sx, int sy, int sw, int sh, unsigned short* dst,
                 int dx, int dy, int dw, int dh, int dpitch, TPalette16& pal, unsigned char hflip) const
    {
        drawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 0);
    }

    DC_ADDRESS(0x074198, 0x64)
    void drawAdvObjWithFlag(int sx, int sy, int sw, int sh, unsigned short* dst,
                 int dx, int dy, int dw, int dh, int dpitch, TPalette16& pal, unsigned short flagcolor, bool hflip) const
    {
        drawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, flagcolor);
    }
    void drawAdvObjWithFlagAlpha(int sx, int sy, int sw, int sh,
                                 unsigned short* dst, int dx, int dy,
                                 int dw, int dh, int dpitch, TPalette16& pal,
                                 unsigned short flagcolor,
                                 unsigned char hflip) const;

    DC_ADDRESS(0x0741fc, 0x5c)
    void drawAdvObjShadow(int sx, int sy, int sw, int sh, unsigned short* dst,
                 int dx, int dy, int dw, int dh, int dpitch, TPalette16& pal, unsigned char hflip) const
    {
        drawAdvObjShadowImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip);
    }
    void drawTile(int sx, int sy, int sw, int sh, unsigned short* dst,
                  int dx, int dy, int dw, int dh, int dpitch,
                  TPalette16& pal, unsigned char hflip,
                  unsigned char vflip) const;
    void drawTileShadow(int sx, int sy, int sw, int sh, unsigned short* dst,
                        int dx, int dy, int dw, int dh, int dpitch,
                        TPalette16& pal, bool hflip,
                        bool vflip) const;

    DC_ADDRESS(0x074258, 0x60)
    void drawHero(int sx, int sy, int sw, int sh, unsigned short* dst,
                 int dx, int dy, int dw, int dh, int dpitch, TPalette16& pal, unsigned char hflip) const
    {
        drawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 0);
    }

    DC_ADDRESS(0x0742b8, 0x5c)
    void drawHeroShadow(int sx, int sy, int sw, int sh, unsigned short* dst,
                 int dx, int dy, int dw, int dh, int dpitch, TPalette16& pal, unsigned char hflip) const
    {
        drawAdvObjShadowImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip);
    }
    void drawSpellEffect(int sx, int sy, int sw, int sh,
                         unsigned short* dst, int dx, int dy, int dw, int dh,
                         int dpitch, TPalette16& pal, unsigned char hflip,
                         unsigned char alpha) const;

    // Original: CSpriteFrame::DrawPointer; CSpriteFrame.h:182
    // Complete expands this whole-frame transparent draw in CSprite::drawPointer.
    DC_ADDRESS(0x074314, 0x68)
    void drawPointer(unsigned short* dst, int dx, int dy, int dw, int dh,
                     int dpitch, TPalette16& pal, unsigned char hflip) const
    {
        draw(0, 0, m_width, m_height, dst, dx, dy, dw, dh, dpitch,
             pal, hflip, 1);
    }

    // Original: CSpriteFrame::DrawInterface; CSpriteFrame.h:187
    DC_ADDRESS(0x07437c, 0x60)
    void drawInterface(int sx, int sy, int sw, int sh, unsigned short* dst,
                       int dx, int dy, int dw, int dh, int dpitch,
                       TPalette16& pal, unsigned char hflip) const
    {
        draw(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 1);
    }

    // Original: CSpriteFrame::DrawShroudTile; CSpriteFrame.h:192
    // The original public ends in _N2: both flip parameters are bool.
    // Its palette/frame parameter lifetimes span both retained retail calls.
    DC_ADDRESS(0x0743dc, 0xa8)
    void drawShroudTile(int sx, int sy, int sw, int sh, unsigned short* dst,
                        int dx, int dy, int dw, int dh, int dpitch,
                        TPalette16& pal, bool hflip,
                        bool vflip) const
    {
        drawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, vflip);
        drawTileShadow(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                       pal, hflip, vflip);
    }
    // DC CSpriteFrame field list records these public const accessors;
    // CSprite::drawPointer expands the corresponding retail width/height loads.
    // Width loads in retail CSprite::drawPointer identify this field.
    // The DC declaration survives, but no body source location does.
    // Header ownership is provisional; no source order is claimed.
    // @dc-declaration-only: 0x1799
    int getWidth() const { return m_width; }
    // Height loads in retail CSprite::drawPointer identify this field.
    // The DC declaration survives, but no body source location does.
    // Header ownership is provisional; no source order is claimed.
    // @dc-declaration-only: 0x1799
    int getHeight() const { return m_height; }

    // DC CSpriteFrame.h:198..200: canonical zero-flag wrapper.
    // DrawSpellEffect calls it at DC line 3792; retail expands the wrapper
    // and calls DrawAdvObjWithFlagAlpha with flagcolor=0 at 0x47efca.
    DC_ADDRESS(0x074484, 0x60)
    void drawHeroAlpha(int sx, int sy, int sw, int sh, unsigned short* dst,
                       int dx, int dy, int dw, int dh, int dpitch,
                       TPalette16& pal, unsigned char hflip) const
    {
        drawAdvObjWithFlagAlpha(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                                pal, 0, hflip);
    }
    void drawAdvObjWithFlagScaled50(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal, unsigned short flagcolor) const;
    void drawAdvObjWithFlagScaled25(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal, unsigned short flagcolor) const;
    void drawAdvObjShadowScaled50(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal) const;
    void drawAdvObjShadowScaled25(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal) const;
    void drawTileScaled50(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal, unsigned char hflip, unsigned char vflip) const;
    void drawTileScaled25(int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        TPalette16& pal, unsigned char hflip, unsigned char vflip) const;
    static void setPixelFormat(unsigned rmask, unsigned gmask,
                               unsigned bmask);

private:
    int importPCXFile(const char* filename);
    int importCroppedPCXFile(const char* filename);
    void encodeGeneral();
    void encodeTileset();
    void encodeAdvObj();
    void clipScaled25(int& sx, int& sy, int& sw, int& sh, int& dx, int& dy,
        int dw, int dh, unsigned char hflip, unsigned char vflip) const;
    void clipScaled50(int& sx, int& sy, int& sw, int& sh, int& dx, int& dy,
        int dw, int dh, unsigned char hflip, unsigned char vflip) const;
    void drawCreatureImpl(int sx, int sy, int sw, int sh,
                          unsigned short* dst, int dx, int dy, int dw,
                          int dh, int dpitch, TPalette16& pal,
                          unsigned char hflip, unsigned short outcolor,
                          unsigned char alpha) const;
    void drawAdvObjImpl(int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh,
                        int dpitch, TPalette16& pal, unsigned char hflip,
                        unsigned short flagcolor) const;
    void drawAdvObjShadowImpl(int sx, int sy, int sw, int sh,
                              unsigned short* dst, int dx, int dy, int dw,
                              int dh, int dpitch, TPalette16& pal,
                              unsigned char hflip) const;
    void clip(int& sx, int& sy, int& sw, int& sh, int& dx, int& dy,
              int dw, int dh, unsigned char hflip,
              unsigned char vflip) const;
};
SIZE(CSpriteFrame, 0x48);

#endif  /* HOMM3_CSPRITEFRAME_H */
