#include "va.h"

#include <string.h>

#include "csprite.h"

#include "cspriteframe.h"
#include "hero.h"
#include "palette.h"

// DC's const draw/accessor bodies first test *Sp_loaded (object +0x20)
// and call SpriteDataReload at 0x73bf4 when zero. That routine reads DEF
// data, rebuilds sequences/frames and palettes, then sets the loaded flag.
// Complete instead loads them in ResourceManager::getSprite; its +0x20
// field is the owned palette. Mac 0x8a72c/0x8a76c/0x8a818 and retail
// 0x47bcc0/0x47bcf0/0x47bd60 have no reload guard. The DC-only alpha and
// scaled wrappers below preserve their frame calls with this Complete
// ownership model, without inventing a reload operation on the palette.

// CSprite vtable 0x63d6b0 slot 0. Defining the virtual destructor below
// naturally emits this wrapper before the importing constructor.
VA_COMPGEN(0x0047b8f0, 0x21, SCALAR_DELETING_DTOR, CSprite)

// Original: CSprite::CSprite; csprite.cpp:82
// Complete omits the DC heap-allocated Sp_loaded cache flag. The seven
// remaining owned fields have the same null/zero initial state.
DC_ADDRESS(0x07215c, 0x80)
CSprite::CSprite()
    : resource(0, RESOURCE_TYPE_NONE), s(0), p(0), p24(0),
      numSequences(0), validSeqMask(0), Width(0), Height(0)
{
}

VA(0x0047b920, 0x118)
DC_ADDRESS(0x0721dc, 0xa6)
MAC_ADDRESS(0x08a37c, 0xd4)
CSprite::CSprite(const char* name, int sprtype, int w, int h)
    : resource(name, (EResourceType)sprtype),
      s(0), p(0), p24(0), numSequences(0), Width(w), Height(h)
{
    numSequences = GetNumSeqs(sprtype);
    if (numSequences) {
        s = new CSequence*[numSequences];
        validSeqMask = new int[numSequences];
        for (int i = 0; i < numSequences; ++i) {
            s[i] = 0;
            validSeqMask[i] = 0;
        }
    }
}

VA(0x0047ba40, 0xae)
DC_ADDRESS(0x072284, 0xc8)
MAC_ADDRESS(0x08a450, 0x108)  // vtable identity + complete owned-field teardown
CSprite::~CSprite()
{
    for (int i = 0; i < numSequences; ++i) {
        if (s[i])
            delete s[i];
    }
    if (s)
        delete[] s;
    if (p)
        delete p;
    if (p24)
        delete p24;
    if (validSeqMask)
        delete[] validSeqMask;
}

// Original: CSprite::clear; csprite.cpp:147
// The reusable clear operation nulls each released pointer. Both DC and
// Complete destructors instead perform their own final teardown.
DC_ADDRESS(0x07234c, 0x8c)
void CSprite::clear()
{
    for (int i = 0; i < numSequences; ++i)
        delete s[i];
    if (s) {
        delete[] s;
        s = 0;
    }
    if (p) {
        delete p;
        p = 0;
    }
    if (p24) {
        delete p24;
        p24 = 0;
    }
    if (validSeqMask) {
        delete[] validSeqMask;
        validSeqMask = 0;
    }
}

VA(0x0047baf0, 0x67)
DC_ADDRESS(0x0723d8, 0x40)
MAC_ADDRESS(0x08a558, 0x7c)
void CSprite::AllocateSeq(int seqnum, int numFrames)
{
    s[seqnum] = new CSequence(numFrames);
    validSeqMask[seqnum] = 1;
}

VA(0x0047bb60, 0x19)
DC_ADDRESS(0x0724c8, 0x18)
MAC_ADDRESS(0x08a5d4, 0x30)
int CSprite::AddFrame(int seqnum, CSpriteFrame* frame)
{
    return s[seqnum]->addFrame(frame);
}

// Original: CSprite::AddFrame; csprite.cpp:187
DC_ADDRESS(0x072418, 0x18)
void CSprite::AddFrame(int seqnum, const char* name)
{
    s[seqnum]->addFrame(name);
}

