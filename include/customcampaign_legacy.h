// customcampaign_legacy.h - fixed pre-v28 campaign save records.
#ifndef HOMM3_CUSTOMCAMPAIGN_LEGACY_H
#define HOMM3_CUSTOMCAMPAIGN_LEGACY_H

#include "hero.h"

// Complete constructs sixteen 0x462-byte heroes inside the 0x66a9-byte
// legacy campaign record before reading it wholesale. Retail then promotes
// only the fields shared with the current 0x492-byte hero representation.
// Dreamcast hero's complete field order, with one-byte packing after the
// proven 0x18-byte base, reproduces every retail conversion offset and the
// 0x462 stride exactly. The gaps below therefore retain the old fields,
// including identifyLevel, which the current hero stores elsewhere.
#pragma pack(push, 1)
struct LegacyCampaignHero : public type_obscuring_object {
    short m_mana;                              // +0x018
    int m_id;                                  // +0x01a
    signed char m_owner;                       // +0x01e
    char m_name[13];                           // +0x01f
    int m_heroClass;                           // +0x02c
    unsigned char m_portrait;                  // +0x030
    int m_targetX; // +0x031
    int m_targetY; // +0x035
    short m_targetZ; // +0x039
    short m_lastMagicSchoolLevel;              // +0x03b
    unsigned short m_targetDistance; // +0x03d
    unsigned char m_targetIsCritical; // +0x03f
    unsigned char m_patrolX; // +0x040
    unsigned char m_patrolY; // +0x041
    signed char m_patrolRadius; // +0x042
    unsigned char m_facing; // +0x043
    unsigned char m_formation; // +0x044
    int m_maxMobility; // +0x045
    int m_currMobility; // +0x049
    int m_experience;                          // +0x04d
    short m_level;                             // +0x051
    unsigned long m_trainingGroundFlags; // +0x053
    unsigned long m_defenseTowerFlags; // +0x057
    unsigned long m_gardenOfRevelationFlags; // +0x05b
    unsigned long m_mercCampFlags; // +0x05f
    unsigned long m_powerSchoolFlags; // +0x063
    unsigned long m_treeOfKnowledgeFlags; // +0x067
    unsigned long m_libraryFlags; // +0x06b
    unsigned long m_arenaFlags; // +0x06f
    unsigned long m_magicSchoolFlags; // +0x073
    unsigned long m_warSchoolFlags; // +0x077
    unsigned long m_universityFlags; // +0x07b
    unsigned long m_shrine1Flags; // +0x07f
    unsigned long m_shrine2Flags; // +0x083
    unsigned long m_shrine3Flags; // +0x087
    unsigned char m_levelSeed;                 // +0x08b
    unsigned char m_lastWisdom;                // +0x08c
    ArmyGroup m_army;                          // +0x08d
    signed char m_skillLevel[28];              // +0x0c5
    unsigned char m_skillOrder[28];            // +0x0e1
    int m_skillCount;                          // +0x0fd
    unsigned long m_flags; // +0x101
    float m_turnExperienceToRvRatio; // +0x105
    signed char m_dWalkSpellsCast; // +0x109
    SkillMastery m_disguiseLevel; // +0x10a
    SkillMastery m_flightLevel; // +0x10e
    SkillMastery m_waterWalkLevel; // +0x112
    SkillMastery m_identifyLevel; // +0x116
    signed char m_moraleBonus; // +0x11a
    signed char m_luckBonus; // +0x11b
    unsigned char m_isSleeping; // +0x11c
    long m_bounty; // +0x11d
    std::bitset<48> m_townSpecialGrantedMask;  // +0x121
    type_artifact m_equipped[18];              // +0x129
    type_artifact m_backpack[64];              // +0x1b9
    signed char m_backpackCount;               // +0x3b9
    unsigned char m_inSpellbook[70];           // +0x3ba
    unsigned char m_availableSpells[70];       // +0x400
    signed char m_stats[4];                    // +0x446
    float m_aggression; // +0x44a
    long m_valueOfPower; // +0x44e
    long m_valueOfDuration; // +0x452
    long m_valueOfKnowledge; // +0x456
    long m_valueOfSpring; // +0x45a
    long m_valueOfWell; // +0x45e
};
SIZE(LegacyCampaignHero, 0x462);

