// ArmyDlg.cpp - Loki h3maped object 88: the seven creature stack editor.
// Each slot is a creature type combo (the "none" row, then every creature
// type) and a quantity entry with its spin button. The editor works on a
// copy of the caller's army and reports occupied stack count changes to its
// client. While it updates the widgets it blocks the prefixed signal
// handlers it finds with dlsym. The assert lines come from the retail
// immediates.
#include "editor/stdafx.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

namespace {
#include <gtk/gtk.h>
}

#include "creaturetype.h"
#include "editor/ArmyDlg.h"
#include "editor/cppbridge.h"
#include "editor/Digits.h"
#include "editor/MapEditorText.h"

GtkWidget* TArmyDlg::army_widget(char* name)
{
    char widgetName[strlen(name) + 30];
    sprintf(widgetName, "%s%s", _m_prefix.c_str(), name);
    return _widget(widgetName);
}

namespace {

bool compareCreatureType(TCreatureType a, TCreatureType b)
{
    TCreatureTypeTraits traitsA = akCreatureTypeTraits[a];
    TCreatureTypeTraits traitsB = akCreatureTypeTraits[b];
    TTownType townA = traitsA.townType != eTownNeutral ? traitsA.townType : kNumTownTypes;
    TTownType townB = traitsB.townType != eTownNeutral ? traitsB.townType : kNumTownTypes;
    if (townA != townB)
        return townA < townB;
    if (traitsA.level != traitsB.level)
        return traitsA.level < traitsB.level;
    return a < b;
}

void addCreatureTypeToComboBox(int* x, GtkCombo* pCombo, TCreatureType type)
{
#line 91
    assert(pCombo != NULL);
    assert(type >= 0 && type < kNumCreatureTypes);
    int index = g_list_length(GTK_LIST(pCombo->list)->children);
#line 112
    assert(index >= 0);
    GtkWidget* item = gtk_list_item_new_with_label(akCreatureTypeTraits[type].m_name);
    gtk_widget_show(item);
    GList* items = g_list_append(NULL, item);
    gtk_list_append_items(GTK_LIST(pCombo->list), items);
    x[index] = type;
}

int findCreatureIndex(int* x, GtkCombo* pCombo, TCreatureType type)
{
#line 125
    assert(pCombo != NULL);
    assert(type >= eCreatureNone && type < kNumCreatureTypes);
    int cbIndex;
    for (cbIndex = 0; x[cbIndex] != type; cbIndex++) {
#line 130
        assert(cbIndex < g_list_length(GTK_LIST(pCombo->list)->children));
    }
    return cbIndex;
}

}

TArmyDlg::TArmyDlg(const TArmy& army, char* prefix)
    : _m_pClient(this),
      _m_originalArmy(army),
      _m_bModified(false),
      _m_bEnabled(false),
      _m_prefix(prefix)
{
    for (int i = 0; i < _s_kNumCreatureStacks; i++) {
        char name[50];
        snprintf(name, 50, "creature_slot%d_type", i);
        _m_aStackData[i].m_typeCombo = GTK_COMBO(army_widget(name));
        snprintf(name, 50, "creature_slot%d_quantity", i);
        GtkSpinButton* pSpin = GTK_SPIN_BUTTON(army_widget(name));
        _m_aStackData[i].m_quantitySpin = pSpin;
        _m_aStackData[i].m_quantityEdit = GTK_ENTRY(pSpin);
        _m_aStackData[i].m_quantity = 0;
    }
    OnInitDialog();
}

TArmyDlg::TArmyDlg(TArmyDlgClient* pClient, const TArmy& army, char* prefix)
    : _m_pClient(pClient),
      _m_originalArmy(army),
      _m_bModified(false),
      _m_prefix(prefix)
{
#line 174
    assert(pClient != NULL);
    for (int i = 0; i < _s_kNumCreatureStacks; i++) {
        char name[50];
        snprintf(name, 50, "creature_slot%d_type", i);
        _m_aStackData[i].m_typeCombo = GTK_COMBO(army_widget(name));
        snprintf(name, 50, "creature_slot%d_quantity", i);
        GtkSpinButton* pSpin = GTK_SPIN_BUTTON(army_widget(name));
        _m_aStackData[i].m_quantitySpin = pSpin;
        _m_aStackData[i].m_quantityEdit = GTK_ENTRY(pSpin);
        _m_aStackData[i].m_quantity = 0;
    }
    OnInitDialog();
}

