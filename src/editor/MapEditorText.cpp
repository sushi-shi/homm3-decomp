// MapEditorText.cpp - the editor's text strings (h3maped 0x462a63..0x4690fd;
// Loki h3maped object 18): editor.txt's general strings, then one group per
// dialog, page and menu, each after its heading line. SText::initialize
// copies every group into a buffer of its own once, spelling each ellipsis
// character out as three periods, and points the group's strings into it.
// Each reference's dynamic initializer stores the address of its group's
// array element, so the definitions follow every group's index order.
#include "editor/stdafx.h"

#include <assert.h>
#include <string.h>

#include "va.h"
#include "exceptions.h"
#include "ownership.h"
#include "resourcemanager.h"
#include "textresource.h"
#include "editor/MapEditorText.h"

namespace {

// The length of a text once each ellipsis is spelled out (Loki's
// stringLength).
VA(0x00462a7f, 0x36)
size_t stringLength(const char* str)
{
    size_t length = 0;
    for (;;) {
        size_t span = strcspn(str, "\x85");
        length += span;
        str += span;
        if (*str == '\0')
            break;
        assert(*str == '\x85');
        str++;
        length += 3;
    }
    return length;
}

// Copies a text, spelling each ellipsis out as three periods (Loki's
// stringCopy).
VA(0x00462ab5, 0x2f)
void stringCopy(char* dest, const char* src)
{
    while (*src != '\0') {
        if (*src == '\x85') {
            *dest++ = '.';
            *dest++ = '.';
            *dest++ = '.';
        } else
            *dest++ = *src;
        src++;
    }
    *dest = '\0';
}

DATA(0x0059ffb4) const char* akGeneralStringImp[kNumGeneralStrings];

}

