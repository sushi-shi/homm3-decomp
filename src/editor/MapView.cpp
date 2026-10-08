// MapView.cpp - Loki h3maped object 53: the map view, the controller that
// ties the document to the map frame, the mini map, the toolkit window and
// the status bar. It turns the edit windows' notifications into document
// operations, opens the property sheets, and carries the menu handlers.
// The view-wide settings (animation, zoom, grid, passability) reach every
// open view through allMapViews. Assert and throw lines come from the
// retail immediates.
#include "editor/stdafx.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <functional>
#include <string>
#include <gtk/gtk.h>

#include "exceptions.h"
#include "objnames.h"
#include "editor/cppbridge.h"
#include "editor/MapView.h"
#include "editor/MapDoc.h"
#include "editor/GameMap.h"
#include "editor/Event.h"
#include "editor/FindDlg.h"
#include "editor/GUIGameObject.h"
#include "editor/Hero.h"
#include "editor/Town.h"
#include "editor/HeroPropsSheet.h"
#include "editor/ArtifactPropsSheet.h"
#include "editor/MapEditorText.h"
#include "editor/MapFrameWnd.h"
#include "editor/MapSpecsSheet.h"
#include "editor/MapValidation.h"
#include "editor/MiniMapWnd.h"
#include "editor/Monster.h"
#include "editor/SignPropsDlg.h"
#include "editor/ToolkitWnd.h"
#include "editor/TownPropsSheet.h"

// The GTK status bar behind TMapView::TStatusUI: "<object> / <player> /
// <width>x<height> with[out] underground". Only inline members (linkonce
// bodies owned by this object); the member names are not proven.
class TStatusUIImpl : public CWnd, public TMapView::TStatusUI {
public:
    TStatusUIImpl(GtkWidget* statusBar);
    virtual ~TStatusUIImpl()
    {
        if (_m_objectName)
            g_free(_m_objectName);
        if (_m_playerName)
            g_free(_m_playerName);
    }

    void showObjectName()
    {
        _m_bShowObjectName = true;
        updateStatus();
    }
    void hideObjectName()
    {
        _m_bShowObjectName = false;
        updateStatus();
    }
    void setObjectName(const char* name)
    {
        _m_objectName = (char*)g_realloc(_m_objectName, strlen(name) + 1);
        strcpy(_m_objectName, name);
        updateStatus();
    }
    void setCurPlayer(TPlayer player)
    {
        if (player == ePlayerNone)
            _m_playerName = NULL;
        else {
            _m_playerName = (char*)g_realloc(_m_playerName, strlen(akPlayerTraits[player].m_pName) + 1);
            strcpy(_m_playerName, akPlayerTraits[player].m_pName);
        }
        updateStatus();
    }
    void setMapSize(unsigned int width, unsigned int height, bool bTwoLayer)
    {
        _m_mapWidth = width;
        _m_mapHeight = height;
        _m_bTwoLayer = bTwoLayer;
        updateStatus();
    }

private:
    void updateStatus()
    {
        const char* objectName = _m_objectName;
        const char* playerName = _m_playerName;
        if (_m_objectName == NULL || *_m_objectName == '\0')
            objectName = "No object selected";
        if (_m_playerName == NULL || *_m_playerName == '\0')
            playerName = "No player selected";
        char status[strlen(objectName) + strlen(playerName)
                    + strlen("%s / %s / %ux%u with%s underground") + 35];
        sprintf(status, "%s / %s / %ux%u with%s underground", objectName, playerName,
                _m_mapWidth, _m_mapHeight, _m_bTwoLayer ? "" : "out");
        gtk_statusbar_pop(GTK_STATUSBAR(_m_hWnd), _m_contextID);
        gtk_statusbar_push(GTK_STATUSBAR(_m_hWnd), _m_contextID, status);
    }

    guint _m_contextID;
    char* _m_objectName;
    char* _m_playerName;
    unsigned int _m_mapWidth;
    unsigned int _m_mapHeight;
    bool _m_bTwoLayer;
    bool _m_bShowObjectName;
};

// Defined after the class: its "default" follows updateStatus's texts.
inline TStatusUIImpl::TStatusUIImpl(GtkWidget* statusBar)
    : _m_objectName(NULL), _m_playerName(NULL), _m_mapWidth(0), _m_mapHeight(0),
      _m_bTwoLayer(false), _m_bShowObjectName(false)
{
    _m_hWnd = statusBar;
    _m_contextID = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusBar), "default");
    updateStatus();
}

namespace {

set<TMapView*> allMapViews;

}  // namespace

TZoom TMapView::_s_zoom = eZoom100;
bool TMapView::_s_bViewGrid = false;
bool TMapView::_s_bViewPassability = false;

TMapView::TMapView(GtkAdjustment* hAdjust, GtkAdjustment* vAdjust)
    : _m_pStatusUI(NULL),
      _m_pMapFrameWnd(NULL),
      _m_pMiniMapWnd(NULL),
      _m_pToolkitWnd(NULL),
      _m_viewPos(0, 0),
      _m_bViewUnderground(false),
      _m_mode(_eModeStartup),
      _m_objectSlot(eSlotDirt),
      _m_brush(_eBrush2x2),
      _m_terrainType(eTerrainDirt),
      _m_riverType(TRiverType(1)),
      _m_roadType(TRoadType(1)),
      _m_currentPlayer(ePlayerNone),
      _m_pFloatingObj(NULL),
      _m_bFloatingObjFromMap(false),
      _m_floatingObjX(0),
      _m_floatingObjY(0),
      _m_lastFindType(NOTHING),
      _m_lastFindSubtype(-1)
{
#line 397
    assert(hAdjust != NULL);
    assert(vAdjust != NULL);
    assert(hAdjust != vAdjust);
    allMapViews.insert(this);
    m_pDocument = new TMapDoc(NULL, this);
    GtkWidget* statusBar = _widget("statusbar");
    _m_pStatusUI = new TStatusUIImpl(statusBar);
    OnInitialUpdate(hAdjust, vAdjust);
}

TMapView::~TMapView()
{
#line 414
    assert(_m_pToolkitWnd == NULL);
    assert(_m_pMiniMapWnd == NULL);
    assert(_m_pMapFrameWnd == NULL);
    allMapViews.erase(this);
}

void TMapView::animate(unsigned int frameNum, bool bForce)
{
    for (set<TMapView*>::const_iterator it = allMapViews.begin(); it != allMapViews.end(); ++it)
        (*it)->_animate(frameNum, bForce);
}

void TMapView::resetAnimation()
{
    for_each(allMapViews.begin(), allMapViews.end(), mem_fun(&TMapView::_resetAnimation));
}

void TMapView::setZoom(TZoom zoom)
{
    for_each(allMapViews.begin(), allMapViews.end(), bind2nd(mem_fun1(&TMapView::_setZoom), zoom));
    _s_zoom = zoom;
}