void TArmyDlg::_retrieveStackQuantities()
{
    for (unsigned int stackNum = 0; stackNum < _s_kNumCreatureStacks; stackNum++) {
        if (_m_aStackData[stackNum].m_creatureType != eCreatureNone) {
            int quantity = 0;
            gchar* text = gtk_entry_get_text(_m_aStackData[stackNum].m_quantityEdit);
            sscanf(text, "%d", &quantity);
            _m_aStackData[stackNum].m_quantity = quantity;
            if (_m_aStackData[stackNum].m_quantity == 0) {
                GtkCombo* typeCombo = _m_aStackData[stackNum].m_typeCombo;
                int* x = _m_aStackData[stackNum].m_typeItemData;
                int index = findCreatureIndex(x, typeCombo, eCreatureNone);
                setCurrentSelection(GTK_LIST(typeCombo->list), index);
                _onSelChangeTypeCombo(stackNum);
            }
        } else
            _m_aStackData[stackNum].m_quantity = 0;
    }
}

void TArmyDlg::_onSelChangeTypeCombo(unsigned int stackNum)
{
#line 259
    assert(stackNum < _s_kNumCreatureStacks);
    GtkCombo* typeCombo = _m_aStackData[stackNum].m_typeCombo;
    int* x = _m_aStackData[stackNum].m_typeItemData;
    int curSel = getCurrentSelection(GTK_LIST(typeCombo->list));
    TCreatureType newType = TCreatureType(x[curSel]);
    if (newType != _m_aStackData[stackNum].m_creatureType) {
        TCreatureType oldType = _m_aStackData[stackNum].m_creatureType;
        _m_aStackData[stackNum].m_creatureType = newType;
        if (newType != eCreatureNone) {
            GtkEntry* pEdit = _m_aStackData[stackNum].m_quantityEdit;
            if (oldType == eCreatureNone) {
                gtk_widget_set_sensitive(GTK_WIDGET(pEdit), TRUE);
                gtk_widget_set_sensitive(GTK_WIDGET(_m_aStackData[stackNum].m_quantitySpin), TRUE);
                gtk_entry_set_text(pEdit, "1");
                unsigned int oldNumOccupiedStacks = _m_numOccupiedStacks++;
                _m_pClient->onNumOccupiedStacksChanged(_m_numOccupiedStacks, oldNumOccupiedStacks);
            } else
                gtk_entry_set_text(pEdit, "1");
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(_m_aStackData[stackNum].m_quantitySpin), FALSE);
            GtkEntry* pEdit = _m_aStackData[stackNum].m_quantityEdit;
            gtk_entry_set_text(pEdit, "");
            gtk_widget_set_sensitive(GTK_WIDGET(pEdit), TRUE);
            unsigned int oldNumOccupiedStacks = _m_numOccupiedStacks--;
            _m_pClient->onNumOccupiedStacksChanged(_m_numOccupiedStacks, oldNumOccupiedStacks);
        }
    }
}

void TArmyDlg::_onKillFocusQtyEdit(unsigned int stackNum)
{
#line 313
    assert(stackNum < _s_kNumCreatureStacks);
    if (_m_aStackData[stackNum].m_creatureType != eCreatureNone) {
        int quantity = 0;
        gchar* text = gtk_entry_get_text(_m_aStackData[stackNum].m_quantityEdit);
        sscanf(text, "%d", &quantity);
        if (quantity == 0) {
            GtkCombo* typeCombo = _m_aStackData[stackNum].m_typeCombo;
            int* x = _m_aStackData[stackNum].m_typeItemData;
            int index = findCreatureIndex(x, typeCombo, eCreatureNone);
            setCurrentSelection(GTK_LIST(typeCombo->list), index);
            _onSelChangeTypeCombo(stackNum);
        }
    }
}

