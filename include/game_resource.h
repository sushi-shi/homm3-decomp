#ifndef HOMM3_GAME_RESOURCE_H
#define HOMM3_GAME_RESOURCE_H

// DC LF_ENUM `EGameResource` (T_INT4, 37 enumerators; the tail past
// GOLD is the reward/dialog domain, not a resource column). The seven
// primary resources are the width of every cost row - GOLD = 6 is the
// column get_gold_income reads out of the silo table (0x688ecc =
// 0x688eb4 + 6*4).
// NUM_RESOURCES is a source constant, not an EGameResource member: the
// complete Dreamcast CodeView enum does not contain it.
#define NUM_RESOURCES 7
enum EGameResource {
    const_no_resource = -1,
    WOOD = 0,
    MERCURY = 1,
    ORE = 2,
    SULFUR = 3,
    CRYSTAL = 4,
    GEMS = 5,
    GOLD = 6,
    ABANDONED = 7,
    RES_ARTIFACT = 8,
    RES_SPELL = 9,
    RES_COLOR = 10,
    RES_GOOD_LUCK = 11,
    RES_NEUTRAL_LUCK = 12,
    RES_BAD_LUCK = 13,
    RES_GOOD_MORALE = 14,
    RES_NEUTRAL_MORALE = 15,
    RES_BAD_MORALE = 16,
    RES_EXPERIENCE = 17,
    RES_HERO = 18,
    RES_ARTIFACT_W_TEXT = 19,
    RES_SECONDARY_SKILL = 20,
    RES_MONSTER = 21,
    RES_BUILDING_TT_0 = 22,
    RES_BUILDING_TT_1 = 23,
    RES_BUILDING_TT_2 = 24,
    RES_BUILDING_TT_3 = 25,
    RES_BUILDING_TT_4 = 26,
    RES_BUILDING_TT_5 = 27,
    RES_BUILDING_TT_6 = 28,
    RES_BUILDING_TT_7 = 29,
    // Complete inserts Conflux ahead of the four primary-skill icons.
    // Retail type_dialog_icon::set proves this shifted tail through its
    // nine-town jump-table arm and frame = resource - 31 lowering.
    RES_BUILDING_TT_8 = 30,
    RES_PRIMARY_SKILL_ATTACK = 31,
    RES_PRIMARY_SKILL_DEFENSE = 32,
    RES_PRIMARY_SKILL_POWER = 33,
    RES_PRIMARY_SKILL_KNOWLEDGE = 34,
    RES_MANA = 35,
    RES_SMALL_GOLD = 36
};

#endif
