#ifndef HOMM3_CSPRITEFRAME_H
#define HOMM3_CSPRITEFRAME_H

#include "resource.h"

class TPalette16;

// CSpriteFrame.h of the Loki port (RoE source). Names follow the Loki
// symbols and the assert text of CSpriteFrame.cpp ("CroppedWidth %
// kCellWidth == 0", "EncodingMethod == eEncodeRaw"); the default arguments
// are those __PRETTY_FUNCTION__ prints ("bool = false, bool = true").
// Layout: resource (name, type, references, vptr at 0x18), then the frame
// fields 0x1c..0x44, 0x48 bytes in all. The inline members are emitted, in
// this declaration order, after CSpriteFrame.o's static initializer.

// Dreamcast CodeView enum 0x1772; Encode asserts
// "method > eEncodeRaw && method < kNumEncodingMethods".
enum TEncodingMethod {
    eEncodeRaw = 0,
    eEncodeGeneralRLE = 1,
    eEncodeTilesetRLE = 2,
    eEncodeAdvObjRLE = 3,
    kNumEncodingMethods = 4
};

class CSpriteFrame : public resource {
public:
    CSpriteFrame();
    CSpriteFrame(const char* name, int w, int h, unsigned char* data,
                 int csize, TEncodingMethod encoding = eEncodeRaw);
    CSpriteFrame(const char* name, int w, int h, unsigned char* data,
                 int csize, TEncodingMethod encoding,
                 int cw, int ch, int cx, int cy);
    CSpriteFrame(const char* name, bool cropped);
    virtual ~CSpriteFrame();

    void clear();
    static void SetPixelFormat(unsigned int rmask, unsigned int gmask,
                               unsigned int bmask);

    unsigned char GetPixel(int x, int y) const;
    int Crop();
    void Encode(TEncodingMethod method);

