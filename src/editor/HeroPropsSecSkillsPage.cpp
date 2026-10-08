// HeroPropsSecSkillsPage.cpp - Loki h3maped object 87: the hero sheet's
// secondary skills page. Each slot's skill combo lists "none" and every
// skill no other slot holds; its mastery combo lists basic to expert. The
// page edits a copy of the hero's custom skills (or shows the prototype's
// while not customized). The assert lines come from the retail immediates.
#include "editor/stdafx.h"

namespace {
#include <gtk/gtk.h>
}

#include "editor/cppbridge.h"
#include "editor/HeroPropsSecSkillsPage.h"
#include "editor/MapEditorText.h"

namespace {

int findSkillIndex(int* x, GtkCombo* pCombo, TSecondarySkill skill)
{
#line 59
    assert(pCombo != NULL);
    assert(skill >= 0 && skill <= kNumSecSkills);
    int cbIndex;
    for (cbIndex = 0; x[cbIndex] != skill; cbIndex++) {
#line 64
        assert(cbIndex < g_list_length(GTK_LIST(pCombo->list)->children));
    }
    return cbIndex;
}

}

THeroPropsSecSkillsPage::TSlotData THeroPropsSecSkillsPage::_s_akSlotData[_s_kNumSkillSlots] = {
    { NULL, NULL, &THeroPropsSecSkillsPage::_m_mastery0 },
    { NULL, NULL, &THeroPropsSecSkillsPage::_m_mastery1 },
    { NULL, NULL, &THeroPropsSecSkillsPage::_m_mastery2 },
    { NULL, NULL, &THeroPropsSecSkillsPage::_m_mastery3 },
    { NULL, NULL, &THeroPropsSecSkillsPage::_m_mastery4 },
    { NULL, NULL, &THeroPropsSecSkillsPage::_m_mastery5 },
    { NULL, NULL, &THeroPropsSecSkillsPage::_m_mastery6 },
    { NULL, NULL, &THeroPropsSecSkillsPage::_m_mastery7 }
};

THeroPropsSecSkillsPage::_TSkillArray::_TSkillArray()
    : TArray<TSkillData, _s_kNumSkillSlots>(TSkillData(kNumSecSkills, eMasteryBasic))
{
}

THeroPropsSecSkillsPage::_TSkillArray::_TSkillArray(const map<TSecondarySkill, TSkillMastery>& skillSet)
    : TArray<TSkillData, _s_kNumSkillSlots>(TSkillData(kNumSecSkills, eMasteryBasic))
{
    unsigned int slot = 0;
    for (map<TSecondarySkill, TSkillMastery>::const_iterator iter = skillSet.begin(); iter != skillSet.end();
         ++iter) {
#line 88
        assert(slot < size());
        (*this)[slot] = TSkillData(iter->first, iter->second);
        slot++;
    }
}

THeroPropsSecSkillsPage::_TSkillArray::operator map<TSecondarySkill, TSkillMastery>() const
{
    map<TSecondarySkill, TSkillMastery> skillSet;
    for (unsigned int slot = 0; slot < size(); slot++) {
        if ((*this)[slot].m_skill != kNumSecSkills) {
#line 103
            assert(skillSet.find( ( *this )[ slot ].m_skill ) == skillSet.end());
            skillSet[(*this)[slot].m_skill] = (*this)[slot].m_mastery;
        }
    }
    return skillSet;
}

void THeroPropsSecSkillsPage::UpdateData(bool bSaveAndValidate)
{
    _m_mastery0 = (TSkillMastery)getCurrentSelection(GTK_LIST(GTK_COMBO(_widget("hero_secskills_mastery_0"))->list));
    _m_mastery1 = (TSkillMastery)getCurrentSelection(GTK_LIST(GTK_COMBO(_widget("hero_secskills_mastery_1"))->list));
    _m_mastery2 = (TSkillMastery)getCurrentSelection(GTK_LIST(GTK_COMBO(_widget("hero_secskills_mastery_2"))->list));
    _m_mastery3 = (TSkillMastery)getCurrentSelection(GTK_LIST(GTK_COMBO(_widget("hero_secskills_mastery_3"))->list));
    _m_mastery4 = (TSkillMastery)getCurrentSelection(GTK_LIST(GTK_COMBO(_widget("hero_secskills_mastery_4"))->list));
    _m_mastery5 = (TSkillMastery)getCurrentSelection(GTK_LIST(GTK_COMBO(_widget("hero_secskills_mastery_5"))->list));
    _m_mastery6 = (TSkillMastery)getCurrentSelection(GTK_LIST(GTK_COMBO(_widget("hero_secskills_mastery_6"))->list));
    _m_mastery7 = (TSkillMastery)getCurrentSelection(GTK_LIST(GTK_COMBO(_widget("hero_secskills_mastery_7"))->list));
    _m_bCustomize = isChecked("hero_secskills_customize");
}

