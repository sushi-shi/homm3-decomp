#ifndef HOMM3_CSPRITE_H
#define HOMM3_CSPRITE_H

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
    int getWidth() const { return m_width; }
    // The adjacent size accessor is expanded throughout drawing's hex-
    // targeted spell animation; Dreamcast retains out-of-line copies of
    // both accessors while retail VC6 folds them to the two dword loads.
    int getHeight() const { return m_height; }
    void clear();
    void allocateSeq(int seqnum, int numFrames);
    int addFrame(int seqnum, CSpriteFrame* frame);
    void addFrame(int seqnum, const char* name);
    int addFrame(int seqnum, const char* name, int w, int h,
                 unsigned char* data, int csize, TEncodingMethod encoding,
                 int croppedWidth, int croppedHeight, int croppedX, int croppedY);
    int addFrame(int seqnum, const char* name, int w, int h,
                 unsigned char* data, int csize, TEncodingMethod encoding);

private:
    CSequence** m_s;

public:
    TPalette16* m_p;
    // DC CodeView type 0x17d1 is TPalette24*. Retail ResetPalette confirms
    // it by passing p24+0x1c (the resource head) to the raw palette ctor.
    TPalette24* m_p24;

private:
    int m_numSequences;
    int* m_validSeqMask;
    int m_width;
    int m_height;

