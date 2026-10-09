// SignPropsDlg.cpp - the sign properties dialog (h3maped
// 0x4b8b94..0x4b909d; Loki h3maped object 89). DoModal stores the edited
// message in the sign on OK; the edit control shows it with Windows line
// breaks. The implicit destructor has no body here: /OPT:ICF folded it
// onto an identical one at 0x481a8d.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/SignPropsDlg.h"

VA(0x004b8d68, 0x76)
TSignPropsDlg::TSignPropsDlg(CWnd* pParent, TSign* pSign)
    : CDialog(TSignPropsDlg::IDD, pParent),
      _m_pSign(pSign),
      _m_bModified(false)
{
    _m_text = _T("");
}

VA_COMPGEN(0x004b8dde, 0x1c, SCALAR_DELETING_DTOR, TSignPropsDlg)

VA(0x004b8dfa, 0x2d)
void TSignPropsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_SIGN_MESSAGE_EDIT, _m_messageEdit);
    DDX_Text(pDX, IDC_SIGN_MESSAGE_EDIT, _m_text);
}

VA(0x004b8e27, 0x6)
BEGIN_MESSAGE_MAP(TSignPropsDlg, CDialog)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x004b8e2d, 0x76)
int TSignPropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (result == IDOK)
        _m_pSign->setText(string(_m_text));
    return result;
}

VA(0x004b8ea3, 0xc1)
BOOL TSignPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_MESSAGE_STATIC)->SetWindowText(SSignPropsDlgText::kMessageStaticStr);
    _m_bModified = false;
    _m_text = _m_pSign->getText().c_str();
    _m_text.Replace("\n", "\r\n");
    CDialog::OnInitDialog();
    _m_messageEdit.LimitText(TSign::s_kMaxTextLen);
    return TRUE;
}

VA(0x004b8f64, 0xa0)
void TSignPropsDlg::OnOK()
{
    CDialog::OnOK();
    _m_text.Replace("\r\n", "\n");
    _m_bModified = _m_bModified || string(_m_text) != _m_pSign->getText();
}

VA(0x004b9004, 0x99)
int TSignPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pSign->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}