void TMapView::setViewGrid(bool bViewGrid)
{
    for_each(allMapViews.begin(), allMapViews.end(),
             bind2nd(mem_fun1(&TMapView::_setViewGrid), bViewGrid));
    _s_bViewGrid = bViewGrid;
}

bool TMapView::getViewGrid()
{
    return _s_bViewGrid;
}

void TMapView::setViewPassability(bool bViewPassability)
{
    for_each(allMapViews.begin(), allMapViews.end(),
             bind2nd(mem_fun1(&TMapView::_setViewPassability), bViewPassability));
    _s_bViewPassability = bViewPassability;
}

bool TMapView::getViewPassability()
{
    return _s_bViewPassability;
}

void TMapView::setStatusUI(TStatusUI* pStatusUI)
{
#line 519
    assert(_m_pStatusUI == NULL && pStatusUI != NULL);
    _m_pStatusUI = pStatusUI;
}

TMapDoc* TMapView::getPDocument()
{
#line 527
    assert(dynamic_cast< TMapDoc * >( m_pDocument ) != NULL);
    return m_pDocument;
}

const TMapDoc* TMapView::getPDocument() const
{
#line 534
    assert(dynamic_cast< TMapDoc const * >( m_pDocument ) != NULL);
    return m_pDocument;
}

bool TMapView::onToolkitCanCreateObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType)
{
#line 565
    assert(pToolkitWnd == _m_pToolkitWnd);
    return getPDocument()->getPMap()->canCreate(objType, _m_currentPlayer);
}

void TMapView::onToolkitGrabObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType)
{
#line 574
    assert(pToolkitWnd == _m_pToolkitWnd);
    TMapDoc* pDoc = getPDocument();
    TGameObject* pObject;
    try {
        pObject = pDoc->getPMap()->createObject(objType, _m_currentPlayer, operator new);
    } catch (const TCreateObjFailureTooManyInstancesOfTypeOnMap& e) {
        char msg[256];
        snprintf(msg, sizeof(msg) - 1, kMaxObjectsOfTypeFmtStr, e.getCap(),
                 akAdvObjectTypeTraits[e.getType()].m_name);
        MessageBeep(-1);
        MessageBox(msg);
        return;
    } catch (const TCreateObjFailureNoAvailableHeroesInClass& e) {
        MessageBeep(-1);
        MessageBox(kNoMoreClassHeroesStr);
        return;
    } catch (const TCreateObjFailureNoOwnerForHero& e) {
        MessageBeep(-1);
        MessageBox(kSelectPlayerBeforePlacingHeroStr);
        return;
    } catch (const TCreateObjFailureTooManyHeroesForPlayer& e) {
        char msg[256];
        snprintf(msg, sizeof(msg) - 1, kMaxHeroesPerPlayerFmtStr, 8);
        MessageBeep(-1);
        MessageBox(msg);
        return;
    } catch (const TCreateObjFailureHolyGrailAlreadyPlaced& e) {
        MessageBeep(-1);
        MessageBox(kGrailAlreadyPlacedStr);
        return;
    } catch (const TCreateObjectFailure& e) {
#line 623
        assert(false);
        MessageBeep(-1);
        MessageBox(e.what());
        return;
    }
    _m_pFloatingObj = dynamic_cast<TGUIGameObject*>(pObject);
#line 630
    assert(_m_pFloatingObj != NULL);
    _m_bFloatingObjFromMap = false;
    _m_pMapFrameWnd->grabObject(*_m_pFloatingObj);
}

void TMapView::onMoveMapViewRect(TMapViewingWnd* pWnd, const CPoint& pos)
{
    _m_viewPos = pos;
    if (_m_pMapFrameWnd != NULL) {
#line 645
        assert(_m_pMapFrameWnd != NULL);
        _m_pMapFrameWnd->moveViewRect(pos);
    }
    if (_m_pMiniMapWnd != NULL) {
#line 651
        assert(_m_pMiniMapWnd != NULL);
        _m_pMiniMapWnd->moveViewRect(pos);
    }
}

void TMapView::onSizeMapViewRect(TMapViewingWnd* pWnd, const CSize& size)
{
    if (_m_pMiniMapWnd != NULL) {
#line 661
        assert(_m_pMiniMapWnd != NULL);
        _m_pMiniMapWnd->sizeViewRect(size);
    }
}

void TMapView::onEditCursorTilePosChanged(TMapEditingWnd* pEditingWnd, const CPoint& pos)
{
#line 669
    assert(pEditingWnd == _m_pMapFrameWnd);
}

void TMapView::onEditObjectSelected(TMapEditingWnd* pEditingWnd, unsigned int objID)
{
#line 676
    assert(pEditingWnd == _m_pMapFrameWnd);
    if (objID != TGameMap::TLayer::s_kInvalidObjID) {
        const TGameMap* pMap = getPDocument()->getPMap();
        const TGameObject& object = pMap->getLayer(_m_bViewUnderground).getObject(objID);
#line 683
        assert(_m_pStatusUI != NULL);
        _m_pStatusUI->setObjectName(object.getTypeName().c_str());
    } else
        _m_pStatusUI->setObjectName("");
}

void TMapView::onEditEditObject(TMapEditingWnd* pEditingWnd, unsigned int objID)
{
#line 693
    assert(pEditingWnd == _m_pMapFrameWnd);
    TGameMap mapBackup(*getPDocument()->getPMap());
    TGameMap::TLayer* pLayer = getPDocument()->getPMap()->getPLayer(_m_bViewUnderground);
    TGUIGameObject* pObject = dynamic_cast<TGUIGameObject*>(pLayer->getPObject(objID));
#line 699
    assert(pObject != NULL);
    if (pObject->edit(this, objID)) {
        getPDocument()->backupMap(mapBackup);
        getPDocument()->onObjectPropertiesChanged(_m_bViewUnderground, objID);
    }
}

void TMapView::onEditDeleteObject(TMapEditingWnd* pEditingWnd, unsigned int objID)
{
#line 711
    assert(pEditingWnd == _m_pMapFrameWnd);
    getPDocument()->backupMap();
    getPDocument()->removeObject(_m_bViewUnderground, objID);
}

void TMapView::onEditGrabObject(TMapEditingWnd* pEditingWnd, unsigned int objID)
{
#line 721
    assert(pEditingWnd == _m_pMapFrameWnd);
    getPDocument()->backupMap();
    TGameMap::TLayer* pLayer = getPDocument()->getPMap()->getPLayer(_m_bViewUnderground);
    TGameObject* pObject = pLayer->getPObject(objID);
    _m_pFloatingObj = dynamic_cast<TGUIGameObject*>(pObject);
#line 729
    assert(_m_pFloatingObj != NULL);
    TTilePoint loc = pLayer->getObjectLoc(objID);
    _m_bFloatingObjFromMap = true;
    _m_floatingObjX = loc.x();
    _m_floatingObjY = loc.y();
    getPDocument()->floatObject(_m_bViewUnderground, objID);
    _m_pMapFrameWnd->grabObject(*_m_pFloatingObj);
}

