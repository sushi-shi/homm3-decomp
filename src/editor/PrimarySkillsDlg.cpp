// PrimarySkillsDlg.cpp - the four primary skill edits (h3maped
// 0x494709..0x494c0b; GOG only). Leaving an edit clamps its value.
#include "editor/stdafx.h"

#include <stdio.h>

#include "exceptions.h"
#include "va.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"
#include "editor/PrimarySkillsDlg.h"

DATA(0x0053ff48)
static const int g_aakSkillControlIDs[kNumPrimarySkills][2] = {
    { IDC_ATTACK_EDIT, IDC_ATTACK_SPIN },
    { IDC_DEFENSE_EDIT, IDC_DEFENSE_SPIN },
    { IDC_SPELL_POWER_EDIT, IDC_SPELL_POWER_SPIN },
    { IDC_KNOWLEDGE_EDIT, IDC_KNOWLEDGE_SPIN }
};

VA(0x00494725, 0x8)
void TPrimarySkillsDlg::OnKillFocusAttackEdit()
{
    _onKillFocusSkillEdit(0);
}

VA(0x0049472d, 0x8)
void TPrimarySkillsDlg::OnKillFocusDefenseEdit()
{
    _onKillFocusSkillEdit(1);
}

VA(0x00494735, 0x8)
void TPrimarySkillsDlg::OnKillFocusSpellPowerEdit()
{
    _onKillFocusSkillEdit(2);
}

VA(0x0049473d, 0x8)
void TPrimarySkillsDlg::OnKillFocusKnowledgeEdit()
{
    _onKillFocusSkillEdit(3);
}

VA(0x00494745, 0xa1)
TPrimarySkillsDlg::TPrimarySkillsDlg(CWnd* pParent)
    : CDialog(TPrimarySkillsDlg::IDD, pParent),
      _m_skills(0)
{
    if (!Create(TPrimarySkillsDlg::IDD, pParent))
        throw TRuntimeError();
}

VA_COMPGEN(0x004947e6, 0x1c, SCALAR_DELETING_DTOR, TPrimarySkillsDlg)
VA_COMPGEN(0x00494802, 0x41, CLASS_CTOR, _TSkillControls)
VA_COMPGEN(0x00494843, 0x35, IMPLICIT_DTOR, _TSkillControls)
VA_COMPGEN(0x00494878, 0x49, IMPLICIT_DTOR, TPrimarySkillsDlg)

VA(0x004948c1, 0x5c)
void TPrimarySkillsDlg::setPrimarySkills(const TArray<int, kNumPrimarySkills>& skills)
{
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++)
        _m_aSkillControls[skill].m_edit.SetWindowText(TFormattedString("%d", skills[skill]));
}

VA(0x0049491d, 0xdc)
TArray<int, kNumPrimarySkills> TPrimarySkillsDlg::getPrimarySkills() const
{
    TArray<int, kNumPrimarySkills> skills(0);
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++) {
        CString text;
        _m_aSkillControls[skill].m_edit.GetWindowText(text);
        int value = 0;
        sscanf(text, "%d", &value);
        skills[skill] = clamp(0, value, int(s_kMaxSkill));
    }
    return skills;
}

VA(0x004949f9, 0xaa)
void TPrimarySkillsDlg::_onKillFocusSkillEdit(unsigned int skill)
{
    CEdit& edit = _m_aSkillControls[skill].m_edit;
    CString text;
    edit.GetWindowText(text);
    int value = 0;
    sscanf(text, "%d", &value);
    if (value < 0 || value > s_kMaxSkill) {
        value = clamp(0, value, int(s_kMaxSkill));
        text.Format("%d", value);
        edit.SetWindowText(text);
    }
}

VA(0x00494aa3, 0x3c)
void TPrimarySkillsDlg::DoDataExchange(CDataExchange* pDX)
{
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++) {
        DDX_Control(pDX, g_aakSkillControlIDs[skill][0], _m_aSkillControls[skill].m_edit);
        DDX_Control(pDX, g_aakSkillControlIDs[skill][1], _m_aSkillControls[skill].m_spin);
    }
}

VA(0x00494adf, 0x6)
BEGIN_MESSAGE_MAP(TPrimarySkillsDlg, CDialog)
    ON_WM_ENABLE()
    ON_EN_KILLFOCUS(IDC_ATTACK_EDIT, OnKillFocusAttackEdit)
    ON_EN_KILLFOCUS(IDC_DEFENSE_EDIT, OnKillFocusDefenseEdit)
    ON_EN_KILLFOCUS(IDC_SPELL_POWER_EDIT, OnKillFocusSpellPowerEdit)
    ON_EN_KILLFOCUS(IDC_KNOWLEDGE_EDIT, OnKillFocusKnowledgeEdit)
END_MESSAGE_MAP()

VA(0x00494ae5, 0xf2)
BOOL TPrimarySkillsDlg::OnInitDialog()
{
    GetDlgItem(IDC_ATTACK_STATIC)->SetWindowText(SPrimarySkillsDlgText::kAttackSkillStaticStr);
    GetDlgItem(IDC_DEFENSE_STATIC)->SetWindowText(SPrimarySkillsDlgText::kDefenseSkillStaticStr);
    GetDlgItem(IDC_SPELL_POWER_STATIC)->SetWindowText(SPrimarySkillsDlgText::kSpellPowerStaticStr);
    GetDlgItem(IDC_KNOWLEDGE_STATIC)->SetWindowText(SPrimarySkillsDlgText::kKnowledgeStaticStr);
    CDialog::OnInitDialog();
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++) {
        _m_aSkillControls[skill].m_edit.LimitText(TDigits<s_kMaxSkill>::getDigits());
        _m_aSkillControls[skill].m_spin.SetRange(0, s_kMaxSkill);
    }
    setPrimarySkills(TArray<int, kNumPrimarySkills>(0));
    return TRUE;
}

VA(0x00494bd7, 0x34)
void TPrimarySkillsDlg::OnEnable(BOOL bEnable)
{
    CDialog::OnEnable(bEnable);
    for (unsigned int skill = 0; skill < kNumPrimarySkills; skill++) {
        _m_aSkillControls[skill].m_edit.EnableWindow(bEnable);
        _m_aSkillControls[skill].m_spin.EnableWindow(bEnable);
    }
}
