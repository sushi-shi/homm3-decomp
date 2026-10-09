// MapSpecsRumorsPage.cpp - the rumors page of the map specifications
// sheet (h3maped 0x47471d..0x475213; Loki h3maped object 73). A map holds
// at most 30 rumors; the Delete key removes the selected one.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/EditRumorDlg.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsRumorsPage.h"

VA(0x0047499f, 0xee)
TMapSpecsRumorsPage::TMapSpecsRumorsPage(const TGameMap& oldMap, TGameMap& newMap)
    : CPropertyPage(TMapSpecsRumorsPage::IDD),
      _m_oldMap(oldMap),
      _m_newMap(newMap),
      _m_bModified(false)
{
    m_psp.pszTitle = m_strCaption = kRumorsPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x00474a8d, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsRumorsPage)

VA(0x00474aa9, 0x8c)
TMapSpecsRumorsPage::~TMapSpecsRumorsPage()
{
}

VA(0x00474b35, 0x67)
void TMapSpecsRumorsPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_RUMORS_ADD_BUTTON, _m_addButton);
    DDX_Control(pDX, IDC_RUMORS_EDIT_BUTTON, _m_editButton);
    DDX_Control(pDX, IDC_RUMORS_REMOVE_ALL_BUTTON, _m_removeAllButton);
    DDX_Control(pDX, IDC_RUMORS_REMOVE_BUTTON, _m_removeButton);
    DDX_Control(pDX, IDC_RUMORS_LIST, _m_rumorsList);
}

VA(0x00474b9c, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsRumorsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_RUMORS_ADD_BUTTON, OnAddRumorButton)
    ON_BN_CLICKED(IDC_RUMORS_EDIT_BUTTON, OnEditRumorButton)
    ON_BN_CLICKED(IDC_RUMORS_REMOVE_BUTTON, OnRemoveRumorButton)
    ON_BN_CLICKED(IDC_RUMORS_REMOVE_ALL_BUTTON, OnRemoveAllRumorButton)
    ON_LBN_SELCHANGE(IDC_RUMORS_LIST, OnSelChangeRumorsList)
    ON_LBN_SELCANCEL(IDC_RUMORS_LIST, OnSelCancelRumorsList)
    ON_LBN_DBLCLK(IDC_RUMORS_LIST, OnDblClkRumorsList)
    ON_WM_VKEYTOITEM()
END_MESSAGE_MAP()

VA(0x00474ba2, 0x58)
void TMapSpecsRumorsPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_newMap.setRumors(_m_rumors);
    _m_bModified = _m_bModified || _m_newMap.getRumors() != _m_oldMap.getRumors();
}

VA(0x00474bfa, 0x189)
BOOL TMapSpecsRumorsPage::OnInitDialog()
{
    GetDlgItem(IDC_RUMORS_STATIC)->SetWindowText(SMapSpecsRumorsPageText::kRumorsStaticStr);
    GetDlgItem(IDC_RUMORS_ADD_BUTTON)->SetWindowText(SMapSpecsRumorsPageText::kAddButtonStr);
    GetDlgItem(IDC_RUMORS_EDIT_BUTTON)->SetWindowText(SMapSpecsRumorsPageText::kEditButtonStr);
    GetDlgItem(IDC_RUMORS_REMOVE_BUTTON)->SetWindowText(SMapSpecsRumorsPageText::kRemoveButtonStr);
    GetDlgItem(IDC_RUMORS_REMOVE_ALL_BUTTON)->SetWindowText(SMapSpecsRumorsPageText::kRemoveAllButtonStr);
    _m_bModified = false;
    _m_rumors = _m_newMap.getRumors();
    CPropertyPage::OnInitDialog();
    for (unsigned int i = 0; i < _m_rumors.size(); i++) {
        int index = _m_rumorsList.AddString(_m_rumors[i].getName().c_str());
        _m_rumorsList.SetItemData(index, i);
    }
    _m_rumorsList.SetCurSel(-1);
    _m_addButton.EnableWindow(_m_rumorsList.GetCount() < s_kMaxNumRumors);
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
    _m_removeAllButton.EnableWindow(_m_rumorsList.GetCount() > 0);
    return TRUE;
}

