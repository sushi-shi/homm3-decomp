// CampaignEditorText.cpp - the campaign editor's text strings: CmpEditr.txt's
// general strings, then one group per dialog, page and menu, each after its
// heading line. SText::initialize copies every group into a buffer of its
// own once, spelling each ellipsis character out as three periods, and
// points the group's strings into it.
#include "campaign_editor/stdafx.h"

#include <string.h>

#include "va.h"
#include "exceptions.h"
#include "ownership.h"
#include "resourcemanager.h"
#include "textresource.h"
#include "campaign_editor/CampaignEditorText.h"

namespace {

// The length of a text once each ellipsis is spelled out.
VA(0x00416ea0, 0x3e)
int getTextLength(const char* pText)
{
    int length = strcspn(pText, "\x85");
    const char* p = pText + length;
    while (*p != '\0') {
        p++;
        int span = strcspn(p, "\x85");
        p += span;
        length += span + 3;
    }
    return length;
}

// Copies a text, spelling each ellipsis out as three periods.
VA(0x00416ee0, 0x2f)
void copyText(char* pDest, const char* pSource)
{
    for (; *pSource != '\0'; pSource++) {
        if (*pSource == '\x85') {
            *pDest++ = '.';
            *pDest++ = '.';
            *pDest = '.';
        } else {
            *pDest = *pSource;
        }
        pDest++;
    }
    *pDest = '\0';
}

DATA(0x004c71f0) const char* akGeneralStringImp[kNumGeneralStrings];

}

