#ifndef HOMM3_CSPRITE_H
#define HOMM3_CSPRITE_H

#include "bitmap16.h"
#include "csequence.h"
#include "cspriteframe.h"
#include "palette.h"
#include "resource.h"

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

// Hero/boat sprite sequence ids, transcribed complete from the Dreamcast
// CodeView enum hero_seqid. GetSequenceID/GetSequenceName name them;
// hero::GetStandSequence (0x4d9110) proves the five stand values.
enum hero_seqid {
    hs_stand_n = 0,
    hs_stand_ne = 1,
    hs_stand_e = 2,
    hs_stand_se = 3,
    hs_stand_s = 4,
    hs_walk_n = 5,
    hs_walk_ne = 6,
    hs_walk_e = 7,
    hs_walk_se = 8,
    hs_walk_s = 9,
    hs_turn_n_ne = 10,
    hs_turn_ne_n = 11,
    hs_turn_ne_e = 12,
    hs_turn_e_ne = 13,
    hs_turn_e_se = 14,
    hs_turn_se_e = 15,
    hs_turn_se_s = 16,
    hs_turn_s_se = 17,
    hs_max = 18
};

// CSprite.h of the Loki port (RoE source; Loki object 44, CSprite.cpp). The
// sprite owns its sequences, the 16- and 24-bit palettes and the valid-
// sequence mask; resource's vtable holds the destructor alone. The inline
// members are emitted, in this declaration order, after CSprite.cpp's own
// functions (Loki 0x819a724 GetWidth .. 0x819b9bc DrawTileScaled25).
class CSprite : public resource {
public:
    CSprite();
    CSprite(const char* name, int sprtype, int w, int h);
    virtual ~CSprite();

    void clear();
    void AllocateSeq(int seqnum, int numFrames);
    void AddFrame(int seqnum, const char* name);
    int AddFrame(int seqnum, const char* name, int w, int h, unsigned char* data,
                 int csize, TEncodingMethod encoding,
                 int croppedWidth, int croppedHeight, int croppedX, int croppedY);
    int AddFrame(int seqnum, const char* name, int w, int h, unsigned char* data,
                 int csize, TEncodingMethod encoding);
    int AddFrame(int seqnum, CSpriteFrame* frame);
    void SetPalette(const unsigned short* pal);
    void ResetPalette();
    unsigned short* GetPalette();
    const unsigned short* GetPalette() const;
    void ColorCycle(int begin, int end, int step);

    void Draw(int seqnum, int framenum, int sx, int sy, int sw, int sh,
              unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
              bool hflip, bool tblit) const;
    void DrawCreature(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                      unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
                      bool hflip, unsigned short outcolor) const;
    void DrawCreatureAlpha(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                           unsigned short* dst, int dx, int dy, int dw, int dh,
                           int dpitch, bool hflip, unsigned short outcolor) const;
    void DrawAdvObj(int framenum, int sx, int sy, int sw, int sh, unsigned short* dst,
                    int dx, int dy, int dw, int dh, int dpitch, bool hflip) const;
    void DrawAdvObjWithFlag(int framenum, int sx, int sy, int sw, int sh,
                            unsigned short* dst, int dx, int dy, int dw, int dh,
                            int dpitch, unsigned short outcolor, bool hflip) const;
    void DrawAdvObjWithFlagAlpha(int framenum, int sx, int sy, int sw, int sh,
                                 unsigned short* dst, int dx, int dy, int dw, int dh,
                                 int dpitch, unsigned short outcolor, bool hflip) const;
    void DrawAdvObjShadow(int framenum, int sx, int sy, int sw, int sh,
                          unsigned short* dst, int dx, int dy, int dw, int dh,
                          int dpitch, bool hflip) const;
    void DrawPointer(int framenum, unsigned short* dst, int dx, int dy, int dw, int dh,
                     int dpitch, bool hflip) const;
    void DrawInterface(int framenum, int sx, int sy, int sw, int sh, unsigned short* dst,
                       int dx, int dy, int dw, int dh, int dpitch, bool hflip) const;
    void DrawTile(int framenum, unsigned short* dst, int dx, int dy, int dpitch,
                  bool hflip, bool vflip) const;
    void DrawTile(int framenum, int sx, int sy, int sw, int sh, unsigned short* dst,
                  int dx, int dy, int dw, int dh, int dpitch, bool hflip,
                  bool vflip) const;
    void DrawTileShadow(int framenum, int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
                        bool hflip, bool vflip) const;
    void DrawShroudTile(int framenum, int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
                        bool hflip, bool vflip) const;
    void DrawHero(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                  unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
                  bool hflip) const;
    void DrawHeroShadow(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
                        bool hflip) const;
    void DrawHeroAlpha(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                       unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
                       bool hflip) const;
    void DrawCombatHero(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
                        bool hflip) const;
    void DrawSpellEffect(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                         unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
                         bool hflip, bool alpha) const;
    void DrawAdvObjWithFlagScaled50(int framenum, int sx, int sy, int sw, int sh,
                                    unsigned short* dst, int dx, int dy, int dw, int dh,
                                    int dpitch, unsigned short outcolor) const;
    void DrawAdvObjShadowScaled50(int framenum, int sx, int sy, int sw, int sh,
                                  unsigned short* dst, int dx, int dy, int dw, int dh,
                                  int dpitch) const;
    void DrawTileScaled50(int framenum, int sx, int sy, int sw, int sh,
                          unsigned short* dst, int dx, int dy, int dw, int dh,
                          int dpitch, bool hflip, bool vflip) const;
    void DrawAdvObjWithFlagScaled25(int framenum, int sx, int sy, int sw, int sh,
                                    unsigned short* dst, int dx, int dy, int dw, int dh,
                                    int dpitch, unsigned short outcolor) const;
    void DrawAdvObjShadowScaled25(int framenum, int sx, int sy, int sw, int sh,
                                  unsigned short* dst, int dx, int dy, int dw, int dh,
                                  int dpitch) const;
    void DrawTileScaled25(int framenum, int sx, int sy, int sw, int sh,
                          unsigned short* dst, int dx, int dy, int dw, int dh,
                          int dpitch, bool hflip, bool vflip) const;

