// MapSpecsSheet.cpp - the map specifications sheet (h3maped
// 0x475a1d..0x4765eb; Loki h3maped object 69). Its OK-holding interface
// folds with the hero definition sheet's (0x457b9c, 0x457bb8).
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Hero.h"
#include "editor/MapEditorText.h"
#include "editor/MapSpecsArtifactsPage.h"
#include "editor/MapSpecsAvailableHeroesPage.h"
#include "editor/MapSpecsGeneralPage.h"
#include "editor/MapSpecsLossCondPage.h"
#include "editor/MapSpecsRumorsPage.h"
#include "editor/MapSpecsSecSkillsPage.h"
#include "editor/MapSpecsSheet.h"
#include "editor/MapSpecsSpellsPage.h"
#include "editor/MapSpecsTeamsPage.h"
#include "editor/MapSpecsTimedEventsPage.h"
#include "editor/MapSpecsVictoryCondPage.h"
#include "editor/Monster.h"
#include "editor/Town.h"

VA(0x00475a1d, 0x902)
TMapSpecsSheet::TMapSpecsSheet(CWnd* pParentWnd, TGameMap* pMap, int terrainStrength)
    : CPropertySheet(kMapSpecsSheetCaptionStr, pParentWnd),
      _m_pMap(pMap),
      _m_numOKDisablers(0)
{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    _m_pNewMap = std::auto_ptr<TGameMap>(new TGameMap(*_m_pMap));
    if (!_m_pNewMap.get())
        throw TAllocationFailure();
    unsigned int numLayers = _m_pNewMap->isTwoLayer() ? 2 : 1;
    for (unsigned int layerNum = 0; layerNum < numLayers; layerNum++) {
        const TGameMap::TLayer& layer = _m_pNewMap->getLayer(layerNum);
        for (TGameMap::TLayer::TObjectIDIter it = layer.objectIDBegin(); it != layer.objectIDEnd(); ++it) {
            const TGameObject* pObject = layer.getPObject(*it);
            const TTown* pTown = dynamic_cast<const TTown*>(pObject);
            if (pTown != NULL) {
                bool bSecondLayer = layerNum != 0;
                _m_aTownsOnMap.push_back(TMapObjectRef(bSecondLayer, *it));
                if (pTown->getPVisitingHero() != NULL)
                    _m_aHeroesOnMap.push_back(TMapObjectRef(bSecondLayer, *it));
            } else if (dynamic_cast<const TBasicHero*>(pObject) != NULL)
                _m_aHeroesOnMap.push_back(TMapObjectRef(layerNum != 0, *it));
            else if (dynamic_cast<const TMonster*>(pObject) != NULL)
                _m_aMonstersOnMap.push_back(TMapObjectRef(layerNum != 0, *it));
        }
    }

    _m_pGeneralPage = std::auto_ptr<TMapSpecsGeneralPage>(
        new TMapSpecsGeneralPage(*_m_pMap, *_m_pNewMap, terrainStrength));
    if (!_m_pGeneralPage.get())
        throw TAllocationFailure();
    _m_pPlayerSpecsPage = std::auto_ptr<TMapSpecsPlayerSpecsPage>(
        new TMapSpecsPlayerSpecsPage(this, *_m_pMap, *_m_pNewMap));
    if (!_m_pPlayerSpecsPage.get())
        throw TAllocationFailure();
    _m_pTeamsPage = std::auto_ptr<TMapSpecsTeamsPage>(new TMapSpecsTeamsPage(*_m_pMap, *_m_pNewMap));
    if (!_m_pTeamsPage.get())
        throw TAllocationFailure();
    _m_pRumorsPage = std::auto_ptr<TMapSpecsRumorsPage>(new TMapSpecsRumorsPage(*_m_pMap, *_m_pNewMap));
    if (!_m_pRumorsPage.get())
        throw TAllocationFailure();
    _m_pTimedEventsPage = std::auto_ptr<TMapSpecsTimedEventsPage>(new TMapSpecsTimedEventsPage(*_m_pMap, *_m_pNewMap));
    if (!_m_pTimedEventsPage.get())
        throw TAllocationFailure();
    _m_pVictoryCondPage = std::auto_ptr<TMapSpecsVictoryCondPage>(new TMapSpecsVictoryCondPage(
        *_m_pMap, *_m_pNewMap, _m_aTownsOnMap, _m_aHeroesOnMap, _m_aMonstersOnMap));
    if (!_m_pVictoryCondPage.get())
        throw TAllocationFailure();
    _m_pLossCondPage = std::auto_ptr<TMapSpecsLossCondPage>(
        new TMapSpecsLossCondPage(*_m_pMap, *_m_pNewMap, _m_aTownsOnMap, _m_aHeroesOnMap));
    if (!_m_pLossCondPage.get())
        throw TAllocationFailure();
    _m_pAvailableHeroesPage = std::auto_ptr<TMapSpecsAvailableHeroesPage>(
        new TMapSpecsAvailableHeroesPage(*_m_pMap, *_m_pNewMap));
    if (!_m_pAvailableHeroesPage.get())
        throw TAllocationFailure();
    _m_pArtifactsPage = std::auto_ptr<TMapSpecsArtifactsPage>(new TMapSpecsArtifactsPage(*_m_pMap, *_m_pNewMap));
    if (!_m_pArtifactsPage.get())
        throw TAllocationFailure();
    _m_pSpellsPage = std::auto_ptr<TMapSpecsSpellsPage>(new TMapSpecsSpellsPage(*_m_pMap, *_m_pNewMap));
    if (!_m_pSpellsPage.get())
        throw TAllocationFailure();
    _m_pSecSkillsPage = std::auto_ptr<TMapSpecsSecSkillsPage>(new TMapSpecsSecSkillsPage(*_m_pMap, *_m_pNewMap));
    if (!_m_pSecSkillsPage.get())
        throw TAllocationFailure();

    AddPage(_m_pGeneralPage.get());
    AddPage(_m_pPlayerSpecsPage.get());
    AddPage(_m_pTeamsPage.get());
    AddPage(_m_pRumorsPage.get());
    AddPage(_m_pTimedEventsPage.get());
    AddPage(_m_pVictoryCondPage.get());
    AddPage(_m_pLossCondPage.get());
    AddPage(_m_pAvailableHeroesPage.get());
    AddPage(_m_pArtifactsPage.get());
    AddPage(_m_pSpellsPage.get());
    AddPage(_m_pSecSkillsPage.get());
}

