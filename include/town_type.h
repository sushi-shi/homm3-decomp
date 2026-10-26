#ifndef HOMM3_TOWN_TYPE_H
#define HOMM3_TOWN_TYPE_H

#include "domains.h"

// Town/faction ids - the domain of town::type and of the creature
// traits' townType column (armygrp's alignment switches case on it:
// good 0-2 / evil 3-5 / neutral 6-8). Dreamcast TTownType spellings
// (NB11 type 0x1ab5) for 0..7 and the neutral sentinel; Conflux is
// Complete-era, absent from the AB-age DC roster, and takes the same
// family's spelling.
H3_ENUM_BEGIN(TTownType)
    // The unassigned/neutral town sentinel.
    eTownNeutral = -1,
    eTownCastle = 0x0,
    eTownRampart = 0x1,
    eTownTower = 0x2,
    eTownInferno = 0x3,
    eTownNecropolis = 0x4,
    eTownDungeon = 0x5,
    eTownStronghold = 0x6,
    eTownFortress = 0x7,
    eTownConflux = 0x8,
    // Dreamcast TTownType::kNumTownTypes, after Complete's Conflux: the
    // nine rows of the per-town tables (initialize_game_data's mask walks
    // step 0x160-byte rows 0..8).
    kNumTownTypes
H3_ENUM_END(TTownType)
H3_ENUM_STEPPED(TTownType)

#endif
