#ifndef HOMM3_CREATURETYPE_H
#define HOMM3_CREATURETYPE_H

#include "va.h"

#include "armygrp.h"
#include "creaturetype_fwd.h"
#include "town.h"

// Complete extends the Dreamcast creature-name domain through id 0x96.
// GetArmyName's retail range guard proves the inclusive upper bound.
const int g_creatureTypeLast = 0x96;
// The 145 creature types before the war machines (CREATURE_CATAPULT is the
// first): the width of the AI's creature value tables and of the campaign
// crossover creature mask (NH3API MAX_CREATURES).
const int g_creatureTypeCount = CREATURE_CATAPULT;

// E:\gamedcs\CreatureType.h:296. Complete retains the army.obj copy;
// events.cpp also expands this at monsters_flee/join/sell_out, passing a
// literal count so each singular/plural selection folds at its call site.
// DC 299..307 keeps the else arm (row 303 is its own jump); the guard-return
// spelling costs /Ob2 more and pushed getArmyHelpText's appends out of line.
VA(0x00440100, 0x3E)  // two-register /Gr ABI + trait lookup
DC_ADDRESS(0x01ef94, 0x5c)
inline const char* getArmyName(int type, int count)
{
    if (type < 0 || type > g_creatureTypeLast) {
        return DATA_COMPGEN(0x00691210, emptyCreatureName, "");
    } else {
        return count == 1 ? akCreatureTypeTraits[type].m_name
                          : akCreatureTypeTraits[type].m_plural_name;
    }
}

// Windows combatMonsterEvent retains four equality arms for this predicate.
// Keep its canonical membership expression: the unsigned-range spelling
// loses the retail expansions in army alignment, morale, luck and terrain.
// Mac can lower equivalent membership tests differently; it is a reference.
#define isBaseElemental(type) \
    ((type) == CREATURE_AIR_ELEMENTAL \
        || (type) == CREATURE_EARTH_ELEMENTAL \
        || (type) == CREATURE_FIRE_ELEMENTAL \
        || (type) == CREATURE_WATER_ELEMENTAL)

// Original GetBaseCreature: creaturetype.cpp:202. The two
// CTownDlg::CreateWin calls are named at DC lines 326/342. Complete Mac
// expands the same row lookup at 0x16c924/0x16ca40, as does Windows; a
// shared header body/inline linkage are inferred for Complete's cross-TU
// expansion, not proven by the older DC declaration.
// The older DC body returns zero outside 0..6. Complete's popup expansions
// have no such fallback; their two loops supply valid dwelling indices.
// Complete's behavior for an invalid dwelling index remains unproven.
DC_ADDRESS(0x0718dc, 0x20)
inline TCreatureType getBaseCreature(TTownType townType, int baseCreatureNbr)
{
    return g_dwellingType[townType][baseCreatureNbr];
}

#endif  /* HOMM3_CREATURETYPE_H */
