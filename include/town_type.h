#ifndef HOMM3_TOWN_TYPE_H
#define HOMM3_TOWN_TYPE_H

// Town/faction ids - the domain of town::type and of the creature
// traits' townType column (armygrp's alignment switches case on it:
// good 0-2 / evil 3-5 / neutral 6-8). NH3API terrain.hpp TTownType
// values with its TOWN_* alias spellings; the DC TTownType enum
// corroborates 0..7 (Conflux is Complete-era, absent from the AB-age
// DC roster).
enum TTownType {
    // Dreamcast NB11 type 0x1ab5: the unassigned/neutral town sentinel.
    eTownNeutral = -1,
    TOWN_CASTLE = 0x0,
    TOWN_RAMPART = 0x1,
    TOWN_TOWER = 0x2,
    TOWN_INFERNO = 0x3,
    TOWN_NECROPOLIS = 0x4,
    TOWN_DUNGEON = 0x5,
    TOWN_STRONGHOLD = 0x6,
    TOWN_FORTRESS = 0x7,
    TOWN_CONFLUX = 0x8
};

enum ETownTypeCount {
    // Complete has nine town types; the requirement-mask walks cover rows 0..8.
    TOWN_TYPE_COUNT = TOWN_CONFLUX + 1,
    // Restoration of Erathia has the first eight; Conflux came later.
    TOWN_TYPE_ROE_COUNT = TOWN_CONFLUX
};

#endif
