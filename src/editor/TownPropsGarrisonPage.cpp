// TownPropsGarrisonPage.cpp - the garrison page of the town property sheet
// (h3maped 0x4c49a7..0x4c5068; Loki h3maped object 92). The army dialog
// takes the frame control's place; unchecking customize disables it.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/ArmyDlg.h"
#include "editor/GameMap.h"
#include "editor/MapEditorText.h"
#include "editor/Town.h"
#include "editor/TownPropsGarrisonPage.h"

VA(0x004c4ba8, 0x12f)
TTownPropsGarrisonPage::TTownPropsGarrisonPage(const TGameMap& oldMap, TGameMap& newMap, TMapObjectRef townRef)
    : CPropertyPage(TTownPropsGarrisonPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_townRef(townRef),
      _m_pOldTown(_getOldTown()),
      _m_bModified(false),
      _m_pArmyDlg(NULL)
{
    _m_formation = -1;
    m_psp.pszTitle = m_strCaption = kGarrisonPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
    TTown* pNewTown = _getNewTown();
    _m_pArmyDlg = new TArmyDlg(pNewTown->getGarrison(), newMap.getVersion(), pNewTown->getType() == RANDOM_TOWN);
    if (!_m_pArmyDlg)
        throw TAllocationFailure();
}

VA_COMPGEN(0x004c4cd7, 0x1c, SCALAR_DELETING_DTOR, TTownPropsGarrisonPage)

VA(0x004c4cf3, 0x54)
TTownPropsGarrisonPage::~TTownPropsGarrisonPage()
{
    delete _m_pArmyDlg;
}

VA(0x004c4d47, 0x42)
const TTown* TTownPropsGarrisonPage::_getOldTown() const
{
    const TGameMap::TLayer& layer = _m_oldMap.getLayer(_m_townRef.getBSecondLayer());
    return dynamic_cast<const TTown*>(layer.getPObject(_m_townRef.getObjectID()));
}

VA(0x004c4d89, 0x42)
TTown* TTownPropsGarrisonPage::_getNewTown()
{
    TGameMap::TLayer& layer = _m_newMap.getLayer(_m_townRef.getBSecondLayer());
    return dynamic_cast<TTown*>(layer.getPObject(_m_townRef.getObjectID()));
}

VA(0x004c4dcb, 0x31)
void TTownPropsGarrisonPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_CUSTOMIZE_CHECK, _m_customizeCheck);
    DDX_Radio(pDX, IDC_SPREAD_RADIO, _m_formation);
}

VA(0x004c4dfc, 0x6)
BEGIN_MESSAGE_MAP(TTownPropsGarrisonPage, CPropertyPage)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_CUSTOMIZE_CHECK, OnCustomizeCheck)
END_MESSAGE_MAP()

VA(0x004c4e02, 0x2d)
int TTownPropsGarrisonPage::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertyPage::OnCreate(lpCreateStruct) == -1 || !_m_pArmyDlg->Create(TArmyDlg::IDD, this))
        return -1;
    return 0;
}

// h3maped 0x4c4e2f..0x4c4e46: the census ends this body before its `ret`
// at a weak seed (0x4c4e45).
void TTownPropsGarrisonPage::OnDestroy()
{
    _m_pArmyDlg->DestroyWindow();
    CPropertyPage::OnDestroy();
}

VA(0x004c4e46, 0x122)
BOOL TTownPropsGarrisonPage::OnInitDialog()
{
    GetDlgItem(IDC_CUSTOMIZE_CHECK)->SetWindowText(kCustomizeCheckStr);
    GetDlgItem(IDC_FORMATION_STATIC)->SetWindowText(STownPropsGarrisonPageText::kFormationStaticStr);
    GetDlgItem(IDC_SPREAD_RADIO)->SetWindowText(STownPropsGarrisonPageText::kSpreadRadioStr);
    GetDlgItem(IDC_GROUPED_RADIO)->SetWindowText(STownPropsGarrisonPageText::kGroupedRadioStr);
    _m_bModified = false;
    const TTown* pNewTown = _getNewTown();
    _m_bCustomGarrison = pNewTown->getBCustomGarrison();
    _m_bGroupedFormation = pNewTown->getBGroupedFormation();
    _m_formation = _m_bGroupedFormation ? 1 : 0;
    CPropertyPage::OnInitDialog();
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pArmyDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    if (_m_bCustomGarrison)
        _m_customizeCheck.SetCheck(1);
    else
        _m_pArmyDlg->EnableWindow(FALSE);
    return TRUE;
}

VA(0x004c4f68, 0xab)
void TTownPropsGarrisonPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_pArmyDlg->OnOK();
    TTown* pNewTown = _getNewTown();
    pNewTown->setBCustomGarrison(_m_customizeCheck.GetCheck() != 0);
    pNewTown->setGarrison(_m_pArmyDlg->getArmy());
    pNewTown->setBGroupedFormation(_m_formation != 0);
    _m_bModified = _m_bModified || _m_pOldTown->getBCustomGarrison() != pNewTown->getBCustomGarrison()
                   || _m_pArmyDlg->wasModified()
                   || _m_pOldTown->getBGroupedFormation() != pNewTown->getBGroupedFormation();
}

VA(0x004c5013, 0x55)
void TTownPropsGarrisonPage::OnCustomizeCheck()
{
    if (_m_customizeCheck.GetCheck()) {
        if (!_m_pArmyDlg->IsWindowEnabled())
            _m_pArmyDlg->EnableWindow(TRUE);
    } else {
        if (_m_pArmyDlg->IsWindowEnabled())
            _m_pArmyDlg->EnableWindow(FALSE);
    }
}
