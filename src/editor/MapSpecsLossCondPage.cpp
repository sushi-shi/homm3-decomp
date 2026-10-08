// MapSpecsLossCondPage.cpp - Loki h3maped object 71: the loss condition page
// of the map specifications sheet. The losscond_* radios choose the
// condition; the town, hero and time choices show the loss_cond_label/box
// and fill loss_cond_combo with the towns or heroes on the map ("name at
// (x, y)") or the time limits. OnOK builds the new condition and records
// whether it differs from the map's. The assert and allocation-check lines
// come from the retail immediates.
#include "editor/stdafx.h"

#include <stdio.h>
#include <string.h>
#include <memory>
#include <vector>

#include "exceptions.h"
#include "objnames.h"
#include "editor/cppbridge.h"
#include "editor/MapSpecsLossCondPage.h"
#include "editor/GameObject.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/Town.h"
#include "editor/GameMap.h"

namespace {
#include <glade/glade.h>
}

// main.c: the loaded glade tree.
extern GladeXML* xml_obj;

#define ARRAY_SIZE( a ) ( sizeof( a ) / sizeof( ( a )[ 0 ] ) )

namespace {

// The time limits in days: 2..7 days, 1..7 weeks, 2..12 months.
const unsigned int akTimeLimitEntryInfo[] = {
    2, 3, 4, 5, 6, 7, 14, 21, 28, 35, 42, 49, 56, 84, 112, 140, 168, 196, 224, 252, 280, 308, 336
};

inline const char* getTimeLimitEntryName(unsigned int i)
{
#line 116
    assert(i < ARRAY_SIZE( akTimeLimitEntryInfo ));
    const char* const akName[ARRAY_SIZE(akTimeLimitEntryInfo)] = {
        k2DaysStr, k3DaysStr, k4DaysStr, k5DaysStr, k6DaysStr, k1WeekStr, k2WeeksStr, k3WeeksStr,
        k4WeeksStr, k5WeeksStr, k6WeeksStr, k7WeeksStr, k2MonthsStr, k3MonthsStr, k4MonthsStr,
        k5MonthsStr, k6MonthsStr, k7MonthsStr, k8MonthsStr, k9MonthsStr, k10MonthsStr, k11MonthsStr,
        k12MonthsStr
    };
    return akName[i];
}

}

// Shows the condition's label and combo with label text, or hides them for
// no condition.
static GtkCombo* showLossConditionWidgets(char* text)
{
    GtkLabel* label = GTK_LABEL(_widget("loss_cond_label"));
    GtkHBox* box = GTK_HBOX(_widget("loss_cond_box"));
    GtkWidget* separator = _widget("loss_cond_separator");
    if (text == NULL) {
        gtk_widget_hide(GTK_WIDGET(box));
        gtk_widget_hide(GTK_WIDGET(separator));
        gtk_widget_hide(GTK_WIDGET(label));
        return NULL;
    }
    gtk_label_set_text(label, text);
    gtk_widget_show(GTK_WIDGET(box));
    gtk_widget_show(GTK_WIDGET(separator));
    gtk_widget_show(GTK_WIDGET(label));
    GtkCombo* c = GTK_COMBO(_widget("loss_cond_combo"));
    return c;
}

void TMapSpecsLossCondPage::LoseNullInit()
{
    showLossConditionWidgets(NULL);
}

auto_ptr<TLossCondition> TMapSpecsLossCondPage::LoseTownCondition() const
{
    auto_ptr<TLossCondition> pLossCondition(new TLCLoseTown(_m_townRef));
    if (!pLossCondition.get())
#line 317
        throw TAllocationFailure(__FILE__, __LINE__);
    return pLossCondition;
}

auto_ptr<TLossCondition> TMapSpecsLossCondPage::LoseNullCondition() const
{
    auto_ptr<TLossCondition> pLossCondition(NULL);
    return pLossCondition;
}

BOOL TMapSpecsLossCondPage::LoseTownInit()
{
    GtkCombo* c = showLossConditionWidgets("Town:");
    GList* items = NULL;
    int itemNum = 0;
    if (_m_aTownsOnMap.size()) {
        for (unsigned int i = 0; i < _m_aTownsOnMap.size(); i++) {
            const TTown* pTown = dynamic_cast<const TTown*>(_m_map.getPObject(_m_aTownsOnMap[i]));
#line 340
            assert(pTown != NULL);
            TPoint<unsigned int> loc = _m_map.getObjectLoc(_m_aTownsOnMap[i]) - pTown->getTriggerLoc();
            char* name = (char*)g_malloc(100);
            snprintf(name, 100, kObjectAtLocationFmtStr, pTown->getTownTypeTraits().m_pName, loc.x(), loc.y(),
                     _m_aTownsOnMap[i].getBSecondLayer());
            items = g_list_append(items, name);
            _m_itemData[itemNum] = i;
            itemNum++;
        }
        gtk_combo_set_popdown_strings(c, items);
        setCurrentSelection(GTK_LIST(c->list), 0);
        for (GList* p = items; p; p = p->next)
            g_free(p->data);
        g_list_free(items);
    }
    return true;
}

