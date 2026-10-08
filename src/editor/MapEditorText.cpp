// MapEditorText.cpp - Loki h3maped object 18: the editor's text strings.
// SText::initialize copies each group of editor.txt into one buffer per
// group (a function-local static TAutoArrayPtr<char>), expanding the
// ellipsis character 0x85 to "...", and points the group's string table
// into it. Assert and throw lines come from the retail immediates; every
// group after the general one is one line, as a macro's expansion.
#include <assert.h>
#include <string.h>
#include <string>

#include "exceptions.h"
#include "autoarrayptr.h"
#include "resourcemanager.h"
#include "resourceptr.h"
#include "textresource.h"
#include "editor/MapEditorText.h"

namespace {

size_t stringLength(const char* str)
{
    size_t length = 0;
    for (;;) {
        size_t span = strcspn(str, "\x85");
        length += span;
        str += span;
        if (*str == '\0')
            break;
#line 50
        assert(*str == '\x85');
        str++;
        length += 3;
    }
    return length;
}

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

bool bInitialized = false;

const char* akGeneralStringImp[kNumGeneralStrings];

}  // namespace

const char* const& kRegistryKeyStr = akGeneralStringImp[0];
const char* const& kAppTitleStr = akGeneralStringImp[1];
const char* const& kCopyrightStr = akGeneralStringImp[2];
const char* const& kVersionFmtStr = akGeneralStringImp[3];
const char* const& kMapFileFilterStr = akGeneralStringImp[4];
const char* const& kMapFileRegNameStr = akGeneralStringImp[5];
const char* const& kMapSpecsSheetCaptionStr = akGeneralStringImp[6];
const char* const& kObjectPropertiesCaptionFmtStr = akGeneralStringImp[7];
const char* const& kEditTimedEventSheetCaptionStr = akGeneralStringImp[8];
const char* const& kSelNoneStr = akGeneralStringImp[9];
const char* const& kRandomStr = akGeneralStringImp[10];
const char* const& kUnknownStr = akGeneralStringImp[11];
const char* const& kBackpackStr = akGeneralStringImp[12];
const char* const& kExperienceStr = akGeneralStringImp[13];
const char* const& kManaStr = akGeneralStringImp[14];
const char* const& kMoraleStr = akGeneralStringImp[15];
const char* const& kLuckStr = akGeneralStringImp[16];
const char* const& kResourceStr = akGeneralStringImp[17];
const char* const& kResourcesStr = akGeneralStringImp[18];
const char* const& kPrimarySkillStr = akGeneralStringImp[19];
const char* const& kPrimarySkillsStr = akGeneralStringImp[20];
const char* const& kArtifactStr = akGeneralStringImp[21];
const char* const& kArtifactsStr = akGeneralStringImp[22];
const char* const& kSpellStr = akGeneralStringImp[23];
const char* const& kSpellsStr = akGeneralStringImp[24];
const char* const& kCreatureStr = akGeneralStringImp[25];
const char* const& kCreaturesStr = akGeneralStringImp[26];
const char* const& kSecondarySkillStr = akGeneralStringImp[27];
const char* const& kSecondarySkillsStr = akGeneralStringImp[28];
const char* const& kPlayerNameFmtStr = akGeneralStringImp[29];
const char* const& kPlayerNoneStr = akGeneralStringImp[30];
const char* const& kNeverStr = akGeneralStringImp[31];
const char* const& kEveryDayStr = akGeneralStringImp[32];
const char* const& kEvery2DaysStr = akGeneralStringImp[33];
const char* const& kEvery3DaysStr = akGeneralStringImp[34];
const char* const& kEvery4DaysStr = akGeneralStringImp[35];
const char* const& kEvery5DaysStr = akGeneralStringImp[36];
const char* const& kEvery6DaysStr = akGeneralStringImp[37];
const char* const& kEvery7DaysStr = akGeneralStringImp[38];
const char* const& kEvery14DaysStr = akGeneralStringImp[39];
const char* const& kEvery21DaysStr = akGeneralStringImp[40];
const char* const& kEvery28DaysStr = akGeneralStringImp[41];
const char* const& kMaxObjectsOfTypeFmtStr = akGeneralStringImp[42];
const char* const& kNoMoreClassHeroesStr = akGeneralStringImp[43];
const char* const& kSelectPlayerBeforePlacingHeroStr = akGeneralStringImp[44];
const char* const& kMaxHeroesPerPlayerFmtStr = akGeneralStringImp[45];
const char* const& kGrailAlreadyPlacedStr = akGeneralStringImp[46];
const char* const& kGrailPlacedTooCloseToEdgeFmtStr = akGeneralStringImp[47];
const char* const& kNowhereToPasteStr = akGeneralStringImp[48];
const char* const& kDebugBuildStr = akGeneralStringImp[49];
const char* const& kReleaseBuildStr = akGeneralStringImp[50];
const char* const& kNeed16BitColorStr = akGeneralStringImp[51];
const char* const& kInvalidMapVersionFmtStr = akGeneralStringImp[52];
const char* const& kInvalidMapFileStr = akGeneralStringImp[53];
const char* const& k2DaysStr = akGeneralStringImp[54];
const char* const& k3DaysStr = akGeneralStringImp[55];
const char* const& k4DaysStr = akGeneralStringImp[56];
const char* const& k5DaysStr = akGeneralStringImp[57];
const char* const& k6DaysStr = akGeneralStringImp[58];
const char* const& k1WeekStr = akGeneralStringImp[59];
const char* const& k2WeeksStr = akGeneralStringImp[60];
const char* const& k3WeeksStr = akGeneralStringImp[61];
const char* const& k4WeeksStr = akGeneralStringImp[62];
const char* const& k5WeeksStr = akGeneralStringImp[63];
const char* const& k6WeeksStr = akGeneralStringImp[64];
const char* const& k7WeeksStr = akGeneralStringImp[65];
const char* const& k2MonthsStr = akGeneralStringImp[66];
const char* const& k3MonthsStr = akGeneralStringImp[67];
const char* const& k4MonthsStr = akGeneralStringImp[68];
const char* const& k5MonthsStr = akGeneralStringImp[69];
const char* const& k6MonthsStr = akGeneralStringImp[70];
const char* const& k7MonthsStr = akGeneralStringImp[71];
const char* const& k8MonthsStr = akGeneralStringImp[72];
const char* const& k9MonthsStr = akGeneralStringImp[73];
const char* const& k10MonthsStr = akGeneralStringImp[74];
const char* const& k11MonthsStr = akGeneralStringImp[75];
const char* const& k12MonthsStr = akGeneralStringImp[76];
const char* const& kAnyTownStr = akGeneralStringImp[77];
const char* const& kObjectAtLocationFmtStr = akGeneralStringImp[78];
const char* const& kSpecificHeroAndClassFmtStr = akGeneralStringImp[79];
const char* const& kRandomMonsterStr = akGeneralStringImp[80];
const char* const& kRandomMonstersStr = akGeneralStringImp[81];
const char* const& kWarriorStr = akGeneralStringImp[82];
const char* const& kBuilderStr = akGeneralStringImp[83];
const char* const& kExplorerStr = akGeneralStringImp[84];
const char* const& kWarningStr = akGeneralStringImp[85];
const char* const& kHeroesNeedOwnerStr = akGeneralStringImp[86];
const char* const& kPlayerHasMaxHeroesStr = akGeneralStringImp[87];
const char* const& kContinuingWillDeleteHeroFmtStr = akGeneralStringImp[88];
const char* const& kNoMoreHeroesStr = akGeneralStringImp[89];
const char* const& kNoPlayersOnMapStr = akGeneralStringImp[90];
const char* const& kRandomTownStr = akGeneralStringImp[91];
const char* const& kTeamFmtStr = akGeneralStringImp[92];
const char* const& kThereAreNoPlayersOnMapStr = akGeneralStringImp[93];
const char* const& kThereAreNoTownsOnMapStr = akGeneralStringImp[94];
const char* const& kMapHasNoNameStr = akGeneralStringImp[95];
const char* const& kMapHasNoDescStr = akGeneralStringImp[96];
const char* const& kGrailPlacedButNoObelisksStr = akGeneralStringImp[97];
const char* const& kPlayerOwnsObjectsButNotPresentFmtStr = akGeneralStringImp[98];
const char* const& kTownHasShipyardButIsLandlockedFmtStr = akGeneralStringImp[99];
const char* const& kOnlyOneMonolithOnMapFmtStr = akGeneralStringImp[100];
const char* const& kMonolithEntranceButNoExitFmtStr = akGeneralStringImp[101];
const char* const& kMonolithExitButNoEntranceFmtStr = akGeneralStringImp[102];
const char* const& kSubterraneanGatesOnOneLayerMapStr = akGeneralStringImp[103];
const char* const& kMoreSubterraneanGatesAboveThanBelowStr = akGeneralStringImp[104];
const char* const& kMoreSubterraneanGatesBelowThanAboveStr = akGeneralStringImp[105];
const char* const& kSeersHutQuestArtifactNotOnMapFmtStr = akGeneralStringImp[106];
const char* const& kOnePlayerPresentButNoSpecialVictoryConditionStr = akGeneralStringImp[107];
const char* const& kOnePlayerPresentButNormalVictoryNotDisabledStr = akGeneralStringImp[108];
const char* const& kThereAreNoProblemsWithMapStr = akGeneralStringImp[109];
const char* const& kSVCAquireArtifactButArtifactNotPresentFmtStr = akGeneralStringImp[110];
const char* const& kSVCAquireArtifactButHeroHasArtifactFmtStr = akGeneralStringImp[111];
const char* const& kSVCAccumCreaturePlayerAlreadyHasEnoughCreaturesFmtStr = akGeneralStringImp[112];
const char* const& kSVCUpgradeTownBuildingsAlreadyBuiltFmtStr = akGeneralStringImp[113];
const char* const& kSVCUpgradeTownBuildingsDisabledFmtStr = akGeneralStringImp[114];
const char* const& kSVCBuildGrailStructAlreadyBuiltFmtStr = akGeneralStringImp[115];
const char* const& kSVCBuildGrailStructDisabledFmtStr = akGeneralStringImp[116];
const char* const& kSVCBuildGrailStructNowhereToBuildStr = akGeneralStringImp[117];
const char* const& kSVCDefeatHeroBelongsToHumanFmtStr = akGeneralStringImp[118];
const char* const& kSVCCaptureTownAlreadyOwnedFmtStr = akGeneralStringImp[119];
const char* const& kSVCCaptureTownOwnedByHumanFmtStr = akGeneralStringImp[120];
const char* const& kSVCFlagGeneratorsNoGeneratorsOnMapStr = akGeneralStringImp[121];
const char* const& kSVCFlagGeneratorsPlayerOwnsAllGeneratorsFmtStr = akGeneralStringImp[122];
const char* const& kSVCFlagMinesNoMinesOnMapStr = akGeneralStringImp[123];
const char* const& kSVCFlagMinesPlayerAlreadyOwnsAllMinesFmtStr = akGeneralStringImp[124];
const char* const& kSVCTransportArtifactNotPresentFmtStr = akGeneralStringImp[125];
const char* const& kSLCLoseTownNotOwnedFmtStr = akGeneralStringImp[126];
const char* const& kSLCLoseTownTeamNotHumanFmtStr = akGeneralStringImp[127];
const char* const& kSLCLoseTownMultipleHumanTeamsFmtStr = akGeneralStringImp[128];
const char* const& kSLCLoseTownPlayerNotHumanFmtStr = akGeneralStringImp[129];
const char* const& kSLCLoseTownMultipleHumanPlayersFmtStr = akGeneralStringImp[130];
const char* const& kSLCLoseHeroNotOwnedFmtStr = akGeneralStringImp[131];
const char* const& kSLCLoseHeroTeamNotHumanFmtStr = akGeneralStringImp[132];
const char* const& kSLCLoseHeroMultipleHumanTeamsFmtStr = akGeneralStringImp[133];
const char* const& kSLCLoseHeroPlayerNotHumanFmtStr = akGeneralStringImp[134];
const char* const& kSLCLoseHeroMultipleHumanPlayersFmtStr = akGeneralStringImp[135];
const char* const& kRecoverAutosavedMapStr = akGeneralStringImp[136];
const char* const& kTextFileFilterStr = akGeneralStringImp[137];
const char* const& kUnableToImportTextFmtStr = akGeneralStringImp[138];
const char* const& kTextImportedSuccessfullyFmtStr = akGeneralStringImp[139];
const char* const& kMapNameStr = akGeneralStringImp[140];
const char* const& kMapDescriptionStr = akGeneralStringImp[141];
const char* const& kRumorsStr = akGeneralStringImp[142];
const char* const& kTimedEventsStr = akGeneralStringImp[143];
const char* const& kObjectsStr = akGeneralStringImp[144];
const char* const& kNameStr = akGeneralStringImp[145];
const char* const& kMessageStr = akGeneralStringImp[146];
const char* const& kTextStr = akGeneralStringImp[147];
const char* const& kVisitingHeroFmtStr = akGeneralStringImp[148];
const char* const& kEndOfFileStr = akGeneralStringImp[149];
const char* const& kTimedEventNameFmtStr = akGeneralStringImp[150];
const char* const& kObjectNotFoundStr = akGeneralStringImp[151];
const char* const& kToolbarCaptionStr = akGeneralStringImp[152];
const char* const& kModeBarCaptionStr = akGeneralStringImp[153];
const char* const& kOneWhirlpoolStr = akGeneralStringImp[154];
const char* const& kObjectIsUnreachableFmtStr = akGeneralStringImp[155];
const char* const& kOKStr = akGeneralStringImp[156];
const char* const& kCancelStr = akGeneralStringImp[157];
const char* const& kHelpStr = akGeneralStringImp[158];
const char* const& kGeneralPageCaptionStr = akGeneralStringImp[159];
const char* const& kContentsPageCaptionStr = akGeneralStringImp[160];
const char* const& kResourcesPageCaptionStr = akGeneralStringImp[161];
const char* const& kBuildingsPageCaptionStr = akGeneralStringImp[162];
const char* const& kCreaturesPageCaptionStr = akGeneralStringImp[163];
const char* const& kSecondarySkillsPageCaptionStr = akGeneralStringImp[164];
const char* const& kSpecialLossConditionPageCaptionStr = akGeneralStringImp[165];
const char* const& kPlayerSpecsPageCaptionStr = akGeneralStringImp[166];
const char* const& kRumorsPageCaptionStr = akGeneralStringImp[167];
const char* const& kTeamsPageCaptionStr = akGeneralStringImp[168];
const char* const& kTimedEventsPageCaptionStr = akGeneralStringImp[169];
const char* const& kSpecialVictoryConditionPageCaptionStr = akGeneralStringImp[170];
const char* const& kTreasurePageCaptionStr = akGeneralStringImp[171];
const char* const& kGarrisonPageCaptionStr = akGeneralStringImp[172];
const char* const& kSpellsPageCaptionStr = akGeneralStringImp[173];
const char* const& kGuardiansPageCaptionStr = akGeneralStringImp[174];
const char* const& kArtifactsPageCaptionStr = akGeneralStringImp[175];
const char* const& kAboutBoxCaptionStr = akGeneralStringImp[176];
const char* const& kEditArtifactCaptionStr = akGeneralStringImp[177];
const char* const& kEditCreatureStackCaptionStr = akGeneralStringImp[178];
const char* const& kEditRumorCaptionStr = akGeneralStringImp[179];
const char* const& kEditSecondarySkillCaptionStr = akGeneralStringImp[180];
const char* const& kFindCaptionStr = akGeneralStringImp[181];
const char* const& kMapValidationCaptionStr = akGeneralStringImp[182];
const char* const& kNewMapCaptionStr = akGeneralStringImp[183];
const char* const& kOptionsCaptionStr = akGeneralStringImp[184];
const char* const& kSelectArtifactCaptionStr = akGeneralStringImp[185];
const char* const& kSelectHeroClassCaptionStr = akGeneralStringImp[186];
const char* const& kSelectSpellCaptionStr = akGeneralStringImp[187];
const char* const& kCustomizeCheckStr = akGeneralStringImp[188];
const char* const& kBrushStr = akGeneralStringImp[189];
const char* const& kTerrainTypeStr = akGeneralStringImp[190];
const char* const& kRiverTypeStr = akGeneralStringImp[191];
const char* const& kRoadTypeStr = akGeneralStringImp[192];
const char* const& kPlacementRadiusStaticStr = akGeneralStringImp[193];
const char* const& kStandStillStr = akGeneralStringImp[194];
const char* const& kRadiusOneSquareStr = akGeneralStringImp[195];
const char* const& kRadiusTwoSquaresStr = akGeneralStringImp[196];
const char* const& kRadiusThreeSquaresStr = akGeneralStringImp[197];
const char* const& kRadiusFourSquaresStr = akGeneralStringImp[198];
const char* const& kRadiusFiveSquaresStr = akGeneralStringImp[199];
const char* const& kRadiusSixSquaresStr = akGeneralStringImp[200];
const char* const& kRadiusSevenSquaresStr = akGeneralStringImp[201];
const char* const& kRadiusEightSquaresStr = akGeneralStringImp[202];
const char* const& kRadiusNineSquaresStr = akGeneralStringImp[203];
const char* const& kRadiusTenSquaresStr = akGeneralStringImp[204];
const char* const& kProblemsStaticStr = akGeneralStringImp[205];
const char* const& kNoPropertiesStr = akGeneralStringImp[206];
const char* const& kContinuingWillDeleteUndergroundLayerStr = akGeneralStringImp[207];

