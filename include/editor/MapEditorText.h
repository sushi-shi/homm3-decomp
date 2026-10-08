// MapEditorText.h - the editor's text strings (MapEditorText.cpp; Loki h3maped object 18).
// SText::initialize loads editor.txt once: the general strings, then one
// group per dialog, page and menu, each after its heading line. Every
// string is exported as a reference to its loaded pointer: users load the
// reference, then the pointer. The groups are namespaces (Loki's image
// names SArmyDlgText::{anonymous}::akStringImp, and __PRETTY_FUNCTION__
// prints "void SText::initialize()" without "static").
//
// Groups, counts and order are GOG's: each reference's dynamic initializer
// (0x462a63..0x4690fd) stores the address of its group's array element, so
// the initializers give every group's members in index order, and the
// image's editor.txt gives each group's heading and texts. GOG's general
// group no longer starts with the registry key. Names Loki's symbols prove
// are kept where the text keeps its meaning; the GOG-only strings and
// groups are named from their texts and headings, as no symbol survives.
// The counts' type is not recorded.
#ifndef HOMM3_EDITOR_MAPEDITORTEXT_H
#define HOMM3_EDITOR_MAPEDITORTEXT_H

#include "va.h"

enum { kNumGeneralStrings = 248 };
DATA(0x005a0da0) extern const char* const& kAppTitleStr;
DATA(0x0059ff14) extern const char* const& kCopyrightStr;
DATA(0x005a05a8) extern const char* const& kVersionFmtStr;
DATA(0x005a1138) extern const char* const& kMapFileFilterStr;
DATA(0x005a03dc) extern const char* const& kMapFileRegNameStr;
DATA(0x0059ffa8) extern const char* const& kMapSpecsSheetCaptionStr;
DATA(0x005a14e4) extern const char* const& kObjectPropertiesCaptionFmtStr;
DATA(0x005a1940) extern const char* const& kEditTimedEventSheetCaptionStr;
DATA(0x005a145c) extern const char* const& kSelNoneStr;
DATA(0x005a0870) extern const char* const& kRandomStr;
DATA(0x0059fef4) extern const char* const& kUnknownStr;
DATA(0x005a1908) extern const char* const& kBackpackStr;
DATA(0x005a0f20) extern const char* const& kExperienceStr;
DATA(0x005a0c34) extern const char* const& kManaStr;
DATA(0x005a0d78) extern const char* const& kMoraleStr;
DATA(0x005a0abc) extern const char* const& kLuckStr;
DATA(0x005a11d4) extern const char* const& kResourceStr;
DATA(0x005a1b08) extern const char* const& kResourcesStr;
DATA(0x0059ff78) extern const char* const& kPrimarySkillStr;
DATA(0x005a0bc8) extern const char* const& kPrimarySkillsStr;
DATA(0x005a0d80) extern const char* const& kArtifactStr;
DATA(0x005a1ac8) extern const char* const& kArtifactsStr;
DATA(0x005a1b04) extern const char* const& kSpellStr;
DATA(0x005a12b0) extern const char* const& kSpellsStr;
DATA(0x005a0fbc) extern const char* const& kCreatureStr;
DATA(0x005a084c) extern const char* const& kCreaturesStr;
DATA(0x0059ffa0) extern const char* const& kSecondarySkillStr;
DATA(0x005a05a4) extern const char* const& kSecondarySkillsStr;
DATA(0x005a180c) extern const char* const& kPlayerNameFmtStr;
DATA(0x005a0e1c) extern const char* const& kPlayerNoneStr;
DATA(0x005a18a8) extern const char* const& kNeverStr;
DATA(0x005a0fc4) extern const char* const& kEveryDayStr;
DATA(0x005a116c) extern const char* const& kEvery2DaysStr;
DATA(0x005a142c) extern const char* const& kEvery3DaysStr;
DATA(0x0059ff20) extern const char* const& kEvery4DaysStr;
DATA(0x005a0c70) extern const char* const& kEvery5DaysStr;
DATA(0x005a1000) extern const char* const& kEvery6DaysStr;
DATA(0x0059ff7c) extern const char* const& kEvery7DaysStr;
DATA(0x005a1ac0) extern const char* const& kEvery14DaysStr;
DATA(0x005a0f64) extern const char* const& kEvery21DaysStr;
DATA(0x005a1a54) extern const char* const& kEvery28DaysStr;
DATA(0x005a0998) extern const char* const& kMaxObjectsOfTypeFmtStr;
DATA(0x005a0408) extern const char* const& kNoMoreClassHeroesStr;
DATA(0x005a0fb8) extern const char* const& kSelectPlayerBeforePlacingHeroStr;
DATA(0x005a11e0) extern const char* const& kMaxHeroesPerPlayerFmtStr;
DATA(0x005a0ecc) extern const char* const& kGrailAlreadyPlacedStr;
DATA(0x005a0400) extern const char* const& kGrailPlacedTooCloseToEdgeFmtStr;
DATA(0x0059ff08) extern const char* const& kNowhereToPasteStr;
DATA(0x005a1030) extern const char* const& kDebugBuildStr;
DATA(0x005a0d84) extern const char* const& kReleaseBuildStr;
DATA(0x005a190c) extern const char* const& kNeed16BitColorStr;
DATA(0x005a14ec) extern const char* const& kInvalidMapVersionFmtStr;
DATA(0x005a11cc) extern const char* const& kInvalidMapFileStr;
DATA(0x005a0f14) extern const char* const& k2DaysStr;
DATA(0x005a1250) extern const char* const& k3DaysStr;
DATA(0x005a1334) extern const char* const& k4DaysStr;
DATA(0x0059ff9c) extern const char* const& k5DaysStr;
DATA(0x005a1178) extern const char* const& k6DaysStr;
DATA(0x005a0ffc) extern const char* const& k1WeekStr;
DATA(0x005a0f2c) extern const char* const& k2WeeksStr;
DATA(0x005a0fd0) extern const char* const& k3WeeksStr;
DATA(0x005a1410) extern const char* const& k4WeeksStr;
DATA(0x005a0cf8) extern const char* const& k5WeeksStr;
DATA(0x005a1320) extern const char* const& k6WeeksStr;
DATA(0x0059ffac) extern const char* const& k7WeeksStr;
DATA(0x005a0fb4) extern const char* const& k2MonthsStr;
DATA(0x005a0d74) extern const char* const& k3MonthsStr;
DATA(0x005a14e0) extern const char* const& k4MonthsStr;
DATA(0x005a0a68) extern const char* const& k5MonthsStr;
DATA(0x005a0f24) extern const char* const& k6MonthsStr;
DATA(0x005a1328) extern const char* const& k7MonthsStr;
DATA(0x005a0c98) extern const char* const& k8MonthsStr;
DATA(0x005a0494) extern const char* const& k9MonthsStr;
DATA(0x005a06e4) extern const char* const& k10MonthsStr;
DATA(0x005a0db8) extern const char* const& k11MonthsStr;
DATA(0x005a0f60) extern const char* const& k12MonthsStr;
DATA(0x005a0490) extern const char* const& kAnyTownStr;
DATA(0x005a11dc) extern const char* const& kObjectAtLocationFmtStr;
DATA(0x005a0fd4) extern const char* const& kSpecificHeroAndClassFmtStr;
DATA(0x005a081c) extern const char* const& kRandomMonsterStr;
DATA(0x005a0528) extern const char* const& kRandomMonstersStr;
DATA(0x005a1a24) extern const char* const& kWarriorStr;
DATA(0x005a0f5c) extern const char* const& kBuilderStr;
DATA(0x005a1890) extern const char* const& kExplorerStr;
DATA(0x005a0818) extern const char* const& kWarningStr;
DATA(0x005a11c0) extern const char* const& kHeroesNeedOwnerStr;
DATA(0x005a05a0) extern const char* const& kPlayerHasMaxHeroesStr;
DATA(0x005a0e68) extern const char* const& kContinuingWillDeleteHeroFmtStr;
DATA(0x005a1ac4) extern const char* const& kNoMoreHeroesStr;
DATA(0x005a1720) extern const char* const& kNoPlayersOnMapStr;
DATA(0x005a1170) extern const char* const& kRandomTownStr;
DATA(0x005a1458) extern const char* const& kTeamFmtStr;
DATA(0x005a117c) extern const char* const& kThereAreNoPlayersOnMapStr;
DATA(0x005a0ec8) extern const char* const& kThereAreNoTownsOnMapStr;
DATA(0x005a0da8) extern const char* const& kMapHasNoNameStr;
DATA(0x005a1324) extern const char* const& kMapHasNoDescStr;
DATA(0x005a0dc0) extern const char* const& kGrailPlacedButNoObelisksStr;
DATA(0x005a18a4) extern const char* const& kPlayerOwnsObjectsButNotPresentFmtStr;
DATA(0x005a0dd0) extern const char* const& kTownHasShipyardButIsLandlockedFmtStr;
DATA(0x005a1160) extern const char* const& kOnlyOneMonolithOnMapFmtStr;
DATA(0x005a0d64) extern const char* const& kMonolithEntranceButNoExitFmtStr;
DATA(0x0059ff10) extern const char* const& kMonolithExitButNoEntranceFmtStr;
DATA(0x005a131c) extern const char* const& kSubterraneanGatesOnOneLayerMapStr;
DATA(0x005a1198) extern const char* const& kMoreSubterraneanGatesAboveThanBelowStr;
DATA(0x005a0f18) extern const char* const& kMoreSubterraneanGatesBelowThanAboveStr;
DATA(0x005a0a94) extern const char* const& kSeersHutQuestArtifactNotOnMapFmtStr;
DATA(0x0059ff98) extern const char* const& kOnePlayerPresentButNoSpecialVictoryConditionStr;
DATA(0x0059ff24) extern const char* const& kOnePlayerPresentButNormalVictoryNotDisabledStr;
DATA(0x0059ffb0) extern const char* const& kThereAreNoProblemsWithMapStr;
DATA(0x005a0820) extern const char* const& kSVCAquireArtifactButArtifactNotPresentFmtStr;
DATA(0x005a0dbc) extern const char* const& kSVCAquireArtifactButHeroHasArtifactFmtStr;
DATA(0x005a0fe0) extern const char* const& kSVCAccumCreaturePlayerAlreadyHasEnoughCreaturesFmtStr;
DATA(0x005a0a6c) extern const char* const& kSVCUpgradeTownBuildingsAlreadyBuiltFmtStr;
DATA(0x005a1174) extern const char* const& kSVCUpgradeTownBuildingsDisabledFmtStr;
DATA(0x005a1a5c) extern const char* const& kSVCBuildGrailStructAlreadyBuiltFmtStr;
DATA(0x005a048c) extern const char* const& kSVCBuildGrailStructDisabledFmtStr;
DATA(0x005a17d8) extern const char* const& kSVCBuildGrailStructNowhereToBuildStr;
DATA(0x005a1340) extern const char* const& kSVCDefeatHeroBelongsToHumanFmtStr;
DATA(0x005a0d6c) extern const char* const& kSVCCaptureTownAlreadyOwnedFmtStr;
DATA(0x005a1460) extern const char* const& kSVCCaptureTownOwnedByHumanFmtStr;
DATA(0x005a133c) extern const char* const& kSVCFlagGeneratorsNoGeneratorsOnMapStr;
DATA(0x005a0a74) extern const char* const& kSVCFlagGeneratorsPlayerOwnsAllGeneratorsFmtStr;
DATA(0x005a1a1c) extern const char* const& kSVCFlagMinesNoMinesOnMapStr;
DATA(0x005a04d8) extern const char* const& kSVCFlagMinesPlayerAlreadyOwnsAllMinesFmtStr;
DATA(0x005a1144) extern const char* const& kSVCTransportArtifactNotPresentFmtStr;
DATA(0x005a0d70) extern const char* const& kSLCLoseTownNotOwnedFmtStr;
DATA(0x005a188c) extern const char* const& kSLCLoseTownTeamNotHumanFmtStr;
DATA(0x005a0fd8) extern const char* const& kSLCLoseTownMultipleHumanTeamsFmtStr;
DATA(0x005a058c) extern const char* const& kSLCLoseTownPlayerNotHumanFmtStr;
DATA(0x005a1134) extern const char* const& kSLCLoseTownMultipleHumanPlayersFmtStr;
DATA(0x005a1254) extern const char* const& kSLCLoseHeroNotOwnedFmtStr;
DATA(0x005a0fc0) extern const char* const& kSLCLoseHeroTeamNotHumanFmtStr;
DATA(0x005a1adc) extern const char* const& kSLCLoseHeroMultipleHumanTeamsFmtStr;
DATA(0x005a1338) extern const char* const& kSLCLoseHeroPlayerNotHumanFmtStr;
DATA(0x005a0588) extern const char* const& kSLCLoseHeroMultipleHumanPlayersFmtStr;
DATA(0x005a0930) extern const char* const& kRecoverAutosavedMapStr;
DATA(0x005a1a20) extern const char* const& kTextFileFilterStr;
DATA(0x0059ff18) extern const char* const& kUnableToImportTextFmtStr;
DATA(0x005a0fe8) extern const char* const& kTextImportedSuccessfullyFmtStr;
DATA(0x005a1230) extern const char* const& kMapNameStr;
DATA(0x005a1258) extern const char* const& kMapDescriptionStr;
DATA(0x005a0fa8) extern const char* const& kRumorsStr;
DATA(0x005a14a4) extern const char* const& kTimedEventsStr;
DATA(0x005a122c) extern const char* const& kObjectsStr;
DATA(0x005a1808) extern const char* const& kNameStr;
DATA(0x005a1190) extern const char* const& kMessageStr;
DATA(0x005a191c) extern const char* const& kTextStr;
DATA(0x005a0e70) extern const char* const& kVisitingHeroFmtStr;
DATA(0x005a1228) extern const char* const& kEndOfFileStr;
DATA(0x005a13e4) extern const char* const& kTimedEventNameFmtStr;
DATA(0x005a12a8) extern const char* const& kObjectNotFoundStr;
DATA(0x005a05b0) extern const char* const& kToolbarCaptionStr;
DATA(0x005a1aec) extern const char* const& kModeBarCaptionStr;
DATA(0x005a0f34) extern const char* const& kOneWhirlpoolStr;
DATA(0x005a1b00) extern const char* const& kObjectIsUnreachableFmtStr;
DATA(0x005a0850) extern const char* const& kOKStr;
DATA(0x005a0844) extern const char* const& kCancelStr;
DATA(0x005a0f58) extern const char* const& kHelpStr;
DATA(0x005a0a88) extern const char* const& kGeneralPageCaptionStr;
DATA(0x005a05b8) extern const char* const& kContentsPageCaptionStr;
DATA(0x005a06e0) extern const char* const& kResourcesPageCaptionStr;
DATA(0x005a13e8) extern const char* const& kBuildingsPageCaptionStr;
DATA(0x005a132c) extern const char* const& kCreaturesPageCaptionStr;
DATA(0x005a1a58) extern const char* const& kSecondarySkillsPageCaptionStr;
DATA(0x005a1a50) extern const char* const& kSpecialLossConditionPageCaptionStr;
DATA(0x005a0f54) extern const char* const& kPlayerSpecsPageCaptionStr;
DATA(0x005a1398) extern const char* const& kRumorsPageCaptionStr;
DATA(0x005a1afc) extern const char* const& kTeamsPageCaptionStr;
DATA(0x005a156c) extern const char* const& kTimedEventsPageCaptionStr;
DATA(0x005a0dc8) extern const char* const& kSpecialVictoryConditionPageCaptionStr;
DATA(0x005a0550) extern const char* const& kTreasurePageCaptionStr;
DATA(0x005a14e8) extern const char* const& kGarrisonPageCaptionStr;
DATA(0x005a0da4) extern const char* const& kSpellsPageCaptionStr;
DATA(0x005a0ab0) extern const char* const& kGuardiansPageCaptionStr;
DATA(0x005a1168) extern const char* const& kArtifactsPageCaptionStr;
DATA(0x005a0f38) extern const char* const& kAboutBoxCaptionStr;
DATA(0x005a1af0) extern const char* const& kEditArtifactCaptionStr;
DATA(0x005a1810) extern const char* const& kEditCreatureStackCaptionStr;
DATA(0x005a15e8) extern const char* const& kEditRumorCaptionStr;
DATA(0x005a0dcc) extern const char* const& kEditSecondarySkillCaptionStr;
DATA(0x005a13ec) extern const char* const& kFindCaptionStr;
DATA(0x005a0c6c) extern const char* const& kMapValidationCaptionStr;
DATA(0x005a0ff8) extern const char* const& kNewMapCaptionStr;
DATA(0x005a1918) extern const char* const& kOptionsCaptionStr;
DATA(0x005a1914) extern const char* const& kSelectArtifactCaptionStr;
DATA(0x0059ff80) extern const char* const& kSelectHeroClassCaptionStr;
DATA(0x005a0404) extern const char* const& kSelectSpellCaptionStr;
DATA(0x005a1264) extern const char* const& kCustomizeCheckStr;
DATA(0x005a1a18) extern const char* const& kBrushStr;
DATA(0x005a11d8) extern const char* const& kTerrainTypeStr;
DATA(0x005a0f50) extern const char* const& kRiverTypeStr;
DATA(0x005a1570) extern const char* const& kRoadTypeStr;
DATA(0x0059ffa4) extern const char* const& kPlacementRadiusStaticStr;
DATA(0x005a1904) extern const char* const& kStandStillStr;
DATA(0x005a1430) extern const char* const& kRadiusOneSquareStr;
DATA(0x005a0fa4) extern const char* const& kRadiusTwoSquaresStr;
DATA(0x005a17bc) extern const char* const& kRadiusThreeSquaresStr;
DATA(0x005a139c) extern const char* const& kRadiusFourSquaresStr;
DATA(0x005a1130) extern const char* const& kRadiusFiveSquaresStr;
DATA(0x005a0a64) extern const char* const& kRadiusSixSquaresStr;
DATA(0x005a1330) extern const char* const& kRadiusSevenSquaresStr;
DATA(0x005a112c) extern const char* const& kRadiusEightSquaresStr;
DATA(0x005a17d4) extern const char* const& kRadiusNineSquaresStr;
DATA(0x005a0848) extern const char* const& kRadiusTenSquaresStr;
DATA(0x005a1164) extern const char* const& kProblemsStaticStr;
DATA(0x0059ff74) extern const char* const& kNoPropertiesStr;
DATA(0x0059ff0c) extern const char* const& kContinuingWillDeleteUndergroundLayerStr;
DATA(0x005a17d0) extern const char* const& kRandomDwellingFmtStr;
DATA(0x005a0f68) extern const char* const& kRandomTownDwellingFmtStr;
DATA(0x005a04b4) extern const char* const& kBiographyStr;
DATA(0x005a0f28) extern const char* const& kMapGenerationProgressCaptionStr;
DATA(0x005a1004) extern const char* const& kGeneratingMapStr;
DATA(0x005a0a98) extern const char* const& kUnableToGenerateRandomMapStr;
DATA(0x005a1af4) extern const char* const& kHeroesStr;
DATA(0x005a0dc4) extern const char* const& kQuestObjectHasNoQuestFmtStr;
DATA(0x005a0f1c) extern const char* const& kObjectNeedsABOrSoDStr;
DATA(0x005a1af8) extern const char* const& kNoQuestStr;
DATA(0x005a0f3c) extern const char* const& kQuestExperienceLevelFmtStr;
DATA(0x005a0d68) extern const char* const& kQuestPrimarySkillLevelFmtStr;
DATA(0x005a0f10) extern const char* const& kQuestAchieveFmtStr;
DATA(0x005a1910) extern const char* const& kQuestAchieveTheFollowingStr;
DATA(0x005a1944) extern const char* const& kQuestDefeatHeroFmtStr;
DATA(0x005a0c94) extern const char* const& kQuestDefeatMonsterFmtStr;
DATA(0x005a13e0) extern const char* const& kQuestReturnWithTheFollowingStr;
DATA(0x005a05ac) extern const char* const& kQuestReturnWithArtifactFmtStr;
DATA(0x005a11d0) extern const char* const& kQuestReturnWithCreaturesFmtStr;
DATA(0x005a054c) extern const char* const& kQuestReturnWithResourcesFmtStr;
DATA(0x005a0e6c) extern const char* const& kProposalMessageStr;
DATA(0x005a0ed0) extern const char* const& kProgressMessageStr;
DATA(0x005a0fe4) extern const char* const& kCompletionMessageStr;
DATA(0x005a1574) extern const char* const& kAndStr;
DATA(0x005a0938) extern const char* const& kUndergroundStr;
DATA(0x005a0f30) extern const char* const& kNoRewardStr;
DATA(0x005a1604) extern const char* const& kRewardExperienceFmtStr;
DATA(0x0059fee0) extern const char* const& kRewardManaFmtStr;
DATA(0x005a12ac) extern const char* const& kRewardMoraleFmtStr;
DATA(0x005a0548) extern const char* const& kRewardLuckFmtStr;
DATA(0x005a1900) extern const char* const& kRewardSpellFmtStr;
DATA(0x005a0fdc) extern const char* const& kKeymastersTentButNoBorderFmtStr;
DATA(0x005a0ec4) extern const char* const& kBorderGuardButNoKeymastersTentFmtStr;
DATA(0x005a04b0) extern const char* const& kBorderGateButNoKeymastersTentFmtStr;
DATA(0x005a1568) extern const char* const& kHutOfTheMagiButNoEyeStr;
DATA(0x005a1ae8) extern const char* const& kEyeOfTheMagiButNoHutStr;
DATA(0x005a0934) extern const char* const& kTooFewAvailableHeroesFmtStr;
DATA(0x005a0c68) extern const char* const& kAllArtifactsDisabledOrReservedStr;
DATA(0x0059ff28) extern const char* const& kObjectNeedsSoDStr;
DATA(0x005a11c8) extern const char* const& kObjectNeedsABInstalledStr;
DATA(0x005a1194) extern const char* const& kHeroesPageCaptionStr;

