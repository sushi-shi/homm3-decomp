// MiniMapWnd.h - the mini map (Loki MiniMapWnd.cpp), a viewing window over
// one map layer, drawn through a back-buffer pixmap at a fixed client size.
// Methods as the image declares them; return types from __PRETTY_FUNCTION__
// texts or the retail bodies. The assert texts name _m_pController, _m_pMap
// and _m_bSecondLayer, and s_kClientSize is the image's symbol; the view
// rectangle, drag flag and back buffer names are not proven (TMapView's
// OnInitialUpdate allocates 0x30 bytes).
#ifndef HOMM3_EDITOR_MINIMAPWND_H
#define HOMM3_EDITOR_MINIMAPWND_H

#include "editor/MapViewingWnd.h"

class TGameMap;

class TMiniMapWnd : public TMapViewingWnd {
public:
    TMiniMapWnd(GtkWidget* thisWidget, TMapViewingWnd::TController* pController,
                const TGameMap* pMap, bool bSecondLayer);
    virtual ~TMiniMapWnd();

    void clearMap();
    void setMapLayer(const TGameMap* pMap, bool bSecondLayer);
    void moveViewRect(const CPoint& pos);
    void sizeViewRect(const CSize& size);
    void update(const CRect& rect);
    void paintTiles(const CRect& rect);

    void OnPaint();
    void OnPaint(const CRect& rect);
    void OnLButtonDown(unsigned int flags, CPoint point);
    void OnLButtonUp(unsigned int flags, CPoint point);
    void OnMouseEnter();
    void OnMouseMove(unsigned int flags, CPoint point);
    void OnSize(unsigned int type, int cx, int cy);
    void OnCaptureChanged(CWnd* pWnd);

private:
    void _processDrag(const CPoint& point);

    static const CSize s_kClientSize;

    TMapViewingWnd::TController* _m_pController;
    const TGameMap* _m_pMap;
    bool _m_bSecondLayer;
    CPoint _m_viewPos;
    CSize _m_viewSize;
    bool _m_bDragging;
    GdkPixmap* _m_pBackBuffer;
};

#endif  /* HOMM3_EDITOR_MINIMAPWND_H */