VA(0x00474d83, 0x134)
void TMapSpecsRumorsPage::OnAddRumorButton()
{
    TEditRumorDlg dlg(this, TRumor());
    if (dlg.DoModal() == IDOK) {
        unsigned int newRumorNum = _m_rumors.size();
        _m_rumors.push_back(dlg.getRumor());
        int index = _m_rumorsList.AddString(_m_rumors.back().getName().c_str());
        _m_rumorsList.SetItemData(index, newRumorNum);
        _m_rumorsList.SetCurSel(index);
        if (_m_rumorsList.GetCount() >= s_kMaxNumRumors)
            _m_addButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(TRUE);
    }
}

VA(0x00474eb7, 0xf8)
void TMapSpecsRumorsPage::OnEditRumorButton()
{
    int curSel = _m_rumorsList.GetCurSel();
    unsigned int rumorNum = _m_rumorsList.GetItemData(curSel);
    TEditRumorDlg dlg(this, _m_rumors[rumorNum]);
    if (dlg.DoModal() == IDOK) {
        _m_rumors[rumorNum] = dlg.getRumor();
        _m_rumorsList.DeleteString(curSel);
        int index = _m_rumorsList.AddString(_m_rumors[rumorNum].getName().c_str());
        _m_rumorsList.SetItemData(index, rumorNum);
        _m_rumorsList.SetCurSel(index);
    }
}

VA(0x00474faf, 0x133)
void TMapSpecsRumorsPage::OnRemoveRumorButton()
{
    int curSel = _m_rumorsList.GetCurSel();
    unsigned int rumorNum = _m_rumorsList.GetItemData(curSel);
    _m_rumors.erase(_m_rumors.begin() + rumorNum);
    _m_rumorsList.DeleteString(curSel);
    int count = _m_rumorsList.GetCount();
    if (count > 0) {
        if (count < s_kMaxNumRumors)
            _m_addButton.EnableWindow(TRUE);
        for (int i = 0; i < count; i++) {
            unsigned int itemRumorNum = _m_rumorsList.GetItemData(i);
            if (itemRumorNum > rumorNum)
                _m_rumorsList.SetItemData(i, itemRumorNum - 1);
        }
        _m_rumorsList.SetCurSel(curSel < count ? curSel : count - 1);
    } else {
        _m_addButton.EnableWindow(TRUE);
        _m_editButton.EnableWindow(FALSE);
        _m_removeButton.EnableWindow(FALSE);
        _m_removeAllButton.EnableWindow(FALSE);
        GotoDlgCtrl(&_m_addButton);
    }
}

VA(0x004750e2, 0x53)
void TMapSpecsRumorsPage::OnRemoveAllRumorButton()
{
    if (_m_rumorsList.GetCurSel() == -1)
        _m_rumorsList.SetCurSel(0);
    for (int count = _m_rumorsList.GetCount(); count > 0; count--)
        OnRemoveRumorButton();
}

VA(0x00475135, 0x1f)
void TMapSpecsRumorsPage::OnSelChangeRumorsList()
{
    _m_editButton.EnableWindow(TRUE);
    _m_removeButton.EnableWindow(TRUE);
}

VA(0x00475154, 0x1f)
void TMapSpecsRumorsPage::OnSelCancelRumorsList()
{
    _m_editButton.EnableWindow(FALSE);
    _m_removeButton.EnableWindow(FALSE);
}

VA(0x00475173, 0x5)
void TMapSpecsRumorsPage::OnDblClkRumorsList()
{
    OnEditRumorButton();
}

VA(0x00475178, 0x47)
int TMapSpecsRumorsPage::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex)
{
    if (nKey == VK_DELETE && _m_rumorsList.GetCurSel() != -1) {
        OnRemoveRumorButton();
        return -2;
    }
    return CPropertyPage::OnVKeyToItem(nKey, pListBox, nIndex);
}
