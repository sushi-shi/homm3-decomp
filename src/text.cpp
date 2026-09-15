// text.cpp - E:\gamedcs\text.cpp (compiland text.obj)
#include <ctype.h>

#include <va.h>
#include "exec.h"
#include "hiscore.h"
#include "resourcemanager.h"
#include "textresource.h"
#include "text.h"

// --- the resource holders and destination tables the loaders below
// write.  Every address is read off the retail loader that stores
// through it; every name is the Dreamcast text.obj global the same
// loader names (`homm3 dreamcast asm Initialize*`).  Element counts come
// from the RETAIL loop bounds, never from the Dreamcast build.

// A datum whose rva already carries a src DATA() claim in another TU is
// defined here WITHOUT one: the model allows a single claim per address
// and moving those claims would edit files this lane does not own.  The
// owning site is named in the comment above each such definition.

DATA(0x006a5328)
TextResource* g_resourceNamesResource;

// 0x006a5390 - datum claimed at include/game.h:2723 (gPrimarySkillNames)
const char* g_statNames[4];

DATA(0x006a53d0)
TextResource* g_mineNames;

DATA(0x006a53d4)
const char* g_specialBuildingNames[10][11];

DATA(0x006a57b8)
TextResource* g_heroBioText;

DATA(0x006a58b8)
const char* g_customCampRclick[67];

DATA(0x006a5c24)
SpreadsheetResource* g_campaignRegionNamesResource;

// 0x006a5c28 - datum claimed at src/townmgr.cpp:9175
const char* g_castleInfo[7];

DATA(0x006a5cc0)
const char* g_campaignDialog[24];

DATA(0x006a5d4c)
const char* g_abbSecondarySkillLevels[3];

DATA(0x006a5d5c)
TextResource* g_generalText;

// 0x006a5d84 - datum claimed at src/townmgr.cpp:421
const char* g_townCommand[35];

DATA(0x006a5e20)
const char* g_mineEventText[8];

// 0x006a5e40 - datum claimed at src/townmgr.cpp:385
const char* g_tavernInfo[8];

DATA(0x006a5e60)
SpreadsheetResource* g_campaignDialogResource;

// 0x006a5e64 - datum claimed at src/seerhut.cpp:290
const char* g_resourceNames[8];

DATA(0x006a5e84)
const char* g_terrainNames[10];

DATA(0x006a5eac)
TextResource* g_hallText;

// 0x006a5ecc - datum claimed at src/hiscore.cpp:34 (gHighScoreDefaults0)
char* g_highScoreCampaignDefault[11][4];

DATA(0x006a5f7c)
TextResource* g_mineEventTextResource;

DATA(0x006a6040)
SpreadsheetResource* g_specialBuildingText;

// 0x006a6048 - datum claimed at src/game.cpp:10974
const char* g_townNames[9][16];

DATA(0x006a6288)
SpreadsheetResource* g_dwellingText;

DATA(0x006a628c)
TextResource* g_tentColorText;

DATA(0x006a6290)
TextResource* g_townText;

DATA(0x006a62a8)
const char* g_dwellingNames[10][14];

DATA(0x006a64e4)
const char* g_neutralBuildingNames[19];

DATA(0x006a6568)
SpreadsheetResource* g_neutralBuildingText;

// 0x006a66d8 - datum claimed at src/hero.cpp:126 (gSharedHeroNames)
const char* g_heroBio[163];

DATA(0x006a69c4)
const char* g_dwellingInfo[10][14];

DATA(0x006a6bf4)
TextResource* g_primaryStatNames;

DATA(0x006a6c48)
TextResource* g_advObjNames;

DATA(0x006a6c78)
TextResource* g_customCampText;

// 0x006a7428 - datum claimed at src/castle.cpp:28
const char* g_hallInfo[10];

DATA(0x006a7450)
TextResource* g_creditsText;

DATA(0x006a74d0)
const char* g_mineDescriptions[8];