void* TArmyDlg::get_army_func(char* name)
{
    char funcName[strlen(name) + 5];
    void* retval = NULL;
    void* handle = dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
#line 366
    assert(handle != NULL);
    sprintf(funcName, "%s%s", _m_prefix.c_str(), name);
    retval = dlsym(handle, funcName);
#line 369
    assert(retval != NULL);
    dlclose(handle);
    return retval;
}

void TArmyDlg::disableArmyDlgSignals()
{
    GtkWidget* w;
    w = army_widget("creature_slot0_type_entry");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot0_type_entry_changed")), NULL);
    w = army_widget("creature_slot1_type_entry");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot1_type_entry_changed")), NULL);
    w = army_widget("creature_slot2_type_entry");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot2_type_entry_changed")), NULL);
    w = army_widget("creature_slot3_type_entry");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot3_type_entry_changed")), NULL);
    w = army_widget("creature_slot4_type_entry");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot4_type_entry_changed")), NULL);
    w = army_widget("creature_slot5_type_entry");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot5_type_entry_changed")), NULL);
    w = army_widget("creature_slot6_type_entry");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot6_type_entry_changed")), NULL);
    w = army_widget("creature_slot0_quantity");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot0_quantity_changed")), NULL);
    w = army_widget("creature_slot1_quantity");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot1_quantity_changed")), NULL);
    w = army_widget("creature_slot2_quantity");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot2_quantity_changed")), NULL);
    w = army_widget("creature_slot3_quantity");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot3_quantity_changed")), NULL);
    w = army_widget("creature_slot4_quantity");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot4_quantity_changed")), NULL);
    w = army_widget("creature_slot5_quantity");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot5_quantity_changed")), NULL);
    w = army_widget("creature_slot6_quantity");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot6_quantity_changed")), NULL);
}

void TArmyDlg::enableArmyDlgSignals()
{
    GtkWidget* w;
    w = army_widget("creature_slot0_type_entry");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot0_type_entry_changed")), NULL);
    w = army_widget("creature_slot1_type_entry");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot1_type_entry_changed")), NULL);
    w = army_widget("creature_slot2_type_entry");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot2_type_entry_changed")), NULL);
    w = army_widget("creature_slot3_type_entry");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot3_type_entry_changed")), NULL);
    w = army_widget("creature_slot4_type_entry");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot4_type_entry_changed")), NULL);
    w = army_widget("creature_slot5_type_entry");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot5_type_entry_changed")), NULL);
    w = army_widget("creature_slot6_type_entry");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot6_type_entry_changed")), NULL);
    w = army_widget("creature_slot0_quantity");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot0_quantity_changed")), NULL);
    w = army_widget("creature_slot1_quantity");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot1_quantity_changed")), NULL);
    w = army_widget("creature_slot2_quantity");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot2_quantity_changed")), NULL);
    w = army_widget("creature_slot3_quantity");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot3_quantity_changed")), NULL);
    w = army_widget("creature_slot4_quantity");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot4_quantity_changed")), NULL);
    w = army_widget("creature_slot5_quantity");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot5_quantity_changed")), NULL);
    w = army_widget("creature_slot6_quantity");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(get_army_func("on_creature_slot6_quantity_changed")), NULL);
}

