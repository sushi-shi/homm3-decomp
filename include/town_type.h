#ifndef HOMM3_TOWN_TYPE_H
#define HOMM3_TOWN_TYPE_H

// TTownType of the RoE source (Dreamcast CodeView enumerators, which the
// Loki port shares): eight towns; eTownNeutral marks the unaligned rows.
enum TTownType {
    eTownNeutral = -1,
    eTownCastle = 0,
    eTownRampart = 1,
    eTownTower = 2,
    eTownInferno = 3,
    eTownNecropolis = 4,
    eTownDungeon = 5,
    eTownStronghold = 6,
    eTownFortress = 7,
    kNumTownTypes = 8,
};

#endif