DATA(0x006a751c)
TextResource* g_playerColors;

DATA(0x006a7520)
const char* g_borderColorNames[8];

DATA(0x006a7550)
TextResource* g_ovText;

// 0x006a7570 - datum claimed at src/levelupwindow.cpp:46 (gSkillMasteryNames)
const char* g_secondarySkillLevels[3];

DATA(0x006a7700)
const char* g_credits[2];

DATA(0x006a7708)
TextResource* g_castleText;

DATA(0x006a770c)
TextResource* g_terrainNamesResource;

DATA(0x006a77c8)
TextResource* g_heroText;

DATA(0x006a77e8)
SpreadsheetResource* g_highScoreDefaults;

DATA(0x006a780c)
const char* g_buildingInfoSpecial[10][11];

DATA(0x006a79ec)
const char* g_quickViewText[232];

DATA(0x006a7d8c)
SpreadsheetResource* g_townNameText;

DATA(0x006a7d90)
TextResource* g_tavernText;

DATA(0x006a7df8)
const char* g_colors[8];

DATA(0x006a7e24)
const char* g_buildingInfoNeutral[28];

// 0x006a7ec0 - datum claimed at src/overview.cpp:71
const char* g_overviewText[16];

DATA(0x006a7f00)
TextResource* g_secondarySkillLevelNames;

// 0x006a7f08 - datum claimed at src/hiscore.cpp:35 (gHighScoreDefaults1)
char* g_highScoreStandardDefault[11][4];

DATA(0x006a7fb8)
const char* g_campaignRegionNames[23];

// 0x006a8014 - datum claimed at src/hero.cpp:2961 (gHeroScreenText0)
const char* g_heroScreen[33];
// --- Help.txt's 23 THelpText tables (below), Dreamcast-named ---

// 0x006a52d0 - datum claimed at src/spellbookwindow.cpp:52 (gSpellbookHelpText)
HelpText g_spellbookHelp[11];

// 0x006a53a8 - datum claimed at src/tradpost.cpp:1341 (gGiveHelpText)
HelpText g_giveResourceWindowHelp[5];

DATA(0x006a55a8)
HelpText g_combatOptionsHelp[39];

// 0x006a56e0 - datum claimed at src/adventuremapwindow.cpp:29
HelpText g_adventureWindowHelp[27];

// 0x006a5868 - datum claimed at src/tradpost.cpp:1342 (gMarketHelpText)
HelpText g_resourceWindowHelp[6];

DATA(0x006a59c8)
HelpText g_campaignBriefHelp[62];

DATA(0x006a5f80)
HelpText g_campaignWindowHelp[24];

// 0x006a6530 - datum claimed at src/adventureoptionswindow.cpp:20
HelpText g_adventureOptionsHelp[7];

DATA(0x006a6570)
HelpText g_multiSelectionHelp[25];

DATA(0x006a6638)
HelpText g_sacrificeWindowHelp2[20];

// 0x006a6968 - datum claimed at src/combatcontrolsubwindow.cpp:28 (gCombatSubWindowHelp)
HelpText g_combatWindowHelp[11];

DATA(0x006a6bf8)
HelpText g_newGameHelp[5];

DATA(0x006a6c20)
HelpText g_mainMenuHelp[5];

// 0x006a6c50 - datum claimed at src/tradpost.cpp:1343 (gSellArtHelpText)
HelpText g_sellArtifactWindowHelp[5];

DATA(0x006a6c80)
HelpText g_singleSelectionHelp[245];

// 0x006a7458 - datum claimed at src/viewarmywindow.cpp:156
HelpText g_viewArmyHelp[15];

DATA(0x006a7518)
SpreadsheetResource* g_helpText;

// 0x006a7558 - datum claimed at src/recruit.cpp:72 (gRecruitMaximumRolloverText)
HelpText g_recruitHelp[3];

DATA(0x006a7580)
HelpText g_systemOptionsHelp[48];

DATA(0x006a7750)
HelpText g_mpHelp[8];

