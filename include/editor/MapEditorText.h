// MapEditorText.h - the editor's text strings (MapEditorText.cpp; Loki h3maped object 18).
// SText::initialize loads editor.txt once: the general strings, then one
// group per dialog, page and menu, each followed by a separator line. Every
// string is exported as a reference to its loaded pointer: users load the
// reference, then the pointer. The groups are namespaces (the image names
// SArmyDlgText::{anonymous}::akStringImp, and __PRETTY_FUNCTION__ prints
// "void SText::initialize()" without "static"); every name below is the
// image's own. The counts come from SText::initialize's bounds; their type
// is not recorded.
#ifndef HOMM3_EDITOR_MAPEDITORTEXT_H
#define HOMM3_EDITOR_MAPEDITORTEXT_H

enum { kNumGeneralStrings = 208 };
extern const char* const& kRegistryKeyStr;
extern const char* const& kAppTitleStr;
extern const char* const& kCopyrightStr;
extern const char* const& kVersionFmtStr;
extern const char* const& kMapFileFilterStr;
extern const char* const& kMapFileRegNameStr;
extern const char* const& kMapSpecsSheetCaptionStr;
extern const char* const& kObjectPropertiesCaptionFmtStr;
extern const char* const& kEditTimedEventSheetCaptionStr;
extern const char* const& kSelNoneStr;
extern const char* const& kRandomStr;
extern const char* const& kUnknownStr;
extern const char* const& kBackpackStr;
extern const char* const& kExperienceStr;
extern const char* const& kManaStr;
extern const char* const& kMoraleStr;
extern const char* const& kLuckStr;
extern const char* const& kResourceStr;
extern const char* const& kResourcesStr;
extern const char* const& kPrimarySkillStr;
extern const char* const& kPrimarySkillsStr;
extern const char* const& kArtifactStr;
extern const char* const& kArtifactsStr;
extern const char* const& kSpellStr;
extern const char* const& kSpellsStr;
extern const char* const& kCreatureStr;
extern const char* const& kCreaturesStr;
extern const char* const& kSecondarySkillStr;
extern const char* const& kSecondarySkillsStr;
extern const char* const& kPlayerNameFmtStr;
extern const char* const& kPlayerNoneStr;
extern const char* const& kNeverStr;
extern const char* const& kEveryDayStr;
extern const char* const& kEvery2DaysStr;
extern const char* const& kEvery3DaysStr;
extern const char* const& kEvery4DaysStr;
extern const char* const& kEvery5DaysStr;
extern const char* const& kEvery6DaysStr;
extern const char* const& kEvery7DaysStr;
extern const char* const& kEvery14DaysStr;
extern const char* const& kEvery21DaysStr;
extern const char* const& kEvery28DaysStr;
extern const char* const& kMaxObjectsOfTypeFmtStr;
extern const char* const& kNoMoreClassHeroesStr;
extern const char* const& kSelectPlayerBeforePlacingHeroStr;
extern const char* const& kMaxHeroesPerPlayerFmtStr;
extern const char* const& kGrailAlreadyPlacedStr;
extern const char* const& kGrailPlacedTooCloseToEdgeFmtStr;
extern const char* const& kNowhereToPasteStr;
extern const char* const& kDebugBuildStr;
extern const char* const& kReleaseBuildStr;
extern const char* const& kNeed16BitColorStr;
extern const char* const& kInvalidMapVersionFmtStr;
extern const char* const& kInvalidMapFileStr;
extern const char* const& k2DaysStr;
extern const char* const& k3DaysStr;
extern const char* const& k4DaysStr;
extern const char* const& k5DaysStr;
extern const char* const& k6DaysStr;
extern const char* const& k1WeekStr;
extern const char* const& k2WeeksStr;
extern const char* const& k3WeeksStr;
extern const char* const& k4WeeksStr;
extern const char* const& k5WeeksStr;
extern const char* const& k6WeeksStr;
extern const char* const& k7WeeksStr;
extern const char* const& k2MonthsStr;
extern const char* const& k3MonthsStr;
extern const char* const& k4MonthsStr;
extern const char* const& k5MonthsStr;
extern const char* const& k6MonthsStr;
extern const char* const& k7MonthsStr;
extern const char* const& k8MonthsStr;
extern const char* const& k9MonthsStr;
extern const char* const& k10MonthsStr;
extern const char* const& k11MonthsStr;
extern const char* const& k12MonthsStr;
extern const char* const& kAnyTownStr;
extern const char* const& kObjectAtLocationFmtStr;
extern const char* const& kSpecificHeroAndClassFmtStr;
extern const char* const& kRandomMonsterStr;
extern const char* const& kRandomMonstersStr;
extern const char* const& kWarriorStr;
extern const char* const& kBuilderStr;
extern const char* const& kExplorerStr;
extern const char* const& kWarningStr;
extern const char* const& kHeroesNeedOwnerStr;
extern const char* const& kPlayerHasMaxHeroesStr;
extern const char* const& kContinuingWillDeleteHeroFmtStr;
extern const char* const& kNoMoreHeroesStr;
extern const char* const& kNoPlayersOnMapStr;
extern const char* const& kRandomTownStr;
extern const char* const& kTeamFmtStr;
extern const char* const& kThereAreNoPlayersOnMapStr;
extern const char* const& kThereAreNoTownsOnMapStr;
extern const char* const& kMapHasNoNameStr;
extern const char* const& kMapHasNoDescStr;
extern const char* const& kGrailPlacedButNoObelisksStr;
extern const char* const& kPlayerOwnsObjectsButNotPresentFmtStr;
extern const char* const& kTownHasShipyardButIsLandlockedFmtStr;
extern const char* const& kOnlyOneMonolithOnMapFmtStr;
extern const char* const& kMonolithEntranceButNoExitFmtStr;
extern const char* const& kMonolithExitButNoEntranceFmtStr;
extern const char* const& kSubterraneanGatesOnOneLayerMapStr;
extern const char* const& kMoreSubterraneanGatesAboveThanBelowStr;
extern const char* const& kMoreSubterraneanGatesBelowThanAboveStr;
extern const char* const& kSeersHutQuestArtifactNotOnMapFmtStr;
extern const char* const& kOnePlayerPresentButNoSpecialVictoryConditionStr;
extern const char* const& kOnePlayerPresentButNormalVictoryNotDisabledStr;
extern const char* const& kThereAreNoProblemsWithMapStr;
extern const char* const& kSVCAquireArtifactButArtifactNotPresentFmtStr;
extern const char* const& kSVCAquireArtifactButHeroHasArtifactFmtStr;
extern const char* const& kSVCAccumCreaturePlayerAlreadyHasEnoughCreaturesFmtStr;
extern const char* const& kSVCUpgradeTownBuildingsAlreadyBuiltFmtStr;
extern const char* const& kSVCUpgradeTownBuildingsDisabledFmtStr;
extern const char* const& kSVCBuildGrailStructAlreadyBuiltFmtStr;
extern const char* const& kSVCBuildGrailStructDisabledFmtStr;
extern const char* const& kSVCBuildGrailStructNowhereToBuildStr;
extern const char* const& kSVCDefeatHeroBelongsToHumanFmtStr;
extern const char* const& kSVCCaptureTownAlreadyOwnedFmtStr;
extern const char* const& kSVCCaptureTownOwnedByHumanFmtStr;
extern const char* const& kSVCFlagGeneratorsNoGeneratorsOnMapStr;
extern const char* const& kSVCFlagGeneratorsPlayerOwnsAllGeneratorsFmtStr;
extern const char* const& kSVCFlagMinesNoMinesOnMapStr;
extern const char* const& kSVCFlagMinesPlayerAlreadyOwnsAllMinesFmtStr;
extern const char* const& kSVCTransportArtifactNotPresentFmtStr;
extern const char* const& kSLCLoseTownNotOwnedFmtStr;
extern const char* const& kSLCLoseTownTeamNotHumanFmtStr;
extern const char* const& kSLCLoseTownMultipleHumanTeamsFmtStr;
extern const char* const& kSLCLoseTownPlayerNotHumanFmtStr;
extern const char* const& kSLCLoseTownMultipleHumanPlayersFmtStr;
extern const char* const& kSLCLoseHeroNotOwnedFmtStr;
extern const char* const& kSLCLoseHeroTeamNotHumanFmtStr;
extern const char* const& kSLCLoseHeroMultipleHumanTeamsFmtStr;
extern const char* const& kSLCLoseHeroPlayerNotHumanFmtStr;
extern const char* const& kSLCLoseHeroMultipleHumanPlayersFmtStr;
extern const char* const& kRecoverAutosavedMapStr;
extern const char* const& kTextFileFilterStr;
extern const char* const& kUnableToImportTextFmtStr;
extern const char* const& kTextImportedSuccessfullyFmtStr;
extern const char* const& kMapNameStr;
extern const char* const& kMapDescriptionStr;
extern const char* const& kRumorsStr;
extern const char* const& kTimedEventsStr;
extern const char* const& kObjectsStr;
extern const char* const& kNameStr;
extern const char* const& kMessageStr;
extern const char* const& kTextStr;
extern const char* const& kVisitingHeroFmtStr;
extern const char* const& kEndOfFileStr;
extern const char* const& kTimedEventNameFmtStr;
extern const char* const& kObjectNotFoundStr;
extern const char* const& kToolbarCaptionStr;
extern const char* const& kModeBarCaptionStr;
extern const char* const& kOneWhirlpoolStr;
extern const char* const& kObjectIsUnreachableFmtStr;
extern const char* const& kOKStr;
extern const char* const& kCancelStr;
extern const char* const& kHelpStr;
extern const char* const& kGeneralPageCaptionStr;
extern const char* const& kContentsPageCaptionStr;
extern const char* const& kResourcesPageCaptionStr;
extern const char* const& kBuildingsPageCaptionStr;
extern const char* const& kCreaturesPageCaptionStr;
extern const char* const& kSecondarySkillsPageCaptionStr;
extern const char* const& kSpecialLossConditionPageCaptionStr;
extern const char* const& kPlayerSpecsPageCaptionStr;
extern const char* const& kRumorsPageCaptionStr;
extern const char* const& kTeamsPageCaptionStr;
extern const char* const& kTimedEventsPageCaptionStr;
extern const char* const& kSpecialVictoryConditionPageCaptionStr;
extern const char* const& kTreasurePageCaptionStr;
extern const char* const& kGarrisonPageCaptionStr;
extern const char* const& kSpellsPageCaptionStr;
extern const char* const& kGuardiansPageCaptionStr;
extern const char* const& kArtifactsPageCaptionStr;
extern const char* const& kAboutBoxCaptionStr;
extern const char* const& kEditArtifactCaptionStr;
extern const char* const& kEditCreatureStackCaptionStr;
extern const char* const& kEditRumorCaptionStr;
extern const char* const& kEditSecondarySkillCaptionStr;
extern const char* const& kFindCaptionStr;
extern const char* const& kMapValidationCaptionStr;
extern const char* const& kNewMapCaptionStr;
extern const char* const& kOptionsCaptionStr;
extern const char* const& kSelectArtifactCaptionStr;
extern const char* const& kSelectHeroClassCaptionStr;
extern const char* const& kSelectSpellCaptionStr;
extern const char* const& kCustomizeCheckStr;
extern const char* const& kBrushStr;
extern const char* const& kTerrainTypeStr;
extern const char* const& kRiverTypeStr;
extern const char* const& kRoadTypeStr;
extern const char* const& kPlacementRadiusStaticStr;
extern const char* const& kStandStillStr;
extern const char* const& kRadiusOneSquareStr;
extern const char* const& kRadiusTwoSquaresStr;
extern const char* const& kRadiusThreeSquaresStr;
extern const char* const& kRadiusFourSquaresStr;
extern const char* const& kRadiusFiveSquaresStr;
extern const char* const& kRadiusSixSquaresStr;
extern const char* const& kRadiusSevenSquaresStr;
extern const char* const& kRadiusEightSquaresStr;
extern const char* const& kRadiusNineSquaresStr;
extern const char* const& kRadiusTenSquaresStr;
extern const char* const& kProblemsStaticStr;
extern const char* const& kNoPropertiesStr;
extern const char* const& kContinuingWillDeleteUndergroundLayerStr;