VA_COMPGEN(0x00416f20, 0xb, STATIC_CTOR, kAppTitleStr)
DATA(0x004c70fc) const char* const& kAppTitleStr = akGeneralStringImp[0];
VA_COMPGEN(0x00416f40, 0xb, STATIC_CTOR, kCopyrightStr)
DATA(0x004c6f68) const char* const& kCopyrightStr = akGeneralStringImp[1];
VA_COMPGEN(0x00416f60, 0xb, STATIC_CTOR, kVersionFmtStr)
DATA(0x004c7048) const char* const& kVersionFmtStr = akGeneralStringImp[2];
VA_COMPGEN(0x00416f80, 0xb, STATIC_CTOR, kCampaignFileFilterStr)
DATA(0x004c7054) const char* const& kCampaignFileFilterStr = akGeneralStringImp[3];
VA_COMPGEN(0x00416fa0, 0xb, STATIC_CTOR, kCampaignFileRegNameStr)
DATA(0x004c72d8) const char* const& kCampaignFileRegNameStr = akGeneralStringImp[4];
VA_COMPGEN(0x00416fc0, 0xb, STATIC_CTOR, kDisplayDepthErrorStr)
DATA(0x004c746c) const char* const& kDisplayDepthErrorStr = akGeneralStringImp[5];
VA_COMPGEN(0x00416fe0, 0xb, STATIC_CTOR, kDebugBuildStr)
DATA(0x004c7154) const char* const& kDebugBuildStr = akGeneralStringImp[6];
VA_COMPGEN(0x00417000, 0xb, STATIC_CTOR, kReleaseBuildStr)
DATA(0x004c70f8) const char* const& kReleaseBuildStr = akGeneralStringImp[7];
VA_COMPGEN(0x00417020, 0xb, STATIC_CTOR, kOKStr)
DATA(0x004c7060) const char* const& kOKStr = akGeneralStringImp[8];
VA_COMPGEN(0x00417040, 0xb, STATIC_CTOR, kAboutCaptionStr)
DATA(0x004c7108) const char* const& kAboutCaptionStr = akGeneralStringImp[9];
VA_COMPGEN(0x00417060, 0xb, STATIC_CTOR, kPlayerNameFmtStr)
DATA(0x004c7454) const char* const& kPlayerNameFmtStr = akGeneralStringImp[10];
VA_COMPGEN(0x00417080, 0xb, STATIC_CTOR, kRandomTownStr)
DATA(0x004c7160) const char* const& kRandomTownStr = akGeneralStringImp[11];
VA_COMPGEN(0x004170a0, 0xb, STATIC_CTOR, kMapInvalidFmtStr)
DATA(0x004c7080) const char* const& kMapInvalidFmtStr = akGeneralStringImp[12];
VA_COMPGEN(0x004170c0, 0xb, STATIC_CTOR, kMapUnplayableFmtStr)
DATA(0x004c7458) const char* const& kMapUnplayableFmtStr = akGeneralStringImp[13];
VA_COMPGEN(0x004170e0, 0xb, STATIC_CTOR, kMapInvalidVersionFmtStr)
DATA(0x004c7088) const char* const& kMapInvalidVersionFmtStr = akGeneralStringImp[14];
VA_COMPGEN(0x00417100, 0xb, STATIC_CTOR, kMapOldVersionFmtStr)
DATA(0x004c7028) const char* const& kMapOldVersionFmtStr = akGeneralStringImp[15];
VA_COMPGEN(0x00417120, 0xb, STATIC_CTOR, kInvalidCampaignVersionFmtStr)
DATA(0x004c7050) const char* const& kInvalidCampaignVersionFmtStr = akGeneralStringImp[16];
VA_COMPGEN(0x00417140, 0xb, STATIC_CTOR, kInvalidCampaignFileStr)
DATA(0x004c732c) const char* const& kInvalidCampaignFileStr = akGeneralStringImp[17];
VA_COMPGEN(0x00417160, 0xb, STATIC_CTOR, kFileExistsFmtStr)
DATA(0x004c710c) const char* const& kFileExistsFmtStr = akGeneralStringImp[18];
VA_COMPGEN(0x00417180, 0xb, STATIC_CTOR, kPropertiesStr)
DATA(0x004c71b8) const char* const& kPropertiesStr = akGeneralStringImp[19];
VA_COMPGEN(0x004171a0, 0xb, STATIC_CTOR, kMostPowerfulHeroStr)
DATA(0x004c7114) const char* const& kMostPowerfulHeroStr = akGeneralStringImp[20];
VA_COMPGEN(0x004171c0, 0xb, STATIC_CTOR, kGeneratedHeroStr)
DATA(0x004c6fbc) const char* const& kGeneratedHeroStr = akGeneralStringImp[21];
VA_COMPGEN(0x004171e0, 0xb, STATIC_CTOR, kAndStr)
DATA(0x004c7390) const char* const& kAndStr = akGeneralStringImp[22];
VA_COMPGEN(0x00417200, 0xb, STATIC_CTOR, kRandomHeroStr)
DATA(0x004c7470) const char* const& kRandomHeroStr = akGeneralStringImp[23];
VA_COMPGEN(0x00417220, 0xb, STATIC_CTOR, kNoneStr)
DATA(0x004c702c) const char* const& kNoneStr = akGeneralStringImp[24];
VA_COMPGEN(0x00417240, 0xb, STATIC_CTOR, kEasyStr)
DATA(0x004c7198) const char* const& kEasyStr = akGeneralStringImp[25];
VA_COMPGEN(0x00417260, 0xb, STATIC_CTOR, kNormalStr)
DATA(0x004c7194) const char* const& kNormalStr = akGeneralStringImp[26];
VA_COMPGEN(0x00417280, 0xb, STATIC_CTOR, kHardStr)
DATA(0x004c6fcc) const char* const& kHardStr = akGeneralStringImp[27];
VA_COMPGEN(0x004172a0, 0xb, STATIC_CTOR, kExpertStr)
DATA(0x004c7394) const char* const& kExpertStr = akGeneralStringImp[28];
VA_COMPGEN(0x004172c0, 0xb, STATIC_CTOR, kImpossibleStr)
DATA(0x004c7084) const char* const& kImpossibleStr = akGeneralStringImp[29];
VA_COMPGEN(0x004172e0, 0xb, STATIC_CTOR, kMapFileFilterStr)
DATA(0x004c715c) const char* const& kMapFileFilterStr = akGeneralStringImp[30];
VA_COMPGEN(0x00417300, 0xb, STATIC_CTOR, kPrologueStr)
DATA(0x004c6fc8) const char* const& kPrologueStr = akGeneralStringImp[31];
VA_COMPGEN(0x00417320, 0xb, STATIC_CTOR, kHasPrologueStr)
DATA(0x004c704c) const char* const& kHasPrologueStr = akGeneralStringImp[32];
VA_COMPGEN(0x00417340, 0xb, STATIC_CTOR, kEpilogueStr)
DATA(0x004c7168) const char* const& kEpilogueStr = akGeneralStringImp[33];
VA_COMPGEN(0x00417360, 0xb, STATIC_CTOR, kHasEpilogueStr)
DATA(0x004c7164) const char* const& kHasEpilogueStr = akGeneralStringImp[34];
VA_COMPGEN(0x00417380, 0xb, STATIC_CTOR, kScenarioPropsSheetCaptionStr)
DATA(0x004c6f64) const char* const& kScenarioPropsSheetCaptionStr = akGeneralStringImp[35];
VA_COMPGEN(0x004173a0, 0xb, STATIC_CTOR, kSpellBonusFmtStr)
DATA(0x004c6f70) const char* const& kSpellBonusFmtStr = akGeneralStringImp[36];
VA_COMPGEN(0x004173c0, 0xb, STATIC_CTOR, kGiveBonusFmtStr)
DATA(0x004c7190) const char* const& kGiveBonusFmtStr = akGeneralStringImp[37];
VA_COMPGEN(0x004173e0, 0xb, STATIC_CTOR, kBuildBonusFmtStr)
DATA(0x004c7158) const char* const& kBuildBonusFmtStr = akGeneralStringImp[38];
VA_COMPGEN(0x00417400, 0xb, STATIC_CTOR, kReceiveBonusFmtStr)
DATA(0x004c6fc4) const char* const& kReceiveBonusFmtStr = akGeneralStringImp[39];
VA_COMPGEN(0x00417420, 0xb, STATIC_CTOR, kCancelStr)
DATA(0x004c705c) const char* const& kCancelStr = akGeneralStringImp[40];
VA_COMPGEN(0x00417440, 0xb, STATIC_CTOR, kHelpStr)
DATA(0x004c7110) const char* const& kHelpStr = akGeneralStringImp[41];
VA_COMPGEN(0x00417460, 0xb, STATIC_CTOR, kTextFileFilterStr)
DATA(0x004c7474) const char* const& kTextFileFilterStr = akGeneralStringImp[42];
VA_COMPGEN(0x00417480, 0xb, STATIC_CTOR, kImportFailedFmtStr)
DATA(0x004c6f6c) const char* const& kImportFailedFmtStr = akGeneralStringImp[43];
VA_COMPGEN(0x004174a0, 0xb, STATIC_CTOR, kImportSucceededFmtStr)
DATA(0x004c7140) const char* const& kImportSucceededFmtStr = akGeneralStringImp[44];
VA_COMPGEN(0x004174c0, 0xb, STATIC_CTOR, kNameStr)
DATA(0x004c7450) const char* const& kNameStr = akGeneralStringImp[45];
VA_COMPGEN(0x004174e0, 0xb, STATIC_CTOR, kDescriptionStr)
DATA(0x004c6f60) const char* const& kDescriptionStr = akGeneralStringImp[46];
VA_COMPGEN(0x00417500, 0xb, STATIC_CTOR, kRightClickTextStr)
DATA(0x004c7150) const char* const& kRightClickTextStr = akGeneralStringImp[47];
VA_COMPGEN(0x00417520, 0xb, STATIC_CTOR, kPrologueTextStr)
DATA(0x004c7058) const char* const& kPrologueTextStr = akGeneralStringImp[48];
VA_COMPGEN(0x00417540, 0xb, STATIC_CTOR, kEpilogueTextStr)
DATA(0x004c6fc0) const char* const& kEpilogueTextStr = akGeneralStringImp[49];

