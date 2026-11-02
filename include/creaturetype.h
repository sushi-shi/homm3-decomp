#ifndef HOMM3_CREATURETYPE_H
#define HOMM3_CREATURETYPE_H

#include "va.h"

#include <assert.h>

#include "armygrp.h"
#include "creaturetype_fwd.h"
#include "town.h"

// Complete Mac expands this four-valued town classifier in armyGroup's
// morale/luck getters and descriptions and army::setMorale/setLuck. Examples:
// 0x58c24..58c68, 0x59fa0..59fe4, 0x49c60..49ca4. Cross-TU Windows
// expansions support a shared header body. The inline qualifier is inferred:
// CodeWarrior retains the ordinary definition but expands the inline spelling
// at the native sites; VC6 emits the same callers for both. Name is inferred.
// Creature callers preserve the canonical game::getAlignment lookup before it.
enum {
    moralAlignmentNone = 0,
    moralAlignmentGood = 1,
    moralAlignmentNeutral = 2,
    moralAlignmentEvil = 3
};

inline int classifyTownAlignment(int townType)
{
    switch (townType) {
    case TOWN_CASTLE:
    case TOWN_RAMPART:
    case TOWN_TOWER:
        return moralAlignmentGood;
    case TOWN_INFERNO:
    case TOWN_NECROPOLIS:
    case TOWN_DUNGEON:
        return moralAlignmentEvil;
    case TOWN_STRONGHOLD:
    case TOWN_FORTRESS:
    case TOWN_CONFLUX:
        return moralAlignmentNeutral;
    default:
        return moralAlignmentNone;
    }
}

// E:\gamedcs\CreatureType.h:296. Complete retains the army.obj copy;
// events.cpp also inlines it at monsters_flee/join/sell_out with a literal
// count. DC 299..307 keeps the else arm (row 303 is its own jump). Loki's
// range assert (DC leaves 297..298 unrecorded) is not ported
// (docs/vc6/inliner.md).
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
// CTownDlg::CreateWin calls are named at DC lines 326/342; inlined across
// TUs, so a shared header body/inline linkage are inferred for Complete,
// not proven by the older DC declaration.
// The older DC body returns zero outside 0..6. Complete's popup expansions
// have no such fallback; their two loops supply valid dwelling indices.
// Complete's behavior for an invalid dwelling index remains unproven.
DC_ADDRESS(0x0718dc, 0x20)
inline TCreatureType GetBaseCreature(TTownType townType, int baseCreatureNbr)
{
    assert(townType != eTownNeutral);
    assert(baseCreatureNbr >= 0 && baseCreatureNbr < TOWN_DWELLING_COUNT);
    return g_dwellingType[townType][baseCreatureNbr];
}

#endif  /* HOMM3_CREATURETYPE_H */
