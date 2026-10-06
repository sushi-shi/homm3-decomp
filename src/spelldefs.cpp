#include "va.h"

#include <stdlib.h>
#include <string.h>

#include "spelldefs.h"

#include "resource.h"
#include "resourcemanager.h"
#include "textresource.h"

// Retail initial spell traits retain sample names, effects and flags before
// sptraits.txt supplies localized text, costs and probabilities.
DATA(0x00685450)
TSpellTraits g_spellTraitsImp[NUM_SPELLS_AND_CREATURE_EFFECTS] = {
    { 0, "SummBoat.wav", eSpellEffectNone, 0x100002, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "ScutBoat.wav", eSpellEffectNone, 0x2, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Visions.wav", eSpellEffectNone, 0x2, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, 0, eSpellEffectNone, 0x2, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Disguise.wav", eSpellEffectNone, 0x2, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, 0, eSpellEffectNone, 0x2, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "FlySpell.wav", eSpellEffectNone, 0x100002, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "WatrWalk.wav", eSpellEffectNone, 0x100002, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Telptout.wav", eSpellEffectNone, 0x100002, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Telptout.wav", eSpellEffectNone, 0x100002, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, 0, eSpellEffectQuicksand, 0x81, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, 0, eSpellEffectLandMine, 0x81, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Forcefld.wav", eSpellEffectNone, 0x81, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Firewall.wav", eSpellEffectFirewall1, 0x81, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Erthquak.wav", eSpellEffectNone, 0x100001, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "MagicBlt.wav", eSpellEffectMagicBoltBurst, 0x8211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "IceRay.wav", eSpellEffectIceRayBurst, 0x8211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "LightBlt.wav", eSpellEffectLightningDust, 0x8211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Decay.wav", eSpellEffectDecay, 0x9211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Chainlte.wav", eSpellEffectChainLightningDust, 0x10211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Frosting.wav", eSpellEffectFrostRing, 0x8281, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Spontcomb.wav", eSpellEffectSpontaneousCombustion, 0x8281, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Fireblst.wav", eSpellEffectFireblast, 0x10281, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Meteor.wav", eSpellEffectMeteorShower, 0x8281, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Deathrip.wav", eSpellEffectDeathRipple, 0x21201, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Sacbreth.wav", eSpellEffectSacredBreath, 0x20201, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Firestrm.wav", eSpellEffectFirestorm, 0x20201, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Shield.wav", eSpellEffectShield, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Airsheld.wav", eSpellEffectAirShield, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Fireshld.wav", eSpellEffectFireShield, 0x44815, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Protecta.wav", eSpellEffectProtectionFromAir, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Protectf.wav", eSpellEffectProtectionFromFire, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Protectw.wav", eSpellEffectProtectionFromWater, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Protecte.wav", eSpellEffectProtectionFromEarth, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Antimagk.wav", eSpellEffectAntiMagic, 0x44815, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Dispell.wav", eSpellEffectDispel, 0x40041, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Backlash.wav", eSpellEffectBacklash, 0x44815, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Cure.wav", eSpellEffectCure, 0x40841, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Resurect.wav", eSpellEffectResurrection, 0x81011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Animdead.wav", eSpellEffectAnimateDead, 0x81011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Sacrif1.wav", eSpellEffectSacrificeResurrect, 0x1011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Bless.wav", eSpellEffectBless, 0x40845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Curse", eSpellEffectCurse, 0x40045, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "BloodLus.wav", eSpellEffectNone, 0x41845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Precison.wav", eSpellEffectPrecision, 0x40845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Weakness.wav", eSpellEffectWeakness, 0x40045, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Tuffskin.wav", eSpellEffectToughSkin, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Disruptr.wav", eSpellEffectDisruptiveRayBurst, 0x40011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Prayer.wav", eSpellEffectPrayer, 0x40845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Mirth.wav", eSpellEffectMirth, 0x40c45, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Sorrow.wav", eSpellEffectSorrow, 0x40445, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Fortune.wav", eSpellEffectFortune, 0x40845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Misfort.wav", eSpellEffectMisfortune, 0x40045, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Tailwind.wav", eSpellEffectTailWind, 0x41845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Muckmire.wav", eSpellEffectMuckAndMire, 0x41045, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Slayer.wav", eSpellEffectSlayer, 0x40815, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Frenzy.wav", eSpellEffectFrenzy, 0x40c15, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "LightBlt.wav", eSpellEffectLightningDust, 0x10a211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Cntrstrk.wav", eSpellEffectCounterstroke, 0x45845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Berserk.wav", eSpellEffectBerserk, 0x41485, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Hypnotiz.wav", eSpellEffectHypnotize, 0x81415, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Forget.wav", eSpellEffectForgetfulness, 0x1425, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Blind.wav", eSpellEffectBlind, 0x41415, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "TelptOut.wav", eSpellEffectNone, 0x41011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Removeob.wav", eSpellEffectRemoveObstacle, 0x101, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Clone.wav", eSpellEffectNone, 0x41011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "SumnElm.wav", eSpellEffectNone, 0x80001, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "SumnElm.wav", eSpellEffectNone, 0x80001, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "SumnElm.wav", eSpellEffectNone, 0x80001, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "SumnElm.wav", eSpellEffectNone, 0x80001, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Paralyze.wav", eSpellEffectNone, 0x101c, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Poison.wav", eSpellEffectPoison, 0x101c, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Bind.wav", eSpellEffectBind, 0x1018, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Disease.wav", eSpellEffectDisease, 0x101c, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Paralyze.wav", eSpellEffectParalyze, 0x101c, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Age.wav", eSpellEffectAge, 0x101c, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Deathcld.wav", eSpellEffectDeathCloud, 0x18, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "LightBlt.wav", eSpellEffectLightningDust, 0x218, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Dispell.wav", eSpellEffectDispel, 0x18, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Deathstr.wav", eSpellEffectDeathStare, 0x18, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Acid.wav", eSpellEffectPoof, 0x18, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
};

DATA(0x00687f58) const TSpellTraits (&akSpellTraits)[NUM_SPELLS_AND_CREATURE_EFFECTS] = g_spellTraitsImp;

static void initializeSpellTraits(
    int id, const std::vector<char*, std::allocator<char*> >& resource);

VA(0x0059e060, 0x30)
DC_ADDRESS(0x14e278, 0x50)
MAC_ADDRESS(0x18e73c, 0x7c)
unsigned char spellTargetsASingleArmy(int spell, int sslevel)
{
    unsigned int flags = akSpellTraits[spell].m_flags;
    unsigned int result;
    if ((flags & SPELL_TARGET_ALWAYS_SINGLE)
        || ((flags & SPELL_TARGET_MASS_AT_EXPERT) && sslevel <= 2)
        || ((flags & SPELL_TARGET_MASS_AT_ADVANCED) && sslevel <= 1))
        result = 1;
    else
        result = 0;
    return result;
}

VA(0x0059e090, 0xB7)
DC_ADDRESS(0x14e2c8, 0xd4)
MAC_ADDRESS(0x18e7b8, 0x13c)
unsigned char InitializeSpellTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::GetSpreadsheet(
        DATA_COMPGEN(0x0068830c, spellTraitsSpreadsheetName,
                     "sptraits.txt"));
    if (!resource)
        return 0;

    if (resource->GetNumberOfRows() < 92) {
        ResourceManager::Dispose(resource);
        return 0;
    }

    int spell = 0;
    int row = 5;
    for (; spell < 10; ++spell, ++row)
        initializeSpellTraits(spell, resource->GetRow(row));

    row += 3;
    int count = 60;
    while (count--) {
        initializeSpellTraits(spell, resource->GetRow(row));
        ++spell;
        ++row;
    }

    row += 3;
    count = 11;
    while (count--) {
        initializeSpellTraits(spell, resource->GetRow(row));
        ++spell;
        ++row;
    }

    ResourceManager::Dispose(resource);
    return 1;
}

