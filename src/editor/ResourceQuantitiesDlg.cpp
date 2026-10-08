// ResourceQuantitiesDlg.cpp - the resource quantities dialog (h3maped
// 0x4b3442..0x4b39d6; Loki h3maped object 26). A taken quantity shows as
// its magnitude with the take radio set.
#include "editor/stdafx.h"

#include <stdio.h>

#include "va.h"
#include "editor/Digits.h"
#include "editor/MapEditorText.h"
#include "editor/ResourceQuantitiesDlg.h"

struct TResourceControlIDs {
    int m_editID;
    int m_spinID;
    int m_giveRadioID;
};

DATA(0x005416b8)
static const TResourceControlIDs g_aControlIDs[kNumGameResourceTypes] = {
    { IDC_RESOURCE1_EDIT, IDC_RESOURCE1_SPIN, IDC_RESOURCE1_GIVE_RADIO },
    { IDC_RESOURCE2_EDIT, IDC_RESOURCE2_SPIN, IDC_RESOURCE2_GIVE_RADIO },
    { IDC_RESOURCE3_EDIT, IDC_RESOURCE3_SPIN, IDC_RESOURCE3_GIVE_RADIO },
    { IDC_RESOURCE4_EDIT, IDC_RESOURCE4_SPIN, IDC_RESOURCE4_GIVE_RADIO },
    { IDC_RESOURCE5_EDIT, IDC_RESOURCE5_SPIN, IDC_RESOURCE5_GIVE_RADIO },
    { IDC_RESOURCE6_EDIT, IDC_RESOURCE6_SPIN, IDC_RESOURCE6_GIVE_RADIO },
    { IDC_RESOURCE7_EDIT, IDC_RESOURCE7_SPIN, IDC_RESOURCE7_GIVE_RADIO }
};

VA(0x004b345e, 0x77)
TResourceQuantitiesDlg::TResourceQuantitiesDlg(const TResourceQuantities& resourceQuantities)
    : CDialog(TResourceQuantitiesDlg::IDD),
      _m_originalQuantities(resourceQuantities),
      _m_bModified(false)
{
}

VA_COMPGEN(0x004b34d5, 0x1c, SCALAR_DELETING_DTOR, TResourceQuantitiesDlg)
VA_COMPGEN(0x004b34f1, 0x41, CLASS_CTOR, _TControls)
VA_COMPGEN(0x004b3532, 0x35, IMPLICIT_DTOR, _TControls)
VA_COMPGEN(0x004b3567, 0x49, IMPLICIT_DTOR, TResourceQuantitiesDlg)

VA(0x004b35b0, 0x49)
void TResourceQuantitiesDlg::DoDataExchange(CDataExchange* pDX)
{
    for (unsigned int i = 0; i < kNumGameResourceTypes; i++) {
        DDX_Control(pDX, g_aControlIDs[i].m_editID, _m_aControls[i].m_edit);
        DDX_Control(pDX, g_aControlIDs[i].m_spinID, _m_aControls[i].m_spin);
        DDX_Radio(pDX, g_aControlIDs[i].m_giveRadioID, _m_aControls[i].m_take);
    }
}

VA(0x004b35f9, 0x6)
BEGIN_MESSAGE_MAP(TResourceQuantitiesDlg, CDialog)
END_MESSAGE_MAP()

VA(0x004b35ff, 0x311)
BOOL TResourceQuantitiesDlg::OnInitDialog()
{
    GetDlgItem(IDC_RESOURCE1_STATIC)->SetWindowText(SResourceQuantitiesDlgText::kRes1StaticStr);
    GetDlgItem(IDC_RESOURCE2_STATIC)->SetWindowText(SResourceQuantitiesDlgText::kRes2StaticStr);
    GetDlgItem(IDC_RESOURCE3_STATIC)->SetWindowText(SResourceQuantitiesDlgText::kRes3StaticStr);
    GetDlgItem(IDC_RESOURCE4_STATIC)->SetWindowText(SResourceQuantitiesDlgText::kRes4StaticStr);
    GetDlgItem(IDC_RESOURCE5_STATIC)->SetWindowText(SResourceQuantitiesDlgText::kRes5StaticStr);
    GetDlgItem(IDC_RESOURCE6_STATIC)->SetWindowText(SResourceQuantitiesDlgText::kRes6StaticStr);
    GetDlgItem(IDC_RESOURCE7_STATIC)->SetWindowText(SResourceQuantitiesDlgText::kRes7StaticStr);
    GetDlgItem(IDC_RESOURCE1_GIVE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kGiveRadioStr);
    GetDlgItem(IDC_RESOURCE2_GIVE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kGiveRadioStr);
    GetDlgItem(IDC_RESOURCE3_GIVE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kGiveRadioStr);
    GetDlgItem(IDC_RESOURCE4_GIVE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kGiveRadioStr);
    GetDlgItem(IDC_RESOURCE5_GIVE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kGiveRadioStr);
    GetDlgItem(IDC_RESOURCE6_GIVE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kGiveRadioStr);
    GetDlgItem(IDC_RESOURCE7_GIVE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kGiveRadioStr);
    GetDlgItem(IDC_RESOURCE1_TAKE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kTakeRadioStr);
    GetDlgItem(IDC_RESOURCE2_TAKE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kTakeRadioStr);
    GetDlgItem(IDC_RESOURCE3_TAKE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kTakeRadioStr);
    GetDlgItem(IDC_RESOURCE4_TAKE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kTakeRadioStr);
    GetDlgItem(IDC_RESOURCE5_TAKE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kTakeRadioStr);
    GetDlgItem(IDC_RESOURCE6_TAKE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kTakeRadioStr);
    GetDlgItem(IDC_RESOURCE7_TAKE_RADIO)->SetWindowText(SResourceQuantitiesDlgText::kTakeRadioStr);
    _m_bModified = false;
    _m_resourceQuantities = _m_originalQuantities;
    unsigned int type;
    for (type = 0; type < kNumGameResourceTypes; type++)
        _m_aControls[type].m_take = _m_resourceQuantities.get(TGameResourceType(type)) < 0;
    CDialog::OnInitDialog();
    for (type = 0; type < kNumGameResourceTypes; type++) {
        _m_aControls[type].m_edit.LimitText(TDigits<TResourceQuantities::s_kMax>::getDigits());
        int quantity = _m_resourceQuantities.get(TGameResourceType(type));
        if (quantity < 0)
            quantity = -quantity;
        CString text;
        text.Format("%d", quantity);
        _m_aControls[type].m_edit.SetWindowText(text);
        _m_aControls[type].m_spin.SetRange(0, UD_MAXVAL);
    }
    return TRUE;
}

VA(0x004b3910, 0xc6)
void TResourceQuantitiesDlg::OnOK()
{
    CDialog::OnOK();
    for (unsigned int type = 0; type < kNumGameResourceTypes; type++) {
        int quantity = 0;
        CString text;
        _m_aControls[type].m_edit.GetWindowText(text);
        sscanf(text, "%d", &quantity);
        if (_m_aControls[type].m_take)
            quantity = -quantity;
        _m_resourceQuantities.set(TGameResourceType(type), quantity);
    }
    _m_bModified = _m_bModified || _m_resourceQuantities != _m_originalQuantities;
}