// editor.txt "Abandoned Mine Properties Dialog"
namespace SAbandonedMinePropsDlgText {
enum { kNumStrings = 7 };
DATA(0x005a14dc) extern const char* const& kPotentialResourcesStaticStr;
DATA(0x005a14d8) extern const char* const& kRes2CheckStr;
DATA(0x005a14d0) extern const char* const& kRes3CheckStr;
DATA(0x005a14cc) extern const char* const& kRes4CheckStr;
DATA(0x005a14ac) extern const char* const& kRes5CheckStr;
DATA(0x005a14a8) extern const char* const& kRes6CheckStr;
DATA(0x005a14d4) extern const char* const& kRes7CheckStr;
}

// editor.txt "Army Dialog"
namespace SArmyDlgText {
enum { kNumStrings = 3 };
DATA(0x005a12a0) extern const char* const& kTypeStaticStr;
DATA(0x005a12a4) extern const char* const& kQuantityStaticStr;
DATA(0x005a1290) extern const char* const& kSlotStaticFmtStr;
}

// editor.txt "Garrison Properties Dialog"
namespace SGarrisonPropertiesDlgText {
enum { kNumStrings = 5 };
DATA(0x005a1268) extern const char* const& kOwnerStaticStr;
DATA(0x005a1288) extern const char* const& kNoneRadioStr;
DATA(0x005a1270) extern const char* const& kPlayerRadioFmtStr;
DATA(0x005a128c) extern const char* const& kCreaturesStaticStr;
DATA(0x005a126c) extern const char* const& kRemovableCheckStr;
}

