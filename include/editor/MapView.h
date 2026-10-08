// MapView.h - the map view (Loki MapView.cpp): the editor's controller
// over the document, the map frame, the mini map and the toolkit window,
// and the menu handlers. Methods as the image declares them; return types
// from __PRETTY_FUNCTION__ texts or the retail bodies. The bases' order
// follows the thunk deltas (CWnd at +0, the editing controller at +0xc, the
// toolkit client at +0x10, the edit context at +0x14); the own virtuals
// are the destructor, OnInitialUpdate and OnUpdate. The view-wide settings
// (animate, zoom, grid, passability) are static: they walk every view
// (the file-static allMapViews set) through the per-view _set* members and
// keep _s_* statics. TStatusUI's slot order is TStatusUIImpl's thunks'
// (cppbridge.cpp). Data members are not declared yet (cppbridge.cpp
// allocates 0x70 bytes); m_pDocument is at +0x18, the map frame at +0x20,
// the mini map at +0x24 and the toolkit window at +0x28. The _TMode and
// _TBrush enumerators are not recovered.
#ifndef HOMM3_EDITOR_MAPVIEW_H
#define HOMM3_EDITOR_MAPVIEW_H

#include "editor/stdafx.h"
#include "editor/GameMap.h"
#include "editor/GUIGameObject.h"
#include "editor/MapViewingWnd.h"
#include "editor/Player.h"
#include "editor/Tile.h"
#include "editor/ToolkitWnd.h"
#include "objecttype.h"
#include "terrain_type.h"

class TMapDoc;
class TMapFrameWnd;
class TMiniMapWnd;
class TMapSpecsSheet;

class TMapView : public CWnd,
                 public TMapEditingWnd::TController,
                 public TToolkitWndClient,
                 public TGUIGameObject::TEditContext {
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
    };

    enum _TBrush {
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
    TMapFrameWnd* getMapFrameWnd();
    TMiniMapWnd* getMiniMapWnd();
    TObjectPaletteWnd* getObjPaletteWnd();
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
};

#endif  /* HOMM3_EDITOR_MAPVIEW_H */
