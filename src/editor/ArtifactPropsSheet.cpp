// ArtifactPropsSheet.cpp - Loki h3maped object 100: the artifact property
// sheet. DoModal runs the artifact_props_dlg glade dialog, lets both pages
// read their widgets back and copies their modified values into the
// artifact. The assert and allocation check lines come from the retail
// immediates.
#include "editor/stdafx.h"

#include <string>

#include "exceptions.h"
#include "editor/cppbridge.h"
#include "editor/ArtifactPropsGeneralPage.h"
#include "editor/ArtifactPropsSheet.h"
#include "editor/ObjectSpecializations.h"
#include "editor/TreasurePropsGuardiansPage.h"

TArtifactPropsSheet* artifactPropsSheetModal = NULL;

TArtifactPropsSheet::TArtifactPropsSheet(void* pParent, TGameArtifact* pArtifact)
    : _m_pArtifact(pArtifact),
      _m_pGeneralPage(NULL),
      _m_pGuardiansPage(NULL)
{
#line 44
    assert(pArtifact != NULL);
    try {
        _m_pGeneralPage = new TArtifactPropsGeneralPage(*_m_pArtifact);
#line 51
        if (!_m_pGeneralPage)
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pGuardiansPage = new TTreasurePropsGuardiansPage(*_m_pArtifact, 0);
#line 60
        if (!_m_pGuardiansPage)
            throw TAllocationFailure(__FILE__, __LINE__);
    } catch (...) {
        _deleteAllPages();
        throw;
    }
}

TArtifactPropsSheet::~TArtifactPropsSheet()
{
    _deleteAllPages();
}

bool TArtifactPropsSheet::wasModified() const
{
    return _m_pGeneralPage->wasModified() || _m_pGuardiansPage->wasModified();
}

void TArtifactPropsSheet::_deleteAllPages()
{
    if (_m_pGuardiansPage)
        delete _m_pGuardiansPage;
    if (_m_pGeneralPage)
        delete _m_pGeneralPage;
}

void TArtifactPropsSheet::OnOK()
{
    _m_result = 1;
}

void TArtifactPropsSheet::OnCancel()
{
    _m_result = 2;
}

int TArtifactPropsSheet::DoModal()
{
    GtkNotebook* pNotebook = GTK_NOTEBOOK(_widget("artifact_props_notebook"));
    gtk_notebook_set_page(pNotebook, 0);

    _m_result = 0;
    artifactPropsSheetModal = this;
    GtkWidget* pDialog = _widget("artifact_props_dlg");
    gtk_widget_show(pDialog);
    while (_m_result == 0)
        gtk_main_iteration();
    gtk_widget_hide(pDialog);
    artifactPropsSheetModal = NULL;
    int result = _m_result;

    _m_pGeneralPage->OnOK();
    if (_m_pGeneralPage->wasModified())
        _m_pArtifact->setMessage(_m_pGeneralPage->getMessage());
    _m_pGuardiansPage->OnOK();
    if (_m_pGuardiansPage->wasModified()) {
        _m_pArtifact->setBCustomGuardians(_m_pGuardiansPage->getBCustomGuardians());
        _m_pArtifact->setGuardians(_m_pGuardiansPage->getGuardians());
    }
    return result;
}
