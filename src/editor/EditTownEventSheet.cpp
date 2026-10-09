// EditTownEventSheet.cpp - the town event property sheet (h3maped
// 0x4181c2..0x418960; Loki h3maped object 97).
#include "editor/stdafx.h"

#include "va.h"
#include "editor/EditTownEventSheet.h"
#include "editor/MapEditorText.h"
#include "editor/ResourceQuantities.h"

VA(0x00418396, 0x2f7)
TEditTownEventSheet::TEditTownEventSheet(CWnd* pParentWnd, const TPlayerMask& playersPresent, TTownType townType,
                                         EGameVersion mapVersion, const TTown::TTimedEvent& event)
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
    _m_pBuildingsPage =
        std::auto_ptr<TEditTownEventBuildingsPage>(new TEditTownEventBuildingsPage(_m_event, townType));
    if (!_m_pBuildingsPage.get())
        throw TAllocationFailure();
    _m_pCreaturesPage =
        std::auto_ptr<TEditTownEventCreaturesPage>(new TEditTownEventCreaturesPage(_m_event, townType));
    if (!_m_pCreaturesPage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pResourcesPage.get());
    AddPage(_m_pBuildingsPage.get());
    AddPage(_m_pCreaturesPage.get());
}

VA_COMPGEN(0x0041868d, 0xf, IMPLICIT_DTOR, TTimedEvent)
VA_COMPGEN(0x004186cc, 0x1c, SCALAR_DELETING_DTOR, TEditTownEventSheet)

VA(0x004186e8, 0xbd)
TEditTownEventSheet::~TEditTownEventSheet()
{
    RemovePage(_m_pCreaturesPage.get());
    RemovePage(_m_pBuildingsPage.get());
    RemovePage(_m_pResourcesPage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x004187a5, 0x6)
BEGIN_MESSAGE_MAP(TEditTownEventSheet, CPropertySheet)
END_MESSAGE_MAP()

VA(0x004187ab, 0x1a1)
int TEditTownEventSheet::DoModal()
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
    if (_m_pBuildingsPage->wasModified())
        _m_event.setBuildMask(_m_pBuildingsPage->getBuildMask());
    if (_m_pCreaturesPage->wasModified())
        _m_event.setGeneratorBonuses(_m_pCreaturesPage->getGeneratorBonuses());
    return result;
}