namespace SAbandonedMinePropsDlgText {
enum { kNumStrings = 7 };
extern const char* const& kPotentialResourcesStaticStr;
extern const char* const& kRes2CheckStr;
extern const char* const& kRes3CheckStr;
extern const char* const& kRes4CheckStr;
extern const char* const& kRes5CheckStr;
extern const char* const& kRes6CheckStr;
extern const char* const& kRes7CheckStr;
}

namespace SArmyDlgText {
enum { kNumStrings = 3 };
extern const char* const& kTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kSlotStaticFmtStr;
}

namespace SGarrisonPropertiesDlgText {
enum { kNumStrings = 4 };
extern const char* const& kOwnerStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kPlayerRadioFmtStr;
extern const char* const& kCreaturesStaticStr;
}

namespace SArtifactPropsGeneralPageText {
enum { kNumStrings = 4 };
extern const char* const& kTypeStaticStr;
extern const char* const& kMessageStaticStr;
extern const char* const& kNoteStaticStr;
extern const char* const& kSpellStaticStr;
}

namespace SBlackBoxPropsGeneralPageText {
enum { kNumStrings = 1 };
extern const char* const& kMessageStaticStr;
}

namespace SEventPropsGeneralPageText {
enum { kNumStrings = 4 };
extern const char* const& kAllowedPlayersStaticStr;
extern const char* const& kPlayerCheckFmtStr;
extern const char* const& kAllowComputerCheckStr;
extern const char* const& kCancelCheckStr;
}