DATA(0x006a77d0)
HelpText g_transformerWindowHelp[3];

// 0x006a7da8 - datum claimed at src/tradpost.cpp:1344 (gBuyArtHelpText)
HelpText g_buyArtifactWindowHelp[5];

// 0x006a7dd8 - datum claimed at src/university_window.cpp:33 (gUniversityWindowHelp)
HelpText g_universityWindowHelp2[4];

// 0x006a7e98 - datum claimed at src/tradpost.cpp:1345 (gSellCreaHelpText)
HelpText g_sellCreatureWindowHelp[5];
// --- Arraytxt.txt's 24 destination tables (below), Dreamcast-named ---

// 0x006a532c - datum claimed at src/viewarmywindow.cpp:168 (gLuckTexts)
const char* g_luckInfo[25];

DATA(0x006a558c)
const char* g_luckText[7];

// 0x006a57bc - datum claimed at src/viewarmywindow.cpp:167 (gMoraleTexts)
const char* g_moraleInfo[42];

DATA(0x006a5898)
const char* g_ownedByColor[8];

DATA(0x006a5bb8)
const char* g_armySizeNames[9][3];

// 0x006a5c48 - datum claimed at src/seerhut.cpp:296 (gQuestMonsterDirections)
const char* g_directions[9];

DATA(0x006a5c6c)
const char* g_speedNames[21];

DATA(0x006a5d24)
const char* g_rumourTerrainDescriptions[10];

// NOT gBorderGuardColors, however the Dreamcast public at dc 0x34f50
// reads: that name arrived here by FILL POSITION, and retail's own bytes
// refute it. The whole image references 0x6a5d60 exactly twice - the fill
// below and 0x469ecc, inside combatManager's moat worker at 0x469e50,
// which loads `[4*defendingTown->type + 0x6a5d60]` and hands it to
// damage_message as the message string. Nine entries, indexed by TOWN
// TYPE, consumed as a message: that is the moat attacker line per faction,
// and no border-guard consumer reads the array at all. Name is still an
// invention (cmbtmgr.h declares the same datum under it), but it is the
// role the retail bytes prove.
DATA(0x006a5d60)
const char* g_moatDamageMessages[9];

DATA(0x006a5e14)
const char* g_agrText[3];

DATA(0x006a5eb0)
const char* g_moraleText[7];

DATA(0x006a6044)
TextResource* g_arrayText;

DATA(0x006a6294)
const char* g_townSizeNames[4];

DATA(0x006a64d8)
const char* g_constWiseTreePriceText[3];

DATA(0x006a74f0)
const char* g_townTypeNames[10];

// 0x006a7540 - datum claimed at src/hero.cpp:121 (gStatDesc)
const char* g_statDesc[4];

// 0x006a7710 - datum claimed at src/game.cpp:446
const char* g_weekNames[15];

// 0x006a7794 - datum claimed at src/townmgr.cpp:483 (gPersonalityNames)
const char* g_personality[4];

// 0x006a77a8 - datum claimed at src/game.cpp:447 (gLastDayWarningFormat)
const char* g_newTurn[8];

DATA(0x006a77ec)
const char* g_difficulty[5];

DATA(0x006a7800)
const char* g_handiText[3];

// 0x006a79c4 - datum claimed at src/game.cpp:445
const char* g_monthNames[10];

DATA(0x006a7d94)
const char* g_mapSize[4];

DATA(0x006a7e18)
const char* g_humanCpu[3];

DATA(0x006a8098)
const char* g_newLoadSaveText[3];
#if 0  // @carcass

// E:\gamedcs\text.cpp:49
DC_ONLY(0x160ff4, 0x4)
void CheckTextResource()
{
    // @stub
}

// E:\gamedcs\text.cpp:86
DC_ONLY(0x160ff8, 0x4)
void CheckSpreadsheetResource()
{
    // @stub
}

#endif  // @carcass

