// HeroPropsSecSkillsPage.cpp - the secondary skills page of the hero
// property sheets (h3maped 0x45336c..0x45381f; Loki h3maped object 19).
// The skills dialog takes the frame control's place.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/HeroPropsSecSkillsPage.h"
#include "editor/MapEditorText.h"

VA(0x00453388, 0xcb)
THeroPropsSecSkillsPage::THeroPropsSecSkillsPage(const THero* pOldHero, THero* pNewHero)
    : CPropertyPage(THeroPropsSecSkillsPage::IDD),
      _m_pOldHero(pOldHero),
      _m_pNewHero(pNewHero),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kSecondarySkillsPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00453453, 0x1c, SCALAR_DELETING_DTOR, THeroPropsSecSkillsPage)

VA(0x0045346f, 0x6e)
THeroPropsSecSkillsPage::~THeroPropsSecSkillsPage()
{
}

VA(0x004534dd, 0x2f)
void THeroPropsSecSkillsPage::setDefaultSecondarySkills(const THeroPrototype::TSecondarySkills& skills)
{
    _m_defaultSkills = skills;
    if (!_m_bCustomSkills)
        _m_pSkillsDlg->setSecondarySkills(_m_defaultSkills);
}

VA(0x0045350c, 0x18)
void THeroPropsSecSkillsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CUSTOMIZE_CHECK, _m_customizeCheck);
}

VA(0x00453524, 0x6)
BEGIN_MESSAGE_MAP(THeroPropsSecSkillsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_CUSTOMIZE_CHECK, OnCustomizeCheck)
END_MESSAGE_MAP()

VA(0x0045352a, 0xc5)
void THeroPropsSecSkillsPage::OnOK()
{
    CPropertyPage::OnOK();
    if (_m_bCustomSkills)
        _m_customSkills = _m_pSkillsDlg->getSecondarySkills();
    _m_pNewHero->setBCustomSecondarySkills(_m_bCustomSkills);
    _m_pNewHero->setSecondarySkills(_m_customSkills);
    _m_bModified = _m_bModified
                   || _m_pOldHero->getBCustomSecondarySkills() != _m_pNewHero->getBCustomSecondarySkills()
                   || !(_m_pNewHero->getSecondarySkills() == _m_pOldHero->getSecondarySkills());
}

VA(0x004535ef, 0x17c)
BOOL THeroPropsSecSkillsPage::OnInitDialog()
{
    GetDlgItem(IDC_CUSTOMIZE_CHECK)->SetWindowText(kCustomizeCheckStr);
    _m_bModified = false;
    _m_bCustomSkills = _m_pNewHero->getBCustomSecondarySkills();
    _m_customSkills = _m_pNewHero->getSecondarySkills();
    CPropertyPage::OnInitDialog();
    _m_pSkillsDlg = std::auto_ptr<TSecondarySkillsDlg>(new TSecondarySkillsDlg(this));
    if (!_m_pSkillsDlg.get())
        throw TAllocationFailure();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pSkillsDlg->SetWindowPos(pFrame, rect.left, rect.top, rect.Width(), rect.Height(), 0);
    pFrame->DestroyWindow();
    _m_customizeCheck.SetCheck(_m_bCustomSkills);
    _m_pSkillsDlg->setSecondarySkills(_m_bCustomSkills ? _m_customSkills : _m_defaultSkills);
    if (!_m_bCustomSkills)
        _m_pSkillsDlg->EnableWindow(FALSE);
    return TRUE;
}

VA(0x0045376b, 0xb4)
void THeroPropsSecSkillsPage::OnCustomizeCheck()
{
    if (_m_customizeCheck.GetCheck()) {
        if (!_m_bCustomSkills) {
            _m_bCustomSkills = true;
            _m_pSkillsDlg->EnableWindow(TRUE);
            _m_pSkillsDlg->setSecondarySkills(_m_customSkills);
        }
    } else if (_m_bCustomSkills) {
        _m_bCustomSkills = false;
        _m_customSkills = _m_pSkillsDlg->getSecondarySkills();
        _m_pSkillsDlg->setSecondarySkills(_m_defaultSkills);
        _m_pSkillsDlg->EnableWindow(FALSE);
    }
}
