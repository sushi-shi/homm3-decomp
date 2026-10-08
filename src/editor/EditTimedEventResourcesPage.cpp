// EditTimedEventResourcesPage.cpp - Loki h3maped object 81: the resources
// page of the timed event sheet, a thin owner of TResourceQuantitiesDlg.
// The allocation check's line comes from the retail immediate.
#include "editor/stdafx.h"

#include "exceptions.h"
#include "editor/EditTimedEventResourcesPage.h"
#include "editor/ResourceQuantitiesDlg.h"
#include "editor/TimedEvent.h"

TEditTimedEventResourcesPage::TEditTimedEventResourcesPage(const TTimedEvent& event)
    : _m_pResourceQuantitiesDlg(NULL)
{
    _m_pResourceQuantitiesDlg = new TResourceQuantitiesDlg(event.getResourceQuantities());
#line 35
    if (!_m_pResourceQuantitiesDlg)
        throw TAllocationFailure(__FILE__, __LINE__);
    OnInitDialog();
}

TEditTimedEventResourcesPage::~TEditTimedEventResourcesPage()
{
    delete _m_pResourceQuantitiesDlg;
}

const TResourceQuantities& TEditTimedEventResourcesPage::getResourceQuantities() const
{
    return _m_pResourceQuantitiesDlg->getResourceQuantities();
}

bool TEditTimedEventResourcesPage::wasModified() const
{
    return _m_pResourceQuantitiesDlg->wasModified();
}

void TEditTimedEventResourcesPage::OnDestroy()
{
}

BOOL TEditTimedEventResourcesPage::OnInitDialog()
{
    _m_pResourceQuantitiesDlg->OnInitDialog();
    return true;
}

void TEditTimedEventResourcesPage::OnOK()
{
    _m_pResourceQuantitiesDlg->OnOK();
}
