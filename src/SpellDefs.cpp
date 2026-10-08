// SpellDefs.cpp of the Loki port (Loki object 30): the RoE spell traits
// table and its sptraits.txt loader, in Loki's function order.
#include "spelldefs.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "resourcemanager.h"
#include "textresource.h"

// Karma, sound, battle effect and class flags are compiled in; sptraits.txt
// fills the names, level, schools, costs, power, effects, town chances, AI
// values and descriptions (InitializeSpellTraits).
static TSpellTraits aSpellTraitsImp[kNumSpellsAndCreatureEffects] = {
    { 0, "SummBoat.wav", eSpellEffectNone, 0x80002 },
    { 0, "ScutBoat.wav", eSpellEffectNone, 0x2 },
    { 0, "Visions.wav", eSpellEffectNone, 0x2 },
    { 0, 0, eSpellEffectNone, 0x2 },
    { 0, "Disguise.wav", eSpellEffectNone, 0x2 },
    { 0, 0, eSpellEffectNone, 0x2 },
    { 0, "FlySpell.wav", eSpellEffectNone, 0x80002 },
    { 0, "WatrWalk.wav", eSpellEffectNone, 0x80002 },
    { 0, "Telptout.wav", eSpellEffectNone, 0x80002 },
    { 0, "Telptout.wav", eSpellEffectNone, 0x80002 },
    { 0, 0, eSpellEffectQuicksand, 0x81 },
    { 0, 0, eSpellEffectLandMine, 0x81 },
    { 0, "Forcefld.wav", eSpellEffectNone, 0x81 },
    { 0, "Firewall.wav", eSpellEffectFirewall1, 0x81 },
    { 0, "Erthquak.wav", eSpellEffectNone, 0x80001 },
    { -1, "MagicBlt.wav", eSpellEffectMagicBoltBurst, 0x4211 },
    { -1, "IceRay.wav", eSpellEffectIceRayBurst, 0x4211 },
    { -1, "LightBlt.wav", eSpellEffectLightningDust, 0x4211 },
    { -1, "Decay.wav", eSpellEffectDecay, 0x5211 },
    { 0, "Chainlte.wav", eSpellEffectChainLightningDust, 0x8211 },
    { 0, "Frosting.wav", eSpellEffectFrostRing, 0x4281 },
    { 0, "Spontcomb.wav", eSpellEffectSpontaneousCombustion, 0x4281 },
    { 0, "Fireblst.wav", eSpellEffectFireblast, 0x8281 },
    { 0, "Meteor.wav", eSpellEffectMeteorShower, 0x4281 },
    { 0, "Deathrip.wav", eSpellEffectDeathRipple, 0x11201 },
    { 0, "Sacbreth.wav", eSpellEffectSacredBreath, 0x10201 },
    { 0, "Firestrm.wav", eSpellEffectFirestorm, 0x10201 },
    { 1, "Shield.wav", eSpellEffectShield, 0x22845 },
    { 1, "Airsheld.wav", eSpellEffectAirShield, 0x22845 },
    { 1, "Fireshld.wav", eSpellEffectFireShield, 0x22815 },
    { 1, "Protecta.wav", eSpellEffectProtectionFromAir, 0x22845 },
    { 1, "Protectf.wav", eSpellEffectProtectionFromFire, 0x22845 },
    { 1, "Protectw.wav", eSpellEffectProtectionFromWater, 0x22845 },
    { 1, "Protecte.wav", eSpellEffectProtectionFromEarth, 0x22845 },
    { 1, "Antimagk.wav", eSpellEffectAntiMagic, 0x22815 },
    { 0, "Dispell.wav", eSpellEffectDispel, 0x20041 },
    { 1, "Backlash.wav", eSpellEffectBacklash, 0x22815 },
    { 1, "Cure.wav", eSpellEffectCure, 0x20841 },
    { 1, "Resurect.wav", eSpellEffectResurrection, 0x41011 },
    { 1, "Animdead.wav", eSpellEffectAnimateDead, 0x41011 },
    { 1, "Sacrif1.wav", eSpellEffectSacrificeResurrect, 0x1011 },
    { 1, "Bless.wav", eSpellEffectBless, 0x20845 },
    { -1, "Curse", eSpellEffectCurse, 0x20045 },
    { 1, "BloodLus.wav", eSpellEffectNone, 0x21845 },
    { 1, "Precison.wav", eSpellEffectPrecision, 0x20845 },
    { -1, "Weakness.wav", eSpellEffectWeakness, 0x20045 },
    { 1, "Tuffskin.wav", eSpellEffectToughSkin, 0x22845 },
    { -1, "Disruptr.wav", eSpellEffectDisruptiveRayBurst, 0x20011 },
    { 1, "Prayer.wav", eSpellEffectPrayer, 0x20845 },
    { 1, "Mirth.wav", eSpellEffectMirth, 0x20c45 },
    { -1, "Sorrow.wav", eSpellEffectSorrow, 0x20445 },
    { 1, "Fortune.wav", eSpellEffectFortune, 0x20845 },
    { -1, "Misfort.wav", eSpellEffectMisfortune, 0x20045 },
    { 1, "Tailwind.wav", eSpellEffectTailWind, 0x21845 },
    { -1, "Muckmire.wav", eSpellEffectMuckAndMire, 0x21045 },
    { 1, "Slayer.wav", eSpellEffectSlayer, 0x20815 },
    { 1, "Frenzy.wav", eSpellEffectFrenzy, 0x20c15 },
    { -1, "Fear.wav", eSpellEffectFear, 0x20415 },
    { 1, "Cntrstrk.wav", eSpellEffectCounterstroke, 0x23845 },
    { -1, "Berserk.wav", eSpellEffectBerserk, 0x21485 },
    { -1, "Hypnotiz.wav", eSpellEffectHypnotize, 0x41415 },
    { -1, "Forget.wav", eSpellEffectForgetfulness, 0x1425 },
    { -1, "Blind.wav", eSpellEffectBlind, 0x21415 },
    { 1, "TelptOut.wav", eSpellEffectNone, 0x21011 },
    { 0, "Removeob.wav", eSpellEffectRemoveObstacle, 0x101 },
    { 1, "Clone.wav", eSpellEffectNone, 0x21011 },
    { 0, "SumnElm.wav", eSpellEffectNone, 0x40001 },
    { 0, "SumnElm.wav", eSpellEffectNone, 0x40001 },
    { 0, "SumnElm.wav", eSpellEffectNone, 0x40001 },
    { 0, "SumnElm.wav", eSpellEffectNone, 0x40001 },
    { -1, "Paralyze.wav", eSpellEffectNone, 0x101c },
    { -1, "Poison.wav", eSpellEffectPoison, 0x101c },
    { -1, "Bind.wav", eSpellEffectBind, 0x1018 },
    { -1, "Disease.wav", eSpellEffectDisease, 0x101c },
    { -1, "Paralyze.wav", eSpellEffectParalyze, 0x101c },
    { -1, "Age.wav", eSpellEffectAge, 0x101c },
    { 0, "Deathcld.wav", eSpellEffectDeathCloud, 0x18 },
    { -1, "LightBlt.wav", eSpellEffectLightningDust, 0x218 },
    { -1, "Dispell.wav", eSpellEffectDispel, 0x18 },
    { -1, "Deathstr.wav", eSpellEffectDeathStare, 0x18 },
};