BOOL TArmyDlg::OnInitDialog()
{
    _m_bModified = false;
    _m_army = _m_originalArmy;
    disableArmyDlgSignals();
    _m_numOccupiedStacks = 0;
    for (unsigned int stackNum = 0; stackNum < _s_kNumCreatureStacks; stackNum++) {
        _m_aStackData[stackNum].m_creatureType = _m_army[stackNum].getCreatureType();
        GtkCombo* typeCombo = _m_aStackData[stackNum].m_typeCombo;
        emptyList(GTK_LIST(typeCombo->list));
        int* x = _m_aStackData[stackNum].m_typeItemData;
        GList* items = NULL;
        unsigned int cbIndex = 0;
        items = g_list_append(items, (gpointer)kSelNoneStr);
#line 487
        assert(cbIndex == 0);
        x[0] = eCreatureNone;
        if (_m_army[stackNum].getCreatureType() == eCreatureNone)
            setCurrentSelection(GTK_LIST(typeCombo->list), cbIndex);
        gtk_combo_set_popdown_strings(typeCombo, items);
        for (unsigned int type = 0; type < kNumCreatureTypes; type++)
            addCreatureTypeToComboBox(x, typeCombo, TCreatureType(type));
        for (cbIndex = 0; x[cbIndex] != _m_army[stackNum].getCreatureType(); cbIndex++) {
#line 504
            assert(cbIndex < g_list_length(GTK_LIST(typeCombo->list)->children));
        }
        GtkEntry* pEdit = _m_aStackData[stackNum].m_quantityEdit;
        gtk_entry_set_max_length(pEdit, TDigits<TCreatureStack::s_kMaxQuantity>::getDigits());
        GtkSpinButton* pSpin = _m_aStackData[stackNum].m_quantitySpin;
        GtkAdjustment* adj = gtk_spin_button_get_adjustment(pSpin);
        adj->lower = 1;
        adj->upper = TCreatureStack::s_kMaxQuantity;
        gtk_adjustment_changed(adj);
        if (_m_army[stackNum].getCreatureType() != eCreatureNone) {
#line 525
            assert(_m_army[ stackNum ].getQuantity() > 0);
            _m_numOccupiedStacks++;
            char text[50];
            int quantity = _m_army[stackNum].getQuantity();
            adj->value = quantity;
            gtk_adjustment_value_changed(adj);
            snprintf(text, 50, "%d", quantity);
            gtk_entry_set_text(pEdit, text);
            _m_aStackData[stackNum].m_quantity = quantity;
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(pSpin), FALSE);
            gtk_entry_set_text(pEdit, "");
            gtk_widget_set_sensitive(GTK_WIDGET(pEdit), FALSE);
        }
    }
    enableArmyDlgSignals();
    OnEnable(true);
    return true;
}

void TArmyDlg::OnOK()
{
    if (IsWindowEnabled())
        _retrieveStackQuantities();
    for (unsigned int stackNum = 0; stackNum < _s_kNumCreatureStacks; stackNum++) {
#line 572
        assert(!IsWindowEnabled() || _m_aStackData[ stackNum ].m_creatureType == _m_aStackData[ stackNum ].m_typeItemData[ getCurrentSelection(GTK_LIST(_m_aStackData[ stackNum ].m_typeCombo->list)) ]);
#line 575
        assert(( _m_aStackData[ stackNum ].m_creatureType == eCreatureNone && _m_aStackData[ stackNum ].m_quantity == 0 ) || ( _m_aStackData[ stackNum ].m_creatureType != eCreatureNone && _m_aStackData[ stackNum ].m_quantity > 0 ));
        _m_army[stackNum].setCreatureType(_m_aStackData[stackNum].m_creatureType);
        _m_army[stackNum].setQuantity(_m_aStackData[stackNum].m_quantity);
    }
    _m_bModified = _m_bModified || _m_army != _m_originalArmy;
}