DATA(0x005a0da0) const char* const& kAppTitleStr = akGeneralStringImp[0];
DATA(0x0059ff14) const char* const& kCopyrightStr = akGeneralStringImp[1];
DATA(0x005a05a8) const char* const& kVersionFmtStr = akGeneralStringImp[2];
DATA(0x005a1138) const char* const& kMapFileFilterStr = akGeneralStringImp[3];
DATA(0x005a03dc) const char* const& kMapFileRegNameStr = akGeneralStringImp[4];
DATA(0x0059ffa8) const char* const& kMapSpecsSheetCaptionStr = akGeneralStringImp[5];
DATA(0x005a14e4) const char* const& kObjectPropertiesCaptionFmtStr = akGeneralStringImp[6];
DATA(0x005a1940) const char* const& kEditTimedEventSheetCaptionStr = akGeneralStringImp[7];
DATA(0x005a145c) const char* const& kSelNoneStr = akGeneralStringImp[8];
DATA(0x005a0870) const char* const& kRandomStr = akGeneralStringImp[9];
DATA(0x0059fef4) const char* const& kUnknownStr = akGeneralStringImp[10];
DATA(0x005a1908) const char* const& kBackpackStr = akGeneralStringImp[11];
DATA(0x005a0f20) const char* const& kExperienceStr = akGeneralStringImp[12];
DATA(0x005a0c34) const char* const& kManaStr = akGeneralStringImp[13];
DATA(0x005a0d78) const char* const& kMoraleStr = akGeneralStringImp[14];
DATA(0x005a0abc) const char* const& kLuckStr = akGeneralStringImp[15];
DATA(0x005a11d4) const char* const& kResourceStr = akGeneralStringImp[16];
DATA(0x005a1b08) const char* const& kResourcesStr = akGeneralStringImp[17];
DATA(0x0059ff78) const char* const& kPrimarySkillStr = akGeneralStringImp[18];
DATA(0x005a0bc8) const char* const& kPrimarySkillsStr = akGeneralStringImp[19];
DATA(0x005a0d80) const char* const& kArtifactStr = akGeneralStringImp[20];
DATA(0x005a1ac8) const char* const& kArtifactsStr = akGeneralStringImp[21];
DATA(0x005a1b04) const char* const& kSpellStr = akGeneralStringImp[22];
DATA(0x005a12b0) const char* const& kSpellsStr = akGeneralStringImp[23];
DATA(0x005a0fbc) const char* const& kCreatureStr = akGeneralStringImp[24];
DATA(0x005a084c) const char* const& kCreaturesStr = akGeneralStringImp[25];
DATA(0x0059ffa0) const char* const& kSecondarySkillStr = akGeneralStringImp[26];
DATA(0x005a05a4) const char* const& kSecondarySkillsStr = akGeneralStringImp[27];
DATA(0x005a180c) const char* const& kPlayerNameFmtStr = akGeneralStringImp[28];
DATA(0x005a0e1c) const char* const& kPlayerNoneStr = akGeneralStringImp[29];
DATA(0x005a18a8) const char* const& kNeverStr = akGeneralStringImp[30];
DATA(0x005a0fc4) const char* const& kEveryDayStr = akGeneralStringImp[31];
DATA(0x005a116c) const char* const& kEvery2DaysStr = akGeneralStringImp[32];
DATA(0x005a142c) const char* const& kEvery3DaysStr = akGeneralStringImp[33];
DATA(0x0059ff20) const char* const& kEvery4DaysStr = akGeneralStringImp[34];
DATA(0x005a0c70) const char* const& kEvery5DaysStr = akGeneralStringImp[35];
DATA(0x005a1000) const char* const& kEvery6DaysStr = akGeneralStringImp[36];
DATA(0x0059ff7c) const char* const& kEvery7DaysStr = akGeneralStringImp[37];
DATA(0x005a1ac0) const char* const& kEvery14DaysStr = akGeneralStringImp[38];
DATA(0x005a0f64) const char* const& kEvery21DaysStr = akGeneralStringImp[39];
DATA(0x005a1a54) const char* const& kEvery28DaysStr = akGeneralStringImp[40];
DATA(0x005a0998) const char* const& kMaxObjectsOfTypeFmtStr = akGeneralStringImp[41];
DATA(0x005a0408) const char* const& kNoMoreClassHeroesStr = akGeneralStringImp[42];
DATA(0x005a0fb8) const char* const& kSelectPlayerBeforePlacingHeroStr = akGeneralStringImp[43];
DATA(0x005a11e0) const char* const& kMaxHeroesPerPlayerFmtStr = akGeneralStringImp[44];
DATA(0x005a0ecc) const char* const& kGrailAlreadyPlacedStr = akGeneralStringImp[45];
DATA(0x005a0400) const char* const& kGrailPlacedTooCloseToEdgeFmtStr = akGeneralStringImp[46];
DATA(0x0059ff08) const char* const& kNowhereToPasteStr = akGeneralStringImp[47];
DATA(0x005a1030) const char* const& kDebugBuildStr = akGeneralStringImp[48];
DATA(0x005a0d84) const char* const& kReleaseBuildStr = akGeneralStringImp[49];
DATA(0x005a190c) const char* const& kNeed16BitColorStr = akGeneralStringImp[50];
DATA(0x005a14ec) const char* const& kInvalidMapVersionFmtStr = akGeneralStringImp[51];
DATA(0x005a11cc) const char* const& kInvalidMapFileStr = akGeneralStringImp[52];
DATA(0x005a0f14) const char* const& k2DaysStr = akGeneralStringImp[53];
DATA(0x005a1250) const char* const& k3DaysStr = akGeneralStringImp[54];
DATA(0x005a1334) const char* const& k4DaysStr = akGeneralStringImp[55];
DATA(0x0059ff9c) const char* const& k5DaysStr = akGeneralStringImp[56];
DATA(0x005a1178) const char* const& k6DaysStr = akGeneralStringImp[57];
DATA(0x005a0ffc) const char* const& k1WeekStr = akGeneralStringImp[58];
DATA(0x005a0f2c) const char* const& k2WeeksStr = akGeneralStringImp[59];
DATA(0x005a0fd0) const char* const& k3WeeksStr = akGeneralStringImp[60];
DATA(0x005a1410) const char* const& k4WeeksStr = akGeneralStringImp[61];
DATA(0x005a0cf8) const char* const& k5WeeksStr = akGeneralStringImp[62];
DATA(0x005a1320) const char* const& k6WeeksStr = akGeneralStringImp[63];
DATA(0x0059ffac) const char* const& k7WeeksStr = akGeneralStringImp[64];
DATA(0x005a0fb4) const char* const& k2MonthsStr = akGeneralStringImp[65];
DATA(0x005a0d74) const char* const& k3MonthsStr = akGeneralStringImp[66];
DATA(0x005a14e0) const char* const& k4MonthsStr = akGeneralStringImp[67];
DATA(0x005a0a68) const char* const& k5MonthsStr = akGeneralStringImp[68];
DATA(0x005a0f24) const char* const& k6MonthsStr = akGeneralStringImp[69];
DATA(0x005a1328) const char* const& k7MonthsStr = akGeneralStringImp[70];
DATA(0x005a0c98) const char* const& k8MonthsStr = akGeneralStringImp[71];
DATA(0x005a0494) const char* const& k9MonthsStr = akGeneralStringImp[72];
DATA(0x005a06e4) const char* const& k10MonthsStr = akGeneralStringImp[73];
DATA(0x005a0db8) const char* const& k11MonthsStr = akGeneralStringImp[74];
DATA(0x005a0f60) const char* const& k12MonthsStr = akGeneralStringImp[75];
DATA(0x005a0490) const char* const& kAnyTownStr = akGeneralStringImp[76];
DATA(0x005a11dc) const char* const& kObjectAtLocationFmtStr = akGeneralStringImp[77];
DATA(0x005a0fd4) const char* const& kSpecificHeroAndClassFmtStr = akGeneralStringImp[78];
DATA(0x005a081c) const char* const& kRandomMonsterStr = akGeneralStringImp[79];
DATA(0x005a0528) const char* const& kRandomMonstersStr = akGeneralStringImp[80];
DATA(0x005a1a24) const char* const& kWarriorStr = akGeneralStringImp[81];
DATA(0x005a0f5c) const char* const& kBuilderStr = akGeneralStringImp[82];
DATA(0x005a1890) const char* const& kExplorerStr = akGeneralStringImp[83];
DATA(0x005a0818) const char* const& kWarningStr = akGeneralStringImp[84];
DATA(0x005a11c0) const char* const& kHeroesNeedOwnerStr = akGeneralStringImp[85];
DATA(0x005a05a0) const char* const& kPlayerHasMaxHeroesStr = akGeneralStringImp[86];
DATA(0x005a0e68) const char* const& kContinuingWillDeleteHeroFmtStr = akGeneralStringImp[87];
DATA(0x005a1ac4) const char* const& kNoMoreHeroesStr = akGeneralStringImp[88];
DATA(0x005a1720) const char* const& kNoPlayersOnMapStr = akGeneralStringImp[89];
DATA(0x005a1170) const char* const& kRandomTownStr = akGeneralStringImp[90];
DATA(0x005a1458) const char* const& kTeamFmtStr = akGeneralStringImp[91];
DATA(0x005a117c) const char* const& kThereAreNoPlayersOnMapStr = akGeneralStringImp[92];
DATA(0x005a0ec8) const char* const& kThereAreNoTownsOnMapStr = akGeneralStringImp[93];
DATA(0x005a0da8) const char* const& kMapHasNoNameStr = akGeneralStringImp[94];
DATA(0x005a1324) const char* const& kMapHasNoDescStr = akGeneralStringImp[95];
DATA(0x005a0dc0) const char* const& kGrailPlacedButNoObelisksStr = akGeneralStringImp[96];
DATA(0x005a18a4) const char* const& kPlayerOwnsObjectsButNotPresentFmtStr = akGeneralStringImp[97];
DATA(0x005a0dd0) const char* const& kTownHasShipyardButIsLandlockedFmtStr = akGeneralStringImp[98];
DATA(0x005a1160) const char* const& kOnlyOneMonolithOnMapFmtStr = akGeneralStringImp[99];
DATA(0x005a0d64) const char* const& kMonolithEntranceButNoExitFmtStr = akGeneralStringImp[100];
DATA(0x0059ff10) const char* const& kMonolithExitButNoEntranceFmtStr = akGeneralStringImp[101];
DATA(0x005a131c) const char* const& kSubterraneanGatesOnOneLayerMapStr = akGeneralStringImp[102];
DATA(0x005a1198) const char* const& kMoreSubterraneanGatesAboveThanBelowStr = akGeneralStringImp[103];
DATA(0x005a0f18) const char* const& kMoreSubterraneanGatesBelowThanAboveStr = akGeneralStringImp[104];
DATA(0x005a0a94) const char* const& kSeersHutQuestArtifactNotOnMapFmtStr = akGeneralStringImp[105];
DATA(0x0059ff98) const char* const& kOnePlayerPresentButNoSpecialVictoryConditionStr = akGeneralStringImp[106];
DATA(0x0059ff24) const char* const& kOnePlayerPresentButNormalVictoryNotDisabledStr = akGeneralStringImp[107];
DATA(0x0059ffb0) const char* const& kThereAreNoProblemsWithMapStr = akGeneralStringImp[108];
DATA(0x005a0820) const char* const& kSVCAquireArtifactButArtifactNotPresentFmtStr = akGeneralStringImp[109];
DATA(0x005a0dbc) const char* const& kSVCAquireArtifactButHeroHasArtifactFmtStr = akGeneralStringImp[110];
DATA(0x005a0fe0) const char* const& kSVCAccumCreaturePlayerAlreadyHasEnoughCreaturesFmtStr = akGeneralStringImp[111];
DATA(0x005a0a6c) const char* const& kSVCUpgradeTownBuildingsAlreadyBuiltFmtStr = akGeneralStringImp[112];
DATA(0x005a1174) const char* const& kSVCUpgradeTownBuildingsDisabledFmtStr = akGeneralStringImp[113];
DATA(0x005a1a5c) const char* const& kSVCBuildGrailStructAlreadyBuiltFmtStr = akGeneralStringImp[114];
DATA(0x005a048c) const char* const& kSVCBuildGrailStructDisabledFmtStr = akGeneralStringImp[115];
DATA(0x005a17d8) const char* const& kSVCBuildGrailStructNowhereToBuildStr = akGeneralStringImp[116];
DATA(0x005a1340) const char* const& kSVCDefeatHeroBelongsToHumanFmtStr = akGeneralStringImp[117];
DATA(0x005a0d6c) const char* const& kSVCCaptureTownAlreadyOwnedFmtStr = akGeneralStringImp[118];
DATA(0x005a1460) const char* const& kSVCCaptureTownOwnedByHumanFmtStr = akGeneralStringImp[119];
DATA(0x005a133c) const char* const& kSVCFlagGeneratorsNoGeneratorsOnMapStr = akGeneralStringImp[120];
DATA(0x005a0a74) const char* const& kSVCFlagGeneratorsPlayerOwnsAllGeneratorsFmtStr = akGeneralStringImp[121];
DATA(0x005a1a1c) const char* const& kSVCFlagMinesNoMinesOnMapStr = akGeneralStringImp[122];
DATA(0x005a04d8) const char* const& kSVCFlagMinesPlayerAlreadyOwnsAllMinesFmtStr = akGeneralStringImp[123];
DATA(0x005a1144) const char* const& kSVCTransportArtifactNotPresentFmtStr = akGeneralStringImp[124];
DATA(0x005a0d70) const char* const& kSLCLoseTownNotOwnedFmtStr = akGeneralStringImp[125];
DATA(0x005a188c) const char* const& kSLCLoseTownTeamNotHumanFmtStr = akGeneralStringImp[126];
DATA(0x005a0fd8) const char* const& kSLCLoseTownMultipleHumanTeamsFmtStr = akGeneralStringImp[127];
DATA(0x005a058c) const char* const& kSLCLoseTownPlayerNotHumanFmtStr = akGeneralStringImp[128];
DATA(0x005a1134) const char* const& kSLCLoseTownMultipleHumanPlayersFmtStr = akGeneralStringImp[129];
DATA(0x005a1254) const char* const& kSLCLoseHeroNotOwnedFmtStr = akGeneralStringImp[130];
DATA(0x005a0fc0) const char* const& kSLCLoseHeroTeamNotHumanFmtStr = akGeneralStringImp[131];
DATA(0x005a1adc) const char* const& kSLCLoseHeroMultipleHumanTeamsFmtStr = akGeneralStringImp[132];
DATA(0x005a1338) const char* const& kSLCLoseHeroPlayerNotHumanFmtStr = akGeneralStringImp[133];
DATA(0x005a0588) const char* const& kSLCLoseHeroMultipleHumanPlayersFmtStr = akGeneralStringImp[134];
DATA(0x005a0930) const char* const& kRecoverAutosavedMapStr = akGeneralStringImp[135];
DATA(0x005a1a20) const char* const& kTextFileFilterStr = akGeneralStringImp[136];
DATA(0x0059ff18) const char* const& kUnableToImportTextFmtStr = akGeneralStringImp[137];
DATA(0x005a0fe8) const char* const& kTextImportedSuccessfullyFmtStr = akGeneralStringImp[138];
DATA(0x005a1230) const char* const& kMapNameStr = akGeneralStringImp[139];
DATA(0x005a1258) const char* const& kMapDescriptionStr = akGeneralStringImp[140];
DATA(0x005a0fa8) const char* const& kRumorsStr = akGeneralStringImp[141];
DATA(0x005a14a4) const char* const& kTimedEventsStr = akGeneralStringImp[142];
DATA(0x005a122c) const char* const& kObjectsStr = akGeneralStringImp[143];
DATA(0x005a1808) const char* const& kNameStr = akGeneralStringImp[144];
DATA(0x005a1190) const char* const& kMessageStr = akGeneralStringImp[145];
DATA(0x005a191c) const char* const& kTextStr = akGeneralStringImp[146];
DATA(0x005a0e70) const char* const& kVisitingHeroFmtStr = akGeneralStringImp[147];
DATA(0x005a1228) const char* const& kEndOfFileStr = akGeneralStringImp[148];
DATA(0x005a13e4) const char* const& kTimedEventNameFmtStr = akGeneralStringImp[149];
DATA(0x005a12a8) const char* const& kObjectNotFoundStr = akGeneralStringImp[150];
DATA(0x005a05b0) const char* const& kToolbarCaptionStr = akGeneralStringImp[151];
DATA(0x005a1aec) const char* const& kModeBarCaptionStr = akGeneralStringImp[152];
DATA(0x005a0f34) const char* const& kOneWhirlpoolStr = akGeneralStringImp[153];
DATA(0x005a1b00) const char* const& kObjectIsUnreachableFmtStr = akGeneralStringImp[154];
DATA(0x005a0850) const char* const& kOKStr = akGeneralStringImp[155];
DATA(0x005a0844) const char* const& kCancelStr = akGeneralStringImp[156];
DATA(0x005a0f58) const char* const& kHelpStr = akGeneralStringImp[157];
DATA(0x005a0a88) const char* const& kGeneralPageCaptionStr = akGeneralStringImp[158];
DATA(0x005a05b8) const char* const& kContentsPageCaptionStr = akGeneralStringImp[159];
DATA(0x005a06e0) const char* const& kResourcesPageCaptionStr = akGeneralStringImp[160];
DATA(0x005a13e8) const char* const& kBuildingsPageCaptionStr = akGeneralStringImp[161];
DATA(0x005a132c) const char* const& kCreaturesPageCaptionStr = akGeneralStringImp[162];
DATA(0x005a1a58) const char* const& kSecondarySkillsPageCaptionStr = akGeneralStringImp[163];
DATA(0x005a1a50) const char* const& kSpecialLossConditionPageCaptionStr = akGeneralStringImp[164];
DATA(0x005a0f54) const char* const& kPlayerSpecsPageCaptionStr = akGeneralStringImp[165];
DATA(0x005a1398) const char* const& kRumorsPageCaptionStr = akGeneralStringImp[166];
DATA(0x005a1afc) const char* const& kTeamsPageCaptionStr = akGeneralStringImp[167];
DATA(0x005a156c) const char* const& kTimedEventsPageCaptionStr = akGeneralStringImp[168];
DATA(0x005a0dc8) const char* const& kSpecialVictoryConditionPageCaptionStr = akGeneralStringImp[169];
DATA(0x005a0550) const char* const& kTreasurePageCaptionStr = akGeneralStringImp[170];
DATA(0x005a14e8) const char* const& kGarrisonPageCaptionStr = akGeneralStringImp[171];
DATA(0x005a0da4) const char* const& kSpellsPageCaptionStr = akGeneralStringImp[172];
DATA(0x005a0ab0) const char* const& kGuardiansPageCaptionStr = akGeneralStringImp[173];
DATA(0x005a1168) const char* const& kArtifactsPageCaptionStr = akGeneralStringImp[174];
DATA(0x005a0f38) const char* const& kAboutBoxCaptionStr = akGeneralStringImp[175];
DATA(0x005a1af0) const char* const& kEditArtifactCaptionStr = akGeneralStringImp[176];
DATA(0x005a1810) const char* const& kEditCreatureStackCaptionStr = akGeneralStringImp[177];
DATA(0x005a15e8) const char* const& kEditRumorCaptionStr = akGeneralStringImp[178];
DATA(0x005a0dcc) const char* const& kEditSecondarySkillCaptionStr = akGeneralStringImp[179];
DATA(0x005a13ec) const char* const& kFindCaptionStr = akGeneralStringImp[180];
DATA(0x005a0c6c) const char* const& kMapValidationCaptionStr = akGeneralStringImp[181];
DATA(0x005a0ff8) const char* const& kNewMapCaptionStr = akGeneralStringImp[182];
DATA(0x005a1918) const char* const& kOptionsCaptionStr = akGeneralStringImp[183];
DATA(0x005a1914) const char* const& kSelectArtifactCaptionStr = akGeneralStringImp[184];
DATA(0x0059ff80) const char* const& kSelectHeroClassCaptionStr = akGeneralStringImp[185];
DATA(0x005a0404) const char* const& kSelectSpellCaptionStr = akGeneralStringImp[186];
DATA(0x005a1264) const char* const& kCustomizeCheckStr = akGeneralStringImp[187];
DATA(0x005a1a18) const char* const& kBrushStr = akGeneralStringImp[188];
DATA(0x005a11d8) const char* const& kTerrainTypeStr = akGeneralStringImp[189];
DATA(0x005a0f50) const char* const& kRiverTypeStr = akGeneralStringImp[190];
DATA(0x005a1570) const char* const& kRoadTypeStr = akGeneralStringImp[191];
DATA(0x0059ffa4) const char* const& kPlacementRadiusStaticStr = akGeneralStringImp[192];
DATA(0x005a1904) const char* const& kStandStillStr = akGeneralStringImp[193];
DATA(0x005a1430) const char* const& kRadiusOneSquareStr = akGeneralStringImp[194];
DATA(0x005a0fa4) const char* const& kRadiusTwoSquaresStr = akGeneralStringImp[195];
DATA(0x005a17bc) const char* const& kRadiusThreeSquaresStr = akGeneralStringImp[196];
DATA(0x005a139c) const char* const& kRadiusFourSquaresStr = akGeneralStringImp[197];
DATA(0x005a1130) const char* const& kRadiusFiveSquaresStr = akGeneralStringImp[198];
DATA(0x005a0a64) const char* const& kRadiusSixSquaresStr = akGeneralStringImp[199];
DATA(0x005a1330) const char* const& kRadiusSevenSquaresStr = akGeneralStringImp[200];
DATA(0x005a112c) const char* const& kRadiusEightSquaresStr = akGeneralStringImp[201];
DATA(0x005a17d4) const char* const& kRadiusNineSquaresStr = akGeneralStringImp[202];
DATA(0x005a0848) const char* const& kRadiusTenSquaresStr = akGeneralStringImp[203];
DATA(0x005a1164) const char* const& kProblemsStaticStr = akGeneralStringImp[204];
DATA(0x0059ff74) const char* const& kNoPropertiesStr = akGeneralStringImp[205];
DATA(0x0059ff0c) const char* const& kContinuingWillDeleteUndergroundLayerStr = akGeneralStringImp[206];
DATA(0x005a17d0) const char* const& kRandomDwellingFmtStr = akGeneralStringImp[207];
DATA(0x005a0f68) const char* const& kRandomTownDwellingFmtStr = akGeneralStringImp[208];
DATA(0x005a04b4) const char* const& kBiographyStr = akGeneralStringImp[209];
DATA(0x005a0f28) const char* const& kMapGenerationProgressCaptionStr = akGeneralStringImp[210];
DATA(0x005a1004) const char* const& kGeneratingMapStr = akGeneralStringImp[211];
DATA(0x005a0a98) const char* const& kUnableToGenerateRandomMapStr = akGeneralStringImp[212];
DATA(0x005a1af4) const char* const& kHeroesStr = akGeneralStringImp[213];
DATA(0x005a0dc4) const char* const& kQuestObjectHasNoQuestFmtStr = akGeneralStringImp[214];
DATA(0x005a0f1c) const char* const& kObjectNeedsABOrSoDStr = akGeneralStringImp[215];
DATA(0x005a1af8) const char* const& kNoQuestStr = akGeneralStringImp[216];
DATA(0x005a0f3c) const char* const& kQuestExperienceLevelFmtStr = akGeneralStringImp[217];
DATA(0x005a0d68) const char* const& kQuestPrimarySkillLevelFmtStr = akGeneralStringImp[218];
DATA(0x005a0f10) const char* const& kQuestAchieveFmtStr = akGeneralStringImp[219];
DATA(0x005a1910) const char* const& kQuestAchieveTheFollowingStr = akGeneralStringImp[220];
DATA(0x005a1944) const char* const& kQuestDefeatHeroFmtStr = akGeneralStringImp[221];
DATA(0x005a0c94) const char* const& kQuestDefeatMonsterFmtStr = akGeneralStringImp[222];
DATA(0x005a13e0) const char* const& kQuestReturnWithTheFollowingStr = akGeneralStringImp[223];
DATA(0x005a05ac) const char* const& kQuestReturnWithArtifactFmtStr = akGeneralStringImp[224];
DATA(0x005a11d0) const char* const& kQuestReturnWithCreaturesFmtStr = akGeneralStringImp[225];
DATA(0x005a054c) const char* const& kQuestReturnWithResourcesFmtStr = akGeneralStringImp[226];
DATA(0x005a0e6c) const char* const& kProposalMessageStr = akGeneralStringImp[227];
DATA(0x005a0ed0) const char* const& kProgressMessageStr = akGeneralStringImp[228];
DATA(0x005a0fe4) const char* const& kCompletionMessageStr = akGeneralStringImp[229];
DATA(0x005a1574) const char* const& kAndStr = akGeneralStringImp[230];
DATA(0x005a0938) const char* const& kUndergroundStr = akGeneralStringImp[231];
DATA(0x005a0f30) const char* const& kNoRewardStr = akGeneralStringImp[232];
DATA(0x005a1604) const char* const& kRewardExperienceFmtStr = akGeneralStringImp[233];
DATA(0x0059fee0) const char* const& kRewardManaFmtStr = akGeneralStringImp[234];
DATA(0x005a12ac) const char* const& kRewardMoraleFmtStr = akGeneralStringImp[235];
DATA(0x005a0548) const char* const& kRewardLuckFmtStr = akGeneralStringImp[236];
DATA(0x005a1900) const char* const& kRewardSpellFmtStr = akGeneralStringImp[237];
DATA(0x005a0fdc) const char* const& kKeymastersTentButNoBorderFmtStr = akGeneralStringImp[238];
DATA(0x005a0ec4) const char* const& kBorderGuardButNoKeymastersTentFmtStr = akGeneralStringImp[239];
DATA(0x005a04b0) const char* const& kBorderGateButNoKeymastersTentFmtStr = akGeneralStringImp[240];
DATA(0x005a1568) const char* const& kHutOfTheMagiButNoEyeStr = akGeneralStringImp[241];
DATA(0x005a1ae8) const char* const& kEyeOfTheMagiButNoHutStr = akGeneralStringImp[242];
DATA(0x005a0934) const char* const& kTooFewAvailableHeroesFmtStr = akGeneralStringImp[243];
DATA(0x005a0c68) const char* const& kAllArtifactsDisabledOrReservedStr = akGeneralStringImp[244];
DATA(0x0059ff28) const char* const& kObjectNeedsSoDStr = akGeneralStringImp[245];
DATA(0x005a11c8) const char* const& kObjectNeedsABInstalledStr = akGeneralStringImp[246];
DATA(0x005a1194) const char* const& kHeroesPageCaptionStr = akGeneralStringImp[247];

