// MapView.h - the map view (MapView.cpp; Loki h3maped object 53): the
// editor's controller over the document, the map frame, the mini map, the
// toolkit window and the status bar, and the Tools, Players, View and Edit
// menu handlers. The Windows view is an MFC CView (DECLARE_DYNCREATE, 0xd4
// bytes); the interfaces follow at +0x40 (the editing window's controller),
// +0x44 (the toolkit window's client) and +0x48 (the objects' edit
// context), as the RTTI's displacements say. The view-wide settings
// (zoom, grid, passability) are static and walk every view through the
// file-static allMapViews set.
//
// GOG adds the obstacle tool: a mask of the tiles to fill with obstacles,
// painted with sixteen brushes and filled by the random object placer. The
// view keeps the mask's own undo history, one list of masks per map
// revision of the document's undo queue, and follows the document's
// backups, undos and redos to switch between them. Erasing rivers and roads
// is a river and road type of 0 (the toolkits' erase buttons); the eraser
// keeps a brush of its own.
//
// The data members follow the constructor's initializers and their users;
// Loki's assert texts name m_pDocument, _m_pStatusUI, _m_pMapFrameWnd,
// _m_pMiniMapWnd, _m_pToolkitWnd, _m_bViewUnderground, _m_mode and
// _m_pFloatingObj. The other names and the enumerators other than
// _eModeStartup, _eModeTerrain and _eModeErase are not proven.
#ifndef HOMM3_EDITOR_MAPVIEW_H
#define HOMM3_EDITOR_MAPVIEW_H

#include "editor/stdafx.h"

#include <map>
#include <memory>
#include <vector>

#include "terrain_type.h"
#include "editor/GameMapMask.h"
#include "editor/GUIGameObject.h"
#include "editor/MapEditingWnd.h"
#include "editor/ObjectTypeTable.h"
#include "editor/Player.h"
#include "editor/Tile.h"
#include "editor/ToolkitWnd.h"

class TMapDoc;
class TMapFrameWnd;
class TMiniMapWnd;