VA_COMPGEN(0x0047631f, 0x1c, SCALAR_DELETING_DTOR, TMapSpecsSheet)

VA(0x0047633b, 0x1ae)
TMapSpecsSheet::~TMapSpecsSheet()
{
    RemovePage(_m_pSecSkillsPage.get());
    RemovePage(_m_pSpellsPage.get());
    RemovePage(_m_pArtifactsPage.get());
    RemovePage(_m_pAvailableHeroesPage.get());
    RemovePage(_m_pLossCondPage.get());
    RemovePage(_m_pVictoryCondPage.get());
    RemovePage(_m_pTimedEventsPage.get());
    RemovePage(_m_pRumorsPage.get());
    RemovePage(_m_pTeamsPage.get());
    RemovePage(_m_pPlayerSpecsPage.get());
    RemovePage(_m_pGeneralPage.get());
}

VA(0x004764e9, 0xa4)
bool TMapSpecsSheet::wasModified() const
{
    return _m_pGeneralPage->wasModified() || _m_pPlayerSpecsPage->wasModified() || _m_pTeamsPage->wasModified()
           || _m_pRumorsPage->wasModified() || _m_pTimedEventsPage->wasModified()
           || _m_pVictoryCondPage->wasModified() || _m_pLossCondPage->wasModified()
           || _m_pAvailableHeroesPage->wasModified() || _m_pArtifactsPage->wasModified()
           || _m_pSpellsPage->wasModified() || _m_pSecSkillsPage->wasModified();
}

VA(0x0047658d, 0x6)
BEGIN_MESSAGE_MAP(TMapSpecsSheet, CPropertySheet)
END_MESSAGE_MAP()

VA(0x00476593, 0x31)
int TMapSpecsSheet::DoModal()
{
    int result = CPropertySheet::DoModal();
    if (result == IDOK && wasModified())
        *_m_pMap = *_m_pNewMap;
    return result;
}

void TMapSpecsSheet::onEnableOK()
{
    if (--_m_numOKDisablers == 0)
        GetDlgItem(IDOK)->EnableWindow(TRUE);
}

void TMapSpecsSheet::onDisableOK()
{
    if (_m_numOKDisablers++ == 0)
        GetDlgItem(IDOK)->EnableWindow(FALSE);
}