void TMapSpecsLossCondPage::LoseTownOnOK()
{
    GtkCombo* c = GTK_COMBO(glade_xml_get_widget(xml_obj, "loss_cond_combo"));
#line 387
    assert(c != NULL);
    GtkEntry* entry = GTK_ENTRY(c->entry);
#line 389
    assert(entry != NULL);
    gchar* text = gtk_entry_get_text(entry);
    unsigned int itemData = _m_itemData[getCurrentSelection(GTK_LIST(c->list))];
#line 394
    assert(itemData >= 0 && itemData < _m_aTownsOnMap.size());
    _m_townRef = _m_aTownsOnMap[itemData];
}

auto_ptr<TLossCondition> TMapSpecsLossCondPage::LoseHeroCondition() const
{
    auto_ptr<TLossCondition> pLossCondition(new TLCLoseHero(_m_heroRef));
    if (!pLossCondition.get())
#line 459
        throw TAllocationFailure(__FILE__, __LINE__);
    return pLossCondition;
}

BOOL TMapSpecsLossCondPage::LoseHeroInit()
{
    int itemNum = 0;
    GtkCombo* c = showLossConditionWidgets("Hero:");
#line 482
    assert(c != NULL);
    GList* items = NULL;
    if (_m_aHeroesOnMap.size()) {
        for (unsigned int i = 0; i < _m_aHeroesOnMap.size(); i++) {
            const TGameObject* pObject = _m_map.getPObject(_m_aHeroesOnMap[i]);
            TPoint<unsigned int> loc = _m_map.getObjectLoc(_m_aHeroesOnMap[i]) - pObject->getTriggerLoc();
            const THero* pHero = dynamic_cast<const THero*>(pObject);
            if (!pHero) {
                const TTown* pTown = dynamic_cast<const TTown*>(pObject);
#line 497
                assert(pTown != NULL);
                pHero = pTown->getPVisitingHero();
#line 499
                assert(pHero != NULL);
            }
            char heroName[256];
            if (pHero->getClass() < kNumHeroClasses || pHero->getBCustomName())
                snprintf(heroName, 255, kSpecificHeroAndClassFmtStr, pHero->getName().c_str(),
                         pHero->getClassTraits().m_name);
            else
                strncpy(heroName, akAdvObjectTypeTraits[RANDOM_HERO].m_name, 255);
            char* name = (char*)g_malloc(255);
            snprintf(name, 255, kObjectAtLocationFmtStr, heroName, loc.x(), loc.y(),
                     _m_aHeroesOnMap[i].getBSecondLayer());
            items = g_list_append(items, name);
            _m_itemData[itemNum] = i;
            itemNum++;
        }
        gtk_combo_set_popdown_strings(c, items);
        for (GList* p = items; p; p = p->next)
            g_free(p->data);
        g_list_free(items);
        setCurrentSelection(GTK_LIST(c->list), 0);
    }
    return true;
}

void TMapSpecsLossCondPage::LoseHeroOnOK()
{
    GtkCombo* c = GTK_COMBO(glade_xml_get_widget(xml_obj, "loss_cond_combo"));
#line 553
    assert(c != NULL);
    assert(GTK_LIST(c->list)->selection != NULL);
    unsigned int itemData = _m_itemData[getCurrentSelection(GTK_LIST(c->list))];
#line 557
    assert(itemData >= 0 && itemData < _m_aHeroesOnMap.size());
    _m_heroRef = _m_aHeroesOnMap[itemData];
}

auto_ptr<TLossCondition> TMapSpecsLossCondPage::LoseTimeCondition() const
{
    auto_ptr<TLossCondition> pLossCondition(new TLCTimeExpires(_m_numDays));
    if (!pLossCondition.get())
#line 611
        throw TAllocationFailure(__FILE__, __LINE__);
    return pLossCondition;
}

BOOL TMapSpecsLossCondPage::LoseTimeInit()
{
    GtkCombo* c = showLossConditionWidgets("Time limit:");
    GList* items = NULL;
    int itemNum = 0;
    for (unsigned int i = 0; i < ARRAY_SIZE(akTimeLimitEntryInfo); i++) {
        items = g_list_append(items, (gpointer)getTimeLimitEntryName(i));
        _m_itemData[itemNum] = i;
        itemNum++;
    }
    gtk_combo_set_popdown_strings(c, items);
    g_list_free(items);
    setCurrentSelection(GTK_LIST(c->list), 0);
    return true;
}

