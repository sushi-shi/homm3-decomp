// ArmyDlg.cpp - the seven creature stack editor (h3maped 0x4025b1..0x4031a1;
// Loki h3maped object 88). The type combos list the version's creatures by
// town, then level, after the "none" row; maps from Armageddon's Blade on
// may offer the random creatures too. A stack whose quantity is cleared
// becomes empty.
#include "editor/stdafx.h"

#include <stdio.h>

#include "town_type.h"
#include "va.h"
#include "editor/ArmyDlg.h"
#include "editor/Digits.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"

DATA(0x0052ff18)
static const int g_aakStackControlIDs[7][3] = {
    { IDC_TYPE_COMBO1, IDC_QTY_EDIT1, IDC_QTY_SPIN1 },
    { IDC_TYPE_COMBO2, IDC_QTY_EDIT2, IDC_QTY_SPIN2 },
    { IDC_TYPE_COMBO3, IDC_QTY_EDIT3, IDC_QTY_SPIN3 },
    { IDC_TYPE_COMBO4, IDC_QTY_EDIT4, IDC_QTY_SPIN4 },
    { IDC_TYPE_COMBO5, IDC_QTY_EDIT5, IDC_QTY_SPIN5 },
    { IDC_TYPE_COMBO6, IDC_QTY_EDIT6, IDC_QTY_SPIN6 },
    { IDC_TYPE_COMBO7, IDC_QTY_EDIT7, IDC_QTY_SPIN7 }
};

// The creature lists' order: by town (the neutrals last), then level, then
// id.
static bool isCreatureBefore(TCreatureType lhs, TCreatureType rhs)
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

// Inserts a creature in the list's order after the "none" row, its item
// data the creature.
VA(0x004025b1, 0x78)
static void addCreature(CComboBox& combo, TCreatureType creature)
{
    int index = combo.GetCount();
    while (index > 0) {
        TCreatureType other = TCreatureType(combo.GetItemData(index - 1));
        if (other == CREATURE_NONE || !isCreatureBefore(creature, other))
            break;
        index--;
    }
    combo.InsertString(index, akCreatureTypeTraits[creature].m_name);
    combo.SetItemData(index, creature);
}

VA(0x00402629, 0x93)
TArmyDlg::TArmyDlg(const TArmy& army, EGameVersion mapVersion, bool bRandomCreatures)
    : CDialog(TArmyDlg::IDD),
      _m_pClient(this),
      _m_originalArmy(army),
      _m_mapVersion(mapVersion),
      _m_bRandomCreatures(bRandomCreatures),
      _m_bModified(false)
{
}

VA(0x004027cc, 0x8b)
TArmyDlg::TArmyDlg(TArmyDlgClient* pClient, const TArmy& army, EGameVersion mapVersion, bool bRandomCreatures)
    : CDialog(TArmyDlg::IDD),
      _m_pClient(pClient),
      _m_originalArmy(army),
      _m_mapVersion(mapVersion),
      _m_bRandomCreatures(bRandomCreatures),
      _m_bModified(false)
{
}

VA(0x00402857, 0x4c)
void TArmyDlg::DoDataExchange(CDataExchange* pDX)
{
    for (unsigned int stackNum = 0; stackNum < _s_kNumCreatureStacks; stackNum++) {
        DDX_Control(pDX, g_aakStackControlIDs[stackNum][0], _m_aStackData[stackNum].m_typeCombo);
        DDX_Control(pDX, g_aakStackControlIDs[stackNum][1], _m_aStackData[stackNum].m_quantityEdit);
        DDX_Control(pDX, g_aakStackControlIDs[stackNum][2], _m_aStackData[stackNum].m_quantitySpin);
    }
}

VA(0x004028a3, 0xd0)
void TArmyDlg::_retrieveStackQuantities()
{
    for (unsigned int stackNum = 0; stackNum < _s_kNumCreatureStacks; stackNum++) {
        _TStackData& stack = _m_aStackData[stackNum];
        if (stack.m_creatureType != CREATURE_NONE) {
            int quantity = 0;
            CString text;
            stack.m_quantityEdit.GetWindowText(text);
            sscanf(text, "%d", &quantity);
            stack.m_quantity = quantity;
            if (stack.m_quantity == 0) {
                int index;
                for (index = 0; stack.m_typeCombo.GetItemData(index) != CREATURE_NONE; index++)
                    ;
                stack.m_typeCombo.SetCurSel(index);
                _onSelChangeTypeCombo(stackNum);
            }
        } else
            stack.m_quantity = 0;
    }
}