namespace SAbandonedMinePropsDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kPotentialResourcesStaticStr = akStringImp[0];
const char* const& kRes2CheckStr = akStringImp[1];
const char* const& kRes3CheckStr = akStringImp[2];
const char* const& kRes4CheckStr = akStringImp[3];
const char* const& kRes5CheckStr = akStringImp[4];
const char* const& kRes6CheckStr = akStringImp[5];
const char* const& kRes7CheckStr = akStringImp[6];
}

namespace SArmyDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kTypeStaticStr = akStringImp[0];
const char* const& kQuantityStaticStr = akStringImp[1];
const char* const& kSlotStaticFmtStr = akStringImp[2];
}

namespace SGarrisonPropertiesDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kOwnerStaticStr = akStringImp[0];
const char* const& kNoneRadioStr = akStringImp[1];
const char* const& kPlayerRadioFmtStr = akStringImp[2];
const char* const& kCreaturesStaticStr = akStringImp[3];
}

namespace SArtifactPropsGeneralPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kTypeStaticStr = akStringImp[0];
const char* const& kMessageStaticStr = akStringImp[1];
const char* const& kNoteStaticStr = akStringImp[2];
const char* const& kSpellStaticStr = akStringImp[3];
}

namespace SBlackBoxPropsGeneralPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kMessageStaticStr = akStringImp[0];
}

