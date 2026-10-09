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

#include "va.h"

enum { kNumGeneralStrings = 50 };
DATA(0x004c70fc) extern const char* const& kAppTitleStr;
DATA(0x004c6f68) extern const char* const& kCopyrightStr;
DATA(0x004c7048) extern const char* const& kVersionFmtStr;
DATA(0x004c7054) extern const char* const& kCampaignFileFilterStr;
DATA(0x004c72d8) extern const char* const& kCampaignFileRegNameStr;
DATA(0x004c746c) extern const char* const& kDisplayDepthErrorStr;
DATA(0x004c7154) extern const char* const& kDebugBuildStr;
DATA(0x004c70f8) extern const char* const& kReleaseBuildStr;
DATA(0x004c7060) extern const char* const& kOKStr;
DATA(0x004c7108) extern const char* const& kAboutCaptionStr;
DATA(0x004c7454) extern const char* const& kPlayerNameFmtStr;
DATA(0x004c7160) extern const char* const& kRandomTownStr;
DATA(0x004c7080) extern const char* const& kMapInvalidFmtStr;
DATA(0x004c7458) extern const char* const& kMapUnplayableFmtStr;
DATA(0x004c7088) extern const char* const& kMapInvalidVersionFmtStr;
DATA(0x004c7028) extern const char* const& kMapOldVersionFmtStr;
DATA(0x004c7050) extern const char* const& kInvalidCampaignVersionFmtStr;
DATA(0x004c732c) extern const char* const& kInvalidCampaignFileStr;
DATA(0x004c710c) extern const char* const& kFileExistsFmtStr;
DATA(0x004c71b8) extern const char* const& kPropertiesStr;
DATA(0x004c7114) extern const char* const& kMostPowerfulHeroStr;
DATA(0x004c6fbc) extern const char* const& kGeneratedHeroStr;
DATA(0x004c7390) extern const char* const& kAndStr;
DATA(0x004c7470) extern const char* const& kRandomHeroStr;
DATA(0x004c702c) extern const char* const& kNoneStr;
DATA(0x004c7198) extern const char* const& kEasyStr;
DATA(0x004c7194) extern const char* const& kNormalStr;
DATA(0x004c6fcc) extern const char* const& kHardStr;
DATA(0x004c7394) extern const char* const& kExpertStr;
DATA(0x004c7084) extern const char* const& kImpossibleStr;
DATA(0x004c715c) extern const char* const& kMapFileFilterStr;
DATA(0x004c6fc8) extern const char* const& kPrologueStr;
DATA(0x004c704c) extern const char* const& kHasPrologueStr;
DATA(0x004c7168) extern const char* const& kEpilogueStr;
DATA(0x004c7164) extern const char* const& kHasEpilogueStr;
DATA(0x004c6f64) extern const char* const& kScenarioPropsSheetCaptionStr;
DATA(0x004c6f70) extern const char* const& kSpellBonusFmtStr;
DATA(0x004c7190) extern const char* const& kGiveBonusFmtStr;
DATA(0x004c7158) extern const char* const& kBuildBonusFmtStr;
DATA(0x004c6fc4) extern const char* const& kReceiveBonusFmtStr;
DATA(0x004c705c) extern const char* const& kCancelStr;
DATA(0x004c7110) extern const char* const& kHelpStr;
DATA(0x004c7474) extern const char* const& kTextFileFilterStr;
DATA(0x004c6f6c) extern const char* const& kImportFailedFmtStr;
DATA(0x004c7140) extern const char* const& kImportSucceededFmtStr;
DATA(0x004c7450) extern const char* const& kNameStr;
DATA(0x004c6f60) extern const char* const& kDescriptionStr;
DATA(0x004c7150) extern const char* const& kRightClickTextStr;
DATA(0x004c7058) extern const char* const& kPrologueTextStr;
DATA(0x004c6fc0) extern const char* const& kEpilogueTextStr;

