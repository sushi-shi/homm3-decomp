// MapSpecsSecSkillsPage.cpp - the secondary skills page of the map
// specifications sheet (h3maped 0x475213..0x475849; GOG only). Before
// Shadow of Death every skill is allowed and the list is read-only.
// Clearing a skill that would leave fewer than four allowed allows the
// next cleared one in the list.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/GameMap.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsSecSkillsPage.h"

VA(0x0047549b, 0x92)
TMapSpecsSecSkillsPage::TMapSpecsSecSkillsPage(const TGameMap& oldMap, TGameMap& newMap)
    : CPropertyPage(TMapSpecsSecSkillsPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = SMapSpecsSecSkillsPageText::kCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x0047552d, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsSecSkillsPage)

VA(0x00475549, 0x3e)
TMapSpecsSecSkillsPage::~TMapSpecsSecSkillsPage()
{
}

VA(0x00475587, 0x18)
void TMapSpecsSecSkillsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_AVAILABLE_SEC_SKILLS_LIST, _m_secSkillsList);
}

VA(0x0047559f, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsSecSkillsPage, CPropertyPage)
    ON_CLBN_CHKCHANGE(IDC_AVAILABLE_SEC_SKILLS_LIST, OnCheckChangeSecSkillsList)
END_MESSAGE_MAP()

VA(0x004755a5, 0x137)
BOOL TMapSpecsSecSkillsPage::OnInitDialog()
{
    GetDlgItem(IDC_AVAILABLE_SEC_SKILLS_STATIC)->SetWindowText(SMapSpecsSecSkillsPageText::kAvailableSecSkillsStaticStr);
    _m_bModified = false;
    _m_disabledSkills = _m_newMap.getDisabledSkills();
    CPropertyPage::OnInitDialog();
    CRect rect;
    _m_secSkillsList.GetClientRect(&rect);
    _m_secSkillsList.SetColumnWidth(rect.Width() / 2);
    for (unsigned int skill = 0; skill < kNumSecSkills; skill++) {
        int index = _m_secSkillsList.AddString(THero::s_akSecondarySkillTraits[skill].m_name);
        _m_secSkillsList.SetItemData(index, skill);
    }
    int count = _m_secSkillsList.GetCount();
    int index;
    if (_m_newMap.getVersion() >= GAME_VERSION_SOD) {
        for (index = 0; index < count; index++) {
            if (!_m_disabledSkills[_m_secSkillsList.GetItemData(index)])
                _m_secSkillsList.SetCheck(index, 1);
        }
    } else {
        for (index = 0; index < count; index++) {
            _m_secSkillsList.SetCheck(index, 1);
            _m_secSkillsList.Enable(index, FALSE);
        }
    }
    return TRUE;
}

VA(0x004756dc, 0x54)
void TMapSpecsSecSkillsPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_newMap.setDisabledSkills(_m_disabledSkills);
    _m_bModified = _m_bModified || _m_newMap.getDisabledSkills() != _m_oldMap.getDisabledSkills();
}

VA(0x00475730, 0x10c)
void TMapSpecsSecSkillsPage::OnCheckChangeSecSkillsList()
{
    int curSel = _m_secSkillsList.GetCurSel();
    unsigned int skill = _m_secSkillsList.GetItemData(curSel);
    if (_m_secSkillsList.GetCheck(curSel)) {
        _m_disabledSkills.set(skill, false);
    } else if (!_m_disabledSkills[skill]) {
        _m_disabledSkills.set(skill, true);
        if ((~_m_disabledSkills).count() < s_kMinNumAllowedSkills) {
            int index = curSel;
            do {
                if (++index >= _m_secSkillsList.GetCount())
                    index = 0;
            } while (_m_secSkillsList.GetCheck(index));
            unsigned int otherSkill = _m_secSkillsList.GetItemData(index);
            _m_secSkillsList.SetCheck(index, 1);
            _m_disabledSkills.set(otherSkill, false);
        }
    }
}