void TMapView::onEditCopyObject(TMapEditingWnd* pEditingWnd, unsigned int objID)
{
#line 744
    assert(pEditingWnd == _m_pMapFrameWnd);
    const TGameMap::TLayer& layer = getPDocument()->getPMap()->getLayer(_m_bViewUnderground);
    const TGameObject& object = layer.getObject(objID);
#line 748
    assert(dynamic_cast< TGUIGameObject const * >( &object ) != NULL);
    TGameObject* pClone = object.clone(operator new);
    if (pClone == NULL)
#line 752
        throw TAllocationFailure(__FILE__, __LINE__);
    _m_pFloatingObj = dynamic_cast<TGUIGameObject*>(pClone);
#line 755
    assert(_m_pFloatingObj != NULL);
    _m_bFloatingObjFromMap = false;
    _m_pMapFrameWnd->grabObject(*_m_pFloatingObj);
}

void TMapView::onEditPlaceObject(TMapEditingWnd* pEditingWnd, const TGUIGameObject& obj,
                                 unsigned int x, unsigned int y)
{
#line 765
    assert(pEditingWnd == _m_pMapFrameWnd);
    assert(&obj == _m_pFloatingObj);
    if (_m_bFloatingObjFromMap) {
        try {
            getPDocument()->unfloatObject(_m_bViewUnderground, x, y);
        } catch (const TPlaceObjectFailure& e) {
            if (dynamic_cast<const TPlaceObjFailureHolyGrailTooCloseToEdge*>(&e) != NULL) {
                const char* msg = "!!!TEMP MESSAGE: kGrailPlacedTooCloseToEdgeFmtStr.";
                MessageBeep(-1);
                MessageBox(msg);
            } else if (dynamic_cast<const TPlaceObjFailureInvalidPlacement*>(&e) != NULL)
                MessageBeep(-1);
            try {
                getPDocument()->unfloatObject(_m_bViewUnderground, _m_floatingObjX, _m_floatingObjY);
            } catch (const TPlaceObjectFailure& e) {
                getPDocument()->getPMap()->removeFloatingObject(_m_bViewUnderground);
            }
        }
    } else {
        try {
            getPDocument()->backupMap();
            _placeObjectWithExceptionHandlingImpl(*_m_pFloatingObj, x, y);
        } catch (const TPlaceObjFailureHolyGrailTooCloseToEdge& e) {
            size_t len = strlen(kGrailPlacedTooCloseToEdgeFmtStr) + 100;
            char msg[len];
            snprintf(msg, len, kGrailPlacedTooCloseToEdgeFmtStr, 9);
            MessageBeep(-1);
            MessageBox(msg);
        } catch (const TPlaceObjFailureInvalidPlacement& e) {
            MessageBeep(-1);
        }
        delete _m_pFloatingObj;
    }
    _m_pFloatingObj = NULL;
}

void TMapView::onEditDiscardObject(TMapEditingWnd* pEditingWnd, const TGUIGameObject& obj)
{
#line 831
    assert(pEditingWnd == _m_pMapFrameWnd);
    assert(&obj == _m_pFloatingObj);
    if (_m_bFloatingObjFromMap)
        getPDocument()->getPMap()->removeFloatingObject(_m_bViewUnderground);
    else
        delete _m_pFloatingObj;
    _m_pFloatingObj = NULL;
}

void TMapView::onEditPasteObject(TMapEditingWnd* pEditingWnd, const TGameObject& obj,
                                 const CRect& rect)
{
#line 845
    assert(pEditingWnd == _m_pMapFrameWnd);
    getPDocument()->backupMap();
    unsigned int minX = rect.left + obj.getWidth() - 1;
    unsigned int minY = rect.top + obj.getHeight() - 1;
    unsigned int maxX = minX + rect.Width() - 1;
    unsigned int maxY = minY + rect.Height() - 1;
    unsigned int x = minX;
    unsigned int y = minY;
    try {
        _placeObjectWithExceptionHandlingImpl(obj, x, y);
        return;
    } catch (const TPlaceObjFailureInvalidPlacement& e) {
    }
    for (;;) {
        if (y + 1 < maxY && x > minX) {
            ++y;
            --x;
        } else {
            x = x + y - minY + 1;
            if (x < maxX)
                y = minY;
            else {
                y = minY + x - (maxX - 1);
                if (y >= maxY)
                    break;
                x = maxX - 1;
            }
        }
        try {
            _placeObjectImpl(obj, x, y);
            return;
        } catch (const TPlaceObjFailureInvalidPlacement& e) {
        }
    }
    MessageBeep(-1);
    MessageBox(kNowhereToPasteStr);
}

void TMapView::onEditBrushBeginDrag(TMapEditingWnd* pEditingWnd, const CRect& brushRect)
{
#line 910
    assert(pEditingWnd == _m_pMapFrameWnd);
    switch (_m_mode) {
    case _eModeTerrain:
        getPDocument()->startTerrainPlacementOp(_m_bViewUnderground, _m_terrainType);
        getPDocument()->terrainFill(brushRect.left, brushRect.top, brushRect.Width(),
                                    brushRect.Height());
        break;
    case _eModeRiver:
#line 920
        assert(brushRect.Width() == 1 && brushRect.Height() == 1);
        getPDocument()->startRiverPlacementOp(_m_bViewUnderground, _m_riverType, brushRect.left,
                                              brushRect.top);
        break;
    case _eModeRoad:
#line 925
        assert(brushRect.Width() == 1 && brushRect.Height() == 1);
        getPDocument()->startRoadPlacementOp(_m_bViewUnderground, _m_roadType, brushRect.left,
                                             brushRect.top);
        break;
    case _eModeErase:
        getPDocument()->startEraseOp(_m_bViewUnderground);
        getPDocument()->erase(brushRect.left, brushRect.top, brushRect.Width(), brushRect.Height());
        break;
    }
}

void TMapView::onEditBrushEndDrag(TMapEditingWnd* pEditingWnd)
{
#line 939
    assert(pEditingWnd == _m_pMapFrameWnd);
    assert(_m_pMiniMapWnd != NULL);
    switch (_m_mode) {
    case _eModeTerrain: {
        CWaitCursor wait;
        getPDocument()->endTerrainPlacementOp();
    }
        break;
    case _eModeRiver:
        getPDocument()->endRiverPlacementOp();
        break;
    case _eModeRoad:
        getPDocument()->endRoadPlacementOp();
        break;
    case _eModeErase:
        getPDocument()->endEraseOp();
        break;
    }
    _m_pMiniMapWnd->OnPaint();
}

