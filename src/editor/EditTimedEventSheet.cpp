// EditTimedEventSheet.cpp - the timed event property sheet (h3maped
// 0x416e02..0x41735d; Loki h3maped object 79).
#include "editor/stdafx.h"

#include "va.h"
#include "editor/EditTimedEventSheet.h"
#include "editor/MapEditorText.h"
#include "editor/ResourceQuantities.h"

VA(0x00416e1e, 0x1aa)
TEditTimedEventSheet::TEditTimedEventSheet(CWnd* pParentWnd, const TPlayerMask& playersPresent,
                                           EGameVersion mapVersion, const TTimedEvent& event)
    : CPropertySheet(kEditTimedEventSheetCaptionStr, pParentWnd),
      _m_event(event)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    _m_pGeneralPage = std::auto_ptr<TEditTimedEventGeneralPage>(
        new TEditTimedEventGeneralPage(this, _m_event, playersPresent, mapVersion));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    _m_pResourcesPage = std::auto_ptr<TEditTimedEventResourcesPage>(new TEditTimedEventResourcesPage(_m_event));
    if (!_m_pResourcesPage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pResourcesPage.get());
}

VA_COMPGEN(0x004170b4, 0x1c, SCALAR_DELETING_DTOR, TEditTimedEventSheet)

VA(0x004170d0, 0x85)
TEditTimedEventSheet::~TEditTimedEventSheet()
{
    RemovePage(_m_pResourcesPage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x00417155, 0x6)
BEGIN_MESSAGE_MAP(TEditTimedEventSheet, CPropertySheet)
END_MESSAGE_MAP()

VA(0x0041715b, 0x14e)
int TEditTimedEventSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (_m_pGeneralPage->wasModified()) {
        _m_event.setName(_m_pGeneralPage->getName());
        _m_event.setMessage(_m_pGeneralPage->getMessage());
        _m_event.setBApplyToHuman(_m_pGeneralPage->getBApplyToHuman());
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