VA(0x005b90f0, 0x19)  // dc 0x160ffc
unsigned char initializeGeneralText()
{
    g_generalText = ResourceManager::getText(
        DATA_COMPGEN(0x006885fc, generalTextName, "genrltxt.txt"));
    return g_generalText != 0;
}

VA(0x005b9110, 0x3d)  // dc 0x16101c
unsigned char initializeCustomCampaignText()
{
    g_customCampText = ResourceManager::getText(
        DATA_COMPGEN(0x0068860c, campaignButtonTextName, "campbttn.txt"));
    if (!g_customCampText)
        return 0;
    for (int i = 1; i <= 67; i++)
        g_customCampRclick[i - 1] = g_customCampText->getText(i);
    return 1;
}

VA(0x005b9150, 0x30)  // dc 0x161068
unsigned char initializeMineEventText()
{
    g_mineEventTextResource = ResourceManager::getText(
        DATA_COMPGEN(0x0068861c, mineEventTextName, "mineevnt.txt"));
    if (!g_mineEventTextResource)
        return 0;
    for (int i = 0; i < 8; i++)
        g_mineEventText[i] = g_mineEventTextResource->getText(i);
    return 1;
}

VA(0x005b9180, 0x43)  // dc 0x16110c
unsigned char initializeCampaignRegionNames()
{
    g_campaignRegionNamesResource = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x0068862c, campaignRegionsName, "regions.txt"));
    if (!g_campaignRegionNamesResource)
        return 0;
    for (int i = 0; i < 23; i++)
        g_campaignRegionNames[i] = g_campaignRegionNamesResource->getRow(i + 1)[0];
    return 1;
}

VA(0x005b91d0, 0xd0)  // dc 0x16115c
unsigned char initializeHighScoreDefaults()
{
    g_highScoreDefaults = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00688638, highScoreDefaultsName, "CampHigh.txt"));
    if (!g_highScoreDefaults)
        return 0;
    int i;
    for (i = 0; i < 11; i++) {
        g_highScoreStandardDefault[i][0] = const_cast<char*>(g_highScoreDefaults->getSpreadsheet(i + 1, 1));
        g_highScoreStandardDefault[i][1] = const_cast<char*>(g_highScoreDefaults->getSpreadsheet(i + 1, 2));
        g_highScoreStandardDefault[i][2] = const_cast<char*>(g_highScoreDefaults->getSpreadsheet(i + 1, 3));
        g_highScoreStandardDefault[i][3] = const_cast<char*>(g_highScoreDefaults->getSpreadsheet(i + 1, 4));
    }
    for (i = 0; i < 11; i++) {
        g_highScoreCampaignDefault[i][0] = const_cast<char*>(g_highScoreDefaults->getSpreadsheet(i + 13, 1));
        g_highScoreCampaignDefault[i][1] = const_cast<char*>(g_highScoreDefaults->getSpreadsheet(i + 13, 2));
        g_highScoreCampaignDefault[i][2] = const_cast<char*>(g_highScoreDefaults->getSpreadsheet(i + 13, 3));
        g_highScoreCampaignDefault[i][3] = const_cast<char*>(g_highScoreDefaults->getSpreadsheet(i + 13, 4));
    }
    g_highScoreManager->resetHighScores();
    return 1;
}

VA(0x005b92a0, 0x30)  // dc 0x161230
unsigned char initializeTerrainNames()
{
    g_terrainNamesResource = ResourceManager::getText(
        DATA_COMPGEN(0x00688648, terrainNamesName, "terrname.txt"));
    if (!g_terrainNamesResource)
        return 0;
    for (int i = 0; i < 10; i++)
        g_terrainNames[i] = g_terrainNamesResource->getText(i);
    return 1;
}

VA(0x005b92d0, 0x33)  // dc 0x16127c
unsigned char initializeAdvObjNames()
{
    g_advObjNames = ResourceManager::getText(
        DATA_COMPGEN(0x006604b4, advObjNamesName, "objnames.txt"));
    if (!g_advObjNames)
        return 0;
    for (int i = 0; i < 232; i++)
        g_quickViewText[i] = g_advObjNames->getText(i);
    return 1;
}

