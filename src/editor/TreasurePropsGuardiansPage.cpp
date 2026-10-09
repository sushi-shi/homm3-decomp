// TreasurePropsGuardiansPage.cpp - the guardians page of the treasure
// property sheets (h3maped 0x4cba60..0x4cbd22; Loki h3maped object 102).
// The army dialog takes the frame control's place; unchecking customize
// disables it. The data exchange folds with another page's identical one.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/ArmyDlg.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/TreasurePropsGuardiansPage.h"

VA(0x004cba7c, 0xd6)
TTreasurePropsGuardiansPage::TTreasurePropsGuardiansPage(TTreasure* pTreasure, EGameVersion mapVersion,
                                                         UINT nIDHelp)
    : CPropertyPage(TTreasurePropsGuardiansPage::IDD),
      _m_pTreasure(pTreasure),
      _m_pArmyDlg(NULL),
      _m_bModified(false)
{
    _m_bCustomize = FALSE;
    m_psp.pszTitle = m_strCaption = kGuardiansPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
    m_nIDHelp = nIDHelp;
    _m_pArmyDlg = new TArmyDlg(_m_pTreasure->getGuardians(), mapVersion, false);
    if (!_m_pArmyDlg)
        throw TAllocationFailure();
}

VA_COMPGEN(0x004cbb52, 0x1c, SCALAR_DELETING_DTOR, TTreasurePropsGuardiansPage)

VA(0x004cbb6e, 0x44)
TTreasurePropsGuardiansPage::~TTreasurePropsGuardiansPage()
{
    delete _m_pArmyDlg;
}

VA(0x004cbbb2, 0xc)
const TArmy& TTreasurePropsGuardiansPage::getGuardians() const
{
    return _m_pArmyDlg->getArmy();
}

void TTreasurePropsGuardiansPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Check(pDX, IDC_CUSTOMIZE_CHECK, _m_bCustomize);
}

VA(0x004cbbbe, 0x6)
BEGIN_MESSAGE_MAP(TTreasurePropsGuardiansPage, CPropertyPage)
    ON_BN_CLICKED(IDC_CUSTOMIZE_CHECK, OnCustomizeCheck)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

VA(0x004cbbc4, 0x2d)
int TTreasurePropsGuardiansPage::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertyPage::OnCreate(lpCreateStruct) == -1 || !_m_pArmyDlg->Create(TArmyDlg::IDD, this))
        return -1;
    return 0;
}

VA(0x004cbbf1, 0x17)
void TTreasurePropsGuardiansPage::OnDestroy()
{
    _m_pArmyDlg->DestroyWindow();
    CPropertyPage::OnDestroy();
}

VA(0x004cbc08, 0xa9)
BOOL TTreasurePropsGuardiansPage::OnInitDialog()
{
    GetDlgItem(IDC_CUSTOMIZE_CHECK)->SetWindowText(kCustomizeCheckStr);
    _m_bModified = false;
    _m_bCustomGuardians = _m_pTreasure->getBCustomGuardians();
    _m_bCustomize = _m_bCustomGuardians ? TRUE : FALSE;
    CPropertyPage::OnInitDialog();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pArmyDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    _m_pArmyDlg->EnableWindow(_m_bCustomize);
    return TRUE;
}

VA(0x004cbcb1, 0x54)
void TTreasurePropsGuardiansPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_pArmyDlg->OnOK();
    _m_bCustomGuardians = _m_bCustomize != FALSE;
    _m_bModified = _m_bModified || _m_bCustomGuardians != _m_pTreasure->getBCustomGuardians()
                   || _m_pArmyDlg->wasModified();
}

VA(0x004cbd05, 0x1d)
void TTreasurePropsGuardiansPage::OnCustomizeCheck()
{
    UpdateData(TRUE);
    _m_pArmyDlg->EnableWindow(_m_bCustomize);
}