namespace SAbandonedMinePropsDlgText {
namespace {
DATA(0x005a14b0) const char* akStringImp[kNumStrings];
}
DATA(0x005a14dc) const char* const& kPotentialResourcesStaticStr = akStringImp[0];
DATA(0x005a14d8) const char* const& kRes2CheckStr = akStringImp[1];
DATA(0x005a14d0) const char* const& kRes3CheckStr = akStringImp[2];
DATA(0x005a14cc) const char* const& kRes4CheckStr = akStringImp[3];
DATA(0x005a14ac) const char* const& kRes5CheckStr = akStringImp[4];
DATA(0x005a14a8) const char* const& kRes6CheckStr = akStringImp[5];
DATA(0x005a14d4) const char* const& kRes7CheckStr = akStringImp[6];
}

namespace SArmyDlgText {
namespace {
DATA(0x005a1294) const char* akStringImp[kNumStrings];
}
DATA(0x005a12a0) const char* const& kTypeStaticStr = akStringImp[0];
DATA(0x005a12a4) const char* const& kQuantityStaticStr = akStringImp[1];
DATA(0x005a1290) const char* const& kSlotStaticFmtStr = akStringImp[2];
}

namespace SGarrisonPropertiesDlgText {
namespace {
DATA(0x005a1274) const char* akStringImp[kNumStrings];
}
DATA(0x005a1268) const char* const& kOwnerStaticStr = akStringImp[0];
DATA(0x005a1288) const char* const& kNoneRadioStr = akStringImp[1];
DATA(0x005a1270) const char* const& kPlayerRadioFmtStr = akStringImp[2];
DATA(0x005a128c) const char* const& kCreaturesStaticStr = akStringImp[3];
DATA(0x005a126c) const char* const& kRemovableCheckStr = akStringImp[4];
}

namespace SArtifactPropsGeneralPageText {
namespace {
DATA(0x005a13c0) const char* akStringImp[kNumStrings];
}
DATA(0x005a13d4) const char* const& kTypeStaticStr = akStringImp[0];
DATA(0x005a13dc) const char* const& kMessageStaticStr = akStringImp[1];
DATA(0x005a13d8) const char* const& kNoteStaticStr = akStringImp[2];
DATA(0x005a13d0) const char* const& kSpellStaticStr = akStringImp[3];
}

namespace SBlackBoxPropsGeneralPageText {
namespace {
DATA(0x005a1800) const char* akStringImp[kNumStrings];
}
DATA(0x005a1804) const char* const& kMessageStaticStr = akStringImp[0];
}

namespace SEventPropsGeneralPageText {
namespace {
DATA(0x005a0828) const char* akStringImp[kNumStrings];
}
DATA(0x005a0838) const char* const& kAllowedPlayersStaticStr = akStringImp[0];
DATA(0x005a083c) const char* const& kPlayerCheckFmtStr = akStringImp[1];
DATA(0x005a0840) const char* const& kAllowComputerCheckStr = akStringImp[2];
DATA(0x005a0824) const char* const& kCancelCheckStr = akStringImp[3];
}

namespace SBlackBoxPropsContentsPageText {
namespace {
DATA(0x005a1664) const char* akStringImp[kNumStrings];
}
DATA(0x005a16d8) const char* const& kCategoryStaticStr = akStringImp[0];
DATA(0x005a165c) const char* const& kAddButtonStr = akStringImp[1];
DATA(0x005a1714) const char* const& kEditButtonStr = akStringImp[2];
DATA(0x005a16e4) const char* const& kRemoveButtonStr = akStringImp[3];
DATA(0x005a16ec) const char* const& kRemoveAllButtonStr = akStringImp[4];
DATA(0x005a16fc) const char* const& kArtifactsStaticStr = akStringImp[5];
DATA(0x005a16d4) const char* const& kCreatureTypeStaticStr = akStringImp[6];
DATA(0x005a1708) const char* const& kQuantityStaticStr = akStringImp[7];
DATA(0x005a16dc) const char* const& kExperienceBonusStaticStr = akStringImp[8];
DATA(0x005a1644) const char* const& kLuckBonusStaticStr = akStringImp[9];
DATA(0x005a16e8) const char* const& kNoneRadioStr = akStringImp[10];
DATA(0x005a1704) const char* const& kPlusOneRadioStr = akStringImp[11];
DATA(0x005a171c) const char* const& kPlusTwoRadioStr = akStringImp[12];
DATA(0x005a170c) const char* const& kPlusThreeRadioStr = akStringImp[13];
DATA(0x005a1648) const char* const& kMinusOneRadioStr = akStringImp[14];
DATA(0x005a1660) const char* const& kMinusTwoRadioStr = akStringImp[15];
DATA(0x005a164c) const char* const& kMinusThreeRadioStr = akStringImp[16];
DATA(0x005a1710) const char* const& kManaBonusStaticStr = akStringImp[17];
DATA(0x005a16e0) const char* const& kGiveRadioStr = akStringImp[18];
DATA(0x005a16f4) const char* const& kTakeRadioStr = akStringImp[19];
DATA(0x005a1718) const char* const& kMoraleBonusStaticStr = akStringImp[20];
DATA(0x005a1700) const char* const& kAttackSkillStaticStr = akStringImp[21];
DATA(0x005a16f8) const char* const& kDefenseSkillStaticStr = akStringImp[22];
DATA(0x005a1658) const char* const& kSpellPowerStaticStr = akStringImp[23];
DATA(0x005a16f0) const char* const& kKnowledgeStaticStr = akStringImp[24];
DATA(0x005a1640) const char* const& kSecondarySkillStaticStr = akStringImp[25];
DATA(0x005a1654) const char* const& kMasteryStaticStr = akStringImp[26];
DATA(0x005a1650) const char* const& kSpellsStaticStr = akStringImp[27];
}

namespace SResourceQuantitiesDlgText {
namespace {
DATA(0x005a039c) const char* akStringImp[kNumStrings];
}
DATA(0x005a03c0) const char* const& kRes1StaticStr = akStringImp[0];
DATA(0x005a0394) const char* const& kRes2StaticStr = akStringImp[1];
DATA(0x005a03d4) const char* const& kRes3StaticStr = akStringImp[2];
DATA(0x005a03c4) const char* const& kRes4StaticStr = akStringImp[3];
DATA(0x005a0398) const char* const& kRes5StaticStr = akStringImp[4];
DATA(0x005a03d0) const char* const& kRes6StaticStr = akStringImp[5];
DATA(0x005a03c8) const char* const& kRes7StaticStr = akStringImp[6];
DATA(0x005a03cc) const char* const& kGiveRadioStr = akStringImp[7];
DATA(0x005a03d8) const char* const& kTakeRadioStr = akStringImp[8];
}

namespace SEditSecondarySkillDlgText {
namespace {
DATA(0x005a0d44) const char* akStringImp[kNumStrings];
}
DATA(0x005a0d58) const char* const& kTypeStaticStr = akStringImp[0];
DATA(0x005a0d3c) const char* const& kMasteryStaticStr = akStringImp[1];
DATA(0x005a0d60) const char* const& kBasicRadioStr = akStringImp[2];
DATA(0x005a0d5c) const char* const& kAdvancedRadioStr = akStringImp[3];
DATA(0x005a0d40) const char* const& kExpertRadioStr = akStringImp[4];
}

namespace SEditCreatureStackDlgText {
namespace {
DATA(0x005a1acc) const char* akStringImp[kNumStrings];
}
DATA(0x005a1ad4) const char* const& kTypeStaticStr = akStringImp[0];
DATA(0x005a1ad8) const char* const& kQuantityStaticStr = akStringImp[1];
}

namespace SEditTimedEventGeneralPageText {
namespace {
DATA(0x005a1344) const char* akStringImp[kNumStrings];
}
DATA(0x005a136c) const char* const& kEventNameStaticStr = akStringImp[0];
DATA(0x005a1370) const char* const& kMessageStaticStr = akStringImp[1];
DATA(0x005a1364) const char* const& kAllowedPlayersStaticStr = akStringImp[2];
DATA(0x005a1378) const char* const& kPlayerCheckFmtStr = akStringImp[3];
DATA(0x005a1380) const char* const& kAllowComputerCheckStr = akStringImp[4];
DATA(0x005a1374) const char* const& kFirstOccurenceStaticStr = akStringImp[5];
DATA(0x005a137c) const char* const& kSubsequentOccurenceStaticStr = akStringImp[6];
DATA(0x005a1368) const char* const& kAllowHumanCheckStr = akStringImp[7];
}