void TMapView::onEditBrushDrag(TMapEditingWnd* pEditingWnd, const CRect& brushRect)
{
#line 971
    assert(pEditingWnd == _m_pMapFrameWnd);
    assert(_m_pMiniMapWnd != NULL);
    switch (_m_mode) {
    case _eModeTerrain:
        getPDocument()->terrainFill(brushRect.left, brushRect.top, brushRect.Width(),
                                    brushRect.Height());
        break;
    case _eModeRiver:
#line 981
        assert(brushRect.Width() == 1 && brushRect.Height() == 1);
        getPDocument()->placeRiver(brushRect.left, brushRect.top);
        break;
    case _eModeRoad:
#line 986
        assert(brushRect.Width() == 1 && brushRect.Height() == 1);
        getPDocument()->placeRoad(brushRect.left, brushRect.top);
        break;
    case _eModeErase:
        getPDocument()->erase(brushRect.left, brushRect.top, brushRect.Width(), brushRect.Height());
        break;
    }
    _m_pMiniMapWnd->paintTiles(brushRect);
}

void TMapView::onEditFillRectAnchor(TMapEditingWnd* pEditingWnd, const CRect& rect)
{
}

void TMapView::onEditFillRectDrag(TMapEditingWnd* pEditingWnd, const CRect& rect)
{
}

void TMapView::onEditFillRectEndDrag(TMapEditingWnd* pEditingWnd, const CRect& rect)
{
#line 1011
    assert(pEditingWnd == _m_pMapFrameWnd);
    switch (_m_mode) {
    case _eModeTerrain: {
        CWaitCursor wait;
        getPDocument()->startTerrainPlacementOp(_m_bViewUnderground, _m_terrainType);
        getPDocument()->terrainFill(rect.left, rect.top, rect.Width(), rect.Height());
        getPDocument()->endTerrainPlacementOp();
    }
        break;
    case _eModeErase: {
        CWaitCursor wait;
        getPDocument()->startEraseOp(_m_bViewUnderground);
        getPDocument()->erase(rect.left, rect.top, rect.Width(), rect.Height());
        getPDocument()->endEraseOp();
    }
        break;
    }
    _m_pMiniMapWnd->OnPaint();
}

bool TMapView::onEditProperties(TGameObject* pGameObject, unsigned int objID)
{
#line 1042
    assert(pGameObject != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pGameObject);
    doMessageBox("This object does not have any editable properties.");
    return false;
}

bool TMapView::onEditProperties(TNonRandomHero* pNonRandomHero, unsigned int objID)
{
#line 1056
    assert(pNonRandomHero != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pNonRandomHero);
    TGameMap* pMap = getPDocument()->getPMap();
    TPlayer oldOwner = pNonRandomHero->getOwner();
    THeroClass heroClass = pNonRandomHero->getClass();
    unsigned int oldProtoNum = pNonRandomHero->getProtoNum();
    TNonRandomHeroPropsSheet sheet(this, pNonRandomHero, pMap->getAvailableHeroOwnersMask(),
                                   pMap->getAvailableHeroesInClass(heroClass));
    sheet.OnInitDialog();
    sheet.DoModal();
    if (pNonRandomHero->getProtoNum() != oldProtoNum)
        pMap->onHeroProtoChanged(heroClass, oldProtoNum, pNonRandomHero->getProtoNum());
    if (pNonRandomHero->getOwner() != oldOwner)
        pMap->onHeroOwnerChanged(*pNonRandomHero, oldOwner);
    return sheet.wasModified();
}

bool TMapView::onEditProperties(TRandomHero* pRandomHero, unsigned int objID)
{
#line 1084
    assert(pRandomHero != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pRandomHero);
    TGameMap* pMap = getPDocument()->getPMap();
    TPlayer oldOwner = pRandomHero->getOwner();
    TRandomHeroPropsSheet sheet(this, pRandomHero, pMap->getAvailableHeroOwnersMask());
    sheet.DoModal();
    if (pRandomHero->getOwner() != oldOwner)
        pMap->onHeroOwnerChanged(*pRandomHero, oldOwner);
    return sheet.wasModified();
}

bool TMapView::onEditProperties(TPrison* pPrison, unsigned int objID)
{
#line 1104
    assert(pPrison != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pPrison);
    TGameMap* pMap = getPDocument()->getPMap();
    TArray<set<unsigned int>, kNumHeroClasses> availableHeroes;
    for (unsigned int heroClass = 0; heroClass < kNumHeroClasses; ++heroClass)
        availableHeroes[heroClass] = pMap->getAvailableHeroesInClass(THeroClass(heroClass));
    THeroClass oldHeroClass = pPrison->getClass();
    unsigned int oldProtoNum = pPrison->getProtoNum();
    TPrisonPropsSheet sheet(this, pPrison, availableHeroes);
    sheet.DoModal();
    if (pPrison->getClass() != oldHeroClass)
        pMap->onHeroClassChanged(oldHeroClass, oldProtoNum, pPrison->getClass(),
                                 pPrison->getProtoNum());
    else if (pPrison->getProtoNum() != oldProtoNum)
        pMap->onHeroProtoChanged(oldHeroClass, oldProtoNum, pPrison->getProtoNum());
    return sheet.wasModified();
}

bool TMapView::onEditProperties(TTown* pTown, unsigned int objID)
{
#line 1135
    assert(pTown != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pTown);
    TTownPropsSheet sheet(_widget("town_props_dlg"), pTown, getPDocument()->getPMap(),
                          _m_bViewUnderground, objID);
    sheet.DoModal();
    return sheet.wasModified();
}

