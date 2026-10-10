// MonsterPropsGeneralPage.cpp - the general page of the monster property
// sheet (h3maped 0x488f0e..0x4898d1; GOG only). A quantity of 0 is random:
// the edit shows nothing and is disabled; entering 0 selects random.
#include "editor/stdafx.h"

#include <stdio.h>

#include "va.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/MapEditorText.h"
#include "editor/Monster.h"
#include "editor/MonsterPropsGeneralPage.h"

VA(0x004890e2, 0xfb)
TMonsterPropsGeneralPage::TMonsterPropsGeneralPage(TMonster* pMonster, EGameVersion mapVersion)
    : CPropertyPage(TMonsterPropsGeneralPage::IDD),
      _m_pMonster(pMonster),
      _m_mapVersion(mapVersion),
      _m_bModified(false)
{
    _m_typeName = _T("");
    _m_disposition = -1;
    _m_message = _T("");
    _m_quantityChoice = -1;
    _m_bNeverFlees = FALSE;
    _m_bNeverGrows = FALSE;
    m_psp.pszTitle = m_strCaption = kGeneralPageCaptionStr;
    m_psp.dwFlags |= PSP_USETITLE;
}

VA_COMPGEN(0x004891dd, 0x1c, SCALAR_DELETING_DTOR, TMonsterPropsGeneralPage)

VA(0x004891f9, 0x7d)
TMonsterPropsGeneralPage::~TMonsterPropsGeneralPage()
{
}

VA(0x00489276, 0xae)
void TMonsterPropsGeneralPage::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_QUANTITY_SPIN, _m_quantitySpin);
    DDX_Control(pDX, IDC_QUANTITY_EDIT, _m_quantityEdit);
    DDX_Control(pDX, IDC_MESSAGE_EDIT, _m_messageEdit);
    DDX_Text(pDX, IDC_CREATURE_TYPE_STATIC, _m_typeName);
    DDX_Radio(pDX, IDC_COMPLIANT_RADIO, _m_disposition);
    DDX_Text(pDX, IDC_MESSAGE_EDIT, _m_message);
    DDX_Radio(pDX, IDC_RANDOM_QTY_RADIO, _m_quantityChoice);
    DDX_Check(pDX, IDC_NEVER_FLEES_CHECK, _m_bNeverFlees);
    DDX_Check(pDX, IDC_NEVER_GROWS_CHECK, _m_bNeverGrows);
}

VA(0x00489324, 0x6)
BEGIN_MESSAGE_MAP(TMonsterPropsGeneralPage, CPropertyPage)
    ON_EN_KILLFOCUS(IDC_QUANTITY_EDIT, OnKillFocusQuantityEdit)
    ON_BN_CLICKED(IDC_RANDOM_QTY_RADIO, OnRandomQtyRadio)
    ON_BN_CLICKED(IDC_CUSTOM_QTY_RADIO, OnCustomQtyRadio)
END_MESSAGE_MAP()

VA(0x0048932a, 0x2ee)
BOOL TMonsterPropsGeneralPage::OnInitDialog()
{
    GetDlgItem(IDC_TYPE_STATIC)->SetWindowText(SMonsterPropsGeneralPageText::kTypeStaticStr);
    GetDlgItem(IDC_QUANTITY_STATIC)->SetWindowText(SMonsterPropsGeneralPageText::kQuantityStaticStr);
    GetDlgItem(IDC_RANDOM_QTY_RADIO)->SetWindowText(SMonsterPropsGeneralPageText::kRandomQtyRadioStr);
    GetDlgItem(IDC_CUSTOM_QTY_RADIO)->SetWindowText(SMonsterPropsGeneralPageText::kCustomQtyRadioStr);
    GetDlgItem(IDC_NEVER_GROWS_CHECK)->SetWindowText(SMonsterPropsGeneralPageText::kNeverGrowsCheckStr);
    GetDlgItem(IDC_DISPOSITION_STATIC)->SetWindowText(SMonsterPropsGeneralPageText::kDispositionStaticStr);
    GetDlgItem(IDC_COMPLIANT_RADIO)->SetWindowText(SMonsterPropsGeneralPageText::kDispositionCompliantRadioStr);
    GetDlgItem(IDC_FRIENDLY_RADIO)->SetWindowText(SMonsterPropsGeneralPageText::kDispositionFriendlyRadioStr);
    GetDlgItem(IDC_AGGRESSIVE_RADIO)->SetWindowText(SMonsterPropsGeneralPageText::kDispositionAggressiveRadioStr);
    GetDlgItem(IDC_HOSTILE_RADIO)->SetWindowText(SMonsterPropsGeneralPageText::kDispositionHostileRadioStr);
    GetDlgItem(IDC_SAVAGE_RADIO)->SetWindowText(SMonsterPropsGeneralPageText::kDispositionSavageRadioStr);
    GetDlgItem(IDC_NEVER_FLEES_CHECK)->SetWindowText(SMonsterPropsGeneralPageText::kNeverFleesCheckStr);
    GetDlgItem(IDC_MESSAGE_STATIC)->SetWindowText(SMonsterPropsGeneralPageText::kMessageStaticStr);
    _m_bModified = false;
    _m_typeName = _m_pMonster->getTypeName().c_str();
    _m_quantity = _m_pMonster->getQuantity();
    _m_quantityChoice = _m_quantity > 0;
    _m_disposition = _m_pMonster->getDisposition();
    _m_bNeverFlees = _m_pMonster->getBNeverFlees();
    _m_bNeverGrows = _m_pMonster->getBNeverGrows();
    _m_message = _m_pMonster->getMessage().c_str();
    _m_message.Replace("\n", "\r\n");
    CPropertyPage::OnInitDialog();
    _m_quantityEdit.LimitText(TDigits<TMonster::s_kMaxQuantity>::getDigits());
    _m_quantitySpin.SetRange(1, TMonster::s_kMaxQuantity);
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
        _m_messageEdit.LimitText(TMonster::s_kMaxMessageLen);
    return TRUE;
}

VA(0x00489618, 0x168)
void TMonsterPropsGeneralPage::OnOK()
{
    CPropertyPage::OnOK();
    _m_message.Replace("\r\n", "\n");
    if (_m_quantityChoice) {
        int value = 0;
        CString text;
        _m_quantityEdit.GetWindowText(text);
        sscanf(text, "%d", &value);
        _m_quantity = clamp(0U, unsigned(value), unsigned(TMonster::s_kMaxQuantity));
    } else {
        _m_quantity = 0;
    }
    _m_bModified = _m_bModified || _m_quantity != _m_pMonster->getQuantity()
                   || _m_disposition != _m_pMonster->getDisposition()
                   || (_m_bNeverFlees != FALSE) != _m_pMonster->getBNeverFlees()
                   || (_m_bNeverGrows != FALSE) != _m_pMonster->getBNeverGrows()
                   || std::string(_m_message) != _m_pMonster->getMessage();
}

VA(0x00489780, 0xbd)
void TMonsterPropsGeneralPage::OnKillFocusQuantityEdit()
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
    } else if (value > TMonster::s_kMaxQuantity) {
        text.Format("%d", TMonster::s_kMaxQuantity);
        _m_quantityEdit.SetWindowText(text);
    }
}

VA(0x0048983d, 0x4a)
void TMonsterPropsGeneralPage::OnRandomQtyRadio()
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

VA(0x00489887, 0x4a)
void TMonsterPropsGeneralPage::OnCustomQtyRadio()
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
