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

const char kEllipsis[] = "\x85";

// The length of a text once each ellipsis is spelled out.
VA(0x00416ea0, 0x3e)
int getTextLength(const char* pText)
{
    int length = strcspn(pText, kEllipsis);
    const char* p = pText + length;
    while (*p != '\0') {
        p++;
        int span = strcspn(p, kEllipsis);
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

DATA(0x004c70fc) const char* const& kAppTitleStr = akGeneralStringImp[0];
DATA(0x004c6f68) const char* const& kCopyrightStr = akGeneralStringImp[1];
DATA(0x004c7048) const char* const& kVersionFmtStr = akGeneralStringImp[2];
DATA(0x004c7054) const char* const& kCampaignFileFilterStr = akGeneralStringImp[3];
DATA(0x004c72d8) const char* const& kCampaignFileRegNameStr = akGeneralStringImp[4];
DATA(0x004c746c) const char* const& kDisplayDepthErrorStr = akGeneralStringImp[5];
DATA(0x004c7154) const char* const& kDebugBuildStr = akGeneralStringImp[6];
DATA(0x004c70f8) const char* const& kReleaseBuildStr = akGeneralStringImp[7];
DATA(0x004c7060) const char* const& kOKStr = akGeneralStringImp[8];
DATA(0x004c7108) const char* const& kAboutCaptionStr = akGeneralStringImp[9];
DATA(0x004c7454) const char* const& kPlayerNameFmtStr = akGeneralStringImp[10];
DATA(0x004c7160) const char* const& kRandomTownStr = akGeneralStringImp[11];
DATA(0x004c7080) const char* const& kMapInvalidFmtStr = akGeneralStringImp[12];
DATA(0x004c7458) const char* const& kMapUnplayableFmtStr = akGeneralStringImp[13];
DATA(0x004c7088) const char* const& kMapInvalidVersionFmtStr = akGeneralStringImp[14];
DATA(0x004c7028) const char* const& kMapOldVersionFmtStr = akGeneralStringImp[15];
DATA(0x004c7050) const char* const& kInvalidCampaignVersionFmtStr = akGeneralStringImp[16];
DATA(0x004c732c) const char* const& kInvalidCampaignFileStr = akGeneralStringImp[17];
DATA(0x004c710c) const char* const& kFileExistsFmtStr = akGeneralStringImp[18];
DATA(0x004c71b8) const char* const& kPropertiesStr = akGeneralStringImp[19];
DATA(0x004c7114) const char* const& kMostPowerfulHeroStr = akGeneralStringImp[20];
DATA(0x004c6fbc) const char* const& kGeneratedHeroStr = akGeneralStringImp[21];
DATA(0x004c7390) const char* const& kAndStr = akGeneralStringImp[22];
DATA(0x004c7470) const char* const& kRandomHeroStr = akGeneralStringImp[23];
DATA(0x004c702c) const char* const& kNoneStr = akGeneralStringImp[24];
DATA(0x004c7198) const char* const& kEasyStr = akGeneralStringImp[25];
DATA(0x004c7194) const char* const& kNormalStr = akGeneralStringImp[26];
DATA(0x004c6fcc) const char* const& kHardStr = akGeneralStringImp[27];
DATA(0x004c7394) const char* const& kExpertStr = akGeneralStringImp[28];
DATA(0x004c7084) const char* const& kImpossibleStr = akGeneralStringImp[29];
DATA(0x004c715c) const char* const& kMapFileFilterStr = akGeneralStringImp[30];
DATA(0x004c6fc8) const char* const& kPrologueStr = akGeneralStringImp[31];
DATA(0x004c704c) const char* const& kHasPrologueStr = akGeneralStringImp[32];
DATA(0x004c7168) const char* const& kEpilogueStr = akGeneralStringImp[33];
DATA(0x004c7164) const char* const& kHasEpilogueStr = akGeneralStringImp[34];
DATA(0x004c6f64) const char* const& kScenarioPropsSheetCaptionStr = akGeneralStringImp[35];
DATA(0x004c6f70) const char* const& kSpellBonusFmtStr = akGeneralStringImp[36];
DATA(0x004c7190) const char* const& kGiveBonusFmtStr = akGeneralStringImp[37];
DATA(0x004c7158) const char* const& kBuildBonusFmtStr = akGeneralStringImp[38];
DATA(0x004c6fc4) const char* const& kReceiveBonusFmtStr = akGeneralStringImp[39];
DATA(0x004c705c) const char* const& kCancelStr = akGeneralStringImp[40];
DATA(0x004c7110) const char* const& kHelpStr = akGeneralStringImp[41];
DATA(0x004c7474) const char* const& kTextFileFilterStr = akGeneralStringImp[42];
DATA(0x004c6f6c) const char* const& kImportFailedFmtStr = akGeneralStringImp[43];
DATA(0x004c7140) const char* const& kImportSucceededFmtStr = akGeneralStringImp[44];
DATA(0x004c7450) const char* const& kNameStr = akGeneralStringImp[45];
DATA(0x004c6f60) const char* const& kDescriptionStr = akGeneralStringImp[46];
DATA(0x004c7150) const char* const& kRightClickTextStr = akGeneralStringImp[47];
DATA(0x004c7058) const char* const& kPrologueTextStr = akGeneralStringImp[48];
DATA(0x004c6fc0) const char* const& kEpilogueTextStr = akGeneralStringImp[49];

namespace SCampaignPropsDlgText {
namespace {
DATA(0x004c6f74) const char* akStringImp[kNumStrings];
}
DATA(0x004c6fb4) const char* const& kCaptionStr = akStringImp[0];
DATA(0x004c6fb0) const char* const& kMapStaticStr = akStringImp[1];
DATA(0x004c6fa8) const char* const& kNameStaticStr = akStringImp[2];
DATA(0x004c6f98) const char* const& kDescriptionStaticStr = akStringImp[3];
DATA(0x004c6fa4) const char* const& kDifficultyChoiceCheckStr = akStringImp[4];
DATA(0x004c6fa0) const char* const& kMusicStaticStr = akStringImp[5];
DATA(0x004c6fb8) const char* const& kVersionGroupStr = akStringImp[6];
DATA(0x004c6f9c) const char* const& kArmageddonsBladeRadioStr = akStringImp[7];
DATA(0x004c6fac) const char* const& kShadowOfDeathRadioStr = akStringImp[8];
}

namespace SEditCrossoverScenarioOptionDlgText {
namespace {
DATA(0x004c72c0) const char* akStringImp[kNumStrings];
}
DATA(0x004c72d0) const char* const& kCaptionStr = akStringImp[0];
DATA(0x004c72cc) const char* const& kScenarioStaticStr = akStringImp[1];
DATA(0x004c72d4) const char* const& kPlayerStaticStr = akStringImp[2];
}

namespace SEditStartingBonusDlgText {
namespace {
DATA(0x004c73a4) const char* akStringImp[kNumStrings];
}
DATA(0x004c7434) const char* const& kCaptionStr = akStringImp[0];
DATA(0x004c7410) const char* const& kTypeStaticStr = akStringImp[1];
DATA(0x004c7424) const char* const& kSpellRadioStr = akStringImp[2];
DATA(0x004c7404) const char* const& kCreatureRadioStr = akStringImp[3];
DATA(0x004c73a0) const char* const& kBuildingRadioStr = akStringImp[4];
DATA(0x004c7440) const char* const& kArtifactRadioStr = akStringImp[5];
DATA(0x004c7420) const char* const& kSpellScrollRadioStr = akStringImp[6];
DATA(0x004c7400) const char* const& kPrimarySkillRadioStr = akStringImp[7];
DATA(0x004c7438) const char* const& kSecondarySkillRadioStr = akStringImp[8];
DATA(0x004c743c) const char* const& kResourceRadioStr = akStringImp[9];
DATA(0x004c7414) const char* const& kRecipientStaticStr = akStringImp[10];
DATA(0x004c7444) const char* const& kArtifactStaticStr = akStringImp[11];
DATA(0x004c7398) const char* const& kCreatureStaticStr = akStringImp[12];
DATA(0x004c739c) const char* const& kQuantityStaticStr = akStringImp[13];
DATA(0x004c7448) const char* const& kBuildingStaticStr = akStringImp[14];
DATA(0x004c7428) const char* const& kSpellStaticStr = akStringImp[15];
DATA(0x004c7430) const char* const& kAttackStaticStr = akStringImp[16];
DATA(0x004c744c) const char* const& kDefenseStaticStr = akStringImp[17];
DATA(0x004c7418) const char* const& kSpellPowerStaticStr = akStringImp[18];
DATA(0x004c740c) const char* const& kKnowledgeStaticStr = akStringImp[19];
DATA(0x004c7408) const char* const& kSecondarySkillStaticStr = akStringImp[20];
DATA(0x004c742c) const char* const& kMasteryStaticStr = akStringImp[21];
DATA(0x004c741c) const char* const& kResourceStaticStr = akStringImp[22];
}

namespace SEditStartingHeroOptionDlgText {
namespace {
DATA(0x004c71c0) const char* akStringImp[kNumStrings];
}
DATA(0x004c71cc) const char* const& kCaptionStr = akStringImp[0];
DATA(0x004c71bc) const char* const& kHeroStaticStr = akStringImp[1];
DATA(0x004c71d0) const char* const& kPlayerStaticStr = akStringImp[2];
}

namespace SNewCampaignDlgText {
namespace {
DATA(0x004c7118) const char* akStringImp[kNumStrings];
}
DATA(0x004c7138) const char* const& kCaptionStr = akStringImp[0];
DATA(0x004c712c) const char* const& kMapStaticStr = akStringImp[1];
DATA(0x004c713c) const char* const& kVersionGroupStr = akStringImp[2];
DATA(0x004c7130) const char* const& kArmageddonsBladeRadioStr = akStringImp[3];
DATA(0x004c7134) const char* const& kShadowOfDeathRadioStr = akStringImp[4];
}

namespace SScenarioPropsCrossoverPageText {
namespace {
DATA(0x004c6fd8) const char* akStringImp[kNumStrings];
}
DATA(0x004c7018) const char* const& kCaptionStr = akStringImp[0];
DATA(0x004c6fd4) const char* const& kRetainStaticStr = akStringImp[1];
DATA(0x004c701c) const char* const& kExperienceCheckStr = akStringImp[2];
DATA(0x004c700c) const char* const& kPrimarySkillsCheckStr = akStringImp[3];
DATA(0x004c7008) const char* const& kSecondarySkillsCheckStr = akStringImp[4];
DATA(0x004c7010) const char* const& kSpellsCheckStr = akStringImp[5];
DATA(0x004c6fd0) const char* const& kArtifactsCheckStr = akStringImp[6];
DATA(0x004c7024) const char* const& kCreaturesStaticStr = akStringImp[7];
DATA(0x004c7020) const char* const& kAllButtonStr = akStringImp[8];
DATA(0x004c7014) const char* const& kNoneButtonStr = akStringImp[9];
DATA(0x004c7004) const char* const& kArtifactsStaticStr = akStringImp[10];
}

namespace SScenarioPropsGeneralPageText {
namespace {
DATA(0x004c7098) const char* akStringImp[kNumStrings];
}
DATA(0x004c70e4) const char* const& kCaptionStr = akStringImp[0];
DATA(0x004c70c8) const char* const& kRegionNameStaticStr = akStringImp[1];
DATA(0x004c70d8) const char* const& kRegionColorStaticStr = akStringImp[2];
DATA(0x004c70e0) const char* const& kScenarioNameStaticStr = akStringImp[3];
DATA(0x004c70ec) const char* const& kMapFileGroupStr = akStringImp[4];
DATA(0x004c70dc) const char* const& kImportButtonStr = akStringImp[5];
DATA(0x004c70f0) const char* const& kRefreshButtonStr = akStringImp[6];
DATA(0x004c70e8) const char* const& kExportButtonStr = akStringImp[7];
DATA(0x004c70cc) const char* const& kRemoveButtonStr = akStringImp[8];
DATA(0x004c70f4) const char* const& kDifficultyStaticStr = akStringImp[9];
DATA(0x004c70d0) const char* const& kPrerequisitesStaticStr = akStringImp[10];
DATA(0x004c70d4) const char* const& kRegionTextStaticStr = akStringImp[11];
}

namespace SScenarioPropsProloguePageText {
namespace {
DATA(0x004c71d8) const char* akStringImp[kNumStrings];
}
DATA(0x004c71e4) const char* const& kMovieStaticStr = akStringImp[0];
DATA(0x004c71e8) const char* const& kMusicStaticStr = akStringImp[1];
DATA(0x004c71ec) const char* const& kMessageStaticStr = akStringImp[2];
}

namespace SScenarioPropsStartingOptionsPageText {
namespace {
DATA(0x004c7330) const char* akStringImp[kNumStrings];
}
DATA(0x004c7380) const char* const& kCaptionStr = akStringImp[0];
DATA(0x004c7370) const char* const& kTypeStaticStr = akStringImp[1];
DATA(0x004c7388) const char* const& kBonusRadioStr = akStringImp[2];
DATA(0x004c7374) const char* const& kCrossoverRadioStr = akStringImp[3];
DATA(0x004c736c) const char* const& kStartingHeroRadioStr = akStringImp[4];
DATA(0x004c7384) const char* const& kPlayerStaticStr = akStringImp[5];
DATA(0x004c737c) const char* const& kOptionsStaticStr = akStringImp[6];
DATA(0x004c7364) const char* const& kAddButtonStr = akStringImp[7];
DATA(0x004c738c) const char* const& kEditButtonStr = akStringImp[8];
DATA(0x004c7378) const char* const& kRemoveButtonStr = akStringImp[9];
DATA(0x004c7360) const char* const& kScenarioColumnStr = akStringImp[10];
DATA(0x004c7368) const char* const& kHeroColumnStr = akStringImp[11];
}

namespace SMainMenuText {
namespace {
DATA(0x004c7034) const char* akStringImp[kNumStrings];
}
DATA(0x004c7040) const char* const& kFileStr = akStringImp[0];
DATA(0x004c7044) const char* const& kEditStr = akStringImp[1];
DATA(0x004c7030) const char* const& kHelpStr = akStringImp[2];
}

namespace SFileMenuText {
namespace {
DATA(0x004c72dc) const char* akStringImp[kNumStrings];
}
DATA(0x004c730c) const char* const& kNewStr = akStringImp[0];
DATA(0x004c7310) const char* const& kOpenStr = akStringImp[1];
DATA(0x004c7320) const char* const& kSaveStr = akStringImp[2];
DATA(0x004c7324) const char* const& kSaveAsStr = akStringImp[3];
DATA(0x004c731c) const char* const& kExportTextStr = akStringImp[4];
DATA(0x004c7328) const char* const& kImportTextStr = akStringImp[5];
DATA(0x004c7304) const char* const& kRefreshMapsStr = akStringImp[6];
DATA(0x004c7308) const char* const& kExportMapsStr = akStringImp[7];
DATA(0x004c7318) const char* const& kRecentFilesStr = akStringImp[8];
DATA(0x004c7314) const char* const& kExitStr = akStringImp[9];
}

namespace SEditMenuText {
namespace {
DATA(0x004c745c) const char* akStringImp[kNumStrings];
}
DATA(0x004c7468) const char* const& kCampaignPropertiesStr = akStringImp[0];
DATA(0x004c7464) const char* const& kScenarioPropertiesStr = akStringImp[1];
}

namespace SHelpMenuText {
namespace {
DATA(0x004c6f50) const char* akStringImp[kNumStrings];
}
DATA(0x004c6f5c) const char* const& kHelpTopicsStr = akStringImp[0];
DATA(0x004c6f58) const char* const& kAboutStr = akStringImp[1];
}

// The first line of each group in CmpEditr.txt: each group follows its heading line.
enum {
    kGeneralLine = 2,
    kCampaignPropsDlgLine = kGeneralLine + kNumGeneralStrings + 1,
    kEditCrossoverScenarioOptionDlgLine = kCampaignPropsDlgLine + SCampaignPropsDlgText::kNumStrings + 1,
    kEditStartingBonusDlgLine = kEditCrossoverScenarioOptionDlgLine + SEditCrossoverScenarioOptionDlgText::kNumStrings + 1,
    kEditStartingHeroOptionDlgLine = kEditStartingBonusDlgLine + SEditStartingBonusDlgText::kNumStrings + 1,
    kNewCampaignDlgLine = kEditStartingHeroOptionDlgLine + SEditStartingHeroOptionDlgText::kNumStrings + 1,
    kScenarioPropsCrossoverPageLine = kNewCampaignDlgLine + SNewCampaignDlgText::kNumStrings + 1,
    kScenarioPropsGeneralPageLine = kScenarioPropsCrossoverPageLine + SScenarioPropsCrossoverPageText::kNumStrings + 1,
    kScenarioPropsProloguePageLine = kScenarioPropsGeneralPageLine + SScenarioPropsGeneralPageText::kNumStrings + 1,
    kScenarioPropsStartingOptionsPageLine = kScenarioPropsProloguePageLine + SScenarioPropsProloguePageText::kNumStrings + 1,
    kMainMenuLine = kScenarioPropsStartingOptionsPageLine + SScenarioPropsStartingOptionsPageText::kNumStrings + 1,
    kFileMenuLine = kMainMenuLine + SMainMenuText::kNumStrings + 1,
    kEditMenuLine = kFileMenuLine + SFileMenuText::kNumStrings + 1,
    kHelpMenuLine = kEditMenuLine + SEditMenuText::kNumStrings + 1
};

// Copies the texts of a group, from its first line on, into a buffer of
// the group's own and points the group's strings at the copies.
#define LOAD_TEXT_GROUP(apStrings, numStrings, firstLine, pBuffer)                  \
    {                                                                             \
        unsigned int size = 0;                                                    \
        for (unsigned int line = firstLine; line < firstLine + numStrings; line++) \
            size += getTextLength(pText->GetText(line)) + 1;                      \
        static TAutoArrayPtr<char> pBuffer(new char[size]);                       \
        char* p = pBuffer.get();                                                  \
        if (p == NULL)                                                            \
            throw TAllocationFailure();                                           \
        for (unsigned int i = 0; i < numStrings; i++) {                           \
            const char* pString = pText->GetText(firstLine + i);                  \
            unsigned int length = getTextLength(pString) + 1;                     \
            copyText(p, pString);                                                 \
            apStrings[i] = p;                                                     \
            p += length;                                                          \
        }                                                                         \
    }

VA(0x00418190, 0xc04)
void SText::initialize()
{
    TResourcePtr<TTextResource> pText(ResourceManager::GetText("CmpEditr.txt"));
    if (pText.get() == NULL)
        throw TRuntimeError();
    LOAD_TEXT_GROUP(akGeneralStringImp, kNumGeneralStrings, kGeneralLine, pGeneralTexts)
    LOAD_TEXT_GROUP(SCampaignPropsDlgText::akStringImp, SCampaignPropsDlgText::kNumStrings, kCampaignPropsDlgLine, pCampaignPropsDlgTexts)
    LOAD_TEXT_GROUP(SEditCrossoverScenarioOptionDlgText::akStringImp, SEditCrossoverScenarioOptionDlgText::kNumStrings, kEditCrossoverScenarioOptionDlgLine, pEditCrossoverScenarioOptionDlgTexts)
    LOAD_TEXT_GROUP(SEditStartingBonusDlgText::akStringImp, SEditStartingBonusDlgText::kNumStrings, kEditStartingBonusDlgLine, pEditStartingBonusDlgTexts)
    LOAD_TEXT_GROUP(SEditStartingHeroOptionDlgText::akStringImp, SEditStartingHeroOptionDlgText::kNumStrings, kEditStartingHeroOptionDlgLine, pEditStartingHeroOptionDlgTexts)
    LOAD_TEXT_GROUP(SNewCampaignDlgText::akStringImp, SNewCampaignDlgText::kNumStrings, kNewCampaignDlgLine, pNewCampaignDlgTexts)
    LOAD_TEXT_GROUP(SScenarioPropsCrossoverPageText::akStringImp, SScenarioPropsCrossoverPageText::kNumStrings, kScenarioPropsCrossoverPageLine, pScenarioPropsCrossoverPageTexts)
    LOAD_TEXT_GROUP(SScenarioPropsGeneralPageText::akStringImp, SScenarioPropsGeneralPageText::kNumStrings, kScenarioPropsGeneralPageLine, pScenarioPropsGeneralPageTexts)
    LOAD_TEXT_GROUP(SScenarioPropsProloguePageText::akStringImp, SScenarioPropsProloguePageText::kNumStrings, kScenarioPropsProloguePageLine, pScenarioPropsProloguePageTexts)
    LOAD_TEXT_GROUP(SScenarioPropsStartingOptionsPageText::akStringImp, SScenarioPropsStartingOptionsPageText::kNumStrings, kScenarioPropsStartingOptionsPageLine, pScenarioPropsStartingOptionsPageTexts)
    LOAD_TEXT_GROUP(SMainMenuText::akStringImp, SMainMenuText::kNumStrings, kMainMenuLine, pMainMenuTexts)
    LOAD_TEXT_GROUP(SFileMenuText::akStringImp, SFileMenuText::kNumStrings, kFileMenuLine, pFileMenuTexts)
    LOAD_TEXT_GROUP(SEditMenuText::akStringImp, SEditMenuText::kNumStrings, kEditMenuLine, pEditMenuTexts)
    LOAD_TEXT_GROUP(SHelpMenuText::akStringImp, SHelpMenuText::kNumStrings, kHelpMenuLine, pHelpMenuTexts)
}

#undef LOAD_TEXT_GROUP