bool TMapView::onEditProperties(TEvent* pEvent, unsigned int objID)
{
#line 1148
    assert(pEvent != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pEvent);
    const TGameMap* const pMap = getPDocument()->getPMap();
    TPlayerMask availablePlayers;
    for (unsigned int player = 0; player < kNumPlayers; ++player)
        availablePlayers[player] = pMap->isPlayerPresent(TPlayer(player));
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

bool TMapView::onEditProperties(TMonster* pMonster, unsigned int objID)
{
#line 1170
    assert(pMonster != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pMonster);
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

bool TMapView::onEditProperties(TFlaggableObject* pFlaggable, unsigned int objID)
{
#line 1185
    assert(pFlaggable != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pFlaggable);
    if (pFlaggable->getType() == MINE && pFlaggable->getExtra() > 6)
        return onEditProperties((TGameObject*)pFlaggable, objID);
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

bool TMapView::onEditProperties(TAbandonedMine* pAbandonedMine, unsigned int objID)
{
#line 1204
    assert(pAbandonedMine != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pAbandonedMine);
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

bool TMapView::onEditProperties(TGarrison* pGarrison, unsigned int objID)
{
#line 1219
    assert(pGarrison != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pGarrison);
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

bool TMapView::onEditProperties(TSign* pSign, unsigned int objID)
{
#line 1234
    assert(pSign != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pSign);
    TSignPropsDlg dlg(this, pSign);
    dlg.OnInitDialog();
    dlg.DoModal();
    return dlg.wasModified();
}

bool TMapView::onEditProperties(TGameArtifact* pArtifact, unsigned int objID)
{
#line 1248
    assert(pArtifact != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pArtifact);
    TArtifactPropsSheet sheet(this, pArtifact);
    sheet.DoModal();
    return sheet.wasModified();
}

bool TMapView::onEditProperties(TSpellScroll* pSpellScroll, unsigned int objID)
{
#line 1261
    assert(pSpellScroll != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pSpellScroll);
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

bool TMapView::onEditProperties(TGameResource* pResource, unsigned int objID)
{
#line 1276
    assert(pResource != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pResource);
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

bool TMapView::onEditProperties(TBlackBox* pBlackBox, unsigned int objID)
{
#line 1291
    assert(pBlackBox != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pBlackBox);
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

bool TMapView::onEditProperties(TScholar* pScholar, unsigned int objID)
{
#line 1306
    assert(pScholar != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pScholar);
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

bool TMapView::onEditProperties(TSeersHut* pSeersHut, unsigned int objID)
{
#line 1321
    assert(pSeersHut != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

bool TMapView::onEditProperties(THolyGrail* pHolyGrail, unsigned int objID)
{
#line 1336
    assert(pHolyGrail != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pHolyGrail);
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

bool TMapView::onEditProperties(TShrine* pShrine, unsigned int objID)
{
#line 1351
    assert(pShrine != NULL);
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
    assert(&getPDocument()->getPMap()->getLayer( _m_bViewUnderground ).getObject( objID ) == pShrine);
    g_warning("MapView.cpp: No propssheet to load.\n");
    return false;
}

void TMapView::_deleteAll()
{
    delete _m_pMiniMapWnd;
    _m_pMiniMapWnd = NULL;
    delete _m_pMapFrameWnd;
    _m_pMapFrameWnd = NULL;
}

void TMapView::_placeObjectImpl(const TGameObject& obj, unsigned int x, unsigned int y)
{
    TMapLayerObjectID placedObjID = getPDocument()->placeObject(_m_bViewUnderground, obj, x, y);
#line 1381
    assert(placedObjID != TGameMap::TLayer::s_kInvalidObjID);
    _m_pMapFrameWnd->selectObject(placedObjID);
}

void TMapView::_placeObjectWithExceptionHandlingImpl(const TGameObject& obj, unsigned int x,
                                                     unsigned int y)
{
    try {
        _placeObjectImpl(obj, x, y);
    } catch (const TPlaceObjFailureTooManyInstancesOfTypeOnMap& e) {
        const char* msg = kMaxObjectsOfTypeFmtStr;
        MessageBeep(-1);
        MessageBox(msg);
    } catch (const TPlaceObjFailureNoAvailableHeroesInClass& e) {
        MessageBeep(-1);
        MessageBox(kNoMoreClassHeroesStr);
    } catch (const TPlaceObjFailureTooManyHeroesForPlayer& e) {
        const char* msg = kMaxHeroesPerPlayerFmtStr;
        MessageBeep(-1);
        MessageBox(msg);
    } catch (const TPlaceObjFailureHolyGrailAlreadyPlaced& e) {
        MessageBeep(-1);
        MessageBox(kGrailAlreadyPlacedStr);
    }
}

int TMapView::_animate(unsigned int frameNum, bool bForce)
{
#line 1459
    assert(_m_pMapFrameWnd != NULL);
    _m_pMapFrameWnd->animate(frameNum, bForce);
    return 0;
}

int TMapView::_resetAnimation()
{
#line 1467
    assert(_m_pMapFrameWnd != NULL);
    _m_pMapFrameWnd->resetAnimation();
    return 0;
}

int TMapView::_setZoom(TZoom zoom)
{
#line 1475
    assert(_m_pMapFrameWnd != NULL);
    _m_pMapFrameWnd->setZoom(zoom);
    return 0;
}

int TMapView::_setViewGrid(bool bViewGrid)
{
#line 1483
    assert(_m_pMapFrameWnd != NULL);
    _m_pMapFrameWnd->showGrid(bViewGrid);
    return 0;
}

int TMapView::_setViewPassability(bool bViewPassability)
{
#line 1491
    assert(_m_pMapFrameWnd != NULL);
    _m_pMapFrameWnd->showPassability(bViewPassability);
    return 0;
}

void TMapView::_setViewUnderground(bool bNewViewUnderground)
{
#line 1499
    assert(bNewViewUnderground != _m_bViewUnderground);
    const TGameMap* pMap = getPDocument()->getPMap();
#line 1503
    assert(pMap != NULL);
    _m_bViewUnderground = bNewViewUnderground;
#line 1507
    assert(_m_pMapFrameWnd != NULL);
    _m_pMapFrameWnd->setMapLayer(pMap, _m_bViewUnderground);
    _m_pMapFrameWnd->moveViewRect(_m_viewPos);
#line 1512
    assert(_m_pMiniMapWnd != NULL);
    _m_pMiniMapWnd->setMapLayer(pMap, _m_bViewUnderground);
    _m_pMiniMapWnd->moveViewRect(_m_viewPos);
}

void TMapView::_setMode(_TMode newMode)
{
#line 1525
    assert(newMode != _eModeStartup);
    assert(_m_pToolkitWnd != NULL);
    assert(_m_pMapFrameWnd != NULL);
    if (newMode == _m_mode)
        return;
    switch (_m_mode) {
    case _eModeObjects:
        _m_pStatusUI->hideObjectName();
        break;
    }
    _m_mode = newMode;
    switch (_m_mode) {
    case _eModeTerrain:
        _m_pToolkitWnd->showTerrainToolkit();
        _realizeBrush();
        break;
    case _eModeRiver:
        _m_pToolkitWnd->showRiverToolkit();
        _m_pMapFrameWnd->brushMode(CSize(1, 1));
        break;
    case _eModeRoad:
        _m_pToolkitWnd->showRoadToolkit();
        _m_pMapFrameWnd->brushMode(CSize(1, 1));
        break;
    case _eModeErase:
        _m_pToolkitWnd->showEraseToolkit();
        _realizeBrush();
        break;
    case _eModeObjects:
        _m_pStatusUI->showObjectName();
        _m_pToolkitWnd->showObjectPalette(_m_objectSlot);
        _m_pMapFrameWnd->selectionMode();
        break;
    }
}

void TMapView::_setObjectSlot(TObjectSlot slot)
{
    _m_pToolkitWnd->showObjectPalette(_m_objectSlot = slot);
}

void TMapView::_setBrush(_TBrush brush)
{
    if (brush == _m_brush)
        return;
    _m_brush = brush;
    if (_m_mode == _eModeTerrain || _m_mode == _eModeErase)
        _realizeBrush();
}

void TMapView::_setTerrainType(TTerrainType terrainType)
{
    _m_terrainType = terrainType;
}

void TMapView::_setRiverType(TRiverType riverType)
{
    _m_riverType = riverType;
}

void TMapView::_setRoadType(TRoadType roadType)
{
    _m_roadType = roadType;
}

void TMapView::_setCurrentPlayer(TPlayer player)
{
    _m_currentPlayer = player;
#line 1615
    assert(_m_pToolkitWnd != NULL);
    _m_pToolkitWnd->setPlayer(_m_currentPlayer);
#line 1618
    assert(_m_pStatusUI != NULL);
    _m_pStatusUI->setCurPlayer(_m_currentPlayer);
}

void TMapView::_realizeBrush()
{
#line 1625
    assert(_m_mode == _eModeTerrain || _m_mode == _eModeErase);
    assert(_m_pMapFrameWnd != NULL);
    switch (_m_brush) {
    case _eBrush1x1:
        _m_pMapFrameWnd->brushMode(CSize(1, 1));
        break;
    case _eBrush2x2:
        _m_pMapFrameWnd->brushMode(CSize(2, 2));
        break;
    case _eBrush4x4:
        _m_pMapFrameWnd->brushMode(CSize(4, 4));
        break;
    case _eBrushFill:
        _m_pMapFrameWnd->fillMode();
        break;
    }
}

void TMapView::OnInitialUpdate(GtkAdjustment* hadj, GtkAdjustment* vadj)
{
    _m_bViewUnderground = false;
#line 1665
    assert(getPDocument() != NULL);
    const TGameMap* pMap = getPDocument()->getPMap();
#line 1668
    assert(_m_pStatusUI != NULL);
    _m_pStatusUI->setCurPlayer(_m_currentPlayer);
    _m_pStatusUI->setMapSize(pMap->getWidth(), pMap->getHeight(), pMap->isTwoLayer());
    if (_m_pMapFrameWnd == NULL) {
#line 1674
        assert(hadj != NULL);
        assert(vadj != NULL);
        assert(hadj != vadj);
        assert(_m_pMiniMapWnd == NULL);
        assert(_m_pToolkitWnd == NULL);
        try {
            if ((_m_pMapFrameWnd = new TMapFrameWnd(NULL, this, 14, pMap, false, _s_zoom, _s_bViewGrid,
                                                    _s_bViewPassability, hadj, vadj)) == NULL)
#line 1682
                throw TAllocationFailure(__FILE__, __LINE__);
            GtkWidget* miniMapWidget = _widget("minimapwnd");
            if ((_m_pMiniMapWnd = new TMiniMapWnd(miniMapWidget, this, pMap, false)) == NULL)
#line 1692
                throw TAllocationFailure(__FILE__, __LINE__);
            if ((_m_pToolkitWnd = new TToolkitWnd(NULL, this)) == NULL)
#line 1696
                throw TAllocationFailure(__FILE__, __LINE__);
        } catch (...) {
            _deleteAll();
            throw;
        }
        _setMode(_eModeTerrain);
    } else {
#line 1708
        assert(_m_pMapFrameWnd != NULL);
        assert(_m_pMiniMapWnd != NULL);
        assert(_m_pToolkitWnd != NULL);
        _m_pMapFrameWnd->setMapLayer(pMap, false);
        _m_pMiniMapWnd->setMapLayer(pMap, false);
    }
    _m_viewPos = CPoint(0, 0);
}

void TMapView::OnUpdate(TMapDoc* pSender, unsigned long hint)
{
    if (_m_pMapFrameWnd != NULL) {
        const TMapDoc::TNotification* pNotification = (const TMapDoc::TNotification*)hint;
        if (pNotification == NULL || pNotification->m_type == TMapDoc::TNotification::eUpdate) {
            CRect rect;
            if (pNotification != NULL) {
#line 1741
                assert(pNotification->m_pUpdateParams != NULL);
                if (pNotification->m_pUpdateParams->m_bSecondLayer != _m_bViewUnderground)
                    return;
                rect = pNotification->m_pUpdateParams->m_rect;
            } else {
                const TGameMap* pMap = getPDocument()->getPMap();
#line 1753
                assert(pMap != NULL);
                rect = CRect(0, 0, pMap->getWidth(), pMap->getHeight());
            }
#line 1757
            assert(_m_pMapFrameWnd != NULL);
            _m_pMapFrameWnd->update(rect);
#line 1760
            assert(_m_pMiniMapWnd != NULL);
            _m_pMiniMapWnd->update(rect);
        } else if (pNotification->m_type == TMapDoc::TNotification::eReplace) {
            if (_m_bViewUnderground && !getPDocument()->getPMap()->isTwoLayer())
                _setViewUnderground(false);
#line 1770
            assert(_m_pMapFrameWnd != NULL);
            _m_pMapFrameWnd->onUndo();
            const TGameMap* pMap = getPDocument()->getPMap();
            CRect rect(0, 0, pMap->getWidth(), pMap->getHeight());
#line 1777
            assert(_m_pMiniMapWnd != NULL);
            _m_pMiniMapWnd->update(rect);
        } else if (pNotification->m_type == TMapDoc::TNotification::eObjectRemoved) {
            if (pNotification->m_pObjectRemovedParams->m_bSecondLayer != _m_bViewUnderground)
                return;
#line 1786
            assert(_m_pMapFrameWnd != NULL);
            _m_pMapFrameWnd->onObjectRemoved(pNotification->m_pObjectRemovedParams->m_objID);
        } else if (pNotification->m_type == TMapDoc::TNotification::eClear) {
#line 1791
            assert(_m_pMapFrameWnd != NULL);
            _m_pMapFrameWnd->clearMap();
#line 1794
            assert(_m_pMiniMapWnd != NULL);
            _m_pMiniMapWnd->clearMap();
        }
    }
}

void TMapView::OnDestroy()
{
    if (_m_pMapFrameWnd != NULL) {
#line 1807
        assert(_m_pMiniMapWnd != NULL);
        assert(_m_pMapFrameWnd != NULL);
        _deleteAll();
    }
}

void TMapView::OnToolsBrush1x1()
{
    _setBrush(_eBrush1x1);
}

void TMapView::OnToolsBrush2x2()
{
    _setBrush(_eBrush2x2);
}

void TMapView::OnToolsBrush4x4()
{
    _setBrush(_eBrush4x4);
}

void TMapView::OnToolsBrushFill()
{
    _setBrush(_eBrushFill);
}

void TMapView::OnToolsErase()
{
    _setMode(_eModeErase);
}

void TMapView::OnToolsErase1x1()
{
    OnToolsErase();
    _setBrush(_eBrush1x1);
}

void TMapView::OnToolsErase2x2()
{
    OnToolsErase();
    _setBrush(_eBrush2x2);
}

void TMapView::OnToolsErase4x4()
{
    OnToolsErase();
    _setBrush(_eBrush4x4);
}

void TMapView::OnToolsEraseFill()
{
    OnToolsErase();
    _setBrush(_eBrushFill);
}

void TMapView::OnToolsMapSpecificationsOK(TMapSpecsSheet& sheet)
{
    TGameMap* pMap = getPDocument()->getPMap();
    TGameMap mapBackup(*pMap);
    sheet.OnOK();
    if (sheet.wasModified()) {
        getPDocument()->backupMap(mapBackup);
        getPDocument()->SetModifiedFlag();
        if (_m_bViewUnderground && !pMap->isTwoLayer())
            _setViewUnderground(false);
    }
}

void TMapView::OnToolsObjectsAllTerrain()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotAllTerrain);
}

void TMapView::OnToolsObjects()
{
    _setMode(_eModeObjects);
}

void TMapView::OnToolsObjectsArtifacts()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotArtifacts);
}

void TMapView::OnToolsObjectsDirt()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotDirt);
}

void TMapView::OnToolsObjectsGrass()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotGrass);
}

void TMapView::OnToolsObjectsHeroes()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotHeroes);
}

void TMapView::OnToolsObjectsLava()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotLava);
}