THeroPropsSecSkillsPage::THeroPropsSecSkillsPage(const THero& hero)
    : _m_hero(hero),
      _m_pPrototype(NULL),
      _m_bModified(false)
{
    _m_mastery0 = -1;
    _m_mastery1 = -1;
    _m_mastery2 = -1;
    _m_mastery3 = -1;
    _m_mastery4 = -1;
    _m_mastery5 = -1;
    _m_mastery6 = -1;
    _m_mastery7 = -1;
    _m_bCustomize = false;
    for (int i = 0; i < _s_kNumSkillSlots; i++) {
        char name[150];
        snprintf(name, 150, "hero_secskills_type_%d", i);
        _s_akSlotData[i].skillCombo = GTK_COMBO(_widget(name));
        snprintf(name, 150, "hero_secskills_mastery_%d", i);
        _s_akSlotData[i].masteryCombo = GTK_COMBO(_widget(name));
    }
}

THeroPropsSecSkillsPage::~THeroPropsSecSkillsPage()
{
}

void THeroPropsSecSkillsPage::setIdentity(THeroClass newHeroClass, unsigned int newProtoNum)
{
#line 194
    assert(newHeroClass >= 0 && newHeroClass < kNumHeroClasses);
    assert(newProtoNum < THero::s_akClassTraits[ newHeroClass ].m_numPrototypes);
    _m_heroClass = newHeroClass;
    _m_protoNum = newProtoNum;
    _m_pPrototype = &THero::s_akClassTraits[_m_heroClass].m_aPrototype[_m_protoNum];
    if (!_m_bCustomize)
        _setSlotControlData(_TSkillArray(_m_pPrototype->getSecondarySkills()));
}

void THeroPropsSecSkillsPage::_setSlotControlData(const _TSkillArray& skills)
{
    int slot;
    for (slot = 0; slot < _s_kNumSkillSlots; slot++) {
        if (_m_aSlotSkill[slot] < kNumSecSkills) {
            GtkCombo* pCombo = _getPSkillCombo(slot);
            setCurrentSelection(GTK_LIST(pCombo->list),
                                findSkillIndex(_s_akSlotData[slot].skillItemData, pCombo, kNumSecSkills));
            _setSlotSkill(slot, kNumSecSkills);
        }
    }
    for (slot = 0; slot < _s_kNumSkillSlots; slot++) {
        if (skills[slot].m_skill < kNumSecSkills) {
            GtkCombo* pCombo = _getPSkillCombo(slot);
            setCurrentSelection(GTK_LIST(pCombo->list),
                                findSkillIndex(_s_akSlotData[slot].skillItemData, pCombo, skills[slot].m_skill));
            _setSlotSkill(slot, skills[slot].m_skill);
            *_getPMastery(slot) = skills[slot].m_mastery - 1;
            setCurrentSelection(GTK_LIST(_getPMasteryCombo(slot)->list), *_getPMastery(slot));
        }
    }
    UpdateData(false);
}

void THeroPropsSecSkillsPage::_retrieveSlotControlData(_TSkillArray* pSkills)
{
    _TSkillArray& skills = *pSkills;
    for (int slot = 0; slot < _s_kNumSkillSlots; slot++) {
        int curSel = getCurrentSelection(GTK_LIST(_getPSkillCombo(slot)->list));
        skills[slot].m_skill = _m_aSlotSkill[slot];
        skills[slot].m_mastery = TSkillMastery(getCurrentSelection(GTK_LIST(_getPMasteryCombo(slot)->list)) + 1);
    }
}

static void disableSecSkillsSignals()
{
    GtkWidget* w;
    w = _widget("hero_secskills_type_entry_0");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_0_changed), NULL);
    w = _widget("hero_secskills_type_entry_1");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_1_changed), NULL);
    w = _widget("hero_secskills_type_entry_2");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_2_changed), NULL);
    w = _widget("hero_secskills_type_entry_3");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_3_changed), NULL);
    w = _widget("hero_secskills_type_entry_4");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_4_changed), NULL);
    w = _widget("hero_secskills_type_entry_5");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_5_changed), NULL);
    w = _widget("hero_secskills_type_entry_6");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_6_changed), NULL);
    w = _widget("hero_secskills_type_entry_7");
    gtk_signal_handler_block_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_7_changed), NULL);
}

static void enableSecSkillsSignals()
{
    GtkWidget* w;
    w = _widget("hero_secskills_type_entry_0");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_0_changed), NULL);
    w = _widget("hero_secskills_type_entry_1");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_1_changed), NULL);
    w = _widget("hero_secskills_type_entry_2");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_2_changed), NULL);
    w = _widget("hero_secskills_type_entry_3");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_3_changed), NULL);
    w = _widget("hero_secskills_type_entry_4");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_4_changed), NULL);
    w = _widget("hero_secskills_type_entry_5");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_5_changed), NULL);
    w = _widget("hero_secskills_type_entry_6");
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(w), GTK_SIGNAL_FUNC(on_hero_secskills_type_entry_6_changed), NULL);
    w = _widget("hero_secskills_type_entry_7");
}

