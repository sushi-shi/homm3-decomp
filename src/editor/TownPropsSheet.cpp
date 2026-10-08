// TownPropsSheet.cpp - Loki h3maped object 94: the town property sheet.
// DoModal runs the town_props_dlg glade dialog, lets each page read its
// widgets back and copies the modified values into the town, telling the
// map about visiting hero and owner changes. The assert and allocation
// check lines come from the retail immediates.
#include "editor/stdafx.h"

#include <bitset>

#include "exceptions.h"
#include "editor/cppbridge.h"
#include "editor/Hero.h"
#include "editor/GameMap.h"
#include "editor/MapObjectRef.h"
#include "editor/TownPropsSheet.h"
#include "editor/Town.h"
#include "editor/TownPropsBuildingsPage.h"
#include "editor/TownPropsGarrisonPage.h"
#include "editor/TownPropsGeneralPage.h"
#include "editor/TownPropsSpellsPage.h"
#include "editor/TownPropsTimedEventsPage.h"

TTownPropsSheet* townPropsSheetModal = NULL;

TTownPropsSheet::TTownPropsSheet(GtkWidget* thisWidget, TTown* pTown, TGameMap* pMap, bool bSecondLayer,
                                 unsigned int objID)
    : _m_pTown(pTown),
      _m_pMap(pMap),
      _m_bSecondLayer(bSecondLayer),
      _m_objID(objID),
      _m_pGeneralPage(NULL),
      _m_pGarrisonPage(NULL),
      _m_pBuildingsPage(NULL),
      _m_pSpellsPage(NULL),
      _m_result(0)
{
#line 56
    assert(thisWidget != NULL);
    assert(pTown != NULL);
    assert(pMap != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&pMap->getLayer( bSecondLayer ).getObject( objID ) == pTown);
    bool bIsMainTown = _m_pTown->getOwner() != ePlayerNone
                       && _m_pMap->getPlayers()[_m_pTown->getOwner()].getMainTownRef()
                              == TMapObjectRef(bSecondLayer, objID);
    _m_pGeneralPage = new TTownPropsGeneralPage(_m_pTown, *_m_pMap, bIsMainTown);
#line 70
    if (!_m_pGeneralPage)
        throw TAllocationFailure(__FILE__, __LINE__);
    try {
        _m_pGarrisonPage = new TTownPropsGarrisonPage(*_m_pTown);
#line 79
        if (!_m_pGarrisonPage)
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pBuildingsPage = new TTownPropsBuildingsPage(*_m_pTown);
#line 85
        if (!_m_pBuildingsPage)
            throw TAllocationFailure(__FILE__, __LINE__);
        _m_pSpellsPage = new TTownPropsSpellsPage(*_m_pTown);
#line 91
        if (!_m_pSpellsPage)
            throw TAllocationFailure(__FILE__, __LINE__);
        TPlayerMask playersPresent;
        for (unsigned int player = 0; player < kNumPlayers; player++)
            playersPresent[player] = _m_pMap->isPlayerPresent(TPlayer(player));
        _m_pTimedEventsPage = new TTownPropsTimedEventsPage(*_m_pTown, playersPresent);
#line 103
        if (!_m_pTimedEventsPage)
            throw TAllocationFailure(__FILE__, __LINE__);
    } catch (...) {
        _deleteAllPages();
        throw;
    }
}

TTownPropsSheet::~TTownPropsSheet()
{
    _deleteAllPages();
}

bool TTownPropsSheet::wasModified() const
{
    return _m_pGeneralPage->wasModified() || _m_pGarrisonPage->wasModified() || _m_pBuildingsPage->wasModified()
           || _m_pSpellsPage->wasModified() || _m_pTimedEventsPage->wasModified();
}

void TTownPropsSheet::_deleteAllPages()
{
    if (_m_pTimedEventsPage)
        delete _m_pTimedEventsPage;
    if (_m_pSpellsPage)
        delete _m_pSpellsPage;
    if (_m_pBuildingsPage)
        delete _m_pBuildingsPage;
    if (_m_pGarrisonPage)
        delete _m_pGarrisonPage;
    if (_m_pGeneralPage)
        delete _m_pGeneralPage;
}

void TTownPropsSheet::OnOK()
{
    _m_result = 1;
}

void TTownPropsSheet::OnCancel()
{
    _m_result = 2;
}

int TTownPropsSheet::DoModal()
{
    GtkNotebook* pNotebook = GTK_NOTEBOOK(_widget("town_props_notebook"));
    gtk_notebook_set_page(pNotebook, 0);

    _m_result = 0;
    townPropsSheetModal = this;
    GtkWidget* pDialog = _widget("town_props_dlg");
    gtk_widget_show(pDialog);
    while (_m_result == 0)
        gtk_main_iteration();
    gtk_widget_hide(pDialog);
    townPropsSheetModal = NULL;
    int result = _m_result;

    _m_pGeneralPage->OnOK();
    if (_m_pGeneralPage->wasModified()) {
        if (_m_pTown->getPVisitingHero()) {
            _m_pMap->onRemovingHero(*_m_pTown->getPVisitingHero());
            _m_pTown->setVisitingHero(NULL);
            if (_m_pGeneralPage->getBVisitingHeroRemoved())
                _m_pMap->onObjectRemoved();
        }
        TPlayer oldOwner = _m_pTown->getOwner();
        _m_pTown->setOwner(_m_pGeneralPage->getOwner());
        if (_m_pTown->getOwner() != oldOwner)
            _m_pMap->onTownOwnerChanged(*_m_pTown, _m_bSecondLayer, _m_objID, oldOwner);
        _m_pTown->setBCustomName(_m_pGeneralPage->getBCustomName());
        _m_pTown->setName(_m_pGeneralPage->getName());
        _m_pTown->setVisitingHero(_m_pGeneralPage->getPVisitingHero());
        if (_m_pTown->getPVisitingHero())
            _m_pMap->onHeroAdded(*_m_pTown->getPVisitingHero());
    }
    _m_pGarrisonPage->OnOK();
    if (_m_pGarrisonPage->wasModified()) {
        _m_pTown->setBCustomGarrison(_m_pGarrisonPage->getBCustomGarrison());
        _m_pTown->setGarrison(_m_pGarrisonPage->getGarrison());
        _m_pTown->setBGroupedFormation(_m_pGarrisonPage->getBGroupedFormation());
    }
    _m_pBuildingsPage->OnOK();
    if (_m_pBuildingsPage->wasModified()) {
        _m_pTown->setBCustomBuildings(_m_pBuildingsPage->getBCustomBuildings());
        _m_pTown->setBuildingStates(_m_pBuildingsPage->getBuildingStates());
    }
    _m_pSpellsPage->OnOK();
    if (_m_pSpellsPage->wasModified())
        _m_pTown->setDisabledSpellsMask(_m_pSpellsPage->getDisabledSpellsMask());
    if (_m_pTimedEventsPage->wasModified())
        _m_pTown->setTimedEvents(_m_pTimedEventsPage->getTimedEvents());
    return result;
}
