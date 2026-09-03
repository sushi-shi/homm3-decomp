// customcampaign_legacy.h - fixed pre-v28 campaign save records.
#ifndef HOMM3_CUSTOMCAMPAIGN_LEGACY_H
#define HOMM3_CUSTOMCAMPAIGN_LEGACY_H

#include "hero.h"

// Complete constructs sixteen 0x462-byte heroes inside the 0x66a9-byte
// legacy campaign record before reading it wholesale. Retail then promotes
// only the fields shared with the current 0x492-byte hero representation.
#pragma pack(push, 1)
struct LegacyCampaignHero : public type_obscuring_object {
    LegacyCampaignHero();

    short mana;                              // +0x018
    int id;                                  // +0x01a
    signed char owner;                       // +0x01e
    char name[13];                           // +0x01f
    int heroClass;                           // +0x02c
    unsigned char portrait;                  // +0x030
    char field_031[0x0a];
    short lastMagicSchoolLevel;              // +0x03b
    char field_03d[0x10];
    int experience;                          // +0x04d
    short level;                             // +0x051
    char field_053[0x38];
    unsigned char levelSeed;                 // +0x08b
    unsigned char lastWisdom;                // +0x08c
    armyGroup army;                          // +0x08d
    signed char skillLevel[28];              // +0x0c5
    unsigned char skillOrder[28];            // +0x0e1
    int skillCount;                          // +0x0fd
    char field_101[0x20];
    std::bitset<48> townSpecialGrantedMask;  // +0x121
    type_artifact equipped[18];              // +0x129
    type_artifact backpack[64];              // +0x1b9
    signed char backpackCount;               // +0x3b9
    unsigned char inSpellbook[70];           // +0x3ba
    unsigned char availableSpells[70];       // +0x400
    signed char stats[4];                    // +0x446
    char field_44a[0x18];
};
SIZE(LegacyCampaignHero, 0x462);

struct LegacyCampaignSave {
    signed char currentMap;                   // +0x0000
    signed char briefingChoice;               // +0x0001
    unsigned char isCheater;                  // +0x0002
    char field_0003[2];
    int numScenarios;                         // +0x0005
    char campaignFilename[101];               // +0x0009
    unsigned char scenarioCompleted[8][32];   // +0x006e
    short scenarioDays[8][32];                // +0x016e
    short scenarioScores[8][32];              // +0x036e
    unsigned char campaignCompleted[7];       // +0x056e
    char field_0575;
    int currentCampaign;                      // +0x0576
    char field_057a[0x1b05];
    LegacyCampaignHero carryOverHeroes[2][8]; // +0x207f
    signed char carryOverHeroCounts[2];       // +0x669f
    char field_66a1[8];
};
SIZE(LegacyCampaignSave, 0x66a9);
#pragma pack(pop)

#endif  // HOMM3_CUSTOMCAMPAIGN_LEGACY_H
