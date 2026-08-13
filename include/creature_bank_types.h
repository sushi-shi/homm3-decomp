// creature_bank_types.h - shared retail creature-bank record layout.
#ifndef HOMM3_CREATURE_BANK_TYPES_H
#define HOMM3_CREATURE_BANK_TYPES_H

#include <va.h>
#include "armygrp.h"

// Retail indexes these records with a 108-byte stride. The first 56 bytes are
// the independently proven armyGroup; the reward tail remains opaque until a
// consuming function names it.
struct type_creature_bank {
    armyGroup guards;
    char pad_038[0x34];
};
SIZE(type_creature_bank, 0x6c);

#endif  // HOMM3_CREATURE_BANK_TYPES_H