// editor.txt "Artifact Properties General Page"
namespace SArtifactPropsGeneralPageText {
enum { kNumStrings = 4 };
DATA(0x005a13d4) extern const char* const& kTypeStaticStr;
DATA(0x005a13dc) extern const char* const& kMessageStaticStr;
DATA(0x005a13d8) extern const char* const& kNoteStaticStr;
DATA(0x005a13d0) extern const char* const& kSpellStaticStr;
}

// editor.txt "Black Box Properties General Page"
namespace SBlackBoxPropsGeneralPageText {
enum { kNumStrings = 1 };
DATA(0x005a1804) extern const char* const& kMessageStaticStr;
}

// editor.txt "Event Properties General Page"
namespace SEventPropsGeneralPageText {
enum { kNumStrings = 4 };
DATA(0x005a0838) extern const char* const& kAllowedPlayersStaticStr;
DATA(0x005a083c) extern const char* const& kPlayerCheckFmtStr;
DATA(0x005a0840) extern const char* const& kAllowComputerCheckStr;
DATA(0x005a0824) extern const char* const& kCancelCheckStr;
}

// editor.txt "Black Box Properties Contents Page"
namespace SBlackBoxPropsContentsPageText {
enum { kNumStrings = 28 };
DATA(0x005a16d8) extern const char* const& kCategoryStaticStr;
DATA(0x005a165c) extern const char* const& kAddButtonStr;
DATA(0x005a1714) extern const char* const& kEditButtonStr;
DATA(0x005a16e4) extern const char* const& kRemoveButtonStr;
DATA(0x005a16ec) extern const char* const& kRemoveAllButtonStr;
DATA(0x005a16fc) extern const char* const& kArtifactsStaticStr;
DATA(0x005a16d4) extern const char* const& kCreatureTypeStaticStr;
DATA(0x005a1708) extern const char* const& kQuantityStaticStr;
DATA(0x005a16dc) extern const char* const& kExperienceBonusStaticStr;
DATA(0x005a1644) extern const char* const& kLuckBonusStaticStr;
DATA(0x005a16e8) extern const char* const& kNoneRadioStr;
DATA(0x005a1704) extern const char* const& kPlusOneRadioStr;
DATA(0x005a171c) extern const char* const& kPlusTwoRadioStr;
DATA(0x005a170c) extern const char* const& kPlusThreeRadioStr;
DATA(0x005a1648) extern const char* const& kMinusOneRadioStr;
DATA(0x005a1660) extern const char* const& kMinusTwoRadioStr;
DATA(0x005a164c) extern const char* const& kMinusThreeRadioStr;
DATA(0x005a1710) extern const char* const& kManaBonusStaticStr;
DATA(0x005a16e0) extern const char* const& kGiveRadioStr;
DATA(0x005a16f4) extern const char* const& kTakeRadioStr;
DATA(0x005a1718) extern const char* const& kMoraleBonusStaticStr;
DATA(0x005a1700) extern const char* const& kAttackSkillStaticStr;
DATA(0x005a16f8) extern const char* const& kDefenseSkillStaticStr;
DATA(0x005a1658) extern const char* const& kSpellPowerStaticStr;
DATA(0x005a16f0) extern const char* const& kKnowledgeStaticStr;
DATA(0x005a1640) extern const char* const& kSecondarySkillStaticStr;
DATA(0x005a1654) extern const char* const& kMasteryStaticStr;
DATA(0x005a1650) extern const char* const& kSpellsStaticStr;
}

// editor.txt "Resource Quantities Dialog"
namespace SResourceQuantitiesDlgText {
enum { kNumStrings = 9 };
DATA(0x005a03c0) extern const char* const& kRes1StaticStr;
DATA(0x005a0394) extern const char* const& kRes2StaticStr;
DATA(0x005a03d4) extern const char* const& kRes3StaticStr;
DATA(0x005a03c4) extern const char* const& kRes4StaticStr;
DATA(0x005a0398) extern const char* const& kRes5StaticStr;
DATA(0x005a03d0) extern const char* const& kRes6StaticStr;
DATA(0x005a03c8) extern const char* const& kRes7StaticStr;
DATA(0x005a03cc) extern const char* const& kGiveRadioStr;
DATA(0x005a03d8) extern const char* const& kTakeRadioStr;
}

// editor.txt "Edit Secondary Skill Dialog"
namespace SEditSecondarySkillDlgText {
enum { kNumStrings = 5 };
DATA(0x005a0d58) extern const char* const& kTypeStaticStr;
DATA(0x005a0d3c) extern const char* const& kMasteryStaticStr;
DATA(0x005a0d60) extern const char* const& kBasicRadioStr;
DATA(0x005a0d5c) extern const char* const& kAdvancedRadioStr;
DATA(0x005a0d40) extern const char* const& kExpertRadioStr;
}