// Original: CSprite::AddFrame; csprite.cpp:194
DC_ADDRESS(0x072430, 0x5c)
int CSprite::AddFrame(int seqnum, const char* name, int w, int h,
                      unsigned char* data, int csize, TEncodingMethod encoding,
                      int croppedWidth, int croppedHeight, int croppedX, int croppedY)
{
    return s[seqnum]->addFrame(name, w, h, data, csize, encoding,
                                croppedWidth, croppedHeight, croppedX, croppedY);
}

// Original: CSprite::AddFrame; csprite.cpp:200
DC_ADDRESS(0x07248c, 0x3a)
int CSprite::AddFrame(int seqnum, const char* name, int w, int h,
                      unsigned char* data, int csize, TEncodingMethod encoding)
{
    return s[seqnum]->addFrame(name, w, h, data, csize, encoding);
}

VA(0x0047bb80, 0x79)
DC_ADDRESS(0x0724e0, 0x56)
MAC_ADDRESS(0x08a604, 0x80)
void CSprite::SetPalette(const unsigned short* pal)
{
    if (p)
        delete p;
    p = new TPalette16(pal);
}

VA(0x0047bc00, 0xb8)
DC_ADDRESS(0x072538, 0x52)
MAC_ADDRESS(0x08a684, 0xa8)
void CSprite::ResetPalette()
{
    TPalette24 palette24(p24->Palette);
#ifdef __clang__
    TPalette16 palette16(palette24);
    SetPalette(palette16);
#else
    SetPalette(TPalette16(palette24));
#endif
}

// E:\gamedcs\csprite.cpp:226
// DC 228 returns the unsigned-short table, not the bootstrap palette view.
// Complete's 14-byte body retains the null/array-address conditional; the
// older SpriteDataReload guard depends on cache fields absent from this
// retail class (as in its GetNumFrames and IsValidSeq accessors).
VA(0x0047bcc0, 0x0e)
DC_ADDRESS(0x07258c, 0x2c)
MAC_ADDRESS(0x08a72c, 0x1c)  // vtable-era TU order + p/data layout
unsigned short* CSprite::GetPalette()
{
    return p ? p->Palette : 0;
}

// Original: CSprite::GetPalette; csprite.cpp:232
// As in the retained non-const overload, Complete owns its palette directly;
// the DC Sp_loaded/SpriteDataReload cache guard has no Complete fields.
DC_ADDRESS(0x0725b8, 0x70)
const unsigned short* CSprite::GetPalette() const
{
    return p ? p->Palette : 0;
}

VA(0x0047bcd0, 0x1b)
DC_ADDRESS(0x072628, 0x3c)
MAC_ADDRESS(0x08a748, 0x24)
void CSprite::ColorCycle(int begin, int end, int step)
{
    p->Cycle(begin, end, step);
}

VA(0x0047bcf0, 0x52)
DC_ADDRESS(0x072664, 0x90)
MAC_ADDRESS(0x08a76c, 0xa4)
void CSprite::Draw(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                   unsigned short* dst, int dx, int dy, int dw, int dh,
                   int dpitch, bool hflip, bool tblit) const
{
    s[seqnum]->m_f[framenum]->Draw(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip, tblit);
}

VA(0x0047bd50, 0x06)
MAC_ADDRESS(0x08a810, 0x8)  // CSprite vtable slot 2 + literal sizeof(CSprite)
unsigned int CSprite::getSize() const
{
    return sizeof(*this);
}

VA(0x0047bd60, 0x54)
DC_ADDRESS(0x0726f4, 0x90)
MAC_ADDRESS(0x08a818, 0xb4)
void CSprite::DrawCreature(int seqnum, int framenum, int sx, int sy,
                           int sw, int sh, unsigned short* dst,
                           int dx, int dy, int dw, int dh, int dpitch,
                           bool hflip, unsigned short outcolor) const
{
    s[seqnum]->m_f[framenum]->DrawCreature(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
        *p, hflip, outcolor);
}

// Original: CSprite::DrawCreatureAlpha; csprite.cpp:274
DC_ADDRESS(0x072784, 0x90)
void CSprite::DrawCreatureAlpha(int seqnum, int framenum, int sx, int sy,
                                int sw, int sh, unsigned short* dst,
                                int dx, int dy, int dw, int dh, int dpitch,
                                bool hflip, unsigned short outcolor) const
{
    s[seqnum]->m_f[framenum]->DrawCreatureAlpha(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip, outcolor);
}

