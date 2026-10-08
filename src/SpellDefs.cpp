// SpellDefs.cpp of the Loki port (Loki object 30): the RoE spell traits
// table and its sptraits.txt loader, in Loki's function order.
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "spelldefs.h"

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
