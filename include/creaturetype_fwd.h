// Non-inline creature-type queries shared by the game interfaces.
// Keep these separate from getArmyName's inline definition: older consumers
// still have local name-selection functions with that spelling.
#ifndef HOMM3_CREATURETYPE_FWD_H
#define HOMM3_CREATURETYPE_FWD_H

#include "armygrp.h"

// Complete extends the Dreamcast creature-name domain through id 0x96.
// GetArmyName's retail range guard proves the inclusive upper bound.
const int g_creatureTypeLast = 0x96;

int isBaseCreature(TCreatureType monType);
unsigned char isSiegeWeapon(TCreatureType creature);
TCreatureType upgradedCreatureType(TCreatureType type);
TCreatureType downgradedCreatureType(TCreatureType type);

#endif  /* HOMM3_CREATURETYPE_FWD_H */