VA(0x0047bdc0, 0x4c)
DC_ADDRESS(0x072814, 0x86)
MAC_ADDRESS(0x08a8cc, 0x94)  // sequence zero + adv-object implementation
void CSprite::DrawAdvObj(int framenum, int sx, int sy, int sw, int sh,
                         unsigned short* dst, int dx, int dy, int dw, int dh,
                         int dpitch, bool hflip) const
{
    s[0]->m_f[framenum]->DrawAdvObj(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip);
}

VA(0x0047be10, 0x4e)
DC_ADDRESS(0x07289c, 0xa8)
MAC_ADDRESS(0x08a960, 0x94)  // sequence zero + adv-object flag forwarding
void CSprite::DrawAdvObjWithFlag(int framenum, int sx, int sy, int sw,
                                 int sh, unsigned short* dst, int dx, int dy,
                                 int dw, int dh, int dpitch,
                                 unsigned short outcolor,
                                 bool hflip) const
{
    s[0]->m_f[framenum]->DrawAdvObjWithFlag(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, outcolor, hflip);
}

// Original: CSprite::DrawAdvObjWithFlagAlpha; csprite.cpp:298
DC_ADDRESS(0x072944, 0x8c)
void CSprite::DrawAdvObjWithFlagAlpha(int framenum, int sx, int sy,
                                     int sw, int sh, unsigned short* dst,
                                     int dx, int dy, int dw, int dh, int dpitch,
                                     unsigned short outcolor,
                                     bool hflip) const
{
    s[0]->m_f[framenum]->DrawAdvObjWithFlagAlpha(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, outcolor, hflip);
}

VA(0x0047be60, 0x4a)
DC_ADDRESS(0x0729d0, 0x86)
MAC_ADDRESS(0x08a9f4, 0x84)  // sequence zero + shadow implementation
void CSprite::DrawAdvObjShadow(int framenum, int sx, int sy, int sw, int sh,
                               unsigned short* dst, int dx, int dy, int dw,
                               int dh, int dpitch, bool hflip) const
{
    s[0]->m_f[framenum]->DrawAdvObjShadow(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip);
}

VA(0x0047beb0, 0x4a)
DC_ADDRESS(0x072a58, 0x6a)
MAC_ADDRESS(0x08aa78, 0x8c)  // full-frame pointer draw through sequence zero
void CSprite::DrawPointer(int framenum, unsigned short* dst, int dx, int dy,
                          int dw, int dh, int dpitch, bool hflip) const
{
    s[0]->m_f[framenum]->DrawPointer(
        dst, dx, dy, dw, dh, dpitch, *p, hflip);
}

VA(0x0047bf00, 0x4c)
DC_ADDRESS(0x072ac4, 0x86)
MAC_ADDRESS(0x08ab04, 0x94)  // sequence zero + transparent draw forwarding
void CSprite::DrawInterface(int framenum, int sx, int sy, int sw, int sh,
                            unsigned short* dst, int dx, int dy, int dw,
                            int dh, int dpitch, bool hflip) const
{
    s[0]->m_f[framenum]->DrawInterface(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip);
}

VA(0x0047bf50, 0x4e)
DC_ADDRESS(0x072b4c, 0x8c)
MAC_ADDRESS(0x08ab98, 0x94)  // sequence zero + tile forwarding
void CSprite::DrawTile(int framenum, int sx, int sy, int sw, int sh,
                       unsigned short* dst, int dx, int dy, int dw, int dh,
                       int dpitch, bool hflip, bool vflip) const
{
    s[0]->m_f[framenum]->DrawTile(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip, vflip);
}

VA(0x0047bfa0, 0x4e)
DC_ADDRESS(0x072bd8, 0x8c)
MAC_ADDRESS(0x08ac2c, 0x94)  // sequence zero + tile-shadow forwarding
void CSprite::DrawTileShadow(int framenum, int sx, int sy, int sw, int sh,
                             unsigned short* dst, int dx, int dy, int dw,
                             int dh, int dpitch, bool hflip,
                             bool vflip) const
{
    s[0]->m_f[framenum]->DrawTileShadow(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip, vflip);
}