namespace SCampaignPropsDlgText {
enum { kNumStrings = 9 };
DATA(0x004c6fb4) extern const char* const& kCaptionStr;
DATA(0x004c6fb0) extern const char* const& kMapStaticStr;
DATA(0x004c6fa8) extern const char* const& kNameStaticStr;
DATA(0x004c6f98) extern const char* const& kDescriptionStaticStr;
DATA(0x004c6fa4) extern const char* const& kDifficultyChoiceCheckStr;
DATA(0x004c6fa0) extern const char* const& kMusicStaticStr;
DATA(0x004c6fb8) extern const char* const& kVersionGroupStr;
DATA(0x004c6f9c) extern const char* const& kArmageddonsBladeRadioStr;
DATA(0x004c6fac) extern const char* const& kShadowOfDeathRadioStr;
}

namespace SEditCrossoverScenarioOptionDlgText {
enum { kNumStrings = 3 };
DATA(0x004c72d0) extern const char* const& kCaptionStr;
DATA(0x004c72cc) extern const char* const& kScenarioStaticStr;
DATA(0x004c72d4) extern const char* const& kPlayerStaticStr;
}

namespace SEditStartingBonusDlgText {
enum { kNumStrings = 23 };
DATA(0x004c7434) extern const char* const& kCaptionStr;
DATA(0x004c7410) extern const char* const& kTypeStaticStr;
DATA(0x004c7424) extern const char* const& kSpellRadioStr;
DATA(0x004c7404) extern const char* const& kCreatureRadioStr;
DATA(0x004c73a0) extern const char* const& kBuildingRadioStr;
DATA(0x004c7440) extern const char* const& kArtifactRadioStr;
DATA(0x004c7420) extern const char* const& kSpellScrollRadioStr;
DATA(0x004c7400) extern const char* const& kPrimarySkillRadioStr;
DATA(0x004c7438) extern const char* const& kSecondarySkillRadioStr;
DATA(0x004c743c) extern const char* const& kResourceRadioStr;
DATA(0x004c7414) extern const char* const& kRecipientStaticStr;
DATA(0x004c7444) extern const char* const& kArtifactStaticStr;
DATA(0x004c7398) extern const char* const& kCreatureStaticStr;
DATA(0x004c739c) extern const char* const& kQuantityStaticStr;
DATA(0x004c7448) extern const char* const& kBuildingStaticStr;
DATA(0x004c7428) extern const char* const& kSpellStaticStr;
DATA(0x004c7430) extern const char* const& kAttackStaticStr;
DATA(0x004c744c) extern const char* const& kDefenseStaticStr;
DATA(0x004c7418) extern const char* const& kSpellPowerStaticStr;
DATA(0x004c740c) extern const char* const& kKnowledgeStaticStr;
DATA(0x004c7408) extern const char* const& kSecondarySkillStaticStr;
DATA(0x004c742c) extern const char* const& kMasteryStaticStr;
DATA(0x004c741c) extern const char* const& kResourceStaticStr;
}

namespace SEditStartingHeroOptionDlgText {
enum { kNumStrings = 3 };
DATA(0x004c71cc) extern const char* const& kCaptionStr;
DATA(0x004c71bc) extern const char* const& kHeroStaticStr;
DATA(0x004c71d0) extern const char* const& kPlayerStaticStr;
}

namespace SNewCampaignDlgText {
enum { kNumStrings = 5 };
DATA(0x004c7138) extern const char* const& kCaptionStr;
DATA(0x004c712c) extern const char* const& kMapStaticStr;
DATA(0x004c713c) extern const char* const& kVersionGroupStr;
DATA(0x004c7130) extern const char* const& kArmageddonsBladeRadioStr;
DATA(0x004c7134) extern const char* const& kShadowOfDeathRadioStr;
}

namespace SScenarioPropsCrossoverPageText {
enum { kNumStrings = 11 };
DATA(0x004c7018) extern const char* const& kCaptionStr;
DATA(0x004c6fd4) extern const char* const& kRetainStaticStr;
DATA(0x004c701c) extern const char* const& kExperienceCheckStr;
DATA(0x004c700c) extern const char* const& kPrimarySkillsCheckStr;
DATA(0x004c7008) extern const char* const& kSecondarySkillsCheckStr;
DATA(0x004c7010) extern const char* const& kSpellsCheckStr;
DATA(0x004c6fd0) extern const char* const& kArtifactsCheckStr;
DATA(0x004c7024) extern const char* const& kCreaturesStaticStr;
DATA(0x004c7020) extern const char* const& kAllButtonStr;
DATA(0x004c7014) extern const char* const& kNoneButtonStr;
DATA(0x004c7004) extern const char* const& kArtifactsStaticStr;
}