namespace SBlackBoxPropsContentsPageText {
enum { kNumStrings = 28 };
extern const char* const& kCategoryStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kRemoveAllButtonStr;
extern const char* const& kArtifactsStaticStr;
extern const char* const& kCreatureTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kExperienceBonusStaticStr;
extern const char* const& kLuckBonusStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kPlusOneRadioStr;
extern const char* const& kPlusTwoRadioStr;
extern const char* const& kPlusThreeRadioStr;
extern const char* const& kMinusOneRadioStr;
extern const char* const& kMinusTwoRadioStr;
extern const char* const& kMinusThreeRadioStr;
extern const char* const& kManaBonusStaticStr;
extern const char* const& kGiveRadioStr;
extern const char* const& kTakeRadioStr;
extern const char* const& kMoraleBonusStaticStr;
extern const char* const& kAttackSkillStaticStr;
extern const char* const& kDefenseSkillStaticStr;
extern const char* const& kSpellPowerStaticStr;
extern const char* const& kKnowledgeStaticStr;
extern const char* const& kSecondarySkillStaticStr;
extern const char* const& kMasteryStaticStr;
extern const char* const& kSpellsStaticStr;
}

namespace SResourceQuantitiesDlgText {
enum { kNumStrings = 9 };
extern const char* const& kRes1StaticStr;
extern const char* const& kRes2StaticStr;
extern const char* const& kRes3StaticStr;
extern const char* const& kRes4StaticStr;
extern const char* const& kRes5StaticStr;
extern const char* const& kRes6StaticStr;
extern const char* const& kRes7StaticStr;
extern const char* const& kGiveRadioStr;
extern const char* const& kTakeRadioStr;
}