VA(0x0047bff0, 0x8b)
DC_ADDRESS(0x072c64, 0xac)
MAC_ADDRESS(0x08acc0, 0xe4)  // paired tile + shadow calls on one selected frame
void CSprite::DrawShroudTile(int framenum, int sx, int sy, int sw, int sh,
                             unsigned short* dst, int dx, int dy, int dw,
                             int dh, int dpitch, bool hflip,
                             bool vflip) const
{
    s[0]->m_f[framenum]->DrawShroudTile(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip, vflip);
}

VA(0x0047c080, 0x50)
DC_ADDRESS(0x072d10, 0x88)
MAC_ADDRESS(0x08ada4, 0xac)  // selected sequence + adv-object implementation
void CSprite::DrawHero(int seqnum, int framenum, int sx, int sy, int sw,
                       int sh, unsigned short* dst, int dx, int dy, int dw,
                       int dh, int dpitch, bool hflip) const
{
    s[seqnum]->m_f[framenum]->DrawHero(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip);
}

VA(0x0047c0d0, 0x4e)
DC_ADDRESS(0x072d98, 0x88)
MAC_ADDRESS(0x08ae50, 0x9c)  // selected sequence + shadow implementation
void CSprite::DrawHeroShadow(int seqnum, int framenum, int sx, int sy,
                             int sw, int sh, unsigned short* dst,
                             int dx, int dy, int dw, int dh, int dpitch,
                             bool hflip) const
{
    s[seqnum]->m_f[framenum]->DrawHeroShadow(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip);
}

VA(0x0047c120, 0x50)
DC_ADDRESS(0x072e20, 0x88)
MAC_ADDRESS(0x08aeec, 0xac)  // selected sequence + hero-alpha implementation
void CSprite::DrawHeroAlpha(int seqnum, int framenum, int sx, int sy,
                            int sw, int sh, unsigned short* dst,
                            int dx, int dy, int dw, int dh, int dpitch,
                            bool hflip) const
{
    s[seqnum]->m_f[framenum]->DrawHeroAlpha(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip);
}

// Original: CSprite::DrawCombatHero; csprite.cpp:396
// The retained Complete bitmap facade instead uses DrawCreature(..., 0).
DC_ADDRESS(0x072ea8, 0x8c)
void CSprite::DrawCombatHero(int seqnum, int framenum, int sx, int sy,
                             int sw, int sh, unsigned short* dst,
                             int dx, int dy, int dw, int dh, int dpitch,
                             bool hflip) const
{
    s[seqnum]->m_f[framenum]->DrawCreature(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip, 0);
}

VA(0x0047c170, 0x52)
DC_ADDRESS(0x072f34, 0x8e)
MAC_ADDRESS(0x08af98, 0xa4)  // selected sequence + spell-effect implementation
void CSprite::DrawSpellEffect(int seqnum, int framenum, int sx, int sy,
                              int sw, int sh, unsigned short* dst,
                              int dx, int dy, int dw, int dh, int dpitch,
                              bool hflip, bool alpha) const
{
    s[seqnum]->m_f[framenum]->DrawSpellEffect(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip, alpha);
}

// Original: CSprite::DrawAdvObjWithFlagScaled50; csprite.cpp:411
DC_ADDRESS(0x072fc4, 0x9c)
void CSprite::DrawAdvObjWithFlagScaled50(int framenum, int sx, int sy, int sw,
    int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    unsigned short outcolor) const
{
    s[0]->m_f[framenum]->DrawAdvObjWithFlagScaled50(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, outcolor);
}

// Original: CSprite::DrawAdvObjShadowScaled50; csprite.cpp:418
DC_ADDRESS(0x073060, 0x78)
void CSprite::DrawAdvObjShadowScaled50(int framenum, int sx, int sy, int sw,
    int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch) const
{
    s[0]->m_f[framenum]->DrawAdvObjShadowScaled50(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p);
}

// Original: CSprite::DrawTileScaled50; csprite.cpp:425
DC_ADDRESS(0x0730d8, 0x8a)
void CSprite::DrawTileScaled50(int framenum, int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    bool hflip, bool vflip) const
{
    s[0]->m_f[framenum]->DrawTileScaled50(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip, vflip);
}