namespace SEditTownEventBuildingsPageText {
namespace {
DATA(0x005a0a80) const char* akStringImp[kNumStrings];
}
DATA(0x005a0a78) const char* const& kBuildingTreeStaticStr = akStringImp[0];
DATA(0x005a0a7c) const char* const& kBuildCheckStr = akStringImp[1];
}

namespace SEditTownEventCreaturesPageText {
namespace {
DATA(0x005a1414) const char* akStringImp[kNumStrings];
}
DATA(0x005a1420) const char* const& kGeneratorNoStaticStr = akStringImp[0];
DATA(0x005a1428) const char* const& kQuantityStaticStr = akStringImp[1];
DATA(0x005a1424) const char* const& kNoteStaticStr = akStringImp[2];
}

namespace SEditArtifactDlgText {
namespace {
DATA(0x005a0f04) const char* akStringImp[kNumStrings];
}
DATA(0x005a0f00) const char* const& kEquipWhereStaticStr = akStringImp[0];
DATA(0x005a0f0c) const char* const& kArtifactStaticStr = akStringImp[1];
}

namespace SEditRumorDlgText {
namespace {
DATA(0x005a1894) const char* akStringImp[kNumStrings];
}
DATA(0x005a18a0) const char* const& kNameStaticStr = akStringImp[0];
DATA(0x005a189c) const char* const& kTextStaticStr = akStringImp[1];
}

namespace SFindDlgText {
namespace {
DATA(0x005a06c4) const char* akStringImp[kNumStrings];
}
DATA(0x005a06c0) const char* const& kFindWhatStaticStr = akStringImp[0];
DATA(0x005a06bc) const char* const& kFindNextButtonStr = akStringImp[1];
DATA(0x005a06d0) const char* const& kFindPrevButtonStr = akStringImp[2];
}

namespace SFlaggablePropsDlgText {
namespace {
DATA(0x005a15f4) const char* akStringImp[kNumStrings];
}
DATA(0x005a15ec) const char* const& kOwnerStaticStr = akStringImp[0];
DATA(0x005a1600) const char* const& kNoneRadioStr = akStringImp[1];
DATA(0x005a15f0) const char* const& kPlayerRadioFmtStr = akStringImp[2];
}

namespace SHeroPropsGeneralPageText {
namespace {
DATA(0x005a05d0) const char* akStringImp[kNumStrings];
}
DATA(0x005a062c) const char* const& kClassStaticStr = akStringImp[0];
DATA(0x005a05c4) const char* const& kPlayerStaticStr = akStringImp[1];
DATA(0x005a05c0) const char* const& kIdentityStaticStr = akStringImp[2];
DATA(0x005a0618) const char* const& kNameStaticStr = akStringImp[3];
DATA(0x005a0620) const char* const& kExperienceStaticStr = akStringImp[4];
DATA(0x005a0634) const char* const& kPortraitStaticStr = akStringImp[5];
DATA(0x005a05c8) const char* const& kPatrolStaticStr = akStringImp[6];
DATA(0x005a0630) const char* const& kCustomizeNameCheckStr = akStringImp[7];
DATA(0x005a0628) const char* const& kCustomizePortraitCheckStr = akStringImp[8];
DATA(0x005a0624) const char* const& kGenderStaticStr = akStringImp[9];
DATA(0x005a0614) const char* const& kDefaultGenderRadioFmtStr = akStringImp[10];
DATA(0x005a05cc) const char* const& kMaleStr = akStringImp[11];
DATA(0x005a061c) const char* const& kFemaleStr = akStringImp[12];
DATA(0x005a0610) const char* const& kMaleRadioStr = akStringImp[13];
DATA(0x005a0638) const char* const& kFemaleRadioStr = akStringImp[14];
DATA(0x005a05bc) const char* const& kCustomizeGenderCheckStr = akStringImp[15];
}

namespace SHeroPropsCreaturesPageText {
namespace {
DATA(0x005a1210) const char* akStringImp[kNumStrings];
}
DATA(0x005a1224) const char* const& kFormationStaticStr = akStringImp[0];
DATA(0x005a1220) const char* const& kSpreadRadioStr = akStringImp[1];
DATA(0x005a121c) const char* const& kGroupedRadioStr = akStringImp[2];
}

namespace SHeroPropsSecSkillsPageText {
namespace {
DATA(0x005a03ec) const char* akStringImp[kNumStrings];
}
DATA(0x005a03f8) const char* const& kTypeStaticStr = akStringImp[0];
DATA(0x005a03e8) const char* const& kMasteryStaticStr = akStringImp[1];
DATA(0x005a03e4) const char* const& kSkillStaticFmtStr = akStringImp[2];
}

namespace SHeroPropsArtifactsPageText {
namespace {
DATA(0x005a0d04) const char* akStringImp[kNumStrings];
}
DATA(0x005a0d2c) const char* const& kHasSpellbookCheckStr = akStringImp[0];
DATA(0x005a0d34) const char* const& kArtifactsStaticStr = akStringImp[1];
DATA(0x005a0d24) const char* const& kNameColumnStr = akStringImp[2];
DATA(0x005a0cfc) const char* const& kLocationColumnStr = akStringImp[3];
DATA(0x005a0d00) const char* const& kAddButtonStr = akStringImp[4];
DATA(0x005a0d38) const char* const& kEditButtonStr = akStringImp[5];
DATA(0x005a0d28) const char* const& kRemoveButtonStr = akStringImp[6];
DATA(0x005a0d30) const char* const& kRemoveAllButtonStr = akStringImp[7];
}

namespace SMapSpecsGeneralPageText {
namespace {
DATA(0x005a1834) const char* akStringImp[kNumStrings];
}
DATA(0x005a1820) const char* const& kDifficultyStaticStr = akStringImp[0];
DATA(0x005a1888) const char* const& kEasyRadioStr = akStringImp[1];
DATA(0x005a1824) const char* const& kNormalRadioStr = akStringImp[2];
DATA(0x005a186c) const char* const& kToughRadioStr = akStringImp[3];
DATA(0x005a1830) const char* const& kExpertRadioStr = akStringImp[4];
DATA(0x005a1884) const char* const& kImpossibleRadioStr = akStringImp[5];
DATA(0x005a187c) const char* const& kTwoLevelMapCheckStr = akStringImp[6];
DATA(0x005a181c) const char* const& kMapNameStaticStr = akStringImp[7];
DATA(0x005a1828) const char* const& kDescriptionStaticStr = akStringImp[8];
DATA(0x005a1878) const char* const& kLimitHeroLevelCheckStr = akStringImp[9];
DATA(0x005a1870) const char* const& kMapVersionStaticStr = akStringImp[10];
DATA(0x005a1880) const char* const& kRoERadioStr = akStringImp[11];
DATA(0x005a182c) const char* const& kABRadioStr = akStringImp[12];
DATA(0x005a1874) const char* const& kSoDRadioStr = akStringImp[13];
}

namespace SMapSpecsPlayerSpecsPageText {
namespace {
DATA(0x005a1748) const char* akStringImp[kNumStrings];
}
DATA(0x005a173c) const char* const& kPlayerStaticStr = akStringImp[0];
DATA(0x005a1738) const char* const& kGenerateHeroCheckStr = akStringImp[1];
DATA(0x005a179c) const char* const& kPlayabilityStaticStr = akStringImp[2];
DATA(0x005a17b0) const char* const& kHumanPlayableCheckStr = akStringImp[3];
DATA(0x005a1724) const char* const& kComputerPlayableCheckStr = akStringImp[4];
DATA(0x005a17a0) const char* const& kBehaviorStaticStr = akStringImp[5];
DATA(0x005a1734) const char* const& kHasMainTownCheckStr = akStringImp[6];
DATA(0x005a17ac) const char* const& kAllowedAlignmentsStaticStr = akStringImp[7];
DATA(0x005a1730) const char* const& kCustomizeCheckStr = akStringImp[8];
DATA(0x005a17a4) const char* const& kRandomTownCheckStr = akStringImp[9];
DATA(0x005a1794) const char* const& kCastleCheckStr = akStringImp[10];
DATA(0x005a1728) const char* const& kRampartCheckStr = akStringImp[11];
DATA(0x005a1744) const char* const& kTowerCheckStr = akStringImp[12];
DATA(0x005a1740) const char* const& kInfernoCheckStr = akStringImp[13];
DATA(0x005a1798) const char* const& kNecropolisCheckStr = akStringImp[14];
DATA(0x005a17a8) const char* const& kDungeonCheckStr = akStringImp[15];
DATA(0x005a17b8) const char* const& kStrongholdCheckStr = akStringImp[16];
DATA(0x005a17b4) const char* const& kFortressCheckStr = akStringImp[17];
DATA(0x005a172c) const char* const& kConfluxCheckStr = akStringImp[18];
}

namespace SMapSpecsTeamsPageText {
namespace {
DATA(0x005a0ba8) const char* akStringImp[kNumStrings];
}
DATA(0x005a0bc0) const char* const& kEnableTeamsCheckStr = akStringImp[0];
DATA(0x005a0bbc) const char* const& kNumberOfTeamsStaticStr = akStringImp[1];
DATA(0x005a0ba4) const char* const& kTeamAssignmentsStaticStr = akStringImp[2];
DATA(0x005a0ba0) const char* const& kTeamNumberStaticStr = akStringImp[3];
DATA(0x005a0bc4) const char* const& kPlayerStaticFmtStr = akStringImp[4];
}

namespace SMapSpecsRumorsPageText {
namespace {
DATA(0x005a1a2c) const char* akStringImp[kNumStrings];
}
DATA(0x005a1a44) const char* const& kRumorsStaticStr = akStringImp[0];
DATA(0x005a1a28) const char* const& kAddButtonStr = akStringImp[1];
DATA(0x005a1a4c) const char* const& kEditButtonStr = akStringImp[2];
DATA(0x005a1a40) const char* const& kRemoveButtonStr = akStringImp[3];
DATA(0x005a1a48) const char* const& kRemoveAllButtonStr = akStringImp[4];
}

namespace SMapSpecsTimedEventsPageText {
namespace {
DATA(0x005a160c) const char* akStringImp[kNumStrings];
}
DATA(0x005a1638) const char* const& kEventsStaticStr = akStringImp[0];
DATA(0x005a1608) const char* const& kAddButtonStr = akStringImp[1];
DATA(0x005a163c) const char* const& kEditButtonStr = akStringImp[2];
DATA(0x005a162c) const char* const& kRemoveButtonStr = akStringImp[3];
DATA(0x005a1634) const char* const& kRemoveAllButtonStr = akStringImp[4];
DATA(0x005a1630) const char* const& kMoveUpButtonStr = akStringImp[5];
DATA(0x005a1628) const char* const& kMoveDownButtonStr = akStringImp[6];
}

namespace SMapSpecsVictoryCondPageText {
namespace {
DATA(0x005a1054) const char* akStringImp[kNumStrings];
}
DATA(0x005a1048) const char* const& kSelectVictoryConditionStaticStr = akStringImp[0];
DATA(0x005a1100) const char* const& kNoneRadioStr = akStringImp[1];
DATA(0x005a103c) const char* const& kAquireArtifactRadioStr = akStringImp[2];
DATA(0x005a1108) const char* const& kAccumCreaturesRadioStr = akStringImp[3];
DATA(0x005a1110) const char* const& kAccumResourcesRadioStr = akStringImp[4];
DATA(0x005a10e0) const char* const& kUpgradeTownRadioStr = akStringImp[5];
DATA(0x005a1104) const char* const& kBuildGrailRadioStr = akStringImp[6];
DATA(0x005a1034) const char* const& kDefeatHeroRadioStr = akStringImp[7];
DATA(0x005a111c) const char* const& kCaptureTownRadioStr = akStringImp[8];
DATA(0x005a1128) const char* const& kDefeatMonsterRadioStr = akStringImp[9];
DATA(0x005a10d8) const char* const& kFlagGeneratorsRadioStr = akStringImp[10];
DATA(0x005a1124) const char* const& kFlagMinesRadioStr = akStringImp[11];
DATA(0x005a10ec) const char* const& kTransportArtifactRadioStr = akStringImp[12];
DATA(0x005a1114) const char* const& kNormalVictoryCheckStr = akStringImp[13];
DATA(0x005a10fc) const char* const& kComputerAlsoCheckStr = akStringImp[14];
DATA(0x005a10dc) const char* const& kArtifactStaticStr = akStringImp[15];
DATA(0x005a10d4) const char* const& kCreatureTypeStaticStr = akStringImp[16];
DATA(0x005a1118) const char* const& kQuantityStaticStr = akStringImp[17];
DATA(0x005a10d0) const char* const& kResourceTypeStaticStr = akStringImp[18];
DATA(0x005a10f8) const char* const& kTownStaticStr = akStringImp[19];
DATA(0x005a10f0) const char* const& kHallLevelStaticStr = akStringImp[20];
DATA(0x005a10e8) const char* const& kTownHallRadioStr = akStringImp[21];
DATA(0x005a10f4) const char* const& kCityHallRadioStr = akStringImp[22];
DATA(0x005a104c) const char* const& kCapitolHallRadioStr = akStringImp[23];
DATA(0x005a110c) const char* const& kCastleLevelStaticStr = akStringImp[24];
DATA(0x005a1120) const char* const& kFortCastleRadioStr = akStringImp[25];
DATA(0x005a1050) const char* const& kCitadelCastleRadioStr = akStringImp[26];
DATA(0x005a1038) const char* const& kCastleCastleRadioStr = akStringImp[27];
DATA(0x005a10e4) const char* const& kHeroStaticStr = akStringImp[28];
DATA(0x005a1044) const char* const& kMonsterStaticStr = akStringImp[29];
DATA(0x005a1040) const char* const& kDestinationStaticStr = akStringImp[30];
}