namespace SCampaignPropsDlgText {
namespace {
DATA(0x004c6f74) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x00417560, 0xb, STATIC_CTOR, SCampaignPropsDlgText::kCaptionStr)
DATA(0x004c6fb4) const char* const& kCaptionStr = akStringImp[0];
VA_COMPGEN(0x00417580, 0xb, STATIC_CTOR, SCampaignPropsDlgText::kMapStaticStr)
DATA(0x004c6fb0) const char* const& kMapStaticStr = akStringImp[1];
VA_COMPGEN(0x004175a0, 0xb, STATIC_CTOR, SCampaignPropsDlgText::kNameStaticStr)
DATA(0x004c6fa8) const char* const& kNameStaticStr = akStringImp[2];
VA_COMPGEN(0x004175c0, 0xb, STATIC_CTOR, SCampaignPropsDlgText::kDescriptionStaticStr)
DATA(0x004c6f98) const char* const& kDescriptionStaticStr = akStringImp[3];
VA_COMPGEN(0x004175e0, 0xb, STATIC_CTOR, SCampaignPropsDlgText::kDifficultyChoiceCheckStr)
DATA(0x004c6fa4) const char* const& kDifficultyChoiceCheckStr = akStringImp[4];
VA_COMPGEN(0x00417600, 0xb, STATIC_CTOR, SCampaignPropsDlgText::kMusicStaticStr)
DATA(0x004c6fa0) const char* const& kMusicStaticStr = akStringImp[5];
VA_COMPGEN(0x00417620, 0xb, STATIC_CTOR, SCampaignPropsDlgText::kVersionGroupStr)
DATA(0x004c6fb8) const char* const& kVersionGroupStr = akStringImp[6];
VA_COMPGEN(0x00417640, 0xb, STATIC_CTOR, SCampaignPropsDlgText::kArmageddonsBladeRadioStr)
DATA(0x004c6f9c) const char* const& kArmageddonsBladeRadioStr = akStringImp[7];
VA_COMPGEN(0x00417660, 0xb, STATIC_CTOR, SCampaignPropsDlgText::kShadowOfDeathRadioStr)
DATA(0x004c6fac) const char* const& kShadowOfDeathRadioStr = akStringImp[8];
}

namespace SEditCrossoverScenarioOptionDlgText {
namespace {
DATA(0x004c72c0) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x00417680, 0xb, STATIC_CTOR, SEditCrossoverScenarioOptionDlgText::kCaptionStr)
DATA(0x004c72d0) const char* const& kCaptionStr = akStringImp[0];
VA_COMPGEN(0x004176a0, 0xb, STATIC_CTOR, SEditCrossoverScenarioOptionDlgText::kScenarioStaticStr)
DATA(0x004c72cc) const char* const& kScenarioStaticStr = akStringImp[1];
VA_COMPGEN(0x004176c0, 0xb, STATIC_CTOR, SEditCrossoverScenarioOptionDlgText::kPlayerStaticStr)
DATA(0x004c72d4) const char* const& kPlayerStaticStr = akStringImp[2];
}

