// CSprite.cpp of the Loki port (Loki object 44), in Loki's function order.
// No assert names the file; Loki's census infers it from the class.
#include <string.h>
#include <strings.h>

#include "csprite.h"

CSprite::CSprite()
    : resource(0, RESOURCE_TYPE_NONE), s(0), p(0), p24(0),
      numSequences(0), validSeqMask(0), Width(0), Height(0)
{
}

CSprite::CSprite(const char* name, int sprtype, int w, int h)
    : resource(name, (EResourceType)sprtype),
      s(0), p(0), p24(0), numSequences(0), Width(w), Height(h)
{
    numSequences = GetNumSeqs(sprtype);
    if (numSequences) {
        s = new CSequence*[numSequences];
        validSeqMask = new int[numSequences];
        for (int i = 0; i < numSequences; i++) {
            s[i] = 0;
            validSeqMask[i] = 0;
        }
    }
}

CSprite::~CSprite()
{
    for (int i = 0; i < numSequences; i++)
        delete s[i];
    if (s)
        delete[] s;
    if (p)
        delete p;
    if (p24)
        delete p24;
    if (validSeqMask)
        delete[] validSeqMask;
}

void CSprite::clear()
{
    for (int i = 0; i < numSequences; i++)
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

void CSprite::AllocateSeq(int seqnum, int numFrames)
{
    s[seqnum] = new CSequence(numFrames);
    validSeqMask[seqnum] = 1;
}

void CSprite::AddFrame(int seqnum, const char* name)
{
    s[seqnum]->AddFrame(name);
}

int CSprite::AddFrame(int seqnum, const char* name, int w, int h,
                      unsigned char* data, int csize, TEncodingMethod encoding,
                      int croppedWidth, int croppedHeight, int croppedX, int croppedY)
{
    return s[seqnum]->AddFrame(name, w, h, data, csize, encoding,
                               croppedWidth, croppedHeight, croppedX, croppedY);
}

int CSprite::AddFrame(int seqnum, const char* name, int w, int h,
                      unsigned char* data, int csize, TEncodingMethod encoding)
{
    return s[seqnum]->AddFrame(name, w, h, data, csize, encoding);
}

int CSprite::AddFrame(int seqnum, CSpriteFrame* frame)
{
    return s[seqnum]->AddFrame(frame);
}

void CSprite::SetPalette(const unsigned short* pal)
{
    if (p)
        delete p;
    p = new TPalette16(pal);
}

void CSprite::ResetPalette()
{
    SetPalette(TPalette16(TPalette24(p24->m_palette)));
}

unsigned short* CSprite::GetPalette()
{
    return p ? p->m_data : 0;
}

const unsigned short* CSprite::GetPalette() const
{
    return p ? p->m_data : 0;
}

void CSprite::ColorCycle(int begin, int end, int step)
{
    p->Cycle(begin, end, step);
}

void CSprite::Draw(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                   unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
                   bool hflip, bool tblit) const
{
    s[seqnum]->f[framenum]->Draw(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip,
        tblit);
}

void CSprite::DrawCreature(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                           unsigned short* dst, int dx, int dy, int dw, int dh,
                           int dpitch, bool hflip, unsigned short outcolor) const
{
    s[seqnum]->f[framenum]->DrawCreature(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p,
        hflip, outcolor);
}

void CSprite::DrawCreatureAlpha(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                                unsigned short* dst, int dx, int dy, int dw, int dh,
                                int dpitch, bool hflip, unsigned short outcolor) const
{
    s[seqnum]->f[framenum]->DrawCreatureAlpha(sx, sy, sw, sh, dst, dx, dy, dw, dh,
        dpitch, *p, hflip, outcolor);
}

void CSprite::DrawAdvObj(int framenum, int sx, int sy, int sw, int sh,
                         unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
                         bool hflip) const
{
    s[0]->f[framenum]->DrawAdvObj(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p,
        hflip);
}

void CSprite::DrawAdvObjWithFlag(int framenum, int sx, int sy, int sw, int sh,
                                 unsigned short* dst, int dx, int dy, int dw, int dh,
                                 int dpitch, unsigned short outcolor, bool hflip) const
{
    s[0]->f[framenum]->DrawAdvObjWithFlag(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
        *p, outcolor, hflip);
}

void CSprite::DrawAdvObjWithFlagAlpha(int framenum, int sx, int sy, int sw, int sh,
                                      unsigned short* dst, int dx, int dy, int dw,
                                      int dh, int dpitch, unsigned short outcolor,
                                      bool hflip) const
{
    s[0]->f[framenum]->DrawAdvObjWithFlagAlpha(sx, sy, sw, sh, dst, dx, dy, dw, dh,
        dpitch, *p, outcolor, hflip);
}

void CSprite::DrawAdvObjShadow(int framenum, int sx, int sy, int sw, int sh,
                               unsigned short* dst, int dx, int dy, int dw, int dh,
                               int dpitch, bool hflip) const
{
    s[0]->f[framenum]->DrawAdvObjShadow(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p,
        hflip);
}

void CSprite::DrawPointer(int framenum, unsigned short* dst, int dx, int dy, int dw,
                          int dh, int dpitch, bool hflip) const
{
    s[0]->f[framenum]->DrawPointer(dst, dx, dy, dw, dh, dpitch, *p, hflip);
}

void CSprite::DrawInterface(int framenum, int sx, int sy, int sw, int sh,
                            unsigned short* dst, int dx, int dy, int dw, int dh,
                            int dpitch, bool hflip) const
{
    s[0]->f[framenum]->DrawInterface(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p,
        hflip);
}

void CSprite::DrawTile(int framenum, unsigned short* dst, int dx, int dy, int dpitch,
                       bool hflip, bool vflip) const
{
    s[0]->f[framenum]->DrawTile(dst, dx, dy, dpitch, *p, hflip, vflip);
}

void CSprite::DrawTile(int framenum, int sx, int sy, int sw, int sh, unsigned short* dst,
                       int dx, int dy, int dw, int dh, int dpitch, bool hflip,
                       bool vflip) const
{
    s[0]->f[framenum]->DrawTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p, hflip,
        vflip);
}