const TSpellTraits (&akSpellTraits)[kNumSpellsAndCreatureEffects] = aSpellTraitsImp;

#ifdef HOMM3_TARGET_LOKI
// The combat spell effects' sprites: the Loki port exports them from this
// object (the Windows game keeps g_spellEffectTraits in cmbtmgr.cpp), and
// nothing in the editor reads them. The Loki record is the Dreamcast roster's eight
// bytes (?akSpellEffectTraits@@3QBUTSpellEffectTraits@@B, m_name at 0):
// the sprite and the placement/alpha flags the Windows game's
// TSpellEffectTraits (cmbtmgr.h) keeps at +8 behind its Immersion name.
struct TSpellEffectTraits {
    const char* m_name;
    unsigned int m_flags;
};

extern const TSpellEffectTraits akSpellEffectTraits[82];

const TSpellEffectTraits akSpellEffectTraits[82] = {
    { "C10spW.def", 256 },
    { "C11spA0.def", 2 },
    { "C01spA0.def", 1 },
    { "C02spA0.def", 1 },
    { "C01spE0.def", 1 },
    { "C02spE0.def", 1 },
    { "C02spF0.def", 1 },
    { "C04spA0.def", 1 },
    { "C04spE0.def", 1 },
    { "C04spF0.def", 1 },
    { "C05spE0.def", 0 },
    { "C05spF0.def", 1 },
    { "C06spF0.def", 15 },
    { "C07spA0.def", 15 },
    { "C07spA1.def", 257 },
    { "C07spE0.def", 257 },
    { "C08spE0.def", 1 },
    { "C08spF0.def", 1 },
    { "C09spA0.def", 1 },
    { "C09spE0.def", 0 },
    { "C09spW0.def", 1 },
    { "C10spA0.def", 1 },
    { "C11spE0.def", 0 },
    { "C11spF0.def", 0 },
    { "C11spW0.def", 0 },
    { "C12spA0.def", 1 },
    { "C13spA0.def", 0 },
    { "C13spE0.def", 1 },
    { "C13spW0.def", 1 },
    { "C14spA0.def", 257 },
    { "C14spE0.def", 1 },
    { "C15spA0.def", 0 },
    { "C15spE0.def", 0 },
    { "C15spE9.def", 0 },
    { "C18spW0.def", 0 },
    { "C01spF0.def", 1 },
    { "C01spW0.def", 1 },
    { "C03spA0.def", 2 },
    { "C03spA1.def", 1 },
    { "C03spW0.def", 1 },
    { "C04spW0.def", 1 },
    { "C05spW0.def", 1 },
    { "C06spW0.def", 1 },
    { "C07spF0.def", 4 },
    { "C07spF9.def", 4 },
    { "C07spW0.def", 1 },
    { "C08spW5.def", 257 },
    { "C09spF0.def", 4 },
    { "C10spF0.def", 1 },
    { "C11spA1.def", 0 },
    { "C12spE0.def", 257 },
    { "C12spF0.def", 1 },
    { "C12spF1.def", 257 },
    { "C13spF.def", 1 },
    { "C16spE0.def", 1 },
    { "C17spE0.def", 4 },
    { "C17spW0.def", 1 },
    { "C09spF3.def", 0 },
    { "C17spE2.def", 4 },
    { "C09spF2.def", 4 },
    { "C15spE2.def", 4 },
    { "C15spE11.def", 4 },
    { "C07spF2.def", 4 },
    { "C07spF11.def", 4 },
    { "C20SPX.DEF", 1 },
    { "C07spF60.def", 4 },
    { "C07spF62.def", 4 },
    { "sp11_.def", 1 },
    { "sp02_.def", 0 },
    { "sp05_.def", 1 },
    { "sp10_.def", 0 },
    { "sp01_.def", 1 },
    { "sp04_.def", 271 },
    { "sp03_.def", 1 },
    { "sp06_.def", 257 },
    { "sp07_A.def", 1 },
    { "sp07_B.def", 1 },
    { "sp08_.def", 0 },
    { "sp09_.def", 3 },
    { "sp12_.def", 257 },
    { "c07spe0.def", 1 },
    { "poof.def", 0 }
};
#endif

