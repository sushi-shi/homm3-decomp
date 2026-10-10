// ScenarioPropsCrossoverPage.cpp - the crossover page of the scenario
// properties sheet (h3ccmped 0x429e30..0x42aa30). The page shows nothing
// to edit while the scenario has no map.
#include "campaign_editor/stdafx.h"

#include <afxcmn.h>

#include "armygrp.h"
#include "artifact.h"
#include "va.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/ScenarioPropsCrossoverPage.h"

VA(0x00429e60, 0x14a)
TScenarioPropsCrossoverPage::TScenarioPropsCrossoverPage(const TScenario& oldScenario, TScenario& newScenario,
                                                         int campaignVersion)
    : CPropertyPage(TScenarioPropsCrossoverPage::IDD),
      _m_oldScenario(oldScenario),
      _m_newScenario(newScenario),
      _m_campaignVersion(campaignVersion),
      _m_bModified(false),
      _m_bInitialized(false)
{
    m_psp.pszTitle = m_strCaption = SScenarioPropsCrossoverPageText::kCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00429fb0, 0x1e, SCALAR_DELETING_DTOR, TScenarioPropsCrossoverPage)

VA(0x00429fd0, 0xb5)
TScenarioPropsCrossoverPage::~TScenarioPropsCrossoverPage()
{
}

VA(0x0042a090, 0x3b)
void TScenarioPropsCrossoverPage::update()
{
    if (_m_bInitialized) {
        _setCrossover(_m_newScenario.getCrossover());
        EnableWindow(_m_newScenario.getMap() != NULL);
    }
}

VA(0x0042a0d0, 0x1a3)
void TScenarioPropsCrossoverPage::_setCrossover(const TScenarioCrossover& crossover)
{
    _m_experienceCheck.SetCheck(crossover.m_retained[TScenarioCrossover::eRetainExperience]);
    _m_primarySkillsCheck.SetCheck(crossover.m_retained[TScenarioCrossover::eRetainPrimarySkills]);
    _m_secondarySkillsCheck.SetCheck(crossover.m_retained[TScenarioCrossover::eRetainSecondarySkills]);
    _m_spellsCheck.SetCheck(crossover.m_retained[TScenarioCrossover::eRetainSpells]);
    _m_artifactsCheck.SetCheck(crossover.m_retained[TScenarioCrossover::eRetainArtifacts]);
    int count = _m_creatureList.GetCount();
    int i;
    for (i = 0; i < count; i++)
        _m_creatureList.SetCheck(i, crossover.m_creatures[_m_creatureList.GetItemData(i)]);
    count = _m_artifactList.GetCount();
    for (i = 0; i < count; i++)
        _m_artifactList.SetCheck(i, crossover.m_artifacts[_m_artifactList.GetItemData(i)]);
}

VA(0x0042a280, 0x248)
void TScenarioPropsCrossoverPage::_getCrossover(TScenarioCrossover& crossover)
{
    crossover.m_retained[TScenarioCrossover::eRetainExperience] = _m_experienceCheck.GetCheck() != 0;
    crossover.m_retained[TScenarioCrossover::eRetainPrimarySkills] = _m_primarySkillsCheck.GetCheck() != 0;
    crossover.m_retained[TScenarioCrossover::eRetainSecondarySkills] = _m_secondarySkillsCheck.GetCheck() != 0;
    crossover.m_retained[TScenarioCrossover::eRetainSpells] = _m_spellsCheck.GetCheck() != 0;
    crossover.m_retained[TScenarioCrossover::eRetainArtifacts] = _m_artifactsCheck.GetCheck() != 0;
    std::bitset<TScenarioCrossover::kNumCreatures> creatures;
    int count = _m_creatureList.GetCount();
    int i;
    for (i = 0; i < count; i++) {
        int creature = _m_creatureList.GetItemData(i);
        creatures.set(creature, _m_creatureList.GetCheck(i) != 0);
    }
    crossover.m_creatures = creatures;
    std::bitset<TScenarioCrossover::kNumArtifacts> artifacts;
    count = _m_artifactList.GetCount();
    for (i = 0; i < count; i++) {
        int artifact = _m_artifactList.GetItemData(i);
        bool bChecked = _m_artifactList.GetCheck(i) != 0;
        artifacts[artifact] = bChecked;
    }
    crossover.m_artifacts = artifacts;
}

VA(0x0042a4d0, 0x8b)
void TScenarioPropsCrossoverPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_ARTIFACTS_CHECK, _m_artifactsCheck);
    DDX_Control(pDX, IDC_SPELLS_CHECK, _m_spellsCheck);
    DDX_Control(pDX, IDC_SECONDARY_SKILLS_CHECK, _m_secondarySkillsCheck);
    DDX_Control(pDX, IDC_PRIMARY_SKILLS_CHECK, _m_primarySkillsCheck);
    DDX_Control(pDX, IDC_EXPERIENCE_CHECK, _m_experienceCheck);
    DDX_Control(pDX, IDC_CREATURE_LIST, _m_creatureList);
    DDX_Control(pDX, IDC_ARTIFACT_LIST, _m_artifactList);
}