namespace SScenarioPropsGeneralPageText {
enum { kNumStrings = 12 };
DATA(0x004c70e4) extern const char* const& kCaptionStr;
DATA(0x004c70c8) extern const char* const& kRegionNameStaticStr;
DATA(0x004c70d8) extern const char* const& kRegionColorStaticStr;
DATA(0x004c70e0) extern const char* const& kScenarioNameStaticStr;
DATA(0x004c70ec) extern const char* const& kMapFileGroupStr;
DATA(0x004c70dc) extern const char* const& kImportButtonStr;
DATA(0x004c70f0) extern const char* const& kRefreshButtonStr;
DATA(0x004c70e8) extern const char* const& kExportButtonStr;
DATA(0x004c70cc) extern const char* const& kRemoveButtonStr;
DATA(0x004c70f4) extern const char* const& kDifficultyStaticStr;
DATA(0x004c70d0) extern const char* const& kPrerequisitesStaticStr;
DATA(0x004c70d4) extern const char* const& kRegionTextStaticStr;
}

namespace SScenarioPropsProloguePageText {
enum { kNumStrings = 3 };
DATA(0x004c71e4) extern const char* const& kMovieStaticStr;
DATA(0x004c71e8) extern const char* const& kMusicStaticStr;
DATA(0x004c71ec) extern const char* const& kMessageStaticStr;
}

namespace SScenarioPropsStartingOptionsPageText {
enum { kNumStrings = 12 };
DATA(0x004c7380) extern const char* const& kCaptionStr;
DATA(0x004c7370) extern const char* const& kTypeStaticStr;
DATA(0x004c7388) extern const char* const& kBonusRadioStr;
DATA(0x004c7374) extern const char* const& kCrossoverRadioStr;
DATA(0x004c736c) extern const char* const& kStartingHeroRadioStr;
DATA(0x004c7384) extern const char* const& kPlayerStaticStr;
DATA(0x004c737c) extern const char* const& kOptionsStaticStr;
DATA(0x004c7364) extern const char* const& kAddButtonStr;
DATA(0x004c738c) extern const char* const& kEditButtonStr;
DATA(0x004c7378) extern const char* const& kRemoveButtonStr;
DATA(0x004c7360) extern const char* const& kScenarioColumnStr;
DATA(0x004c7368) extern const char* const& kHeroColumnStr;
}

namespace SMainMenuText {
enum { kNumStrings = 3 };
DATA(0x004c7040) extern const char* const& kFileStr;
DATA(0x004c7044) extern const char* const& kEditStr;
DATA(0x004c7030) extern const char* const& kHelpStr;
}

namespace SFileMenuText {
enum { kNumStrings = 10 };
DATA(0x004c730c) extern const char* const& kNewStr;
DATA(0x004c7310) extern const char* const& kOpenStr;
DATA(0x004c7320) extern const char* const& kSaveStr;
DATA(0x004c7324) extern const char* const& kSaveAsStr;
DATA(0x004c731c) extern const char* const& kExportTextStr;
DATA(0x004c7328) extern const char* const& kImportTextStr;
DATA(0x004c7304) extern const char* const& kRefreshMapsStr;
DATA(0x004c7308) extern const char* const& kExportMapsStr;
DATA(0x004c7318) extern const char* const& kRecentFilesStr;
DATA(0x004c7314) extern const char* const& kExitStr;
}

namespace SEditMenuText {
enum { kNumStrings = 2 };
DATA(0x004c7468) extern const char* const& kCampaignPropertiesStr;
DATA(0x004c7464) extern const char* const& kScenarioPropertiesStr;
}

namespace SHelpMenuText {
enum { kNumStrings = 2 };
DATA(0x004c6f5c) extern const char* const& kHelpTopicsStr;
DATA(0x004c6f58) extern const char* const& kAboutStr;
}
#endif  /* HOMM3_CAMPAIGN_EDITOR_CAMPAIGNEDITORTEXT_H */
