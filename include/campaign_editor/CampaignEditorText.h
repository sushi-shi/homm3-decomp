// CampaignEditorText.h - the campaign editor's text strings
// (CampaignEditorText.cpp). The loader reads CmpEditr.txt once: the general
// strings, then one group per dialog, page and menu, each after its heading
// line. Every string is exported as a reference to its loaded pointer, as the
// map editor's text is (MapEditorText.h); each reference's dynamic
// initializer (0x416f20..0x418180) stores the address of its group's array
// element, which gives every group's members in index order. The names are
// read from the texts and headings: no symbol survives.
#ifndef HOMM3_CAMPAIGN_EDITOR_CAMPAIGNEDITORTEXT_H
#define HOMM3_CAMPAIGN_EDITOR_CAMPAIGNEDITORTEXT_H


enum { kNumGeneralStrings = 50 };
extern const char* const& kAppTitleStr;
extern const char* const& kCopyrightStr;
extern const char* const& kVersionFmtStr;
extern const char* const& kCampaignFileFilterStr;
extern const char* const& kCampaignFileRegNameStr;
extern const char* const& kDisplayDepthErrorStr;
extern const char* const& kDebugBuildStr;
extern const char* const& kReleaseBuildStr;
extern const char* const& kOKStr;
extern const char* const& kAboutCaptionStr;
extern const char* const& kPlayerNameFmtStr;
extern const char* const& kRandomTownStr;
extern const char* const& kMapInvalidFmtStr;
extern const char* const& kMapUnplayableFmtStr;
extern const char* const& kMapInvalidVersionFmtStr;
extern const char* const& kMapOldVersionFmtStr;
extern const char* const& kInvalidCampaignVersionFmtStr;
extern const char* const& kInvalidCampaignFileStr;
extern const char* const& kFileExistsFmtStr;
extern const char* const& kPropertiesStr;
extern const char* const& kMostPowerfulHeroStr;
extern const char* const& kGeneratedHeroStr;
extern const char* const& kAndStr;
extern const char* const& kRandomHeroStr;
extern const char* const& kNoneStr;
extern const char* const& kEasyStr;
extern const char* const& kNormalStr;
extern const char* const& kHardStr;
extern const char* const& kExpertStr;
extern const char* const& kImpossibleStr;
extern const char* const& kMapFileFilterStr;
extern const char* const& kPrologueStr;
extern const char* const& kHasPrologueStr;
extern const char* const& kEpilogueStr;
extern const char* const& kHasEpilogueStr;
extern const char* const& kScenarioPropsSheetCaptionStr;
extern const char* const& kSpellBonusFmtStr;
extern const char* const& kGiveBonusFmtStr;
extern const char* const& kBuildBonusFmtStr;
extern const char* const& kReceiveBonusFmtStr;
extern const char* const& kCancelStr;
extern const char* const& kHelpStr;
extern const char* const& kTextFileFilterStr;
extern const char* const& kImportFailedFmtStr;
extern const char* const& kImportSucceededFmtStr;
extern const char* const& kNameStr;
extern const char* const& kDescriptionStr;
extern const char* const& kRightClickTextStr;
extern const char* const& kPrologueTextStr;
extern const char* const& kEpilogueTextStr;

namespace SCampaignPropsDlgText {
enum { kNumStrings = 9 };
extern const char* const& kCaptionStr;
extern const char* const& kMapStaticStr;
extern const char* const& kNameStaticStr;
extern const char* const& kDescriptionStaticStr;
extern const char* const& kDifficultyChoiceCheckStr;
extern const char* const& kMusicStaticStr;
extern const char* const& kVersionGroupStr;
extern const char* const& kArmageddonsBladeRadioStr;
extern const char* const& kShadowOfDeathRadioStr;
}

namespace SEditCrossoverScenarioOptionDlgText {
enum { kNumStrings = 3 };
extern const char* const& kCaptionStr;
extern const char* const& kScenarioStaticStr;
extern const char* const& kPlayerStaticStr;
}