class TMapView : public CView,
                 private TMapEditingWnd::TController,
                 private TToolkitWndClient,
                 private TGUIGameObject::TEditContext {
    DECLARE_DYNCREATE(TMapView)

public:
    // The status bar: the main frame implements it (h3maped 0x538ccc).
    class TStatusUI {
    public:
        virtual void showObjectName() = 0;
        virtual void hideObjectName() = 0;
        virtual void setObjectName(const char* name) = 0;
        virtual void setCurPlayer(TPlayer player) = 0;
        virtual void setMapSize(unsigned int width, unsigned int height, bool bTwoLayer) = 0;
    };

    virtual ~TMapView();
    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
    virtual void OnInitialUpdate();

    static void animate(unsigned int frameNum, bool bForce);
    static void resetAnimation();
    static void setZoom(TZoom zoom);
    static void setViewGrid(bool bViewGrid);
    static void setViewPassability(bool bViewPassability);
    void setStatusUI(TStatusUI* pStatusUI);
    TMapDoc* getPDocument();

protected:
    TMapView();

    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    virtual void OnDraw(CDC* pDC);

    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnToolsBrush1x1();
    afx_msg void OnUpdateToolsBrush1x1(CCmdUI* pCmdUI);
    afx_msg void OnToolsBrush2x2();
    afx_msg void OnUpdateToolsBrush2x2(CCmdUI* pCmdUI);
    afx_msg void OnToolsBrush4x4();
    afx_msg void OnUpdateToolsBrush4x4(CCmdUI* pCmdUI);
    afx_msg void OnToolsBrushFill();
    afx_msg void OnUpdateToolsBrushFill(CCmdUI* pCmdUI);
    afx_msg void OnToolsErase();
    afx_msg void OnUpdateToolsErase(CCmdUI* pCmdUI);
    afx_msg void OnToolsErase1x1();
    afx_msg void OnUpdateToolsErase1x1(CCmdUI* pCmdUI);
    afx_msg void OnToolsErase2x2();
    afx_msg void OnUpdateToolsErase2x2(CCmdUI* pCmdUI);
    afx_msg void OnToolsErase4x4();
    afx_msg void OnUpdateToolsErase4x4(CCmdUI* pCmdUI);
    afx_msg void OnToolsEraseFill();
    afx_msg void OnUpdateToolsEraseFill(CCmdUI* pCmdUI);
    afx_msg void OnToolsMapSpecifications();
    afx_msg void OnUpdateToolsMapSpecifications(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsAllTerrain();
    afx_msg void OnUpdateToolsObjectsAllTerrain(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsArtifacts();
    afx_msg void OnUpdateToolsObjectsArtifacts(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsDirt();
    afx_msg void OnUpdateToolsObjectsDirt(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsGrass();
    afx_msg void OnUpdateToolsObjectsGrass(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsHeroes();
    afx_msg void OnUpdateToolsObjectsHeroes(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsLava();
    afx_msg void OnUpdateToolsObjectsLava(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsMonsters();
    afx_msg void OnUpdateToolsObjectsMonsters(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsRough();
    afx_msg void OnUpdateToolsObjectsRough(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsSand();
    afx_msg void OnUpdateToolsObjectsSand(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsSnow();
    afx_msg void OnUpdateToolsObjectsSnow(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsSubterranean();
    afx_msg void OnUpdateToolsObjectsSubterranean(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsSwamp();
    afx_msg void OnUpdateToolsObjectsSwamp(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsTowns();
    afx_msg void OnUpdateToolsObjectsTowns(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsTreasures();
    afx_msg void OnUpdateToolsObjectsTreasures(CCmdUI* pCmdUI);
    afx_msg void OnToolsObjectsWater();
    afx_msg void OnUpdateToolsObjectsWater(CCmdUI* pCmdUI);
    afx_msg void OnToolsRivers();
    afx_msg void OnUpdateToolsRivers(CCmdUI* pCmdUI);
    afx_msg void OnToolsRiversClear();
    afx_msg void OnUpdateToolsRiversClear(CCmdUI* pCmdUI);
    afx_msg void OnToolsRiversIcy();
    afx_msg void OnUpdateToolsRiversIcy(CCmdUI* pCmdUI);
    afx_msg void OnToolsRiversLava();
    afx_msg void OnUpdateToolsRiversLava(CCmdUI* pCmdUI);
    afx_msg void OnToolsRiversMuddy();
    afx_msg void OnUpdateToolsRiversMuddy(CCmdUI* pCmdUI);
    afx_msg void OnToolsRoads();
    afx_msg void OnUpdateToolsRoads(CCmdUI* pCmdUI);
    afx_msg void OnToolsRoadsCobblestone();
    afx_msg void OnUpdateToolsRoadsCobblestone(CCmdUI* pCmdUI);
    afx_msg void OnToolsRoadsDirt();
    afx_msg void OnUpdateToolsRoadsDirt(CCmdUI* pCmdUI);
    afx_msg void OnToolsRoadsGravel();
    afx_msg void OnUpdateToolsRoadsGravel(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrain();
    afx_msg void OnUpdateToolsTerrain(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrain1x1();
    afx_msg void OnUpdateToolsTerrain1x1(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrain2x2();
    afx_msg void OnUpdateToolsTerrain2x2(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrain4x4();
    afx_msg void OnUpdateToolsTerrain4x4(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrainFill();
    afx_msg void OnUpdateToolsTerrainFill(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrainDirt();
    afx_msg void OnUpdateToolsTerrainDirt(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrainGrass();
    afx_msg void OnUpdateToolsTerrainGrass(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrainLava();
    afx_msg void OnUpdateToolsTerrainLava(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrainRough();
    afx_msg void OnUpdateToolsTerrainRough(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrainSand();
    afx_msg void OnUpdateToolsTerrainSand(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrainSnow();
    afx_msg void OnUpdateToolsTerrainSnow(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrainSubterranean();
    afx_msg void OnUpdateToolsTerrainSubterranean(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrainSwamp();
    afx_msg void OnUpdateToolsTerrainSwamp(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrainWater();
    afx_msg void OnUpdateToolsTerrainWater(CCmdUI* pCmdUI);
    afx_msg void OnToolsTerrainRock();
    afx_msg void OnUpdateToolsTerrainRock(CCmdUI* pCmdUI);
    afx_msg void OnViewUnderground();
    afx_msg void OnUpdateViewUnderground(CCmdUI* pCmdUI);
    afx_msg void OnPlayersNone();
    afx_msg void OnUpdatePlayersNone(CCmdUI* pCmdUI);
    afx_msg void OnPlayersPlayer1();
    afx_msg void OnUpdatePlayersPlayer1(CCmdUI* pCmdUI);
    afx_msg void OnPlayersPlayer2();
    afx_msg void OnUpdatePlayersPlayer2(CCmdUI* pCmdUI);
    afx_msg void OnPlayersPlayer3();
    afx_msg void OnUpdatePlayersPlayer3(CCmdUI* pCmdUI);
    afx_msg void OnPlayersPlayer4();
    afx_msg void OnUpdatePlayersPlayer4(CCmdUI* pCmdUI);
    afx_msg void OnPlayersPlayer5();
    afx_msg void OnUpdatePlayersPlayer5(CCmdUI* pCmdUI);
    afx_msg void OnPlayersPlayer6();
    afx_msg void OnUpdatePlayersPlayer6(CCmdUI* pCmdUI);
    afx_msg void OnPlayersPlayer7();
    afx_msg void OnUpdatePlayersPlayer7(CCmdUI* pCmdUI);
    afx_msg void OnPlayersPlayer8();
    afx_msg void OnUpdatePlayersPlayer8(CCmdUI* pCmdUI);
    afx_msg void OnEditUndo();
    afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
    afx_msg void OnEditRedo();
    afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
    afx_msg void OnToolsValidateMap();
    afx_msg void OnEditFind();
    afx_msg void OnUpdateEditFind(CCmdUI* pCmdUI);
    afx_msg void OnEditFindNext();
    afx_msg void OnUpdateEditFindNext(CCmdUI* pCmdUI);
    afx_msg void OnEditFindPrev();
    afx_msg void OnToolsObstacles();
    afx_msg void OnUpdateToolsObstacles(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesArea1x1();
    afx_msg void OnUpdateToolsObstaclesArea1x1(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesArea2x2();
    afx_msg void OnUpdateToolsObstaclesArea2x2(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesArea4x4();
    afx_msg void OnUpdateToolsObstaclesArea4x4(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesAreaFill();
    afx_msg void OnUpdateToolsObstaclesAreaFill(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesBorderedArea1x1();
    afx_msg void OnUpdateToolsObstaclesBorderedArea1x1(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesBorderedArea2x2();
    afx_msg void OnUpdateToolsObstaclesBorderedArea2x2(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesBorderedArea4x4();
    afx_msg void OnUpdateToolsObstaclesBorderedArea4x4(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesBorderedAreaFill();
    afx_msg void OnUpdateToolsObstaclesBorderedAreaFill(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesErase1x1();
    afx_msg void OnUpdateToolsObstaclesErase1x1(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesErase2x2();
    afx_msg void OnUpdateToolsObstaclesErase2x2(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesErase4x4();
    afx_msg void OnUpdateToolsObstaclesErase4x4(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesEraseFill();
    afx_msg void OnUpdateToolsObstaclesEraseFill(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesBorderedErase1x1();
    afx_msg void OnUpdateToolsObstaclesBorderedErase1x1(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesBorderedErase2x2();
    afx_msg void OnUpdateToolsObstaclesBorderedErase2x2(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesBorderedErase4x4();
    afx_msg void OnUpdateToolsObstaclesBorderedErase4x4(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesBorderedEraseFill();
    afx_msg void OnUpdateToolsObstaclesBorderedEraseFill(CCmdUI* pCmdUI);
    afx_msg void OnToolsObstaclesPlace();
    afx_msg void OnUpdateToolsObstaclesPlace(CCmdUI* pCmdUI);
    afx_msg void OnToolsRiversErase();
    afx_msg void OnUpdateToolsRiversErase(CCmdUI* pCmdUI);
    afx_msg void OnToolsRoadsErase();
    afx_msg void OnUpdateToolsRoadsErase(CCmdUI* pCmdUI);
    DECLARE_MESSAGE_MAP()

private:
    enum _TMode {
        _eModeStartup,
        _eModeTerrain,
        _eModeRiver,
        _eModeRoad,
        _eModeErase,
        _eModeObjects,
        _eModeObstacles
    };

    enum _TBrush {
        _eBrush1x1,
        _eBrush2x2,
        _eBrush4x4,
        _eBrushFill
    };

    // The obstacle brushes, each in the four brush shapes (the obstacle
    // brush tool bar's order): obstacles, obstacles with a clear border,
    // erasing, and erasing that leaves the obstacles around it a clear
    // border.
    enum _TObstacleBrush {
        _eObstacleBrushArea1x1,
        _eObstacleBrushArea2x2,
        _eObstacleBrushArea4x4,
        _eObstacleBrushAreaFill,
        _eObstacleBrushBorderedArea1x1,
        _eObstacleBrushBorderedArea2x2,
        _eObstacleBrushBorderedArea4x4,
        _eObstacleBrushBorderedAreaFill,
        _eObstacleBrushErase1x1,
        _eObstacleBrushErase2x2,
        _eObstacleBrushErase4x4,
        _eObstacleBrushEraseFill,
        _eObstacleBrushBorderedErase1x1,
        _eObstacleBrushBorderedErase2x2,
        _eObstacleBrushBorderedErase4x4,
        _eObstacleBrushBorderedEraseFill
    };

    // The obstacle mask's undo history of one map revision.
    typedef std::vector<TGameMapMask> _TObstacleMaskHistory;
    typedef std::map<unsigned int, _TObstacleMaskHistory, std::less<unsigned int> > _TObstacleMaskHistoryMap;

    virtual bool onToolkitCanCreateObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType);
    virtual void onToolkitGrabObject(TToolkitWnd* pToolkitWnd, const TObjectType& objType);

    virtual void onMoveMapViewRect(TMapViewingWnd* pWnd, const CPoint& pos);
    virtual void onSizeMapViewRect(TMapViewingWnd* pWnd, const CSize& size);
    virtual void onEditCursorTilePosChanged(TMapEditingWnd* pWnd, const CPoint& pos);
    virtual void onEditObjectSelected(TMapEditingWnd* pWnd, unsigned int objID);
    virtual void onEditEditObject(TMapEditingWnd* pWnd, unsigned int objID);
    virtual void onEditDeleteObject(TMapEditingWnd* pWnd, unsigned int objID);
    virtual void onEditGrabObject(TMapEditingWnd* pWnd, unsigned int objID);
    virtual void onEditCopyObject(TMapEditingWnd* pWnd, unsigned int objID);
    virtual void onEditPlaceObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj, unsigned int x,
                                   unsigned int y);
    virtual void onEditDiscardObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj);
    virtual void onEditPasteObject(TMapEditingWnd* pWnd, const TGameObject& obj, const CRect& rect);
    virtual void onEditBrushBeginDrag(TMapEditingWnd* pWnd, const CRect& rect);
    virtual void onEditBrushEndDrag(TMapEditingWnd* pWnd);
    virtual void onEditBrushDrag(TMapEditingWnd* pWnd, const CRect& rect);
    virtual void onEditFillRectAnchor(TMapEditingWnd* pWnd, const CRect& rect);
    virtual void onEditFillRectDrag(TMapEditingWnd* pWnd, const CRect& rect);
    virtual void onEditFillRectEndDrag(TMapEditingWnd* pWnd, const CRect& rect);

    virtual bool onEditProperties(TGameObject* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(THeroPlaceholder* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TNonRandomHero* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TRandomHero* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TPrison* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TTown* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TEvent* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TMonster* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TFlaggableObject* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TAbandonedMine* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TGarrison* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TSign* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TGameArtifact* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TSpellScroll* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TGameResource* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TBlackBox* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TScholar* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TSeersHut* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(THolyGrail* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TShrine* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TRandomGenerator* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TRandomlyAlignedGenerator* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TRandomlyLeveledGenerator* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TQuestGuard* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);
    virtual bool onEditProperties(TWitchHut* pObj, TGameMap* pMap, bool bSecondLayer, unsigned int objID);

    void _setTerrainBrush(_TBrush brush)
    {
        if (_m_brush != brush) {
            _m_brush = brush;
            if (_m_mode == _eModeTerrain)
                _realizeBrush(brush);
        }
    }
    void _setEraseBrush(_TBrush brush)
    {
        if (_m_eraseBrush != brush) {
            _m_eraseBrush = brush;
            if (_m_mode == _eModeErase)
                _realizeBrush(brush);
        }
    }
    void _setObstacleBrush(_TObstacleBrush brush)
    {
        if (_m_obstacleBrush != brush) {
            _m_obstacleBrush = brush;
            if (_m_mode == _eModeObstacles)
                _realizeObstacleBrush();
        }
    }
    void _setObjectSlot(TObjectSlot slot) { _m_pToolkitWnd->showObjectPalette(_m_objectSlot = slot); }

    void _deleteAll();
    void _placeObjectImpl(std::auto_ptr<TGameObject> pObj, unsigned int x, unsigned int y);
    void _placeObjectWithExceptionHandlingImpl(std::auto_ptr<TGameObject> pObj, unsigned int x, unsigned int y);
    void _layout(int cx, int cy);
    int _resetAnimation();
    int _setZoom(TZoom zoom);
    int _setViewGrid(bool bViewGrid);
    int _setViewPassability(bool bViewPassability);
    void _setViewUnderground(bool bViewUnderground);
    void _setMode(_TMode mode);
    void _setCurrentPlayer(TPlayer player);
    void _realizeBrush(_TBrush brush);
    void _realizeObstacleBrush();
    void _backupObstacleMask();
    void _paintObstacleArea(const CRect& rect);
    void _paintBorderedObstacleArea(const CRect& rect);
    void _eraseObstacleArea(const CRect& rect);
    void _eraseBorderedObstacleArea(const CRect& rect);

    static TZoom _s_zoom;
    static bool _s_bViewGrid;
    static bool _s_bViewPassability;

    TStatusUI* _m_pStatusUI;
    TMapFrameWnd* _m_pMapFrameWnd;
    TMiniMapWnd* _m_pMiniMapWnd;
    TToolkitWnd* _m_pToolkitWnd;
    unsigned int _m_revision;
    std::auto_ptr<TGameMapMask> _m_pObstacleMask;
    _TObstacleMaskHistory _m_obstacleMaskHistory;
    unsigned int _m_obstacleMaskHistoryPos;
    _TObstacleMaskHistoryMap _m_obstacleMaskHistories;
    CPoint _m_viewPos;
    bool _m_bViewUnderground;
    _TMode _m_mode;
    TObjectSlot _m_objectSlot;
    _TBrush _m_brush;
    TTerrainType _m_terrainType;
    int _m_riverType;
    int _m_roadType;
    _TBrush _m_eraseBrush;
    _TObstacleBrush _m_obstacleBrush;
    TPlayer _m_currentPlayer;
    const TGUIGameObject* _m_pFloatingObj;
    std::auto_ptr<TGameObject> _m_pNewFloatingObj;
    unsigned int _m_floatingObjX;
    unsigned int _m_floatingObjY;
    int _m_lastFindType;
};

#endif  /* HOMM3_EDITOR_MAPVIEW_H */