namespace {

// CodeView field pStr; each loader owns its own private string class.
class TAutoStrPtr {
public:

    // E:\gamedcs\spelldefs.cpp:320
    DC_ADDRESS(0x14e78c, 0x8)
    TAutoStrPtr() : m_string(0) {}

    // E:\gamedcs\spelldefs.cpp:321
    DC_ADDRESS(0x14e794, 0x18)
    ~TAutoStrPtr() { delete[] m_string; }

    // E:\gamedcs\spelldefs.cpp:323
    DC_ADDRESS(0x14e7ac, 0x4)
    void set(char* value) { m_string = value; }

    // E:\gamedcs\spelldefs.cpp:325
    DC_ADDRESS(0x14e7b0, 0x4)
    char* get() const { return m_string; }

    // Project-inferred allocation/copy used to initialize owned table text.
    void copyText(const char* source);

private:
    char* m_string;
};

// Keep the native Set/Get operations and store ownership before copying.
// Like the existing initialization sequence, this does not release an old value.
void TAutoStrPtr::copyText(const char* source)
{
    set(new char[strlen(source) + 1]);
    strcpy(get(), source);
}

}

VA(0x0059e150, 0x35F)
DC_ADDRESS(0x14e39c, 0x362)
MAC_ADDRESS(0x18e8f4, 0x3a4)
static void initializeSpellTraits(
    int id, const std::vector<char*, std::allocator<char*> >& resource)
{
    TSpellTraits& traits = g_spellTraitsImp[id];

    DATA_COMPGEN_GUARD(0x006a3650, spellStringsGuard, spellNames)
    DATA(0x006a350c)
    static TAutoStrPtr spellNames[NUM_SPELLS_AND_CREATURE_EFFECTS];

    spellNames[id].set(new char[strlen(resource[0]) + 1]);
    strcpy(spellNames[id].get(), resource[0]);
    traits.m_name = spellNames[id].get();

    DATA(0x006a3654)
    static TAutoStrPtr abbreviatedSpellNames[NUM_SPELLS_AND_CREATURE_EFFECTS];

    abbreviatedSpellNames[id].set(new char[strlen(resource[1]) + 1]);
    strcpy(abbreviatedSpellNames[id].get(), resource[1]);
    traits.m_abbreviated_name = abbreviatedSpellNames[id].get();

    traits.m_level = atoi(resource[2]);
    traits.m_schoolBits = 0;
    if (resource[3][0] && resource[3][0] != ' ')
        traits.m_schoolBits |= 8;
    if (resource[4][0] && resource[4][0] != ' ')
        traits.m_schoolBits |= 4;
    if (resource[5][0] && resource[5][0] != ' ')
        traits.m_schoolBits |= 2;
    if (resource[6][0] && resource[6][0] != ' ')
        traits.m_schoolBits |= 1;

    int column = 7;
    int i;
    for (i = 0; i < 4; ++i) {
        traits.m_manaCost[i] = atoi(resource[column]);
        ++column;
    }

    traits.m_power_factor = atoi(resource[column++]);

    for (i = 0; i < 4; ++i) {
        traits.m_mastery_bonus[i] = atoi(resource[column]);
        ++column;
    }

    for (i = 0; i < 9; ++i) {
        traits.m_townGetsItChance[i] = atoi(resource[column]);
        ++column;
    }

    for (i = 0; i < 4; ++i) {
        traits.m_AI_value[i] = atoi(resource[column]);
        ++column;
    }

    DATA(0x006a3798)
    static TAutoStrPtr spellDescriptions[NUM_SPELLS_AND_CREATURE_EFFECTS][4];

    for (i = 0; i < 4; ++i) {
        spellDescriptions[id][i].set(
            new char[strlen(resource[column]) + 1]);
        strcpy(spellDescriptions[id][i].get(), resource[column]);
        traits.m_description[i] = spellDescriptions[id][i].get();
        ++column;
    }
}

VA_COMPGEN(0x0059e4b0, 0x17, STATIC_DTOR, spellDescriptions)

VA_COMPGEN(0x0059e4d0, 0x14, STATIC_DTOR, abbreviatedSpellNames)

VA_COMPGEN(0x0059e4f0, 0x14, STATIC_DTOR, spellNames)