VA(0x005b9310, 0x30)  // dc 0x1612c8
unsigned char initializeResourceNames()
{
    g_resourceNamesResource = ResourceManager::getText(
        DATA_COMPGEN(0x00688658, resourceNamesName, "restypes.txt"));
    if (!g_resourceNamesResource)
        return 0;
    for (int i = 0; i < 8; i++)
        g_resourceNames[i] = g_resourceNamesResource->getText(i);
    return 1;
}

VA(0x005b9340, 0x30)  // dc 0x161314
unsigned char initializeMineNames()
{
    g_mineNames = ResourceManager::getText(
        DATA_COMPGEN(0x00688668, mineNamesName, "minename.txt"));
    if (!g_mineNames)
        return 0;
    for (int i = 0; i < 8; i++)
        g_mineDescriptions[i] = g_mineNames->getText(i);
    return 1;
}

VA(0x005b9370, 0x48)  // dc 0x161360
unsigned char initializePlayerColors()
{
    g_playerColors = ResourceManager::getText(
        DATA_COMPGEN(0x00688678, playerColorsName, "plcolors.txt"));
    if (!g_playerColors)
        return 0;
    for (int i = 0; i < 8; i++) {
        char* color = const_cast<char*>(g_playerColors->getText(i));
        *color = static_cast<char>(toupper(*color));
        g_colors[i] = color;
    }
    return 1;
}

VA(0x005b93c0, 0x30)  // dc 0x161428
unsigned char initializePrimaryStatNames()
{
    g_primaryStatNames = ResourceManager::getText(
        DATA_COMPGEN(0x00688688, primaryStatNamesName, "priskill.txt"));
    if (!g_primaryStatNames)
        return 0;
    for (int i = 0; i < 4; i++)
        g_statNames[i] = g_primaryStatNames->getText(i);
    return 1;
}

VA(0x005b93f0, 0x55)  // dc 0x161474
unsigned char initializeSecondarySkillLevelNames()
{
    g_secondarySkillLevelNames = ResourceManager::getText(
        DATA_COMPGEN(0x00688698, skillLevelNamesName, "skilllev.txt"));
    if (!g_secondarySkillLevelNames)
        return 0;
    for (int i = 0; i < 3; i++)
        g_secondarySkillLevels[i] = g_secondarySkillLevelNames->getText(i);
    for (int j = 0; j < 3; j++)
        g_abbSecondarySkillLevels[j] = g_secondarySkillLevelNames->getText(j + 3);
    return 1;
}

VA(0x005b9450, 0x8f)  // dc 0x1614e4
unsigned char initializeNeutralBuildingText()
{
    g_neutralBuildingText = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x006886a8, neutralBuildingTextName, "bldgneut.txt"));
    if (!g_neutralBuildingText)
        return 0;
    for (int i = 0; i < 19; i++) {
        if (static_cast<int>(g_neutralBuildingText->getRow(i).size()) > 1)
            g_neutralBuildingNames[i] = g_neutralBuildingText->getRow(i)[0];
        else
            g_neutralBuildingNames[i] = DATA_COMPGEN(0x00691210, textEmptyText, "");
    }
    for (int j = 0; j < 28; j++) {
        if (static_cast<int>(g_neutralBuildingText->getRow(j).size()) > 1)
            g_buildingInfoNeutral[j] = g_neutralBuildingText->getRow(j)[1];
        else
            g_buildingInfoNeutral[j] = DATA_COMPGEN(0x00691210, textEmptyText, "");
    }
    return 1;
}

