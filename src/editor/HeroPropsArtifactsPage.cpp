// HeroPropsArtifactsPage.cpp - Loki h3maped object 86: the hero sheet's
// artifacts page. The list shows each worn artifact with its slot and each
// backpack artifact; adding or editing goes through TEditArtifactDlg, which
// offers the unused slots and the backpack while it has room. The assert
// lines come from the retail immediates.
#include "editor/stdafx.h"

#include <strings.h>

namespace {
#include <gtk/gtk.h>
}

#include "editor/cppbridge.h"
#include "editor/HeroPropsArtifactsPage.h"
#include "editor/MapEditorText.h"

namespace {

int compareItems(unsigned long lParam1, unsigned long lParam2, unsigned long lParamSort)
{
    TArtifact artifact1 = TArtifact(lParam1 & 0xffff);
    TArtifact artifact2 = TArtifact((unsigned short)lParam2);
    TArtifactSlot slot1 = TArtifactSlot((lParam1 & 0xffff0000) >> 16);
    TArtifactSlot slot2 = TArtifactSlot((lParam2 & 0xffff0000) >> 16);
    const char* name1 = akArtifactTraits[artifact1].m_name;
    const char* name2 = akArtifactTraits[artifact2].m_name;
    const char* slotName1 = slot1 < kNumArtifactSlots ? akArtifactSlotTraits[slot1].m_name : kBackpackStr;
    const char* slotName2 = slot2 < kNumArtifactSlots ? akArtifactSlotTraits[slot2].m_name : kBackpackStr;
    if (lParamSort == 0) {
        int result = strcasecmp(name1, name2);
        return result ? result : strcasecmp(slotName1, slotName2);
    }
#line 97
    assert(lParamSort == 1);
    int result = strcasecmp(slotName1, slotName2);
    return result ? result : strcasecmp(name1, name2);
}

}

// The worn slots the page offers, in list order.
static const TArtifactSlot akWornSlots[] = {
    eArtifactSlotHead, eArtifactSlotShoulders, eArtifactSlotNeck, eArtifactSlotRightHand,
    eArtifactSlotLeftHand, eArtifactSlotTorso, eArtifactSlotRightRing, eArtifactSlotLeftRing,
    eArtifactSlotFeet, eArtifactSlotMisc1, eArtifactSlotMisc2, eArtifactSlotMisc3,
    eArtifactSlotMisc4, eArtifactSlotWarMachine1, eArtifactSlotWarMachine2, eArtifactSlotWarMachine3,
};

THeroPropsArtifactsPage::THeroPropsArtifactsPage(const THero& hero)
    : _m_hero(hero),
      _m_pPrototype(NULL)
{
    _m_bModified = false;
    _m_itemCount = 0;
    _m_bCustom = false;
}

THeroPropsArtifactsPage::~THeroPropsArtifactsPage()
{
}

void THeroPropsArtifactsPage::setIdentity(THeroClass newHeroClass, unsigned int newProtoNum)
{
#line 136
    assert(newHeroClass >= 0 && newHeroClass < kNumHeroClasses);
    assert(newProtoNum < THero::s_akClassTraits[ newHeroClass ].m_numPrototypes);
    _m_heroClass = newHeroClass;
    _m_protoNum = newProtoNum;
    _m_pPrototype = &THero::s_akClassTraits[_m_heroClass].m_aPrototype[_m_protoNum];
    if (!_m_bCustom)
        _setArtifacts(_m_pPrototype->getArtifacts());
}