namespace SEditStartingBonusDlgText {
enum { kNumStrings = 23 };
extern const char* const& kCaptionStr;
extern const char* const& kTypeStaticStr;
extern const char* const& kSpellRadioStr;
extern const char* const& kCreatureRadioStr;
extern const char* const& kBuildingRadioStr;
extern const char* const& kArtifactRadioStr;
extern const char* const& kSpellScrollRadioStr;
extern const char* const& kPrimarySkillRadioStr;
extern const char* const& kSecondarySkillRadioStr;
extern const char* const& kResourceRadioStr;
extern const char* const& kRecipientStaticStr;
extern const char* const& kArtifactStaticStr;
extern const char* const& kCreatureStaticStr;
extern const char* const& kQuantityStaticStr;
extern const char* const& kBuildingStaticStr;
extern const char* const& kSpellStaticStr;
extern const char* const& kAttackStaticStr;
extern const char* const& kDefenseStaticStr;
extern const char* const& kSpellPowerStaticStr;
extern const char* const& kKnowledgeStaticStr;
extern const char* const& kSecondarySkillStaticStr;
extern const char* const& kMasteryStaticStr;
extern const char* const& kResourceStaticStr;
}

namespace SEditStartingHeroOptionDlgText {
enum { kNumStrings = 3 };
extern const char* const& kCaptionStr;
extern const char* const& kHeroStaticStr;
extern const char* const& kPlayerStaticStr;
}

namespace SNewCampaignDlgText {
enum { kNumStrings = 5 };
extern const char* const& kCaptionStr;
extern const char* const& kMapStaticStr;
extern const char* const& kVersionGroupStr;
extern const char* const& kArmageddonsBladeRadioStr;
extern const char* const& kShadowOfDeathRadioStr;
}

namespace SScenarioPropsCrossoverPageText {
enum { kNumStrings = 11 };
extern const char* const& kCaptionStr;
extern const char* const& kRetainStaticStr;
extern const char* const& kExperienceCheckStr;
extern const char* const& kPrimarySkillsCheckStr;
extern const char* const& kSecondarySkillsCheckStr;
extern const char* const& kSpellsCheckStr;
extern const char* const& kArtifactsCheckStr;
extern const char* const& kCreaturesStaticStr;
extern const char* const& kAllButtonStr;
extern const char* const& kNoneButtonStr;
extern const char* const& kArtifactsStaticStr;
}

namespace SScenarioPropsGeneralPageText {
enum { kNumStrings = 12 };
extern const char* const& kCaptionStr;
extern const char* const& kRegionNameStaticStr;
extern const char* const& kRegionColorStaticStr;
extern const char* const& kScenarioNameStaticStr;
extern const char* const& kMapFileGroupStr;
extern const char* const& kImportButtonStr;
extern const char* const& kRefreshButtonStr;
extern const char* const& kExportButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kDifficultyStaticStr;
extern const char* const& kPrerequisitesStaticStr;
extern const char* const& kRegionTextStaticStr;
}

namespace SScenarioPropsProloguePageText {
enum { kNumStrings = 3 };
extern const char* const& kMovieStaticStr;
extern const char* const& kMusicStaticStr;
extern const char* const& kMessageStaticStr;
}

namespace SScenarioPropsStartingOptionsPageText {
enum { kNumStrings = 12 };
extern const char* const& kCaptionStr;
extern const char* const& kTypeStaticStr;
extern const char* const& kBonusRadioStr;
extern const char* const& kCrossoverRadioStr;
extern const char* const& kStartingHeroRadioStr;
extern const char* const& kPlayerStaticStr;
extern const char* const& kOptionsStaticStr;
extern const char* const& kAddButtonStr;
extern const char* const& kEditButtonStr;
extern const char* const& kRemoveButtonStr;
extern const char* const& kScenarioColumnStr;
extern const char* const& kHeroColumnStr;
}

namespace SMainMenuText {
enum { kNumStrings = 3 };
extern const char* const& kFileStr;
extern const char* const& kEditStr;
extern const char* const& kHelpStr;
}

namespace SFileMenuText {
enum { kNumStrings = 10 };
extern const char* const& kNewStr;
extern const char* const& kOpenStr;
extern const char* const& kSaveStr;
extern const char* const& kSaveAsStr;
extern const char* const& kExportTextStr;
extern const char* const& kImportTextStr;
extern const char* const& kRefreshMapsStr;
extern const char* const& kExportMapsStr;
extern const char* const& kRecentFilesStr;
extern const char* const& kExitStr;
}

namespace SEditMenuText {
enum { kNumStrings = 2 };
extern const char* const& kCampaignPropertiesStr;
extern const char* const& kScenarioPropertiesStr;
}

namespace SHelpMenuText {
enum { kNumStrings = 2 };
extern const char* const& kHelpTopicsStr;
extern const char* const& kAboutStr;
}
namespace SText {
void initialize();
}

#endif  /* HOMM3_CAMPAIGN_EDITOR_CAMPAIGNEDITORTEXT_H */
