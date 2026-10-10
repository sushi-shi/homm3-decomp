// WitchHutPropsDlg.cpp - the witch hut's skills dialog (h3maped
// 0x4cd09b..0x4cd567; GOG only). OnOK stores changed skills in the hut;
// unchecking the last skill checks the next one. The implicit destructor
// is MapView's copy (0x482046).
#include "editor/stdafx.h"

#include "va.h"
#include "editor/FormattedString.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/WitchHutPropsDlg.h"

VA(0x004cd26f, 0x66)
TWitchHutPropsDlg::TWitchHutPropsDlg(CWnd* pParent, TWitchHut* pWitchHut, EGameVersion version)
    : CDialog(TWitchHutPropsDlg::IDD, pParent),
      _m_pWitchHut(pWitchHut),
      _m_version(version),
      _m_bModified(false)
{
}

VA_COMPGEN(0x004cd2d5, 0x1c, SCALAR_DELETING_DTOR, TWitchHutPropsDlg)

VA(0x004cd2f1, 0x15)
void TWitchHutPropsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_POTENTIAL_SKILLS_LIST, _m_skillsList);
}

VA(0x004cd306, 0x6)
BEGIN_MESSAGE_MAP(TWitchHutPropsDlg, CDialog)
    ON_WM_CREATE()
    ON_CLBN_CHKCHANGE(IDC_POTENTIAL_SKILLS_LIST, OnCheckChangeSkillsList)
END_MESSAGE_MAP()

VA(0x004cd30c, 0x8d)
int TWitchHutPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(TFormattedString(kObjectPropertiesCaptionFmtStr, _m_pWitchHut->getTypeName().c_str()));
    return 0;
}

VA(0x004cd399, 0xf1)
BOOL TWitchHutPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_POTENTIAL_SKILLS_STATIC)->SetWindowText(SWitchHutPropsDlgText::kPotentialSkillsStaticStr);
    _m_bModified = false;
    _m_abPotentialSkill = _m_pWitchHut->getPotentialSkills();
    CDialog::OnInitDialog();
    for (unsigned int skill = 0; skill < kNumSecSkills; skill++) {
        int index = _m_skillsList.AddString(THero::s_akSecondarySkillTraits[skill].m_name);
        _m_skillsList.SetItemData(index, skill);
        _m_skillsList.SetCheck(index, _m_abPotentialSkill[skill]);
        if (_m_version < GAME_VERSION_AB)
            _m_skillsList.Enable(index, FALSE);
    }
    return TRUE;
}

VA(0x004cd48a, 0x2e)
void TWitchHutPropsDlg::OnOK()
{
    CDialog::OnOK();
    if (_m_abPotentialSkill != _m_pWitchHut->getPotentialSkills()) {
        _m_pWitchHut->setPotentialSkills(_m_abPotentialSkill);
        _m_bModified = true;
    }
}

VA(0x004cd4b8, 0xaf)
void TWitchHutPropsDlg::OnCheckChangeSkillsList()
{
    int index = _m_skillsList.GetCaretIndex();
    unsigned int skill = _m_skillsList.GetItemData(index);
    _m_abPotentialSkill[skill] = _m_skillsList.GetCheck(index) != 0;
    if (_m_abPotentialSkill.none()) {
        if (++index >= _m_skillsList.GetCount())
            index = 0;
        _m_abPotentialSkill.set(_m_skillsList.GetItemData(index), true);
        _m_skillsList.SetCheck(index, 1);
    }
}
