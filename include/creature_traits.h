#ifndef HOMM3_CREATURE_TRAITS_H
#define HOMM3_CREATURE_TRAITS_H

// PROVEN layout (2026-08-04): stride 116 from the retail index math
// (idx*29 dwords in HasAllUndead/get_AI_value), attributes @0x10 and
// AI_value @0x40 from the same bodies; field names are the NH3API
// roster, which lands exactly on those offsets with cost[7].
struct TCreatureTypeTraits {
    int m_townType;
    int m_level;
    const char* m_samplePrefix;
    const char* m_spriteName;
    unsigned int m_attributes;
    const char* m_name;
    const char* m_pluralName;
    const char* m_specialAbility;
    int m_cost[7];
    int m_baseFightValue;
    int m_aiValue;
    int m_growthRate;
    // SIXTEEN BITS, not 32: the crtraits.txt parser (0x47b480) stores this
    // column's atoi result with `mov word ptr [esi+0x48], ax` where every
    // neighbouring column takes a dword.
    short m_hordeGrowthRate;
    // Two alignment bytes before hitPoints at +0x4c. Dreamcast declares
    // horde_growth_rate as a short; retail parser 0x47b480 writes a word at +0x48.
    // NH3API widens that field, so its int32 facade is not used here.
    char m_paddingAfterHordeGrowth[2];
    int m_hitPoints;
    int m_speed;
    int m_attackSkill;
    int m_defenseSkill;
    int m_damageLowBound;
    int m_damageHighBound;
    int m_numShots;
    int m_hasSpell;
    int m_wanderingLow;
    int m_wanderingHigh;
};

#endif
