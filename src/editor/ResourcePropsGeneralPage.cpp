// ResourcePropsGeneralPage.cpp - the general page of the resource property
// sheet (h3maped 0x4b266a..0x4b2ec0; GOG only). A quantity of 0 is random:
// the edit shows nothing and is disabled; entering 0 selects random.
#include "editor/stdafx.h"

#include <stdio.h>

#include "va.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/MapEditorText.h"
#include "editor/ObjectSpecializations.h"
#include "editor/ResourcePropsGeneralPage.h"

VA(0x004b2822, 0xe6)
TResourcePropsGeneralPage::TResourcePropsGeneralPage(TGameResource* pResource, EGameVersion mapVersion)
    : CPropertyPage(TResourcePropsGeneralPage::IDD),
      _m_pResource(pResource),
      _m_mapVersion(mapVersion),
      _m_bModified(false)
{
    _m_message = _T("");
    _m_quantityChoice = -1;
    _m_typeName = _T("");
    m_psp.pszTitle = m_strCaption = kGeneralPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x004b2908, 0x1c, SCALAR_DELETING_DTOR, TResourcePropsGeneralPage)

VA(0x004b2924, 0x7d)
TResourcePropsGeneralPage::~TResourcePropsGeneralPage()
{
}

VA(0x004b29a1, 0x78)
void TResourcePropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_QUANTITY_SPIN, _m_quantitySpin);
    DDX_Control(pDX, IDC_QUANTITY_EDIT, _m_quantityEdit);
    DDX_Control(pDX, IDC_MESSAGE_EDIT, _m_messageEdit);
    DDX_Text(pDX, IDC_MESSAGE_EDIT, _m_message);
    DDX_Radio(pDX, IDC_RANDOM_QTY_RADIO, _m_quantityChoice);
    DDX_Text(pDX, IDC_RESOURCE_TYPE_STATIC, _m_typeName);
}

VA(0x004b2a19, 0x6)
BEGIN_MESSAGE_MAP(TResourcePropsGeneralPage, CPropertyPage)
    ON_BN_CLICKED(IDC_RANDOM_QTY_RADIO, OnRandomQtyRadio)
    ON_BN_CLICKED(IDC_CUSTOM_QTY_RADIO, OnCustomQtyRadio)
    ON_EN_KILLFOCUS(IDC_QUANTITY_EDIT, OnKillFocusQuantityEdit)
END_MESSAGE_MAP()

VA(0x004b2a1f, 0x22b)
BOOL TResourcePropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_TYPE_STATIC)->SetWindowText(SResourcePropsGeneralPageText::kTypeStaticStr);
    GetDlgItem(IDC_QUANTITY_STATIC)->SetWindowText(SResourcePropsGeneralPageText::kQuantityStaticStr);
    GetDlgItem(IDC_RANDOM_QTY_RADIO)->SetWindowText(SResourcePropsGeneralPageText::kRandomQtyRadioStr);
    GetDlgItem(IDC_CUSTOM_QTY_RADIO)->SetWindowText(SResourcePropsGeneralPageText::kCustomQtyRadioStr);
    GetDlgItem(IDC_QUANTITY_NOTE_STATIC)->SetWindowText(SResourcePropsGeneralPageText::kQuantityNoteStaticStr);
    GetDlgItem(IDC_MESSAGE_STATIC)->SetWindowText(SResourcePropsGeneralPageText::kMessageStaticStr);
    GetDlgItem(IDC_NOTE_STATIC)->SetWindowText(SResourcePropsGeneralPageText::kNoteStaticStr);
    _m_bModified = false;
    _m_typeName = _m_pResource->getTypeName().c_str();
    _m_quantity = _m_pResource->getQuantity();
    _m_quantityChoice = _m_quantity > 0;
    _m_message = _m_pResource->getMessage().c_str();
    _m_message.Replace("\n", "\r\n");
    CPropertyPage::OnInitDialog();
    _m_quantityEdit.LimitText(TDigits<TGameResource::s_kMaxQuantity>::getDigits());
    _m_quantitySpin.SetRange(1, UD_MAXVAL);
    if (_m_quantityChoice) {
        CString text;
        text.Format("%d", _m_quantity);
        _m_quantityEdit.SetWindowText(text);
    } else {
        _m_quantityEdit.SetWindowText("");
        _m_quantitySpin.EnableWindow(FALSE);
        _m_quantityEdit.EnableWindow(FALSE);
    }
    if (_m_mapVersion == GAME_VERSION_ROE)
        _m_messageEdit.LimitText(TTreasure::s_kMaxMessageLen);
    return TRUE;
}

VA(0x004b2c4a, 0x125)
void TResourcePropsGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_message.Replace("\r\n", "\n");
    if (_m_quantityChoice) {
        int value = 0;
        CString text;
        _m_quantityEdit.GetWindowText(text);
        sscanf(text, "%d", &value);
        _m_quantity = clamp(0U, unsigned(value), unsigned(TGameResource::s_kMaxQuantity));
    } else {
        _m_quantity = 0;
    }
    _m_bModified = _m_bModified || _m_quantity != _m_pResource->getQuantity()
                   || std::string(_m_message) != _m_pResource->getMessage();
}

VA(0x004b2d6f, 0xbd)
void TResourcePropsGeneralPage::OnKillFocusQuantityEdit()
{
    int value = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &value);
    if (value == 0) {
        _m_quantityChoice = 0;
        UpdateData(FALSE);
        _m_quantityEdit.SetWindowText("");
        _m_quantitySpin.EnableWindow(FALSE);
        _m_quantityEdit.EnableWindow(FALSE);
    } else if (value > TGameResource::s_kMaxQuantity) {
        text.Format("%d", TGameResource::s_kMaxQuantity);
        _m_quantityEdit.SetWindowText(text);
    }
}

VA(0x004b2e2c, 0x4a)
void TResourcePropsGeneralPage::OnRandomQtyRadio()
{
    if (_m_quantityChoice) {
        UpdateData(TRUE);
        if (!_m_quantityChoice) {
            _m_quantityEdit.SetWindowText("");
            _m_quantitySpin.EnableWindow(FALSE);
            _m_quantityEdit.EnableWindow(FALSE);
        }
    }
}

VA(0x004b2e76, 0x4a)
void TResourcePropsGeneralPage::OnCustomQtyRadio()
{
    if (!_m_quantityChoice) {
        UpdateData(TRUE);
        if (_m_quantityChoice) {
            _m_quantityEdit.EnableWindow(TRUE);
            _m_quantitySpin.EnableWindow(TRUE);
            _m_quantityEdit.SetWindowText("1");
        }
    }
}