namespace SEditStartingBonusDlgText {
namespace {
DATA(0x004c73a4) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x004176e0, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kCaptionStr)
DATA(0x004c7434) const char* const& kCaptionStr = akStringImp[0];
VA_COMPGEN(0x00417700, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kTypeStaticStr)
DATA(0x004c7410) const char* const& kTypeStaticStr = akStringImp[1];
VA_COMPGEN(0x00417720, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kSpellRadioStr)
DATA(0x004c7424) const char* const& kSpellRadioStr = akStringImp[2];
VA_COMPGEN(0x00417740, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kCreatureRadioStr)
DATA(0x004c7404) const char* const& kCreatureRadioStr = akStringImp[3];
VA_COMPGEN(0x00417760, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kBuildingRadioStr)
DATA(0x004c73a0) const char* const& kBuildingRadioStr = akStringImp[4];
VA_COMPGEN(0x00417780, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kArtifactRadioStr)
DATA(0x004c7440) const char* const& kArtifactRadioStr = akStringImp[5];
VA_COMPGEN(0x004177a0, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kSpellScrollRadioStr)
DATA(0x004c7420) const char* const& kSpellScrollRadioStr = akStringImp[6];
VA_COMPGEN(0x004177c0, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kPrimarySkillRadioStr)
DATA(0x004c7400) const char* const& kPrimarySkillRadioStr = akStringImp[7];
VA_COMPGEN(0x004177e0, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kSecondarySkillRadioStr)
DATA(0x004c7438) const char* const& kSecondarySkillRadioStr = akStringImp[8];
VA_COMPGEN(0x00417800, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kResourceRadioStr)
DATA(0x004c743c) const char* const& kResourceRadioStr = akStringImp[9];
VA_COMPGEN(0x00417820, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kRecipientStaticStr)
DATA(0x004c7414) const char* const& kRecipientStaticStr = akStringImp[10];
VA_COMPGEN(0x00417840, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kArtifactStaticStr)
DATA(0x004c7444) const char* const& kArtifactStaticStr = akStringImp[11];
VA_COMPGEN(0x00417860, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kCreatureStaticStr)
DATA(0x004c7398) const char* const& kCreatureStaticStr = akStringImp[12];
VA_COMPGEN(0x00417880, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kQuantityStaticStr)
DATA(0x004c739c) const char* const& kQuantityStaticStr = akStringImp[13];
VA_COMPGEN(0x004178a0, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kBuildingStaticStr)
DATA(0x004c7448) const char* const& kBuildingStaticStr = akStringImp[14];
VA_COMPGEN(0x004178c0, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kSpellStaticStr)
DATA(0x004c7428) const char* const& kSpellStaticStr = akStringImp[15];
VA_COMPGEN(0x004178e0, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kAttackStaticStr)
DATA(0x004c7430) const char* const& kAttackStaticStr = akStringImp[16];
VA_COMPGEN(0x00417900, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kDefenseStaticStr)
DATA(0x004c744c) const char* const& kDefenseStaticStr = akStringImp[17];
VA_COMPGEN(0x00417920, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kSpellPowerStaticStr)
DATA(0x004c7418) const char* const& kSpellPowerStaticStr = akStringImp[18];
VA_COMPGEN(0x00417940, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kKnowledgeStaticStr)
DATA(0x004c740c) const char* const& kKnowledgeStaticStr = akStringImp[19];
VA_COMPGEN(0x00417960, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kSecondarySkillStaticStr)
DATA(0x004c7408) const char* const& kSecondarySkillStaticStr = akStringImp[20];
VA_COMPGEN(0x00417980, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kMasteryStaticStr)
DATA(0x004c742c) const char* const& kMasteryStaticStr = akStringImp[21];
VA_COMPGEN(0x004179a0, 0xb, STATIC_CTOR, SEditStartingBonusDlgText::kResourceStaticStr)
DATA(0x004c741c) const char* const& kResourceStaticStr = akStringImp[22];
}

namespace SEditStartingHeroOptionDlgText {
namespace {
DATA(0x004c71c0) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x004179c0, 0xb, STATIC_CTOR, SEditStartingHeroOptionDlgText::kCaptionStr)
DATA(0x004c71cc) const char* const& kCaptionStr = akStringImp[0];
VA_COMPGEN(0x004179e0, 0xb, STATIC_CTOR, SEditStartingHeroOptionDlgText::kHeroStaticStr)
DATA(0x004c71bc) const char* const& kHeroStaticStr = akStringImp[1];
VA_COMPGEN(0x00417a00, 0xb, STATIC_CTOR, SEditStartingHeroOptionDlgText::kPlayerStaticStr)
DATA(0x004c71d0) const char* const& kPlayerStaticStr = akStringImp[2];
}

