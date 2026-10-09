#ifndef HOMM3_CSPRITE_H
#define HOMM3_CSPRITE_H

#include "va.h"

#include "bitmap16.h"
#include "csequence.h"
#include "cspriteframe.h"
#include "palette.h"
#include "resource.h"

class palette;
class paletteHiColor;
class TPalette24;

// Creature sprite sequence ids (DC CodeView enum creature_seqid,
// NH3API creatures.hpp identical); GetSequenceID/GetSequenceName retain
// the complete file-format vocabulary.
enum creature_seqid {
    cs_walk = 0,
    cs_fidget = 1,
    cs_wait = 2,
    cs_wince = 3,
    cs_defend = 4,
    // DC creature_seqid.cs_death. combatManager::PowEffect (0x468990)
    // proves the value three times over: it selects sequence 5 exactly
    // when a stack's bAllUnitsKilled is up, budgets that sequence's
    // frame count into the same wince slot, and excludes 5 alongside
    // cs_wait from the frames that fall back to idle - a dead stack
    // holds its last frame.
    cs_death = 5,
    cs_specdeath = 6,
    cs_turn_rf = 7,
    cs_turn_fr = 8,
    cs_turn_lf = 9,
    cs_turn_fl = 10,
    cs_attack_ur = 11,
    cs_attack_r = 12,
    cs_attack_dr = 13,
    cs_range_ur = 14,
    cs_range_r = 15,
    cs_range_dr = 16,
    // DC creature_seqid.cs_special_{ur,r,dr} (dump enum 0x4962), the
    // cast-animation triple. Byte-proven by army::cast_spell
    // (0x448260): it selects 17/18/19 from the missile angle and falls
    // back to the cs_attack triple when the sprite has no frames for
    // them.
    cs_special_ur = 17,
    cs_special_r = 18,
    cs_special_dr = 19,
    cs_prewalk = 0x14,
    cs_postwalk = 0x15
};

// CSprite::GetSequenceID (DC csprite.cpp:605, 0x73468) and GetSequenceName
// (774, 0x73880) map these five named combat-hero animation sequences to 0..4.
enum CombatHeroSequence {
    combatHeroStand = 0,
    combatHeroFidget = 1,
    combatHeroDefeat = 2,
    combatHeroVictory = 3,
    combatHeroCast = 4
};

// Live VIEW (grown from the button.h bootstrap). Retail layout proven
// by consumers: GetPalette (0x47bcc0) returns p ? p + 0x1c : 0;
// button::Draw reads s@0x1c (CSequence**), numSequences@0x28,
// validSeqMask@0x2c, Width@0x30, Height@0x34; the button-family dtors
// call Dispose through slot 1. The DC roster names the fields (retail
// dropped SpecialCacheFlag/Sp_loaded and hoisted s ahead of p).
// Retail vtable 0x63d6b0: slot 0 = scalar deleting dtor (0x47b8f0),
// slot 1 = Dispose (0x55d1a0), slot 2 = resource size (0x47bd50).
class CSprite : public resource {
public:
    CSprite();
    CSprite(const char* name, int sprtype, int w, int h);
    virtual ~CSprite();  // slot 0

    // CSprite.h:145. DrawWallAt expands this DC header accessor at its
    // archer site; the retail load is the Width dword above.
    DC_ADDRESS(0x01f148, 0x2c)
    int GetWidth() const { return Width; }

    // The adjacent size accessor is expanded throughout drawing's hex-
    // targeted spell animation; Dreamcast retains out-of-line copies of
    // both accessors while retail VC6 folds them to the two dword loads.
    DC_ADDRESS(0x01f174, 0x2c)
    int GetHeight() const { return Height; }
    void clear();
    void AllocateSeq(int seqnum, int numFrames);
    int AddFrame(int seqnum, CSpriteFrame* frame);
    void AddFrame(int seqnum, const char* name);
    int AddFrame(int seqnum, const char* name, int w, int h,
                 unsigned char* data, int csize, TEncodingMethod encoding,
                 int croppedWidth, int croppedHeight, int croppedX, int croppedY);
    int AddFrame(int seqnum, const char* name, int w, int h,
                 unsigned char* data, int csize, TEncodingMethod encoding);

private:
    CSequence** s;

public:
    TPalette16* p;
    // DC CodeView type 0x17d1 is TPalette24*. Retail ResetPalette confirms
    // it by passing p24+0x1c (the resource head) to the raw palette ctor.
    TPalette24* p24;

private:
    int numSequences;
    int* validSeqMask;
    int Width;
    int Height;

public:
    virtual void dispose();
    virtual unsigned int getSize() const;  // slot 2, retail 0x47bd50

