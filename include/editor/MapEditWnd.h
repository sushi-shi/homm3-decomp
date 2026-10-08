// MapEditWnd.h - the map edit window (Loki MapEditWnd.cpp), the editing
// window over one map layer. Methods as the image declares them; return
// types from __PRETTY_FUNCTION__ texts or the retail bodies; the virtuals in
// vtable order (OnCaptureChanged, the destructor, getBDoHScroll,
// getBDoVScroll, onPan). The data members are in the constructor's
// initialization order (sizeof 0x114, TMapFrameWnd's new); the names the
// asserts give are _m_timers, _m_hAdjust, _m_vAdjust, _m_pController,
// _m_pMap, _m_bSecondLayer, _m_bDragging, _m_bAutoScrollOn, _m_mode,
// _m_selectedObjID, _m_pFloatingObj, _m_potentialGrabID and _m_bBrushOn;
// the others are named from their uses. The _TMode enumerators and
// _s_kNumModes come from the asserts, _drawSelectionFrame's default argument
// and the GdkDrawable parameters from __PRETTY_FUNCTION__. getSelectedObjectID
// and _getMapLayer are in-class (strong after the static initialization).
#ifndef HOMM3_EDITOR_MAPEDITWND_H
#define HOMM3_EDITOR_MAPEDITWND_H

#include "editor/MapViewingWnd.h"
#include "editor/GameMap.h"
#include "editor/Tile.h"
#include "editor/MapEditingWnd.h"

class TGUIGameObject;

class TMapEditWnd : public TMapEditingWnd {
public:
    enum _TMode {
        _eModeSel,
        _eModeBrush,
        _eModeFill
    };
    static const int _s_kNumModes = 3;

    TMapEditWnd(GtkWidget* thisWidget, TMapEditingWnd::TController* pController, int id,
                const TGameMap* pMap, bool bUnderground, TZoom zoom, bool bShowGrid,
                bool bShowPassability, GtkAdjustment* hAdjust, GtkAdjustment* vAdjust);
    virtual void OnCaptureChanged(GtkWidget* pWidget);
    virtual ~TMapEditWnd();
    virtual bool getBDoHScroll() const;
    virtual bool getBDoVScroll() const;
    virtual void onPan(int dx, int dy);

    void dropUnexpectedFloater();
    unsigned int getCurrentTime();
    UINT SetTimer(int id, unsigned int elapse, void* pTimerFunc);
    void clearMap();
    void setMapLayer(const TGameMap* pMap, bool bUnderground);
    void moveViewRect(const CPoint& pos);
    void update(const CRect& rect);
    void incZoom();
    void decZoom();
    void setZoom(TZoom zoom);
    void showGrid(bool bShow);
    void showPassability(bool bShow);
    void selectObject(unsigned int objID);
    void grabObject(const TGUIGameObject& obj);
    void onUndo();
    void onObjectRemoved(unsigned int objID);
    void animate(unsigned int frameNum, bool bForce);
    void resetAnimation();
    void makeVisible(unsigned int objID);
    void selectionMode();
    void brushMode(const CSize& size);
    void brushMode();
    void fillMode();
    CSize getMinWndSize() const;
    TMapLayerObjectID getSelectedObjectID() const { return _m_selectedObjID; }