namespace SNewCampaignDlgText {
namespace {
DATA(0x004c7118) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x00417a20, 0xb, STATIC_CTOR, SNewCampaignDlgText::kCaptionStr)
DATA(0x004c7138) const char* const& kCaptionStr = akStringImp[0];
VA_COMPGEN(0x00417a40, 0xb, STATIC_CTOR, SNewCampaignDlgText::kMapStaticStr)
DATA(0x004c712c) const char* const& kMapStaticStr = akStringImp[1];
VA_COMPGEN(0x00417a60, 0xb, STATIC_CTOR, SNewCampaignDlgText::kVersionGroupStr)
DATA(0x004c713c) const char* const& kVersionGroupStr = akStringImp[2];
VA_COMPGEN(0x00417a80, 0xb, STATIC_CTOR, SNewCampaignDlgText::kArmageddonsBladeRadioStr)
DATA(0x004c7130) const char* const& kArmageddonsBladeRadioStr = akStringImp[3];
VA_COMPGEN(0x00417aa0, 0xb, STATIC_CTOR, SNewCampaignDlgText::kShadowOfDeathRadioStr)
DATA(0x004c7134) const char* const& kShadowOfDeathRadioStr = akStringImp[4];
}

namespace SScenarioPropsCrossoverPageText {
namespace {
DATA(0x004c6fd8) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x00417ac0, 0xb, STATIC_CTOR, SScenarioPropsCrossoverPageText::kCaptionStr)
DATA(0x004c7018) const char* const& kCaptionStr = akStringImp[0];
VA_COMPGEN(0x00417ae0, 0xb, STATIC_CTOR, SScenarioPropsCrossoverPageText::kRetainStaticStr)
DATA(0x004c6fd4) const char* const& kRetainStaticStr = akStringImp[1];
VA_COMPGEN(0x00417b00, 0xb, STATIC_CTOR, SScenarioPropsCrossoverPageText::kExperienceCheckStr)
DATA(0x004c701c) const char* const& kExperienceCheckStr = akStringImp[2];
VA_COMPGEN(0x00417b20, 0xb, STATIC_CTOR, SScenarioPropsCrossoverPageText::kPrimarySkillsCheckStr)
DATA(0x004c700c) const char* const& kPrimarySkillsCheckStr = akStringImp[3];
VA_COMPGEN(0x00417b40, 0xb, STATIC_CTOR, SScenarioPropsCrossoverPageText::kSecondarySkillsCheckStr)
DATA(0x004c7008) const char* const& kSecondarySkillsCheckStr = akStringImp[4];
VA_COMPGEN(0x00417b60, 0xb, STATIC_CTOR, SScenarioPropsCrossoverPageText::kSpellsCheckStr)
DATA(0x004c7010) const char* const& kSpellsCheckStr = akStringImp[5];
VA_COMPGEN(0x00417b80, 0xb, STATIC_CTOR, SScenarioPropsCrossoverPageText::kArtifactsCheckStr)
DATA(0x004c6fd0) const char* const& kArtifactsCheckStr = akStringImp[6];
VA_COMPGEN(0x00417ba0, 0xb, STATIC_CTOR, SScenarioPropsCrossoverPageText::kCreaturesStaticStr)
DATA(0x004c7024) const char* const& kCreaturesStaticStr = akStringImp[7];
VA_COMPGEN(0x00417bc0, 0xb, STATIC_CTOR, SScenarioPropsCrossoverPageText::kAllButtonStr)
DATA(0x004c7020) const char* const& kAllButtonStr = akStringImp[8];
VA_COMPGEN(0x00417be0, 0xb, STATIC_CTOR, SScenarioPropsCrossoverPageText::kNoneButtonStr)
DATA(0x004c7014) const char* const& kNoneButtonStr = akStringImp[9];
VA_COMPGEN(0x00417c00, 0xb, STATIC_CTOR, SScenarioPropsCrossoverPageText::kArtifactsStaticStr)
DATA(0x004c7004) const char* const& kArtifactsStaticStr = akStringImp[10];
}

namespace SScenarioPropsGeneralPageText {
namespace {
DATA(0x004c7098) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x00417c20, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kCaptionStr)
DATA(0x004c70e4) const char* const& kCaptionStr = akStringImp[0];
VA_COMPGEN(0x00417c40, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kRegionNameStaticStr)
DATA(0x004c70c8) const char* const& kRegionNameStaticStr = akStringImp[1];
VA_COMPGEN(0x00417c60, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kRegionColorStaticStr)
DATA(0x004c70d8) const char* const& kRegionColorStaticStr = akStringImp[2];
VA_COMPGEN(0x00417c80, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kScenarioNameStaticStr)
DATA(0x004c70e0) const char* const& kScenarioNameStaticStr = akStringImp[3];
VA_COMPGEN(0x00417ca0, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kMapFileGroupStr)
DATA(0x004c70ec) const char* const& kMapFileGroupStr = akStringImp[4];
VA_COMPGEN(0x00417cc0, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kImportButtonStr)
DATA(0x004c70dc) const char* const& kImportButtonStr = akStringImp[5];
VA_COMPGEN(0x00417ce0, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kRefreshButtonStr)
DATA(0x004c70f0) const char* const& kRefreshButtonStr = akStringImp[6];
VA_COMPGEN(0x00417d00, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kExportButtonStr)
DATA(0x004c70e8) const char* const& kExportButtonStr = akStringImp[7];
VA_COMPGEN(0x00417d20, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kRemoveButtonStr)
DATA(0x004c70cc) const char* const& kRemoveButtonStr = akStringImp[8];
VA_COMPGEN(0x00417d40, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kDifficultyStaticStr)
DATA(0x004c70f4) const char* const& kDifficultyStaticStr = akStringImp[9];
VA_COMPGEN(0x00417d60, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kPrerequisitesStaticStr)
DATA(0x004c70d0) const char* const& kPrerequisitesStaticStr = akStringImp[10];
VA_COMPGEN(0x00417d80, 0xb, STATIC_CTOR, SScenarioPropsGeneralPageText::kRegionTextStaticStr)
DATA(0x004c70d4) const char* const& kRegionTextStaticStr = akStringImp[11];
}