void CSprite::DrawTileShadow(int framenum, int sx, int sy, int sw, int sh,
                             unsigned short* dst, int dx, int dy, int dw, int dh,
                             int dpitch, bool hflip, bool vflip) const
{
    s[0]->f[framenum]->DrawTileShadow(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p,
        hflip, vflip);
}

void CSprite::DrawShroudTile(int framenum, int sx, int sy, int sw, int sh,
                             unsigned short* dst, int dx, int dy, int dw, int dh,
                             int dpitch, bool hflip, bool vflip) const
{
    s[0]->f[framenum]->DrawShroudTile(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p,
        hflip, vflip);
}

void CSprite::DrawHero(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                       unsigned short* dst, int dx, int dy, int dw, int dh, int dpitch,
                       bool hflip) const
{
    s[seqnum]->f[framenum]->DrawHero(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p,
        hflip);
}

void CSprite::DrawHeroShadow(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                             unsigned short* dst, int dx, int dy, int dw, int dh,
                             int dpitch, bool hflip) const
{
    s[seqnum]->f[framenum]->DrawHeroShadow(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
        *p, hflip);
}

void CSprite::DrawHeroAlpha(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                            unsigned short* dst, int dx, int dy, int dw, int dh,
                            int dpitch, bool hflip) const
{
    s[seqnum]->f[framenum]->DrawHeroAlpha(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
        *p, hflip);
}

void CSprite::DrawCombatHero(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                             unsigned short* dst, int dx, int dy, int dw, int dh,
                             int dpitch, bool hflip) const
{
    s[seqnum]->f[framenum]->DrawCreature(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p,
        hflip, 0);
}

void CSprite::DrawSpellEffect(int seqnum, int framenum, int sx, int sy, int sw, int sh,
                              unsigned short* dst, int dx, int dy, int dw, int dh,
                              int dpitch, bool hflip, bool alpha) const
{
    s[seqnum]->f[framenum]->DrawSpellEffect(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch,
        *p, hflip, alpha);
}