    // CSprite.h:148-151.  The Dreamcast image carries out-of-line copies;
    // the retail remote caller expands these in place.
    DC_ADDRESS(0x04cb9c, 0x44)
    int GetCroppedX(int seq, int frame) const
    {
        return s[seq]->f[frame]->GetCroppedX();
    }

    DC_ADDRESS(0x04cbe0, 0x44)
    int GetCroppedY(int seq, int frame) const
    {
        return s[seq]->f[frame]->GetCroppedY();
    }

    DC_ADDRESS(0x04cc24, 0x44)
    int GetCroppedWidth(int seq, int frame) const
    {
        return s[seq]->f[frame]->GetCroppedWidth();
    }

    DC_ADDRESS(0x087350, 0x44)
    int GetCroppedHeight(int seq, int frame) const
    {
        return s[seq]->f[frame]->GetCroppedHeight();
    }

    // DC CSprite.h:154 proves this non-const header accessor.
    // Complete's dispose frame loop expands the same sequence/frame loads.
    DC_ADDRESS(0x122ba8, 0xe)
    CSpriteFrame* GetFrame(int sequence, int frame)
    {
        return s[sequence]->f[frame];
    }

    // Original: CSprite::SetPixelFormat; CSprite.h:157
    DC_ADDRESS(0x122bb8, 0x18)
    static void SetPixelFormat(unsigned int rmask, unsigned int gmask, unsigned int bmask)
    {
        CSpriteFrame::SetPixelFormat(rmask, gmask, bmask);
    }
    // CodeView LF_MFUNCTION marks every Draw-family receiver const.
    // Drawing writes through the destination/frame pointers, not this object.
    // Retained Draw* publics encode _N for flip/alpha/transparency flags
    // in both raw-buffer and Bitmap16Bit overloads.
    void Draw(int seqnum, int framenum, int sx, int sy, int sw, int sh,
              unsigned short* dst, int dx, int dy, int dw, int dh,
              int dpitch, bool hflip, bool tblit) const;
    void DrawCreature(int seqnum, int framenum, int sx, int sy, int sw,
                      int sh, unsigned short* dst, int dx, int dy, int dw,
                      int dh, int dpitch, bool hflip,
                      unsigned short outcolor) const;
    void DrawCreatureAlpha(int seqnum, int framenum, int sx, int sy,
                           int sw, int sh, unsigned short* dst, int dx, int dy,
                           int dw, int dh, int dpitch, bool hflip,
                           unsigned short outcolor) const;
    void DrawAdvObj(int framenum, int sx, int sy, int sw, int sh,
                    unsigned short* dst, int dx, int dy, int dw, int dh,
                    int dpitch, bool hflip) const;
    // Original DrawAdvObjWithFlag public encodes G_N: color then bool flip.
    void DrawAdvObjWithFlag(int framenum, int sx, int sy, int sw, int sh,
                            unsigned short* dst, int dx, int dy, int dw,
                            int dh, int dpitch, unsigned short outcolor,
                            bool hflip) const;
    void DrawAdvObjWithFlagAlpha(int framenum, int sx, int sy, int sw, int sh,
                                 unsigned short* dst, int dx, int dy, int dw,
                                 int dh, int dpitch, unsigned short outcolor,
                                 bool hflip) const;
    void DrawAdvObjShadow(int framenum, int sx, int sy, int sw, int sh,
                          unsigned short* dst, int dx, int dy, int dw,
                          int dh, int dpitch, bool hflip) const;
    void DrawPointer(int framenum, unsigned short* dst, int dx, int dy,
                     int dw, int dh, int dpitch, bool hflip) const;
    void DrawInterface(int framenum, int sx, int sy, int sw, int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch, bool hflip) const;
    void DrawTile(int framenum, int sx, int sy, int sw, int sh,
                  unsigned short* dst, int dx, int dy, int dw, int dh,
                  int dpitch, bool hflip, bool vflip) const;
    // Both TileShadow/ShroudTile overload publics encode _N for both flips.
    void DrawTileShadow(int framenum, int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh,
                        int dpitch, bool hflip,
                        bool vflip) const;
    void DrawShroudTile(int framenum, int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh,
                        int dpitch, bool hflip,
                        bool vflip) const;
    void DrawHero(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                  unsigned short* dst, int dx, int dy, int dw, int dh,
                  int dpitch, bool hflip) const;
    void DrawHeroShadow(int seqnum, int framenum, int sx, int sy, int sw,
                        int sh, unsigned short* dst, int dx, int dy, int dw,
                        int dh, int dpitch, bool hflip) const;
    void DrawHeroAlpha(int seqnum, int framenum, int sx, int sy, int sw,
                       int sh, unsigned short* dst, int dx, int dy, int dw,
                       int dh, int dpitch, bool hflip) const;
    void DrawCombatHero(int seqnum, int framenum, int sx, int sy, int sw,
                        int sh, unsigned short* dst, int dx, int dy, int dw,
                        int dh, int dpitch, bool hflip) const;
    void DrawSpellEffect(int seqnum, int framenum, int sx, int sy, int sw,
                         int sh, unsigned short* dst, int dx, int dy, int dw,
                         int dh, int dpitch, bool hflip,
                         bool alpha) const;
    void SetPalette(const unsigned short* pal);