void TMapView::OnToolsObjectsMonsters()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotMonsters);
}

void TMapView::OnToolsObjectsRough()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotRough);
}

void TMapView::OnToolsObjectsSand()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotSand);
}

void TMapView::OnToolsObjectsSnow()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotSnow);
}

void TMapView::OnToolsObjectsSubterranean()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotSubterranean);
}

void TMapView::OnToolsObjectsSwamp()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotSwamp);
}

void TMapView::OnToolsObjectsTowns()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotTowns);
}

void TMapView::OnToolsObjectsTreasures()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotTreasures);
}

void TMapView::OnToolsObjectsWater()
{
    _setMode(_eModeObjects);
    _setObjectSlot(eSlotWater);
}

void TMapView::OnToolsRivers()
{
    _setMode(_eModeRiver);
}

void TMapView::OnToolsRiversClear()
{
    OnToolsRivers();
    _setRiverType(TRiverType(1));
}

void TMapView::OnToolsRiversIcy()
{
    OnToolsRivers();
    _setRiverType(TRiverType(2));
}

void TMapView::OnToolsRiversLava()
{
    OnToolsRivers();
    _setRiverType(TRiverType(4));
}

void TMapView::OnToolsRiversMuddy()
{
    OnToolsRivers();
    _setRiverType(TRiverType(3));
}

