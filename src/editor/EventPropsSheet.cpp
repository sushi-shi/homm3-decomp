// EventPropsSheet.cpp - the event property sheet (h3maped
// 0x419da1..0x41a5e4; GOG only). The sheet has no help button.
#include "editor/stdafx.h"

#include "objnames.h"
#include "va.h"
#include "editor/Event.h"
#include "editor/EventPropsSheet.h"
#include "editor/FormattedString.h"
#include "editor/MapEditorText.h"

VA(0x00419f75, 0x252)
TEventPropsSheet::TEventPropsSheet(CWnd* pParentWnd, TEvent* pEvent, const TPlayerMask& playersPresent,
                                   EGameVersion mapVersion)
    : CPropertySheet(TFormattedString(kObjectPropertiesCaptionFmtStr, akAdvObjectTypeTraits[EVENT].m_name),
                     pParentWnd),
      _m_pEvent(pEvent)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    m_psh.dwFlags &= ~PSH_HASHELP;
    _m_pGeneralPage = std::auto_ptr<TEventPropsGeneralPage>(
        new TEventPropsGeneralPage(_m_pEvent, playersPresent, mapVersion));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    _m_pGuardiansPage = std::auto_ptr<TTreasurePropsGuardiansPage>(
        new TTreasurePropsGuardiansPage(_m_pEvent, mapVersion, IDH_EVENT_GUARDIANS));
    if (!_m_pGuardiansPage.get())
        throw TAllocationFailure();
    _m_pContentsPage = std::auto_ptr<TBlackBoxPropsContentsPage>(
        new TBlackBoxPropsContentsPage(_m_pEvent, mapVersion, IDH_EVENT_CONTENTS));
    if (!_m_pContentsPage.get())
        throw TAllocationFailure();
    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pGuardiansPage.get());
    AddPage(_m_pContentsPage.get());
}

VA_COMPGEN(0x0041a1c7, 0x1c, SCALAR_DELETING_DTOR, TEventPropsSheet)

VA(0x0041a1e3, 0x88)
TEventPropsSheet::~TEventPropsSheet()
{
    RemovePage(_m_pContentsPage.get());
    RemovePage(_m_pGuardiansPage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x0041a26b, 0x34)
bool TEventPropsSheet::wasModified() const
{
    return _m_pGeneralPage->wasModified() || _m_pGuardiansPage->wasModified() || _m_pContentsPage->wasModified();
}

VA(0x0041a29f, 0x6)
BEGIN_MESSAGE_MAP(TEventPropsSheet, CPropertySheet)
END_MESSAGE_MAP()

VA(0x0041a2a5, 0x306)
int TEventPropsSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (_m_pGeneralPage->wasModified()) {
        _m_pEvent->setMessage(_m_pGeneralPage->getMessage());
        for (unsigned int player = 0; player < kNumPlayers; player++)
            _m_pEvent->setBAllowPlayer(TPlayer(player), _m_pGeneralPage->getBAllowPlayer(TPlayer(player)));
        _m_pEvent->setBAllowComputer(_m_pGeneralPage->getBAllowComputer());
        _m_pEvent->setBCancelAfterVisit(_m_pGeneralPage->getBCancelAfterVisit());
    }
    if (_m_pGuardiansPage->wasModified()) {
        _m_pEvent->setBCustomGuardians(_m_pGuardiansPage->getBCustomGuardians());
        _m_pEvent->setGuardians(_m_pGuardiansPage->getGuardians());
    }
    if (_m_pContentsPage->wasModified()) {
        _m_pEvent->getPContents()->setExperienceBonus(_m_pContentsPage->getExperienceBonus());
        _m_pEvent->getPContents()->setManaBonus(_m_pContentsPage->getManaBonus());
        _m_pEvent->getPContents()->setMoraleBonus(_m_pContentsPage->getMoraleBonus());
        _m_pEvent->getPContents()->setLuckBonus(_m_pContentsPage->getLuckBonus());
        _m_pEvent->getPContents()->setResourceQuantities(_m_pContentsPage->getResourceQuantities());
        _m_pEvent->getPContents()->setPrimarySkillBonuses(_m_pContentsPage->getPrimarySkillBonuses());
        _m_pEvent->getPContents()->setSecondarySkills(_m_pContentsPage->getSecondarySkills());
        _m_pEvent->getPContents()->setArtifacts(_m_pContentsPage->getArtifacts());
        _m_pEvent->getPContents()->setSpells(_m_pContentsPage->getSpells());
        _m_pEvent->getPContents()->setCreatureStacks(_m_pContentsPage->getCreatureStacks());
    }
    return result;
}
