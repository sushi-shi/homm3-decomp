// HeroPropsCreaturesPage.cpp - the creatures page of the hero property
// sheets (h3maped 0x44f06d..0x44f548; Loki h3maped object 17). The army
// dialog takes the frame control's place; unchecking customize disables
// it.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Hero.h"
#include "editor/HeroPropsCreaturesPage.h"
#include "editor/MapEditorText.h"

VA(0x0044f089, 0x11c)
THeroPropsCreaturesPage::THeroPropsCreaturesPage(TParentSheet* pParentSheet, const THero* pOldHero,
                                                 THero* pNewHero, EGameVersion mapVersion, bool bRandomCreatures)
    : CPropertyPage(THeroPropsCreaturesPage::IDD),
      _m_pParentSheet(pParentSheet),
      _m_pOldHero(pOldHero),
      _m_pNewHero(pNewHero),
      _m_bModified(false),
      _m_pArmyDlg(NULL)
{
    _m_formation = -1;
    m_psp.pszTitle = m_strCaption = kCreaturesPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
    _m_pArmyDlg = new TArmyDlg(this, _m_pNewHero->getArmy(), mapVersion, bRandomCreatures);
    if (!_m_pArmyDlg)
        throw TAllocationFailure();
}

VA_COMPGEN(0x0044f1a5, 0x1c, SCALAR_DELETING_DTOR, THeroPropsCreaturesPage)

VA(0x0044f1c1, 0x5e)
THeroPropsCreaturesPage::~THeroPropsCreaturesPage()
{
    delete _m_pArmyDlg;
}

VA(0x0044f21f, 0x42)
void THeroPropsCreaturesPage::onNumOccupiedStacksChanged(unsigned int newNum, unsigned int oldNum)
{
    if (_m_customizeCheck.GetCheck()) {
        if (newNum == 0) {
            if (oldNum > 0)
                _m_pParentSheet->onDisableOK();
        } else if (oldNum == 0)
            _m_pParentSheet->onEnableOK();
    }
}

VA(0x0044f261, 0x31)
void THeroPropsCreaturesPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CUSTOMIZE_CHECK, _m_customizeCheck);
    DDX_Radio(pDX, IDC_SPREAD_RADIO, _m_formation);
}

VA(0x0044f292, 0x6)
BEGIN_MESSAGE_MAP(THeroPropsCreaturesPage, CPropertyPage)
    ON_BN_CLICKED(IDC_CUSTOMIZE_CHECK, OnCustomizeCheck)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

VA(0x0044f298, 0x2d)
int THeroPropsCreaturesPage::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertyPage::OnCreate(lpCreateStruct) == -1 || !_m_pArmyDlg->Create(TArmyDlg::IDD, this))
        return -1;
    return 0;
}

VA(0x0044f2c5, 0x17)
void THeroPropsCreaturesPage::OnDestroy()
{
    _m_pArmyDlg->DestroyWindow();
    CPropertyPage::OnDestroy();
}

VA(0x0044f2dc, 0x112)
BOOL THeroPropsCreaturesPage::OnInitDialog()
{
    GetDlgItem(IDC_CUSTOMIZE_CHECK)->SetWindowText(kCustomizeCheckStr);
    GetDlgItem(IDC_FORMATION_STATIC)->SetWindowText(SHeroPropsCreaturesPageText::kFormationStaticStr);
    GetDlgItem(IDC_SPREAD_RADIO)->SetWindowText(SHeroPropsCreaturesPageText::kSpreadRadioStr);
    GetDlgItem(IDC_GROUPED_RADIO)->SetWindowText(SHeroPropsCreaturesPageText::kGroupedRadioStr);
    _m_bModified = false;
    _m_formation = _m_pNewHero->getBGroupedFormation();
    CPropertyPage::OnInitDialog();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pArmyDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    bool bCustomArmy = _m_pNewHero->getBCustomArmy();
    if (!bCustomArmy)
        _m_pArmyDlg->EnableWindow(FALSE);
    _m_customizeCheck.SetCheck(bCustomArmy);
    return TRUE;
}

VA(0x0044f3ee, 0xd6)
void THeroPropsCreaturesPage::OnOK()
{
    _m_pArmyDlg->OnOK();
    CPropertyPage::OnOK();
    _m_pNewHero->setBCustomArmy(_m_customizeCheck.GetCheck() != 0);
    _m_pNewHero->setArmy(_m_pArmyDlg->getArmy());
    _m_pNewHero->setBGroupedFormation(_m_formation != 0);
    _m_bModified = _m_bModified || _m_pOldHero->getBCustomArmy() != _m_pNewHero->getBCustomArmy()
                   || !(_m_pNewHero->getArmy() == _m_pOldHero->getArmy())
                   || _m_pNewHero->getBGroupedFormation() != _m_pOldHero->getBGroupedFormation();
}

VA(0x0044f4c4, 0x84)
void THeroPropsCreaturesPage::OnCustomizeCheck()
{
    if (!_m_customizeCheck.GetCheck()) {
        if (_m_pArmyDlg->IsWindowEnabled()) {
            if (_m_pArmyDlg->getNumOccupiedStacks() == 0)
                _m_pParentSheet->onEnableOK();
            _m_pArmyDlg->EnableWindow(FALSE);
        }
    } else if (!_m_pArmyDlg->IsWindowEnabled()) {
        _m_pArmyDlg->EnableWindow(TRUE);
        if (_m_pArmyDlg->getNumOccupiedStacks() == 0)
            _m_pParentSheet->onDisableOK();
    }
}
