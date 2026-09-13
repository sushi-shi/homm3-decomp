// text.h - prototypes of text.cpp (compiland text.obj)
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

// --- globals ---
// CODEVIEW(E:\gamedcs\text.cpp:49, dc 0x160ff4) void CheckTextResource();
// CODEVIEW(E:\gamedcs\text.cpp:86, dc 0x160ff8) void CheckSpreadsheetResource();

// --- TSpreadsheetResource ---
// CODEVIEW(E:\gamedcs\TextResource.h:113, dc 0x162910) int TSpreadsheetResource::GetNumberOfColumns(int r);
// CODEVIEW(E:\gamedcs\TextResource.h:120, dc 0x162934) const char* TSpreadsheetResource::GetSpreadsheet(int r, int c);

// --- std ---
// CODEVIEW(..\stlport\stl_vector.h:195, dc 0x16295c) unsigned std::vector<char *,std::allocator<char *> >::size();
// CODEVIEW(..\stlport\stl_vector.h:203, dc 0x162968) char** std::vector<char *,std::allocator<char *> >::operator[](unsigned __n);
// CODEVIEW(..\stlport\stl_vector.h:179, dc 0x162988) char** std::vector<char *,std::allocator<char *> >::begin();

#endif  /* HOMM3_TEXT_H */
