#ifndef HOMM3_CREATURETYPE_H
#define HOMM3_CREATURETYPE_H

#include "armygrp.h"
#include "creaturetype_fwd.h"
#include "town.h"

#define isCreatureTypeInAcceptedRange(type)                                                         \
    ((type) >= CREATURE_ROSTER_BEGIN && (type) <= CREATURE_ACCEPTED_RANGE_MAX)

// E:\gamedcs\CreatureType.h:296. Complete retains the army.obj copy;
// events.cpp also expands this at monsters_flee/join/sell_out, passing a
// literal count so each singular/plural selection folds at its call site.
VA(0x00440100, 0x3E)  // two-register /Gr ABI + trait lookup, dc 0x1ef94
inline const char* getArmyName(
    H3_ENUM_PARAM(TCreatureType, int) type, int count)
{
    if (type < CREATURE_ROSTER_BEGIN || type > CREATURE_ACCEPTED_RANGE_MAX) {
        return DATA_COMPGEN(0x00691210, emptyCreatureName, "");
    } else {
        return count == 1 ? H3_AT(g_creatureTypeTraits, type).m_name
                          : H3_AT(g_creatureTypeTraits, type).m_pluralName;
    }
}

#define isBaseElemental(type) \
    ((type) == CREATURE_AIR_ELEMENTAL || (type) == CREATURE_EARTH_ELEMENTAL \
        || (type) == CREATURE_FIRE_ELEMENTAL || (type) == CREATURE_WATER_ELEMENTAL)

TCreatureType getBaseCreature(TTownType townType, int baseCreatureNbr);

#endif  /* HOMM3_CREATURETYPE_H */