// Original: CSprite::DrawAdvObjWithFlagScaled25; csprite.cpp:432
DC_ADDRESS(0x073164, 0x84)
void CSprite::DrawAdvObjWithFlagScaled25(int framenum, int sx, int sy, int sw,
    int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    unsigned short outcolor) const
{
    s[0]->m_f[framenum]->DrawAdvObjWithFlagScaled25(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, outcolor);
}

// Original: CSprite::DrawAdvObjShadowScaled25; csprite.cpp:439
DC_ADDRESS(0x0731e8, 0x78)
void CSprite::DrawAdvObjShadowScaled25(int framenum, int sx, int sy, int sw,
    int sh, unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch) const
{
    s[0]->m_f[framenum]->DrawAdvObjShadowScaled25(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p);
}

// Original: CSprite::DrawTileScaled25; csprite.cpp:446
DC_ADDRESS(0x073260, 0x8a)
void CSprite::DrawTileScaled25(int framenum, int sx, int sy, int sw, int sh,
    unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
    bool hflip, bool vflip) const
{
    s[0]->m_f[framenum]->DrawTileScaled25(
        sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip, vflip);
}

// Original: CSprite::GetSpriteType; csprite.cpp:454
DC_ADDRESS(0x0732ec, 0xc8)
int CSprite::GetSpriteType(const char* name)
{
    if (!stricmp(name, "sprite"))
        return RESOURCE_TYPE_SPRITE;
    if (!stricmp(name, "creature"))
        return RESOURCE_TYPE_CREATURE;
    if (!stricmp(name, "advobj"))
        return RESOURCE_TYPE_ADVENTURE_OBJECT;
    if (!stricmp(name, "hero"))
        return RESOURCE_TYPE_HERO;
    if (!stricmp(name, "tileset"))
        return RESOURCE_TYPE_TILESET;
    if (!stricmp(name, "pointer"))
        return RESOURCE_TYPE_POINTER;
    if (!stricmp(name, "interface"))
        return RESOURCE_TYPE_INTERFACE;
    if (!stricmp(name, "combathero"))
        return RESOURCE_TYPE_COMBAT_HERO;
    return RESOURCE_TYPE_INVALID;
}

// Original: CSprite::GetSpriteTypeName; csprite.cpp:506
DC_ADDRESS(0x0733b4, 0x78)
const char* CSprite::GetSpriteTypeName(const int type)
{
    switch (type) {
    case RESOURCE_TYPE_SPRITE: return "Sprite";
    case RESOURCE_TYPE_SPRITE_DEFINITION: return "SpriteDef";
    case RESOURCE_TYPE_PALETTE: return "Palette";
    case RESOURCE_TYPE_CREATURE: return "Creature";
    case RESOURCE_TYPE_ADVENTURE_OBJECT: return "AdvObj";
    case RESOURCE_TYPE_HERO: return "Hero";
    case RESOURCE_TYPE_TILESET: return "Tileset";
    case RESOURCE_TYPE_POINTER: return "Pointer";
    case RESOURCE_TYPE_INTERFACE: return "Interface";
    case RESOURCE_TYPE_SPRITE_FRAME: return "SpriteFrame";
    case RESOURCE_TYPE_ADVENTURE_MASK: return "AdvObjMask";
    case RESOURCE_TYPE_COMBAT_HERO: return "CombatHero";
    default: return "?";
    }
}

VA(0x0047c1d0, 0x6c)
DC_ADDRESS(0x07342c, 0x3a)
MAC_ADDRESS(0x08b03c, 0x50)
int CSprite::GetNumSeqs(int type)
{
    switch (type) {
    case RESOURCE_TYPE_CREATURE:
        return 22;
    case RESOURCE_TYPE_HERO:
        return 18;
    case RESOURCE_TYPE_COMBAT_HERO:
        return 5;
    case RESOURCE_TYPE_SPRITE:
    case RESOURCE_TYPE_ADVENTURE_OBJECT:
    case RESOURCE_TYPE_TILESET:
    case RESOURCE_TYPE_POINTER:
    case RESOURCE_TYPE_INTERFACE:
        return 1;
    case RESOURCE_TYPE_SPRITE_DEFINITION:
    case RESOURCE_TYPE_SPRITE_FRAME:
    case RESOURCE_TYPE_RESERVED_74:
    case RESOURCE_TYPE_RESERVED_75:
    case RESOURCE_TYPE_RESERVED_76:
    case RESOURCE_TYPE_RESERVED_77:
    case RESOURCE_TYPE_RESERVED_78:
    case RESOURCE_TYPE_ADVENTURE_MASK:
        return 0;
    default:
        return 0;
    }
}

