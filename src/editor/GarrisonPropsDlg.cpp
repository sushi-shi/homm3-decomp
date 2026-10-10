// GarrisonPropsDlg.cpp - the garrison's properties dialog (h3maped
// 0x43d588..0x43dde6; GOG only). DoModal stores the owner, the army and
// the removable flag when OK changed any of them.
#include "editor/stdafx.h"

#include "exceptions.h"
#include "va.h"
#include "editor/ArmyDlg.h"
#include "editor/FormattedString.h"
#include "editor/GarrisonPropsDlg.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/Player.h"

VA(0x0043d75c, 0xd6)
TGarrisonPropsDlg::TGarrisonPropsDlg(CWnd* pParent, TGarrison* pGarrison, EGameVersion version)
    : CDialog(TGarrisonPropsDlg::IDD, pParent),
      _m_pGarrison(pGarrison),
      _m_version(version),
      _m_pArmyDlg(NULL),
      _m_bModified(false)
{
    _m_owner = -1;
    _m_pArmyDlg = new TArmyDlg(_m_pGarrison->getArmy(), version, false);
    if (!_m_pArmyDlg)
        throw TAllocationFailure();
}

VA_COMPGEN(0x0043d832, 0x1c, SCALAR_DELETING_DTOR, TGarrisonPropsDlg)

VA(0x0043d84e, 0x51)
TGarrisonPropsDlg::~TGarrisonPropsDlg()
{
    delete _m_pArmyDlg;
}

VA(0x0043d89f, 0x2e)
void TGarrisonPropsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_REMOVABLE_CHECK, _m_removableCheck);
    DDX_Radio(pDX, IDC_NONE_RADIO, _m_owner);
}

VA(0x0043d8cd, 0x6)
BEGIN_MESSAGE_MAP(TGarrisonPropsDlg, CDialog)
    ON_WM_CREATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

VA(0x0043d8d3, 0xb3)
int TGarrisonPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1 || !_m_pArmyDlg->Create(TArmyDlg::IDD, this))
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pGarrison->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}

VA(0x0043d986, 0x17)
void TGarrisonPropsDlg::OnDestroy()
{
    _m_pArmyDlg->DestroyWindow();
    CDialog::OnDestroy();
}

VA(0x0043d99d, 0x54)
int TGarrisonPropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (result == IDOK && _m_bModified) {
        _m_pGarrison->setOwner(TPlayer(_m_owner - 1));
        _m_pGarrison->setArmy(_m_pArmyDlg->getArmy());
        _m_pGarrison->setBRemovableUnits(_m_bRemovableUnits);
    }
    return result;
}

VA(0x0043d9f1, 0x381)
BOOL TGarrisonPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_OWNER_STATIC)->SetWindowText(SGarrisonPropertiesDlgText::kOwnerStaticStr);
    GetDlgItem(IDC_NONE_RADIO)->SetWindowText(SGarrisonPropertiesDlgText::kNoneRadioStr);
    GetDlgItem(IDC_OWNER_PLAYER1_RADIO)->SetWindowText(
        TFormattedString(SGarrisonPropertiesDlgText::kPlayerRadioFmtStr, 1, akPlayerTraits[0].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER2_RADIO)->SetWindowText(
        TFormattedString(SGarrisonPropertiesDlgText::kPlayerRadioFmtStr, 2, akPlayerTraits[1].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER3_RADIO)->SetWindowText(
        TFormattedString(SGarrisonPropertiesDlgText::kPlayerRadioFmtStr, 3, akPlayerTraits[2].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER4_RADIO)->SetWindowText(
        TFormattedString(SGarrisonPropertiesDlgText::kPlayerRadioFmtStr, 4, akPlayerTraits[3].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER5_RADIO)->SetWindowText(
        TFormattedString(SGarrisonPropertiesDlgText::kPlayerRadioFmtStr, 5, akPlayerTraits[4].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER6_RADIO)->SetWindowText(
        TFormattedString(SGarrisonPropertiesDlgText::kPlayerRadioFmtStr, 6, akPlayerTraits[5].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER7_RADIO)->SetWindowText(
        TFormattedString(SGarrisonPropertiesDlgText::kPlayerRadioFmtStr, 7, akPlayerTraits[6].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER8_RADIO)->SetWindowText(
        TFormattedString(SGarrisonPropertiesDlgText::kPlayerRadioFmtStr, 8, akPlayerTraits[7].m_pColorName));
    GetDlgItem(IDC_CREATURES_STATIC)->SetWindowText(SGarrisonPropertiesDlgText::kCreaturesStaticStr);
    GetDlgItem(IDC_REMOVABLE_CHECK)->SetWindowText(SGarrisonPropertiesDlgText::kRemovableCheckStr);
    _m_bModified = false;
    _m_owner = _m_pGarrison->getOwner() + 1;
    _m_bRemovableUnits = _m_pGarrison->getBRemovableUnits();
    CDialog::OnInitDialog();
    _m_removableCheck.SetCheck(_m_bRemovableUnits);
    if (_m_version < GAME_VERSION_AB)
        _m_removableCheck.EnableWindow(FALSE);
    CWnd* pFrame = GetDlgItem(IDC_FRAME);
    CRect rect;
    pFrame->GetWindowRect(&rect);
    ScreenToClient(&rect);
    _m_pArmyDlg->SetWindowPos(pFrame, rect.left, rect.top, 0, 0, SWP_NOSIZE);
    pFrame->DestroyWindow();
    return TRUE;
}

VA(0x0043dd72, 0x74)
void TGarrisonPropsDlg::OnOK()
{
    CDialog::OnOK();
    _m_bRemovableUnits = _m_removableCheck.GetCheck() != 0;
    _m_pArmyDlg->OnOK();
    _m_bModified = _m_bModified || _m_owner - 1 != _m_pGarrison->getOwner()
                   || _m_bRemovableUnits != _m_pGarrison->getBRemovableUnits()
                   || _m_pArmyDlg->wasModified();
}
