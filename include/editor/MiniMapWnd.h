// MiniMapWnd.h - the mini map (Loki MiniMapWnd.cpp), a viewing window over
// one map layer. Methods as the image declares them; return types from
// __PRETTY_FUNCTION__ texts or the retail bodies. Data members are not
// declared yet (cppbridge.cpp allocates 0x34 bytes).
#ifndef HOMM3_EDITOR_MINIMAPWND_H
#define HOMM3_EDITOR_MINIMAPWND_H

#include "editor/MapViewingWnd.h"

class TGameMap;

class TMiniMapWnd : public TMapViewingWnd {
public:
    TMiniMapWnd(GtkWidget* thisWidget, TMapViewingWnd::TController* pController,
                const TGameMap* pMap, bool bUnderground);
    virtual ~TMiniMapWnd();

    void clearMap();
    void setMapLayer(const TGameMap* pMap, bool bUnderground);
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
};

#endif  /* HOMM3_EDITOR_MINIMAPWND_H */