    void OnSize(unsigned int type, int cx, int cy);
    void OnHScroll(unsigned int code);
    void OnVScroll(unsigned int code);
    void OnPaint();
    void OnPaint(CRect& rect);
    void OnMouseEnter();
    void OnMouseLeave();
    void OnMouseMove(unsigned int flags, CPoint point);
    void OnLButtonDown(unsigned int flags, CPoint point);
    void OnLButtonUp(unsigned int flags, CPoint point);
    void OnLButtonDblClk(unsigned int flags, CPoint point);
    int OnRButtonDown(unsigned int flags, CPoint point);
    void OnRButtonUp(unsigned int flags, CPoint point);
    void OnMButtonDown(unsigned int flags, CPoint point);
    gint OnTimer(unsigned int id);
    void OnEditProperties();
    void OnEditDelete();
    void OnLeft();
    void OnRight();
    void OnUp();
    void OnDown();
    void OnEditCut();
    void OnEditCopy();
    void OnEditPaste();
    void OnContextMenu(GtkWidget* pWidget, CPoint point);
    void OnDeferredScroll(unsigned long hPos, unsigned long vPos);

private:
    void _turnAutoScrollOn();
    void _turnAutoScrollOff();
    void _handleAutoScroll(const CPoint& point);
    void _setMode(_TMode mode);
    void _setCursor(GdkCursor* pCursor);
    void _setToolTipObj(unsigned int objID);
    void _clearToolTipObj();
    void _turnBrushOn();
    void _turnBrushOff();
    void _setBrushPos(const CPoint& pos);
    void _setBrushSize(const CSize& size);
    void _turnFillRectOff();
    void _setFillRectAnchor(const CPoint& pos);
    void _setFillRectDragPos(const CPoint& pos);
    void _drawSelectionFrame(GdkDrawable* drawable, GdkGC* gc, bool bErase = false);
    void _drawBrush(GdkDrawable* drawable, GdkGC* gc);
    void _paintRect(CRect& rect);
    void _drawMap(GdkGC* pGC, CRect& rect);
    void _generateMouseMove();
    TMapLayerObjectID _pickObject(const CPoint& point) const;
    CPoint _computeFloatingObjPos(const CPoint& point) const;
    CRect _computeFloatingObjRect(const CPoint& point) const;
    CSize _computeViewRectSize(int cx, int cy) const;
    CPoint _computeBrushPos(const CPoint& point) const;
    CRect _computeFillRect() const;
    CPoint _computeFillRectDragPos(const CPoint& point) const;
    const TGameMap::TLayer& _getMapLayer() const { return _m_pMap->getLayer(_m_bSecondLayer); }

    gint _m_timers[15];
    GtkAdjustment* _m_hAdjust;
    GtkAdjustment* _m_vAdjust;
    TMapEditingWnd::TController* _m_pController;
    int _m_id;
    const TGameMap* _m_pMap;
    bool _m_bSecondLayer;
    CPoint _m_viewPos;
    GdkImage* _m_pImage;
    unsigned int _m_frameNum;
    bool _m_bDragging;
    GdkCursor* _m_hCursor;
    bool _m_bMouseInside;
    CPoint _m_cursorTilePos;
    CPoint _m_scrollDelta;
    bool _m_bAutoScrollOn;
    CPoint _m_autoScrollPoint;
    unsigned int _m_autoScrollTime;
    int _m_hAutoScrollDir;
    unsigned int _m_hAutoScrollInterval;
    unsigned int _m_hAutoScrollDelay;
    int _m_vAutoScrollDir;
    unsigned int _m_vAutoScrollInterval;
    unsigned int _m_vAutoScrollDelay;
    _TMode _m_mode;
    TZoom _m_zoom;
    bool _m_bShowGrid;
    bool _m_bShowPassability;
    TMapLayerObjectID _m_selectedObjID;
    TMapLayerObjectID _m_toolTipObjID;
    const TGUIGameObject* _m_pFloatingObj;
    CPoint _m_floatingObjPos;
    TMapLayerObjectID _m_potentialGrabID;
    bool _m_bPotentialCopy;
    CPoint _m_potentialGrabPoint;
    bool _m_bBrushOn;
    CPoint _m_brushPos;
    CSize _m_brushSize;
    CPoint _m_fillRectAnchor;
    CPoint _m_fillRectDragPos;
    bool _m_bPanning;
    CPoint _m_panPoint;
};

#endif  /* HOMM3_EDITOR_MAPEDITWND_H */