    // Complete expands this wrapper in ResetPalette.
    // E:\gamedcs\CSprite.h:259
    DC_ADDRESS(0x0744e4, 0x64)
    void SetPalette(TPalette16& pal)
    {
        if (p)
            delete p;
        p = new TPalette16(&pal);
    }
    void ResetPalette();
    unsigned short* GetPalette();
    const unsigned short* GetPalette() const;
    void ColorCycle(int begin, int end, int step);
    void DrawAdvObjWithFlagScaled50(int framenum, int sx, int sy, int sw,
        int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        unsigned short outcolor) const;
    void DrawAdvObjWithFlagScaled25(int framenum, int sx, int sy, int sw,
        int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        unsigned short outcolor) const;
    void DrawAdvObjShadowScaled50(int framenum, int sx, int sy, int sw,
        int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch) const;
    void DrawAdvObjShadowScaled25(int framenum, int sx, int sy, int sw,
        int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch) const;
    void DrawTileScaled50(int framenum, int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        bool hflip, bool vflip) const;
    void DrawTileScaled25(int framenum, int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        bool hflip, bool vflip) const;
    static int GetSpriteType(const char* name);
    static const char* GetSpriteTypeName(const int type);
    static int GetNumSeqs(int type);
    static int GetSequenceID(int type, const char* name);
    static const char* GetSequenceName(int type, int num);

    // Original GetPalette24, CSprite.h:284.
    DC_ADDRESS(0x057dbc, 0x24)
    TPalette24& GetPalette24() { return *p24; }

    // Original: CSprite::GetPaletteColor; CSprite.h:287
    // Complete keeps the sprite resident. UpdateRadar's two expansions
    // read the palette directly; DC's removed reload/cache guard is absent.
    DC_ADDRESS(0x01f1a0, 0x3c)
    unsigned short GetPaletteColor(unsigned char index) const
    {
        return p->Palette[index];
    }

    // Header inline, DC CSprite.h:293 (emitted into
    // advmgr.obj there). Byte-proven by iconwdgt's frame walkers: each
    // USE re-expands the guard (the else arm constant-folds to a
    // literal 0 divisor, `xor ecx,ecx; idiv ecx`), which a cached
    // frame-count local cannot reproduce.
    // DC 0x1f1fc calls IsValidSeq; its true/false values join at 0x1f220
    // before a single return. Keep that helper and conditional expression.
    // An if/return spelling spills the third boat-row divisor into a
    // parameter home; this expression closes both VWDrawHeroPart twins.
    // The preceding DC SpriteDataReload guard belongs to its removed cache
    // fields; Complete's frame walkers have no corresponding reload arm.
    DC_ADDRESS(0x01f1dc, 0x58)
    int GetNumFrames(int seq) const
    {
        return IsValidSeq(seq) ? s[seq]->numFrames : 0;
    }