VA(0x005b94e0, 0x8d)  // dc 0x16158c
unsigned char initializeSpecialBuildingText()
{
    g_specialBuildingText = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x006886b8, specialBuildingTextName, "bldgspec.txt"));
    if (!g_specialBuildingText)
        return 0;
    int n = 0;
    for (int faction = 0; faction < 10; faction++) {
        for (int slot = 0; slot < 11; slot++) {
            if (static_cast<int>(g_specialBuildingText->getRow(n).size()) > 1) {
                g_specialBuildingNames[faction][slot] =
                    g_specialBuildingText->getRow(n)[0];
                g_buildingInfoSpecial[faction][slot] =
                    g_specialBuildingText->getRow(n)[1];
            } else {
                g_specialBuildingNames[faction][slot] =
                    DATA_COMPGEN(0x00691210, textEmptyText, "");
                g_buildingInfoSpecial[faction][slot] =
                    DATA_COMPGEN(0x00691210, textEmptyText, "");
            }
            n++;
        }
    }
    return 1;
}

VA(0x005b9570, 0x90)  // dc 0x161698
unsigned char initializeDwellingText()
{
    g_dwellingText = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x006886c8, dwellingTextName, "dwelling.txt"));
    if (!g_dwellingText)
        return 0;
    int n = 0;
    for (int faction = 0; faction < 10; faction++) {
        for (int slot = 0; slot < 14; slot++) {
            if (static_cast<int>(g_dwellingText->getRow(n).size()) > 0) {
                g_dwellingNames[faction][slot] = g_dwellingText->getRow(n)[0];
                g_dwellingInfo[faction][slot] = g_dwellingText->getRow(n)[1];
            } else {
                g_dwellingNames[faction][slot] =
                    DATA_COMPGEN(0x00691210, textEmptyText, "");
                g_dwellingInfo[faction][slot] =
                    DATA_COMPGEN(0x00691210, textEmptyText, "");
            }
            n++;
        }
    }
    return 1;
}

VA(0x005b9600, 0x73)  // dc 0x161748
unsigned char initializeTownNameText()
{
    g_townNameText = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x006886d8, townNameTextName, "townname.txt"));
    if (!g_townNameText)
        return 0;
    int n = 0;
    for (int faction = 0; faction < 9; faction++) {
        for (int slot = 0; slot < 16; slot++) {
            if (static_cast<int>(g_townNameText->getRow(n).size()) > 0)
                g_townNames[faction][slot] = g_townNameText->getRow(n)[0];
            else
                g_townNames[faction][slot] =
                    DATA_COMPGEN(0x00691210, textEmptyText, "");
            n++;
        }
    }
    return 1;
}

VA(0x005b9680, 0x33)  // dc 0x1617d8
unsigned char initializeHeroBioText()
{
    g_heroBioText = ResourceManager::getText(
        DATA_COMPGEN(0x006886e8, heroBioTextName, "HeroBios.txt"));
    if (!g_heroBioText)
        return 0;
    for (int i = 0; i < 163; i++)
        g_heroBio[i] = g_heroBioText->getText(i);
    return 1;
}

VA(0x005b96c0, 0x30)  // dc 0x161824
unsigned char initializeCastleText()
{
    g_castleText = ResourceManager::getText(
        DATA_COMPGEN(0x006886f8, castleTextName, "CastInfo.txt"));
    if (!g_castleText)
        return 0;
    for (int i = 0; i < 7; i++)
        g_castleInfo[i] = g_castleText->getText(i);
    return 1;
}

VA(0x005b96f0, 0x30)  // dc 0x161870
unsigned char initializeTavernText()
{
    g_tavernText = ResourceManager::getText(
        DATA_COMPGEN(0x00688708, tavernTextName, "TvrnInfo.txt"));
    if (!g_tavernText)
        return 0;
    for (int i = 0; i < 8; i++)
        g_tavernInfo[i] = g_tavernText->getText(i);
    return 1;
}

VA(0x005b9720, 0x30)  // dc 0x1618bc
unsigned char initializeHallText()
{
    g_hallText = ResourceManager::getText(
        DATA_COMPGEN(0x00688718, hallTextName, "HallInfo.txt"));
    if (!g_hallText)
        return 0;
    for (int i = 0; i < 10; i++)
        g_hallInfo[i] = g_hallText->getText(i);
    return 1;
}

