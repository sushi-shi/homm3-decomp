// Non-inline creature-type queries shared by the game interfaces.
// Keep these separate from getArmyName's inline definition: older consumers
// still have local name-selection functions with that spelling.
#ifndef HOMM3_CREATURETYPE_FWD_H
#define HOMM3_CREATURETYPE_FWD_H

#include "armygrp.h"

int isBaseCreature(TCreatureType monType);
unsigned char isSiegeWeapon(TCreatureType creature);
TCreatureType upgradedCreatureType(TCreatureType type);
TCreatureType downgradedCreatureType(TCreatureType type);

#endif  /* HOMM3_CREATURETYPE_FWD_H */