// editor.txt "Edit Creature Stack Dialog"
namespace SEditCreatureStackDlgText {
enum { kNumStrings = 2 };
DATA(0x005a1ad4) extern const char* const& kTypeStaticStr;
DATA(0x005a1ad8) extern const char* const& kQuantityStaticStr;
}

// editor.txt "Edit Timed Event General Page"
namespace SEditTimedEventGeneralPageText {
enum { kNumStrings = 8 };
DATA(0x005a136c) extern const char* const& kEventNameStaticStr;
DATA(0x005a1370) extern const char* const& kMessageStaticStr;
DATA(0x005a1364) extern const char* const& kAllowedPlayersStaticStr;
DATA(0x005a1378) extern const char* const& kPlayerCheckFmtStr;
DATA(0x005a1380) extern const char* const& kAllowComputerCheckStr;
DATA(0x005a1374) extern const char* const& kFirstOccurenceStaticStr;
DATA(0x005a137c) extern const char* const& kSubsequentOccurenceStaticStr;
DATA(0x005a1368) extern const char* const& kAllowHumanCheckStr;
}

// editor.txt "Edit Town Event Buildings Page"
namespace SEditTownEventBuildingsPageText {
enum { kNumStrings = 2 };
DATA(0x005a0a78) extern const char* const& kBuildingTreeStaticStr;
DATA(0x005a0a7c) extern const char* const& kBuildCheckStr;
}

// editor.txt "Edit Town Event Creatures Page"
namespace SEditTownEventCreaturesPageText {
enum { kNumStrings = 3 };
DATA(0x005a1420) extern const char* const& kGeneratorNoStaticStr;
DATA(0x005a1428) extern const char* const& kQuantityStaticStr;
DATA(0x005a1424) extern const char* const& kNoteStaticStr;
}

// editor.txt "Edit Artifact Dialog"
namespace SEditArtifactDlgText {
enum { kNumStrings = 2 };
DATA(0x005a0f00) extern const char* const& kEquipWhereStaticStr;
DATA(0x005a0f0c) extern const char* const& kArtifactStaticStr;
}

// editor.txt "Edit Rumor Dialog"
namespace SEditRumorDlgText {
enum { kNumStrings = 2 };
DATA(0x005a18a0) extern const char* const& kNameStaticStr;
DATA(0x005a189c) extern const char* const& kTextStaticStr;
}

// editor.txt "Find Dialog"
namespace SFindDlgText {
enum { kNumStrings = 3 };
DATA(0x005a06c0) extern const char* const& kFindWhatStaticStr;
DATA(0x005a06bc) extern const char* const& kFindNextButtonStr;
DATA(0x005a06d0) extern const char* const& kFindPrevButtonStr;
}

// editor.txt "Flaggable Properties Dialog"
namespace SFlaggablePropsDlgText {
enum { kNumStrings = 3 };
DATA(0x005a15ec) extern const char* const& kOwnerStaticStr;
DATA(0x005a1600) extern const char* const& kNoneRadioStr;
DATA(0x005a15f0) extern const char* const& kPlayerRadioFmtStr;
}

// editor.txt "Hero Properties General Page"
namespace SHeroPropsGeneralPageText {
enum { kNumStrings = 16 };
DATA(0x005a062c) extern const char* const& kClassStaticStr;
DATA(0x005a05c4) extern const char* const& kPlayerStaticStr;
DATA(0x005a05c0) extern const char* const& kIdentityStaticStr;
DATA(0x005a0618) extern const char* const& kNameStaticStr;
DATA(0x005a0620) extern const char* const& kExperienceStaticStr;
DATA(0x005a0634) extern const char* const& kPortraitStaticStr;
DATA(0x005a05c8) extern const char* const& kPatrolStaticStr;
DATA(0x005a0630) extern const char* const& kCustomizeNameCheckStr;
DATA(0x005a0628) extern const char* const& kCustomizePortraitCheckStr;
DATA(0x005a0624) extern const char* const& kGenderStaticStr;
DATA(0x005a0614) extern const char* const& kDefaultGenderRadioFmtStr;
DATA(0x005a05cc) extern const char* const& kMaleStr;
DATA(0x005a061c) extern const char* const& kFemaleStr;
DATA(0x005a0610) extern const char* const& kMaleRadioStr;
DATA(0x005a0638) extern const char* const& kFemaleRadioStr;
DATA(0x005a05bc) extern const char* const& kCustomizeGenderCheckStr;
}

// editor.txt "Hero Properties Creatures Page"
namespace SHeroPropsCreaturesPageText {
enum { kNumStrings = 3 };
DATA(0x005a1224) extern const char* const& kFormationStaticStr;
DATA(0x005a1220) extern const char* const& kSpreadRadioStr;
DATA(0x005a121c) extern const char* const& kGroupedRadioStr;
}

// editor.txt "Secondary Skills Dialog"
namespace SHeroPropsSecSkillsPageText {
enum { kNumStrings = 3 };
DATA(0x005a03f8) extern const char* const& kTypeStaticStr;
DATA(0x005a03e8) extern const char* const& kMasteryStaticStr;
DATA(0x005a03e4) extern const char* const& kSkillStaticFmtStr;
}

// editor.txt "Hero Properties Artifacts Page"
namespace SHeroPropsArtifactsPageText {
enum { kNumStrings = 8 };
DATA(0x005a0d2c) extern const char* const& kHasSpellbookCheckStr;
DATA(0x005a0d34) extern const char* const& kArtifactsStaticStr;
DATA(0x005a0d24) extern const char* const& kNameColumnStr;
DATA(0x005a0cfc) extern const char* const& kLocationColumnStr;
DATA(0x005a0d00) extern const char* const& kAddButtonStr;
DATA(0x005a0d38) extern const char* const& kEditButtonStr;
DATA(0x005a0d28) extern const char* const& kRemoveButtonStr;
DATA(0x005a0d30) extern const char* const& kRemoveAllButtonStr;
}

// editor.txt "Map Specs General Page"
namespace SMapSpecsGeneralPageText {
enum { kNumStrings = 14 };
DATA(0x005a1820) extern const char* const& kDifficultyStaticStr;
DATA(0x005a1888) extern const char* const& kEasyRadioStr;
DATA(0x005a1824) extern const char* const& kNormalRadioStr;
DATA(0x005a186c) extern const char* const& kToughRadioStr;
DATA(0x005a1830) extern const char* const& kExpertRadioStr;
DATA(0x005a1884) extern const char* const& kImpossibleRadioStr;
DATA(0x005a187c) extern const char* const& kTwoLevelMapCheckStr;
DATA(0x005a181c) extern const char* const& kMapNameStaticStr;
DATA(0x005a1828) extern const char* const& kDescriptionStaticStr;
DATA(0x005a1878) extern const char* const& kLimitHeroLevelCheckStr;
DATA(0x005a1870) extern const char* const& kMapVersionStaticStr;
DATA(0x005a1880) extern const char* const& kRoERadioStr;
DATA(0x005a182c) extern const char* const& kABRadioStr;
DATA(0x005a1874) extern const char* const& kSoDRadioStr;
}

// editor.txt "Map Specs Player Specs Page"
namespace SMapSpecsPlayerSpecsPageText {
enum { kNumStrings = 19 };
DATA(0x005a173c) extern const char* const& kPlayerStaticStr;
DATA(0x005a1738) extern const char* const& kGenerateHeroCheckStr;
DATA(0x005a179c) extern const char* const& kPlayabilityStaticStr;
DATA(0x005a17b0) extern const char* const& kHumanPlayableCheckStr;
DATA(0x005a1724) extern const char* const& kComputerPlayableCheckStr;
DATA(0x005a17a0) extern const char* const& kBehaviorStaticStr;
DATA(0x005a1734) extern const char* const& kHasMainTownCheckStr;
DATA(0x005a17ac) extern const char* const& kAllowedAlignmentsStaticStr;
DATA(0x005a1730) extern const char* const& kCustomizeCheckStr;
DATA(0x005a17a4) extern const char* const& kRandomTownCheckStr;
DATA(0x005a1794) extern const char* const& kCastleCheckStr;
DATA(0x005a1728) extern const char* const& kRampartCheckStr;
DATA(0x005a1744) extern const char* const& kTowerCheckStr;
DATA(0x005a1740) extern const char* const& kInfernoCheckStr;
DATA(0x005a1798) extern const char* const& kNecropolisCheckStr;
DATA(0x005a17a8) extern const char* const& kDungeonCheckStr;
DATA(0x005a17b8) extern const char* const& kStrongholdCheckStr;
DATA(0x005a17b4) extern const char* const& kFortressCheckStr;
DATA(0x005a172c) extern const char* const& kConfluxCheckStr;
}

// editor.txt "Map Specs Teams Page"
namespace SMapSpecsTeamsPageText {
enum { kNumStrings = 5 };
DATA(0x005a0bc0) extern const char* const& kEnableTeamsCheckStr;
DATA(0x005a0bbc) extern const char* const& kNumberOfTeamsStaticStr;
DATA(0x005a0ba4) extern const char* const& kTeamAssignmentsStaticStr;
DATA(0x005a0ba0) extern const char* const& kTeamNumberStaticStr;
DATA(0x005a0bc4) extern const char* const& kPlayerStaticFmtStr;
}