void CSprite::DrawAdvObjWithFlagScaled50(int framenum, int sx, int sy, int sw, int sh,
                                         unsigned short* dst, int dx, int dy, int dw,
                                         int dh, int dpitch,
                                         unsigned short outcolor) const
{
    s[0]->f[framenum]->DrawAdvObjWithFlagScaled50(sx, sy, sw, sh, dst, dx, dy, dw, dh,
        dpitch, *p, outcolor);
}

void CSprite::DrawAdvObjShadowScaled50(int framenum, int sx, int sy, int sw, int sh,
                                       unsigned short* dst, int dx, int dy, int dw,
                                       int dh, int dpitch) const
{
    s[0]->f[framenum]->DrawAdvObjShadowScaled50(sx, sy, sw, sh, dst, dx, dy, dw, dh,
        dpitch, *p);
}

void CSprite::DrawTileScaled50(int framenum, int sx, int sy, int sw, int sh,
                               unsigned short* dst, int dx, int dy, int dw, int dh,
                               int dpitch, bool hflip, bool vflip) const
{
    s[0]->f[framenum]->DrawTileScaled50(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p,
        hflip, vflip);
}

void CSprite::DrawAdvObjWithFlagScaled25(int framenum, int sx, int sy, int sw, int sh,
                                         unsigned short* dst, int dx, int dy, int dw,
                                         int dh, int dpitch,
                                         unsigned short outcolor) const
{
    s[0]->f[framenum]->DrawAdvObjWithFlagScaled25(sx, sy, sw, sh, dst, dx, dy, dw, dh,
        dpitch, *p, outcolor);
}

void CSprite::DrawAdvObjShadowScaled25(int framenum, int sx, int sy, int sw, int sh,
                                       unsigned short* dst, int dx, int dy, int dw,
                                       int dh, int dpitch) const
{
    s[0]->f[framenum]->DrawAdvObjShadowScaled25(sx, sy, sw, sh, dst, dx, dy, dw, dh,
        dpitch, *p);
}

void CSprite::DrawTileScaled25(int framenum, int sx, int sy, int sw, int sh,
                               unsigned short* dst, int dx, int dy, int dw, int dh,
                               int dpitch, bool hflip, bool vflip) const
{
    s[0]->f[framenum]->DrawTileScaled25(sx, sy, sw, sh, dst, dx, dy, dw, dh, dpitch, *p,
        hflip, vflip);
}

int CSprite::GetSpriteType(const char* name)
{
    if (!strcasecmp(name, "sprite"))
        return RESOURCE_TYPE_SPRITE;
    if (!strcasecmp(name, "creature"))
        return RESOURCE_TYPE_CREATURE;
    if (!strcasecmp(name, "advobj"))
        return RESOURCE_TYPE_ADVENTURE_OBJECT;
    if (!strcasecmp(name, "hero"))
        return RESOURCE_TYPE_HERO;
    if (!strcasecmp(name, "tileset"))
        return RESOURCE_TYPE_TILESET;
    if (!strcasecmp(name, "pointer"))
        return RESOURCE_TYPE_POINTER;
    if (!strcasecmp(name, "interface"))
        return RESOURCE_TYPE_INTERFACE;
    if (!strcasecmp(name, "combathero"))
        return RESOURCE_TYPE_COMBAT_HERO;
    return RESOURCE_TYPE_INVALID;
}

const char* CSprite::GetSpriteTypeName(int type)
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
    case RESOURCE_TYPE_ADVENTURE_MASK:
        return 0;
    default:
        return 0;
    }
}

