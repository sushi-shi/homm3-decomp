// MapView.h - the map view (Loki MapView.cpp): the editor's controller
// over the document, the map frame, the mini map and the toolkit window,
// and the menu handlers. Methods as the image declares them; return types
// from __PRETTY_FUNCTION__ texts or the retail bodies. The bases' order
// follows the thunk deltas (CWnd at +0, the editing controller at +0xc, the
// toolkit client at +0x10, the edit context at +0x14); the own virtuals
// are the destructor, OnInitialUpdate and OnUpdate. The view-wide settings
// (animate, zoom, grid, passability) are static: they walk every view
// (the file-static allMapViews set) through the per-view _set* members and
// keep _s_* statics. TStatusUI's slot order is TStatusUIImpl's thunks'.
//
// The data members (0x70 bytes, as cppbridge.cpp allocates) follow the
// constructor's initializers and their users; the assert texts name
// m_pDocument, _m_pStatusUI, _m_pMapFrameWnd, _m_pMiniMapWnd,
// _m_pToolkitWnd, _m_bViewUnderground, _m_mode, _m_pFloatingObj and
// _m_lastFindType. The other member names, the _TMode/_TBrush enumerators
// (_eModeStartup, _eModeTerrain and _eModeErase are named by asserts; the
// mode is 0 at construction and 5 for the object palette) and the
// floating-object origin's meaning are not proven.
//
// The status bar behind TStatusUI is MapView.cpp's TStatusUIImpl.
#ifndef HOMM3_EDITOR_MAPVIEW_H
#define HOMM3_EDITOR_MAPVIEW_H

#include "editor/stdafx.h"
#include "editor/MapEditingWnd.h"
#include "editor/Player.h"
#include "editor/ToolkitWnd.h"
#include "editor/GUIGameObject.h"
#include "adventureobjecttype.h"
#include "objecttype.h"
#include "terrain_type.h"

class TMapDoc;
class TMapFrameWnd;
class TMiniMapWnd;
class TMapSpecsSheet;
class TFindDlg;