// Dreamcast TCarryOverPoolNumber, values e_pool_1/e_pool_2/e_pool_choice/e_pool_both.
// Before normalization (type): TCarryOverPoolNumber.
#ifndef CarryOverPoolNumber
#define CarryOverPoolNumber TCarryOverPoolNumber
#endif
enum CarryOverPoolNumber {
    ePool1 = 0,
    ePool2 = 1,
    ePoolChoice = 2,
    ePoolBoth = 3
};

// Packed saved counterpart of Dreamcast TArtifactRequirement. These old
// data records are read wholesale; no current runtime constructor is implied.
struct LegacyCampaignArtifactRequirement {
    Artifact m_artifact;
    signed char m_guardBit;
};
SIZE(LegacyCampaignArtifactRequirement, 5);

// Dreamcast TCustomCampaignTraits, packed to 27 bytes in the old PC file.
// Eight-by-32 records plus the five-byte carryover_artifact fill exactly
// +0x057a..+0x207e before the retail-proven carryover hero array.
struct LegacyCampaignMapTraits {
    int m_expCap;
    signed char m_numIncomingHeroes;
    signed char m_numOutgoingHeroes;
    CarryOverPoolNumber m_incomingHeroPool;
    CarryOverPoolNumber m_outgoingHeroPool;
    LegacyCampaignArtifactRequirement m_artifactReq[2];
    signed char m_startingPosition[2];
    signed char m_difficulty;
};
SIZE(LegacyCampaignMapTraits, 27);

struct LegacyCampaignSave {
    signed char m_currentMap;                   // +0x0000
    // +0x0001 and +0x0576 are the CAMPAIGN ORDINAL and the BRIEFING CHOICE
    // in that order, not the reverse: retail's pre-v28 arm sign-extends the
    // byte at +0x0001 into SCampaign+0x04 (0x48a377/0x48a396) and loads the
    // dword at +0x0576 into SCampaign+0x10 (0x48a38a/0x48a393), and
    // SCampaign+0x04 is the subscript the same arm shifts by 5 to index
    // scenarioDays/scenarioScores/legacyCampaignScenarioIndices at
    // 0x48a465 - i.e. it is currentCampaign.
    signed char m_currentCampaign;              // +0x0001
    unsigned char m_isCheater;                  // +0x0002
    unsigned char m_secretActive;
    unsigned char m_customCampaign;
    int m_numScenarios;                         // +0x0005
    char m_campaignFilename[61];                // +0x0009
    unsigned char m_scenarioChoosable[8];        // +0x0046
    unsigned char m_mapChoosable[32];            // +0x004e
    // Retail SCampaign::Load copies this byte directly into the current
    // record's bool. An unsigned-char source makes VC6 insert test/setne.
    bool m_scenarioCompleted[8][32];            // +0x006e
    short m_scenarioDays[8][32];                // +0x016e
    short m_scenarioScores[8][32];              // +0x036e
    // Dreamcast bCampaignCompleted has eight slots; field_0575 was the last.
    // Retail promotion copies only the seven built-in campaign flags.
    unsigned char m_campaignCompleted[8];       // +0x056e
    int m_briefingChoice;                       // +0x0576
    LegacyCampaignMapTraits m_mapTraits[8][32]; // +0x057a
    LegacyCampaignArtifactRequirement m_carryoverArtifact; // +0x207a
    LegacyCampaignHero m_carryOverHeroes[2][8]; // +0x207f
    signed char m_carryOverHeroCounts[2];       // +0x669f
    signed char m_assignedCarryover[8];          // +0x66a1
};
SIZE(LegacyCampaignSave, 0x66a9);
#pragma pack(pop)

#endif  // HOMM3_CUSTOMCAMPAIGN_LEGACY_H
