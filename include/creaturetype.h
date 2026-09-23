#ifndef HOMM3_CREATURETYPE_H
#define HOMM3_CREATURETYPE_H

#include "armygrp.h"
#include "creaturetype_fwd.h"
#include "town.h"

// Complete extends the Dreamcast creature-name domain through id 0x96.
// GetArmyName's retail range guard proves the inclusive upper bound.
const int g_creatureTypeLast = 0x96;

#include "inline/creaturetype_get_army_name.inl"

#define isBaseElemental(type) \
    ((type) == CREATURE_AIR_ELEMENTAL || (type) == CREATURE_EARTH_ELEMENTAL \
        || (type) == CREATURE_FIRE_ELEMENTAL || (type) == CREATURE_WATER_ELEMENTAL)

TCreatureType getBaseCreature(TTownType townType, int baseCreatureNbr);

#endif  /* HOMM3_CREATURETYPE_H */