VA(0x00402973, 0xce)
void TArmyDlg::_onSelChangeTypeCombo(unsigned int stackNum)
{
    TCreatureType newType = TCreatureType(
        _m_aStackData[stackNum].m_typeCombo.GetItemData(_m_aStackData[stackNum].m_typeCombo.GetCurSel()));
    TCreatureType oldType = _m_aStackData[stackNum].m_creatureType;
    if (newType != oldType) {
        _m_aStackData[stackNum].m_creatureType = newType;
        if (newType != CREATURE_NONE) {
            CEdit& quantityEdit = _m_aStackData[stackNum].m_quantityEdit;
            if (oldType == CREATURE_NONE) {
                quantityEdit.EnableWindow(TRUE);
                _m_aStackData[stackNum].m_quantitySpin.EnableWindow(TRUE);
                quantityEdit.SetWindowText("1");
                unsigned int oldNumOccupiedStacks = _m_numOccupiedStacks++;
                _m_pClient->onNumOccupiedStacksChanged(_m_numOccupiedStacks, oldNumOccupiedStacks);
            } else
                quantityEdit.SetWindowText("1");
        } else {
            _m_aStackData[stackNum].m_quantitySpin.EnableWindow(FALSE);
            _m_aStackData[stackNum].m_quantityEdit.SetWindowText("");
            _m_aStackData[stackNum].m_quantityEdit.EnableWindow(FALSE);
            unsigned int oldNumOccupiedStacks = _m_numOccupiedStacks--;
            _m_pClient->onNumOccupiedStacksChanged(_m_numOccupiedStacks, oldNumOccupiedStacks);
        }
    }
}

VA(0x00402a41, 0xcb)
void TArmyDlg::_onKillFocusQtyEdit(unsigned int stackNum)
{
    if (_m_aStackData[stackNum].m_creatureType != CREATURE_NONE) {
        int quantity = 0;
        CString text;
        _m_aStackData[stackNum].m_quantityEdit.GetWindowText(text);
        sscanf(text, "%d", &quantity);
        if (quantity == 0) {
            int index;
            for (index = 0; _m_aStackData[stackNum].m_typeCombo.GetItemData(index) != CREATURE_NONE; index++)
                ;
            _m_aStackData[stackNum].m_typeCombo.SetCurSel(index);
            _onSelChangeTypeCombo(stackNum);
        }
    }
}

VA(0x00402b0c, 0x6)
BEGIN_MESSAGE_MAP(TArmyDlg, CDialog)
    ON_CBN_SELCHANGE(IDC_TYPE_COMBO1, OnSelChangeTypeCombo1)
    ON_CBN_SELCHANGE(IDC_TYPE_COMBO2, OnSelChangeTypeCombo2)
    ON_CBN_SELCHANGE(IDC_TYPE_COMBO3, OnSelChangeTypeCombo3)
    ON_CBN_SELCHANGE(IDC_TYPE_COMBO4, OnSelChangeTypeCombo4)
    ON_CBN_SELCHANGE(IDC_TYPE_COMBO5, OnSelChangeTypeCombo5)
    ON_CBN_SELCHANGE(IDC_TYPE_COMBO6, OnSelChangeTypeCombo6)
    ON_CBN_SELCHANGE(IDC_TYPE_COMBO7, OnSelChangeTypeCombo7)
    ON_EN_KILLFOCUS(IDC_QTY_EDIT1, OnKillFocusQtyEdit1)
    ON_EN_KILLFOCUS(IDC_QTY_EDIT2, OnKillFocusQtyEdit2)
    ON_EN_KILLFOCUS(IDC_QTY_EDIT3, OnKillFocusQtyEdit3)
    ON_EN_KILLFOCUS(IDC_QTY_EDIT4, OnKillFocusQtyEdit4)
    ON_EN_KILLFOCUS(IDC_QTY_EDIT5, OnKillFocusQtyEdit5)
    ON_EN_KILLFOCUS(IDC_QTY_EDIT6, OnKillFocusQtyEdit6)
    ON_EN_KILLFOCUS(IDC_QTY_EDIT7, OnKillFocusQtyEdit7)
    ON_WM_ENABLE()