// editor.txt "Map Specs Rumors Page"
namespace SMapSpecsRumorsPageText {
enum { kNumStrings = 5 };
DATA(0x005a1a44) extern const char* const& kRumorsStaticStr;
DATA(0x005a1a28) extern const char* const& kAddButtonStr;
DATA(0x005a1a4c) extern const char* const& kEditButtonStr;
DATA(0x005a1a40) extern const char* const& kRemoveButtonStr;
DATA(0x005a1a48) extern const char* const& kRemoveAllButtonStr;
}

// editor.txt "Map Specs Timed Events Page"
namespace SMapSpecsTimedEventsPageText {
enum { kNumStrings = 7 };
DATA(0x005a1638) extern const char* const& kEventsStaticStr;
DATA(0x005a1608) extern const char* const& kAddButtonStr;
DATA(0x005a163c) extern const char* const& kEditButtonStr;
DATA(0x005a162c) extern const char* const& kRemoveButtonStr;
DATA(0x005a1634) extern const char* const& kRemoveAllButtonStr;
DATA(0x005a1630) extern const char* const& kMoveUpButtonStr;
DATA(0x005a1628) extern const char* const& kMoveDownButtonStr;
}

// editor.txt "Map Specs Special Victory Condition Page"
namespace SMapSpecsVictoryCondPageText {
enum { kNumStrings = 31 };
DATA(0x005a1048) extern const char* const& kSelectVictoryConditionStaticStr;
DATA(0x005a1100) extern const char* const& kNoneRadioStr;
DATA(0x005a103c) extern const char* const& kAquireArtifactRadioStr;
DATA(0x005a1108) extern const char* const& kAccumCreaturesRadioStr;
DATA(0x005a1110) extern const char* const& kAccumResourcesRadioStr;
DATA(0x005a10e0) extern const char* const& kUpgradeTownRadioStr;
DATA(0x005a1104) extern const char* const& kBuildGrailRadioStr;
DATA(0x005a1034) extern const char* const& kDefeatHeroRadioStr;
DATA(0x005a111c) extern const char* const& kCaptureTownRadioStr;
DATA(0x005a1128) extern const char* const& kDefeatMonsterRadioStr;
DATA(0x005a10d8) extern const char* const& kFlagGeneratorsRadioStr;
DATA(0x005a1124) extern const char* const& kFlagMinesRadioStr;
DATA(0x005a10ec) extern const char* const& kTransportArtifactRadioStr;
DATA(0x005a1114) extern const char* const& kNormalVictoryCheckStr;
DATA(0x005a10fc) extern const char* const& kComputerAlsoCheckStr;
DATA(0x005a10dc) extern const char* const& kArtifactStaticStr;
DATA(0x005a10d4) extern const char* const& kCreatureTypeStaticStr;
DATA(0x005a1118) extern const char* const& kQuantityStaticStr;
DATA(0x005a10d0) extern const char* const& kResourceTypeStaticStr;
DATA(0x005a10f8) extern const char* const& kTownStaticStr;
DATA(0x005a10f0) extern const char* const& kHallLevelStaticStr;
DATA(0x005a10e8) extern const char* const& kTownHallRadioStr;
DATA(0x005a10f4) extern const char* const& kCityHallRadioStr;
DATA(0x005a104c) extern const char* const& kCapitolHallRadioStr;
DATA(0x005a110c) extern const char* const& kCastleLevelStaticStr;
DATA(0x005a1120) extern const char* const& kFortCastleRadioStr;
DATA(0x005a1050) extern const char* const& kCitadelCastleRadioStr;
DATA(0x005a1038) extern const char* const& kCastleCastleRadioStr;
DATA(0x005a10e4) extern const char* const& kHeroStaticStr;
DATA(0x005a1044) extern const char* const& kMonsterStaticStr;
DATA(0x005a1040) extern const char* const& kDestinationStaticStr;
}

// editor.txt "Map Specs Special Loss Condition Page"
namespace SMapSpecsLossCondPageText {
enum { kNumStrings = 8 };
DATA(0x005a0488) extern const char* const& kSelectLossConditionStaticStr;
DATA(0x005a047c) extern const char* const& kNoneRadioStr;
DATA(0x005a044c) extern const char* const& kLoseTownRadioStr;
DATA(0x005a0470) extern const char* const& kLoseHeroRadioStr;
DATA(0x005a0480) extern const char* const& kTimeExpiresRadioStr;
DATA(0x005a0478) extern const char* const& kTownStaticStr;
DATA(0x005a0474) extern const char* const& kHeroStaticStr;
DATA(0x005a0484) extern const char* const& kTimeLimitStaticStr;
}

// editor.txt "Monster Properties General Page"
namespace SMonsterPropsGeneralPageText {
enum { kNumStrings = 13 };
DATA(0x005a12f4) extern const char* const& kTypeStaticStr;
DATA(0x005a1318) extern const char* const& kQuantityStaticStr;
DATA(0x005a1310) extern const char* const& kRandomQtyRadioStr;
DATA(0x005a1300) extern const char* const& kCustomQtyRadioStr;
DATA(0x005a130c) extern const char* const& kNeverGrowsCheckStr;
DATA(0x005a1304) extern const char* const& kDispositionStaticStr;
DATA(0x005a12f8) extern const char* const& kDispositionCompliantRadioStr;
DATA(0x005a12f0) extern const char* const& kDispositionFriendlyRadioStr;
DATA(0x005a12ec) extern const char* const& kDispositionAggressiveRadioStr;
DATA(0x005a1308) extern const char* const& kDispositionHostileRadioStr;
DATA(0x005a12b4) extern const char* const& kDispositionSavageRadioStr;
DATA(0x005a1314) extern const char* const& kNeverFleesCheckStr;
DATA(0x005a12fc) extern const char* const& kMessageStaticStr;
}

// editor.txt "Monster Properties Treasure Page"
namespace SMonsterPropsTreasurePageText {
enum { kNumStrings = 9 };
DATA(0x005a0dd4) extern const char* const& kResourcesStaticStr;
DATA(0x005a0e04) extern const char* const& kRes1StaticStr;
DATA(0x005a0dd8) extern const char* const& kRes2StaticStr;
DATA(0x005a0e18) extern const char* const& kRes3StaticStr;
DATA(0x005a0e08) extern const char* const& kRes4StaticStr;
DATA(0x005a0ddc) extern const char* const& kRes5StaticStr;
DATA(0x005a0e14) extern const char* const& kRes6StaticStr;
DATA(0x005a0e0c) extern const char* const& kRes7StaticStr;
DATA(0x005a0e10) extern const char* const& kArtifactStaticStr;
}

// editor.txt "New Map Dialog"
namespace SNewMapDlgText {
enum { kNumStrings = 25 };
DATA(0x005a09b0) extern const char* const& kMapSizeStaticStr;
DATA(0x005a0a4c) extern const char* const& k36x36RadioStr;
DATA(0x005a099c) extern const char* const& k72x72RadioStr;
DATA(0x005a0a60) extern const char* const& k108x108RadioStr;
DATA(0x005a0a38) extern const char* const& k144x144RadioStr;
DATA(0x005a0a44) extern const char* const& kTwoLevelMapCheckStr;
DATA(0x005a0a24) extern const char* const& kMapVersionStaticStr;
DATA(0x005a0a48) extern const char* const& kRoERadioStr;
DATA(0x005a09ac) extern const char* const& kABRadioStr;
DATA(0x005a0a50) extern const char* const& kGenerateRandomMapCheckStr;
DATA(0x005a0a20) extern const char* const& kNumberOfPlayersStaticStr;
DATA(0x005a0a30) extern const char* const& kHumanOrComputerStaticStr;
DATA(0x005a0a54) extern const char* const& kComputerOnlyStaticStr;
DATA(0x005a0a40) extern const char* const& kNumberOfTeamsStaticStr;
DATA(0x005a0a58) extern const char* const& kWaterContentStaticStr;
DATA(0x005a0a3c) extern const char* const& kWaterRandomRadioStr;
DATA(0x005a09a0) extern const char* const& kWaterNoneRadioStr;
DATA(0x005a0a5c) extern const char* const& kWaterNormalRadioStr;
DATA(0x005a09b8) extern const char* const& kWaterIslandsRadioStr;
DATA(0x005a0a34) extern const char* const& kMonsterStrengthStaticStr;
DATA(0x005a09a8) extern const char* const& kMonsterRandomRadioStr;
DATA(0x005a09b4) extern const char* const& kMonsterWeakRadioStr;
DATA(0x005a0a2c) extern const char* const& kMonsterNormalRadioStr;
DATA(0x005a09a4) extern const char* const& kMonsterStrongRadioStr;
DATA(0x005a0a28) extern const char* const& kSoDRadioStr;
}

// editor.txt "Options Dialog"
namespace SOptionsDlgText {
enum { kNumStrings = 8 };
DATA(0x005a1528) extern const char* const& kTileFrequencyStaticStr;
DATA(0x005a155c) extern const char* const& kNoneStaticStr;
DATA(0x005a1564) extern const char* const& kLotsStaticStr;
DATA(0x005a152c) extern const char* const& kRepaintCheckStr;
DATA(0x005a1554) extern const char* const& kAutosaveOptionsStaticStr;
DATA(0x005a1530) extern const char* const& kEnableAutosaveCheckStr;
DATA(0x005a1560) extern const char* const& kAutosaveEveryStaticStr;
DATA(0x005a1558) extern const char* const& kMinutesStaticStr;
}

