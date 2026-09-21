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
SSpellTraits g_spellTraitsImp[81] = {
    { 0, "SummBoat.wav", -1, 0x100002, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "ScutBoat.wav", -1, 0x2, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Visions.wav", -1, 0x2, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, 0, -1, 0x2, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Disguise.wav", -1, 0x2, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, 0, -1, 0x2, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "FlySpell.wav", -1, 0x100002, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "WatrWalk.wav", -1, 0x100002, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Telptout.wav", -1, 0x100002, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Telptout.wav", -1, 0x100002, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, 0, 55, 0x81, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, 0, 47, 0x81, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Forcefld.wav", -1, 0x81, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Firewall.wav", 65, 0x81, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Erthquak.wav", -1, 0x100001, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "MagicBlt.wav", 64, 0x8211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "IceRay.wav", 46, 0x8211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "LightBlt.wav", 49, 0x8211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Decay.wav", 10, 0x9211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Chainlte.wav", 38, 0x10211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Frosting.wav", 45, 0x8281, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Spontcomb.wav", 53, 0x8281, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Fireblst.wav", 9, 0x10281, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Meteor.wav", 16, 0x8281, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Deathrip.wav", 8, 0x21201, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Sacbreth.wav", 29, 0x20201, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Firestrm.wav", 12, 0x20201, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Shield.wav", 27, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Airsheld.wav", 2, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Fireshld.wav", 11, 0x44815, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Protecta.wav", 22, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Protectf.wav", 24, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Protectw.wav", 23, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Protecte.wav", 26, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Antimagk.wav", 5, 0x44815, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Dispell.wav", 41, 0x40041, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Backlash.wav", 3, 0x44815, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Cure.wav", 39, 0x40841, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Resurect.wav", 50, 0x81011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Animdead.wav", 4, 0x81011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Sacrif1.wav", 52, 0x1011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Bless.wav", 36, 0x40845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Curse", 40, 0x40045, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "BloodLus.wav", -1, 0x41845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Precison.wav", 25, 0x40845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Weakness.wav", 56, 0x40045, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Tuffskin.wav", 54, 0x44845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Disruptr.wav", 14, 0x40011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Prayer.wav", 0, 0x40845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Mirth.wav", 20, 0x40c45, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Sorrow.wav", 30, 0x40445, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Fortune.wav", 18, 0x40845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Misfort.wav", 48, 0x40045, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Tailwind.wav", 31, 0x41845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Muckmire.wav", 19, 0x41045, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Slayer.wav", 28, 0x40815, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Frenzy.wav", 17, 0x40c15, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "LightBlt.wav", 49, 0x10a211, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Cntrstrk.wav", 7, 0x45845, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Berserk.wav", 35, 0x41485, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Hypnotiz.wav", 21, 0x81415, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Forget.wav", 42, 0x1425, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Blind.wav", 6, 0x41415, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "TelptOut.wav", -1, 0x41011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Removeob.wav", 34, 0x101, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 1, "Clone.wav", -1, 0x41011, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "SumnElm.wav", -1, 0x80001, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "SumnElm.wav", -1, 0x80001, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "SumnElm.wav", -1, 0x80001, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "SumnElm.wav", -1, 0x80001, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Paralyze.wav", -1, 0x101c, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Poison.wav", 67, 0x101c, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Bind.wav", 68, 0x1018, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Disease.wav", 69, 0x101c, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Paralyze.wav", 70, 0x101c, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Age.wav", 71, 0x101c, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { 0, "Deathcld.wav", 72, 0x18, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "LightBlt.wav", 49, 0x218, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Dispell.wav", 41, 0x18, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Deathstr.wav", 80, 0x18, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
    { -1, "Acid.wav", 81, 0x18, 0, 0, 0, { TSpellSchool(0) }, { 0, 0, 0, 0 }, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },
};

DATA(0x00687f58) const SSpellTraits (&g_spellTraits)[81] = g_spellTraitsImp;

static void initializeSpellTraits(
    int id, const std::vector<char*, std::allocator<char*> >& resource);

VA(0x0059e060, 0x30)  // dc 0x14e278
unsigned char spellTargetsASingleArmy(int spell, int sslevel)
{
    unsigned int flags = g_spellTraits[spell].m_flags;
    unsigned int result;
    if ((flags & SPELL_TARGET_ALWAYS_SINGLE)
        || ((flags & SPELL_TARGET_MASS_AT_EXPERT) && sslevel <= 2)
        || ((flags & SPELL_TARGET_MASS_AT_ADVANCED) && sslevel <= 1))
        result = 1;
    else
        result = 0;
    return result;
}

VA(0x0059e090, 0xB7)  // dc 0x14e2c8
unsigned char initializeSpellTraitsTable()
{
    TSpreadsheetResource* resource = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x0068830c, spellTraitsSpreadsheetName,
                     "sptraits.txt"));
    if (!resource)
        return 0;

    if (resource->getNumberOfRows() < 92) {
        resource->dispose();
        return 0;
    }

    int spell = 0;
    int row = 5;
    for (; spell < 10; ++spell, ++row)
        initializeSpellTraits(spell, resource->getRow(row));

    row += 3;
    int count = 60;
    while (count--) {
        initializeSpellTraits(spell, resource->getRow(row));
        ++spell;
        ++row;
    }

    row += 3;
    count = 11;
    while (count--) {
        initializeSpellTraits(spell, resource->getRow(row));
        ++spell;
        ++row;
    }

    resource->dispose();
    return 1;
}