int CSprite::GetSequenceID(int type, const char* name)
{
    switch (type) {
    case RESOURCE_TYPE_CREATURE:
        if (!strcasecmp(name, "cs_walk")) return cs_walk;
        if (!strcasecmp(name, "cs_fidget")) return cs_fidget;
        if (!strcasecmp(name, "cs_wait")) return cs_wait;
        if (!strcasecmp(name, "cs_wince")) return cs_wince;
        if (!strcasecmp(name, "cs_defend")) return cs_defend;
        if (!strcasecmp(name, "cs_death")) return cs_death;
        if (!strcasecmp(name, "cs_specdeath")) return cs_specdeath;
        if (!strcasecmp(name, "cs_turn_rf")) return cs_turn_rf;
        if (!strcasecmp(name, "cs_turn_fr")) return cs_turn_fr;
        if (!strcasecmp(name, "cs_turn_lf")) return cs_turn_lf;
        if (!strcasecmp(name, "cs_turn_fl")) return cs_turn_fl;
        if (!strcasecmp(name, "cs_attack_ur")) return cs_attack_ur;
        if (!strcasecmp(name, "cs_attack_r")) return cs_attack_r;
        if (!strcasecmp(name, "cs_attack_dr")) return cs_attack_dr;
        if (!strcasecmp(name, "cs_range_ur")) return cs_range_ur;
        if (!strcasecmp(name, "cs_range_r")) return cs_range_r;
        if (!strcasecmp(name, "cs_range_dr")) return cs_range_dr;
        if (!strcasecmp(name, "cs_special_ur")) return cs_special_ur;
        if (!strcasecmp(name, "cs_special_r")) return cs_special_r;
        if (!strcasecmp(name, "cs_special_dr")) return cs_special_dr;
        if (!strcasecmp(name, "cs_prewalk")) return cs_prewalk;
        if (!strcasecmp(name, "cs_postwalk")) return cs_postwalk;
        break;
    case RESOURCE_TYPE_HERO:
        if (!strcasecmp(name, "hs_stand_n")) return hs_stand_n;
        if (!strcasecmp(name, "hs_stand_ne")) return hs_stand_ne;
        if (!strcasecmp(name, "hs_stand_e")) return hs_stand_e;
        if (!strcasecmp(name, "hs_stand_se")) return hs_stand_se;
        if (!strcasecmp(name, "hs_stand_s")) return hs_stand_s;
        if (!strcasecmp(name, "hs_walk_n")) return hs_walk_n;
        if (!strcasecmp(name, "hs_walk_ne")) return hs_walk_ne;
        if (!strcasecmp(name, "hs_walk_e")) return hs_walk_e;
        if (!strcasecmp(name, "hs_walk_se")) return hs_walk_se;
        if (!strcasecmp(name, "hs_walk_s")) return hs_walk_s;
        if (!strcasecmp(name, "hs_turn_n_ne")) return hs_turn_n_ne;
        if (!strcasecmp(name, "hs_turn_ne_n")) return hs_turn_ne_n;
        if (!strcasecmp(name, "hs_turn_ne_e")) return hs_turn_ne_e;
        if (!strcasecmp(name, "hs_turn_e_ne")) return hs_turn_e_ne;
        if (!strcasecmp(name, "hs_turn_e_se")) return hs_turn_e_se;
        if (!strcasecmp(name, "hs_turn_se_e")) return hs_turn_se_e;
        if (!strcasecmp(name, "hs_turn_se_s")) return hs_turn_se_s;
        if (!strcasecmp(name, "hs_turn_s_se")) return hs_turn_s_se;
        break;
    case RESOURCE_TYPE_COMBAT_HERO:
        if (!strcasecmp(name, "chs_stand")) return combatHeroStand;
        if (!strcasecmp(name, "chs_fidget")) return combatHeroFidget;
        if (!strcasecmp(name, "chs_defeat")) return combatHeroDefeat;
        if (!strcasecmp(name, "chs_victory")) return combatHeroVictory;
        if (!strcasecmp(name, "chs_cast")) return combatHeroCast;
        break;
    case RESOURCE_TYPE_SPRITE:
    case RESOURCE_TYPE_ADVENTURE_OBJECT:
    case RESOURCE_TYPE_TILESET:
    case RESOURCE_TYPE_POINTER:
    case RESOURCE_TYPE_INTERFACE:
        if (!strcasecmp(name, "default")) return 0;
        break;
    case RESOURCE_TYPE_SPRITE_DEFINITION:
    case RESOURCE_TYPE_SPRITE_FRAME:
    case RESOURCE_TYPE_ADVENTURE_MASK:
        return -1;
    }
    return -1;
}

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
