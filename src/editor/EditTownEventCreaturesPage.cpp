// EditTownEventCreaturesPage.cpp - Loki h3maped object 99: the creatures
// page of the town event sheet, one 0..s_kMax spin button per creature
// generator. The assert lines come from the retail immediates.
#include "editor/stdafx.h"

#include <stdio.h>

namespace {
#include <gtk/gtk.h>
}

#include "editor/EditTownEventCreaturesPage.h"

TEditTownEventCreaturesPage::TEditTownEventCreaturesPage(const TTown::TTimedEvent& event, TTownType townType)
    : _m_event(event),
      _m_townType(townType),
      _m_bModified(false)
{
#line 42
    assert(townType >= 0 && townType <= kNumTownTypes);
    for (unsigned int i = 0; i < TTown::s_kNumGeneratorTypes; i++) {
        char name[100];
        snprintf(name, 100, "edit_town_event_label%d", i + 1);
        _m_aGeneratorWidgets[i].m_pLabel = GTK_LABEL(_widget(name));
        snprintf(name, 100, "edit_town_event_spin%d", i + 1);
        _m_aGeneratorWidgets[i].m_pSpin = GTK_SPIN_BUTTON(_widget(name));
    }
    OnInitDialog();
}

TEditTownEventCreaturesPage::~TEditTownEventCreaturesPage()
{
}

BOOL TEditTownEventCreaturesPage::OnInitDialog()
{
    _m_bModified = false;
    _m_generatorBonuses = _m_event.getGeneratorBonuses();
    for (unsigned int i = 0; i < TTown::s_kNumGeneratorTypes; i++) {
        GtkAdjustment* adj = gtk_spin_button_get_adjustment(_m_aGeneratorWidgets[i].m_pSpin);
        adj->lower = 0;
        adj->upper = TTown::TGeneratorBonuses::s_kMax;
        gtk_adjustment_changed(adj);
    }
    return true;
}

void TEditTownEventCreaturesPage::OnOK()
{
    for (unsigned int i = 0; i < TTown::s_kNumGeneratorTypes; i++) {
        int qty = 0;
        GtkAdjustment* adj = gtk_spin_button_get_adjustment(_m_aGeneratorWidgets[i].m_pSpin);
        qty = (int)adj->value;
#line 174
        assert(qty >= 0 && qty <= TTown::TGeneratorBonuses::s_kMax);
        _m_generatorBonuses.set(TTown::TGeneratorType(i), qty);
    }
    _m_bModified = _m_bModified || _m_generatorBonuses != _m_event.getGeneratorBonuses();
}
