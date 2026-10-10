// HolyGrailPropsDlg.cpp - the grail's placement radius dialog (h3maped
// 0x45802b..0x4585e9; GOG only). DoModal stores the radius on OK; leaving
// the edit clamps a too large radius.
#include "editor/stdafx.h"

#include <stdio.h>

#include "va.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/HolyGrailPropsDlg.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"

VA(0x004581ff, 0x72)
THolyGrailPropsDlg::THolyGrailPropsDlg(CWnd* pParent, THolyGrail* pHolyGrail)
    : CDialog(THolyGrailPropsDlg::IDD, pParent),
      _m_pHolyGrail(pHolyGrail),
      _m_bModified(false)
{
}

VA_COMPGEN(0x00458271, 0x1c, SCALAR_DELETING_DTOR, THolyGrailPropsDlg)
VA_COMPGEN(0x0045828d, 0x47, IMPLICIT_DTOR, THolyGrailPropsDlg)

VA(0x004582d4, 0x2e)
void THolyGrailPropsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_RADIUS_SPIN, _m_radiusSpin);
    DDX_Control(pDX, IDC_RADIUS_EDIT, _m_radiusEdit);
}

VA(0x00458302, 0x6)
BEGIN_MESSAGE_MAP(THolyGrailPropsDlg, CDialog)
    ON_EN_KILLFOCUS(IDC_RADIUS_EDIT, OnKillFocusRadiusEdit)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x00458308, 0x99)
int THolyGrailPropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pHolyGrail->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}

VA(0x004583a1, 0x26)
int THolyGrailPropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (result == IDOK)
        _m_pHolyGrail->setRadius(_m_radius);
    return result;
}

VA(0x004583c7, 0x103)
BOOL THolyGrailPropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_RADIUS_STATIC)->SetWindowText(kPlacementRadiusStaticStr);
    _m_bModified = false;
    _m_radius = _m_pHolyGrail->getRadius();
    CDialog::OnInitDialog();
    _m_radiusEdit.LimitText(TDigits<THolyGrail::s_kMaxRadius>::getDigits());
    CString text;
    text.Format("%d", _m_radius);
    _m_radiusEdit.SetWindowText(text);
    _m_radiusSpin.SetRange(0, THolyGrail::s_kMaxRadius);
    return TRUE;
}

VA(0x004584ca, 0xa2)
void THolyGrailPropsDlg::OnOK()
{
    CDialog::OnOK();
    int value = 0;
    CString text;
    _m_radiusEdit.GetWindowText(text);
    sscanf(text, "%d", &value);
    _m_radius = clamp(0U, unsigned(value), unsigned(THolyGrail::s_kMaxRadius));
    _m_bModified = _m_bModified || _m_radius != _m_pHolyGrail->getRadius();
}

VA(0x0045856c, 0x7d)
void THolyGrailPropsDlg::OnKillFocusRadiusEdit()
{
    int value = 0;
    CString text;
    _m_radiusEdit.GetWindowText(text);
    sscanf(text, "%d", &value);
    if (value > THolyGrail::s_kMaxRadius) {
        text.Format("%d", THolyGrail::s_kMaxRadius);
        _m_radiusEdit.SetWindowText(text);
    }
}
