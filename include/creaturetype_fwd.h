// Non-inline creature-type queries shared by the game interfaces.
// Keep these separate from getArmyName's inline definition: older consumers
// still have local name-selection functions with that spelling.
#ifndef HOMM3_CREATURETYPE_FWD_H
#define HOMM3_CREATURETYPE_FWD_H

#include "armygrp.h"

// Before normalization (function): IsBaseCreature.
int isBaseCreature(TCreatureType monType);
// Before normalization (function): IsSiegeWeapon.
unsigned char isSiegeWeapon(TCreatureType creature);
// Before normalization (function): UpgradedCreatureType.
TCreatureType upgradedCreatureType(TCreatureType type);
// Before normalization (function): DowngradedCreatureType.
TCreatureType downgradedCreatureType(TCreatureType type);

#endif  /* HOMM3_CREATURETYPE_FWD_H */
