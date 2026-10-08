// EditRumorDlg.cpp - the rumor editor (h3maped 0x41576b..0x415dc1; Loki
// h3maped object 76). The edit control shows the text with Windows line
// breaks; OnOK stores it back with the map's bare newlines.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/EditRumorDlg.h"
#include "editor/MapEditorText.h"
#include "editor/StringUtil.h"

VA(0x0041593f, 0xab)
TEditRumorDlg::TEditRumorDlg(CWnd* pParent, const TRumor& rumor)
    : CDialog(TEditRumorDlg::IDD, pParent),
      _m_rumor(rumor)
{
    _m_rumorName = _T("");
    _m_rumorText = _T("");
}

VA_COMPGEN(0x004159ea, 0x39, IMPLICIT_DTOR, TRumor)
VA_COMPGEN(0x00415a23, 0x1c, SCALAR_DELETING_DTOR, TEditRumorDlg)
VA_COMPGEN(0x00415a3f, 0x6b, IMPLICIT_COPY_CTOR, TRumor)
VA_COMPGEN(0x00415aaa, 0x8d, IMPLICIT_DTOR, TEditRumorDlg)

VA(0x00415b37, 0x4e)
void TEditRumorDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_RUMOR_TEXT_EDIT, _m_rumorTextEdit);
    DDX_Control(pDX, IDOK, _m_okButton);
    DDX_Text(pDX, IDC_RUMOR_NAME_EDIT, _m_rumorName);
    DDX_Text(pDX, IDC_RUMOR_TEXT_EDIT, _m_rumorText);
}

VA(0x00415b85, 0xbc)
void TEditRumorDlg::OnOK()
{
    CDialog::OnOK();
    _m_rumorText.Replace("\r\n", "\n");
    _m_rumor.setNameAndText(string(_m_rumorName), string(_m_rumorText));
}

VA(0x00415c41, 0x41)
void TEditRumorDlg::_checkStrings()
{
    UpdateData(TRUE);
    _m_okButton.EnableWindow(!_isspace(_m_rumorName) && !_isspace(_m_rumorText));
}

VA(0x00415c82, 0x6)
BEGIN_MESSAGE_MAP(TEditRumorDlg, CDialog)
    ON_EN_CHANGE(IDC_RUMOR_NAME_EDIT, OnChangeRumorNameEdit)
    ON_EN_CHANGE(IDC_RUMOR_TEXT_EDIT, OnChangeRumorTextEdit)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x00415c88, 0x25)
int TEditRumorDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kEditRumorCaptionStr);
    return 0;
}

VA(0x00415cad, 0x10f)
BOOL TEditRumorDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_RUMOR_NAME_STATIC)->SetWindowText(SEditRumorDlgText::kNameStaticStr);
    GetDlgItem(IDC_RUMOR_TEXT_STATIC)->SetWindowText(SEditRumorDlgText::kTextStaticStr);
    _m_rumorName = _m_rumor.getName().c_str();
    _m_rumorText = _m_rumor.getText().c_str();
    _m_rumorText.Replace("\n", "\r\n");
    CDialog::OnInitDialog();
    _m_rumorTextEdit.LimitText(TRumor::s_kMaxTextLen);
    if (_isspace(_m_rumorName) || _isspace(_m_rumorText))
        _m_okButton.EnableWindow(FALSE);
    return TRUE;
}

// /OPT:ICF folded the two identical change handlers onto one body; the
// message map's two EN_CHANGE entries both name it.
VA(0x00415dbc, 0x5)
void TEditRumorDlg::OnChangeRumorNameEdit()
{
    _checkStrings();
}

void TEditRumorDlg::OnChangeRumorTextEdit()
{
    _checkStrings();
}