namespace SEventPropsGeneralPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kAllowedPlayersStaticStr = akStringImp[0];
const char* const& kPlayerCheckFmtStr = akStringImp[1];
const char* const& kAllowComputerCheckStr = akStringImp[2];
const char* const& kCancelCheckStr = akStringImp[3];
}

namespace SBlackBoxPropsContentsPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kCategoryStaticStr = akStringImp[0];
const char* const& kAddButtonStr = akStringImp[1];
const char* const& kEditButtonStr = akStringImp[2];
const char* const& kRemoveButtonStr = akStringImp[3];
const char* const& kRemoveAllButtonStr = akStringImp[4];
const char* const& kArtifactsStaticStr = akStringImp[5];
const char* const& kCreatureTypeStaticStr = akStringImp[6];
const char* const& kQuantityStaticStr = akStringImp[7];
const char* const& kExperienceBonusStaticStr = akStringImp[8];
const char* const& kLuckBonusStaticStr = akStringImp[9];
const char* const& kNoneRadioStr = akStringImp[10];
const char* const& kPlusOneRadioStr = akStringImp[11];
const char* const& kPlusTwoRadioStr = akStringImp[12];
const char* const& kPlusThreeRadioStr = akStringImp[13];
const char* const& kMinusOneRadioStr = akStringImp[14];
const char* const& kMinusTwoRadioStr = akStringImp[15];
const char* const& kMinusThreeRadioStr = akStringImp[16];
const char* const& kManaBonusStaticStr = akStringImp[17];
const char* const& kGiveRadioStr = akStringImp[18];
const char* const& kTakeRadioStr = akStringImp[19];
const char* const& kMoraleBonusStaticStr = akStringImp[20];
const char* const& kAttackSkillStaticStr = akStringImp[21];
const char* const& kDefenseSkillStaticStr = akStringImp[22];
const char* const& kSpellPowerStaticStr = akStringImp[23];
const char* const& kKnowledgeStaticStr = akStringImp[24];
const char* const& kSecondarySkillStaticStr = akStringImp[25];
const char* const& kMasteryStaticStr = akStringImp[26];
const char* const& kSpellsStaticStr = akStringImp[27];
}

