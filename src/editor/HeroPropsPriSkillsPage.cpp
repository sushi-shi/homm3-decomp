// HeroPropsPriSkillsPage.cpp - the primary skills page of the hero
// property sheets (h3maped 0x452ce9..0x4531b4; GOG only). The skills
// dialog takes the frame control's place; the data exchange folds with
// the secondary skills page's.
#include "editor/stdafx.h"

#include <algorithm>

#include "va.h"
#include "editor/Hero.h"
#include "editor/HeroPropsPriSkillsPage.h"
#include "editor/MapEditorText.h"

VA(0x00452d05, 0xd4)
THeroPropsPriSkillsPage::THeroPropsPriSkillsPage(const THero* pOldHero, THero* pNewHero, EGameVersion mapVersion)
    : CPropertyPage(THeroPropsPriSkillsPage::IDD),
      _m_pOldHero(pOldHero),
      _m_pNewHero(pNewHero),
      _m_mapVersion(mapVersion),
      _m_bModified(false),
      _m_defaultSkills(0),
      _m_customSkills(0)
{
    m_psp.pszTitle = m_strCaption = SHeroPropsPriSkillsPageText::kCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00452dd9, 0x1c, SCALAR_DELETING_DTOR, THeroPropsPriSkillsPage)

VA(0x00452df5, 0x63)
THeroPropsPriSkillsPage::~THeroPropsPriSkillsPage()
{
}

VA(0x00452e58, 0x43)
void THeroPropsPriSkillsPage::setDefaultPrimarySkills(const TArray<int, kNumPrimarySkills>& skills)
{
    _m_defaultSkills = skills;
    if (!_m_bCustomSkills)
        _m_pSkillsDlg->setPrimarySkills(_m_defaultSkills);
}

void THeroPropsPriSkillsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CUSTOMIZE_CHECK, _m_customizeCheck);
}

VA(0x00452e9b, 0x6)
BEGIN_MESSAGE_MAP(THeroPropsPriSkillsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_CUSTOMIZE_CHECK, OnCustomizeCheck)
END_MESSAGE_MAP()

VA(0x00452ea1, 0x1a3)
BOOL THeroPropsPriSkillsPage::OnInitDialog()
{
    GetDlgItem(IDC_CUSTOMIZE_CHECK)->SetWindowText(SHeroPropsPriSkillsPageText::kCustomizeCheckStr);
    _m_bModified = false;
    _m_bCustomSkills = _m_pNewHero->getBCustomPrimarySkills();
    _m_customSkills = _m_pNewHero->getPrimarySkills();
    std::fill(_m_defaultSkills.begin(), _m_defaultSkills.end(), 0);
    CPropertyPage::OnInitDialog();
    _m_pSkillsDlg = std::auto_ptr<TPrimarySkillsDlg>(new TPrimarySkillsDlg(this));
    if (!_m_pSkillsDlg.get())
        throw TAllocationFailure();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pSkillsDlg->SetWindowPos(pFrame, rect.left, rect.top, rect.Width(), rect.Height(), 0);
    pFrame->DestroyWindow();
    _m_customizeCheck.SetCheck(_m_bCustomSkills);
    if (_m_mapVersion < GAME_VERSION_SOD)
        _m_customizeCheck.EnableWindow(FALSE);
    _m_pSkillsDlg->setPrimarySkills(_m_bCustomSkills ? _m_customSkills : _m_defaultSkills);
    if (!_m_bCustomSkills)
        _m_pSkillsDlg->EnableWindow(FALSE);
    return TRUE;
}

VA(0x00453044, 0xc9)
void THeroPropsPriSkillsPage::OnOK()
{
    CPropertyPage::OnOK();
    if (_m_bCustomSkills)
        _m_customSkills = _m_pSkillsDlg->getPrimarySkills();
    _m_pNewHero->setBCustomPrimarySkills(_m_bCustomSkills);
    _m_pNewHero->setPrimarySkills(_m_customSkills);
    _m_bModified = _m_bModified
                   || _m_pOldHero->getBCustomPrimarySkills() != _m_pNewHero->getBCustomPrimarySkills()
                   || !(_m_pNewHero->getPrimarySkills() == _m_pOldHero->getPrimarySkills());
}

VA(0x0045310d, 0xa7)
void THeroPropsPriSkillsPage::OnCustomizeCheck()
{
    if (_m_customizeCheck.GetCheck()) {
        if (!_m_bCustomSkills) {
            _m_bCustomSkills = true;
            _m_pSkillsDlg->EnableWindow(TRUE);
            _m_pSkillsDlg->setPrimarySkills(_m_customSkills);
        }
    } else if (_m_bCustomSkills) {
        _m_bCustomSkills = false;
        _m_customSkills = _m_pSkillsDlg->getPrimarySkills();
        _m_pSkillsDlg->setPrimarySkills(_m_defaultSkills);
        _m_pSkillsDlg->EnableWindow(FALSE);
    }
}
