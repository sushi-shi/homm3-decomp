// Shared creature-combo ordering and insertion for editor dialogs.
#ifndef HOMM3_EDITOR_CREATURECOMBO_H
#define HOMM3_EDITOR_CREATURECOMBO_H

#include "armygrp.h"

class CComboBox;

// Retail retains these calls across dialog translation units. Keep ordinary
// source bodies at their claimed retail owners.
// Army and creature-stack dialogs retain a separate ordering body.
bool isCreatureTypeBefore(TCreatureType lhs, TCreatureType rhs);

bool isCreatureBefore(TCreatureType lhs, TCreatureType rhs);
void insertCreature(CComboBox& combo, TCreatureType creature);

#endif  /* HOMM3_EDITOR_CREATURECOMBO_H */