class TMapView : public CWnd,
                 private TMapEditingWnd::TController,
                 private TToolkitWndClient,
                 private TGUIGameObject::TEditContext {
public:
    class TStatusUI {
    public:
        virtual void showObjectName() = 0;
        virtual void hideObjectName() = 0;
        virtual void setObjectName(const char* name) = 0;
        virtual void setCurPlayer(TPlayer player) = 0;
        virtual void setMapSize(unsigned int width, unsigned int height, bool bTwoLayer) = 0;
    };

    enum _TMode {
        _eModeStartup,
        _eModeTerrain,
        _eModeRiver,
        _eModeRoad,
        _eModeErase,
        _eModeObjects
    };

    enum _TBrush {
        _eBrush1x1,
        _eBrush2x2,
        _eBrush4x4,
        _eBrushFill
    };

    TMapView(GtkAdjustment* pHAdjustment, GtkAdjustment* pVAdjustment);
    virtual ~TMapView();
    virtual void OnInitialUpdate(GtkAdjustment* pHAdjustment, GtkAdjustment* pVAdjustment);
    virtual void OnUpdate(TMapDoc* pDoc, unsigned long hint);

    static void animate(unsigned int frameNum, bool bForce);
    static void resetAnimation();
    static void setZoom(TZoom zoom);
    static void setViewGrid(bool bViewGrid);
    static bool getViewGrid();
    static void setViewPassability(bool bViewPassability);
    static bool getViewPassability();
    void setStatusUI(TStatusUI* pStatusUI);

    TMapDoc* getPDocument();
    const TMapDoc* getPDocument() const;
    TMapFrameWnd* getMapFrameWnd() { return _m_pMapFrameWnd; }
    TMiniMapWnd* getMiniMapWnd() { return _m_pMiniMapWnd; }
    TObjectPaletteWnd* getObjPaletteWnd() { return _m_pToolkitWnd->getObjPal(); }
    void updateMapStatus();

    bool onToolkitCanCreateObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType);
    void onToolkitGrabObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType);

    void onMoveMapViewRect(TMapViewingWnd* pWnd, const CPoint& pos);
    void onSizeMapViewRect(TMapViewingWnd* pWnd, const CSize& size);
    void onEditCursorTilePosChanged(TMapEditingWnd* pWnd, const CPoint& pos);
    void onEditObjectSelected(TMapEditingWnd* pWnd, unsigned int objID);
    void onEditEditObject(TMapEditingWnd* pWnd, unsigned int objID);
    void onEditDeleteObject(TMapEditingWnd* pWnd, unsigned int objID);
    void onEditGrabObject(TMapEditingWnd* pWnd, unsigned int objID);
    void onEditCopyObject(TMapEditingWnd* pWnd, unsigned int objID);
    void onEditPlaceObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj,
                           unsigned int x, unsigned int y);
    void onEditDiscardObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj);
    void onEditPasteObject(TMapEditingWnd* pWnd, const TGameObject& obj, const CRect& rect);
    void onEditBrushBeginDrag(TMapEditingWnd* pWnd, const CRect& rect);
    void onEditBrushEndDrag(TMapEditingWnd* pWnd);
    void onEditBrushDrag(TMapEditingWnd* pWnd, const CRect& rect);
    void onEditFillRectAnchor(TMapEditingWnd* pWnd, const CRect& rect);
    void onEditFillRectDrag(TMapEditingWnd* pWnd, const CRect& rect);
    void onEditFillRectEndDrag(TMapEditingWnd* pWnd, const CRect& rect);

    bool onEditProperties(TGameObject* pObj, unsigned int objID);
    bool onEditProperties(TNonRandomHero* pObj, unsigned int objID);
    bool onEditProperties(TRandomHero* pObj, unsigned int objID);
    bool onEditProperties(TPrison* pObj, unsigned int objID);
    bool onEditProperties(TTown* pObj, unsigned int objID);
    bool onEditProperties(TEvent* pObj, unsigned int objID);
    bool onEditProperties(TMonster* pObj, unsigned int objID);
    bool onEditProperties(TFlaggableObject* pObj, unsigned int objID);
    bool onEditProperties(TAbandonedMine* pObj, unsigned int objID);
    bool onEditProperties(TGarrison* pObj, unsigned int objID);
    bool onEditProperties(TSign* pObj, unsigned int objID);
    bool onEditProperties(TGameArtifact* pObj, unsigned int objID);
    bool onEditProperties(TSpellScroll* pObj, unsigned int objID);
    bool onEditProperties(TGameResource* pObj, unsigned int objID);
    bool onEditProperties(TBlackBox* pObj, unsigned int objID);
    bool onEditProperties(TScholar* pObj, unsigned int objID);
    bool onEditProperties(TSeersHut* pObj, unsigned int objID);
    bool onEditProperties(THolyGrail* pObj, unsigned int objID);
    bool onEditProperties(TShrine* pObj, unsigned int objID);

    void OnDestroy();
    void OnToolsMapSpecificationsOK(TMapSpecsSheet& sheet);
    void OnToolsBrush1x1();
    void OnToolsBrush2x2();
    void OnToolsBrush4x4();
    void OnToolsBrushFill();
    void OnToolsErase();
    void OnToolsErase1x1();
    void OnToolsErase2x2();
    void OnToolsErase4x4();
    void OnToolsEraseFill();
    void OnToolsObjectsAllTerrain();
    void OnToolsObjects();
    void OnToolsObjectsArtifacts();
    void OnToolsObjectsDirt();
    void OnToolsObjectsGrass();
    void OnToolsObjectsHeroes();
    void OnToolsObjectsLava();
    void OnToolsObjectsMonsters();
    void OnToolsObjectsRough();
    void OnToolsObjectsSand();
    void OnToolsObjectsSnow();
    void OnToolsObjectsSubterranean();
    void OnToolsObjectsSwamp();
    void OnToolsObjectsTowns();
    void OnToolsObjectsTreasures();
    void OnToolsObjectsWater();
    void OnToolsRivers();
    void OnToolsRiversClear();
    void OnToolsRiversIcy();
    void OnToolsRiversLava();
    void OnToolsRiversMuddy();
    void OnToolsRoads();
    void OnToolsRoadsCobblestone();
    void OnToolsRoadsDirt();
    void OnToolsRoadsGravel();
    void OnToolsTerrain();
    void OnToolsTerrain1x1();
    void OnToolsTerrain2x2();
    void OnToolsTerrain4x4();
    void OnToolsTerrainFill();
    void OnToolsTerrainDirt();
    void OnToolsTerrainGrass();
    void OnToolsTerrainLava();
    void OnToolsTerrainRough();
    void OnToolsTerrainSand();
    void OnToolsTerrainSnow();
    void OnToolsTerrainSubterranean();
    void OnToolsTerrainSwamp();
    void OnToolsTerrainWater();
    void OnToolsTerrainRock();
    void OnViewUnderground();
    void OnPlayersNone();
    void OnPlayersPlayer1();
    void OnPlayersPlayer2();
    void OnPlayersPlayer3();
    void OnPlayersPlayer4();
    void OnPlayersPlayer5();
    void OnPlayersPlayer6();
    void OnPlayersPlayer7();
    void OnPlayersPlayer8();
    void OnEditUndo();
    void OnEditRedo();
    void OnToolsValidateMap();
    void OnEditFind();
    void OnEditFindNext();
    void OnEditFindPrev();

private:
    void _deleteAll();
    void _placeObjectImpl(const TGameObject& obj, unsigned int x, unsigned int y);
    void _placeObjectWithExceptionHandlingImpl(const TGameObject& obj, unsigned int x, unsigned int y);
    int _animate(unsigned int frameNum, bool bForce);
    int _resetAnimation();
    int _setZoom(TZoom zoom);
    int _setViewGrid(bool bViewGrid);
    int _setViewPassability(bool bViewPassability);
    void _setViewUnderground(bool bUnderground);
    void _setMode(_TMode mode);
    void _setObjectSlot(TObjectSlot slot);
    void _setBrush(_TBrush brush);
    void _setTerrainType(TTerrainType terrainType);
    void _setRiverType(TRiverType riverType);
    void _setRoadType(TRoadType roadType);
    void _setCurrentPlayer(TPlayer player);
    void _realizeBrush();

    static TZoom _s_zoom;
    static bool _s_bViewGrid;
    static bool _s_bViewPassability;

protected:
    TMapDoc* m_pDocument;

private:
    TStatusUI* _m_pStatusUI;
    TMapFrameWnd* _m_pMapFrameWnd;
    TMiniMapWnd* _m_pMiniMapWnd;
    TToolkitWnd* _m_pToolkitWnd;
    CPoint _m_viewPos;
    bool _m_bViewUnderground;
    _TMode _m_mode;
    TObjectSlot _m_objectSlot;
    _TBrush _m_brush;
    TTerrainType _m_terrainType;
    TRiverType _m_riverType;
    TRoadType _m_roadType;
    TPlayer _m_currentPlayer;
    TGUIGameObject* _m_pFloatingObj;
    bool _m_bFloatingObjFromMap;
    unsigned int _m_floatingObjX;
    unsigned int _m_floatingObjY;
    int _m_lastFindType;
    int _m_lastFindSubtype;
    TFindDlg* _m_pFindDlg;
};

#endif  /* HOMM3_EDITOR_MAPVIEW_H */