VA(0x0042a560, 0x6)
BEGIN_MESSAGE_MAP(TScenarioPropsCrossoverPage, CPropertyPage)
    ON_BN_CLICKED(IDC_ALL_BUTTON, OnAllButton)
    ON_BN_CLICKED(IDC_NONE_BUTTON, OnNoneButton)
    ON_WM_DESTROY()
    ON_WM_ENABLE()
END_MESSAGE_MAP()

VA(0x0042a570, 0xc)
void TScenarioPropsCrossoverPage::OnDestroy()
{
    _m_bInitialized = false;
    CPropertyPage::OnDestroy();
}

VA(0x0042a580, 0x288)
BOOL TScenarioPropsCrossoverPage::OnInitDialog()
{
    GetDlgItem(IDC_RETAIN_STATIC)->SetWindowText(SScenarioPropsCrossoverPageText::kRetainStaticStr);
    GetDlgItem(IDC_EXPERIENCE_CHECK)->SetWindowText(SScenarioPropsCrossoverPageText::kExperienceCheckStr);
    GetDlgItem(IDC_PRIMARY_SKILLS_CHECK)->SetWindowText(SScenarioPropsCrossoverPageText::kPrimarySkillsCheckStr);
    GetDlgItem(IDC_SECONDARY_SKILLS_CHECK)->SetWindowText(SScenarioPropsCrossoverPageText::kSecondarySkillsCheckStr);
    GetDlgItem(IDC_SPELLS_CHECK)->SetWindowText(SScenarioPropsCrossoverPageText::kSpellsCheckStr);
    GetDlgItem(IDC_ARTIFACTS_CHECK)->SetWindowText(SScenarioPropsCrossoverPageText::kArtifactsCheckStr);
    GetDlgItem(IDC_CREATURES_STATIC)->SetWindowText(SScenarioPropsCrossoverPageText::kCreaturesStaticStr);
    GetDlgItem(IDC_ALL_BUTTON)->SetWindowText(SScenarioPropsCrossoverPageText::kAllButtonStr);
    GetDlgItem(IDC_NONE_BUTTON)->SetWindowText(SScenarioPropsCrossoverPageText::kNoneButtonStr);
    GetDlgItem(IDC_ARTIFACTS_STATIC)->SetWindowText(SScenarioPropsCrossoverPageText::kArtifactsStaticStr);
    _m_bModified = false;
    CPropertyPage::OnInitDialog();
    CRect rect;
    _m_creatureList.GetClientRect(&rect);
    _m_creatureList.SetColumnWidth(rect.Width() / 2);
    for (unsigned int creature = 0; creature < kNumCreatureTypes; creature++) {
        if (akCreatureTypeTraits[creature].level >= 0)
            _m_creatureList.SetItemData(_m_creatureList.AddString(akCreatureTypeTraits[creature].m_name), creature);
    }
    _m_artifactList.GetClientRect(&rect);
    _m_artifactList.SetColumnWidth(rect.Width() / 2);
    unsigned int numArtifacts = _m_campaignVersion >= eCampaignVersionShadowOfDeath ? 144 : 129;
    for (unsigned int artifact = 0; artifact < numArtifacts; artifact++) {
        if (!akArtifactTraits[artifact].m_disabled && !(akArtifactTraits[artifact].m_class & ArtifactClassSpecial))
            _m_artifactList.SetItemData(_m_artifactList.AddString(akArtifactTraits[artifact].m_name), artifact);
    }
    _setCrossover(_m_newScenario.getCrossover());
    if (_m_newScenario.getMap() == NULL)
        EnableWindow(FALSE);
    _m_bInitialized = true;
    return TRUE;
}

VA(0x0042a810, 0x140)
void TScenarioPropsCrossoverPage::OnOK()
{
    CPropertyPage::OnOK();
    if (_m_newScenario.getMap() != NULL) {
        TScenarioCrossover crossover;
        _getCrossover(crossover);
        if (!(crossover == _m_newScenario.getCrossover()))
            _m_newScenario.setCrossover(crossover);
    }
    _m_bModified = _m_bModified || !(_m_newScenario.getCrossover() == _m_oldScenario.getCrossover());
}

VA(0x0042a950, 0x3c)
void TScenarioPropsCrossoverPage::OnAllButton()
{
    int count = _m_creatureList.GetCount();
    for (int i = 0; i < count; i++)
        _m_creatureList.SetCheck(i, 1);
}

VA(0x0042a990, 0x3c)
void TScenarioPropsCrossoverPage::OnNoneButton()
{
    int count = _m_creatureList.GetCount();
    for (int i = 0; i < count; i++)
        _m_creatureList.SetCheck(i, 0);
}

void TScenarioPropsCrossoverPage::OnEnable(BOOL bEnable)
{
    for (CWnd* pChild = GetWindow(GW_CHILD); pChild != NULL; pChild = pChild->GetWindow(GW_HWNDNEXT))
        pChild->EnableWindow(bEnable);
    CPropertyPage::OnEnable(bEnable);
}