    // E:\gamedcs\CSprite.h:294
    // The attack-frame chooser uses this header boundary rather than reading
    // numSequences/validSeqMask directly. Retail VC6 folds it back to the
    // same two loads and tests at each constant-sequence call site.
    DC_ADDRESS(0x01f234, 0x32)
    int IsValidSeq(int seqnum) const
    {
        return seqnum < numSequences && validSeqMask[seqnum];
    }

    VA(0x004f0050, 0x47)  // COMDAT owner (kb.obj emits ?Draw@CSprite@@QBEXHHHHHHPAVBitmap16Bit@@HHEE@Z), body in csprite.h
    DC_ADDRESS(0x01f268, 0xbc)
    void Draw(int seqnum, int framenum, int sx, int sy, int sw, int sh,
              Bitmap16Bit* dst, int dx, int dy, bool hflip,
              bool tblit) const
    {
        Draw(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
             dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip,
             tblit);
    }

    // CSprite.h:342 header wrapper (DC retains its own copy); retail expands the Bitmap16Bit forwarding in remote.
    DC_ADDRESS(0x087394, 0xa4)
    void DrawCreature(int seqnum, int framenum, int sx, int sy, int sw,
                      int sh, Bitmap16Bit* dst, int dx, int dy,
                      bool hflip, unsigned short outcolor) const
    {
        DrawCreature(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                     dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip, outcolor);
    }

    // Original: CSprite::DrawCreatureAlpha; CSprite.h:348
    DC_ADDRESS(0x087438, 0xa4)
    void DrawCreatureAlpha(int seqnum, int framenum, int sx, int sy, int sw,
        int sh, Bitmap16Bit* dst, int dx, int dy, bool hflip,
        unsigned short outcolor) const
    {
        DrawCreatureAlpha(seqnum, framenum, sx, sy, sw, sh,
            dst->GetMap(0, 0), dx, dy, dst->GetWidth(), dst->GetHeight(),
            dst->GetPitch(), hflip, outcolor);
    }

    // DC CSprite.h:355 calls all four Bitmap16Bit accessors before the
    // raw-map overload. Preserve those nested boundaries in retail callers.
    DC_ADDRESS(0x01f324, 0xa4)
    void DrawAdvObj(int framenum, int sx, int sy, int sw, int sh,
                    Bitmap16Bit* dst, int dx, int dy, bool hflip) const
    {
        DrawAdvObj(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                   dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }

    DC_ADDRESS(0x01f3c8, 0xb8)
    void DrawAdvObjWithFlag(int framenum, int sx, int sy, int sw, int sh,
                            Bitmap16Bit* dst, int dx, int dy,
                            unsigned short outcolor, bool hflip) const
    {
        DrawAdvObjWithFlag(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                           dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), outcolor,
                           hflip);
    }