// editor.txt "Resource Properties General Page"
namespace SResourcePropsGeneralPageText {
enum { kNumStrings = 7 };
DATA(0x005a0980) extern const char* const& kTypeStaticStr;
DATA(0x005a0994) extern const char* const& kQuantityStaticStr;
DATA(0x005a0990) extern const char* const& kRandomQtyRadioStr;
DATA(0x005a098c) extern const char* const& kCustomQtyRadioStr;
DATA(0x005a0960) extern const char* const& kQuantityNoteStaticStr;
DATA(0x005a0988) extern const char* const& kMessageStaticStr;
DATA(0x005a0984) extern const char* const& kNoteStaticStr;
}

// editor.txt "Scholar Properties Dialog"
namespace SScholarPropsDlgText {
enum { kNumStrings = 5 };
DATA(0x005a100c) extern const char* const& kRewardStaticStr;
DATA(0x005a1008) extern const char* const& kRandomRewardRadioStr;
DATA(0x005a102c) extern const char* const& kPriSkillRewardRadioStr;
DATA(0x005a1028) extern const char* const& kSecSkillRewardRadioStr;
DATA(0x005a1024) extern const char* const& kSpellRewardRadioStr;
}

// editor.txt "Seer's Hut Properties Dialog"
namespace SSeersHutPropsDlgText {
enum { kNumStrings = 11 };
DATA(0x005a0cac) extern const char* const& kQuestStaticStr;
DATA(0x005a0ca0) extern const char* const& kEditQuestButtonStr;
DATA(0x005a0ca8) extern const char* const& kHasDeadlineCheckStr;
DATA(0x005a0cf0) extern const char* const& kMonthStaticStr;
DATA(0x005a0cec) extern const char* const& kWeekStaticStr;
DATA(0x005a0ca4) extern const char* const& kDayStaticStr;
DATA(0x005a0ce0) extern const char* const& kCustomizeCheckStr;
DATA(0x005a0cf4) extern const char* const& kRewardStaticStr;
DATA(0x005a0ce4) extern const char* const& kEditRewardButtonStr;
DATA(0x005a0ce8) extern const char* const& kBeHeroFmtStr;
DATA(0x005a0cb0) extern const char* const& kBelongToPlayerFmtStr;
}

// editor.txt "Shrine Properties Dialog"
namespace SShrinePropsDlgText {
enum { kNumStrings = 3 };
DATA(0x005a0498) extern const char* const& kSpellStaticFmtStr;
DATA(0x005a04ac) extern const char* const& kRandomSpellRadioStr;
DATA(0x005a04a8) extern const char* const& kCustomSpellRadioStr;
}

// editor.txt "Sign Properties Dialog"
namespace SSignPropsDlgText {
enum { kNumStrings = 1 };
DATA(0x005a0ab8) extern const char* const& kMessageStaticStr;
}

// editor.txt "Town Properties General Page"
namespace STownPropsGeneralPageText {
enum { kNumStrings = 13 };
DATA(0x005a0c10) extern const char* const& kTownTypeStaticStr;
DATA(0x005a0bd8) extern const char* const& kPlayerStaticStr;
DATA(0x005a0bcc) extern const char* const& kTownNameStaticStr;
DATA(0x005a0c2c) extern const char* const& kCustomizeCheckStr;
DATA(0x005a0c30) extern const char* const& kVisitingHeroStaticStr;
DATA(0x005a0c20) extern const char* const& kHeroNameStaticStr;
DATA(0x005a0bd0) extern const char* const& kHeroClassStaticStr;
DATA(0x005a0bd4) extern const char* const& kAddButtonStr;
DATA(0x005a0c28) extern const char* const& kEditButtonStr;
DATA(0x005a0c14) extern const char* const& kRemoveButtonStr;
DATA(0x005a0c18) extern const char* const& kAlignmentStaticStr;
DATA(0x005a0c24) extern const char* const& kSameAsTownFmtStr;
DATA(0x005a0c1c) extern const char* const& kSameAsOwnerOrRandomStr;
}

// editor.txt "Town Properties Garrison Page"
namespace STownPropsGarrisonPageText {
enum { kNumStrings = 3 };
DATA(0x005a0d9c) extern const char* const& kFormationStaticStr;
DATA(0x005a0d98) extern const char* const& kSpreadRadioStr;
DATA(0x005a0d94) extern const char* const& kGroupedRadioStr;
}

// editor.txt "Town Properties Buildings Page"
namespace STownPropsBuildingsPageText {
enum { kNumStrings = 6 };
DATA(0x005a0c58) extern const char* const& kHasFortCheckStr;
DATA(0x005a0c38) extern const char* const& kBuildingTreeStaticStr;
DATA(0x005a0c64) extern const char* const& kEnabledCheckStr;
DATA(0x005a0c54) extern const char* const& kBuiltCheckStr;
DATA(0x005a0c60) extern const char* const& kBuildAllButtonStr;
DATA(0x005a0c5c) extern const char* const& kDemolishAllButtonStr;
}

// editor.txt "Town Properties Spells Page"
namespace STownPropsSpellsPageText {
enum { kNumStrings = 3 };
DATA(0x005a1148) extern const char* const& kSpellLevelStaticStr;
DATA(0x005a1158) extern const char* const& kMustAppearStaticStr;
DATA(0x005a115c) extern const char* const& kMayAppearStaticStr;
}

// editor.txt "Town Properties Timed Events Page"
namespace STownPropsTimedEventsPageText {
enum { kNumStrings = 7 };
DATA(0x005a0f9c) extern const char* const& kEventsStaticStr;
DATA(0x005a0f6c) extern const char* const& kAddButtonStr;
DATA(0x005a0fa0) extern const char* const& kEditButtonStr;
DATA(0x005a0f90) extern const char* const& kRemoveButtonStr;
DATA(0x005a0f98) extern const char* const& kRemoveAllButtonStr;
DATA(0x005a0f94) extern const char* const& kMoveUpButtonStr;
DATA(0x005a0f8c) extern const char* const& kMoveDownButtonStr;
}

// editor.txt "Edit Quest Dialog"
namespace SEditQuestDlgText {
enum { kNumStrings = 36 };
DATA(0x005a07dc) extern const char* const& kCaptionStr;
DATA(0x005a07bc) extern const char* const& kSelectQuestTypeStaticStr;
DATA(0x005a07e0) extern const char* const& kNoneRadioStr;
DATA(0x005a07d0) extern const char* const& kExperienceLevelRadioStr;
DATA(0x005a07b0) extern const char* const& kPrimarySkillLevelRadioStr;
DATA(0x005a06fc) extern const char* const& kDefeatHeroRadioStr;
DATA(0x005a0810) extern const char* const& kDefeatMonsterRadioStr;
DATA(0x005a07b8) extern const char* const& kArtifactsRadioStr;
DATA(0x005a0700) extern const char* const& kCreaturesRadioStr;
DATA(0x005a07e8) extern const char* const& kResourcesRadioStr;
DATA(0x005a07f8) extern const char* const& kExperienceLevelStaticStr;
DATA(0x005a07c8) extern const char* const& kAttackSkillStaticStr;
DATA(0x005a0814) extern const char* const& kDefenseSkillStaticStr;
DATA(0x005a07ac) extern const char* const& kSpellPowerStaticStr;
DATA(0x005a0718) extern const char* const& kKnowledgeStaticStr;
DATA(0x005a0714) extern const char* const& kHeroStaticStr;
DATA(0x005a07c0) extern const char* const& kMonsterStaticStr;
DATA(0x005a07f4) extern const char* const& kArtifactsStaticStr;
DATA(0x005a0710) extern const char* const& kAddButtonStr;
DATA(0x005a0800) extern const char* const& kEditButtonStr;
DATA(0x005a07cc) extern const char* const& kRemoveButtonStr;
DATA(0x005a07e4) extern const char* const& kRemoveAllButtonStr;
DATA(0x005a06f8) extern const char* const& kCreatureTypeStaticStr;
DATA(0x005a0704) extern const char* const& kQuantityStaticStr;
DATA(0x005a07c4) extern const char* const& kRes0StaticStr;
DATA(0x005a0804) extern const char* const& kRes1StaticStr;
DATA(0x005a07ec) extern const char* const& kRes2StaticStr;
DATA(0x005a07fc) extern const char* const& kRes3StaticStr;
DATA(0x005a07d4) extern const char* const& kRes4StaticStr;
DATA(0x005a0708) extern const char* const& kRes5StaticStr;
DATA(0x005a080c) extern const char* const& kRes6StaticStr;
DATA(0x005a070c) extern const char* const& kBeHeroRadioStr;
DATA(0x005a07f0) extern const char* const& kBelongToPlayerRadioStr;
DATA(0x005a07d8) extern const char* const& kClassStaticStr;
DATA(0x005a0808) extern const char* const& kIdentityStaticStr;
DATA(0x005a07b4) extern const char* const& kPlayerStaticStr;
}