namespace {

// Owns one loaded string; the loader keeps the pointers in the traits rows.
class TAutoStrPtr {
public:
    TAutoStrPtr() : str(0) {}
    ~TAutoStrPtr() { delete[] str; }
    void set(char* newStr) { str = newStr; }
    char* get() const { return str; }

private:
    char* str;
};

}

static void InitializeSpellTraits(int id, const vector<char*>& resource);

bool SpellTargetsASingleArmy(int spell, int sslevel)
{
    const unsigned int flags = akSpellTraits[spell].m_flags;
    return (flags & SPELL_TARGET_ALWAYS_SINGLE)
        || ((flags & SPELL_TARGET_MASS_AT_EXPERT) && sslevel <= 2)
        || ((flags & SPELL_TARGET_MASS_AT_ADVANCED) && sslevel <= 1);
}

bool InitializeSpellTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::GetSpreadsheet("sptraits.txt");
    if (!resource)
        return false;
    if (resource->GetNumberOfRows() < 91) {
        ResourceManager::Dispose(resource);
        return false;
    }

    int i;
    int row = 0;
    int id = 0;
    row += 5;
    for (i = 0; i < 10; i++) {
        InitializeSpellTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    row += 3;
    for (i = 0; i < 60; i++) {
        InitializeSpellTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    row += 3;
    for (i = 0; i < 10; i++) {
        InitializeSpellTraits(id, resource->GetRow(row));
        id++;
        row++;
    }
    ResourceManager::Dispose(resource);
    return true;
}

static void InitializeSpellTraits(int id, const vector<char*>& resource)
{
#line 336
    assert(id >= 0 && id < kNumSpellsAndCreatureEffects);
    assert(resource.size() >= 32);
    TSpellTraits* const traits = &aSpellTraitsImp[id];

    static TAutoStrPtr names[kNumSpellsAndCreatureEffects];
    names[id].set(new char[strlen(resource[0]) + 1]);
    strcpy(names[id].get(), resource[0]);
    traits->m_name = names[id].get();

    static TAutoStrPtr abbreviatedNames[kNumSpellsAndCreatureEffects];
    abbreviatedNames[id].set(new char[strlen(resource[1]) + 1]);
    strcpy(abbreviatedNames[id].get(), resource[1]);
    traits->m_abbreviatedName = abbreviatedNames[id].get();

    traits->m_level = atoi(resource[2]);
    traits->m_schoolBits = 0;
    if (*resource[3] && *resource[3] != ' ')
        traits->m_schoolBits |= 8;
    if (*resource[4] && *resource[4] != ' ')
        traits->m_schoolBits |= 4;
    if (*resource[5] && *resource[5] != ' ')
        traits->m_schoolBits |= 2;
    if (*resource[6] && *resource[6] != ' ')
        traits->m_schoolBits |= 1;

    int i;
    int col = 7;
    for (i = 0; i < 4; i++) {
        traits->m_manaCost[i] = atoi(resource[col]);
        col++;
    }
#line 385
    assert(col == 11);
    traits->m_powerFactor = atoi(resource[col++]);
#line 388
    assert(col == 12);
    for (i = 0; i < 4; i++) {
        traits->m_masteryBonus[i] = atoi(resource[col]);
        col++;
    }
#line 397
    assert(col == 16);
    for (i = 0; i < 8; i++) {
        traits->m_townGetsItChance[i] = atoi(resource[col]);
        col++;
    }
#line 406
    assert(col == 24);
    for (i = 0; i < 4; i++) {
        traits->m_masteryValues[i] = atoi(resource[col]);
        col++;
    }

    static TAutoStrPtr descriptions[kNumSpellsAndCreatureEffects][4];
#line 417
    assert(col == 28);
    for (i = 0; i < 4; i++) {
        descriptions[id][i].set(new char[strlen(resource[col]) + 1]);
        strcpy(descriptions[id][i].get(), resource[col]);
        traits->m_levelDescriptions[i] = descriptions[id][i].get();
        col++;
    }
}
