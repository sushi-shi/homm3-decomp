// MapSpecsSheet.cpp - Loki h3maped object 69: the map specifications sheet.
// OnInitDialog collects the towns (with their visiting heroes), heroes and
// monsters on both layers and creates the pages, deleting them all again if
// one fails; OnOK lets every page commit and copies what changed into the
// map, adding a rock-filled second layer or removing it. The assert and
// allocation-check lines come from the retail immediates.
#include "editor/stdafx.h"

#include <vector>

#include "exceptions.h"
#include "editor/MapSpecsLossCondPage.h"
#include "editor/GameMap.h"
#include "editor/MapSpecsSheet.h"
#include "editor/GameObject.h"
#include "editor/Hero.h"
#include "editor/Town.h"
#include "editor/MapDoc.h"
#include "editor/MapSpecsGeneralPage.h"
#include "editor/MapSpecsPlayerSpecsPage.h"
#include "editor/MapSpecsRumorsPage.h"
#include "editor/MapSpecsTeamsPage.h"
#include "editor/MapSpecsTimedEventsPage.h"
#include "editor/Monster.h"
#include "editor/TerrainPlacement.h"

namespace {
#include <gtk/gtk.h>
}

TMapSpecsSheet::TMapSpecsSheet(GtkWidget* thisWidget, TGameMap* pMap)
    : _m_pMap(pMap),
      _m_pGeneralPage(NULL),
      _m_pPlayerSpecsPage(NULL),
      _m_pTeamsPage(NULL),
      _m_pRumorsPage(NULL),
      _m_pTimedEventsPage(NULL),
      _m_pVictoryCondPage(NULL),
      _m_pLossCondPage(NULL)
{
#line 55
    assert(_m_pMap != NULL);
    assert(thisWidget != NULL);
    _m_hWnd = thisWidget;
}

void TMapSpecsSheet::OnInitDialog()
{
    GtkNotebook* notebook = GTK_NOTEBOOK(_widget("map_specs_notebook"));
    gtk_notebook_set_page(notebook, 0);
    unsigned int numLayers = _m_pMap->isTwoLayer() ? 2 : 1;
    for (unsigned int layerNum = 0; layerNum < numLayers; layerNum++) {
        const TGameMap::TLayer& layer = _m_pMap->getLayer(layerNum);
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            const TGameObject& obj = layer.getObject(*iter);
            const TTown* pTown = dynamic_cast<const TTown*>(&obj);
            if (pTown) {
                _m_aTownsOnMap.push_back(TMapObjectRef(layerNum != 0, *iter));
                if (pTown->getPVisitingHero())
                    _m_aHeroesOnMap.push_back(TMapObjectRef(layerNum != 0, *iter));
            } else {
                const THero* pHero = dynamic_cast<const THero*>(&obj);
                if (pHero)
                    _m_aHeroesOnMap.push_back(TMapObjectRef(layerNum != 0, *iter));
                else {
                    const TMonster* pMonster = dynamic_cast<const TMonster*>(&obj);
                    if (pMonster)
                        _m_aMonstersOnMap.push_back(TMapObjectRef(layerNum != 0, *iter));
                }
            }
        }
    }

    try {
        _m_pGeneralPage = new TMapSpecsGeneralPage(*_m_pMap);
        if (!_m_pGeneralPage)
#line 108
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pGeneralPage->OnInitDialog();
        _m_pPlayerSpecsPage = new TMapSpecsPlayerSpecsPage(*_m_pMap);
        if (!_m_pPlayerSpecsPage)
#line 118
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pPlayerSpecsPage->OnInitDialog();
        _m_pTeamsPage = new TMapSpecsTeamsPage(*_m_pMap);
        if (!_m_pTeamsPage)
#line 125
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pTeamsPage->OnInitDialog();
        _m_pRumorsPage = new TMapSpecsRumorsPage(*_m_pMap);
        if (!_m_pRumorsPage)
#line 132
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pRumorsPage->OnInitDialog();
        _m_pTimedEventsPage = new TMapSpecsTimedEventsPage(*_m_pMap);
        if (!_m_pTimedEventsPage)
#line 139
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pTimedEventsPage->OnInitDialog();
        _m_pLossCondPage = new TMapSpecsLossCondPage(*_m_pMap, _m_aTownsOnMap, _m_aHeroesOnMap);
        if (!_m_pLossCondPage)
#line 155
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pLossCondPage->OnInitDialog();
    } catch (...) {
        _deleteAllPages();
        throw;
    }
}

TMapSpecsSheet::~TMapSpecsSheet()
{
    _deleteAllPages();
}

bool TMapSpecsSheet::wasModified() const
{
    return _m_pGeneralPage->wasModified() || _m_pPlayerSpecsPage->wasModified() || _m_pTeamsPage->wasModified()
           || _m_pRumorsPage->wasModified() || _m_pTimedEventsPage->wasModified()
           || _m_pLossCondPage->wasModified();
}

void TMapSpecsSheet::_deleteAllPages()
{
    if (_m_pLossCondPage)
        delete _m_pLossCondPage;
    if (_m_pTimedEventsPage)
        delete _m_pTimedEventsPage;
    if (_m_pRumorsPage)
        delete _m_pRumorsPage;
    if (_m_pTeamsPage)
        delete _m_pTeamsPage;
    if (_m_pPlayerSpecsPage)
        delete _m_pPlayerSpecsPage;
    if (_m_pGeneralPage)
        delete _m_pGeneralPage;
}

int TMapSpecsSheet::OnOK()
{
    _m_pPlayerSpecsPage->OnOK();
    if (_m_pPlayerSpecsPage->wasModified())
        _m_pMap->setPlayers(_m_pPlayerSpecsPage->getPlayers());
    _m_pTeamsPage->OnOK();
    if (_m_pTeamsPage->wasModified())
        _m_pMap->setTeamInfo(_m_pTeamsPage->getTeamInfo());
    _m_pRumorsPage->OnOK();
    if (_m_pRumorsPage->wasModified())
        _m_pMap->setRumors(_m_pRumorsPage->getRumors());
    _m_pTimedEventsPage->OnOK();
    if (_m_pTimedEventsPage->wasModified())
        _m_pMap->setTimedEvents(_m_pTimedEventsPage->getTimedEvents());
    _m_pLossCondPage->OnOK();
    if (_m_pLossCondPage->wasModified())
        _m_pMap->setLossCondition(_m_pLossCondPage->getPLossCondition());
    _m_pGeneralPage->OnOK();
    if (_m_pGeneralPage->wasModified()) {
        _m_pMap->setName(_m_pGeneralPage->getName());
        _m_pMap->setDesc(_m_pGeneralPage->getDesc());
        _m_pMap->setDifficulty(_m_pGeneralPage->getDifficulty());
        if (_m_pGeneralPage->getBTwoLevelMap()) {
            if (!_m_pMap->isTwoLayer()) {
                _m_pMap->addSecondLayer();
                TTerrainPlacementOpClient client;
                TTerrainPlacementOp op(&client, _m_pMap, true, eTerrainRock, TMapDoc::getSpecialTileFrequency());
                op(0, 0, _m_pMap->getWidth(), _m_pMap->getHeight());
            }
        } else if (_m_pMap->isTwoLayer())
            _m_pMap->removeSecondLayer();
    }
    return 1;
}