void TMapView::OnToolsRoads()
{
    _setMode(_eModeRoad);
}

void TMapView::OnToolsRoadsCobblestone()
{
    OnToolsRoads();
    _setRoadType(TRoadType(3));
}

void TMapView::OnToolsRoadsDirt()
{
    OnToolsRoads();
    _setRoadType(TRoadType(1));
}

void TMapView::OnToolsRoadsGravel()
{
    OnToolsRoads();
    _setRoadType(TRoadType(2));
}

void TMapView::OnToolsTerrain()
{
    _setMode(_eModeTerrain);
}

void TMapView::OnToolsTerrain1x1()
{
    OnToolsTerrain();
    _setBrush(_eBrush1x1);
}

void TMapView::OnToolsTerrain2x2()
{
    OnToolsTerrain();
    _setBrush(_eBrush2x2);
}

void TMapView::OnToolsTerrain4x4()
{
    OnToolsTerrain();
    _setBrush(_eBrush4x4);
}

void TMapView::OnToolsTerrainFill()
{
    OnToolsTerrain();
    _setBrush(_eBrushFill);
}

void TMapView::OnToolsTerrainDirt()
{
    OnToolsTerrain();
    _setTerrainType(eTerrainDirt);
}

void TMapView::OnToolsTerrainGrass()
{
    OnToolsTerrain();
    _setTerrainType(eTerrainGrass);
}

void TMapView::OnToolsTerrainLava()
{
    OnToolsTerrain();
    _setTerrainType(eTerrainLava);
}

void TMapView::OnToolsTerrainRough()
{
    OnToolsTerrain();
    _setTerrainType(eTerrainRough);
}

void TMapView::OnToolsTerrainSand()
{
    OnToolsTerrain();
    _setTerrainType(eTerrainSand);
}

void TMapView::OnToolsTerrainSnow()
{
    OnToolsTerrain();
    _setTerrainType(eTerrainSnow);
}

void TMapView::OnToolsTerrainSubterranean()
{
    OnToolsTerrain();
    _setTerrainType(eTerrainSubterranean);
}

void TMapView::OnToolsTerrainSwamp()
{
    OnToolsTerrain();
    _setTerrainType(eTerrainSwamp);
}

void TMapView::OnToolsTerrainWater()
{
    OnToolsTerrain();
    _setTerrainType(eTerrainWater);
}

void TMapView::OnToolsTerrainRock()
{
    OnToolsTerrain();
    _setTerrainType(eTerrainRock);
}

void TMapView::OnViewUnderground()
{
    if (_m_pFloatingObj == NULL) {
        if (getPDocument()->getPMap()->isTwoLayer())
            _setViewUnderground(!_m_bViewUnderground);
        else
#line 2589
            assert(!_m_bViewUnderground);
    }
}

void TMapView::OnPlayersNone()
{
    _setCurrentPlayer(ePlayerNone);
}

void TMapView::OnPlayersPlayer1()
{
    _setCurrentPlayer(TPlayer(0));
}

void TMapView::OnPlayersPlayer2()
{
    _setCurrentPlayer(TPlayer(1));
}

void TMapView::OnPlayersPlayer3()
{
    _setCurrentPlayer(TPlayer(2));
}

void TMapView::OnPlayersPlayer4()
{
    _setCurrentPlayer(TPlayer(3));
}

void TMapView::OnPlayersPlayer5()
{
    _setCurrentPlayer(TPlayer(4));
}

void TMapView::OnPlayersPlayer6()
{
    _setCurrentPlayer(TPlayer(5));
}

void TMapView::OnPlayersPlayer7()
{
    _setCurrentPlayer(TPlayer(6));
}

void TMapView::OnPlayersPlayer8()
{
    _setCurrentPlayer(TPlayer(7));
}

void TMapView::OnEditUndo()
{
    if (getPDocument()->canUndo()) {
        CWaitCursor wait;
        getPDocument()->undo();
    }
}