namespace SScenarioPropsProloguePageText {
namespace {
DATA(0x004c71d8) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x00417da0, 0xb, STATIC_CTOR, SScenarioPropsProloguePageText::kMovieStaticStr)
DATA(0x004c71e4) const char* const& kMovieStaticStr = akStringImp[0];
VA_COMPGEN(0x00417dc0, 0xb, STATIC_CTOR, SScenarioPropsProloguePageText::kMusicStaticStr)
DATA(0x004c71e8) const char* const& kMusicStaticStr = akStringImp[1];
VA_COMPGEN(0x00417de0, 0xb, STATIC_CTOR, SScenarioPropsProloguePageText::kMessageStaticStr)
DATA(0x004c71ec) const char* const& kMessageStaticStr = akStringImp[2];
}

namespace SScenarioPropsStartingOptionsPageText {
namespace {
DATA(0x004c7330) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x00417e00, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kCaptionStr)
DATA(0x004c7380) const char* const& kCaptionStr = akStringImp[0];
VA_COMPGEN(0x00417e20, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kTypeStaticStr)
DATA(0x004c7370) const char* const& kTypeStaticStr = akStringImp[1];
VA_COMPGEN(0x00417e40, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kBonusRadioStr)
DATA(0x004c7388) const char* const& kBonusRadioStr = akStringImp[2];
VA_COMPGEN(0x00417e60, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kCrossoverRadioStr)
DATA(0x004c7374) const char* const& kCrossoverRadioStr = akStringImp[3];
VA_COMPGEN(0x00417e80, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kStartingHeroRadioStr)
DATA(0x004c736c) const char* const& kStartingHeroRadioStr = akStringImp[4];
VA_COMPGEN(0x00417ea0, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kPlayerStaticStr)
DATA(0x004c7384) const char* const& kPlayerStaticStr = akStringImp[5];
VA_COMPGEN(0x00417ec0, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kOptionsStaticStr)
DATA(0x004c737c) const char* const& kOptionsStaticStr = akStringImp[6];
VA_COMPGEN(0x00417ee0, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kAddButtonStr)
DATA(0x004c7364) const char* const& kAddButtonStr = akStringImp[7];
VA_COMPGEN(0x00417f00, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kEditButtonStr)
DATA(0x004c738c) const char* const& kEditButtonStr = akStringImp[8];
VA_COMPGEN(0x00417f20, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kRemoveButtonStr)
DATA(0x004c7378) const char* const& kRemoveButtonStr = akStringImp[9];
VA_COMPGEN(0x00417f40, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kScenarioColumnStr)
DATA(0x004c7360) const char* const& kScenarioColumnStr = akStringImp[10];
VA_COMPGEN(0x00417f60, 0xb, STATIC_CTOR, SScenarioPropsStartingOptionsPageText::kHeroColumnStr)
DATA(0x004c7368) const char* const& kHeroColumnStr = akStringImp[11];
}

namespace SMainMenuText {
namespace {
DATA(0x004c7034) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x00417f80, 0xb, STATIC_CTOR, SMainMenuText::kFileStr)
DATA(0x004c7040) const char* const& kFileStr = akStringImp[0];
VA_COMPGEN(0x00417fa0, 0xb, STATIC_CTOR, SMainMenuText::kEditStr)
DATA(0x004c7044) const char* const& kEditStr = akStringImp[1];
VA_COMPGEN(0x00417fc0, 0xb, STATIC_CTOR, SMainMenuText::kHelpStr)
DATA(0x004c7030) const char* const& kHelpStr = akStringImp[2];
}

