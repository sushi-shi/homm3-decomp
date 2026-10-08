// EditTownEventSheet.cpp - Loki h3maped object 97: the town event sheet.
// It reuses the timed event general and resources pages and adds the
// buildings and creatures pages; DoModal runs the timed_event_dlg glade
// dialog with all four notebook pages sensitive and copies the modified
// page values into the event. The assert and allocation check lines come
// from the retail immediates.
#include "editor/stdafx.h"

#include <string>

#include "exceptions.h"
#include "editor/cppbridge.h"
#include "editor/EditTownEventSheet.h"
#include "editor/EditTimedEventGeneralPage.h"
#include "editor/EditTimedEventResourcesPage.h"
#include "editor/EditTownEventBuildingsPage.h"
#include "editor/EditTownEventCreaturesPage.h"

TEditTownEventSheet* editTownEventSheetModal = NULL;

TEditTownEventSheet::TEditTownEventSheet(GtkWidget* pParent, const TPlayerMask& playersPresent,
                                         TTownType townType, const TTown::TTimedEvent& event)
    : _m_event(event),
      _m_pGeneralPage(NULL),
      _m_pResourcesPage(NULL),
      _m_pBuildingsPage(NULL),
      _m_pCreaturesPage(NULL)
{
#line 50
    assert(townType >= 0 && townType <= kNumTownTypes);
    _m_pGeneralPage = new TEditTimedEventGeneralPage(this, _m_event, playersPresent);
#line 57
    if (!_m_pGeneralPage)
        throw TAllocationFailure(__FILE__, __LINE__);
    try {
        _m_pResourcesPage = new TEditTimedEventResourcesPage(_m_event);
#line 66
        if (!_m_pResourcesPage)
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pBuildingsPage = new TEditTownEventBuildingsPage(_m_event, townType);
#line 72
        if (!_m_pBuildingsPage)
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pCreaturesPage = new TEditTownEventCreaturesPage(_m_event, townType);
#line 78
        if (!_m_pCreaturesPage)
            throw TAllocationFailure(__FILE__, __LINE__);
    } catch (...) {
        _deleteAllPages();
        throw;
    }
}

TEditTownEventSheet::~TEditTownEventSheet()
{
    _deleteAllPages();
}

void TEditTownEventSheet::_deleteAllPages()
{
    if (_m_pCreaturesPage)
        delete _m_pCreaturesPage;
    if (_m_pBuildingsPage)
        delete _m_pBuildingsPage;
    if (_m_pResourcesPage)
        delete _m_pResourcesPage;
    if (_m_pGeneralPage)
        delete _m_pGeneralPage;
}

int TEditTownEventSheet::DoModal()
{
    GtkNotebook* pNotebook = GTK_NOTEBOOK(_widget("edit_timed_event_notebook"));
    gtk_notebook_set_page(pNotebook, 0);
    gtk_widget_set_sensitive(gtk_notebook_get_nth_page(pNotebook, 2), TRUE);
    gtk_widget_set_sensitive(gtk_notebook_get_nth_page(pNotebook, 3), TRUE);

    _m_result = 0;
    editTownEventSheetModal = this;
    GtkWidget* pDialog = _widget("timed_event_dlg");
    gtk_widget_show(pDialog);
    while (_m_result == 0)
        gtk_main_iteration();
    gtk_widget_hide(pDialog);
    editTownEventSheetModal = NULL;
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
    if (_m_pBuildingsPage->wasModified())
        _m_event.setBuildMask(_m_pBuildingsPage->getBuildMask());
    if (_m_pCreaturesPage->wasModified())
        _m_event.setGeneratorBonuses(_m_pCreaturesPage->getGeneratorBonuses());
    return result;
}
