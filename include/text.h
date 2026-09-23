#ifndef HOMM3_TEXT_H
#define HOMM3_TEXT_H

#include "window.h"

// Shared text storage filled by the resource loaders in text.cpp.

// Named rows from Credits.txt consumed by initializeCreditsText.
enum ECreditsTextIndex {
    CREDITS_TEXT_STAFF = 1,
    CREDITS_TEXT_LEGAL = 2
};

extern const char* g_abbSecondarySkillLevels[3];
extern const char* g_statNames[4];
extern const char* g_castleInfo[7];
extern const char* g_townCommand[35];
extern char* g_highScoreCampaignDefault[11][4];
extern const char* g_neutralBuildingNames[19];
extern const char* g_heroBio[163];
extern const char* g_secondarySkillLevels[3];
extern const char* g_quickViewText[232];
extern char* g_highScoreStandardDefault[11][4];
extern const char* g_heroScreen[33];
extern THelpText g_adventureWindowHelp[27];
extern THelpText g_combatWindowHelp[11];
extern THelpText g_recruitHelp[3];
extern const char* g_luckInfo[25];
extern const char* g_moraleInfo[42];
extern const char* g_ownedByColor[8];
extern const char* g_armySizeNames[9][3];
extern const char* g_directions[9];
extern const char* g_rumourTerrainDescriptions[10];
extern const char* g_constWiseTreePriceText[3];
extern const char* g_personality[4];
extern const char* g_newTurn[8];

// Other Arraytxt.txt and building-text destinations shared with their readers.
extern const char* g_agrText[3];
extern const char* g_difficulty[5];
extern const char* g_handiText[3];
extern const char* g_humanCpu[3];
extern const char* g_newLoadSaveText[3];
extern const char* g_colors[8];
extern const char* g_resourceNames[8];
extern const char* g_buildingInfoSpecial[10][11];
extern const char* g_buildingInfoNeutral[28];
extern const char* g_dwellingNames[10][14];
extern const char* g_dwellingInfo[10][14];

// DC public gSpecialBuildingNames has eleven text columns per row. Retail
// InitializeSpecialBuildingText writes 110 pointers at 0x6a53d4 from
// bldgspec.txt, proving ten rows; market entry points consume the
// faction-specific building-name column when no visiting hero is present.
extern const char* g_specialBuildingNames[10][11];

// DC gTownTypeNames; retail InitializeArrayText fills ten entries at
// 0x6a74f0. The faction-name subtable starts at element one (0x6a74f4).
extern const char* g_townTypeNames[10];

unsigned char initializeGeneralText();               // 0x5b90f0
unsigned char initializeCustomCampaignText();        // 0x5b9110
unsigned char initializeMineEventText();             // 0x5b9150
unsigned char initializeCampaignRegionNames();       // 0x5b9180
unsigned char initializeHighScoreDefaults();         // 0x5b91d0
unsigned char initializeTerrainNames();              // 0x5b92a0
unsigned char initializeAdvObjNames();               // 0x5b92d0
unsigned char initializeResourceNames();             // 0x5b9310
unsigned char initializeMineNames();                 // 0x5b9340
unsigned char initializePlayerColors();              // 0x5b9370
unsigned char initializePrimaryStatNames();          // 0x5b93c0
unsigned char initializeSecondarySkillLevelNames();  // 0x5b93f0
unsigned char initializeNeutralBuildingText();       // 0x5b9450
unsigned char initializeSpecialBuildingText();       // 0x5b94e0
unsigned char initializeDwellingText();              // 0x5b9570
unsigned char initializeTownNameText();              // 0x5b9600
unsigned char initializeHeroBioText();               // 0x5b9680
unsigned char initializeCastleText();                // 0x5b96c0
unsigned char initializeTavernText();                // 0x5b96f0
unsigned char initializeHallText();                  // 0x5b9720
unsigned char initializeTownText();                  // 0x5b9750
unsigned char initializeOverviewText();              // 0x5b9790
unsigned char initializeHeroText();                  // 0x5b97c0
unsigned char initializeCampaignDialogText();        // 0x5b9800
unsigned char initializeCreditsText();               // 0x5b9840
// Complete-only: tentcolr.txt, the border-guard tent colour names.
unsigned char initializeTentColorText();             // 0x5b9880
unsigned char initializeHelpText();                  // 0x5b98b0
unsigned char initializeArrayText();                 // 0x5b9cc0

#endif  /* HOMM3_TEXT_H */
