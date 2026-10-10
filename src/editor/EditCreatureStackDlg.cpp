// EditCreatureStackDlg.cpp - the creature stack editor (h3maped
// 0x40ddd9..0x40e4f5; GOG only). The type combo lists the creatures in the
// army editor's order: by town (the neutrals last), then level, then id.
// The implicit destructor is the black box contents page's copy
// (0x40ae04).
#include "editor/stdafx.h"

#include <stdio.h>

#include "town_type.h"
#include "va.h"
#include "editor/Clamp.h"
#include "editor/Digits.h"
#include "editor/EditCreatureStackDlg.h"
#include "editor/MapEditorText.h"

VA(0x0040ddf5, 0x80)
static bool isCreatureTypeBefore(TCreatureType lhs, TCreatureType rhs)
{
    TCreatureTypeTraits lhsTraits = akCreatureTypeTraits[lhs];
    TCreatureTypeTraits rhsTraits = akCreatureTypeTraits[rhs];
    int lhsTown = lhsTraits.townType == eTownNeutral ? kNumTownTypes : lhsTraits.townType;
    int rhsTown = rhsTraits.townType == eTownNeutral ? kNumTownTypes : rhsTraits.townType;
    if (lhsTown != rhsTown)
        return lhsTown < rhsTown;
    if (lhsTraits.level != rhsTraits.level)
        return lhsTraits.level < rhsTraits.level;
    return lhs < rhs;
}

VA(0x0040de75, 0x73)
static void insertCreatureType(CComboBox& combo, TCreatureType creature)
{
    int index = combo.GetCount();
    while (index > 0 && isCreatureTypeBefore(creature, TCreatureType(combo.GetItemData(index - 1))))
        index--;
    combo.InsertString(index, akCreatureTypeTraits[creature].m_name);
    combo.SetItemData(index, creature);
}

VA(0x0040dee8, 0xe3)
TEditCreatureStackDlg::TEditCreatureStackDlg(CWnd* pParent, EGameVersion mapVersion,
                                             const std::bitset<kNumCreatureTypes>& abAllowedCreature)
    : CDialog(TEditCreatureStackDlg::IDD, pParent),
      _m_mapVersion(mapVersion),
      _m_abAllowedCreature(abAllowedCreature)
{
    unsigned int creature = 0;
    while (akCreatureTypeTraits[creature].level < 0 || !_m_abAllowedCreature[creature])
        creature++;
    _m_stack = TCreatureStack(TCreatureType(creature), 1);
}

VA_COMPGEN(0x0040dfcb, 0x1c, SCALAR_DELETING_DTOR, TEditCreatureStackDlg)

VA(0x0040dfe7, 0xbc)
TEditCreatureStackDlg::TEditCreatureStackDlg(CWnd* pParent, const TCreatureStack& stack, EGameVersion mapVersion,
                                             const std::bitset<kNumCreatureTypes>& abAllowedCreature)
    : CDialog(TEditCreatureStackDlg::IDD, pParent),
      _m_stack(stack),
      _m_mapVersion(mapVersion),
      _m_abAllowedCreature(abAllowedCreature)
{
    _m_abAllowedCreature.set(_m_stack.getCreatureType(), true);
}

VA(0x0040e0a3, 0x40)
void TEditCreatureStackDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_STACK_QUANTITY_SPIN, _m_quantitySpin);
    DDX_Control(pDX, IDC_STACK_QUANTITY_EDIT, _m_quantityEdit);
    DDX_Control(pDX, IDC_TYPE_COMBO, _m_typeCombo);
}

VA(0x0040e0e3, 0x6)
BEGIN_MESSAGE_MAP(TEditCreatureStackDlg, CDialog)
    ON_WM_CREATE()
    ON_EN_KILLFOCUS(IDC_STACK_QUANTITY_EDIT, OnKillFocusQuantityEdit)
END_MESSAGE_MAP()

VA(0x0040e0e9, 0x25)
int TEditCreatureStackDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialog::OnCreate(lpCreateStruct) == -1)
        return -1;
    SetWindowText(kEditCreatureStackCaptionStr);
    return 0;
}

VA(0x0040e10e, 0x1aa)
BOOL TEditCreatureStackDlg::OnInitDialog()
{
    GetDlgItem(IDOK)->SetWindowText(kOKStr);
    GetDlgItem(IDCANCEL)->SetWindowText(kCancelStr);
    GetDlgItem(ID_HELP)->SetWindowText(kHelpStr);
    GetDlgItem(IDC_TYPE_STATIC)->SetWindowText(SEditCreatureStackDlgText::kTypeStaticStr);
    GetDlgItem(IDC_QUANTITY_STATIC)->SetWindowText(SEditCreatureStackDlgText::kQuantityStaticStr);
    CDialog::OnInitDialog();
    unsigned int numCreatures = _m_mapVersion >= GAME_VERSION_AB ? CREATURE_CATAPULT : CREATURE_PIXIE;
    for (unsigned int creature = 0; creature < numCreatures; creature++) {
        if (akCreatureTypeTraits[creature].level >= 0 && _m_abAllowedCreature[creature])
            insertCreatureType(_m_typeCombo, TCreatureType(creature));
    }
    int index = 0;
    while (_m_typeCombo.GetItemData(index) != _m_stack.getCreatureType())
        index++;
    _m_typeCombo.SetCurSel(index);
    _m_quantityEdit.LimitText(TDigits<TCreatureStack::s_kMaxQuantity>::getDigits());
    CString text;
    text.Format("%d", _m_stack.getQuantity());
    _m_quantityEdit.SetWindowText(text);
    _m_quantitySpin.SetRange(1, TCreatureStack::s_kMaxQuantity);
    return TRUE;
}

VA(0x0040e2b8, 0xbf)
void TEditCreatureStackDlg::OnOK()
{
    CDialog::OnOK();
    TCreatureType creature = TCreatureType(_m_typeCombo.GetItemData(_m_typeCombo.GetCurSel()));
    int quantity = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &quantity);
    quantity = clamp(1, quantity, int(TCreatureStack::s_kMaxQuantity));
    _m_stack = TCreatureStack(creature, quantity);
}

VA(0x0040e377, 0xa8)
void TEditCreatureStackDlg::OnKillFocusQuantityEdit()
{
    int quantity = 0;
    CString text;
    _m_quantityEdit.GetWindowText(text);
    sscanf(text, "%d", &quantity);
    if (quantity <= 0 || quantity > TCreatureStack::s_kMaxQuantity) {
        quantity = clamp(1, quantity, int(TCreatureStack::s_kMaxQuantity));
        text.Format("%d", quantity);
        _m_quantityEdit.SetWindowText(text);
    }
}
