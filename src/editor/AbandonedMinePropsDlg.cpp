// AbandonedMinePropsDlg.cpp - the abandoned mine's resources dialog
// (h3maped 0x401000..0x401d08; GOG only). Unchecking the last checked
// resource checks the next one; DoModal stores the resources when OK
// changed them.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/AbandonedMinePropsDlg.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"

DATA(0x0052f760)
static const int g_akResourceCheckIDs[kNumGameResourceTypes] = {
    0,
    IDC_RES2_CHECK,
    IDC_RES3_CHECK,
    IDC_RES4_CHECK,
    IDC_RES5_CHECK,
    IDC_RES6_CHECK,
    IDC_RES7_CHECK
};

VA(0x004011b8, 0x5d)
TAbandonedMinePropsDlg::TAbandonedMinePropsDlg(CWnd* pParent, TAbandonedMine* pMine)
    : CDialog(TAbandonedMinePropsDlg::IDD, pParent),
      _m_pMine(pMine),
      _m_bModified(false),
      _m_abPotentialResource(0)
{
}

VA_COMPGEN(0x00401215, 0x1c, SCALAR_DELETING_DTOR, TAbandonedMinePropsDlg)
VA_COMPGEN(0x00401231, 0x3f, IMPLICIT_DTOR, TAbandonedMinePropsDlg)

VA(0x00401270, 0xc1)
void TAbandonedMinePropsDlg::_onResourceCheck(unsigned int type)
{
    while (!_m_aResourceCheck[type].GetCheck()) {
        if (!_m_abPotentialResource[type])
            return;
        _m_abPotentialResource.set(type, false);
        if (--_m_numPotentialResources != 0)
            return;
        do {
            if (++type >= kNumGameResourceTypes)
                type = 0;
        } while (type == eResourceWood);
        _m_aResourceCheck[type].SetCheck(1);
    }
    if (!_m_abPotentialResource[type]) {
        _m_abPotentialResource.set(type, true);
        ++_m_numPotentialResources;
    }
}

VA(0x00401331, 0x31)
void TAbandonedMinePropsDlg::DoDataExchange(CDataExchange* pDX)
{
    for (unsigned int type = 0; type < kNumGameResourceTypes; type++) {
        if (type != eResourceWood)
            DDX_Control(pDX, g_akResourceCheckIDs[type], _m_aResourceCheck[type]);
    }
}

VA(0x00401362, 0x6)
BEGIN_MESSAGE_MAP(TAbandonedMinePropsDlg, CDialog)
    ON_BN_CLICKED(IDC_RES2_CHECK, OnRes2Check)
    ON_BN_CLICKED(IDC_RES3_CHECK, OnRes3Check)
    ON_BN_CLICKED(IDC_RES4_CHECK, OnRes4Check)
    ON_BN_CLICKED(IDC_RES5_CHECK, OnRes5Check)
    ON_BN_CLICKED(IDC_RES6_CHECK, OnRes6Check)
    ON_BN_CLICKED(IDC_RES7_CHECK, OnRes7Check)
    ON_WM_CREATE()
END_MESSAGE_MAP()

VA(0x00401368, 0x96)
int TAbandonedMinePropsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    CString caption;
    caption.Format(kObjectPropertiesCaptionFmtStr, _m_pMine->getTypeName().c_str());
    SetWindowText(caption);
    return 0;
}

VA(0x004013fe, 0x45)
int TAbandonedMinePropsDlg::DoModal()
{
    int result = CDialog::DoModal();
    if (_m_bModified) {
        for (unsigned int type = 0; type < kNumGameResourceTypes; type++) {
            if (type != eResourceWood)
                _m_pMine->setBIsPotentialResource(TGameResourceType(type), _m_abPotentialResource[type]);
        }
    }
    return result;
}

VA(0x00401443, 0x186)
BOOL TAbandonedMinePropsDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_POTENTIAL_RESOURCES_GROUP)->SetWindowText(SAbandonedMinePropsDlgText::kPotentialResourcesStaticStr);
    GetDlgItem(IDC_RES2_CHECK)->SetWindowText(SAbandonedMinePropsDlgText::kRes2CheckStr);
    GetDlgItem(IDC_RES3_CHECK)->SetWindowText(SAbandonedMinePropsDlgText::kRes3CheckStr);
    GetDlgItem(IDC_RES4_CHECK)->SetWindowText(SAbandonedMinePropsDlgText::kRes4CheckStr);
    GetDlgItem(IDC_RES5_CHECK)->SetWindowText(SAbandonedMinePropsDlgText::kRes5CheckStr);
    GetDlgItem(IDC_RES6_CHECK)->SetWindowText(SAbandonedMinePropsDlgText::kRes6CheckStr);
    GetDlgItem(IDC_RES7_CHECK)->SetWindowText(SAbandonedMinePropsDlgText::kRes7CheckStr);
    _m_bModified = false;
    _m_numPotentialResources = _m_pMine->getNumPotentialResources();
    unsigned int type;
    for (type = 0; type < kNumGameResourceTypes; type++) {
        if (type != eResourceWood)
            _m_abPotentialResource[type] = _m_pMine->getBIsPotentialResource(TGameResourceType(type));
    }
    CDialog::OnInitDialog();
    for (type = 0; type < kNumGameResourceTypes; type++) {
        if (type != eResourceWood)
            _m_aResourceCheck[type].SetCheck(_m_abPotentialResource[type]);
    }
    return TRUE;
}

VA(0x004015c9, 0x81)
void TAbandonedMinePropsDlg::OnOK()
{
    CDialog::OnOK();
    unsigned int type;
    for (type = 0; type < kNumGameResourceTypes; type++) {
        if (type != eResourceWood)
            _m_abPotentialResource.set(type, _m_aResourceCheck[type].GetCheck() != 0);
    }
    for (type = 0; !_m_bModified && type < kNumGameResourceTypes; type++) {
        if (type != eResourceWood)
            _m_bModified = _m_abPotentialResource[type] != _m_pMine->getBIsPotentialResource(TGameResourceType(type));
    }
}

VA(0x0040164a, 0x8)
void TAbandonedMinePropsDlg::OnRes2Check()
{
    _onResourceCheck(1);
}

VA(0x00401652, 0x8)
void TAbandonedMinePropsDlg::OnRes3Check()
{
    _onResourceCheck(2);
}

VA(0x0040165a, 0x8)
void TAbandonedMinePropsDlg::OnRes4Check()
{
    _onResourceCheck(3);
}

VA(0x00401662, 0x8)
void TAbandonedMinePropsDlg::OnRes5Check()
{
    _onResourceCheck(4);
}

VA(0x0040166a, 0x8)
void TAbandonedMinePropsDlg::OnRes6Check()
{
    _onResourceCheck(5);
}

VA(0x00401672, 0x8)
void TAbandonedMinePropsDlg::OnRes7Check()
{
    _onResourceCheck(6);
}