void THeroPropsArtifactsPage::_setArtifacts(const THeroPrototype::TArtifactContainer& artifacts)
{
#line 150
    assert(artifacts.getSlot( eArtifactSlotSpellbook ) == eArtifactSpellbook || artifacts.getSlot( eArtifactSlotSpellbook ) == eArtifactNone);
    OnRemoveAllArtifactButton();
    for (unsigned int i = 0; i < sizeof(akWornSlots) / sizeof(akWornSlots[0]); i++) {
        TArtifactSlot slot = akWornSlots[i];
        TArtifact artifact = artifacts.getSlot(slot);
        if (artifact != eArtifactNone && _addItem(artifact, slot) != -1) {
#line 163
            assert(_m_unusedSlots.find( slot ) != _m_unusedSlots.end());
            _m_unusedSlots.erase(slot);
        }
    }
    const vector<TArtifact>& backpack = artifacts.getBackpack();
    for (vector<TArtifact>::const_iterator backpackIter = backpack.begin(); backpackIter != backpack.end();
         backpackIter++) {
#line 175
        assert(*backpackIter >= 0 && *backpackIter < kNumArtifacts);
        _addItem(*backpackIter, kNumArtifactSlots);
    }
    _m_backpackSize = backpack.size();
    if (_m_bCustom && _m_unusedSlots.empty() && _m_backpackSize >= THero::s_kMaxBackpackSize)
        gtk_widget_set_sensitive(_widget("hero_artifacts_add"), FALSE);
    if (_m_itemCount > 0 && _m_bCustom)
        gtk_widget_set_sensitive(_widget("hero_artifacts_remall"), TRUE);
    GtkToggleButton* pButton = GTK_TOGGLE_BUTTON(_widget("hero_artifacts_has_spellbook"));
    gtk_toggle_button_set_active(pButton, artifacts.getSlot(eArtifactSlotSpellbook) == eArtifactSpellbook);
}

void THeroPropsArtifactsPage::_retrieveArtifacts(THeroPrototype::TArtifactContainer* pArtifacts)
{
    THeroPrototype::TArtifactContainer& artifacts = *pArtifacts;
    GtkCList* pList = GTK_CLIST(_widget("hero_artifacts_list"));
    artifacts = THeroPrototype::TArtifactContainer();
    for (int item = 0; item < _m_itemCount; item++) {
        unsigned long data = (unsigned long)gtk_clist_get_row_data(pList, item);
        TArtifact artifact = TArtifact(data & 0xffff);
        TArtifactSlot slot = TArtifactSlot((data & 0xffff0000) >> 16);
#line 227
        assert(artifact >= 0 && artifact < kNumArtifacts);
        assert(slot >= 0);
        if (slot < kNumArtifactSlots)
            artifacts.setSlot(slot, artifact);
        else
            artifacts.getPBackpack()->push_back(artifact);
    }
    if (isChecked("hero_artifacts_has_spellbook"))
        artifacts.setSlot(eArtifactSlotSpellbook, eArtifactSpellbook);
#line 241
    assert(artifacts.getBackpack().size() == _m_backpackSize);
}

void THeroPropsArtifactsPage::_enableControls()
{
    gtk_widget_set_sensitive(_widget("hero_artifacts_has_spellbook"), TRUE);
    if (!_m_unusedSlots.empty() || _m_backpackSize < THero::s_kMaxBackpackSize)
        gtk_widget_set_sensitive(_widget("hero_artifacts_add"), TRUE);
    if (_m_itemCount > 0)
        gtk_widget_set_sensitive(_widget("hero_artifacts_remall"), TRUE);
    gtk_widget_set_sensitive(_widget("hero_artifacts_list"), TRUE);
}

void THeroPropsArtifactsPage::_disableControls()
{
    GtkCList* pList = GTK_CLIST(_widget("hero_artifacts_list"));
    gtk_widget_set_sensitive(GTK_WIDGET(pList), FALSE);
    gtk_widget_set_sensitive(_widget("hero_artifacts_has_spellbook"), FALSE);
    gtk_widget_set_sensitive(_widget("hero_artifacts_add"), FALSE);
    gtk_widget_set_sensitive(_widget("hero_artifacts_edit"), FALSE);
    gtk_widget_set_sensitive(_widget("hero_artifacts_remove"), FALSE);
    gtk_widget_set_sensitive(_widget("hero_artifacts_remall"), FALSE);
}

int THeroPropsArtifactsPage::_addItem(TArtifact artifact, TArtifactSlot slot)
{
    GtkCList* pList = GTK_CLIST(_widget("hero_artifacts_list"));
    gchar* text[2];
    text[0] = (gchar*)akArtifactTraits[artifact].m_name;
    text[1] = (gchar*)(slot < kNumArtifactSlots ? akArtifactSlotTraits[slot].m_name : kBackpackStr);
    int row = gtk_clist_append(pList, text);
    gtk_clist_set_row_data(pList, row, (gpointer)((slot << 16) | artifact));
    gtk_clist_columns_autosize(pList);
    _m_itemCount++;
    return row;
}