namespace SFileMenuText {
namespace {
DATA(0x004c72dc) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x00417fe0, 0xb, STATIC_CTOR, SFileMenuText::kNewStr)
DATA(0x004c730c) const char* const& kNewStr = akStringImp[0];
VA_COMPGEN(0x00418000, 0xb, STATIC_CTOR, SFileMenuText::kOpenStr)
DATA(0x004c7310) const char* const& kOpenStr = akStringImp[1];
VA_COMPGEN(0x00418020, 0xb, STATIC_CTOR, SFileMenuText::kSaveStr)
DATA(0x004c7320) const char* const& kSaveStr = akStringImp[2];
VA_COMPGEN(0x00418040, 0xb, STATIC_CTOR, SFileMenuText::kSaveAsStr)
DATA(0x004c7324) const char* const& kSaveAsStr = akStringImp[3];
VA_COMPGEN(0x00418060, 0xb, STATIC_CTOR, SFileMenuText::kExportTextStr)
DATA(0x004c731c) const char* const& kExportTextStr = akStringImp[4];
VA_COMPGEN(0x00418080, 0xb, STATIC_CTOR, SFileMenuText::kImportTextStr)
DATA(0x004c7328) const char* const& kImportTextStr = akStringImp[5];
VA_COMPGEN(0x004180a0, 0xb, STATIC_CTOR, SFileMenuText::kRefreshMapsStr)
DATA(0x004c7304) const char* const& kRefreshMapsStr = akStringImp[6];
VA_COMPGEN(0x004180c0, 0xb, STATIC_CTOR, SFileMenuText::kExportMapsStr)
DATA(0x004c7308) const char* const& kExportMapsStr = akStringImp[7];
VA_COMPGEN(0x004180e0, 0xb, STATIC_CTOR, SFileMenuText::kRecentFilesStr)
DATA(0x004c7318) const char* const& kRecentFilesStr = akStringImp[8];
VA_COMPGEN(0x00418100, 0xb, STATIC_CTOR, SFileMenuText::kExitStr)
DATA(0x004c7314) const char* const& kExitStr = akStringImp[9];
}

namespace SEditMenuText {
namespace {
DATA(0x004c745c) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x00418120, 0xb, STATIC_CTOR, SEditMenuText::kCampaignPropertiesStr)
DATA(0x004c7468) const char* const& kCampaignPropertiesStr = akStringImp[0];
VA_COMPGEN(0x00418140, 0xb, STATIC_CTOR, SEditMenuText::kScenarioPropertiesStr)
DATA(0x004c7464) const char* const& kScenarioPropertiesStr = akStringImp[1];
}

namespace SHelpMenuText {
namespace {
DATA(0x004c6f50) const char* akStringImp[kNumStrings];
}
VA_COMPGEN(0x00418160, 0xb, STATIC_CTOR, SHelpMenuText::kHelpTopicsStr)
DATA(0x004c6f5c) const char* const& kHelpTopicsStr = akStringImp[0];
VA_COMPGEN(0x00418180, 0xb, STATIC_CTOR, SHelpMenuText::kAboutStr)
DATA(0x004c6f58) const char* const& kAboutStr = akStringImp[1];
}

// Loki MapEditorText.cpp uses a running baseID and one index for both passes.
// The campaign retail body follows the same group-copy algorithm; advancing
// past each following heading here recovers its address-calculation order.
#define LOAD_TEXT_GROUP(apStrings, numStrings, pBuffer, bufferDeclaration)               \
    {                                                                                    \
        size_t size = 0;                                                                 \
        unsigned int i;                                                                  \
        for (i = 0; i < numStrings; i++)                                                 \
            size += getTextLength(pText->GetText(baseID + i)) + 1;                       \
        bufferDeclaration                                                                \
        char* p = pBuffer.get();                                                         \
        if (p == NULL)                                                                   \
            throw TAllocationFailure();                                                  \
        for (i = 0; i < numStrings; i++) {                                               \
            const char* pString = pText->GetText(baseID + i);                            \
            size_t length = getTextLength(pString) + 1;                                  \
            copyText(p, pString);                                                        \
            apStrings[i] = p;                                                            \
            p += length;                                                                 \
        }                                                                                \
        baseID += numStrings + 1;                                                        \
    }