void THeroPropsSecSkillsPage::_enableSlotControls()
{
    disableSecSkillsSignals();
    for (unsigned int slot = 0; slot < _s_kNumSkillSlots; slot++) {
        int curSel = getCurrentSelection(GTK_LIST(_getPSkillCombo(slot)->list));
#line 384
        assert(_m_aSlotSkill[ slot ] == _s_akSlotData[ slot ].skillItemData[curSel]);
        gtk_widget_set_sensitive(GTK_WIDGET(_getPSkillCombo(slot)), TRUE);
        if (_m_aSlotSkill[slot] < kNumSecSkills)
            gtk_widget_set_sensitive(GTK_WIDGET(_getPSkillCombo(slot)), TRUE);
    }
    enableSecSkillsSignals();
}

void THeroPropsSecSkillsPage::_disableSlotControls()
{
    disableSecSkillsSignals();
    for (unsigned int slot = 0; slot < _s_kNumSkillSlots; slot++) {
        int curSel = getCurrentSelection(GTK_LIST(_getPSkillCombo(slot)->list));
#line 405
        assert(_m_aSlotSkill[ slot ] == _s_akSlotData[ slot ].skillItemData[curSel]);
        gtk_widget_set_sensitive(GTK_WIDGET(_getPSkillCombo(slot)), FALSE);
        if (_m_aSlotSkill[slot] >= kNumSecSkills)
            gtk_widget_set_sensitive(GTK_WIDGET(_getPMasteryCombo(slot)), FALSE);
    }
    enableSecSkillsSignals();
}

void THeroPropsSecSkillsPage::_setSlotSkill(unsigned int slot, TSecondarySkill newSkill)
{
#line 421
    assert(slot < _s_kNumSkillSlots);
    assert(newSkill >= 0 && newSkill <= kNumSecSkills);
    TSecondarySkill oldSkill = _m_aSlotSkill[slot];
    if (newSkill == oldSkill)
        return;
    if (newSkill < kNumSecSkills) {
        if (_m_bCustomize)
            gtk_widget_set_sensitive(GTK_WIDGET(_getPMasteryCombo(slot)), TRUE);
        *_getPMastery(slot) = 0;
        for (unsigned int otherSlot = 0; otherSlot < _s_kNumSkillSlots; otherSlot++) {
            if (otherSlot == slot)
                continue;
            GtkCombo* pCombo = _getPSkillCombo(otherSlot);
            int index = findSkillIndex(_s_akSlotData[otherSlot].skillItemData, pCombo, newSkill);
            gtk_list_clear_items(GTK_LIST(pCombo->list), index, index + 1);
        }
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(_getPMasteryCombo(slot)), FALSE);
        *_getPMastery(slot) = -1;
    }
    if (oldSkill < kNumSecSkills) {
        for (unsigned int otherSlot = 0; otherSlot < _s_kNumSkillSlots; otherSlot++) {
            if (otherSlot == slot)
                continue;
            GtkCombo* pCombo = _getPSkillCombo(otherSlot);
            int index = g_list_length(GTK_LIST(pCombo->list)->children);
            while (index > 1) {
                if (oldSkill >= _s_akSlotData[otherSlot].skillItemData[index - 1])
                    break;
                index--;
            }
            GtkWidget* item = gtk_list_item_new_with_label(THero::s_akSecondarySkillTraits[oldSkill].m_name);
            gtk_widget_show(item);
            GList* items = g_list_append(NULL, item);
            gtk_list_insert_items(GTK_LIST(pCombo->list), items, index);
            _s_akSlotData[otherSlot].skillItemData[index] = oldSkill;
        }
    }
    _m_aSlotSkill[slot] = newSkill;
}

void THeroPropsSecSkillsPage::_onSelChangeSkillCombo(unsigned int slot)
{
#line 493
    assert(slot < _s_kNumSkillSlots);
    GtkCombo* pCombo = _getPSkillCombo(slot);
    int curSel = getCurrentSelection(GTK_LIST(pCombo->list));
#line 499
    assert(curSel != -1);
    TSecondarySkill newSkill = TSecondarySkill(_s_akSlotData[slot].skillItemData[curSel]);
#line 501
    assert(newSkill >= 0 && newSkill <= kNumSecSkills);
    if (newSkill != _m_aSlotSkill[slot]) {
        UpdateData(false);
        _setSlotSkill(slot, newSkill);
        UpdateData(false);
    }
}