namespace SEditSecondarySkillDlgText {
enum { kNumStrings = 5 };
extern const char* const& kTypeStaticStr;
extern const char* const& kMasteryStaticStr;
extern const char* const& kBasicRadioStr;
extern const char* const& kAdvancedRadioStr;
extern const char* const& kExpertRadioStr;
}

namespace SEditCreatureStackDlgText {
enum { kNumStrings = 2 };
extern const char* const& kTypeStaticStr;
extern const char* const& kQuantityStaticStr;
}

namespace SEditTimedEventGeneralPageText {
enum { kNumStrings = 7 };
extern const char* const& kEventNameStaticStr;
extern const char* const& kMessageStaticStr;
extern const char* const& kAllowedPlayersStaticStr;
extern const char* const& kPlayerCheckFmtStr;
extern const char* const& kAllowComputerCheckStr;
extern const char* const& kFirstOccurenceStaticStr;
extern const char* const& kSubsequentOccurenceStaticStr;
}

namespace SEditTownEventBuildingsPageText {
enum { kNumStrings = 2 };
extern const char* const& kBuildingTreeStaticStr;
extern const char* const& kBuildCheckStr;
}

namespace SEditTownEventCreaturesPageText {
enum { kNumStrings = 3 };
extern const char* const& kGeneratorNoStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kNoteStaticStr;
}

