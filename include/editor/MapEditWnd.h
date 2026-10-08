// MapEditWnd.h - the map edit window (Loki MapEditWnd.cpp), the editing
// window over one map layer. Methods as the image declares them; return
// types from __PRETTY_FUNCTION__ texts or the retail bodies; the virtuals in
// vtable order (OnCaptureChanged, the destructor, getBDoHScroll,
// getBDoVScroll, onPan). Data members, the _TMode enumerators and the
// file-static helpers are not declared yet; _getMapLayer's return type is
// assumed (the bodies use it as an address).
#ifndef HOMM3_EDITOR_MAPEDITWND_H
#define HOMM3_EDITOR_MAPEDITWND_H

#include "editor/MapViewingWnd.h"
#include "editor/GameMap.h"
#include "editor/Tile.h"

class TGUIGameObject;

class TMapEditWnd : public TMapEditingWnd {
public:
    enum _TMode {
    };

    TMapEditWnd(GtkWidget* thisWidget, TMapEditingWnd::TController* pController, int id,
                const TGameMap* pMap, bool bUnderground, TZoom zoom, bool bShowGrid,
                bool bShowPassability, GtkAdjustment* pHAdjustment, GtkAdjustment* pVAdjustment);
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
    TMapLayerObjectID getSelectedObjectID() const;

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
    void _drawSelectionFrame(GdkWindow* pWindow, GdkGC* pGC, bool bErase);
    void _drawBrush(GdkWindow* pWindow, GdkGC* pGC);
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
    const TGameMap::TLayer& _getMapLayer() const;
};

#endif  /* HOMM3_EDITOR_MAPEDITWND_H */