// Original: CSprite::GetSequenceID; csprite.cpp:605
// The debug strings and switch tables retain the animation-file vocabulary.
DC_ADDRESS(0x073468, 0x416)
int CSprite::GetSequenceID(int type, const char* name)
{
    switch (type) {
    case RESOURCE_TYPE_CREATURE:
        if (!stricmp(name, "cs_walk")) return cs_walk;
        if (!stricmp(name, "cs_fidget")) return cs_fidget;
        if (!stricmp(name, "cs_wait")) return cs_wait;
        if (!stricmp(name, "cs_wince")) return cs_wince;
        if (!stricmp(name, "cs_defend")) return cs_defend;
        if (!stricmp(name, "cs_death")) return cs_death;
        if (!stricmp(name, "cs_specdeath")) return cs_specdeath;
        if (!stricmp(name, "cs_turn_rf")) return cs_turn_rf;
        if (!stricmp(name, "cs_turn_fr")) return cs_turn_fr;
        if (!stricmp(name, "cs_turn_lf")) return cs_turn_lf;
        if (!stricmp(name, "cs_turn_fl")) return cs_turn_fl;
        if (!stricmp(name, "cs_attack_ur")) return cs_attack_ur;
        if (!stricmp(name, "cs_attack_r")) return cs_attack_r;
        if (!stricmp(name, "cs_attack_dr")) return cs_attack_dr;
        if (!stricmp(name, "cs_range_ur")) return cs_range_ur;
        if (!stricmp(name, "cs_range_r")) return cs_range_r;
        if (!stricmp(name, "cs_range_dr")) return cs_range_dr;
        if (!stricmp(name, "cs_special_ur")) return cs_special_ur;
        if (!stricmp(name, "cs_special_r")) return cs_special_r;
        if (!stricmp(name, "cs_special_dr")) return cs_special_dr;
        if (!stricmp(name, "cs_prewalk")) return cs_prewalk;
        if (!stricmp(name, "cs_postwalk")) return cs_postwalk;
        break;
    case RESOURCE_TYPE_HERO:
        if (!stricmp(name, "hs_stand_n")) return hs_stand_n;
        if (!stricmp(name, "hs_stand_ne")) return hs_stand_ne;
        if (!stricmp(name, "hs_stand_e")) return hs_stand_e;
        if (!stricmp(name, "hs_stand_se")) return hs_stand_se;
        if (!stricmp(name, "hs_stand_s")) return hs_stand_s;
        if (!stricmp(name, "hs_walk_n")) return hs_walk_n;
        if (!stricmp(name, "hs_walk_ne")) return hs_walk_ne;
        if (!stricmp(name, "hs_walk_e")) return hs_walk_e;
        if (!stricmp(name, "hs_walk_se")) return hs_walk_se;
        if (!stricmp(name, "hs_walk_s")) return hs_walk_s;
        if (!stricmp(name, "hs_turn_n_ne")) return hs_turn_n_ne;
        if (!stricmp(name, "hs_turn_ne_n")) return hs_turn_ne_n;
        if (!stricmp(name, "hs_turn_ne_e")) return hs_turn_ne_e;
        if (!stricmp(name, "hs_turn_e_ne")) return hs_turn_e_ne;
        if (!stricmp(name, "hs_turn_e_se")) return hs_turn_e_se;
        if (!stricmp(name, "hs_turn_se_e")) return hs_turn_se_e;
        if (!stricmp(name, "hs_turn_se_s")) return hs_turn_se_s;
        if (!stricmp(name, "hs_turn_s_se")) return hs_turn_s_se;
        break;
    case RESOURCE_TYPE_COMBAT_HERO:
        if (!stricmp(name, "chs_stand")) return combatHeroStand;
        if (!stricmp(name, "chs_fidget")) return combatHeroFidget;
        if (!stricmp(name, "chs_defeat")) return combatHeroDefeat;
        if (!stricmp(name, "chs_victory")) return combatHeroVictory;
        if (!stricmp(name, "chs_cast")) return combatHeroCast;
        break;
    case RESOURCE_TYPE_SPRITE:
    case RESOURCE_TYPE_ADVENTURE_OBJECT:
    case RESOURCE_TYPE_TILESET:
    case RESOURCE_TYPE_POINTER:
    case RESOURCE_TYPE_INTERFACE:
        if (!stricmp(name, "default")) return 0;
        break;
    case RESOURCE_TYPE_SPRITE_DEFINITION:
    case RESOURCE_TYPE_SPRITE_FRAME:
    case RESOURCE_TYPE_ADVENTURE_MASK:
        return -1;
    }
    return -1;
}