    static int GetSpriteType(const char* name);
    static const char* GetSpriteTypeName(int type);
    static int GetNumSeqs(int type);
    static int GetSequenceID(int type, const char* name);
    static const char* GetSequenceName(int type, int num);

    int GetWidth() const { return Width; }
    int GetHeight() const { return Height; }
    int GetCroppedX(int seq, int frame) const { return s[seq]->f[frame]->GetCroppedX(); }
    int GetCroppedY(int seq, int frame) const { return s[seq]->f[frame]->GetCroppedY(); }
    int GetCroppedWidth(int seq, int frame) const
    {
        return s[seq]->f[frame]->GetCroppedWidth();
    }
    int GetCroppedHeight(int seq, int frame) const
    {
        return s[seq]->f[frame]->GetCroppedHeight();
    }
    CSequence* GetSequence(int seq) { return s[seq]; }
    CSpriteFrame* GetFrame(int seq, int frame) { return s[seq]->f[frame]; }
    static void SetPixelFormat(unsigned int rmask, unsigned int gmask, unsigned int bmask)
    {
        CSpriteFrame::SetPixelFormat(rmask, gmask, bmask);
    }
    void SetPalette(const TPalette16& pal)
    {
        if (p)
            delete p;
        p = new TPalette16(pal);
    }
    void SetPalette(const TPalette24& pal)
    {
        if (p24)
            delete p24;
        p24 = new TPalette24(pal);
    }
    TPalette24* GetPalette24() { return p24; }
    const TPalette24* GetPalette24() const { return p24; }
    unsigned short GetPaletteColor(unsigned char index) const { return p->m_data[index]; }
    int GetNumSeqs() const { return numSequences; }
    int GetNumFrames(int seq = 0) const { return IsValidSeq(seq) ? s[seq]->numFrames : 0; }
    int IsValidSeq(int seq) const { return seq < numSequences && validSeqMask[seq]; }

    void Draw(int seqnum, int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
              int dx, int dy, bool hflip, bool tblit) const
    {
        Draw(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip, tblit);
    }

    void DrawCreature(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                      Bitmap16Bit* dst, int dx, int dy, bool hflip,
                      unsigned short outcolor) const
    {
        DrawCreature(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip, outcolor);
    }

    void DrawCreatureAlpha(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                           Bitmap16Bit* dst, int dx, int dy, bool hflip,
                           unsigned short outcolor) const
    {
        DrawCreatureAlpha(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip, outcolor);
    }

    void DrawAdvObj(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                    int dx, int dy, bool hflip) const
    {
        DrawAdvObj(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy, dst->GetWidth(),
            dst->GetHeight(), dst->GetPitch(), hflip);
    }