public:
    virtual void dispose();
    virtual unsigned int getSize() const;  // slot 2, retail 0x47bd50
    // CSprite.h:148-151.  The Dreamcast image carries out-of-line copies;
    // the retail remote caller expands these in place.
    int getCroppedX(int seq, int frame) const
    {
        return m_s[seq]->m_f[frame]->getCroppedX();
    }
    int getCroppedY(int seq, int frame) const
    {
        return m_s[seq]->m_f[frame]->getCroppedY();
    }
    int getCroppedWidth(int seq, int frame) const
    {
        return m_s[seq]->m_f[frame]->getCroppedWidth();
    }
    int getCroppedHeight(int seq, int frame) const
    {
        return m_s[seq]->m_f[frame]->getCroppedHeight();
    }
    // DC CSprite.h:154 (0x122ba8) proves this non-const header accessor.
    // Complete's dispose frame loop expands the same sequence/frame loads.
    CSpriteFrame* getFrame(int sequence, int frame)
    {
        return m_s[sequence]->m_f[frame];
    }
    // Original: CSprite::SetPixelFormat; CSprite.h:157, dc 0x122bb8
    static void setPixelFormat(unsigned int rmask, unsigned int gmask, unsigned int bmask)
    {
        CSpriteFrame::setPixelFormat(rmask, gmask, bmask);
    }
    // CodeView LF_MFUNCTION marks every Draw-family receiver const.
    // Drawing writes through the destination/frame pointers, not this object.
    void draw(int seqnum, int framenum, int sx, int sy, int sw, int sh,
              unsigned short* dst, int dx, int dy, int dw, int dh,
              int dpitch, bool hflip, bool tblit) const;
    void drawCreature(int seqnum, int framenum, int sx, int sy, int sw,
                      int sh, unsigned short* dst, int dx, int dy, int dw,
                      int dh, int dpitch, bool hflip,
                      unsigned short outcolor) const;
    void drawCreatureAlpha(int seqnum, int framenum, int sx, int sy,
                           int sw, int sh, unsigned short* dst, int dx, int dy,
                           int dw, int dh, int dpitch, unsigned char hflip,
                           unsigned short outcolor) const;
    void drawAdvObj(int framenum, int sx, int sy, int sw, int sh,
                    unsigned short* dst, int dx, int dy, int dw, int dh,
                    int dpitch, bool hflip) const;
    void drawAdvObjWithFlag(int framenum, int sx, int sy, int sw, int sh,
                            unsigned short* dst, int dx, int dy, int dw,
                            int dh, int dpitch, unsigned short outcolor,
                            unsigned char hflip) const;
    void drawAdvObjWithFlagAlpha(int framenum, int sx, int sy, int sw, int sh,
                                 unsigned short* dst, int dx, int dy, int dw,
                                 int dh, int dpitch, unsigned short outcolor,
                                 unsigned char hflip) const;
    void drawAdvObjShadow(int framenum, int sx, int sy, int sw, int sh,
                          unsigned short* dst, int dx, int dy, int dw,
                          int dh, int dpitch, unsigned char hflip) const;
    void drawPointer(int framenum, unsigned short* dst, int dx, int dy,
                     int dw, int dh, int dpitch, bool hflip) const;
    void drawInterface(int framenum, int sx, int sy, int sw, int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch, bool hflip) const;
    void drawTile(int framenum, int sx, int sy, int sw, int sh,
                  unsigned short* dst, int dx, int dy, int dw, int dh,
                  int dpitch, bool hflip, bool vflip) const;
    void drawTileShadow(int framenum, int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh,
                        int dpitch, unsigned char hflip,
                        unsigned char vflip) const;
    void drawShroudTile(int framenum, int sx, int sy, int sw, int sh,
                        unsigned short* dst, int dx, int dy, int dw, int dh,
                        int dpitch, unsigned char hflip,
                        unsigned char vflip) const;
    void drawHero(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                  unsigned short* dst, int dx, int dy, int dw, int dh,
                  int dpitch, bool hflip) const;
    void drawHeroShadow(int seqnum, int framenum, int sx, int sy, int sw,
                        int sh, unsigned short* dst, int dx, int dy, int dw,
                        int dh, int dpitch, bool hflip) const;
    void drawHeroAlpha(int seqnum, int framenum, int sx, int sy, int sw,
                       int sh, unsigned short* dst, int dx, int dy, int dw,
                       int dh, int dpitch, unsigned char hflip) const;
    void drawCombatHero(int seqnum, int framenum, int sx, int sy, int sw,
                        int sh, unsigned short* dst, int dx, int dy, int dw,
                        int dh, int dpitch, unsigned char hflip) const;
    void drawSpellEffect(int seqnum, int framenum, int sx, int sy, int sw,
                         int sh, unsigned short* dst, int dx, int dy, int dw,
                         int dh, int dpitch, bool hflip,
                         bool alpha) const;
    void setPalette(const unsigned short* pal);
    // Complete expands this wrapper in ResetPalette.
    // E:\gamedcs\CSprite.h:259, dc 0x744e4
    void setPalette(TPalette16& pal)
    {
        if (m_p)
            delete m_p;
        m_p = new TPalette16(&pal);
    }
    void resetPalette();
    unsigned short* getPalette();
    const unsigned short* getPalette() const;
    void colorCycle(int begin, int end, int step);
    void drawAdvObjWithFlagScaled50(int framenum, int sx, int sy, int sw,
        int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        unsigned short outcolor) const;
    void drawAdvObjWithFlagScaled25(int framenum, int sx, int sy, int sw,
        int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        unsigned short outcolor) const;
    void drawAdvObjShadowScaled50(int framenum, int sx, int sy, int sw,
        int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch) const;
    void drawAdvObjShadowScaled25(int framenum, int sx, int sy, int sw,
        int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch) const;
    void drawTileScaled50(int framenum, int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        unsigned char hflip, unsigned char vflip) const;
    void drawTileScaled25(int framenum, int sx, int sy, int sw, int sh,
        unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
        unsigned char hflip, unsigned char vflip) const;
    static int getSpriteType(const char* name);
    static const char* getSpriteTypeName(const int type);
    static int getNumSeqs(int type);
    static int getSequenceId(int type, const char* name);
    static const char* getSequenceName(int type, int num);
    // Original GetPalette24, CSprite.h:284, dc 0x57dbc.
    TPalette24& getPalette24() { return *m_p24; }
    // Original: CSprite::GetPaletteColor; CSprite.h:287, dc 0x1f1a0
    // Complete keeps the sprite resident. UpdateRadar's two expansions
    // read the palette directly; DC's removed reload/cache guard is absent.
    unsigned short getPaletteColor(unsigned char index) const
    {
        return m_p->m_data[index];
    }
    // Header inline, DC CSprite.h:293 (dc 0x1f1dc, emitted into
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
    int getNumFrames(int seq) const
    {
        return isValidSeq(seq) ? m_s[seq]->m_numFrames : 0;
    }
    // E:\gamedcs\CSprite.h:294
    // The attack-frame chooser uses this header boundary rather than reading
    // numSequences/validSeqMask directly. Retail VC6 folds it back to the
    // same two loads and tests at each constant-sequence call site.
    int isValidSeq(int seqnum) const
    {
        return seqnum < m_numSequences && m_validSeqMask[seqnum] != 0;
    }
    VA(0x004f0050, 0x47)  // COMDAT owner (kb.obj emits ?Draw@CSprite@@QBEXHHHHHHPAVBitmap16Bit@@HHEE@Z), body in csprite.h
    void draw(int seqnum, int framenum, int sx, int sy, int sw, int sh,
              Bitmap16Bit* dst, int dx, int dy, bool hflip,
              bool tblit) const
    {
        draw(seqnum, framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
             dst->getWidth(), dst->getHeight(), dst->getPitch(), hflip,
             tblit);
    }
    // CSprite.h:342 header wrapper (the DC compiler emits its own copy at
    // 0x87394); retail expands the Bitmap16Bit forwarding in remote.
    void drawCreature(int seqnum, int framenum, int sx, int sy, int sw,
                      int sh, Bitmap16Bit* dst, int dx, int dy,
                      bool hflip, unsigned short outcolor) const
    {
        drawCreature(seqnum, framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                     dst->getWidth(), dst->getHeight(), dst->getPitch(), hflip, outcolor);
    }
    // Original: CSprite::DrawCreatureAlpha; CSprite.h:348, dc 0x87438.
    void drawCreatureAlpha(int seqnum, int framenum, int sx, int sy, int sw,
        int sh, Bitmap16Bit* dst, int dx, int dy, bool hflip,
        unsigned short outcolor) const
    {
        drawCreatureAlpha(seqnum, framenum, sx, sy, sw, sh,
            dst->getMap(0, 0), dx, dy, dst->getWidth(), dst->getHeight(),
            dst->getPitch(), hflip, outcolor);
    }

    // DC CSprite.h:355 calls all four Bitmap16Bit accessors before the
    // raw-map overload. Preserve those nested boundaries in retail callers.
    void drawAdvObj(int framenum, int sx, int sy, int sw, int sh,
                    Bitmap16Bit* dst, int dx, int dy, bool hflip) const
    {
        drawAdvObj(framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                   dst->getWidth(), dst->getHeight(), dst->getPitch(), hflip);
    }
    void drawAdvObjWithFlag(int framenum, int sx, int sy, int sw, int sh,
                            Bitmap16Bit* dst, int dx, int dy,
                            unsigned short outcolor, unsigned char hflip) const
    {
        drawAdvObjWithFlag(framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                           dst->getWidth(), dst->getHeight(), dst->getPitch(), outcolor,
                           hflip);
    }
    void drawAdvObjShadow(int framenum, int sx, int sy, int sw, int sh,
                          Bitmap16Bit* dst, int dx, int dy,
                          unsigned char hflip) const
    {
        drawAdvObjShadow(framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                         dst->getWidth(), dst->getHeight(), dst->getPitch(), hflip);
    }
    // CSprite.h:378..381, dc 0xd9f98: bitmap DrawPointer facade.
    void drawPointer(int framenum, Bitmap16Bit* dst, int dx, int dy,
                     bool hflip) const
    {
        drawPointer(framenum, dst->getMap(0, 0), dx, dy,
                    dst->getWidth(), dst->getHeight(), dst->getPitch(), hflip);
    }
    // Header wrapper (DC CSprite.h:385). KeyAccel's four expanded call sites
    // byte-prove the Bitmap16Bit member forwarding in retail.
    void drawInterface(int framenum, int sx, int sy, int sw, int sh,
                       Bitmap16Bit* dst, int dx, int dy,
                       bool hflip) const
    {
        drawInterface(framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                      dst->getWidth(), dst->getHeight(), dst->getPitch(), hflip);
    }
    // DC CSprite.h:393 forwards through the same four bitmap accessors.
    void drawTile(int framenum, int sx, int sy, int sw, int sh,
                  Bitmap16Bit* dst, int dx, int dy, bool hflip,
                  bool vflip) const
    {
        drawTile(framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                 dst->getWidth(), dst->getHeight(), dst->getPitch(),
                 hflip, vflip);
    }
    void drawTileShadow(int framenum, int sx, int sy, int sw, int sh,
                        Bitmap16Bit* dst, int dx, int dy,
                        unsigned char hflip, unsigned char vflip) const
    {
        drawTileShadow(framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                       dst->getWidth(), dst->getHeight(), dst->getPitch(), hflip, vflip);
    }
    void drawShroudTile(int framenum, int sx, int sy, int sw, int sh,
                        Bitmap16Bit* dst, int dx, int dy,
                        unsigned char hflip, unsigned char vflip) const
    {
        drawShroudTile(framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                       dst->getWidth(), dst->getHeight(), dst->getPitch(), hflip, vflip);
    }
    // Header wrapper (DC CSprite.h:426): retail advmgr inlines this view,
    // then calls the raw-map overload above.
    void drawHero(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                  Bitmap16Bit* dst, int dx, int dy, bool hflip) const
    {
        drawHero(seqnum, framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                 dst->getWidth(), dst->getHeight(), dst->getPitch(), hflip);
    }
    // DC publics at 0x1f8a4 and 0x72d98 encode _N for hflip in both
    // DrawHeroShadow overloads; T_UCHAR debug lowering is not source uchar.
    void drawHeroShadow(int seqnum, int framenum, int sx, int sy, int sw,
                        int sh, Bitmap16Bit* dst, int dx, int dy,
                        bool hflip) const
    {
        drawHeroShadow(seqnum, framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                       dst->getWidth(), dst->getHeight(), dst->getPitch(), hflip);
    }
    // E:\\gamedcs\\CSprite.h:438. DrawCursorAlpha reaches the bitmap
    // overload four times; Dreamcast's line table shows this header boundary
    // and Complete expands it into the raw map/width/height/pitch call.
    // DC 0x7a1e8 in DrawCursorAlpha expands the bitmap overload declared
    // by function type 0x17e5, loading its map/width/height/pitch and calling
    // the raw DrawHeroAlpha member. The same row recurs at three more sites.
    // @dc-inline-origin: 0x17e5 0x7a1e8
    void drawHeroAlpha(int seqnum, int framenum, int sx, int sy, int sw,
                       int sh, Bitmap16Bit* dst, int dx, int dy,
                       unsigned char hflip) const
    {
        drawHeroAlpha(seqnum, framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                      dst->getWidth(), dst->getHeight(), dst->getPitch(), hflip);
    }
    // DC CSprite.h:444/445, dc 0x874dc: const bitmap facade.
    // Complete's combatManager::drawCombatHero (0x4952b0) calls the general
    // CSprite::drawCreature (0x47bd60) with color zero. This version bypasses
    // the older raw-map drawCombatHero overload.
    void drawCombatHero(int seqnum, int framenum, int sx, int sy, int sw,
                        int sh, Bitmap16Bit* dst, int dx, int dy,
                        bool hflip) const
    {
        drawCreature(seqnum, framenum, sx, sy, sw, sh, dst->getMap(0, 0), dx, dy,
                     dst->getWidth(), dst->getHeight(), dst->getPitch(),
                     hflip, 0);
    }
    // CSprite.h:450/451 (dc drawing.obj:0x8757c) preserves the same
    // bitmap forwarding boundary. The public suffix HH_N1@Z proves both
    // Boolean parameters; its four bitmap accessors remain source calls.
    void drawSpellEffect(int seqnum, int framenum, int sx, int sy, int sw,
                         int sh, Bitmap16Bit* dst, int dx, int dy,
                         bool hflip, bool alpha) const
    {
        drawSpellEffect(seqnum, framenum, sx, sy, sw, sh, dst->getMap(0, 0),
                        dx, dy, dst->getWidth(), dst->getHeight(),
                        dst->getPitch(), hflip, alpha);
    }
};

#endif  /* HOMM3_CSPRITE_H */