// Original: CSprite::GetSequenceName; csprite.cpp:774
DC_ADDRESS(0x073880, 0x290)
const char* CSprite::GetSequenceName(int type, int num)
{
    switch (type) {
    case RESOURCE_TYPE_CREATURE:
        switch (num) {
        case cs_walk: return "cs_walk";
        case cs_fidget: return "cs_fidget";
        case cs_wait: return "cs_wait";
        case cs_wince: return "cs_wince";
        case cs_defend: return "cs_defend";
        case cs_death: return "cs_death";
        case cs_specdeath: return "cs_specdeath";
        case cs_turn_rf: return "cs_turn_rf";
        case cs_turn_fr: return "cs_turn_fr";
        case cs_turn_lf: return "cs_turn_lf";
        case cs_turn_fl: return "cs_turn_fl";
        case cs_attack_ur: return "cs_attack_ur";
        case cs_attack_r: return "cs_attack_r";
        case cs_attack_dr: return "cs_attack_dr";
        case cs_range_ur: return "cs_range_ur";
        case cs_range_r: return "cs_range_r";
        case cs_range_dr: return "cs_range_dr";
        case cs_special_ur: return "cs_special_ur";
        case cs_special_r: return "cs_special_r";
        case cs_special_dr: return "cs_special_dr";
        case cs_prewalk: return "cs_prewalk";
        case cs_postwalk: return "cs_postwalk";
        }
        break;
    case RESOURCE_TYPE_HERO:
        switch (num) {
        case hs_stand_n: return "hs_stand_n";
        case hs_stand_ne: return "hs_stand_ne";
        case hs_stand_e: return "hs_stand_e";
        case hs_stand_se: return "hs_stand_se";
        case hs_stand_s: return "hs_stand_s";
        case hs_walk_n: return "hs_walk_n";
        case hs_walk_ne: return "hs_walk_ne";
        case hs_walk_e: return "hs_walk_e";
        case hs_walk_se: return "hs_walk_se";
        case hs_walk_s: return "hs_walk_s";
        case hs_turn_n_ne: return "hs_turn_n_ne";
        case hs_turn_ne_n: return "hs_turn_ne_n";
        case hs_turn_ne_e: return "hs_turn_ne_e";
        case hs_turn_e_ne: return "hs_turn_e_ne";
        case hs_turn_e_se: return "hs_turn_e_se";
        case hs_turn_se_e: return "hs_turn_se_e";
        case hs_turn_se_s: return "hs_turn_se_s";
        case hs_turn_s_se: return "hs_turn_s_se";
        }
        break;
    case RESOURCE_TYPE_COMBAT_HERO:
        switch (num) {
        case combatHeroStand: return "chs_stand";
        case combatHeroFidget: return "chs_fidget";
        case combatHeroDefeat: return "chs_defeat";
        case combatHeroVictory: return "chs_victory";
        case combatHeroCast: return "chs_cast";
        }
        break;
    case RESOURCE_TYPE_SPRITE:
    case RESOURCE_TYPE_ADVENTURE_OBJECT:
    case RESOURCE_TYPE_TILESET:
    case RESOURCE_TYPE_POINTER:
    case RESOURCE_TYPE_INTERFACE:
        return "default";
    case RESOURCE_TYPE_SPRITE_DEFINITION:
    case RESOURCE_TYPE_SPRITE_FRAME:
    case RESOURCE_TYPE_ADVENTURE_MASK:
        return "?";
    }
    return "?";
}