bool THeroPropsArtifactsPage::_setItem(int item, TArtifact artifact, TArtifactSlot slot)
{
    GtkCList* pList = GTK_CLIST(_widget("hero_artifacts_list"));
#line 342
    assert(item >= 0 && item < _m_itemCount);
    gchar* text[2];
    text[0] = (gchar*)akArtifactTraits[artifact].m_name;
    text[1] = (gchar*)(slot < kNumArtifactSlots ? akArtifactSlotTraits[slot].m_name : kBackpackStr);
    gtk_clist_remove(pList, item);
    gtk_clist_insert(pList, item, text);
    gtk_clist_set_row_data(pList, item, (gpointer)((slot << 16) | artifact));
    gtk_clist_columns_autosize(pList);
}

void THeroPropsArtifactsPage::UpdateData(bool bSaveAndValidate)
{
    _m_bCustom = isChecked("hero_artifacts_customize");
}

BOOL THeroPropsArtifactsPage::OnInitDialog()
{
    GtkCList* pList = GTK_CLIST(_widget("hero_artifacts_list"));
    gtk_clist_clear(pList);
    _m_bModified = false;
    _m_curSel = -1;
    _m_bCustomArtifacts = _m_hero.getBCustomArtifacts();
    _m_artifacts = _m_hero.getCustomArtifacts();
    _m_heroClass = _m_hero.getClass();
    _m_protoNum = _m_hero.getProtoNum();
    _m_pPrototype = &THero::s_akClassTraits[_m_heroClass].m_aPrototype[_m_protoNum];
    _m_bCustom = _m_bCustomArtifacts;
    _m_editArtifacts = _m_artifacts;
    gtk_widget_set_sensitive(_widget("hero_artifacts_edit"), FALSE);
    gtk_widget_set_sensitive(_widget("hero_artifacts_remove"), FALSE);
    gtk_widget_set_sensitive(_widget("hero_artifacts_remall"), FALSE);
    _m_backpackSize = 0;
    for (unsigned int i = 0; i < sizeof(akWornSlots) / sizeof(akWornSlots[0]); i++)
        _m_unusedSlots.insert(akWornSlots[i]);
    _setArtifacts(_m_bCustom ? _m_editArtifacts : _m_pPrototype->getArtifacts());
    GtkToggleButton* pButton = GTK_TOGGLE_BUTTON(_widget("hero_artifacts_customize"));
    gtk_signal_handler_block_by_func(GTK_OBJECT(pButton), GTK_SIGNAL_FUNC(on_hero_artifacts_customize_toggled), NULL);
    gtk_toggle_button_set_active(pButton, _m_bCustom);
    gtk_signal_handler_unblock_by_func(GTK_OBJECT(pButton), GTK_SIGNAL_FUNC(on_hero_artifacts_customize_toggled), NULL);
    if (!_m_bCustom)
        _disableControls();
    else
        _enableControls();
    return true;
}

void THeroPropsArtifactsPage::OnOK()
{
    if (_m_bCustom)
        _retrieveArtifacts(&_m_editArtifacts);
    _m_bCustomArtifacts = _m_bCustom != FALSE;
    _m_artifacts = _m_editArtifacts;
    _m_bModified = _m_bModified || _m_bCustomArtifacts != _m_hero.getBCustomArtifacts()
                   || _m_artifacts != _m_hero.getCustomArtifacts();
}

void THeroPropsArtifactsPage::OnCustomizeCheck()
{
    bool bWasCustom = _m_bCustom != FALSE;
    UpdateData(false);
    if (_m_bCustom) {
        if (!bWasCustom) {
            _enableControls();
            _setArtifacts(_m_editArtifacts);
        }
    } else if (bWasCustom) {
        _retrieveArtifacts(&_m_editArtifacts);
        _setArtifacts(_m_pPrototype->getArtifacts());
        _disableControls();
    }
}

void THeroPropsArtifactsPage::OnAddArtifactButton()
{
#line 543
    assert(_m_bCustom);
    assert(!_m_unusedSlots.empty() || _m_backpackSize < THero::s_kMaxBackpackSize);
    TEditArtifactDlg dlg(NULL, _m_unusedSlots, _m_backpackSize >= THero::s_kMaxBackpackSize);
    if (dlg.DoModal() == 1) {
        TArtifact artifact = dlg.getArtifact();
        TArtifactSlot slot = dlg.getSlot();
#line 555
        assert(artifact >= 0 && artifact < kNumArtifacts);
        assert(slot >= kNumArtifactSlots || _m_unusedSlots.find( slot ) != _m_unusedSlots.end());
        if (_addItem(artifact, slot) != -1) {
            if (slot < kNumArtifactSlots)
                _m_unusedSlots.erase(slot);
            else
                _m_backpackSize++;
            gtk_widget_set_sensitive(_widget("hero_artifacts_edit"), TRUE);
            gtk_widget_set_sensitive(_widget("hero_artifacts_remove"), TRUE);
            if (_m_curSel < 0) {
                GtkCList* pList = GTK_CLIST(_widget("hero_artifacts_list"));
                gtk_clist_select_row(pList, 0, 0);
            }
            if (_m_unusedSlots.empty() && _m_backpackSize >= THero::s_kMaxBackpackSize)
                gtk_widget_set_sensitive(_widget("hero_artifacts_add"), FALSE);
            gtk_widget_set_sensitive(_widget("hero_artifacts_remall"), TRUE);
        }
    }
}