namespace SResourceQuantitiesDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kRes1StaticStr = akStringImp[0];
const char* const& kRes2StaticStr = akStringImp[1];
const char* const& kRes3StaticStr = akStringImp[2];
const char* const& kRes4StaticStr = akStringImp[3];
const char* const& kRes5StaticStr = akStringImp[4];
const char* const& kRes6StaticStr = akStringImp[5];
const char* const& kRes7StaticStr = akStringImp[6];
const char* const& kGiveRadioStr = akStringImp[7];
const char* const& kTakeRadioStr = akStringImp[8];
}

namespace SEditSecondarySkillDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kTypeStaticStr = akStringImp[0];
const char* const& kMasteryStaticStr = akStringImp[1];
const char* const& kBasicRadioStr = akStringImp[2];
const char* const& kAdvancedRadioStr = akStringImp[3];
const char* const& kExpertRadioStr = akStringImp[4];
}

namespace SEditCreatureStackDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kTypeStaticStr = akStringImp[0];
const char* const& kQuantityStaticStr = akStringImp[1];
}

namespace SEditTimedEventGeneralPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kEventNameStaticStr = akStringImp[0];
const char* const& kMessageStaticStr = akStringImp[1];
const char* const& kAllowedPlayersStaticStr = akStringImp[2];
const char* const& kPlayerCheckFmtStr = akStringImp[3];
const char* const& kAllowComputerCheckStr = akStringImp[4];
const char* const& kFirstOccurenceStaticStr = akStringImp[5];
const char* const& kSubsequentOccurenceStaticStr = akStringImp[6];
}

namespace SEditTownEventBuildingsPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kBuildingTreeStaticStr = akStringImp[0];
const char* const& kBuildCheckStr = akStringImp[1];
}

namespace SEditTownEventCreaturesPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kGeneratorNoStaticStr = akStringImp[0];
const char* const& kQuantityStaticStr = akStringImp[1];
const char* const& kNoteStaticStr = akStringImp[2];
}

namespace SEditArtifactDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kEquipWhereStaticStr = akStringImp[0];
const char* const& kArtifactStaticStr = akStringImp[1];
}

namespace SEditRumorDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kNameStaticStr = akStringImp[0];
const char* const& kTextStaticStr = akStringImp[1];
}

namespace SFindDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kFindWhatStaticStr = akStringImp[0];
const char* const& kFindNextButtonStr = akStringImp[1];
const char* const& kFindPrevButtonStr = akStringImp[2];
}

namespace SFlaggablePropsDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kOwnerStaticStr = akStringImp[0];
const char* const& kNoneRadioStr = akStringImp[1];
const char* const& kPlayerRadioFmtStr = akStringImp[2];
}

namespace SHeroPropsGeneralPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kClassStaticStr = akStringImp[0];
const char* const& kPlayerStaticStr = akStringImp[1];
const char* const& kIdentityStaticStr = akStringImp[2];
const char* const& kNameStaticStr = akStringImp[3];
const char* const& kExperienceStaticStr = akStringImp[4];
const char* const& kPortraitStaticStr = akStringImp[5];
const char* const& kPatrolStaticStr = akStringImp[6];
const char* const& kCustomizeNameCheckStr = akStringImp[7];
const char* const& kCustomizePortraitCheckStr = akStringImp[8];
}

namespace SHeroPropsCreaturesPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kFormationStaticStr = akStringImp[0];
const char* const& kSpreadRadioStr = akStringImp[1];
const char* const& kGroupedRadioStr = akStringImp[2];
}

namespace SHeroPropsSecSkillsPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kTypeStaticStr = akStringImp[0];
const char* const& kMasteryStaticStr = akStringImp[1];
const char* const& kSkillStaticFmtStr = akStringImp[2];
}

namespace SHeroPropsArtifactsPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kHasSpellbookCheckStr = akStringImp[0];
const char* const& kArtifactsStaticStr = akStringImp[1];
const char* const& kNameColumnStr = akStringImp[2];
const char* const& kLocationColumnStr = akStringImp[3];
const char* const& kAddButtonStr = akStringImp[4];
const char* const& kEditButtonStr = akStringImp[5];
const char* const& kRemoveButtonStr = akStringImp[6];
const char* const& kRemoveAllButtonStr = akStringImp[7];
}

namespace SMapSpecsGeneralPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kDifficultyStaticStr = akStringImp[0];
const char* const& kEasyRadioStr = akStringImp[1];
const char* const& kNormalRadioStr = akStringImp[2];
const char* const& kToughRadioStr = akStringImp[3];
const char* const& kExpertRadioStr = akStringImp[4];
const char* const& kImpossibleRadioStr = akStringImp[5];
const char* const& kTwoLevelMapCheckStr = akStringImp[6];
const char* const& kMapNameStaticStr = akStringImp[7];
const char* const& kDescriptionStaticStr = akStringImp[8];
}

namespace SMapSpecsPlayerSpecsPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kPlayerStaticStr = akStringImp[0];
const char* const& kGenerateHeroCheckStr = akStringImp[1];
const char* const& kPlayabilityStaticStr = akStringImp[2];
const char* const& kHumanPlayableCheckStr = akStringImp[3];
const char* const& kComputerPlayableCheckStr = akStringImp[4];
const char* const& kBehaviorStaticStr = akStringImp[5];
}

namespace SMapSpecsTeamsPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kEnableTeamsCheckStr = akStringImp[0];
const char* const& kNumberOfTeamsStaticStr = akStringImp[1];
const char* const& kTeamAssignmentsStaticStr = akStringImp[2];
const char* const& kTeamNumberStaticStr = akStringImp[3];
const char* const& kPlayerStaticFmtStr = akStringImp[4];
}

namespace SMapSpecsRumorsPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kRumorsStaticStr = akStringImp[0];
const char* const& kAddButtonStr = akStringImp[1];
const char* const& kEditButtonStr = akStringImp[2];
const char* const& kRemoveButtonStr = akStringImp[3];
const char* const& kRemoveAllButtonStr = akStringImp[4];
}

namespace SMapSpecsTimedEventsPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kEventsStaticStr = akStringImp[0];
const char* const& kAddButtonStr = akStringImp[1];
const char* const& kEditButtonStr = akStringImp[2];
const char* const& kRemoveButtonStr = akStringImp[3];
const char* const& kRemoveAllButtonStr = akStringImp[4];
const char* const& kMoveUpButtonStr = akStringImp[5];
const char* const& kMoveDownButtonStr = akStringImp[6];
}

namespace SMapSpecsVictoryCondPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kSelectVictoryConditionStaticStr = akStringImp[0];
const char* const& kNoneRadioStr = akStringImp[1];
const char* const& kAquireArtifactRadioStr = akStringImp[2];
const char* const& kAccumCreaturesRadioStr = akStringImp[3];
const char* const& kAccumResourcesRadioStr = akStringImp[4];
const char* const& kUpgradeTownRadioStr = akStringImp[5];
const char* const& kBuildGrailRadioStr = akStringImp[6];
const char* const& kDefeatHeroRadioStr = akStringImp[7];
const char* const& kCaptureTownRadioStr = akStringImp[8];
const char* const& kDefeatMonsterRadioStr = akStringImp[9];
const char* const& kFlagGeneratorsRadioStr = akStringImp[10];
const char* const& kFlagMinesRadioStr = akStringImp[11];
const char* const& kTransportArtifactRadioStr = akStringImp[12];
const char* const& kNormalVictoryCheckStr = akStringImp[13];
const char* const& kComputerAlsoCheckStr = akStringImp[14];
const char* const& kArtifactStaticStr = akStringImp[15];
const char* const& kCreatureTypeStaticStr = akStringImp[16];
const char* const& kQuantityStaticStr = akStringImp[17];
const char* const& kResourceTypeStaticStr = akStringImp[18];
const char* const& kTownStaticStr = akStringImp[19];
const char* const& kHallLevelStaticStr = akStringImp[20];
const char* const& kTownHallRadioStr = akStringImp[21];
const char* const& kCityHallRadioStr = akStringImp[22];
const char* const& kCapitolHallRadioStr = akStringImp[23];
const char* const& kCastleLevelStaticStr = akStringImp[24];
const char* const& kFortCastleRadioStr = akStringImp[25];
const char* const& kCitadelCastleRadioStr = akStringImp[26];
const char* const& kCastleCastleRadioStr = akStringImp[27];
const char* const& kHeroStaticStr = akStringImp[28];
const char* const& kMonsterStaticStr = akStringImp[29];
const char* const& kDestinationStaticStr = akStringImp[30];
}

namespace SMapSpecsLossCondPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kSelectLossConditionStaticStr = akStringImp[0];
const char* const& kNoneRadioStr = akStringImp[1];
const char* const& kLoseTownRadioStr = akStringImp[2];
const char* const& kLoseHeroRadioStr = akStringImp[3];
const char* const& kTimeExpiresRadioStr = akStringImp[4];
const char* const& kTownStaticStr = akStringImp[5];
const char* const& kHeroStaticStr = akStringImp[6];
const char* const& kTimeLimitStaticStr = akStringImp[7];
}

namespace SMonsterPropsGeneralPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kTypeStaticStr = akStringImp[0];
const char* const& kQuantityStaticStr = akStringImp[1];
const char* const& kRandomQtyRadioStr = akStringImp[2];
const char* const& kCustomQtyRadioStr = akStringImp[3];
const char* const& kNeverGrowsCheckStr = akStringImp[4];
const char* const& kDispositionStaticStr = akStringImp[5];
const char* const& kDispositionCompliantRadioStr = akStringImp[6];
const char* const& kDispositionFriendlyRadioStr = akStringImp[7];
const char* const& kDispositionAggressiveRadioStr = akStringImp[8];
const char* const& kDispositionHostileRadioStr = akStringImp[9];
const char* const& kDispositionSavageRadioStr = akStringImp[10];
const char* const& kNeverFleesCheckStr = akStringImp[11];
const char* const& kMessageStaticStr = akStringImp[12];
}

namespace SMonsterPropsTreasurePageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kResourcesStaticStr = akStringImp[0];
const char* const& kRes1StaticStr = akStringImp[1];
const char* const& kRes2StaticStr = akStringImp[2];
const char* const& kRes3StaticStr = akStringImp[3];
const char* const& kRes4StaticStr = akStringImp[4];
const char* const& kRes5StaticStr = akStringImp[5];
const char* const& kRes6StaticStr = akStringImp[6];
const char* const& kRes7StaticStr = akStringImp[7];
const char* const& kArtifactStaticStr = akStringImp[8];
}

namespace SNewMapDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kMapSizeStaticStr = akStringImp[0];
const char* const& k36x36RadioStr = akStringImp[1];
const char* const& k72x72RadioStr = akStringImp[2];
const char* const& k108x108RadioStr = akStringImp[3];
const char* const& k144x144RadioStr = akStringImp[4];
const char* const& kTwoLevelMapCheckStr = akStringImp[5];
}

namespace SOptionsDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kTileFrequencyStaticStr = akStringImp[0];
const char* const& kNoneStaticStr = akStringImp[1];
const char* const& kLotsStaticStr = akStringImp[2];
const char* const& kRepaintCheckStr = akStringImp[3];
const char* const& kAutosaveOptionsStaticStr = akStringImp[4];
const char* const& kEnableAutosaveCheckStr = akStringImp[5];
const char* const& kAutosaveEveryStaticStr = akStringImp[6];
const char* const& kMinutesStaticStr = akStringImp[7];
}

namespace SResourcePropsGeneralPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kTypeStaticStr = akStringImp[0];
const char* const& kQuantityStaticStr = akStringImp[1];
const char* const& kRandomQtyRadioStr = akStringImp[2];
const char* const& kCustomQtyRadioStr = akStringImp[3];
const char* const& kQuantityNoteStaticStr = akStringImp[4];
const char* const& kMessageStaticStr = akStringImp[5];
const char* const& kNoteStaticStr = akStringImp[6];
}

namespace SScholarPropsDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kRewardStaticStr = akStringImp[0];
const char* const& kRandomRewardRadioStr = akStringImp[1];
const char* const& kPriSkillRewardRadioStr = akStringImp[2];
const char* const& kSecSkillRewardRadioStr = akStringImp[3];
const char* const& kSpellRewardRadioStr = akStringImp[4];
}

namespace SSeersHutPropsDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kQuestArtifactStaticStr = akStringImp[0];
const char* const& kQuestRewardStaticStr = akStringImp[1];
const char* const& kTypeStaticStr = akStringImp[2];
const char* const& kArtifactStaticStr = akStringImp[3];
const char* const& kCreatureTypeStaticStr = akStringImp[4];
const char* const& kQuantityStaticStr = akStringImp[5];
const char* const& kExperienceBonusStaticStr = akStringImp[6];
const char* const& kLuckBonusStaticStr = akStringImp[7];
const char* const& kPlusOneRadioStr = akStringImp[8];
const char* const& kPlusTwoRadioStr = akStringImp[9];
const char* const& kPlusThreeRadioStr = akStringImp[10];
const char* const& kManaBonusStaticStr = akStringImp[11];
const char* const& kMoraleBonusStaticStr = akStringImp[12];
const char* const& kPrimarySkillStaticStr = akStringImp[13];
const char* const& kBonusStaticStr = akStringImp[14];
const char* const& kResourceTypeStaticStr = akStringImp[15];
const char* const& kSecondarySkillStaticStr = akStringImp[16];
const char* const& kMasteryStaticStr = akStringImp[17];
const char* const& kSpellStaticStr = akStringImp[18];
}

namespace SShrinePropsDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kSpellStaticFmtStr = akStringImp[0];
const char* const& kRandomSpellRadioStr = akStringImp[1];
const char* const& kCustomSpellRadioStr = akStringImp[2];
}

namespace SSignPropsDlgText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kMessageStaticStr = akStringImp[0];
}

namespace STownPropsGeneralPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kTownTypeStaticStr = akStringImp[0];
const char* const& kPlayerStaticStr = akStringImp[1];
const char* const& kTownNameStaticStr = akStringImp[2];
const char* const& kCustomizeCheckStr = akStringImp[3];
const char* const& kVisitingHeroStaticStr = akStringImp[4];
const char* const& kHeroNameStaticStr = akStringImp[5];
const char* const& kHeroClassStaticStr = akStringImp[6];
const char* const& kAddButtonStr = akStringImp[7];
const char* const& kEditButtonStr = akStringImp[8];
const char* const& kRemoveButtonStr = akStringImp[9];
}

namespace STownPropsGarrisonPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kFormationStaticStr = akStringImp[0];
const char* const& kSpreadRadioStr = akStringImp[1];
const char* const& kGroupedRadioStr = akStringImp[2];
}

namespace STownPropsBuildingsPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kHasFortCheckStr = akStringImp[0];
const char* const& kBuildingTreeStaticStr = akStringImp[1];
const char* const& kEnabledCheckStr = akStringImp[2];
const char* const& kBuiltCheckStr = akStringImp[3];
const char* const& kBuildAllButtonStr = akStringImp[4];
const char* const& kDemolishAllButtonStr = akStringImp[5];
}

namespace STownPropsSpellsPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kSpellsStaticStr = akStringImp[0];
}