namespace SEditArtifactDlgText {
enum { kNumStrings = 2 };
extern const char* const& kEquipWhereStaticStr;
extern const char* const& kArtifactStaticStr;
}

namespace SEditRumorDlgText {
enum { kNumStrings = 2 };
extern const char* const& kNameStaticStr;
extern const char* const& kTextStaticStr;
}

namespace SFindDlgText {
enum { kNumStrings = 3 };
extern const char* const& kFindWhatStaticStr;
extern const char* const& kFindNextButtonStr;
extern const char* const& kFindPrevButtonStr;
}

namespace SFlaggablePropsDlgText {
enum { kNumStrings = 3 };
extern const char* const& kOwnerStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kPlayerRadioFmtStr;
}

namespace SHeroPropsGeneralPageText {
enum { kNumStrings = 9 };
extern const char* const& kClassStaticStr;
extern const char* const& kPlayerStaticStr;
extern const char* const& kIdentityStaticStr;
extern const char* const& kNameStaticStr;
extern const char* const& kExperienceStaticStr;
extern const char* const& kPortraitStaticStr;
extern const char* const& kPatrolStaticStr;
extern const char* const& kCustomizeNameCheckStr;
extern const char* const& kCustomizePortraitCheckStr;
}

namespace SHeroPropsCreaturesPageText {
enum { kNumStrings = 3 };
extern const char* const& kFormationStaticStr;
extern const char* const& kSpreadRadioStr;
extern const char* const& kGroupedRadioStr;
}

namespace SHeroPropsSecSkillsPageText {
enum { kNumStrings = 3 };
extern const char* const& kTypeStaticStr;
extern const char* const& kMasteryStaticStr;
extern const char* const& kSkillStaticFmtStr;
}

namespace SHeroPropsArtifactsPageText {
enum { kNumStrings = 8 };
extern const char* const& kHasSpellbookCheckStr;
extern const char* const& kArtifactsStaticStr;
extern const char* const& kNameColumnStr;
extern const char* const& kLocationColumnStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kRemoveAllButtonStr;
}

namespace SMapSpecsGeneralPageText {
enum { kNumStrings = 9 };
extern const char* const& kDifficultyStaticStr;
extern const char* const& kEasyRadioStr;
extern const char* const& kNormalRadioStr;
extern const char* const& kToughRadioStr;
extern const char* const& kExpertRadioStr;
extern const char* const& kImpossibleRadioStr;
extern const char* const& kTwoLevelMapCheckStr;
extern const char* const& kMapNameStaticStr;
extern const char* const& kDescriptionStaticStr;
}

namespace SMapSpecsPlayerSpecsPageText {
enum { kNumStrings = 6 };
extern const char* const& kPlayerStaticStr;
extern const char* const& kGenerateHeroCheckStr;
extern const char* const& kPlayabilityStaticStr;
extern const char* const& kHumanPlayableCheckStr;
extern const char* const& kComputerPlayableCheckStr;
extern const char* const& kBehaviorStaticStr;
}

namespace SMapSpecsTeamsPageText {
enum { kNumStrings = 5 };
extern const char* const& kEnableTeamsCheckStr;
extern const char* const& kNumberOfTeamsStaticStr;
extern const char* const& kTeamAssignmentsStaticStr;
extern const char* const& kTeamNumberStaticStr;
extern const char* const& kPlayerStaticFmtStr;
}

namespace SMapSpecsRumorsPageText {
enum { kNumStrings = 5 };
extern const char* const& kRumorsStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kRemoveAllButtonStr;
}

namespace SMapSpecsTimedEventsPageText {
enum { kNumStrings = 7 };
extern const char* const& kEventsStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kRemoveAllButtonStr;
extern const char* const& kMoveUpButtonStr;
extern const char* const& kMoveDownButtonStr;
}

