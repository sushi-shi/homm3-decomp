// ResourceQuantitiesDlg.cpp - Loki h3maped object 26: the resource
// quantities dialog over the timed event glade widgets. A negative quantity
// shows as its magnitude with the "give" toggle set. The assert line comes
// from the retail immediate.
#include "editor/stdafx.h"

#include <stdio.h>
#include <string>

namespace {
#include <gtk/gtk.h>
}

#include "editor/ResourceQuantitiesDlg.h"

TResourceQuantitiesDlg::TResourceQuantitiesDlg(const TResourceQuantities& resourceQuantities)
    : _m_originalQuantities(resourceQuantities),
      _m_bModified(false)
{
}

BOOL TResourceQuantitiesDlg::OnInitDialog()
{
    _m_bModified = false;
    _m_resourceQuantities = _m_originalQuantities;

    char name[50];
    unsigned int type;
    for (type = 0; type < kNumGameResourceTypes; type++) {
        snprintf(name, 50, "timed_event_give%d", type);
        GtkToggleButton* pGiveButton = GTK_TOGGLE_BUTTON(_widget(name));
        gtk_toggle_button_set_active(pGiveButton,
                                     _m_resourceQuantities.get(TGameResourceType(type)) < 0);
    }
    for (type = 0; type < kNumGameResourceTypes; type++) {
        snprintf(name, 50, "timed_event_rsrc%d", type);
        GtkSpinButton* pSpinButton = GTK_SPIN_BUTTON(_widget(name));
        int quantity = _m_resourceQuantities.get(TGameResourceType(type));
        if (quantity < 0)
            quantity = -quantity;
        gtk_spin_button_set_value(pSpinButton, quantity);
        GtkAdjustment* adj = gtk_spin_button_get_adjustment(pSpinButton);
#line 163
        assert(adj != NULL);
        adj->lower = 0;
        adj->upper = 255.0f;
    }
    return true;
}

void TResourceQuantitiesDlg::OnOK()
{
    for (unsigned int type = 0; type < kNumGameResourceTypes; type++) {
        int quantity = 0;
        char name[50];
        snprintf(name, 50, "timed_event_rsrc%d", type);
        GtkSpinButton* pSpinButton = GTK_SPIN_BUTTON(_widget(name));
        quantity = gtk_spin_button_get_value_as_int(pSpinButton);
        snprintf(name, 50, "timed_event_give%d", type);
        GtkToggleButton* pGiveButton = GTK_TOGGLE_BUTTON(_widget(name));
        if (gtk_toggle_button_get_active(pGiveButton))
            quantity = -quantity;
        _m_resourceQuantities.set(TGameResourceType(type), quantity);
    }
    _m_bModified = _m_bModified || _m_resourceQuantities != _m_originalQuantities;
}