namespace STownPropsTimedEventsPageText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kEventsStaticStr = akStringImp[0];
const char* const& kAddButtonStr = akStringImp[1];
const char* const& kEditButtonStr = akStringImp[2];
const char* const& kRemoveButtonStr = akStringImp[3];
const char* const& kRemoveAllButtonStr = akStringImp[4];
const char* const& kMoveUpButtonStr = akStringImp[5];
const char* const& kMoveDownButtonStr = akStringImp[6];
}

namespace SMainMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kFileStr = akStringImp[0];
const char* const& kEditStr = akStringImp[1];
const char* const& kViewStr = akStringImp[2];
const char* const& kToolsStr = akStringImp[3];
const char* const& kPlayerStr = akStringImp[4];
const char* const& kHelpStr = akStringImp[5];
}

namespace SFileMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kNewStr = akStringImp[0];
const char* const& kOpenStr = akStringImp[1];
const char* const& kSaveStr = akStringImp[2];
const char* const& kSaveAsStr = akStringImp[3];
const char* const& kExportTextStr = akStringImp[4];
const char* const& kImportTextStr = akStringImp[5];
const char* const& kRecentFileStr = akStringImp[6];
const char* const& kExitStr = akStringImp[7];
}

namespace SEditMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kUndoStr = akStringImp[0];
const char* const& kRedoStr = akStringImp[1];
const char* const& kCutStr = akStringImp[2];
const char* const& kCopyStr = akStringImp[3];
const char* const& kPasteStr = akStringImp[4];
const char* const& kDeleteStr = akStringImp[5];
const char* const& kFindStr = akStringImp[6];
const char* const& kFindNextStr = akStringImp[7];
const char* const& kFindPrevStr = akStringImp[8];
const char* const& kPropertiesStr = akStringImp[9];
}

namespace SViewMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kZoomInStr = akStringImp[0];
const char* const& kZoomOutStr = akStringImp[1];
const char* const& kUndergroundStr = akStringImp[2];
const char* const& kGridStr = akStringImp[3];
const char* const& kPassabilityStr = akStringImp[4];
const char* const& kObjectAnimationStr = akStringImp[5];
const char* const& kTerrainAnimationStr = akStringImp[6];
const char* const& kToolbarStr = akStringImp[7];
const char* const& kModeBarStr = akStringImp[8];
const char* const& kStatusBarStr = akStringImp[9];
}

namespace SToolsMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kTerrainStr = akStringImp[0];
const char* const& kRiversStr = akStringImp[1];
const char* const& kRoadsStr = akStringImp[2];
const char* const& kEraseStr = akStringImp[3];
const char* const& kObjectsStr = akStringImp[4];
const char* const& kMapSpecificationsStr = akStringImp[5];
const char* const& kValidateMapStr = akStringImp[6];
const char* const& kOptionsStr = akStringImp[7];
}

namespace SToolsTerrainMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& k1x1Str = akStringImp[0];
const char* const& k2x2Str = akStringImp[1];
const char* const& k4x4Str = akStringImp[2];
const char* const& kFillStr = akStringImp[3];
const char* const& kDirtStr = akStringImp[4];
const char* const& kSandStr = akStringImp[5];
const char* const& kGrassStr = akStringImp[6];
const char* const& kSnowStr = akStringImp[7];
const char* const& kSwampStr = akStringImp[8];
const char* const& kRoughStr = akStringImp[9];
const char* const& kSubterraneanStr = akStringImp[10];
const char* const& kLavaStr = akStringImp[11];
const char* const& kWaterStr = akStringImp[12];
const char* const& kRockStr = akStringImp[13];
}

namespace SToolsRiversMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kClearStr = akStringImp[0];
const char* const& kIcyStr = akStringImp[1];
const char* const& kMuddyStr = akStringImp[2];
const char* const& kLavaStr = akStringImp[3];
}

namespace SToolsRoadsMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kDirtStr = akStringImp[0];
const char* const& kGravelStr = akStringImp[1];
const char* const& kCobblestoneStr = akStringImp[2];
}

namespace SToolsEraseMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& k1x1Str = akStringImp[0];
const char* const& k2x2Str = akStringImp[1];
const char* const& k4x4Str = akStringImp[2];
const char* const& kFillStr = akStringImp[3];
}

namespace SToolsObjectsMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kDirtObjectsStr = akStringImp[0];
const char* const& kSandObjectsStr = akStringImp[1];
const char* const& kGrassObjectsStr = akStringImp[2];
const char* const& kSnowObjectsStr = akStringImp[3];
const char* const& kSwampObjectsStr = akStringImp[4];
const char* const& kRoughObjectsStr = akStringImp[5];
const char* const& kSubterraneanObjectsStr = akStringImp[6];
const char* const& kLavaObjectsStr = akStringImp[7];
const char* const& kWaterObjectsStr = akStringImp[8];
const char* const& kAllTerrainObjectsStr = akStringImp[9];
const char* const& kTownsStr = akStringImp[10];
const char* const& kMonstersStr = akStringImp[11];
const char* const& kHeroesStr = akStringImp[12];
const char* const& kArtifactsStr = akStringImp[13];
const char* const& kTreasuresStr = akStringImp[14];
}

namespace SPlayerMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kNoneStr = akStringImp[0];
const char* const& kPlayer1FmtStr = akStringImp[1];
const char* const& kPlayer2FmtStr = akStringImp[2];
const char* const& kPlayer3FmtStr = akStringImp[3];
const char* const& kPlayer4FmtStr = akStringImp[4];
const char* const& kPlayer5FmtStr = akStringImp[5];
const char* const& kPlayer6FmtStr = akStringImp[6];
const char* const& kPlayer7FmtStr = akStringImp[7];
const char* const& kPlayer8FmtStr = akStringImp[8];
}

namespace SHelpMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kHelpTopicsStr = akStringImp[0];
const char* const& kAboutMapEditorStr = akStringImp[1];
}

