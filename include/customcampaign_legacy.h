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
// including identifyLevel, which the current hero no longer stores here.
// Original spellings below come from the complete Dreamcast hero field list.
#pragma pack(push, 1)
struct LegacyCampaignHero : public type_obscuring_object {
    // Before normalization: mana.
    short m_mana;                              // +0x018
    // Before normalization: id.
    int m_id;                                  // +0x01a
    // Before normalization: owner.
    signed char m_owner;                       // +0x01e
    // Before normalization: name.
    char m_name[13];                           // +0x01f
    // Before normalization: heroClass.
    int m_heroClass;                           // +0x02c
    // Before normalization: portrait.
    unsigned char m_portrait;                  // +0x030
    // Previously field_031; packed Dreamcast member sequence.
    // Original: targetX.
    int m_targetX; // +0x031
    // Original: targetY.
    int m_targetY; // +0x035
    // Original: targetZ.
    short m_targetZ; // +0x039
    // Before normalization: lastMagicSchoolLevel.
    short m_lastMagicSchoolLevel;              // +0x03b
    // Previously field_03d; packed Dreamcast member sequence.
    // Original: target_distance.
    unsigned short m_targetDistance; // +0x03d
    // Original: target_is_critical.
    unsigned char m_targetIsCritical; // +0x03f
    // Original: patrolX.
    unsigned char m_patrolX; // +0x040
    // Original: patrolY.
    unsigned char m_patrolY; // +0x041
    // Original: patrolRadius.
    signed char m_patrolRadius; // +0x042
    // Original: facing.
    unsigned char m_facing; // +0x043
    // Original: formation.
    unsigned char m_formation; // +0x044
    // Original: maxMobility.
    int m_maxMobility; // +0x045
    // Original: currMobility.
    int m_currMobility; // +0x049
    // Before normalization: experience.
    int m_experience;                          // +0x04d
    // Before normalization: level.
    short m_level;                             // +0x051
    // Previously field_053; packed Dreamcast member sequence.
    // Original: TrainingGroundFlags.
    unsigned long m_trainingGroundFlags; // +0x053
    // Original: DefenseTowerFlags.
    unsigned long m_defenseTowerFlags; // +0x057
    // Original: GardenOfRevelationFlags.
    unsigned long m_gardenOfRevelationFlags; // +0x05b
    // Original: MercCampFlags.
    unsigned long m_mercCampFlags; // +0x05f
    // Original: PowerSchoolFlags.
    unsigned long m_powerSchoolFlags; // +0x063
    // Original: TreeOfKnowledgeFlags.
    unsigned long m_treeOfKnowledgeFlags; // +0x067
    // Original: LibraryFlags.
    unsigned long m_libraryFlags; // +0x06b
    // Original: ArenaFlags.
    unsigned long m_arenaFlags; // +0x06f
    // Original: MagicSchoolFlags.
    unsigned long m_magicSchoolFlags; // +0x073
    // Original: WarSchoolFlags.
    unsigned long m_warSchoolFlags; // +0x077
    // Original: UniversityFlags.
    unsigned long m_universityFlags; // +0x07b
    // Original: Shrine1Flags.
    unsigned long m_shrine1Flags; // +0x07f
    // Original: Shrine2Flags.
    unsigned long m_shrine2Flags; // +0x083
    // Original: Shrine3Flags.
    unsigned long m_shrine3Flags; // +0x087
    // Before normalization: levelSeed.
    unsigned char m_levelSeed;                 // +0x08b
    // Before normalization: lastWisdom.
    unsigned char m_lastWisdom;                // +0x08c
    // Before normalization: army.
    armyGroup m_army;                          // +0x08d
    // Before normalization: skillLevel.
    signed char m_skillLevel[28];              // +0x0c5
    // Before normalization: skillOrder.
    unsigned char m_skillOrder[28];            // +0x0e1
    // Before normalization: skillCount.
    int m_skillCount;                          // +0x0fd
    // Previously field_101; packed Dreamcast member sequence.
    // Original: flags.
    unsigned long m_flags; // +0x101
    // Original: turnExperienceToRVRatio.
    float m_turnExperienceToRvRatio; // +0x105
    // Original: dWalkSpellsCast.
    signed char m_dWalkSpellsCast; // +0x109
    // Original: disguiseLevel.
    TSkillMastery m_disguiseLevel; // +0x10a
    // Original: flightLevel.
    TSkillMastery m_flightLevel; // +0x10e
    // Original: waterWalkLevel.
    TSkillMastery m_waterWalkLevel; // +0x112
    // Original: identifyLevel.
    TSkillMastery m_identifyLevel; // +0x116
    // Original: moraleBonus.
    signed char m_moraleBonus; // +0x11a
    // Original: luckBonus.
    signed char m_luckBonus; // +0x11b
    // Original: IsSleeping.
    unsigned char m_isSleeping; // +0x11c
    // Original: bounty.
    long m_bounty; // +0x11d
    // Before normalization: townSpecialGrantedMask.
    std::bitset<48> m_townSpecialGrantedMask;  // +0x121
    // Before normalization: equipped.
    type_artifact m_equipped[18];              // +0x129
    // Before normalization: backpack.
    type_artifact m_backpack[64];              // +0x1b9
    // Before normalization: backpackCount.
    signed char m_backpackCount;               // +0x3b9
    // Before normalization: inSpellbook.
    unsigned char m_inSpellbook[70];           // +0x3ba
    // Before normalization: availableSpells.
    unsigned char m_availableSpells[70];       // +0x400
    // Before normalization: stats.
    signed char m_stats[4];                    // +0x446
    // Previously field_44a; packed Dreamcast member sequence.
    // Original: aggression.
    float m_aggression; // +0x44a
    // Original: value_of_power.
    long m_valueOfPower; // +0x44e
    // Original: value_of_duration.
    long m_valueOfDuration; // +0x452
    // Original: value_of_knowledge.
    long m_valueOfKnowledge; // +0x456
    // Original: value_of_spring.
    long m_valueOfSpring; // +0x45a
    // Original: value_of_well.
    long m_valueOfWell; // +0x45e
};
SIZE(LegacyCampaignHero, 0x462);