END_MESSAGE_MAP()

VA(0x00402b12, 0x3d5)
BOOL TArmyDlg::OnInitDialog()
{
    GetDlgItem(IDC_TYPE_STATIC)->SetWindowText(SArmyDlgText::kTypeStaticStr);
    GetDlgItem(IDC_QUANTITY_STATIC)->SetWindowText(SArmyDlgText::kQuantityStaticStr);
    GetDlgItem(IDC_SLOT1_STATIC)->SetWindowText(TFormattedString(SArmyDlgText::kSlotStaticFmtStr, 1));
    GetDlgItem(IDC_SLOT2_STATIC)->SetWindowText(TFormattedString(SArmyDlgText::kSlotStaticFmtStr, 2));
    GetDlgItem(IDC_SLOT3_STATIC)->SetWindowText(TFormattedString(SArmyDlgText::kSlotStaticFmtStr, 3));
    GetDlgItem(IDC_SLOT4_STATIC)->SetWindowText(TFormattedString(SArmyDlgText::kSlotStaticFmtStr, 4));
    GetDlgItem(IDC_SLOT5_STATIC)->SetWindowText(TFormattedString(SArmyDlgText::kSlotStaticFmtStr, 5));
    GetDlgItem(IDC_SLOT6_STATIC)->SetWindowText(TFormattedString(SArmyDlgText::kSlotStaticFmtStr, 6));
    GetDlgItem(IDC_SLOT7_STATIC)->SetWindowText(TFormattedString(SArmyDlgText::kSlotStaticFmtStr, 7));
    _m_bModified = false;
    _m_army = _m_originalArmy;
    CDialog::OnInitDialog();
    _m_numOccupiedStacks = 0;
    for (unsigned int stackNum = 0; stackNum < _s_kNumCreatureStacks; stackNum++) {
        _TStackData& stack = _m_aStackData[stackNum];
        stack.m_creatureType = _m_army[stackNum].getCreatureType();
        int index = stack.m_typeCombo.AddString(kSelNoneStr);
        stack.m_typeCombo.SetItemData(index, CREATURE_NONE);
        if (_m_army[stackNum].getCreatureType() == CREATURE_NONE)
            stack.m_typeCombo.SetCurSel(index);
        unsigned int numCreatures = _m_mapVersion >= GAME_VERSION_AB ? CREATURE_CATAPULT : CREATURE_PIXIE;
        for (unsigned int creature = 0; creature < numCreatures; creature++) {
            if (akCreatureTypeTraits[creature].level >= 0)
                addCreature(stack.m_typeCombo, TCreatureType(creature));
        }
        if (_m_mapVersion >= GAME_VERSION_AB && _m_bRandomCreatures) {
            for (unsigned int random = 0; random < kNumRandomCreatureTypes; random++) {
                index = stack.m_typeCombo.AddString(akRandomCreatureTraits[random].m_name);
                stack.m_typeCombo.SetItemData(index, akRandomCreatureTraits[random].m_creatureType);
            }
        }
        for (index = 0; stack.m_typeCombo.GetItemData(index) != _m_army[stackNum].getCreatureType(); index++)
            ;
        stack.m_typeCombo.SetCurSel(index);
        stack.m_quantityEdit.LimitText(TDigits<TCreatureStack::s_kMaxQuantity>::getDigits());
        stack.m_quantitySpin.SetRange(1, TCreatureStack::s_kMaxQuantity);
        if (_m_army[stackNum].getCreatureType() != CREATURE_NONE) {
            _m_numOccupiedStacks++;
            CString text;
            text.Format("%d", _m_army[stackNum].getQuantity());
            stack.m_quantityEdit.SetWindowText(text);
        } else {
            stack.m_quantitySpin.EnableWindow(FALSE);
            stack.m_quantityEdit.SetWindowText("");
            stack.m_quantityEdit.EnableWindow(FALSE);
        }
    }
    return TRUE;
}

