#ifndef HOMM3_AI_H
#define HOMM3_AI_H

class army;
struct type_AI_combat_parameters;

// Creature identities use the canonical TCreatureType domain in armygrp.h.

// --- globals ---
long getAreaAttackValue(const army* currentArmy, long hex, long ourGroup,
                           type_AI_combat_parameters* data);

#endif  /* HOMM3_AI_H */