// Dreamcast TCarryOverPoolNumber, values e_pool_1/e_pool_2/e_pool_choice/e_pool_both.
enum TCarryOverPoolNumber {
    ePool1 = 0,
    ePool2 = 1,
    ePoolChoice = 2,
    ePoolBoth = 3
};

// Packed saved counterpart of Dreamcast TArtifactRequirement. These old
// data records are read wholesale; no current runtime constructor is implied.
struct LegacyCampaignArtifactRequirement {
    // Original: artifact.
    TArtifact m_artifact;
    // Original: guard_bit.
    signed char m_guardBit;
};
SIZE(LegacyCampaignArtifactRequirement, 5);

// Dreamcast TCustomCampaignTraits, packed to 27 bytes in the old PC file.
// Eight-by-32 records plus the five-byte carryover_artifact fill exactly
// +0x057a..+0x207e before the retail-proven carryover hero array.
struct LegacyCampaignMapTraits {
    // Original: exp_cap.
    int m_expCap;
    // Original: num_incoming_heroes.
    signed char m_numIncomingHeroes;
    // Original: num_outgoing_heroes.
    signed char m_numOutgoingHeroes;
    // Original: incoming_hero_pool.
    TCarryOverPoolNumber m_incomingHeroPool;
    // Original: outgoing_hero_pool.
    TCarryOverPoolNumber m_outgoingHeroPool;
    // Original: artifact_req.
    LegacyCampaignArtifactRequirement m_artifactReq[2];
    // Original: starting_position.
    signed char m_startingPosition[2];
    // Original: difficulty.
    signed char m_difficulty;
};
SIZE(LegacyCampaignMapTraits, 27);

struct LegacyCampaignSave {
    // Before normalization: currentMap.
    signed char m_currentMap;                   // +0x0000
    // +0x0001 and +0x0576 are the CAMPAIGN ORDINAL and the BRIEFING CHOICE
    // in that order, not the reverse: retail's pre-v28 arm sign-extends the
    // byte at +0x0001 into SCampaign+0x04 (0x48a377/0x48a396) and loads the
    // dword at +0x0576 into SCampaign+0x10 (0x48a38a/0x48a393), and
    // SCampaign+0x04 is the subscript the same arm shifts by 5 to index
    // scenarioDays/scenarioScores/legacyCampaignScenarioIndices at
    // 0x48a465 - i.e. it is currentCampaign.
    // Before normalization: currentCampaign.
    signed char m_currentCampaign;              // +0x0001
    // Before normalization: isCheater.
    unsigned char m_isCheater;                  // +0x0002
    // Original: bSecretActive, bCustomCampaign; previously field_0003.
    unsigned char m_secretActive;
    unsigned char m_customCampaign;
    // Before normalization: numScenarios.
    int m_numScenarios;                         // +0x0005
    // Before normalization: campaignFilename.
    // Dreamcast CampaignFilename[61], bScenarioChoosable[8], bMapChoosable[32].
    // The former 101-byte array incorrectly combined all three declarations.
    char m_campaignFilename[61];                // +0x0009
    unsigned char m_scenarioChoosable[8];        // +0x0046
    unsigned char m_mapChoosable[32];            // +0x004e
    // Retail SCampaign::Load copies this byte directly into the current
    // record's bool. An unsigned-char source makes VC6 insert test/setne.
    // Before normalization: scenarioCompleted.
    bool m_scenarioCompleted[8][32];            // +0x006e
    // Before normalization: scenarioDays.
    short m_scenarioDays[8][32];                // +0x016e
    // Before normalization: scenarioScores.
    short m_scenarioScores[8][32];              // +0x036e
    // Before normalization: campaignCompleted.
    // Dreamcast bCampaignCompleted has eight slots; field_0575 was the last.
    // Retail promotion copies only the seven built-in campaign flags.
    unsigned char m_campaignCompleted[8];       // +0x056e
    // Before normalization: briefingChoice.
    int m_briefingChoice;                       // +0x0576
    // Original: map_traits and carryover_artifact; previously field_057a.
    LegacyCampaignMapTraits m_mapTraits[8][32]; // +0x057a
    LegacyCampaignArtifactRequirement m_carryoverArtifact; // +0x207a
    // Before normalization: carryOverHeroes.
    LegacyCampaignHero m_carryOverHeroes[2][8]; // +0x207f
    // Before normalization: carryOverHeroCounts.
    signed char m_carryOverHeroCounts[2];       // +0x669f
    // Original: assigned_carryover; previously field_66a1.
    signed char m_assignedCarryover[8];          // +0x66a1
};
SIZE(LegacyCampaignSave, 0x66a9);
#pragma pack(pop)

#endif  // HOMM3_CUSTOMCAMPAIGN_LEGACY_H