// editor.txt "Edit Reward Dialog"
namespace SEditRewardDlgText {
enum { kNumStrings = 26 };
DATA(0x005a19f0) extern const char* const& kCaptionStr;
DATA(0x005a195c) extern const char* const& kSelectRewardTypeStaticStr;
DATA(0x005a19f4) extern const char* const& kNoneRadioStr;
DATA(0x005a1a14) extern const char* const& kExperienceRadioStr;
DATA(0x005a1950) extern const char* const& kManaRadioStr;
DATA(0x005a19c8) extern const char* const& kMoraleRadioStr;
DATA(0x005a19d4) extern const char* const& kLuckRadioStr;
DATA(0x005a19f8) extern const char* const& kResourceRadioStr;
DATA(0x005a1a04) extern const char* const& kPrimarySkillRadioStr;
DATA(0x005a1a0c) extern const char* const& kSecondarySkillRadioStr;
DATA(0x005a19fc) extern const char* const& kArtifactRadioStr;
DATA(0x005a19d0) extern const char* const& kSpellRadioStr;
DATA(0x005a1954) extern const char* const& kCreaturesRadioStr;
DATA(0x005a19ec) extern const char* const& kExperienceBonusStaticStr;
DATA(0x005a19e4) extern const char* const& kManaBonusStaticStr;
DATA(0x005a1a08) extern const char* const& kMoraleBonusStaticStr;
DATA(0x005a1a10) extern const char* const& kLuckBonusStaticStr;
DATA(0x005a19cc) extern const char* const& kResourceTypeStaticStr;
DATA(0x005a194c) extern const char* const& kQuantityStaticStr;
DATA(0x005a1958) extern const char* const& kPrimarySkillStaticStr;
DATA(0x005a19dc) extern const char* const& kBonusStaticStr;
DATA(0x005a19e8) extern const char* const& kSecondarySkillStaticStr;
DATA(0x005a19e0) extern const char* const& kMasteryStaticStr;
DATA(0x005a1a00) extern const char* const& kArtifactStaticStr;
DATA(0x005a19d8) extern const char* const& kSpellStaticStr;
DATA(0x005a1948) extern const char* const& kCreatureTypeStaticStr;
}

// editor.txt "Hero Placeholder Properties Dialog"
namespace SHeroPlaceholderPropsDlgText {
enum { kNumStrings = 7 };
DATA(0x005a14f0) extern const char* const& kOwnerStaticStr;
DATA(0x005a151c) extern const char* const& kIdentityStaticStr;
DATA(0x005a1510) extern const char* const& kAnyHeroRadioStr;
DATA(0x005a1520) extern const char* const& kSpecificHeroRadioStr;
DATA(0x005a1514) extern const char* const& kPowerRatingStaticStr;
DATA(0x005a1518) extern const char* const& kClassStaticStr;
DATA(0x005a1524) extern const char* const& kHeroIdentityStaticStr;
}

// editor.txt "Hero Properties Biography Page"
namespace SHeroPropsBiographyPageText {
enum { kNumStrings = 2 };
DATA(0x005a17c8) extern const char* const& kCaptionStr;
DATA(0x005a17cc) extern const char* const& kCustomizeCheckStr;
}

// editor.txt "Map Specs Artifacts Page"
namespace SMapSpecsArtifactsPageText {
enum { kNumStrings = 1 };
DATA(0x005a0b9c) extern const char* const& kEnabledArtifactsStaticStr;
}

// editor.txt "Map Specs Available Heroes Page"
namespace SMapSpecsAvailableHeroesPageText {
enum { kNumStrings = 2 };
DATA(0x0059ff04) extern const char* const& kAvailableHeroesStaticStr;
DATA(0x0059ff00) extern const char* const& kPropertiesButtonStr;
}

// editor.txt "Random Generator Properties Dialog"
namespace SRandomGeneratorPropsDlgText {
enum { kNumStrings = 16 };
DATA(0x005a0648) extern const char* const& kOwnerStaticStr;
DATA(0x005a06b4) extern const char* const& kCreatureLevelStaticStr;
DATA(0x005a0694) extern const char* const& kMinimumStaticStr;
DATA(0x005a0644) extern const char* const& kMaximumStaticStr;
DATA(0x005a06a0) extern const char* const& kAlignmentStaticStr;
DATA(0x005a06a8) extern const char* const& kSelectOneOfRadioStr;
DATA(0x005a06b8) extern const char* const& kSameAsRadioStr;
DATA(0x005a06b0) extern const char* const& kCastleCheckStr;
DATA(0x005a0640) extern const char* const& kRampartCheckStr;
DATA(0x005a063c) extern const char* const& kTowerCheckStr;
DATA(0x005a068c) extern const char* const& kInfernoCheckStr;
DATA(0x005a0690) extern const char* const& kNecropolisCheckStr;
DATA(0x005a069c) extern const char* const& kDungeonCheckStr;
DATA(0x005a0698) extern const char* const& kStrongholdCheckStr;
DATA(0x005a06a4) extern const char* const& kFortressCheckStr;
DATA(0x005a06ac) extern const char* const& kConfluxCheckStr;
}

// editor.txt "Witch Hut Properties Dialog"
namespace SWitchHutPropsDlgText {
enum { kNumStrings = 1 };
DATA(0x005a0a90) extern const char* const& kPotentialSkillsStaticStr;
}

// editor.txt "Hero Properties Primary Skils Page"
namespace SHeroPropsPriSkillsPageText {
enum { kNumStrings = 2 };
DATA(0x005a0598) extern const char* const& kCaptionStr;
DATA(0x005a059c) extern const char* const& kCustomizeCheckStr;
}

// editor.txt "Hero Properties Spells Page"
namespace SHeroPropsSpellsPageText {
enum { kNumStrings = 2 };
DATA(0x005a118c) extern const char* const& kCaptionStr;
DATA(0x005a1188) extern const char* const& kCustomizeCheckStr;
}

// editor.txt "Hero Prototype Properties Artifacts Page"
namespace SHeroPrototypePropsArtifactsPageText {
enum { kNumStrings = 1 };
DATA(0x005a1818) extern const char* const& kCaptionStr;
}

// editor.txt "Hero Prototype Properties Biography Page"
namespace SHeroPrototypePropsBiographyPageText {
enum { kNumStrings = 1 };
DATA(0x005a1260) extern const char* const& kCaptionStr;
}

// editor.txt "Hero Prototype Properties General Page"
namespace SHeroPrototypePropsGeneralPageText {
enum { kNumStrings = 3 };
DATA(0x005a0b10) extern const char* const& kCaptionStr;
DATA(0x005a0b0c) extern const char* const& kMayBeHiredByStaticStr;
DATA(0x005a0b14) extern const char* const& kPlayerCheckFmtStr;
}

// editor.txt "Hero Prototype Properties Primary Skills Page"
namespace SHeroPrototypePropsPriSkillsPageText {
enum { kNumStrings = 1 };
DATA(0x005a1388) extern const char* const& kCaptionStr;
}

// editor.txt "Hero Prototype Properties Secondary Skills Page"
namespace SHeroPrototypePropsSecSkillsPageText {
enum { kNumStrings = 1 };
DATA(0x005a0fb0) extern const char* const& kCaptionStr;
}

// editor.txt "Hero Prototype Properties Sheet"
namespace SHeroPrototypePropsSheetText {
enum { kNumStrings = 1 };
DATA(0x005a0efc) extern const char* const& kCaptionStr;
}

// editor.txt "Hero Prototype Properties Spells Page"
namespace SHeroPrototypePropsSpellsPageText {
enum { kNumStrings = 1 };
DATA(0x005a1140) extern const char* const& kCaptionStr;
}

// editor.txt "Primary Skills Dialog"
namespace SPrimarySkillsDlgText {
enum { kNumStrings = 4 };
DATA(0x005a0444) extern const char* const& kAttackSkillStaticStr;
DATA(0x005a0448) extern const char* const& kDefenseSkillStaticStr;
DATA(0x005a0440) extern const char* const& kSpellPowerStaticStr;
DATA(0x005a042c) extern const char* const& kKnowledgeStaticStr;
}

// editor.txt "Spells Dialog"
namespace SSpellsDlgText {
enum { kNumStrings = 1 };
DATA(0x005a0fc8) extern const char* const& kSpellsStaticStr;
}

// editor.txt "Map Specs Secondary Skills Page"
namespace SMapSpecsSecSkillsPageText {
enum { kNumStrings = 2 };
DATA(0x005a0f48) extern const char* const& kCaptionStr;
DATA(0x005a0f4c) extern const char* const& kAvailableSecSkillsStaticStr;
}

// editor.txt "Map Specs Spells Page"
namespace SMapSpecsSpellsPageText {
enum { kNumStrings = 2 };
DATA(0x005a06f0) extern const char* const& kCaptionStr;
DATA(0x005a06f4) extern const char* const& kAvailableSpellsStaticStr;
}

// editor.txt "Main Menu"
namespace SMainMenuText {
enum { kNumStrings = 6 };
DATA(0x005a055c) extern const char* const& kFileStr;
DATA(0x005a057c) extern const char* const& kEditStr;
DATA(0x005a0580) extern const char* const& kViewStr;
DATA(0x005a0584) extern const char* const& kToolsStr;
DATA(0x005a0560) extern const char* const& kPlayerStr;
DATA(0x005a0558) extern const char* const& kHelpStr;
}

// editor.txt "File Menu"
namespace SFileMenuText {
enum { kNumStrings = 8 };
DATA(0x005a1464) extern const char* const& kNewStr;
DATA(0x005a1488) extern const char* const& kOpenStr;
DATA(0x005a1498) extern const char* const& kSaveStr;
DATA(0x005a149c) extern const char* const& kSaveAsStr;
DATA(0x005a1494) extern const char* const& kExportTextStr;
DATA(0x005a14a0) extern const char* const& kImportTextStr;
DATA(0x005a1490) extern const char* const& kRecentFileStr;
DATA(0x005a148c) extern const char* const& kExitStr;
}

