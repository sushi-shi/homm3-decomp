#ifndef HOMM3_CREATURETYPE_H
#define HOMM3_CREATURETYPE_H

#include "armygrp.h"
#include "creaturetype_fwd.h"
#include "town.h"

// Complete extends the Dreamcast creature-name domain through id 0x96.
// GetArmyName's retail range guard proves the inclusive upper bound.
const int g_creatureTypeLast = 0x96;

// E:\gamedcs\CreatureType.h:296. Complete retains the army.obj copy;
// events.cpp also expands this at monsters_flee/join/sell_out, passing a
// literal count so each singular/plural selection folds at its call site.
VA(0x00440100, 0x3E)  // two-register /Gr ABI + trait lookup, dc 0x1ef94
inline const char* getArmyName(int type, int count)
{
    if (type < 0 || type > g_creatureTypeLast) {
        return DATA_COMPGEN(0x00691210, emptyCreatureName, "");
    }
    return count == 1 ? g_creatureTypeTraits[type].m_name
                      : g_creatureTypeTraits[type].m_pluralName;
}

// Native Mac chooseWeakestArmy0x30200 and valueOfAddingArmy0x30310
// subtract AIR and compare the unsigned offset with3. Keep this shared range
// test; four equality checks retain extra native branches under CodeWarrior.
#define isBaseElemental(type) \
    (static_cast<unsigned int>((type) - CREATURE_AIR_ELEMENTAL) \
        <= CREATURE_WATER_ELEMENTAL - CREATURE_AIR_ELEMENTAL)

TCreatureType getBaseCreature(TTownType townType, int baseCreatureNbr);

#endif  /* HOMM3_CREATURETYPE_H */