namespace SMapSpecsLossCondPageText {
namespace {
DATA(0x005a0450) const char* akStringImp[kNumStrings];
}
DATA(0x005a0488) const char* const& kSelectLossConditionStaticStr = akStringImp[0];
DATA(0x005a047c) const char* const& kNoneRadioStr = akStringImp[1];
DATA(0x005a044c) const char* const& kLoseTownRadioStr = akStringImp[2];
DATA(0x005a0470) const char* const& kLoseHeroRadioStr = akStringImp[3];
DATA(0x005a0480) const char* const& kTimeExpiresRadioStr = akStringImp[4];
DATA(0x005a0478) const char* const& kTownStaticStr = akStringImp[5];
DATA(0x005a0474) const char* const& kHeroStaticStr = akStringImp[6];
DATA(0x005a0484) const char* const& kTimeLimitStaticStr = akStringImp[7];
}

namespace SMonsterPropsGeneralPageText {
namespace {
DATA(0x005a12b8) const char* akStringImp[kNumStrings];
}
DATA(0x005a12f4) const char* const& kTypeStaticStr = akStringImp[0];
DATA(0x005a1318) const char* const& kQuantityStaticStr = akStringImp[1];
DATA(0x005a1310) const char* const& kRandomQtyRadioStr = akStringImp[2];
DATA(0x005a1300) const char* const& kCustomQtyRadioStr = akStringImp[3];
DATA(0x005a130c) const char* const& kNeverGrowsCheckStr = akStringImp[4];
DATA(0x005a1304) const char* const& kDispositionStaticStr = akStringImp[5];
DATA(0x005a12f8) const char* const& kDispositionCompliantRadioStr = akStringImp[6];
DATA(0x005a12f0) const char* const& kDispositionFriendlyRadioStr = akStringImp[7];
DATA(0x005a12ec) const char* const& kDispositionAggressiveRadioStr = akStringImp[8];
DATA(0x005a1308) const char* const& kDispositionHostileRadioStr = akStringImp[9];
DATA(0x005a12b4) const char* const& kDispositionSavageRadioStr = akStringImp[10];
DATA(0x005a1314) const char* const& kNeverFleesCheckStr = akStringImp[11];
DATA(0x005a12fc) const char* const& kMessageStaticStr = akStringImp[12];
}

namespace SMonsterPropsTreasurePageText {
namespace {
DATA(0x005a0de0) const char* akStringImp[kNumStrings];
}
DATA(0x005a0dd4) const char* const& kResourcesStaticStr = akStringImp[0];
DATA(0x005a0e04) const char* const& kRes1StaticStr = akStringImp[1];
DATA(0x005a0dd8) const char* const& kRes2StaticStr = akStringImp[2];
DATA(0x005a0e18) const char* const& kRes3StaticStr = akStringImp[3];
DATA(0x005a0e08) const char* const& kRes4StaticStr = akStringImp[4];
DATA(0x005a0ddc) const char* const& kRes5StaticStr = akStringImp[5];
DATA(0x005a0e14) const char* const& kRes6StaticStr = akStringImp[6];
DATA(0x005a0e0c) const char* const& kRes7StaticStr = akStringImp[7];
DATA(0x005a0e10) const char* const& kArtifactStaticStr = akStringImp[8];
}

namespace SNewMapDlgText {
namespace {
DATA(0x005a09bc) const char* akStringImp[kNumStrings];
}
DATA(0x005a09b0) const char* const& kMapSizeStaticStr = akStringImp[0];
DATA(0x005a0a4c) const char* const& k36x36RadioStr = akStringImp[1];
DATA(0x005a099c) const char* const& k72x72RadioStr = akStringImp[2];
DATA(0x005a0a60) const char* const& k108x108RadioStr = akStringImp[3];
DATA(0x005a0a38) const char* const& k144x144RadioStr = akStringImp[4];
DATA(0x005a0a44) const char* const& kTwoLevelMapCheckStr = akStringImp[5];
DATA(0x005a0a24) const char* const& kMapVersionStaticStr = akStringImp[6];
DATA(0x005a0a48) const char* const& kRoERadioStr = akStringImp[7];
DATA(0x005a09ac) const char* const& kABRadioStr = akStringImp[8];
DATA(0x005a0a50) const char* const& kGenerateRandomMapCheckStr = akStringImp[9];
DATA(0x005a0a20) const char* const& kNumberOfPlayersStaticStr = akStringImp[10];
DATA(0x005a0a30) const char* const& kHumanOrComputerStaticStr = akStringImp[11];
DATA(0x005a0a54) const char* const& kComputerOnlyStaticStr = akStringImp[12];
DATA(0x005a0a40) const char* const& kNumberOfTeamsStaticStr = akStringImp[13];
DATA(0x005a0a58) const char* const& kWaterContentStaticStr = akStringImp[14];
DATA(0x005a0a3c) const char* const& kWaterRandomRadioStr = akStringImp[15];
DATA(0x005a09a0) const char* const& kWaterNoneRadioStr = akStringImp[16];
DATA(0x005a0a5c) const char* const& kWaterNormalRadioStr = akStringImp[17];
DATA(0x005a09b8) const char* const& kWaterIslandsRadioStr = akStringImp[18];
DATA(0x005a0a34) const char* const& kMonsterStrengthStaticStr = akStringImp[19];
DATA(0x005a09a8) const char* const& kMonsterRandomRadioStr = akStringImp[20];
DATA(0x005a09b4) const char* const& kMonsterWeakRadioStr = akStringImp[21];
DATA(0x005a0a2c) const char* const& kMonsterNormalRadioStr = akStringImp[22];
DATA(0x005a09a4) const char* const& kMonsterStrongRadioStr = akStringImp[23];
DATA(0x005a0a28) const char* const& kSoDRadioStr = akStringImp[24];
}

namespace SOptionsDlgText {
namespace {
DATA(0x005a1534) const char* akStringImp[kNumStrings];
}
DATA(0x005a1528) const char* const& kTileFrequencyStaticStr = akStringImp[0];
DATA(0x005a155c) const char* const& kNoneStaticStr = akStringImp[1];
DATA(0x005a1564) const char* const& kLotsStaticStr = akStringImp[2];
DATA(0x005a152c) const char* const& kRepaintCheckStr = akStringImp[3];
DATA(0x005a1554) const char* const& kAutosaveOptionsStaticStr = akStringImp[4];
DATA(0x005a1530) const char* const& kEnableAutosaveCheckStr = akStringImp[5];
DATA(0x005a1560) const char* const& kAutosaveEveryStaticStr = akStringImp[6];
DATA(0x005a1558) const char* const& kMinutesStaticStr = akStringImp[7];
}

namespace SResourcePropsGeneralPageText {
namespace {
DATA(0x005a0964) const char* akStringImp[kNumStrings];
}
DATA(0x005a0980) const char* const& kTypeStaticStr = akStringImp[0];
DATA(0x005a0994) const char* const& kQuantityStaticStr = akStringImp[1];
DATA(0x005a0990) const char* const& kRandomQtyRadioStr = akStringImp[2];
DATA(0x005a098c) const char* const& kCustomQtyRadioStr = akStringImp[3];
DATA(0x005a0960) const char* const& kQuantityNoteStaticStr = akStringImp[4];
DATA(0x005a0988) const char* const& kMessageStaticStr = akStringImp[5];
DATA(0x005a0984) const char* const& kNoteStaticStr = akStringImp[6];
}

namespace SScholarPropsDlgText {
namespace {
DATA(0x005a1010) const char* akStringImp[kNumStrings];
}
DATA(0x005a100c) const char* const& kRewardStaticStr = akStringImp[0];
DATA(0x005a1008) const char* const& kRandomRewardRadioStr = akStringImp[1];
DATA(0x005a102c) const char* const& kPriSkillRewardRadioStr = akStringImp[2];
DATA(0x005a1028) const char* const& kSecSkillRewardRadioStr = akStringImp[3];
DATA(0x005a1024) const char* const& kSpellRewardRadioStr = akStringImp[4];
}

namespace SSeersHutPropsDlgText {
namespace {
DATA(0x005a0cb4) const char* akStringImp[kNumStrings];
}
DATA(0x005a0cac) const char* const& kQuestStaticStr = akStringImp[0];
DATA(0x005a0ca0) const char* const& kEditQuestButtonStr = akStringImp[1];
DATA(0x005a0ca8) const char* const& kHasDeadlineCheckStr = akStringImp[2];
DATA(0x005a0cf0) const char* const& kMonthStaticStr = akStringImp[3];
DATA(0x005a0cec) const char* const& kWeekStaticStr = akStringImp[4];
DATA(0x005a0ca4) const char* const& kDayStaticStr = akStringImp[5];
DATA(0x005a0ce0) const char* const& kCustomizeCheckStr = akStringImp[6];
DATA(0x005a0cf4) const char* const& kRewardStaticStr = akStringImp[7];
DATA(0x005a0ce4) const char* const& kEditRewardButtonStr = akStringImp[8];
DATA(0x005a0ce8) const char* const& kBeHeroFmtStr = akStringImp[9];
DATA(0x005a0cb0) const char* const& kBelongToPlayerFmtStr = akStringImp[10];
}

namespace SShrinePropsDlgText {
namespace {
DATA(0x005a049c) const char* akStringImp[kNumStrings];
}
DATA(0x005a0498) const char* const& kSpellStaticFmtStr = akStringImp[0];
DATA(0x005a04ac) const char* const& kRandomSpellRadioStr = akStringImp[1];
DATA(0x005a04a8) const char* const& kCustomSpellRadioStr = akStringImp[2];
}

namespace SSignPropsDlgText {
namespace {
DATA(0x005a0ab4) const char* akStringImp[kNumStrings];
}
DATA(0x005a0ab8) const char* const& kMessageStaticStr = akStringImp[0];
}

namespace STownPropsGeneralPageText {
namespace {
DATA(0x005a0bdc) const char* akStringImp[kNumStrings];
}
DATA(0x005a0c10) const char* const& kTownTypeStaticStr = akStringImp[0];
DATA(0x005a0bd8) const char* const& kPlayerStaticStr = akStringImp[1];
DATA(0x005a0bcc) const char* const& kTownNameStaticStr = akStringImp[2];
DATA(0x005a0c2c) const char* const& kCustomizeCheckStr = akStringImp[3];
DATA(0x005a0c30) const char* const& kVisitingHeroStaticStr = akStringImp[4];
DATA(0x005a0c20) const char* const& kHeroNameStaticStr = akStringImp[5];
DATA(0x005a0bd0) const char* const& kHeroClassStaticStr = akStringImp[6];
DATA(0x005a0bd4) const char* const& kAddButtonStr = akStringImp[7];
DATA(0x005a0c28) const char* const& kEditButtonStr = akStringImp[8];
DATA(0x005a0c14) const char* const& kRemoveButtonStr = akStringImp[9];
DATA(0x005a0c18) const char* const& kAlignmentStaticStr = akStringImp[10];
DATA(0x005a0c24) const char* const& kSameAsTownFmtStr = akStringImp[11];
DATA(0x005a0c1c) const char* const& kSameAsOwnerOrRandomStr = akStringImp[12];
}

