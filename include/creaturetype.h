#ifndef HOMM3_CREATURETYPE_H
#define HOMM3_CREATURETYPE_H

#include "armygrp.h"
#include "creature_flags.h"
#include "town_type.h"

// CreatureType.h of the Loki port (RoE source). Field names are the
// Dreamcast CodeView ones; the layout is the 96-byte record CreatureType.cpp
// fills from crtraits.txt (costs, fight and AI values and the horde growth
// as shorts).

enum { kNumCreatureTypesPerTown = 7 };

struct TCreatureTypeTraits {
    TTownType townType;
    int level;
    const char* cSamplePrefix;
    const char* m_sprite_name;
    unsigned long attributes;
    const char* m_name;
    const char* m_plural_name;
    const char* special_ability;
    short cost[7];
    short baseFightValue;
    short AI_value;
    int growthRate;
    short horde_growth_rate;
    int hitPoints;
    int speed;
    int attackSkill;
    int defenseSkill;
    int damageLowBound;
    int damageHighBound;
    int numShots;
    int wanderingLow;
    int wanderingHigh;
};

// Loki exports the 4-byte reference cell (0x841a140) right after the anonymous
// array; Dreamcast types it as a reference to the const 122-row array
// (?akCreatureTypeTraits@@3AAY0HK@$$CBUTCreatureTypeTraits@@A).
extern const TCreatureTypeTraits (&akCreatureTypeTraits)[kNumCreatureAndSiegeWeaponTypes];

TCreatureType GetBaseCreature(TTownType townType, int baseCreatureNbr);
bool IsBaseCreature(TCreatureType type);
bool IsSiegeWeapon(TCreatureType type);
TCreatureType UpgradedCreatureType(TCreatureType type);
bool InitializeCreatureTypeTraitsTable();

#endif  /* HOMM3_CREATURETYPE_H */
