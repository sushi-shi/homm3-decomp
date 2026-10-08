// EditTimedEventSheet.cpp - Loki h3maped object 79: the timed event sheet.
// DoModal runs the timed_event_dlg glade dialog (its third and fourth
// notebook pages stay insensitive) and copies the pages' modified values
// into the event. The allocation checks' lines come from the retail
// immediates.
#include "editor/stdafx.h"

#include <string>

#include "exceptions.h"
#include "editor/cppbridge.h"
#include "editor/EditTimedEventGeneralPage.h"
#include "editor/EditTimedEventResourcesPage.h"
#include "editor/EditTimedEventSheet.h"

TEditTimedEventSheet* timedEventSheetModal = NULL;

TEditTimedEventSheet::TEditTimedEventSheet(const TPlayerMask& playersPresent, const TTimedEvent& event)
    : _m_event(event),
      _m_pGeneralPage(NULL),
      _m_pResourcesPage(NULL)
{
    _m_pGeneralPage = new TEditTimedEventGeneralPage(this, _m_event, playersPresent);
#line 47
    if (!_m_pGeneralPage)
        throw TAllocationFailure(__FILE__, __LINE__);
    try {
        _m_pResourcesPage = new TEditTimedEventResourcesPage(_m_event);
#line 56
        if (!_m_pResourcesPage)
            throw TAllocationFailure(__FILE__, __LINE__);
    } catch (...) {
        _deleteAllPages();
        throw;
    }
}

TEditTimedEventSheet::~TEditTimedEventSheet()
{
    _deleteAllPages();
}

void TEditTimedEventSheet::OnOK()
{
    _m_result = 1;
}

void TEditTimedEventSheet::OnCancel()
{
    _m_result = 2;
}

void TEditTimedEventSheet::_deleteAllPages()
{
    if (_m_pResourcesPage)
        delete _m_pResourcesPage;
    if (_m_pGeneralPage)
        delete _m_pGeneralPage;
}

void TEditTimedEventSheet::onEnableOK()
{
    enableWidget("timed_event_ok", TRUE);
}

void TEditTimedEventSheet::onDisableOK()
{
    enableWidget("timed_event_ok", FALSE);
}

int TEditTimedEventSheet::DoModal()
{
    GtkNotebook* pNotebook = GTK_NOTEBOOK(_widget("edit_timed_event_notebook"));
    gtk_notebook_set_page(pNotebook, 0);
    gtk_widget_set_sensitive(gtk_notebook_get_nth_page(pNotebook, 2), FALSE);
    gtk_widget_set_sensitive(gtk_notebook_get_nth_page(pNotebook, 3), FALSE);

    _m_result = 0;
    timedEventSheetModal = this;
    GtkWidget* pDialog = _widget("timed_event_dlg");
    gtk_widget_show(pDialog);
    while (_m_result == 0)
        gtk_main_iteration();
    gtk_widget_hide(pDialog);
    timedEventSheetModal = NULL;
    int result = _m_result;

    if (_m_pGeneralPage->wasModified()) {
        _m_event.setName(_m_pGeneralPage->getName());
        _m_event.setMessage(_m_pGeneralPage->getMessage());
        _m_event.setBApplyToComputer(_m_pGeneralPage->getBApplyToComputer());
        _m_event.setFirstOccurence(_m_pGeneralPage->getFirstOccurence());
        _m_event.setSubsequentInterval(_m_pGeneralPage->getSubsequentInterval());
        for (unsigned int player = 0; player < kNumPlayers; player++)
            _m_event.setBApplyToPlayer(TPlayer(player), _m_pGeneralPage->getBApplyToPlayer(TPlayer(player)));
    }
    if (_m_pResourcesPage->wasModified())
        _m_event.setResourceQuantities(_m_pResourcesPage->getResourceQuantities());
    return result;
}