VA(0x005b9750, 0x33)  // dc 0x16196c
unsigned char initializeTownText()
{
    g_townText = ResourceManager::getText(
        DATA_COMPGEN(0x00688728, townTextName, "TCommand.txt"));
    if (!g_townText)
        return 0;
    for (int i = 0; i < 35; i++)
        g_townCommand[i] = g_townText->getText(i);
    return 1;
}

VA(0x005b9790, 0x30)  // dc 0x1619b8
unsigned char initializeOverviewText()
{
    g_ovText = ResourceManager::getText(
        DATA_COMPGEN(0x00688738, overviewTextName, "Overview.txt"));
    if (!g_ovText)
        return 0;
    for (int i = 0; i < 16; i++)
        g_overviewText[i] = g_ovText->getText(i);
    return 1;
}

VA(0x005b97c0, 0x33)  // dc 0x161a04
unsigned char initializeHeroText()
{
    g_heroText = ResourceManager::getText(
        DATA_COMPGEN(0x00688748, heroTextName, "HeroScrn.txt"));
    if (!g_heroText)
        return 0;
    for (int i = 0; i < 33; i++)
        g_heroScreen[i] = g_heroText->getText(i);
    return 1;
}

VA(0x005b9800, 0x36)  // dc 0x161a50
unsigned char initializeCampaignDialogText()
{
    g_campaignDialogResource = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00688758, campaignDialogName, "CampDiag.txt"));
    if (!g_campaignDialogResource)
        return 0;
    for (int i = 0; i < 24; i++)
        g_campaignDialog[i] = g_campaignDialogResource->getRow(i)[1];
    return 1;
}

VA(0x005b9840, 0x31)  // dc 0x161aa4
unsigned char initializeCreditsText()
{
    g_creditsText = ResourceManager::getText(
        DATA_COMPGEN(0x00688768, creditsTextName, "Credits.txt"));
    if (!g_creditsText)
        return 0;
    g_credits[0] = g_creditsText->getText(1);
    g_credits[1] = g_creditsText->getText(2);
    return 1;
}

VA(0x005b9880, 0x30)
unsigned char initializeTentColorText()
{
    g_tentColorText = ResourceManager::getText(
        DATA_COMPGEN(0x00688774, tentColorTextName, "TentColr.txt"));
    if (!g_tentColorText)
        return 0;
    for (int i = 0; i < 8; i++)
        g_borderColorNames[i] = g_tentColorText->getText(i);
    return 1;
}

// Read each window's run of rollover/right-click text pairs in file order,
// skipping two rows between runs.