    void DrawAdvObjWithFlag(int framenum, int sx, int sy, int sw, int sh,
                            Bitmap16Bit* dst, int dx, int dy, unsigned short outcolor,
                            bool hflip) const
    {
        DrawAdvObjWithFlag(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), outcolor, hflip);
    }

    void DrawAdvObjWithFlagAlpha(int framenum, int sx, int sy, int sw, int sh,
                                 Bitmap16Bit* dst, int dx, int dy,
                                 unsigned short outcolor, bool hflip) const
    {
        DrawAdvObjWithFlagAlpha(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), outcolor, hflip);
    }

    void DrawAdvObjShadow(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                          int dx, int dy, bool hflip) const
    {
        DrawAdvObjShadow(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }

    void DrawPointer(int framenum, Bitmap16Bit* dst, int dx, int dy, bool hflip) const
    {
        DrawPointer(framenum, dst->GetMap(0, 0), dx, dy, dst->GetWidth(),
            dst->GetHeight(), dst->GetPitch(), hflip);
    }

    void DrawInterface(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                       int dx, int dy, bool hflip) const
    {
        DrawInterface(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }

    void DrawTile(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst, int dx,
                  int dy, bool hflip, bool vflip) const
    {
        DrawTile(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy, dst->GetWidth(),
            dst->GetHeight(), dst->GetPitch(), hflip, vflip);
    }

    void DrawTile(int framenum, Bitmap16Bit* dst, int dx, int dy, bool hflip,
                  bool vflip) const
    {
        DrawTile(framenum, dst->GetMap(0, 0), dx, dy, dst->GetPitch(), hflip, vflip);
    }

    void DrawTileShadow(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                        int dx, int dy, bool hflip, bool vflip) const
    {
        DrawTileShadow(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip, vflip);
    }

    void DrawShroudTile(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                        int dx, int dy, bool hflip, bool vflip) const
    {
        DrawShroudTile(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip, vflip);
    }

    void DrawHero(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                  Bitmap16Bit* dst, int dx, int dy, bool hflip) const
    {
        DrawHero(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }

    void DrawHeroShadow(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                        Bitmap16Bit* dst, int dx, int dy, bool hflip) const
    {
        DrawHeroShadow(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }

    void DrawHeroAlpha(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                       Bitmap16Bit* dst, int dx, int dy, bool hflip) const
    {
        DrawHeroAlpha(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip);
    }

    void DrawCombatHero(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                        Bitmap16Bit* dst, int dx, int dy, bool hflip) const
    {
        DrawCreature(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip, 0);
    }

    void DrawSpellEffect(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                         Bitmap16Bit* dst, int dx, int dy, bool hflip, bool alpha) const
    {
        DrawSpellEffect(seqnum, framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), hflip, alpha);
    }

    void DrawAdvObjWithFlagScaled50(int framenum, int sx, int sy, int sw, int sh,
                                    Bitmap16Bit* dst, int dx, int dy,
                                    unsigned short outcolor) const
    {
        DrawAdvObjWithFlagScaled50(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), outcolor);
    }

    void DrawAdvObjShadowScaled50(int framenum, int sx, int sy, int sw, int sh,
                                  Bitmap16Bit* dst, int dx, int dy) const
    {
        DrawAdvObjShadowScaled50(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch());
    }

    void DrawTileScaled50(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                          int dx, int dy, bool hflip, bool vflip) const
    {
        DrawTileScaled50(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), false, false);
    }

    void DrawAdvObjWithFlagScaled25(int framenum, int sx, int sy, int sw, int sh,
                                    Bitmap16Bit* dst, int dx, int dy,
                                    unsigned short outcolor) const
    {
        DrawAdvObjWithFlagScaled25(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), outcolor);
    }

    void DrawAdvObjShadowScaled25(int framenum, int sx, int sy, int sw, int sh,
                                  Bitmap16Bit* dst, int dx, int dy) const
    {
        DrawAdvObjShadowScaled25(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch());
    }

    void DrawTileScaled25(int framenum, int sx, int sy, int sw, int sh, Bitmap16Bit* dst,
                          int dx, int dy, bool hflip, bool vflip) const
    {
        DrawTileScaled25(framenum, sx, sy, sw, sh, dst->GetMap(0, 0), dx, dy,
            dst->GetWidth(), dst->GetHeight(), dst->GetPitch(), false, false);
    }


private:
    CSequence** s;
    TPalette16* p;
    TPalette24* p24;
    int numSequences;
    int* validSeqMask;
    int Width;
    int Height;
};

#endif  /* HOMM3_CSPRITE_H */