namespace STownPropsGarrisonPageText {
namespace {
DATA(0x005a0d88) const char* akStringImp[kNumStrings];
}
DATA(0x005a0d9c) const char* const& kFormationStaticStr = akStringImp[0];
DATA(0x005a0d98) const char* const& kSpreadRadioStr = akStringImp[1];
DATA(0x005a0d94) const char* const& kGroupedRadioStr = akStringImp[2];
}

namespace STownPropsBuildingsPageText {
namespace {
DATA(0x005a0c3c) const char* akStringImp[kNumStrings];
}
DATA(0x005a0c58) const char* const& kHasFortCheckStr = akStringImp[0];
DATA(0x005a0c38) const char* const& kBuildingTreeStaticStr = akStringImp[1];
DATA(0x005a0c64) const char* const& kEnabledCheckStr = akStringImp[2];
DATA(0x005a0c54) const char* const& kBuiltCheckStr = akStringImp[3];
DATA(0x005a0c60) const char* const& kBuildAllButtonStr = akStringImp[4];
DATA(0x005a0c5c) const char* const& kDemolishAllButtonStr = akStringImp[5];
}

namespace STownPropsSpellsPageText {
namespace {
DATA(0x005a114c) const char* akStringImp[kNumStrings];
}
DATA(0x005a1148) const char* const& kSpellLevelStaticStr = akStringImp[0];
DATA(0x005a1158) const char* const& kMustAppearStaticStr = akStringImp[1];
DATA(0x005a115c) const char* const& kMayAppearStaticStr = akStringImp[2];
}

namespace STownPropsTimedEventsPageText {
namespace {
DATA(0x005a0f70) const char* akStringImp[kNumStrings];
}
DATA(0x005a0f9c) const char* const& kEventsStaticStr = akStringImp[0];
DATA(0x005a0f6c) const char* const& kAddButtonStr = akStringImp[1];
DATA(0x005a0fa0) const char* const& kEditButtonStr = akStringImp[2];
DATA(0x005a0f90) const char* const& kRemoveButtonStr = akStringImp[3];
DATA(0x005a0f98) const char* const& kRemoveAllButtonStr = akStringImp[4];
DATA(0x005a0f94) const char* const& kMoveUpButtonStr = akStringImp[5];
DATA(0x005a0f8c) const char* const& kMoveDownButtonStr = akStringImp[6];
}

namespace SEditQuestDlgText {
namespace {
DATA(0x005a071c) const char* akStringImp[kNumStrings];
}
DATA(0x005a07dc) const char* const& kCaptionStr = akStringImp[0];
DATA(0x005a07bc) const char* const& kSelectQuestTypeStaticStr = akStringImp[1];
DATA(0x005a07e0) const char* const& kNoneRadioStr = akStringImp[2];
DATA(0x005a07d0) const char* const& kExperienceLevelRadioStr = akStringImp[3];
DATA(0x005a07b0) const char* const& kPrimarySkillLevelRadioStr = akStringImp[4];
DATA(0x005a06fc) const char* const& kDefeatHeroRadioStr = akStringImp[5];
DATA(0x005a0810) const char* const& kDefeatMonsterRadioStr = akStringImp[6];
DATA(0x005a07b8) const char* const& kArtifactsRadioStr = akStringImp[7];
DATA(0x005a0700) const char* const& kCreaturesRadioStr = akStringImp[8];
DATA(0x005a07e8) const char* const& kResourcesRadioStr = akStringImp[9];
DATA(0x005a07f8) const char* const& kExperienceLevelStaticStr = akStringImp[10];
DATA(0x005a07c8) const char* const& kAttackSkillStaticStr = akStringImp[11];
DATA(0x005a0814) const char* const& kDefenseSkillStaticStr = akStringImp[12];
DATA(0x005a07ac) const char* const& kSpellPowerStaticStr = akStringImp[13];
DATA(0x005a0718) const char* const& kKnowledgeStaticStr = akStringImp[14];
DATA(0x005a0714) const char* const& kHeroStaticStr = akStringImp[15];
DATA(0x005a07c0) const char* const& kMonsterStaticStr = akStringImp[16];
DATA(0x005a07f4) const char* const& kArtifactsStaticStr = akStringImp[17];
DATA(0x005a0710) const char* const& kAddButtonStr = akStringImp[18];
DATA(0x005a0800) const char* const& kEditButtonStr = akStringImp[19];
DATA(0x005a07cc) const char* const& kRemoveButtonStr = akStringImp[20];
DATA(0x005a07e4) const char* const& kRemoveAllButtonStr = akStringImp[21];
DATA(0x005a06f8) const char* const& kCreatureTypeStaticStr = akStringImp[22];
DATA(0x005a0704) const char* const& kQuantityStaticStr = akStringImp[23];
DATA(0x005a07c4) const char* const& kRes0StaticStr = akStringImp[24];
DATA(0x005a0804) const char* const& kRes1StaticStr = akStringImp[25];
DATA(0x005a07ec) const char* const& kRes2StaticStr = akStringImp[26];
DATA(0x005a07fc) const char* const& kRes3StaticStr = akStringImp[27];
DATA(0x005a07d4) const char* const& kRes4StaticStr = akStringImp[28];
DATA(0x005a0708) const char* const& kRes5StaticStr = akStringImp[29];
DATA(0x005a080c) const char* const& kRes6StaticStr = akStringImp[30];
DATA(0x005a070c) const char* const& kBeHeroRadioStr = akStringImp[31];
DATA(0x005a07f0) const char* const& kBelongToPlayerRadioStr = akStringImp[32];
DATA(0x005a07d8) const char* const& kClassStaticStr = akStringImp[33];
DATA(0x005a0808) const char* const& kIdentityStaticStr = akStringImp[34];
DATA(0x005a07b4) const char* const& kPlayerStaticStr = akStringImp[35];
}

namespace SEditRewardDlgText {
namespace {
DATA(0x005a1960) const char* akStringImp[kNumStrings];
}
DATA(0x005a19f0) const char* const& kCaptionStr = akStringImp[0];
DATA(0x005a195c) const char* const& kSelectRewardTypeStaticStr = akStringImp[1];
DATA(0x005a19f4) const char* const& kNoneRadioStr = akStringImp[2];
DATA(0x005a1a14) const char* const& kExperienceRadioStr = akStringImp[3];
DATA(0x005a1950) const char* const& kManaRadioStr = akStringImp[4];
DATA(0x005a19c8) const char* const& kMoraleRadioStr = akStringImp[5];
DATA(0x005a19d4) const char* const& kLuckRadioStr = akStringImp[6];
DATA(0x005a19f8) const char* const& kResourceRadioStr = akStringImp[7];
DATA(0x005a1a04) const char* const& kPrimarySkillRadioStr = akStringImp[8];
DATA(0x005a1a0c) const char* const& kSecondarySkillRadioStr = akStringImp[9];
DATA(0x005a19fc) const char* const& kArtifactRadioStr = akStringImp[10];
DATA(0x005a19d0) const char* const& kSpellRadioStr = akStringImp[11];
DATA(0x005a1954) const char* const& kCreaturesRadioStr = akStringImp[12];
DATA(0x005a19ec) const char* const& kExperienceBonusStaticStr = akStringImp[13];
DATA(0x005a19e4) const char* const& kManaBonusStaticStr = akStringImp[14];
DATA(0x005a1a08) const char* const& kMoraleBonusStaticStr = akStringImp[15];
DATA(0x005a1a10) const char* const& kLuckBonusStaticStr = akStringImp[16];
DATA(0x005a19cc) const char* const& kResourceTypeStaticStr = akStringImp[17];
DATA(0x005a194c) const char* const& kQuantityStaticStr = akStringImp[18];
DATA(0x005a1958) const char* const& kPrimarySkillStaticStr = akStringImp[19];
DATA(0x005a19dc) const char* const& kBonusStaticStr = akStringImp[20];
DATA(0x005a19e8) const char* const& kSecondarySkillStaticStr = akStringImp[21];
DATA(0x005a19e0) const char* const& kMasteryStaticStr = akStringImp[22];
DATA(0x005a1a00) const char* const& kArtifactStaticStr = akStringImp[23];
DATA(0x005a19d8) const char* const& kSpellStaticStr = akStringImp[24];
DATA(0x005a1948) const char* const& kCreatureTypeStaticStr = akStringImp[25];
}

namespace SHeroPlaceholderPropsDlgText {
namespace {
DATA(0x005a14f4) const char* akStringImp[kNumStrings];
}
DATA(0x005a14f0) const char* const& kOwnerStaticStr = akStringImp[0];
DATA(0x005a151c) const char* const& kIdentityStaticStr = akStringImp[1];
DATA(0x005a1510) const char* const& kAnyHeroRadioStr = akStringImp[2];
DATA(0x005a1520) const char* const& kSpecificHeroRadioStr = akStringImp[3];
DATA(0x005a1514) const char* const& kPowerRatingStaticStr = akStringImp[4];
DATA(0x005a1518) const char* const& kClassStaticStr = akStringImp[5];
DATA(0x005a1524) const char* const& kHeroIdentityStaticStr = akStringImp[6];
}

namespace SHeroPropsBiographyPageText {
namespace {
DATA(0x005a17c0) const char* akStringImp[kNumStrings];
}
DATA(0x005a17c8) const char* const& kCaptionStr = akStringImp[0];
DATA(0x005a17cc) const char* const& kCustomizeCheckStr = akStringImp[1];
}

namespace SMapSpecsArtifactsPageText {
namespace {
DATA(0x005a0b98) const char* akStringImp[kNumStrings];
}
DATA(0x005a0b9c) const char* const& kEnabledArtifactsStaticStr = akStringImp[0];
}

namespace SMapSpecsAvailableHeroesPageText {
namespace {
DATA(0x0059fef8) const char* akStringImp[kNumStrings];
}
DATA(0x0059ff04) const char* const& kAvailableHeroesStaticStr = akStringImp[0];
DATA(0x0059ff00) const char* const& kPropertiesButtonStr = akStringImp[1];
}

namespace SRandomGeneratorPropsDlgText {
namespace {
DATA(0x005a064c) const char* akStringImp[kNumStrings];
}
DATA(0x005a0648) const char* const& kOwnerStaticStr = akStringImp[0];
DATA(0x005a06b4) const char* const& kCreatureLevelStaticStr = akStringImp[1];
DATA(0x005a0694) const char* const& kMinimumStaticStr = akStringImp[2];
DATA(0x005a0644) const char* const& kMaximumStaticStr = akStringImp[3];
DATA(0x005a06a0) const char* const& kAlignmentStaticStr = akStringImp[4];
DATA(0x005a06a8) const char* const& kSelectOneOfRadioStr = akStringImp[5];
DATA(0x005a06b8) const char* const& kSameAsRadioStr = akStringImp[6];
DATA(0x005a06b0) const char* const& kCastleCheckStr = akStringImp[7];
DATA(0x005a0640) const char* const& kRampartCheckStr = akStringImp[8];
DATA(0x005a063c) const char* const& kTowerCheckStr = akStringImp[9];
DATA(0x005a068c) const char* const& kInfernoCheckStr = akStringImp[10];
DATA(0x005a0690) const char* const& kNecropolisCheckStr = akStringImp[11];
DATA(0x005a069c) const char* const& kDungeonCheckStr = akStringImp[12];
DATA(0x005a0698) const char* const& kStrongholdCheckStr = akStringImp[13];
DATA(0x005a06a4) const char* const& kFortressCheckStr = akStringImp[14];
DATA(0x005a06ac) const char* const& kConfluxCheckStr = akStringImp[15];
}

namespace SWitchHutPropsDlgText {
namespace {
DATA(0x005a0a8c) const char* akStringImp[kNumStrings];
}
DATA(0x005a0a90) const char* const& kPotentialSkillsStaticStr = akStringImp[0];
}

namespace SHeroPropsPriSkillsPageText {
namespace {
DATA(0x005a0590) const char* akStringImp[kNumStrings];
}
DATA(0x005a0598) const char* const& kCaptionStr = akStringImp[0];
DATA(0x005a059c) const char* const& kCustomizeCheckStr = akStringImp[1];
}

namespace SHeroPropsSpellsPageText {
namespace {
DATA(0x005a1180) const char* akStringImp[kNumStrings];
}
DATA(0x005a118c) const char* const& kCaptionStr = akStringImp[0];
DATA(0x005a1188) const char* const& kCustomizeCheckStr = akStringImp[1];
}