VA(0x005b98b0, 0x405)  // dc 0x161ae4
unsigned char initializeHelpText()
{
    int i;
    unsigned int j;

    g_helpText = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00688784, helpTextName, "Help.txt"));
    if (!g_helpText)
        return 0;

    i = 3;
    for (j = 0; j < 5; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_mainMenuHelp[j].m_text = row[0];
        g_mainMenuHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 5; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_newGameHelp[j].m_text = row[0];
        g_newGameHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 245; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_singleSelectionHelp[j].m_text = row[0];
        g_singleSelectionHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 25; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_multiSelectionHelp[j].m_text = row[0];
        g_multiSelectionHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 27; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_adventureWindowHelp[j].m_text = row[0];
        g_adventureWindowHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 48; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_systemOptionsHelp[j].m_text = row[0];
        g_systemOptionsHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 7; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_adventureOptionsHelp[j].m_text = row[0];
        g_adventureOptionsHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 11; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_combatWindowHelp[j].m_text = row[0];
        g_combatWindowHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 39; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_combatOptionsHelp[j].m_text = row[0];
        g_combatOptionsHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 15; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_viewArmyHelp[j].m_text = row[0];
        g_viewArmyHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 11; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_spellbookHelp[j].m_text = row[0];
        g_spellbookHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 62; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_campaignBriefHelp[j].m_text = row[0];
        g_campaignBriefHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 24; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_campaignWindowHelp[j].m_text = row[0];
        g_campaignWindowHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 3; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_recruitHelp[j].m_text = row[0];
        g_recruitHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 8; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_mpHelp[j].m_text = row[0];
        g_mpHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 20; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_sacrificeWindowHelp2[j].m_text = row[0];
        g_sacrificeWindowHelp2[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 3; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_transformerWindowHelp[j].m_text = row[0];
        g_transformerWindowHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 6; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_resourceWindowHelp[j].m_text = row[0];
        g_resourceWindowHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 5; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_giveResourceWindowHelp[j].m_text = row[0];
        g_giveResourceWindowHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 5; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_buyArtifactWindowHelp[j].m_text = row[0];
        g_buyArtifactWindowHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 5; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_sellArtifactWindowHelp[j].m_text = row[0];
        g_sellArtifactWindowHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 5; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_sellCreatureWindowHelp[j].m_text = row[0];
        g_sellCreatureWindowHelp[j].m_rclick = row[1];
    }
    i += 2;
    for (j = 0; j < 4; j++, i++) {
        const SpreadsheetResource::TStringVector& row =
            g_helpText->getRow(i);

        g_universityWindowHelp2[j].m_text = row[0];
        g_universityWindowHelp2[j].m_rclick = row[1];
    }
    return 1;
}

VA(0x005b9cc0, 0x2BC)  // dc 0x162308
unsigned char initializeArrayText()
{
    int i;
    int j;

    g_arrayText = ResourceManager::getText(
        DATA_COMPGEN(0x00688790, arrayTextName, "Arraytxt.txt"));
    if (!g_arrayText)
        return 0;

    i = 2;
    for (j = 0; j < 4; j++, i++)
        g_statDesc[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 7; j++, i++)
        g_luckText[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 7; j++, i++)
        g_moraleText[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 8; j++, i++)
        g_ownedByColor[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 10; j++, i++)
        g_monthNames[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 15; j++, i++)
        g_weekNames[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 25; j++, i++)
        g_luckInfo[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 42; j++, i++)
        g_moraleInfo[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 8; j++, i++)
        g_newTurn[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 4; j++, i++)
        g_mapSize[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 5; j++, i++)
        g_difficulty[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 9; j++, i++)
        g_directions[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 10; j++, i++)
        g_rumourTerrainDescriptions[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 4; j++, i++)
        g_personality[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 9; j++) {
        int k;

        for (k = 0; k < 3; i++, k++)
            g_armySizeNames[j][k] = g_arrayText->getText(i);
    }
    i++;
    for (j = 0; j < 3; j++, i++)
        g_constWiseTreePriceText[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 3; j++, i++)
        g_humanCpu[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 3; j++, i++)
        g_handiText[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 3; j++, i++)
        g_agrText[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 10; j++, i++)
        g_townTypeNames[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 3; j++, i++)
        g_newLoadSaveText[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 21; j++, i++)
        g_speedNames[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 4; j++, i++)
        g_townSizeNames[j] = g_arrayText->getText(i);
    i++;
    for (j = 0; j < 9; j++, i++)
        g_moatDamageMessages[j] = g_arrayText->getText(i);
    return 1;
}

#if 0  // @carcass

// E:\gamedcs\TextResource.h:113
DC_ONLY(0x162910, 0x24)
int SpreadsheetResource::GetNumberOfColumns(int r)
{
    // @stub
}

// E:\gamedcs\TextResource.h:120
DC_ONLY(0x162934, 0x28)
const char* SpreadsheetResource::getSpreadsheet(int r, int c)
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x16295c, 0xC)
unsigned std::vector<char *,std::allocator<char *> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x162968, 0x20)
char** std::vector<char *,std::allocator<char *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x162988, 0x4)
char** std::vector<char *,std::allocator<char *> >::begin()
{
    // @stub
}

#endif  // @carcass