namespace SMapSpecsVictoryCondPageText {
enum { kNumStrings = 31 };
extern const char* const& kSelectVictoryConditionStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kAquireArtifactRadioStr;
extern const char* const& kAccumCreaturesRadioStr;
extern const char* const& kAccumResourcesRadioStr;
extern const char* const& kUpgradeTownRadioStr;
extern const char* const& kBuildGrailRadioStr;
extern const char* const& kDefeatHeroRadioStr;
extern const char* const& kCaptureTownRadioStr;
extern const char* const& kDefeatMonsterRadioStr;
extern const char* const& kFlagGeneratorsRadioStr;
extern const char* const& kFlagMinesRadioStr;
extern const char* const& kTransportArtifactRadioStr;
extern const char* const& kNormalVictoryCheckStr;
extern const char* const& kComputerAlsoCheckStr;
extern const char* const& kArtifactStaticStr;
extern const char* const& kCreatureTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kResourceTypeStaticStr;
extern const char* const& kTownStaticStr;
extern const char* const& kHallLevelStaticStr;
extern const char* const& kTownHallRadioStr;
extern const char* const& kCityHallRadioStr;
extern const char* const& kCapitolHallRadioStr;
extern const char* const& kCastleLevelStaticStr;
extern const char* const& kFortCastleRadioStr;
extern const char* const& kCitadelCastleRadioStr;
extern const char* const& kCastleCastleRadioStr;
extern const char* const& kHeroStaticStr;
extern const char* const& kMonsterStaticStr;
extern const char* const& kDestinationStaticStr;
}

namespace SMapSpecsLossCondPageText {
enum { kNumStrings = 8 };
extern const char* const& kSelectLossConditionStaticStr;
extern const char* const& kNoneRadioStr;
extern const char* const& kLoseTownRadioStr;
extern const char* const& kLoseHeroRadioStr;
extern const char* const& kTimeExpiresRadioStr;
extern const char* const& kTownStaticStr;
extern const char* const& kHeroStaticStr;
extern const char* const& kTimeLimitStaticStr;
}

namespace SMonsterPropsGeneralPageText {
enum { kNumStrings = 13 };
extern const char* const& kTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kRandomQtyRadioStr;
extern const char* const& kCustomQtyRadioStr;
extern const char* const& kNeverGrowsCheckStr;
extern const char* const& kDispositionStaticStr;
extern const char* const& kDispositionCompliantRadioStr;
extern const char* const& kDispositionFriendlyRadioStr;
extern const char* const& kDispositionAggressiveRadioStr;
extern const char* const& kDispositionHostileRadioStr;
extern const char* const& kDispositionSavageRadioStr;
extern const char* const& kNeverFleesCheckStr;
extern const char* const& kMessageStaticStr;
}

namespace SMonsterPropsTreasurePageText {
enum { kNumStrings = 9 };
extern const char* const& kResourcesStaticStr;
extern const char* const& kRes1StaticStr;
extern const char* const& kRes2StaticStr;
extern const char* const& kRes3StaticStr;
extern const char* const& kRes4StaticStr;
extern const char* const& kRes5StaticStr;
extern const char* const& kRes6StaticStr;
extern const char* const& kRes7StaticStr;
extern const char* const& kArtifactStaticStr;
}

namespace SNewMapDlgText {
enum { kNumStrings = 6 };
extern const char* const& kMapSizeStaticStr;
extern const char* const& k36x36RadioStr;
extern const char* const& k72x72RadioStr;
extern const char* const& k108x108RadioStr;
extern const char* const& k144x144RadioStr;
extern const char* const& kTwoLevelMapCheckStr;
}

namespace SOptionsDlgText {
enum { kNumStrings = 8 };
extern const char* const& kTileFrequencyStaticStr;
extern const char* const& kNoneStaticStr;
extern const char* const& kLotsStaticStr;
extern const char* const& kRepaintCheckStr;
extern const char* const& kAutosaveOptionsStaticStr;
extern const char* const& kEnableAutosaveCheckStr;
extern const char* const& kAutosaveEveryStaticStr;
extern const char* const& kMinutesStaticStr;
}

namespace SResourcePropsGeneralPageText {
enum { kNumStrings = 7 };
extern const char* const& kTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kRandomQtyRadioStr;
extern const char* const& kCustomQtyRadioStr;
extern const char* const& kQuantityNoteStaticStr;
extern const char* const& kMessageStaticStr;
extern const char* const& kNoteStaticStr;
}