    int GetDataSize() const { return DataSize; }
    int GetImageSize() const { return ImageSize; }
    TEncodingMethod GetEncodingMethod() const { return EncodingMethod; }
    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }
    int GetCroppedWidth() const { return CroppedWidth; }
    int GetCroppedHeight() const { return CroppedHeight; }
    int GetCroppedX() const { return CroppedX; }
    int GetCroppedY() const { return CroppedY; }
    int GetPitch() const { return Pitch; }
    unsigned char* GetMap() { return map; }

    void Draw(int sx, int sy, int sw, int sh, unsigned short* dst,
              int dx, int dy, int dw, int dh, int dpitch,
              TPalette16& pal, bool hflip = false, bool tblit = true) const;

    void DrawCreature(int sx, int sy, int sw, int sh, unsigned short* dst,
                      int dx, int dy, int dw, int dh, int dpitch,
                      TPalette16& pal, bool hflip, unsigned short outcolor) const
    {
        DrawCreatureImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                         pal, hflip, outcolor, false);
    }

    void DrawCreatureAlpha(int sx, int sy, int sw, int sh, unsigned short* dst,
                           int dx, int dy, int dw, int dh, int dpitch,
                           TPalette16& pal, bool hflip, unsigned short outcolor) const
    {
        DrawCreatureImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
                         pal, hflip, outcolor, true);
    }

    void DrawAdvObj(int sx, int sy, int sw, int sh, unsigned short* dst,
                    int dx, int dy, int dw, int dh, int dpitch,
                    TPalette16& pal, bool hflip) const
    {
        DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 0);
    }

    void DrawAdvObjWithFlag(int sx, int sy, int sw, int sh, unsigned short* dst,
                            int dx, int dy, int dw, int dh, int dpitch,
                            TPalette16& pal, unsigned short flagcolor, bool hflip) const
    {
        DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, flagcolor);
    }

    void DrawAdvObjWithFlagAlpha(int sx, int sy, int sw, int sh, unsigned short* dst,
                                 int dx, int dy, int dw, int dh, int dpitch,
                                 TPalette16& pal, unsigned short flagcolor,
                                 bool hflip = false) const;

    void DrawAdvObjShadow(int sx, int sy, int sw, int sh, unsigned short* dst,
                          int dx, int dy, int dw, int dh, int dpitch,
                          TPalette16& pal, bool hflip) const
    {
        DrawAdvObjShadowImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip);
    }

    void DrawHero(int sx, int sy, int sw, int sh, unsigned short* dst,
                  int dx, int dy, int dw, int dh, int dpitch,
                  TPalette16& pal, bool hflip) const
    {
        DrawAdvObjImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 0);
    }

    void DrawHeroShadow(int sx, int sy, int sw, int sh, unsigned short* dst,
                        int dx, int dy, int dw, int dh, int dpitch,
                        TPalette16& pal, bool hflip) const
    {
        DrawAdvObjShadowImpl(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip);
    }

    void DrawPointer(unsigned short* dst, int dx, int dy, int dw, int dh,
                     int dpitch, TPalette16& pal, bool hflip) const
    {
        Draw(0, 0, Width, Height, dst, dx, dy, dw, dh, dpitch, pal, hflip, true);
    }

    void DrawInterface(int sx, int sy, int sw, int sh, unsigned short* dst,
                       int dx, int dy, int dw, int dh, int dpitch,
                       TPalette16& pal, bool hflip) const
    {
        Draw(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, true);
    }

    void DrawTile(unsigned short* dst, int dx, int dy, int dpitch,
                  TPalette16& pal, bool hflip, bool vflip) const;
    void DrawTile(int sx, int sy, int sw, int sh, unsigned short* dst,
                  int dx, int dy, int dw, int dh, int dpitch,
                  TPalette16& pal, bool hflip = false, bool vflip = false) const;
    void DrawTileShadow(int sx, int sy, int sw, int sh, unsigned short* dst,
                        int dx, int dy, int dw, int dh, int dpitch,
                        TPalette16& pal, bool hflip = false, bool vflip = false) const;

    void DrawShroudTile(int sx, int sy, int sw, int sh, unsigned short* dst,
                        int dx, int dy, int dw, int dh, int dpitch,
                        TPalette16& pal, bool hflip, bool vflip) const
    {
        DrawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, vflip);
        DrawTileShadow(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, vflip);
    }

    void DrawSpellEffect(int sx, int sy, int sw, int sh, unsigned short* dst,
                         int dx, int dy, int dw, int dh, int dpitch,
                         TPalette16& pal, bool hflip = false, bool alpha = false) const;

    void DrawHeroAlpha(int sx, int sy, int sw, int sh, unsigned short* dst,
                       int dx, int dy, int dw, int dh, int dpitch,
                       TPalette16& pal, bool hflip) const
    {
        DrawAdvObjWithFlagAlpha(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, 0, hflip);
    }

    void DrawCombatHero(int sx, int sy, int sw, int sh, unsigned short* dst,
                        int dx, int dy, int dw, int dh, int dpitch,
                        TPalette16& pal, bool hflip) const
    {
        DrawCreature(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, pal, hflip, 0);
    }

    void DrawAdvObjWithFlagScaled50(int sx, int sy, int sw, int sh, unsigned short* dst,
                                    int dx, int dy, int dw, int dh, int dpitch,
                                    TPalette16& pal, unsigned short flagcolor) const;
    void DrawAdvObjShadowScaled50(int sx, int sy, int sw, int sh, unsigned short* dst,
                                  int dx, int dy, int dw, int dh, int dpitch,
                                  TPalette16& pal) const;
    void DrawTileScaled50(int sx, int sy, int sw, int sh, unsigned short* dst,
                          int dx, int dy, int dw, int dh, int dpitch,
                          TPalette16& pal, bool hflip = false, bool vflip = false) const;
    void DrawAdvObjWithFlagScaled25(int sx, int sy, int sw, int sh, unsigned short* dst,
                                    int dx, int dy, int dw, int dh, int dpitch,
                                    TPalette16& pal, unsigned short flagcolor) const;
    void DrawAdvObjShadowScaled25(int sx, int sy, int sw, int sh, unsigned short* dst,
                                  int dx, int dy, int dw, int dh, int dpitch,
                                  TPalette16& pal) const;
    void DrawTileScaled25(int sx, int sy, int sw, int sh, unsigned short* dst,
                          int dx, int dy, int dw, int dh, int dpitch,
                          TPalette16& pal, bool hflip = false, bool vflip = false) const;

    static unsigned short div2mask;
    static unsigned short div4mask;

private:
    int importPCXFile(const char* filename);
    int importCroppedPCXFile(const char* filename);
    void EncodeGeneral();
    void EncodeTileset();
    void EncodeAdvObj();

    void DrawCreatureImpl(int sx, int sy, int sw, int sh, unsigned short* dst,
                          int dx, int dy, int dw, int dh, int dpitch,
                          TPalette16& pal, bool hflip, unsigned short outcolor,
                          bool alpha) const;
    void DrawAdvObjImpl(int sx, int sy, int sw, int sh, unsigned short* dst,
                        int dx, int dy, int dw, int dh, int dpitch,
                        TPalette16& pal, bool hflip, unsigned short flagcolor) const;
    void DrawAdvObjShadowImpl(int sx, int sy, int sw, int sh, unsigned short* dst,
                              int dx, int dy, int dw, int dh, int dpitch,
                              TPalette16& pal, bool hflip) const;

    void Clip(int& sx, int& sy, int& sw, int& sh, int& dx, int& dy,
              int dw, int dh, bool hflip, bool vflip) const;
    void ClipScaled50(int& sx, int& sy, int& sw, int& sh, int& dx, int& dy,
                      int dw, int dh, bool hflip, bool vflip) const;
    void ClipScaled25(int& sx, int& sy, int& sw, int& sh, int& dx, int& dy,
                      int dw, int dh, bool hflip, bool vflip) const;

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
};

#endif  /* HOMM3_CSPRITEFRAME_H */