void TMapView::OnEditRedo()
{
    if (getPDocument()->canRedo()) {
        CWaitCursor wait;
        getPDocument()->redo();
    }
}

void TMapView::OnToolsValidateMap()
{
    TMapValidationFunc validate(*getPDocument()->getPMap());
    GtkWidget* widget = _widget("validation_text");
    guint length = gtk_text_get_length(GTK_TEXT(widget));
    if (length != 0)
        gtk_editable_delete_text(GTK_EDITABLE(widget), 0, length);
    gtk_text_set_point(GTK_TEXT(widget), 0);
    gtk_text_insert(GTK_TEXT(widget), NULL, NULL, NULL, validate().c_str(), -1);
    widget = _widget("validation_dlg");
    gtk_widget_show(widget);
}

void TMapView::OnEditFind()
{
    if (_m_mode == _eModeObjects) {
        int findType;
        int findExtra;
        TMapLayerObjectID objID = _m_pMapFrameWnd->getSelectedObjectID();
        if (objID == TGameMap::TLayer::s_kInvalidObjID) {
            findType = _m_lastFindType;
            findExtra = _m_lastFindSubtype;
        } else {
            const TGameMap::TLayer& layer = getPDocument()->getPMap()->getLayer(_m_bViewUnderground);
            const TGameObject& object = layer.getObject(objID);
            findType = object.getType();
            findExtra = object.getExtra();
        }
        _m_pFindDlg = new TFindDlg(findType, findExtra);
    }
}

void TMapView::OnEditFindNext()
{
    _m_pFindDlg->OnOK();
    _m_lastFindType = _m_pFindDlg->getFindType();
    _m_lastFindSubtype = _m_pFindDlg->getFindExtra();
    delete _m_pFindDlg;
    if (_m_mode == _eModeObjects && _m_lastFindType != NOTHING) {
        try {
#line 2846
            assert(_m_lastFindType > NOTHING && _m_lastFindType < MAX_EVENT_TYPE);
            const TGameMap* const pMap = getPDocument()->getPMap();
            bool abLayerHasObjects[2];
            abLayerHasObjects[0] =
                pMap->getLayer(0U).getFirstObjectID() != TGameMap::TLayer::s_kInvalidObjID;
            abLayerHasObjects[1] =
                pMap->isTwoLayer()
                && pMap->getLayer(1U).getFirstObjectID() != TGameMap::TLayer::s_kInvalidObjID;
            if (!abLayerHasObjects[0] && !abLayerHasObjects[1])
                throw false;
            unsigned int startLayer;
            TMapLayerObjectID startObjID;
            TMapLayerObjectID selectedObjID = _m_pMapFrameWnd->getSelectedObjectID();
            if (selectedObjID == TGameMap::TLayer::s_kInvalidObjID) {
                startLayer = abLayerHasObjects[1] ? 1 : 0;
                startObjID = pMap->getLayer(startLayer).getLastObjectID();
            } else {
                startLayer = _m_bViewUnderground ? 1 : 0;
                startObjID = selectedObjID;
            }
            unsigned int layer = startLayer;
            TMapLayerObjectID objID = startObjID;
            for (;;) {
                objID = pMap->getLayer(layer).getNextObjectID(objID);
                if (objID == TGameMap::TLayer::s_kInvalidObjID) {
                    do
                        layer = !layer;
                    while (!abLayerHasObjects[layer]);
                    objID = pMap->getLayer(layer).getFirstObjectID();
                }
                const TGameObject& object = pMap->getLayer(layer).getObject(objID);
                if (object.getType() == _m_lastFindType
                    && (_m_lastFindSubtype == -1 || object.getExtra() == _m_lastFindSubtype))
                    break;
                if (objID == startObjID && layer == startLayer)
                    throw false;
            }
            if ((layer != 0) != _m_bViewUnderground)
                _setViewUnderground(!_m_bViewUnderground);
            _m_pMapFrameWnd->selectObject(objID);
            _m_pMapFrameWnd->makeVisible(objID);
        } catch (bool) {
            MessageBox(kObjectNotFoundStr);
        }
    }
}

void TMapView::OnEditFindPrev()
{
    _m_pFindDlg->OnOK();
    _m_lastFindType = _m_pFindDlg->getFindType();
    _m_lastFindSubtype = _m_pFindDlg->getFindExtra();
    delete _m_pFindDlg;
    if (_m_mode == _eModeObjects && _m_lastFindType != NOTHING) {
        try {
#line 2928
            assert(_m_lastFindType > NOTHING && _m_lastFindType < MAX_EVENT_TYPE);
            const TGameMap* const pMap = getPDocument()->getPMap();
            bool abLayerHasObjects[2];
            abLayerHasObjects[0] =
                pMap->getLayer(0U).getFirstObjectID() != TGameMap::TLayer::s_kInvalidObjID;
            abLayerHasObjects[1] =
                pMap->isTwoLayer()
                && pMap->getLayer(1U).getFirstObjectID() != TGameMap::TLayer::s_kInvalidObjID;
            if (!abLayerHasObjects[0] && !abLayerHasObjects[1])
                throw false;
            unsigned int startLayer;
            TMapLayerObjectID startObjID;
            TMapLayerObjectID selectedObjID = _m_pMapFrameWnd->getSelectedObjectID();
            if (selectedObjID == TGameMap::TLayer::s_kInvalidObjID) {
                startLayer = abLayerHasObjects[0] ? 0 : 1;
                startObjID = pMap->getLayer(startLayer).getFirstObjectID();
            } else {
                startLayer = _m_bViewUnderground ? 1 : 0;
                startObjID = selectedObjID;
            }
            unsigned int layer = startLayer;
            TMapLayerObjectID objID = startObjID;
            for (;;) {
                objID = pMap->getLayer(layer).getPrevObjectID(objID);
                if (objID == TGameMap::TLayer::s_kInvalidObjID) {
                    do
                        layer = !layer;
                    while (!abLayerHasObjects[layer]);
                    objID = pMap->getLayer(layer).getLastObjectID();
                }
                const TGameObject& object = pMap->getLayer(layer).getObject(objID);
                if (object.getType() == _m_lastFindType
                    && (_m_lastFindSubtype == -1 || object.getExtra() == _m_lastFindSubtype))
                    break;
                if (objID == startObjID && layer == startLayer)
                    throw false;
            }
            if ((layer != 0) != _m_bViewUnderground)
                _setViewUnderground(!_m_bViewUnderground);
            _m_pMapFrameWnd->selectObject(objID);
            _m_pMapFrameWnd->makeVisible(objID);
        } catch (bool) {
            MessageBox(kObjectNotFoundStr);
        }
    }
}

void TMapView::updateMapStatus()
{
    TGameMap* pMap = getPDocument()->getPMap();
    _m_pStatusUI->setMapSize(pMap->getWidth(), pMap->getHeight(), pMap->isTwoLayer());
}