void TArmyDlg::OnEnable(bool bEnable)
{
    disableArmyDlgSignals();
    _m_bEnabled = bEnable;
    if (bEnable) {
        for (unsigned int stackNum = 0; stackNum < _s_kNumCreatureStacks; stackNum++) {
            GtkCombo* typeCombo = _m_aStackData[stackNum].m_typeCombo;
            int* x = _m_aStackData[stackNum].m_typeItemData;
            gtk_widget_set_sensitive(GTK_WIDGET(typeCombo), TRUE);
            setCurrentSelection(GTK_LIST(typeCombo->list),
                                findCreatureIndex(x, typeCombo, _m_aStackData[stackNum].m_creatureType));
            if (_m_aStackData[stackNum].m_creatureType != eCreatureNone) {
#line 611
                assert(_m_aStackData[ stackNum ].m_quantity > 0);
                GtkEntry* pEdit = _m_aStackData[stackNum].m_quantityEdit;
                char text[50];
                snprintf(text, 50, "%d", _m_aStackData[stackNum].m_quantity);
                gtk_entry_set_text(pEdit, text);
                gtk_widget_set_sensitive(GTK_WIDGET(pEdit), TRUE);
                gtk_widget_set_sensitive(GTK_WIDGET(_m_aStackData[stackNum].m_quantitySpin), TRUE);
            } else {
#line 628
                assert(_m_aStackData[ stackNum ].m_quantity == 0);
            }
        }
    } else {
        _retrieveStackQuantities();
        for (unsigned int stackNum = 0; stackNum < _s_kNumCreatureStacks; stackNum++) {
            GtkCombo* typeCombo = _m_aStackData[stackNum].m_typeCombo;
            int* x = _m_aStackData[stackNum].m_typeItemData;
            int curSel = getCurrentSelection(GTK_LIST(typeCombo->list));
#line 644
            assert(curSel != -1);
            assert(_m_aStackData[ stackNum ].m_creatureType == x[curSel]);
            if (_m_aStackData[stackNum].m_creatureType != eCreatureNone) {
#line 649
                assert(_m_aStackData[ stackNum ].m_quantity > 0);
                setCurrentSelection(GTK_LIST(typeCombo->list), findCreatureIndex(x, typeCombo, eCreatureNone));
                gtk_widget_set_sensitive(GTK_WIDGET(_m_aStackData[stackNum].m_quantitySpin), FALSE);
                GtkEntry* pEdit = _m_aStackData[stackNum].m_quantityEdit;
                gtk_entry_set_text(pEdit, "");
                gtk_widget_set_sensitive(GTK_WIDGET(pEdit), FALSE);
            } else {
#line 663
                assert(_m_aStackData[ stackNum ].m_quantity == 0);
            }
            gtk_widget_set_sensitive(GTK_WIDGET(typeCombo), FALSE);
        }
    }
    enableArmyDlgSignals();
}

void TArmyDlg::OnSelChangeTypeCombo1()
{
    _onSelChangeTypeCombo(0);
}

void TArmyDlg::OnSelChangeTypeCombo2()
{
    _onSelChangeTypeCombo(1);
}

void TArmyDlg::OnSelChangeTypeCombo3()
{
    _onSelChangeTypeCombo(2);
}

void TArmyDlg::OnSelChangeTypeCombo4()
{
    _onSelChangeTypeCombo(3);
}

void TArmyDlg::OnSelChangeTypeCombo5()
{
    _onSelChangeTypeCombo(4);
}

void TArmyDlg::OnSelChangeTypeCombo6()
{
    _onSelChangeTypeCombo(5);
}

void TArmyDlg::OnSelChangeTypeCombo7()
{
    _onSelChangeTypeCombo(6);
}

void TArmyDlg::OnKillFocusQtyEdit1()
{
    _onKillFocusQtyEdit(0);
}

void TArmyDlg::OnKillFocusQtyEdit2()
{
    _onKillFocusQtyEdit(1);
}

void TArmyDlg::OnKillFocusQtyEdit3()
{
    _onKillFocusQtyEdit(2);
}

void TArmyDlg::OnKillFocusQtyEdit4()
{
    _onKillFocusQtyEdit(3);
}

void TArmyDlg::OnKillFocusQtyEdit5()
{
    _onKillFocusQtyEdit(4);
}

void TArmyDlg::OnKillFocusQtyEdit6()
{
    _onKillFocusQtyEdit(5);
}

void TArmyDlg::OnKillFocusQtyEdit7()
{
    _onKillFocusQtyEdit(6);
}