namespace SHeroPrototypePropsArtifactsPageText {
namespace {
DATA(0x005a1814) const char* akStringImp[kNumStrings];
}
DATA(0x005a1818) const char* const& kCaptionStr = akStringImp[0];
}

namespace SHeroPrototypePropsBiographyPageText {
namespace {
DATA(0x005a125c) const char* akStringImp[kNumStrings];
}
DATA(0x005a1260) const char* const& kCaptionStr = akStringImp[0];
}

namespace SHeroPrototypePropsGeneralPageText {
namespace {
DATA(0x005a0b00) const char* akStringImp[kNumStrings];
}
DATA(0x005a0b10) const char* const& kCaptionStr = akStringImp[0];
DATA(0x005a0b0c) const char* const& kMayBeHiredByStaticStr = akStringImp[1];
DATA(0x005a0b14) const char* const& kPlayerCheckFmtStr = akStringImp[2];
}

namespace SHeroPrototypePropsPriSkillsPageText {
namespace {
DATA(0x005a1384) const char* akStringImp[kNumStrings];
}
DATA(0x005a1388) const char* const& kCaptionStr = akStringImp[0];
}

namespace SHeroPrototypePropsSecSkillsPageText {
namespace {
DATA(0x005a0fac) const char* akStringImp[kNumStrings];
}
DATA(0x005a0fb0) const char* const& kCaptionStr = akStringImp[0];
}

namespace SHeroPrototypePropsSheetText {
namespace {
DATA(0x005a0ef8) const char* akStringImp[kNumStrings];
}
DATA(0x005a0efc) const char* const& kCaptionStr = akStringImp[0];
}

namespace SHeroPrototypePropsSpellsPageText {
namespace {
DATA(0x005a113c) const char* akStringImp[kNumStrings];
}
DATA(0x005a1140) const char* const& kCaptionStr = akStringImp[0];
}

namespace SPrimarySkillsDlgText {
namespace {
DATA(0x005a0430) const char* akStringImp[kNumStrings];
}
DATA(0x005a0444) const char* const& kAttackSkillStaticStr = akStringImp[0];
DATA(0x005a0448) const char* const& kDefenseSkillStaticStr = akStringImp[1];
DATA(0x005a0440) const char* const& kSpellPowerStaticStr = akStringImp[2];
DATA(0x005a042c) const char* const& kKnowledgeStaticStr = akStringImp[3];
}

namespace SSpellsDlgText {
namespace {
DATA(0x005a0fcc) const char* akStringImp[kNumStrings];
}
DATA(0x005a0fc8) const char* const& kSpellsStaticStr = akStringImp[0];
}

namespace SMapSpecsSecSkillsPageText {
namespace {
DATA(0x005a0f40) const char* akStringImp[kNumStrings];
}
DATA(0x005a0f48) const char* const& kCaptionStr = akStringImp[0];
DATA(0x005a0f4c) const char* const& kAvailableSecSkillsStaticStr = akStringImp[1];
}

namespace SMapSpecsSpellsPageText {
namespace {
DATA(0x005a06e8) const char* akStringImp[kNumStrings];
}
DATA(0x005a06f0) const char* const& kCaptionStr = akStringImp[0];
DATA(0x005a06f4) const char* const& kAvailableSpellsStaticStr = akStringImp[1];
}

namespace SMainMenuText {
namespace {
DATA(0x005a0564) const char* akStringImp[kNumStrings];
}
DATA(0x005a055c) const char* const& kFileStr = akStringImp[0];
DATA(0x005a057c) const char* const& kEditStr = akStringImp[1];
DATA(0x005a0580) const char* const& kViewStr = akStringImp[2];
DATA(0x005a0584) const char* const& kToolsStr = akStringImp[3];
DATA(0x005a0560) const char* const& kPlayerStr = akStringImp[4];
DATA(0x005a0558) const char* const& kHelpStr = akStringImp[5];
}

namespace SFileMenuText {
namespace {
DATA(0x005a1468) const char* akStringImp[kNumStrings];
}
DATA(0x005a1464) const char* const& kNewStr = akStringImp[0];
DATA(0x005a1488) const char* const& kOpenStr = akStringImp[1];
DATA(0x005a1498) const char* const& kSaveStr = akStringImp[2];
DATA(0x005a149c) const char* const& kSaveAsStr = akStringImp[3];
DATA(0x005a1494) const char* const& kExportTextStr = akStringImp[4];
DATA(0x005a14a0) const char* const& kImportTextStr = akStringImp[5];
DATA(0x005a1490) const char* const& kRecentFileStr = akStringImp[6];
DATA(0x005a148c) const char* const& kExitStr = akStringImp[7];
}

namespace SEditMenuText {
namespace {
DATA(0x005a18bc) const char* akStringImp[kNumStrings];
}
DATA(0x005a18e4) const char* const& kUndoStr = akStringImp[0];
DATA(0x005a18b4) const char* const& kRedoStr = akStringImp[1];
DATA(0x005a18e8) const char* const& kCutStr = akStringImp[2];
DATA(0x005a18f4) const char* const& kCopyStr = akStringImp[3];
DATA(0x005a18f0) const char* const& kPasteStr = akStringImp[4];
DATA(0x005a18ec) const char* const& kDeleteStr = akStringImp[5];
DATA(0x005a18b0) const char* const& kFindStr = akStringImp[6];
DATA(0x005a18b8) const char* const& kFindNextStr = akStringImp[7];
DATA(0x005a18ac) const char* const& kFindPrevStr = akStringImp[8];
DATA(0x005a18f8) const char* const& kPropertiesStr = akStringImp[9];
}

namespace SViewMenuText {
namespace {
DATA(0x005a0e80) const char* akStringImp[kNumStrings];
}
DATA(0x005a0eac) const char* const& kZoomInStr = akStringImp[0];
DATA(0x005a0ea8) const char* const& kZoomOutStr = akStringImp[1];
DATA(0x005a0e78) const char* const& kUndergroundStr = akStringImp[2];
DATA(0x005a0e74) const char* const& kGridStr = akStringImp[3];
DATA(0x005a0eb0) const char* const& kPassabilityStr = akStringImp[4];
DATA(0x005a0ebc) const char* const& kObjectAnimationStr = akStringImp[5];
DATA(0x005a0ec0) const char* const& kTerrainAnimationStr = akStringImp[6];
DATA(0x005a0eb8) const char* const& kToolbarStr = akStringImp[7];
DATA(0x005a0eb4) const char* const& kModeBarStr = akStringImp[8];
DATA(0x005a0e7c) const char* const& kStatusBarStr = akStringImp[9];
}

namespace SToolsMenuText {
namespace {
DATA(0x0059ff34) const char* akStringImp[kNumStrings];
}
DATA(0x0059ff68) const char* const& kTerrainStr = akStringImp[0];
DATA(0x0059ff64) const char* const& kRiversStr = akStringImp[1];
DATA(0x0059ff70) const char* const& kRoadsStr = akStringImp[2];
DATA(0x0059ff58) const char* const& kEraseStr = akStringImp[3];
DATA(0x0059ff6c) const char* const& kObjectsStr = akStringImp[4];
DATA(0x0059ff2c) const char* const& kMapSpecificationsStr = akStringImp[5];
DATA(0x0059ff5c) const char* const& kValidateMapStr = akStringImp[6];
DATA(0x0059ff60) const char* const& kOptionsStr = akStringImp[7];
DATA(0x0059ff30) const char* const& kObstaclesStr = akStringImp[8];
}

namespace SToolsTerrainMenuText {
namespace {
DATA(0x005a158c) const char* akStringImp[kNumStrings];
}
DATA(0x005a15d8) const char* const& k1x1Str = akStringImp[0];
DATA(0x005a1578) const char* const& k2x2Str = akStringImp[1];
DATA(0x005a15e4) const char* const& k4x4Str = akStringImp[2];
DATA(0x005a157c) const char* const& kFillStr = akStringImp[3];
DATA(0x005a1584) const char* const& kDirtStr = akStringImp[4];
DATA(0x005a15d4) const char* const& kSandStr = akStringImp[5];
DATA(0x005a15dc) const char* const& kGrassStr = akStringImp[6];
DATA(0x005a15cc) const char* const& kSnowStr = akStringImp[7];
DATA(0x005a15d0) const char* const& kSwampStr = akStringImp[8];
DATA(0x005a1588) const char* const& kRoughStr = akStringImp[9];
DATA(0x005a15c8) const char* const& kSubterraneanStr = akStringImp[10];
DATA(0x005a15e0) const char* const& kLavaStr = akStringImp[11];
DATA(0x005a1580) const char* const& kWaterStr = akStringImp[12];
DATA(0x005a15c4) const char* const& kRockStr = akStringImp[13];
}

namespace SToolsRiversMenuText {
namespace {
DATA(0x005a04e0) const char* akStringImp[kNumStrings];
}
DATA(0x005a04fc) const char* const& kClearStr = akStringImp[0];
DATA(0x005a04dc) const char* const& kIcyStr = akStringImp[1];
DATA(0x005a04f4) const char* const& kMuddyStr = akStringImp[2];
DATA(0x005a0500) const char* const& kLavaStr = akStringImp[3];
DATA(0x005a04f8) const char* const& kEraserStr = akStringImp[4];
}

namespace SToolsRoadsMenuText {
namespace {
DATA(0x005a0414) const char* akStringImp[kNumStrings];
}
DATA(0x005a0410) const char* const& kDirtStr = akStringImp[0];
DATA(0x005a0424) const char* const& kGravelStr = akStringImp[1];
DATA(0x005a040c) const char* const& kCobblestoneStr = akStringImp[2];
DATA(0x005a0428) const char* const& kEraserStr = akStringImp[3];
}

namespace SToolsEraseMenuText {
namespace {
DATA(0x005a0c7c) const char* akStringImp[kNumStrings];
}
DATA(0x005a0c8c) const char* const& k1x1Str = akStringImp[0];
DATA(0x005a0c74) const char* const& k2x2Str = akStringImp[1];
DATA(0x005a0c90) const char* const& k4x4Str = akStringImp[2];
DATA(0x005a0c78) const char* const& kFillStr = akStringImp[3];
}

namespace SToolsObjectsMenuText {
namespace {
DATA(0x005a0b24) const char* akStringImp[kNumStrings];
}
DATA(0x005a0b90) const char* const& kDirtObjectsStr = akStringImp[0];
DATA(0x005a0b20) const char* const& kSandObjectsStr = akStringImp[1];
DATA(0x005a0b6c) const char* const& kGrassObjectsStr = akStringImp[2];
DATA(0x005a0b78) const char* const& kSnowObjectsStr = akStringImp[3];
DATA(0x005a0b88) const char* const& kSwampObjectsStr = akStringImp[4];
DATA(0x005a0b8c) const char* const& kRoughObjectsStr = akStringImp[5];
DATA(0x005a0b60) const char* const& kSubterraneanObjectsStr = akStringImp[6];
DATA(0x005a0b64) const char* const& kLavaObjectsStr = akStringImp[7];
DATA(0x005a0b80) const char* const& kWaterObjectsStr = akStringImp[8];
DATA(0x005a0b70) const char* const& kAllTerrainObjectsStr = akStringImp[9];
DATA(0x005a0b74) const char* const& kTownsStr = akStringImp[10];
DATA(0x005a0b68) const char* const& kMonstersStr = akStringImp[11];
DATA(0x005a0b7c) const char* const& kHeroesStr = akStringImp[12];
DATA(0x005a0b94) const char* const& kArtifactsStr = akStringImp[13];
DATA(0x005a0b84) const char* const& kTreasuresStr = akStringImp[14];
}

namespace SPlayerMenuText {
namespace {
DATA(0x005a0e20) const char* akStringImp[kNumStrings];
}
DATA(0x005a0e48) const char* const& kNoneStr = akStringImp[0];
DATA(0x005a0e58) const char* const& kPlayer1FmtStr = akStringImp[1];
DATA(0x005a0e60) const char* const& kPlayer2FmtStr = akStringImp[2];
DATA(0x005a0e5c) const char* const& kPlayer3FmtStr = akStringImp[3];
DATA(0x005a0e64) const char* const& kPlayer4FmtStr = akStringImp[4];
DATA(0x005a0e54) const char* const& kPlayer5FmtStr = akStringImp[5];
DATA(0x005a0e50) const char* const& kPlayer6FmtStr = akStringImp[6];
DATA(0x005a0e4c) const char* const& kPlayer7FmtStr = akStringImp[7];
DATA(0x005a0e44) const char* const& kPlayer8FmtStr = akStringImp[8];
}