// editor.txt "Edit Menu"
namespace SEditMenuText {
enum { kNumStrings = 10 };
DATA(0x005a18e4) extern const char* const& kUndoStr;
DATA(0x005a18b4) extern const char* const& kRedoStr;
DATA(0x005a18e8) extern const char* const& kCutStr;
DATA(0x005a18f4) extern const char* const& kCopyStr;
DATA(0x005a18f0) extern const char* const& kPasteStr;
DATA(0x005a18ec) extern const char* const& kDeleteStr;
DATA(0x005a18b0) extern const char* const& kFindStr;
DATA(0x005a18b8) extern const char* const& kFindNextStr;
DATA(0x005a18ac) extern const char* const& kFindPrevStr;
DATA(0x005a18f8) extern const char* const& kPropertiesStr;
}

// editor.txt "View Menu"
namespace SViewMenuText {
enum { kNumStrings = 10 };
DATA(0x005a0eac) extern const char* const& kZoomInStr;
DATA(0x005a0ea8) extern const char* const& kZoomOutStr;
DATA(0x005a0e78) extern const char* const& kUndergroundStr;
DATA(0x005a0e74) extern const char* const& kGridStr;
DATA(0x005a0eb0) extern const char* const& kPassabilityStr;
DATA(0x005a0ebc) extern const char* const& kObjectAnimationStr;
DATA(0x005a0ec0) extern const char* const& kTerrainAnimationStr;
DATA(0x005a0eb8) extern const char* const& kToolbarStr;
DATA(0x005a0eb4) extern const char* const& kModeBarStr;
DATA(0x005a0e7c) extern const char* const& kStatusBarStr;
}

// editor.txt "Tools Menu"
namespace SToolsMenuText {
enum { kNumStrings = 9 };
DATA(0x0059ff68) extern const char* const& kTerrainStr;
DATA(0x0059ff64) extern const char* const& kRiversStr;
DATA(0x0059ff70) extern const char* const& kRoadsStr;
DATA(0x0059ff58) extern const char* const& kEraseStr;
DATA(0x0059ff6c) extern const char* const& kObjectsStr;
DATA(0x0059ff2c) extern const char* const& kMapSpecificationsStr;
DATA(0x0059ff5c) extern const char* const& kValidateMapStr;
DATA(0x0059ff60) extern const char* const& kOptionsStr;
DATA(0x0059ff30) extern const char* const& kObstaclesStr;
}

// editor.txt "Tools|Terrain Menu"
namespace SToolsTerrainMenuText {
enum { kNumStrings = 14 };
DATA(0x005a15d8) extern const char* const& k1x1Str;
DATA(0x005a1578) extern const char* const& k2x2Str;
DATA(0x005a15e4) extern const char* const& k4x4Str;
DATA(0x005a157c) extern const char* const& kFillStr;
DATA(0x005a1584) extern const char* const& kDirtStr;
DATA(0x005a15d4) extern const char* const& kSandStr;
DATA(0x005a15dc) extern const char* const& kGrassStr;
DATA(0x005a15cc) extern const char* const& kSnowStr;
DATA(0x005a15d0) extern const char* const& kSwampStr;
DATA(0x005a1588) extern const char* const& kRoughStr;
DATA(0x005a15c8) extern const char* const& kSubterraneanStr;
DATA(0x005a15e0) extern const char* const& kLavaStr;
DATA(0x005a1580) extern const char* const& kWaterStr;
DATA(0x005a15c4) extern const char* const& kRockStr;
}

// editor.txt "Tools|Rivers Menu"
namespace SToolsRiversMenuText {
enum { kNumStrings = 5 };
DATA(0x005a04fc) extern const char* const& kClearStr;
DATA(0x005a04dc) extern const char* const& kIcyStr;
DATA(0x005a04f4) extern const char* const& kMuddyStr;
DATA(0x005a0500) extern const char* const& kLavaStr;
DATA(0x005a04f8) extern const char* const& kEraserStr;
}

// editor.txt "Tools|Roads Menu"
namespace SToolsRoadsMenuText {
enum { kNumStrings = 4 };
DATA(0x005a0410) extern const char* const& kDirtStr;
DATA(0x005a0424) extern const char* const& kGravelStr;
DATA(0x005a040c) extern const char* const& kCobblestoneStr;
DATA(0x005a0428) extern const char* const& kEraserStr;
}

// editor.txt "Tools|Erase Menu"
namespace SToolsEraseMenuText {
enum { kNumStrings = 4 };
DATA(0x005a0c8c) extern const char* const& k1x1Str;
DATA(0x005a0c74) extern const char* const& k2x2Str;
DATA(0x005a0c90) extern const char* const& k4x4Str;
DATA(0x005a0c78) extern const char* const& kFillStr;
}

// editor.txt "Tools|Objects Menu"
namespace SToolsObjectsMenuText {
enum { kNumStrings = 15 };
DATA(0x005a0b90) extern const char* const& kDirtObjectsStr;
DATA(0x005a0b20) extern const char* const& kSandObjectsStr;
DATA(0x005a0b6c) extern const char* const& kGrassObjectsStr;
DATA(0x005a0b78) extern const char* const& kSnowObjectsStr;
DATA(0x005a0b88) extern const char* const& kSwampObjectsStr;
DATA(0x005a0b8c) extern const char* const& kRoughObjectsStr;
DATA(0x005a0b60) extern const char* const& kSubterraneanObjectsStr;
DATA(0x005a0b64) extern const char* const& kLavaObjectsStr;
DATA(0x005a0b80) extern const char* const& kWaterObjectsStr;
DATA(0x005a0b70) extern const char* const& kAllTerrainObjectsStr;
DATA(0x005a0b74) extern const char* const& kTownsStr;
DATA(0x005a0b68) extern const char* const& kMonstersStr;
DATA(0x005a0b7c) extern const char* const& kHeroesStr;
DATA(0x005a0b94) extern const char* const& kArtifactsStr;
DATA(0x005a0b84) extern const char* const& kTreasuresStr;
}

// editor.txt "Player Menu"
namespace SPlayerMenuText {
enum { kNumStrings = 9 };
DATA(0x005a0e48) extern const char* const& kNoneStr;
DATA(0x005a0e58) extern const char* const& kPlayer1FmtStr;
DATA(0x005a0e60) extern const char* const& kPlayer2FmtStr;
DATA(0x005a0e5c) extern const char* const& kPlayer3FmtStr;
DATA(0x005a0e64) extern const char* const& kPlayer4FmtStr;
DATA(0x005a0e54) extern const char* const& kPlayer5FmtStr;
DATA(0x005a0e50) extern const char* const& kPlayer6FmtStr;
DATA(0x005a0e4c) extern const char* const& kPlayer7FmtStr;
DATA(0x005a0e44) extern const char* const& kPlayer8FmtStr;
}

// editor.txt "Help Menu"
namespace SHelpMenuText {
enum { kNumStrings = 2 };
DATA(0x0059fef0) extern const char* const& kHelpTopicsStr;
DATA(0x0059feec) extern const char* const& kAboutMapEditorStr;
}

// editor.txt "Context Menu"
namespace SContextMenuText {
enum { kNumStrings = 12 };
DATA(0x005a1ab0) extern const char* const& kWhatsThisStr;
DATA(0x005a1aa0) extern const char* const& kUndoStr;
DATA(0x005a1a68) extern const char* const& kRedoStr;
DATA(0x005a1aa8) extern const char* const& kCutStr;
DATA(0x005a1ab8) extern const char* const& kCopyStr;
DATA(0x005a1ab4) extern const char* const& kPasteStr;
DATA(0x005a1aac) extern const char* const& kDeleteStr;
DATA(0x005a1a64) extern const char* const& kFindStr;
DATA(0x005a1a6c) extern const char* const& kFindNextStr;
DATA(0x005a1a60) extern const char* const& kFindPrevStr;
DATA(0x005a1abc) extern const char* const& kPropertiesStr;
DATA(0x005a1aa4) extern const char* const& kPlaceObstaclesStr;
}

// editor.txt "Tools|Erase Menu"
namespace SToolsBrushMenuText {
enum { kNumStrings = 17 };
DATA(0x005a08ec) extern const char* const& k1x1SolidBrushStr;
DATA(0x005a08f0) extern const char* const& k2x2SolidBrushStr;
DATA(0x005a08e8) extern const char* const& k4x4SolidBrushStr;
DATA(0x005a08f4) extern const char* const& kSolidAreaBrushStr;
DATA(0x005a08c4) extern const char* const& k1x1FuzzyBrushStr;
DATA(0x005a08c8) extern const char* const& k2x2FuzzyBrushStr;
DATA(0x005a08dc) extern const char* const& k4x4FuzzyBrushStr;
DATA(0x005a08d0) extern const char* const& kFuzzyAreaBrushStr;
DATA(0x005a0878) extern const char* const& k1x1SolidEraserStr;
DATA(0x005a087c) extern const char* const& k2x2SolidEraserStr;
DATA(0x005a08d4) extern const char* const& k4x4SolidEraserStr;
DATA(0x005a0874) extern const char* const& kSolidAreaEraserStr;
DATA(0x005a08d8) extern const char* const& k1x1FuzzyEraserStr;
DATA(0x005a08e4) extern const char* const& k2x2FuzzyEraserStr;
DATA(0x005a08e0) extern const char* const& k4x4FuzzyEraserStr;
DATA(0x005a08f8) extern const char* const& kFuzzyAreaEraserStr;
DATA(0x005a08cc) extern const char* const& kPlaceObstaclesStr;
}

namespace SText {
void initialize();
}

#endif  /* HOMM3_EDITOR_MAPEDITORTEXT_H */