namespace {

// CodeView field pStr; each loader owns its own private string class.
class TAutoStrPtr {
public:
    // E:\gamedcs\spelldefs.cpp:320, dc 0x14e78c
    TAutoStrPtr() : m_string(0) {}
    // E:\gamedcs\spelldefs.cpp:321, dc 0x14e794
    ~TAutoStrPtr() { delete[] m_string; }
    // E:\gamedcs\spelldefs.cpp:323, dc 0x14e7ac
    void set(char* value) { m_string = value; }
    // E:\gamedcs\spelldefs.cpp:325, dc 0x14e7b0
    char* get() const { return m_string; }

private:
    char* m_string;
};

}

VA(0x0059e150, 0x35F)  // dc 0x14e39c
static void initializeSpellTraits(
    int id, const std::vector<char*, std::allocator<char*> >& resource)
{
    SSpellTraits& traits = g_spellTraitsImp[id];

    DATA_COMPGEN_GUARD(0x006a3650, spellStringsGuard, spellNames)
    DATA(0x006a350c)
    static TAutoStrPtr spellNames[81];

    spellNames[id].set(new char[strlen(resource[0]) + 1]);
    strcpy(spellNames[id].get(), resource[0]);
    traits.m_name = spellNames[id].get();

    DATA(0x006a3654)
    static TAutoStrPtr abbreviatedSpellNames[81];

    abbreviatedSpellNames[id].set(new char[strlen(resource[1]) + 1]);
    strcpy(abbreviatedSpellNames[id].get(), resource[1]);
    traits.m_abbreviatedName = abbreviatedSpellNames[id].get();

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

    traits.m_powerFactor = atoi(resource[column++]);

    for (i = 0; i < 4; ++i) {
        traits.m_masteryBonus[i] = atoi(resource[column]);
        ++column;
    }

    for (i = 0; i < 9; ++i) {
        traits.m_townProbability[i] = atoi(resource[column]);
        ++column;
    }

    for (i = 0; i < 4; ++i) {
        traits.m_masteryValues[i] = atoi(resource[column]);
        ++column;
    }

    DATA(0x006a3798)
    static TAutoStrPtr spellDescriptions[81][4];

    for (i = 0; i < 4; ++i) {
        spellDescriptions[id][i].set(
            new char[strlen(resource[column]) + 1]);
        strcpy(spellDescriptions[id][i].get(), resource[column]);
        traits.m_levelDescriptions[i] = spellDescriptions[id][i].get();
        ++column;
    }
}

VA_COMPGEN(0x0059e4b0, 0x17, STATIC_DTOR, spellDescriptions)
VA_COMPGEN(0x0059e4d0, 0x14, STATIC_DTOR, abbreviatedSpellNames)
VA_COMPGEN(0x0059e4f0, 0x14, STATIC_DTOR, spellNames)