void THeroPropsArtifactsPage::OnEditArtifactButton()
{
#line 595
    assert(_m_bCustom);
    GtkCList* pList = GTK_CLIST(_widget("hero_artifacts_list"));
    int curSel = _m_curSel;
#line 602
    assert(curSel != -1);
    unsigned long data = (unsigned long)gtk_clist_get_row_data(pList, curSel);
    TArtifact artifact = TArtifact(data & 0xffff);
    TArtifactSlot slot = TArtifactSlot((data & 0xffff0000) >> 16);
    g_warning("editing slot (%d), artifact (%d).", slot, artifact);
#line 613
    assert(artifact >= 0 && artifact < kNumArtifacts);
    assert(_m_unusedSlots.find( slot ) == _m_unusedSlots.end());
    TEditArtifactDlg dlg(NULL, _m_unusedSlots, _m_backpackSize >= THero::s_kMaxBackpackSize, artifact, slot);
    if (dlg.DoModal() == 1) {
        TArtifact newArtifact = dlg.getArtifact();
        TArtifactSlot newSlot = dlg.getSlot();
        if ((artifact != newArtifact || slot != newSlot) && _setItem(curSel, newArtifact, newSlot)
            && slot != newSlot) {
#line 634
            assert(newSlot >= kNumArtifactSlots || _m_unusedSlots.find( newSlot ) != _m_unusedSlots.end());
            if (newSlot < kNumArtifactSlots)
                _m_unusedSlots.erase(newSlot);
            else
                _m_backpackSize++;
            if (slot < kNumArtifactSlots)
                _m_unusedSlots.insert(slot);
            else
                _m_backpackSize--;
        }
    }
}

void THeroPropsArtifactsPage::OnRemoveArtifactButton()
{
#line 657
    assert(_m_bCustom);
    GtkCList* pList = GTK_CLIST(_widget("hero_artifacts_list"));
    int curSel = _m_curSel;
#line 663
    assert(curSel != -1);
    unsigned long data = (unsigned long)gtk_clist_get_row_data(pList, curSel);
    TArtifactSlot slot = TArtifactSlot((data & 0xffff0000) >> 16);
    if (slot < kNumArtifactSlots)
        _m_unusedSlots.insert(slot);
    else
        _m_backpackSize--;
    gtk_clist_remove(pList, curSel);
    _m_itemCount--;
    if (!_m_unusedSlots.empty() || _m_backpackSize < THero::s_kMaxBackpackSize)
        gtk_widget_set_sensitive(_widget("hero_artifacts_add"), TRUE);
    int count = _m_itemCount;
    if (count > 0) {
        curSel = count - 1 <? curSel;
        gtk_clist_select_row(pList, curSel, 0);
    } else
        gtk_widget_set_sensitive(_widget("hero_artifacts_remall"), FALSE);
}

void THeroPropsArtifactsPage::OnRemoveAllArtifactButton()
{
    GtkCList* pList = GTK_CLIST(_widget("hero_artifacts_list"));
    for (int item = _m_itemCount; item > 0; item--) {
        unsigned long data = (unsigned long)gtk_clist_get_row_data(pList, 0);
        TArtifactSlot slot = TArtifactSlot((data & 0xffff0000) >> 16);
        if (slot < kNumArtifactSlots)
            _m_unusedSlots.insert(slot);
        gtk_clist_remove(pList, 0);
    }
    _m_backpackSize = 0;
    _m_itemCount = 0;
    if (_m_bCustom) {
        gtk_widget_set_sensitive(_widget("hero_artifacts_add"), TRUE);
        gtk_widget_set_sensitive(_widget("hero_artifacts_remall"), FALSE);
    }
}
