// OptionsDlg.cpp - the options dialog (h3maped 0x4930c8..0x49381d). The
// autosave check enables the controls after it up to OK and fills the
// period with its default; the period is kept within 1..60 minutes. The
// implicit destructor has no body here: /OPT:ICF folded it onto an
// identical one at 0x45d4ca.
#include "editor/stdafx.h"

#include <stdio.h>

#include "va.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/MapEditorText.h"
#include "editor/Options.h"
#include "editor/OptionsDlg.h"

VA(0x0049329c, 0x97)
TOptionsDlg::TOptionsDlg(CWnd* pParent)
    : CDialog(TOptionsDlg::IDD, pParent)
{
    _m_bRepaint = FALSE;
}

VA_COMPGEN(0x00493333, 0x1c, SCALAR_DELETING_DTOR, TOptionsDlg)

VA(0x0049334f, 0x64)
void TOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TILE_FREQUENCY_SLIDER, _m_frequencySlider);
    DDX_Control(pDX, IDC_AUTOSAVE_MINUTES_SPIN, _m_minutesSpin);
    DDX_Control(pDX, IDC_AUTOSAVE_MINUTES_EDIT, _m_minutesEdit);
    DDX_Control(pDX, IDC_AUTOSAVE_CHECK, _m_autosaveCheck);
    DDX_Check(pDX, IDC_REPAINT_CHECK, _m_bRepaint);
}

VA(0x004933b3, 0x6)
BEGIN_MESSAGE_MAP(TOptionsDlg, CDialog)
    ON_BN_CLICKED(IDC_AUTOSAVE_CHECK, OnAutosaveCheck)
    ON_EN_KILLFOCUS(IDC_AUTOSAVE_MINUTES_EDIT, OnKillFocusMinutesEdit)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x004933b9, 0x25)
int TOptionsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kOptionsCaptionStr);
    return 0;
}

VA(0x004933de, 0x249)
BOOL TOptionsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_TILE_FREQUENCY_STATIC)->SetWindowText(SOptionsDlgText::kTileFrequencyStaticStr);
    GetDlgItem(IDC_TILE_FREQUENCY_NONE_STATIC)->SetWindowText(SOptionsDlgText::kNoneStaticStr);
    GetDlgItem(IDC_TILE_FREQUENCY_LOTS_STATIC)->SetWindowText(SOptionsDlgText::kLotsStaticStr);
    GetDlgItem(IDC_REPAINT_CHECK)->SetWindowText(SOptionsDlgText::kRepaintCheckStr);
    GetDlgItem(IDC_AUTOSAVE_OPTIONS_STATIC)->SetWindowText(SOptionsDlgText::kAutosaveOptionsStaticStr);
    GetDlgItem(IDC_AUTOSAVE_CHECK)->SetWindowText(SOptionsDlgText::kEnableAutosaveCheckStr);
    GetDlgItem(IDC_AUTOSAVE_EVERY_STATIC)->SetWindowText(SOptionsDlgText::kAutosaveEveryStaticStr);
    GetDlgItem(IDC_AUTOSAVE_MINUTES_STATIC)->SetWindowText(SOptionsDlgText::kMinutesStaticStr);
    _m_autosaveMinutes = SOptions::kAutosaveMinutes;
    _m_specialTileFrequency = SOptions::kSpecialTileFrequency;
    _m_bRepaint = FALSE;
    CDialog::OnInitDialog();
    _m_minutesEdit.LimitText(TDigits<SOptions::kMaxAutosaveMinutes>::getDigits());
    _m_minutesSpin.SetRange(1, SOptions::kMaxAutosaveMinutes);
    if (_m_autosaveMinutes > 0) {
        _m_autosaveCheck.SetCheck(1);
        CString text;
        text.Format("%d", _m_autosaveMinutes);
        _m_minutesEdit.SetWindowText(text);
    } else {
        _m_autosaveCheck.SetCheck(0);
        for (CWnd* pWnd = _m_autosaveCheck.GetNextWindow(); pWnd->GetDlgCtrlID() != IDOK;
             pWnd = pWnd->GetNextWindow())
            pWnd->EnableWindow(FALSE);
    }
    _m_frequencySlider.SetRange(0, 8);
    _m_frequencySlider.SetPos(_m_specialTileFrequency);
    return TRUE;
}

VA(0x00493627, 0xc4)
void TOptionsDlg::OnOK()
{
    CDialog::OnOK();
    if (_m_autosaveCheck.GetCheck()) {
        int minutes = 0;
        CString text;
        _m_minutesEdit.GetWindowText(text);
        sscanf(text, "%d", &minutes);
        minutes = clamp<int>(1, minutes, SOptions::kMaxAutosaveMinutes);
        _m_autosaveMinutes = minutes;
    } else {
        _m_autosaveMinutes = 0;
    }
    _m_specialTileFrequency = _m_frequencySlider.GetPos();
}

VA(0x004936eb, 0x70)
void TOptionsDlg::OnAutosaveCheck()
{
    BOOL bAutosave = _m_autosaveCheck.GetCheck();
    for (CWnd* pWnd = _m_autosaveCheck.GetNextWindow(); pWnd->GetDlgCtrlID() != IDOK;
         pWnd = pWnd->GetNextWindow())
        pWnd->EnableWindow(bAutosave);
    if (bAutosave)
        _m_minutesEdit.SetWindowText("5");
    else
        _m_minutesEdit.SetWindowText("0");
}

VA(0x0049375b, 0xa6)
void TOptionsDlg::OnKillFocusMinutesEdit()
{
    int minutes = 0;
    CString text;
    _m_minutesEdit.GetWindowText(text);
    sscanf(text, "%d", &minutes);
    if (minutes < 1 || minutes > SOptions::kMaxAutosaveMinutes) {
        minutes = clamp<int>(1, minutes, SOptions::kMaxAutosaveMinutes);
        text.Format("%d", minutes);
        _m_minutesEdit.SetWindowText(text);
    }
}
