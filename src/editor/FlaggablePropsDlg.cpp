// FlaggablePropsDlg.cpp - the owner dialog of flaggable objects (h3maped
// 0x41df9c..0x41e57b). DoModal stores the chosen owner on OK; a double
// click on any owner radio is an OK. The implicit destructor has no body
// here: /OPT:ICF folded it onto an identical one at 0x418b8c.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/FlaggablePropsDlg.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/Player.h"

VA(0x0041e170, 0x2c)
TFlaggablePropsDlg::TFlaggablePropsDlg(CWnd* pParent, TFlaggableObject* pFlaggable)
    : CDialog(TFlaggablePropsDlg::IDD, pParent),
      _m_pFlaggable(pFlaggable),
      _m_bModified(false)
{
    _m_owner = -1;
}

VA_COMPGEN(0x0041e19c, 0x1c, SCALAR_DELETING_DTOR, TFlaggablePropsDlg)

VA(0x0041e1b8, 0x15)
void TFlaggablePropsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Radio(pDX, IDC_OWNER_NONE_RADIO, _m_owner);
}

VA(0x0041e1cd, 0x6)
BEGIN_MESSAGE_MAP(TFlaggablePropsDlg, CDialog)
    ON_BN_DOUBLECLICKED(IDC_OWNER_NONE_RADIO, OnDoubleClickedOwnerRadio)
    ON_BN_DOUBLECLICKED(IDC_OWNER_PLAYER1_RADIO, OnDoubleClickedOwnerRadio)
    ON_BN_DOUBLECLICKED(IDC_OWNER_PLAYER2_RADIO, OnDoubleClickedOwnerRadio)
    ON_BN_DOUBLECLICKED(IDC_OWNER_PLAYER3_RADIO, OnDoubleClickedOwnerRadio)
    ON_BN_DOUBLECLICKED(IDC_OWNER_PLAYER4_RADIO, OnDoubleClickedOwnerRadio)
    ON_BN_DOUBLECLICKED(IDC_OWNER_PLAYER5_RADIO, OnDoubleClickedOwnerRadio)
    ON_BN_DOUBLECLICKED(IDC_OWNER_PLAYER6_RADIO, OnDoubleClickedOwnerRadio)
    ON_BN_DOUBLECLICKED(IDC_OWNER_PLAYER7_RADIO, OnDoubleClickedOwnerRadio)
    ON_BN_DOUBLECLICKED(IDC_OWNER_PLAYER8_RADIO, OnDoubleClickedOwnerRadio)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x0041e1d3, 0x96)
int TFlaggablePropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pFlaggable->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}

VA(0x0041e269, 0x19)
int TFlaggablePropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (result == IDOK)
        _m_pFlaggable->setOwner(TPlayer(_m_owner - 1));
    return result;
}

VA(0x0041e282, 0x2ce)
BOOL TFlaggablePropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_OWNER_STATIC)->SetWindowText(SFlaggablePropsDlgText::kOwnerStaticStr);
    GetDlgItem(IDC_OWNER_NONE_RADIO)->SetWindowText(SFlaggablePropsDlgText::kNoneRadioStr);
    GetDlgItem(IDC_OWNER_PLAYER1_RADIO)->SetWindowText(
        TFormattedString(SFlaggablePropsDlgText::kPlayerRadioFmtStr, 1, akPlayerTraits[0].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER2_RADIO)->SetWindowText(
        TFormattedString(SFlaggablePropsDlgText::kPlayerRadioFmtStr, 2, akPlayerTraits[1].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER3_RADIO)->SetWindowText(
        TFormattedString(SFlaggablePropsDlgText::kPlayerRadioFmtStr, 3, akPlayerTraits[2].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER4_RADIO)->SetWindowText(
        TFormattedString(SFlaggablePropsDlgText::kPlayerRadioFmtStr, 4, akPlayerTraits[3].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER5_RADIO)->SetWindowText(
        TFormattedString(SFlaggablePropsDlgText::kPlayerRadioFmtStr, 5, akPlayerTraits[4].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER6_RADIO)->SetWindowText(
        TFormattedString(SFlaggablePropsDlgText::kPlayerRadioFmtStr, 6, akPlayerTraits[5].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER7_RADIO)->SetWindowText(
        TFormattedString(SFlaggablePropsDlgText::kPlayerRadioFmtStr, 7, akPlayerTraits[6].m_pColorName));
    GetDlgItem(IDC_OWNER_PLAYER8_RADIO)->SetWindowText(
        TFormattedString(SFlaggablePropsDlgText::kPlayerRadioFmtStr, 8, akPlayerTraits[7].m_pColorName));
    _m_bModified = false;
    _m_owner = _m_pFlaggable->getOwner() + 1;
    CDialog::OnInitDialog();
    return TRUE;
}

VA(0x0041e550, 0x23)
void TFlaggablePropsDlg::OnOK()
{
    CDialog::OnOK();
    _m_bModified = _m_bModified || _m_owner - 1 != _m_pFlaggable->getOwner();
}

VA(0x0041e573, 0x8)
void TFlaggablePropsDlg::OnDoubleClickedOwnerRadio()
{
    OnOK();
}
