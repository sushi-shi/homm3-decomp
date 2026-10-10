// ScholarPropsDlg.cpp - the scholar's reward dialog (h3maped
// 0x4b4bae..0x4b5361; GOG only). Only the chosen kind's combo is enabled;
// DoModal stores the reward type and all three choices on OK. The implicit
// destructor is MapView's copy (0x481cd9).
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/ScholarPropsDlg.h"

VA(0x004b4d82, 0x8b)
TScholarPropsDlg::TScholarPropsDlg(CWnd* pParent, TScholar* pScholar)
    : CDialog(TScholarPropsDlg::IDD, pParent),
      _m_pScholar(pScholar),
      _m_bModified(false)
{
    _m_rewardChoice = -1;
}

VA_COMPGEN(0x004b4e0d, 0x1c, SCALAR_DELETING_DTOR, TScholarPropsDlg)

VA(0x004b4e29, 0x52)
void TScholarPropsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_SPELL_REWARD_COMBO, _m_spellCombo);
    DDX_Control(pDX, IDC_SEC_SKILL_REWARD_COMBO, _m_secondarySkillCombo);
    DDX_Control(pDX, IDC_PRI_SKILL_REWARD_COMBO, _m_primarySkillCombo);
    DDX_Radio(pDX, IDC_RANDOM_REWARD_RADIO, _m_rewardChoice);
}

VA(0x004b4e7b, 0x6)
BEGIN_MESSAGE_MAP(TScholarPropsDlg, CDialog)
    ON_BN_CLICKED(IDC_RANDOM_REWARD_RADIO, OnRewardRadio)
    ON_BN_CLICKED(IDC_PRI_SKILL_REWARD_RADIO, OnRewardRadio)
    ON_BN_CLICKED(IDC_SEC_SKILL_REWARD_RADIO, OnRewardRadio)
    ON_BN_CLICKED(IDC_SPELL_REWARD_RADIO, OnRewardRadio)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x004b4e81, 0x99)
int TScholarPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pScholar->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}

VA(0x004b4f1a, 0x5b)
int TScholarPropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (result == IDOK) {
        _m_pScholar->setRewardType(TScholar::TRewardType(_m_rewardChoice - 1));
        _m_pScholar->setPrimarySkill(_m_primarySkill);
        _m_pScholar->setSecondarySkill(_m_secondarySkill);
        _m_pScholar->setSpell(_m_spell);
    }
    return result;
}

VA(0x004b4f75, 0x2c6)
BOOL TScholarPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_REWARD_GROUP)->SetWindowText(SScholarPropsDlgText::kRewardStaticStr);
    GetDlgItem(IDC_RANDOM_REWARD_RADIO)->SetWindowText(SScholarPropsDlgText::kRandomRewardRadioStr);
    GetDlgItem(IDC_PRI_SKILL_REWARD_RADIO)->SetWindowText(SScholarPropsDlgText::kPriSkillRewardRadioStr);
    GetDlgItem(IDC_SEC_SKILL_REWARD_RADIO)->SetWindowText(SScholarPropsDlgText::kSecSkillRewardRadioStr);
    GetDlgItem(IDC_SPELL_REWARD_RADIO)->SetWindowText(SScholarPropsDlgText::kSpellRewardRadioStr);
    _m_bModified = false;
    _m_rewardChoice = _m_pScholar->getRewardType() + 1;
    _m_primarySkill = _m_pScholar->getPrimarySkill();
    _m_secondarySkill = _m_pScholar->getSecondarySkill();
    _m_spell = _m_pScholar->getSpell();
    CDialog::OnInitDialog();
    int index;
    for (unsigned int primarySkill = 0; primarySkill < kNumPrimarySkills; primarySkill++)
        _m_primarySkillCombo.SetItemData(
            _m_primarySkillCombo.AddString(THero::s_akPrimarySkillTraits[primarySkill].m_name), primarySkill);
    index = 0;
    while (_m_primarySkillCombo.GetItemData(index) != _m_primarySkill)
        index++;
    _m_primarySkillCombo.SetCurSel(index);
    _m_primarySkillCombo.EnableWindow(FALSE);
    for (unsigned int secondarySkill = 0; secondarySkill < kNumSecSkills; secondarySkill++)
        _m_secondarySkillCombo.SetItemData(
            _m_secondarySkillCombo.AddString(THero::s_akSecondarySkillTraits[secondarySkill].m_name), secondarySkill);
    index = 0;
    while (_m_secondarySkillCombo.GetItemData(index) != _m_secondarySkill)
        index++;
    _m_secondarySkillCombo.SetCurSel(index);
    _m_secondarySkillCombo.EnableWindow(FALSE);
    for (unsigned int spell = 0; spell < kNumSpells; spell++) {
        if (!(akSpellTraits[spell].m_flags & 0x2000) && akSpellTraits[spell].m_schoolBits != 0)
            _m_spellCombo.SetItemData(_m_spellCombo.AddString(akSpellTraits[spell].m_name), spell);
    }
    index = 0;
    while (_m_spellCombo.GetItemData(index) != _m_spell)
        index++;
    _m_spellCombo.SetCurSel(index);
    _m_spellCombo.EnableWindow(FALSE);
    switch (_m_rewardChoice - 1) {
    case TScholar::eRewardPrimarySkill:
        _m_primarySkillCombo.EnableWindow(TRUE);
        break;
    case TScholar::eRewardSecondarySkill:
        _m_secondarySkillCombo.EnableWindow(TRUE);
        break;
    case TScholar::eRewardSpell:
        _m_spellCombo.EnableWindow(TRUE);
        break;
    }
    return TRUE;
}

VA(0x004b523b, 0xb9)
void TScholarPropsDlg::OnOK()
{
    CDialog::OnOK();
    _m_primarySkill = TPrimarySkill(_m_primarySkillCombo.GetItemData(_m_primarySkillCombo.GetCurSel()));
    _m_secondarySkill = TSecondarySkill(_m_secondarySkillCombo.GetItemData(_m_secondarySkillCombo.GetCurSel()));
    _m_spell = ESpellId(_m_spellCombo.GetItemData(_m_spellCombo.GetCurSel()));
    _m_bModified = _m_bModified || _m_rewardChoice - 1 != _m_pScholar->getRewardType()
                   || _m_primarySkill != _m_pScholar->getPrimarySkill()
                   || _m_secondarySkill != _m_pScholar->getSecondarySkill()
                   || _m_spell != _m_pScholar->getSpell();
}

VA(0x004b52f4, 0x6d)
void TScholarPropsDlg::OnRewardRadio()
{
    switch (_m_rewardChoice - 1) {
    case TScholar::eRewardPrimarySkill:
        _m_primarySkillCombo.EnableWindow(FALSE);
        break;
    case TScholar::eRewardSecondarySkill:
        _m_secondarySkillCombo.EnableWindow(FALSE);
        break;
    case TScholar::eRewardSpell:
        _m_spellCombo.EnableWindow(FALSE);
        break;
    }
    UpdateData(TRUE);
    switch (_m_rewardChoice - 1) {
    case TScholar::eRewardPrimarySkill:
        _m_primarySkillCombo.EnableWindow(TRUE);
        break;
    case TScholar::eRewardSecondarySkill:
        _m_secondarySkillCombo.EnableWindow(TRUE);
        break;
    case TScholar::eRewardSpell:
        _m_spellCombo.EnableWindow(TRUE);
        break;
    }
}