namespace SScholarPropsDlgText {
enum { kNumStrings = 5 };
extern const char* const& kRewardStaticStr;
extern const char* const& kRandomRewardRadioStr;
extern const char* const& kPriSkillRewardRadioStr;
extern const char* const& kSecSkillRewardRadioStr;
extern const char* const& kSpellRewardRadioStr;
}

namespace SSeersHutPropsDlgText {
enum { kNumStrings = 19 };
extern const char* const& kQuestArtifactStaticStr;
extern const char* const& kQuestRewardStaticStr;
extern const char* const& kTypeStaticStr;
extern const char* const& kArtifactStaticStr;
extern const char* const& kCreatureTypeStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kExperienceBonusStaticStr;
extern const char* const& kLuckBonusStaticStr;
extern const char* const& kPlusOneRadioStr;
extern const char* const& kPlusTwoRadioStr;
extern const char* const& kPlusThreeRadioStr;
extern const char* const& kManaBonusStaticStr;
extern const char* const& kMoraleBonusStaticStr;
extern const char* const& kPrimarySkillStaticStr;
extern const char* const& kBonusStaticStr;
extern const char* const& kResourceTypeStaticStr;
extern const char* const& kSecondarySkillStaticStr;
extern const char* const& kMasteryStaticStr;
extern const char* const& kSpellStaticStr;
}

namespace SShrinePropsDlgText {
enum { kNumStrings = 3 };
extern const char* const& kSpellStaticFmtStr;
extern const char* const& kRandomSpellRadioStr;
extern const char* const& kCustomSpellRadioStr;
}

namespace SSignPropsDlgText {
enum { kNumStrings = 1 };
extern const char* const& kMessageStaticStr;
}

namespace STownPropsGeneralPageText {
enum { kNumStrings = 10 };
extern const char* const& kTownTypeStaticStr;
extern const char* const& kPlayerStaticStr;
extern const char* const& kTownNameStaticStr;
extern const char* const& kCustomizeCheckStr;
extern const char* const& kVisitingHeroStaticStr;
extern const char* const& kHeroNameStaticStr;
extern const char* const& kHeroClassStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
}

namespace STownPropsGarrisonPageText {
enum { kNumStrings = 3 };
extern const char* const& kFormationStaticStr;
extern const char* const& kSpreadRadioStr;
extern const char* const& kGroupedRadioStr;
}

namespace STownPropsBuildingsPageText {
enum { kNumStrings = 6 };
extern const char* const& kHasFortCheckStr;
extern const char* const& kBuildingTreeStaticStr;
extern const char* const& kEnabledCheckStr;
extern const char* const& kBuiltCheckStr;
extern const char* const& kBuildAllButtonStr;
extern const char* const& kDemolishAllButtonStr;
}

namespace STownPropsSpellsPageText {
enum { kNumStrings = 1 };
extern const char* const& kSpellsStaticStr;
}

namespace STownPropsTimedEventsPageText {
enum { kNumStrings = 7 };
extern const char* const& kEventsStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kRemoveAllButtonStr;
extern const char* const& kMoveUpButtonStr;
extern const char* const& kMoveDownButtonStr;
}

namespace SMainMenuText {
enum { kNumStrings = 6 };
extern const char* const& kFileStr;
extern const char* const& kEditStr;
extern const char* const& kViewStr;
extern const char* const& kToolsStr;
extern const char* const& kPlayerStr;
extern const char* const& kHelpStr;
}

namespace SFileMenuText {
enum { kNumStrings = 8 };
extern const char* const& kNewStr;
extern const char* const& kOpenStr;
extern const char* const& kSaveStr;
extern const char* const& kSaveAsStr;
extern const char* const& kExportTextStr;
extern const char* const& kImportTextStr;
extern const char* const& kRecentFileStr;
extern const char* const& kExitStr;
}

namespace SEditMenuText {
enum { kNumStrings = 10 };
extern const char* const& kUndoStr;
extern const char* const& kRedoStr;
extern const char* const& kCutStr;
extern const char* const& kCopyStr;
extern const char* const& kPasteStr;
extern const char* const& kDeleteStr;
extern const char* const& kFindStr;
extern const char* const& kFindNextStr;
extern const char* const& kFindPrevStr;
extern const char* const& kPropertiesStr;
}