namespace SContextMenuText {
namespace {
const char* akStringImp[kNumStrings];
}
const char* const& kWhatsThisStr = akStringImp[0];
const char* const& kUndoStr = akStringImp[1];
const char* const& kRedoStr = akStringImp[2];
const char* const& kCutStr = akStringImp[3];
const char* const& kCopyStr = akStringImp[4];
const char* const& kPasteStr = akStringImp[5];
const char* const& kDeleteStr = akStringImp[6];
const char* const& kFindStr = akStringImp[7];
const char* const& kFindNextStr = akStringImp[8];
const char* const& kFindPrevStr = akStringImp[9];
const char* const& kPropertiesStr = akStringImp[10];
}

// One group of editor.txt: skip the separator line, then load the group.
// The assert text spells "Group ::kNumStrings" with the space retail has.
#define INITIALIZE_TEXT(Group) { baseID++; assert(pTextResource->GetNumberOfStrings() - baseID >= Group ::kNumStrings); size_t totalLength = 0; unsigned int i; for (i = 0; i < Group::kNumStrings; i++) totalLength += stringLength(pTextResource->GetText(baseID + i)) + 1; static TAutoArrayPtr<char> apBuffer(new char[totalLength]); if (!apBuffer.get()) throw TAllocationFailure(__FILE__, __LINE__); char* pDest = apBuffer.get(); for (i = 0; i < Group::kNumStrings; i++) { const char* pSrc = pTextResource->GetText(baseID + i); size_t length = stringLength(pSrc) + 1; stringCopy(pDest, pSrc); Group::akStringImp[i] = pDest; pDest += length; } baseID += Group::kNumStrings; }

void SText::initialize()
{
#line 2767
    assert(!bInitialized);
    bInitialized = true;

    TResourcePtr<TTextResource> pTextResource(ResourceManager::GetText("editor.txt"));
    if (!pTextResource.get())
#line 2774
        throw TRuntimeError(__FILE__, __LINE__, "Unable to load \"editor.txt\".");

    unsigned int baseID = 2;
    {
#line 2779
        assert(pTextResource->GetNumberOfStrings() - baseID >= kNumGeneralStrings);
        size_t totalLength = 0;
        unsigned int i;
        for (i = 0; i < kNumGeneralStrings; i++)
            totalLength += stringLength(pTextResource->GetText(baseID + i)) + 1;
        static TAutoArrayPtr<char> apBuffer(new char[totalLength]);
        if (!apBuffer.get())
#line 2790
            throw TAllocationFailure(__FILE__, __LINE__);
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
#line 2836
    INITIALIZE_TEXT(SAbandonedMinePropsDlgText)
    INITIALIZE_TEXT(SArmyDlgText)
    INITIALIZE_TEXT(SGarrisonPropertiesDlgText)
    INITIALIZE_TEXT(SArtifactPropsGeneralPageText)
    INITIALIZE_TEXT(SBlackBoxPropsGeneralPageText)
    INITIALIZE_TEXT(SEventPropsGeneralPageText)
    INITIALIZE_TEXT(SBlackBoxPropsContentsPageText)
    INITIALIZE_TEXT(SResourceQuantitiesDlgText)
    INITIALIZE_TEXT(SEditSecondarySkillDlgText)
    INITIALIZE_TEXT(SEditCreatureStackDlgText)
    INITIALIZE_TEXT(SEditTimedEventGeneralPageText)
    INITIALIZE_TEXT(SEditTownEventBuildingsPageText)
    INITIALIZE_TEXT(SEditTownEventCreaturesPageText)
    INITIALIZE_TEXT(SEditArtifactDlgText)
    INITIALIZE_TEXT(SEditRumorDlgText)
    INITIALIZE_TEXT(SFindDlgText)
    INITIALIZE_TEXT(SFlaggablePropsDlgText)
    INITIALIZE_TEXT(SHeroPropsGeneralPageText)
    INITIALIZE_TEXT(SHeroPropsCreaturesPageText)
    INITIALIZE_TEXT(SHeroPropsSecSkillsPageText)
    INITIALIZE_TEXT(SHeroPropsArtifactsPageText)
    INITIALIZE_TEXT(SMapSpecsGeneralPageText)
    INITIALIZE_TEXT(SMapSpecsPlayerSpecsPageText)
    INITIALIZE_TEXT(SMapSpecsTeamsPageText)
    INITIALIZE_TEXT(SMapSpecsRumorsPageText)
    INITIALIZE_TEXT(SMapSpecsTimedEventsPageText)
    INITIALIZE_TEXT(SMapSpecsVictoryCondPageText)
    INITIALIZE_TEXT(SMapSpecsLossCondPageText)
    INITIALIZE_TEXT(SMonsterPropsGeneralPageText)
    INITIALIZE_TEXT(SMonsterPropsTreasurePageText)
    INITIALIZE_TEXT(SNewMapDlgText)
    INITIALIZE_TEXT(SOptionsDlgText)
    INITIALIZE_TEXT(SResourcePropsGeneralPageText)
    INITIALIZE_TEXT(SScholarPropsDlgText)
    INITIALIZE_TEXT(SSeersHutPropsDlgText)
    INITIALIZE_TEXT(SShrinePropsDlgText)
    INITIALIZE_TEXT(SSignPropsDlgText)
    INITIALIZE_TEXT(STownPropsGeneralPageText)
    INITIALIZE_TEXT(STownPropsGarrisonPageText)
    INITIALIZE_TEXT(STownPropsBuildingsPageText)
    INITIALIZE_TEXT(STownPropsSpellsPageText)
    INITIALIZE_TEXT(STownPropsTimedEventsPageText)
    INITIALIZE_TEXT(SMainMenuText)
    INITIALIZE_TEXT(SFileMenuText)
    INITIALIZE_TEXT(SEditMenuText)
    INITIALIZE_TEXT(SViewMenuText)
    INITIALIZE_TEXT(SToolsMenuText)
    INITIALIZE_TEXT(SToolsTerrainMenuText)
    INITIALIZE_TEXT(SToolsRiversMenuText)
    INITIALIZE_TEXT(SToolsRoadsMenuText)
    INITIALIZE_TEXT(SToolsEraseMenuText)
    INITIALIZE_TEXT(SToolsObjectsMenuText)
    INITIALIZE_TEXT(SPlayerMenuText)
    INITIALIZE_TEXT(SHelpMenuText)
    INITIALIZE_TEXT(SContextMenuText)
}
