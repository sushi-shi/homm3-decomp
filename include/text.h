// text.h - text.cpp (compiland text.obj)
#ifndef HOMM3_TEXT_H
#define HOMM3_TEXT_H

// DC public gSpecialBuildingNames has eleven text columns per row. Retail
// InitializeSpecialBuildingText writes 110 pointers at 0x6a53d4 from
// bldgspec.txt, proving ten rows; market entry points consume the
// faction-specific building-name column when no visiting hero is present.
extern const char* g_specialBuildingNames[10][11];

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