namespace SViewMenuText {
enum { kNumStrings = 10 };
extern const char* const& kZoomInStr;
extern const char* const& kZoomOutStr;
extern const char* const& kUndergroundStr;
extern const char* const& kGridStr;
extern const char* const& kPassabilityStr;
extern const char* const& kObjectAnimationStr;
extern const char* const& kTerrainAnimationStr;
extern const char* const& kToolbarStr;
extern const char* const& kModeBarStr;
extern const char* const& kStatusBarStr;
}

namespace SToolsMenuText {
enum { kNumStrings = 8 };
extern const char* const& kTerrainStr;
extern const char* const& kRiversStr;
extern const char* const& kRoadsStr;
extern const char* const& kEraseStr;
extern const char* const& kObjectsStr;
extern const char* const& kMapSpecificationsStr;
extern const char* const& kValidateMapStr;
extern const char* const& kOptionsStr;
}

namespace SToolsTerrainMenuText {
enum { kNumStrings = 14 };
extern const char* const& k1x1Str;
extern const char* const& k2x2Str;
extern const char* const& k4x4Str;
extern const char* const& kFillStr;
extern const char* const& kDirtStr;
extern const char* const& kSandStr;
extern const char* const& kGrassStr;
extern const char* const& kSnowStr;
extern const char* const& kSwampStr;
extern const char* const& kRoughStr;
extern const char* const& kSubterraneanStr;
extern const char* const& kLavaStr;
extern const char* const& kWaterStr;
extern const char* const& kRockStr;
}

namespace SToolsRiversMenuText {
enum { kNumStrings = 4 };
extern const char* const& kClearStr;
extern const char* const& kIcyStr;
extern const char* const& kMuddyStr;
extern const char* const& kLavaStr;
}

namespace SToolsRoadsMenuText {
enum { kNumStrings = 3 };
extern const char* const& kDirtStr;
extern const char* const& kGravelStr;
extern const char* const& kCobblestoneStr;
}

namespace SToolsEraseMenuText {
enum { kNumStrings = 4 };
extern const char* const& k1x1Str;
extern const char* const& k2x2Str;
extern const char* const& k4x4Str;
extern const char* const& kFillStr;
}

namespace SToolsObjectsMenuText {
enum { kNumStrings = 15 };
extern const char* const& kDirtObjectsStr;
extern const char* const& kSandObjectsStr;
extern const char* const& kGrassObjectsStr;
extern const char* const& kSnowObjectsStr;
extern const char* const& kSwampObjectsStr;
extern const char* const& kRoughObjectsStr;
extern const char* const& kSubterraneanObjectsStr;
extern const char* const& kLavaObjectsStr;
extern const char* const& kWaterObjectsStr;
extern const char* const& kAllTerrainObjectsStr;
extern const char* const& kTownsStr;
extern const char* const& kMonstersStr;
extern const char* const& kHeroesStr;
extern const char* const& kArtifactsStr;
extern const char* const& kTreasuresStr;
}

namespace SPlayerMenuText {
enum { kNumStrings = 9 };
extern const char* const& kNoneStr;
extern const char* const& kPlayer1FmtStr;
extern const char* const& kPlayer2FmtStr;
extern const char* const& kPlayer3FmtStr;
extern const char* const& kPlayer4FmtStr;
extern const char* const& kPlayer5FmtStr;
extern const char* const& kPlayer6FmtStr;
extern const char* const& kPlayer7FmtStr;
extern const char* const& kPlayer8FmtStr;
}

namespace SHelpMenuText {
enum { kNumStrings = 2 };
extern const char* const& kHelpTopicsStr;
extern const char* const& kAboutMapEditorStr;
}

namespace SContextMenuText {
enum { kNumStrings = 11 };
extern const char* const& kWhatsThisStr;
extern const char* const& kUndoStr;
extern const char* const& kRedoStr;
extern const char* const& kCutStr;
extern const char* const& kCopyStr;
extern const char* const& kPasteStr;
extern const char* const& kDeleteStr;
extern const char* const& kFindStr;
extern const char* const& kFindNextStr;
extern const char* const& kFindPrevStr;
extern const char* const& kPropertiesStr;
}

namespace SText {
void initialize();
}

#endif  /* HOMM3_EDITOR_MAPEDITORTEXT_H */