void THeroPropsSecSkillsPage::OnOK()
{
    if (_m_bCustomize)
        _retrieveSlotControlData(&_m_skillArray);
    _m_bCustomSecondarySkills = _m_bCustomize;
    _m_secondarySkills = _m_skillArray;
    _m_bModified = _m_bModified || _m_bCustomSecondarySkills != _m_hero.getBCustomSecondarySkills()
                   || _m_secondarySkills != _m_hero.getCustomSecondarySkills();
}

BOOL THeroPropsSecSkillsPage::OnInitDialog()
{
    _m_bModified = false;
    _m_bCustomSecondarySkills = _m_hero.getBCustomSecondarySkills();
    _m_secondarySkills = _m_hero.getCustomSecondarySkills();
    _m_heroClass = _m_hero.getClass();
    _m_protoNum = _m_hero.getProtoNum();
    _m_pPrototype = &THero::s_akClassTraits[_m_heroClass].m_aPrototype[_m_protoNum];
    _m_bCustomize = _m_bCustomSecondarySkills;
    _m_skillArray = _TSkillArray(_m_secondarySkills);
    unsigned int slot;
    for (slot = 0; slot < _s_kNumSkillSlots; slot++)
        *_getPMastery(slot) = -1;
    for (slot = 0; slot < _s_kNumSkillSlots; slot++) {
        int item = 0;
        GList* items = NULL;
        GtkCombo* pMasteryCombo = _getPMasteryCombo(slot);
        emptyList(GTK_LIST(pMasteryCombo->list));
        for (unsigned int mastery = 0; mastery < kNumMasteries - 1; mastery++)
            items = g_list_append(items, (gpointer)THero::s_akSkillMasteryTraits[mastery].m_name);
        gtk_combo_set_popdown_strings(pMasteryCombo, items);
        gtk_widget_set_sensitive(GTK_WIDGET(pMasteryCombo), FALSE);
        GtkCombo* pSkillCombo = _getPSkillCombo(slot);
        emptyList(GTK_LIST(pSkillCombo->list));
        items = NULL;
        items = g_list_append(items, (gpointer)kSelNoneStr);
        _s_akSlotData[slot].skillItemData[0] = kNumSecSkills;
        setCurrentSelection(GTK_LIST(pSkillCombo->list), 0);
        item = 1;
        for (unsigned int skill = 0; skill < kNumSecSkills; skill++) {
            items = g_list_append(items, (gpointer)THero::s_akSecondarySkillTraits[skill].m_name);
            _s_akSlotData[slot].skillItemData[item] = skill;
            item++;
        }
        gtk_combo_set_popdown_strings(pSkillCombo, items);
    }
    fill_n(_m_aSlotSkill, sizeof(_m_aSlotSkill) / sizeof(_m_aSlotSkill[0]), kNumSecSkills);
    GtkToggleButton* pButton = GTK_TOGGLE_BUTTON(_widget("hero_secskills_customize"));
    gtk_toggle_button_set_active(pButton, _m_bCustomize);
    if (_m_bCustomize)
        _setSlotControlData(_m_skillArray);
    else
        _setSlotControlData(_TSkillArray(_m_pPrototype->getSecondarySkills()));
    if (!_m_bCustomize)
        _disableSlotControls();
    return true;
}

void THeroPropsSecSkillsPage::OnCustomizeCheck()
{
    bool bWasCustomized = _m_bCustomize;
    UpdateData(false);
    if (_m_bCustomize) {
        if (!bWasCustomized) {
            _enableSlotControls();
            _setSlotControlData(_m_skillArray);
        }
    } else if (bWasCustomized) {
        _retrieveSlotControlData(&_m_skillArray);
        _setSlotControlData(_TSkillArray(_m_pPrototype->getSecondarySkills()));
        _disableSlotControls();
    }
}

void THeroPropsSecSkillsPage::OnSelChangeSkill1Combo()
{
    _onSelChangeSkillCombo(0);
}

void THeroPropsSecSkillsPage::OnSelChangeSkill2Combo()
{
    _onSelChangeSkillCombo(1);
}

void THeroPropsSecSkillsPage::OnSelChangeSkill3Combo()
{
    _onSelChangeSkillCombo(2);
}

void THeroPropsSecSkillsPage::OnSelChangeSkill4Combo()
{
    _onSelChangeSkillCombo(3);
}

void THeroPropsSecSkillsPage::OnSelChangeSkill5Combo()
{
    _onSelChangeSkillCombo(4);
}

void THeroPropsSecSkillsPage::OnSelChangeSkill6Combo()
{
    _onSelChangeSkillCombo(5);
}

void THeroPropsSecSkillsPage::OnSelChangeSkill7Combo()
{
    _onSelChangeSkillCombo(6);
}

void THeroPropsSecSkillsPage::OnSelChangeSkill8Combo()
{
    _onSelChangeSkillCombo(7);
}