void TMapSpecsLossCondPage::LoseTimeOnOK()
{
    GtkCombo* c = GTK_COMBO(glade_xml_get_widget(xml_obj, "loss_cond_combo"));
#line 659
    assert(c != NULL);
    GtkList* l = GTK_LIST(c->list);
#line 661
    assert(l != NULL);
#line 664
    assert(l->selection != NULL);
    unsigned int itemData = _m_itemData[getCurrentSelection(l)];
#line 667
    assert(itemData >= 0 && itemData < ARRAY_SIZE( akTimeLimitEntryInfo ));
    _m_numDays = akTimeLimitEntryInfo[itemData];
}

TMapSpecsLossCondPage::TMapSpecsLossCondPage(const TGameMap& map, const vector<TMapObjectRef>& townsOnMap,
                                             const vector<TMapObjectRef>& heroesOnMap)
    : _m_lossConditionType(0),
      _m_map(map),
      _m_aTownsOnMap(townsOnMap),
      _m_bTownsOnMap(townsOnMap.size()),
      _m_aHeroesOnMap(heroesOnMap),
      _m_bHeroesOnMap(heroesOnMap.size()),
      _m_bModified(false),
      _m_pLossCondition(NULL)
{
    GtkWidget* w = _widget("losscond_town");
    gtk_widget_set_sensitive(w, _m_bTownsOnMap);
    w = _widget("losscond_hero");
    gtk_widget_set_sensitive(w, _m_bHeroesOnMap);
}

TMapSpecsLossCondPage::~TMapSpecsLossCondPage()
{
}

BOOL TMapSpecsLossCondPage::OnInitDialog()
{
    _m_bModified = false;
    if (_m_map.getPLossCondition()) {
        _m_pLossCondition = auto_ptr<TLossCondition>(TLossCondition::clone(*_m_map.getPLossCondition(), NULL));
        if (!_m_pLossCondition.get())
#line 735
            throw TAllocationFailure(__FILE__, __LINE__);
    } else
        _m_pLossCondition = auto_ptr<TLossCondition>(NULL);

    GtkWidget* w = _widget("losscond_town");
    gtk_widget_set_sensitive(w, _m_bTownsOnMap);
    w = _widget("losscond_hero");
    gtk_widget_set_sensitive(w, _m_bHeroesOnMap);
    if (_m_pLossCondition.get())
        _m_pLossCondition->accept(this);
    showLossConditionWidgets(NULL);

    w = _widget("losscond_none");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
    int kind = 0;
    if (_m_pLossCondition.get())
        kind = _m_pLossCondition.get()->_m_kind;
    switch (kind) {
    case 0:
        w = _widget("losscond_none");
        break;
    case 1:
        w = _widget("losscond_town");
        break;
    case 2:
        w = _widget("losscond_hero");
        break;
    case 3:
        w = _widget("losscond_time");
        break;
    default:
#line 779
        assert(false);
    }
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
    return true;
}

void TMapSpecsLossCondPage::OnOK()
{
    if (_m_lossConditionType == 0)
        _m_pLossCondition = LoseNullCondition();
    else if (_m_lossConditionType == 1) {
        LoseTownOnOK();
        _m_pLossCondition = LoseTownCondition();
    } else if (_m_lossConditionType == 2) {
        LoseHeroOnOK();
        _m_pLossCondition = LoseHeroCondition();
    } else if (_m_lossConditionType == 3) {
        LoseTimeOnOK();
        _m_pLossCondition = LoseTimeCondition();
    } else {
#line 809
        assert(0);
    }
    _m_bModified = _m_bModified
                   || ((_m_pLossCondition.get() != NULL) != (_m_map.getPLossCondition() != NULL))
                   || (_m_pLossCondition.get()
                       && !TLossCondition::equivalent(*_m_pLossCondition, *_m_map.getPLossCondition()));
}

void TMapSpecsLossCondPage::OnLoseTownToggled()
{
    _m_lossConditionType = 1;
    LoseTownInit();
}

void TMapSpecsLossCondPage::OnLoseNullToggled()
{
    _m_lossConditionType = 0;
    LoseNullInit();
}

void TMapSpecsLossCondPage::OnLoseHeroToggled()
{
    _m_lossConditionType = 2;
    LoseHeroInit();
}

void TMapSpecsLossCondPage::OnLoseTimeToggled()
{
    _m_lossConditionType = 3;
    LoseTimeInit();
}