// Retail packs the first eight buffer-initialization flags at 0x004c719c
// and the remaining six at 0x004c71d4 (loads/stores at +0xc1/+0xcd and
// +0x731/+0x73d). Each claim names the shared byte through its first owner.
VA(0x00418190, 0xc04)
void SText::initialize()
{
    TResourcePtr<TTextResource> pText(ResourceManager::GetText("CmpEditr.txt"));
    if (pText.get() == NULL)
        throw TRuntimeError();
    unsigned int baseID = 2;
    LOAD_TEXT_GROUP(akGeneralStringImp, kNumGeneralStrings, pGeneralTexts,
        DATA_COMPGEN_GUARD(0x004c719c, firstTextBuffersGuard, pGeneralTexts)
        DATA(0x004c7100) static TAutoArrayPtr<char> pGeneralTexts(new char[size]);)
    LOAD_TEXT_GROUP(SCampaignPropsDlgText::akStringImp, SCampaignPropsDlgText::kNumStrings, pCampaignPropsDlgTexts,
        DATA(0x004c7148) static TAutoArrayPtr<char> pCampaignPropsDlgTexts(new char[size]);)
    LOAD_TEXT_GROUP(SEditCrossoverScenarioOptionDlgText::akStringImp, SEditCrossoverScenarioOptionDlgText::kNumStrings, pEditCrossoverScenarioOptionDlgTexts,
        DATA(0x004c72b8) static TAutoArrayPtr<char> pEditCrossoverScenarioOptionDlgTexts(new char[size]);)
    LOAD_TEXT_GROUP(SEditStartingBonusDlgText::akStringImp, SEditStartingBonusDlgText::kNumStrings, pEditStartingBonusDlgTexts,
        DATA(0x004c7078) static TAutoArrayPtr<char> pEditStartingBonusDlgTexts(new char[size]);)
    LOAD_TEXT_GROUP(SEditStartingHeroOptionDlgText::akStringImp, SEditStartingHeroOptionDlgText::kNumStrings, pEditStartingHeroOptionDlgTexts,
        DATA(0x004c7188) static TAutoArrayPtr<char> pEditStartingHeroOptionDlgTexts(new char[size]);)
    LOAD_TEXT_GROUP(SNewCampaignDlgText::akStringImp, SNewCampaignDlgText::kNumStrings, pNewCampaignDlgTexts,
        DATA(0x004c7070) static TAutoArrayPtr<char> pNewCampaignDlgTexts(new char[size]);)
    LOAD_TEXT_GROUP(SScenarioPropsCrossoverPageText::akStringImp, SScenarioPropsCrossoverPageText::kNumStrings, pScenarioPropsCrossoverPageTexts,
        DATA(0x004c7180) static TAutoArrayPtr<char> pScenarioPropsCrossoverPageTexts(new char[size]);)
    LOAD_TEXT_GROUP(SScenarioPropsGeneralPageText::akStringImp, SScenarioPropsGeneralPageText::kNumStrings, pScenarioPropsGeneralPageTexts,
        DATA(0x004c7068) static TAutoArrayPtr<char> pScenarioPropsGeneralPageTexts(new char[size]);)
    LOAD_TEXT_GROUP(SScenarioPropsProloguePageText::akStringImp, SScenarioPropsProloguePageText::kNumStrings, pScenarioPropsProloguePageTexts,
        DATA_COMPGEN_GUARD(0x004c71d4, remainingTextBuffersGuard, pScenarioPropsProloguePageTexts)
        DATA(0x004c7178) static TAutoArrayPtr<char> pScenarioPropsProloguePageTexts(new char[size]);)
    LOAD_TEXT_GROUP(SScenarioPropsStartingOptionsPageText::akStringImp, SScenarioPropsStartingOptionsPageText::kNumStrings, pScenarioPropsStartingOptionsPageTexts,
        DATA(0x004c71b0) static TAutoArrayPtr<char> pScenarioPropsStartingOptionsPageTexts(new char[size]);)
    LOAD_TEXT_GROUP(SMainMenuText::akStringImp, SMainMenuText::kNumStrings, pMainMenuTexts,
        DATA(0x004c7170) static TAutoArrayPtr<char> pMainMenuTexts(new char[size]);)
    LOAD_TEXT_GROUP(SFileMenuText::akStringImp, SFileMenuText::kNumStrings, pFileMenuTexts,
        DATA(0x004c71a8) static TAutoArrayPtr<char> pFileMenuTexts(new char[size]);)
    LOAD_TEXT_GROUP(SEditMenuText::akStringImp, SEditMenuText::kNumStrings, pEditMenuTexts,
        DATA(0x004c7090) static TAutoArrayPtr<char> pEditMenuTexts(new char[size]);)
    LOAD_TEXT_GROUP(SHelpMenuText::akStringImp, SHelpMenuText::kNumStrings, pHelpMenuTexts,
        DATA(0x004c71a0) static TAutoArrayPtr<char> pHelpMenuTexts(new char[size]);)
}

#undef LOAD_TEXT_GROUP

// Retail initialize() registers these cleanups in group order, from 0x4190b0
// down to 0x418f10. Each cleanup tests its buffer's ownership byte and deletes
// its pointer at +4; the DATA claims above identify the same initialization stores.
VA_COMPGEN(0x004190b0, 0x16, STATIC_DTOR, pGeneralTexts)
VA_COMPGEN(0x00419090, 0x16, STATIC_DTOR, pCampaignPropsDlgTexts)
VA_COMPGEN(0x00419070, 0x16, STATIC_DTOR, pEditCrossoverScenarioOptionDlgTexts)
VA_COMPGEN(0x00419050, 0x16, STATIC_DTOR, pEditStartingBonusDlgTexts)
VA_COMPGEN(0x00419030, 0x16, STATIC_DTOR, pEditStartingHeroOptionDlgTexts)
VA_COMPGEN(0x00419010, 0x16, STATIC_DTOR, pNewCampaignDlgTexts)
VA_COMPGEN(0x00418ff0, 0x16, STATIC_DTOR, pScenarioPropsCrossoverPageTexts)
VA_COMPGEN(0x00418fd0, 0x16, STATIC_DTOR, pScenarioPropsGeneralPageTexts)
VA_COMPGEN(0x00418fb0, 0x16, STATIC_DTOR, pScenarioPropsProloguePageTexts)
VA_COMPGEN(0x00418f90, 0x16, STATIC_DTOR, pScenarioPropsStartingOptionsPageTexts)
VA_COMPGEN(0x00418f70, 0x16, STATIC_DTOR, pMainMenuTexts)
VA_COMPGEN(0x00418f50, 0x16, STATIC_DTOR, pFileMenuTexts)
VA_COMPGEN(0x00418f30, 0x16, STATIC_DTOR, pEditMenuTexts)
VA_COMPGEN(0x00418f10, 0x16, STATIC_DTOR, pHelpMenuTexts)