VA(0x00402f1d, 0x76)
void TArmyDlg::OnOK()
{
    if (IsWindowEnabled())
        _retrieveStackQuantities();
    for (unsigned int stackNum = 0; stackNum < _s_kNumCreatureStacks; stackNum++) {
        _m_army[stackNum].setCreatureType(_m_aStackData[stackNum].m_creatureType);
        _m_army[stackNum].setQuantity(_m_aStackData[stackNum].m_quantity);
    }
    _m_bModified = _m_bModified || !(_m_army == _m_originalArmy);
}

VA(0x00402f93, 0x16d)
void TArmyDlg::OnEnable(BOOL bEnable)
{
    CDialog::OnEnable(bEnable);
    unsigned int stackNum;
    if (bEnable) {
        for (stackNum = 0; stackNum < _s_kNumCreatureStacks; stackNum++) {
            _TStackData& stack = _m_aStackData[stackNum];
            stack.m_typeCombo.EnableWindow(TRUE);
            TCreatureType creatureType = stack.m_creatureType;
            int index;
            for (index = 0; stack.m_typeCombo.GetItemData(index) != creatureType; index++)
                ;
            stack.m_typeCombo.SetCurSel(index);
            if (stack.m_creatureType != CREATURE_NONE) {
                CString text;
                text.Format("%d", stack.m_quantity);
                stack.m_quantityEdit.SetWindowText(text);
                stack.m_quantityEdit.EnableWindow(TRUE);
                stack.m_quantitySpin.EnableWindow(TRUE);
            }
        }
    } else {
        _retrieveStackQuantities();
        for (stackNum = 0; stackNum < _s_kNumCreatureStacks; stackNum++) {
            _TStackData& stack = _m_aStackData[stackNum];
            if (stack.m_creatureType != CREATURE_NONE) {
                int index;
                for (index = 0; stack.m_typeCombo.GetItemData(index) != CREATURE_NONE; index++)
                    ;
                stack.m_typeCombo.SetCurSel(index);
                stack.m_quantitySpin.EnableWindow(FALSE);
                stack.m_quantityEdit.SetWindowText("");
                stack.m_quantityEdit.EnableWindow(FALSE);
            }
            stack.m_typeCombo.EnableWindow(FALSE);
        }
    }
}

VA(0x00403100, 0x8)
void TArmyDlg::OnSelChangeTypeCombo1()
{
    _onSelChangeTypeCombo(0);
}

VA(0x00403108, 0x8)
void TArmyDlg::OnSelChangeTypeCombo2()
{
    _onSelChangeTypeCombo(1);
}

VA(0x00403110, 0x8)
void TArmyDlg::OnSelChangeTypeCombo3()
{
    _onSelChangeTypeCombo(2);
}

VA(0x00403118, 0x8)
void TArmyDlg::OnSelChangeTypeCombo4()
{
    _onSelChangeTypeCombo(3);
}

VA(0x00403120, 0x8)
void TArmyDlg::OnSelChangeTypeCombo5()
{
    _onSelChangeTypeCombo(4);
}

VA(0x00403128, 0x8)
void TArmyDlg::OnSelChangeTypeCombo6()
{
    _onSelChangeTypeCombo(5);
}

VA(0x00403130, 0x8)
void TArmyDlg::OnSelChangeTypeCombo7()
{
    _onSelChangeTypeCombo(6);
}

VA(0x00403138, 0x8)
void TArmyDlg::OnKillFocusQtyEdit1()
{
    _onKillFocusQtyEdit(0);
}

VA(0x00403140, 0x8)
void TArmyDlg::OnKillFocusQtyEdit2()
{
    _onKillFocusQtyEdit(1);
}

VA(0x00403148, 0x8)
void TArmyDlg::OnKillFocusQtyEdit3()
{
    _onKillFocusQtyEdit(2);
}

VA(0x00403150, 0x8)
void TArmyDlg::OnKillFocusQtyEdit4()
{
    _onKillFocusQtyEdit(3);
}

VA(0x00403158, 0x8)
void TArmyDlg::OnKillFocusQtyEdit5()
{
    _onKillFocusQtyEdit(4);
}

VA(0x00403160, 0x8)
void TArmyDlg::OnKillFocusQtyEdit6()
{
    _onKillFocusQtyEdit(5);
}

VA(0x00403168, 0x8)
void TArmyDlg::OnKillFocusQtyEdit7()
{
    _onKillFocusQtyEdit(6);
}