namespace SHelpMenuText {
namespace {
DATA(0x0059fee4) const char* akStringImp[kNumStrings];
}
DATA(0x0059fef0) const char* const& kHelpTopicsStr = akStringImp[0];
DATA(0x0059feec) const char* const& kAboutMapEditorStr = akStringImp[1];
}

namespace SContextMenuText {
namespace {
DATA(0x005a1a70) const char* akStringImp[kNumStrings];
}
DATA(0x005a1ab0) const char* const& kWhatsThisStr = akStringImp[0];
DATA(0x005a1aa0) const char* const& kUndoStr = akStringImp[1];
DATA(0x005a1a68) const char* const& kRedoStr = akStringImp[2];
DATA(0x005a1aa8) const char* const& kCutStr = akStringImp[3];
DATA(0x005a1ab8) const char* const& kCopyStr = akStringImp[4];
DATA(0x005a1ab4) const char* const& kPasteStr = akStringImp[5];
DATA(0x005a1aac) const char* const& kDeleteStr = akStringImp[6];
DATA(0x005a1a64) const char* const& kFindStr = akStringImp[7];
DATA(0x005a1a6c) const char* const& kFindNextStr = akStringImp[8];
DATA(0x005a1a60) const char* const& kFindPrevStr = akStringImp[9];
DATA(0x005a1abc) const char* const& kPropertiesStr = akStringImp[10];
DATA(0x005a1aa4) const char* const& kPlaceObstaclesStr = akStringImp[11];
}

namespace SToolsBrushMenuText {
namespace {
DATA(0x005a0880) const char* akStringImp[kNumStrings];
}
DATA(0x005a08ec) const char* const& k1x1SolidBrushStr = akStringImp[0];
DATA(0x005a08f0) const char* const& k2x2SolidBrushStr = akStringImp[1];
DATA(0x005a08e8) const char* const& k4x4SolidBrushStr = akStringImp[2];
DATA(0x005a08f4) const char* const& kSolidAreaBrushStr = akStringImp[3];
DATA(0x005a08c4) const char* const& k1x1FuzzyBrushStr = akStringImp[4];
DATA(0x005a08c8) const char* const& k2x2FuzzyBrushStr = akStringImp[5];
DATA(0x005a08dc) const char* const& k4x4FuzzyBrushStr = akStringImp[6];
DATA(0x005a08d0) const char* const& kFuzzyAreaBrushStr = akStringImp[7];
DATA(0x005a0878) const char* const& k1x1SolidEraserStr = akStringImp[8];
DATA(0x005a087c) const char* const& k2x2SolidEraserStr = akStringImp[9];
DATA(0x005a08d4) const char* const& k4x4SolidEraserStr = akStringImp[10];
DATA(0x005a0874) const char* const& kSolidAreaEraserStr = akStringImp[11];
DATA(0x005a08d8) const char* const& k1x1FuzzyEraserStr = akStringImp[12];
DATA(0x005a08e4) const char* const& k2x2FuzzyEraserStr = akStringImp[13];
DATA(0x005a08e0) const char* const& k4x4FuzzyEraserStr = akStringImp[14];
DATA(0x005a08f8) const char* const& kFuzzyAreaEraserStr = akStringImp[15];
DATA(0x005a08cc) const char* const& kPlaceObstaclesStr = akStringImp[16];
}

// Copies the texts of a group, after its heading line, into a buffer of
// the group's own and points the group's strings at the copies.
#define INITIALIZE_TEXT(Group)                                                      \
    {                                                                               \
        baseID++;                                                                   \
        assert(pTextResource->GetNumberOfStrings() - baseID >= Group::kNumStrings); \
        size_t totalLength = 0;                                                     \
        unsigned int i;                                                             \
        for (i = 0; i < Group::kNumStrings; i++)                                    \
            totalLength += stringLength(pTextResource->GetText(baseID + i)) + 1;    \
        static TAutoArrayPtr<char> apBuffer(new char[totalLength]);                 \
        if (apBuffer.get() == NULL)                                                     \
            throw TAllocationFailure();                                             \
        char* pDest = apBuffer.get();                                               \
        for (i = 0; i < Group::kNumStrings; i++) {                                  \
            const char* pSrc = pTextResource->GetText(baseID + i);                  \
            size_t length = stringLength(pSrc) + 1;                                 \
            stringCopy(pDest, pSrc);                                                \
            Group::akStringImp[i] = pDest;                                          \
            pDest += length;                                                        \
        }                                                                           \
        baseID += Group::kNumStrings;                                               \
    }

// Every group's apBuffer registers its own teardown with atexit. Retail
// places the 78 destructors in reverse registration order, 0x4690d0 for the
// general strings down to 0x468a32 for the brush menu; each group's claim
// precedes its static, in that registration order.
VA(0x00464db2, 0x3c73)
void SText::initialize()
{
    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("editor.txt"));
    if (!pTextResource.get())
        throw TRuntimeError();

    unsigned int baseID = 2;
    {
        assert(pTextResource->GetNumberOfStrings() - baseID >= kNumGeneralStrings);
        size_t totalLength = 0;
        unsigned int i;
        for (i = 0; i < kNumGeneralStrings; i++)
            totalLength += stringLength(pTextResource->GetText(baseID + i)) + 1;
        VA_COMPGEN(0x004690d0, 0x16, STATIC_DTOR, apBuffer)
        static TAutoArrayPtr<char> apBuffer(new char[totalLength]);
        if (apBuffer.get() == NULL)
            throw TAllocationFailure();
        char* pDest = apBuffer.get();
        for (i = 0; i < kNumGeneralStrings; i++) {
            const char* pSrc = pTextResource->GetText(baseID + i);
            size_t length = stringLength(pSrc) + 1;
            stringCopy(pDest, pSrc);
            akGeneralStringImp[i] = pDest;
            pDest += length;
        }
        baseID += kNumGeneralStrings;
    }
    VA_COMPGEN(0x004690ba, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SAbandonedMinePropsDlgText)
    VA_COMPGEN(0x004690a4, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SArmyDlgText)
    VA_COMPGEN(0x0046908e, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SGarrisonPropertiesDlgText)
    VA_COMPGEN(0x00469078, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SArtifactPropsGeneralPageText)
    VA_COMPGEN(0x00469062, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SBlackBoxPropsGeneralPageText)
    VA_COMPGEN(0x0046904c, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SEventPropsGeneralPageText)
    VA_COMPGEN(0x00469036, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SBlackBoxPropsContentsPageText)
    VA_COMPGEN(0x00469020, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SResourceQuantitiesDlgText)
    VA_COMPGEN(0x0046900a, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SEditSecondarySkillDlgText)
    VA_COMPGEN(0x00468ff4, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SEditCreatureStackDlgText)
    VA_COMPGEN(0x00468fde, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SEditTimedEventGeneralPageText)
    VA_COMPGEN(0x00468fc8, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SEditTownEventBuildingsPageText)
    VA_COMPGEN(0x00468fb2, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SEditTownEventCreaturesPageText)
    VA_COMPGEN(0x00468f9c, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SEditArtifactDlgText)
    VA_COMPGEN(0x00468f86, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SEditRumorDlgText)
    VA_COMPGEN(0x00468f70, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SFindDlgText)
    VA_COMPGEN(0x00468f5a, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SFlaggablePropsDlgText)
    VA_COMPGEN(0x00468f44, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPropsGeneralPageText)
    VA_COMPGEN(0x00468f2e, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPropsCreaturesPageText)
    VA_COMPGEN(0x00468f18, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPropsSecSkillsPageText)
    VA_COMPGEN(0x00468f02, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPropsArtifactsPageText)
    VA_COMPGEN(0x00468eec, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMapSpecsGeneralPageText)
    VA_COMPGEN(0x00468ed6, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMapSpecsPlayerSpecsPageText)
    VA_COMPGEN(0x00468ec0, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMapSpecsTeamsPageText)
    VA_COMPGEN(0x00468eaa, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMapSpecsRumorsPageText)
    VA_COMPGEN(0x00468e94, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMapSpecsTimedEventsPageText)
    VA_COMPGEN(0x00468e7e, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMapSpecsVictoryCondPageText)
    VA_COMPGEN(0x00468e68, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMapSpecsLossCondPageText)
    VA_COMPGEN(0x00468e52, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMonsterPropsGeneralPageText)
    VA_COMPGEN(0x00468e3c, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMonsterPropsTreasurePageText)
    VA_COMPGEN(0x00468e26, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SNewMapDlgText)
    VA_COMPGEN(0x00468e10, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SOptionsDlgText)
    VA_COMPGEN(0x00468dfa, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SResourcePropsGeneralPageText)
    VA_COMPGEN(0x00468de4, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SScholarPropsDlgText)
    VA_COMPGEN(0x00468dce, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SSeersHutPropsDlgText)
    VA_COMPGEN(0x00468db8, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SShrinePropsDlgText)
    VA_COMPGEN(0x00468da2, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SSignPropsDlgText)
    VA_COMPGEN(0x00468d8c, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(STownPropsGeneralPageText)
    VA_COMPGEN(0x00468d76, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(STownPropsGarrisonPageText)
    VA_COMPGEN(0x00468d60, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(STownPropsBuildingsPageText)
    VA_COMPGEN(0x00468d4a, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(STownPropsSpellsPageText)
    VA_COMPGEN(0x00468d34, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(STownPropsTimedEventsPageText)
    VA_COMPGEN(0x00468d1e, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SEditQuestDlgText)
    VA_COMPGEN(0x00468d08, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SEditRewardDlgText)
    VA_COMPGEN(0x00468cf2, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPlaceholderPropsDlgText)
    VA_COMPGEN(0x00468cdc, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPropsBiographyPageText)
    VA_COMPGEN(0x00468cc6, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMapSpecsArtifactsPageText)
    VA_COMPGEN(0x00468cb0, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMapSpecsAvailableHeroesPageText)
    VA_COMPGEN(0x00468c9a, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SRandomGeneratorPropsDlgText)
    VA_COMPGEN(0x00468c84, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SWitchHutPropsDlgText)
    VA_COMPGEN(0x00468c6e, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPropsPriSkillsPageText)
    VA_COMPGEN(0x00468c58, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPropsSpellsPageText)
    VA_COMPGEN(0x00468c42, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPrototypePropsArtifactsPageText)
    VA_COMPGEN(0x00468c2c, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPrototypePropsBiographyPageText)
    VA_COMPGEN(0x00468c16, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPrototypePropsGeneralPageText)
    VA_COMPGEN(0x00468c00, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPrototypePropsPriSkillsPageText)
    VA_COMPGEN(0x00468bea, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPrototypePropsSecSkillsPageText)
    VA_COMPGEN(0x00468bd4, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPrototypePropsSheetText)
    VA_COMPGEN(0x00468bbe, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHeroPrototypePropsSpellsPageText)
    VA_COMPGEN(0x00468ba8, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SPrimarySkillsDlgText)
    VA_COMPGEN(0x00468b92, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SSpellsDlgText)
    VA_COMPGEN(0x00468b7c, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMapSpecsSecSkillsPageText)
    VA_COMPGEN(0x00468b66, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMapSpecsSpellsPageText)
    VA_COMPGEN(0x00468b50, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SMainMenuText)
    VA_COMPGEN(0x00468b3a, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SFileMenuText)
    VA_COMPGEN(0x00468b24, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SEditMenuText)
    VA_COMPGEN(0x00468b0e, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SViewMenuText)
    VA_COMPGEN(0x00468af8, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SToolsMenuText)
    VA_COMPGEN(0x00468ae2, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SToolsTerrainMenuText)
    VA_COMPGEN(0x00468acc, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SToolsRiversMenuText)
    VA_COMPGEN(0x00468ab6, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SToolsRoadsMenuText)
    VA_COMPGEN(0x00468aa0, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SToolsEraseMenuText)
    VA_COMPGEN(0x00468a8a, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SToolsObjectsMenuText)
    VA_COMPGEN(0x00468a74, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SPlayerMenuText)
    VA_COMPGEN(0x00468a5e, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SHelpMenuText)
    VA_COMPGEN(0x00468a48, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SContextMenuText)
    VA_COMPGEN(0x00468a32, 0x16, STATIC_DTOR, apBuffer)
    INITIALIZE_TEXT(SToolsBrushMenuText)
}

#undef INITIALIZE_TEXT