    DC_ADDRESS(0x01f480, 0xa4)
    void DrawAdvObjShadow(int framenum, int sx, int sy, int sw, int sh,
                          Bitmap16Bit* dst, int dx, int dy,
                          bool hflip) const
    {
        DrawAdvObjShadow(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                         dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }

    // CSprite.h:378..381: bitmap DrawPointer facade.
    DC_ADDRESS(0x0d9f98, 0x80)
    void DrawPointer(int framenum, Bitmap16Bit* dst, int dx, int dy,
                     bool hflip) const
    {
        DrawPointer(framenum, dst->GetMap(0, 0), dx, dy,
                    dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }

    // Header wrapper (DC CSprite.h:385). KeyAccel's four expanded call sites
    // byte-prove the Bitmap16Bit member forwarding in retail.
    DC_ADDRESS(0x01f524, 0xa4)
    void DrawInterface(int framenum, int sx, int sy, int sw, int sh,
                       Bitmap16Bit* dst, int dx, int dy,
                       bool hflip) const
    {
        DrawInterface(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                      dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }

    // DC CSprite.h:393 forwards through the same four bitmap accessors.
    DC_ADDRESS(0x01f5c8, 0xb8)
    void DrawTile(int framenum, int sx, int sy, int sw, int sh,
                  Bitmap16Bit* dst, int dx, int dy, bool hflip,
                  bool vflip) const
    {
        DrawTile(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                 dst->GetWidth(), dst->GetHeight(), dst->GetPitch(),
                 hflip, vflip);
    }

    DC_ADDRESS(0x01f680, 0xb8)
    void DrawTileShadow(int framenum, int sx, int sy, int sw, int sh,
                        Bitmap16Bit* dst, int dx, int dy,
                        bool hflip, bool vflip) const
    {
        DrawTileShadow(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                       dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip, vflip);
    }

    DC_ADDRESS(0x01f738, 0xb8)
    void DrawShroudTile(int framenum, int sx, int sy, int sw, int sh,
                        Bitmap16Bit* dst, int dx, int dy,
                        bool hflip, bool vflip) const
    {
        DrawShroudTile(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                       dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip, vflip);
    }

    // Header wrapper (DC CSprite.h:426): retail advmgr inlines this view,
    // then calls the raw-map overload above.
    DC_ADDRESS(0x01f7f0, 0xb4)
    void DrawHero(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                  Bitmap16Bit* dst, int dx, int dy, bool hflip) const
    {
        DrawHero(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                 dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }

    // DC publics at 0x1f8a4 and 0x72d98 encode _N for hflip in both
    // DrawHeroShadow overloads; T_UCHAR debug lowering is not source uchar.
    DC_ADDRESS(0x01f8a4, 0xb4)
    void DrawHeroShadow(int seqnum, int framenum, int sx, int sy, int sw,
                        int sh, Bitmap16Bit* dst, int dx, int dy,
                        bool hflip) const
    {
        DrawHeroShadow(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                       dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }
    // E:\\gamedcs\\CSprite.h:438. DrawCursorAlpha reaches the bitmap
    // overload four times; Dreamcast's line table shows this header boundary
    // and Complete expands it into the raw map/width/height/pitch call.
    // DC 0x7a1e8 in DrawCursorAlpha expands the bitmap overload declared
    // by function type 0x17e5, loading its map/width/height/pitch and calling
    // the raw DrawHeroAlpha member. The same row recurs at three more sites.
    // The inlined facade carries the raw overload's bool flip domain;
    // only the latter retains an independently typed original public.
    // @dc-inline-origin: 0x17e5 0x7a1e8
    void DrawHeroAlpha(int seqnum, int framenum, int sx, int sy, int sw,
                       int sh, Bitmap16Bit* dst, int dx, int dy,
                       bool hflip) const
    {
        DrawHeroAlpha(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                      dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }

    // DC CSprite.h:444/445: const bitmap facade.
    // Complete's combatManager::DrawCombatHero (0x4952b0) calls the general
    // CSprite::DrawCreature (0x47bd60) with color zero. This version bypasses
    // the older raw-map DrawCombatHero overload.
    DC_ADDRESS(0x0874dc, 0xa0)
    void DrawCombatHero(int seqnum, int framenum, int sx, int sy, int sw,
                        int sh, Bitmap16Bit* dst, int dx, int dy,
                        bool hflip) const
    {
        DrawCreature(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
                     dst->GetWidth(), dst->GetHeight(), dst->GetPitch(),
                     hflip, 0);
    }

    // CSprite.h:450/451 (DC drawing.obj) preserves the same
    // bitmap forwarding boundary. The public suffix HH_N1@Z proves both
    // Boolean parameters; its four bitmap accessors remain source calls.
    DC_ADDRESS(0x08757c, 0xa4)
    void DrawSpellEffect(int seqnum, int framenum, int sx, int sy, int sw,
                         int sh, Bitmap16Bit* dst, int dx, int dy,
                         bool hflip, bool alpha) const
    {
        DrawSpellEffect(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0),
                        dx, dy, dst->GetWidth(), dst->GetHeight(),
                        dst->GetPitch(), hflip, alpha);
    }
};

#endif  /* HOMM3_CSPRITE_H */
