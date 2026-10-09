// MiniMapWnd.h - the mini map (MiniMapWnd.cpp; Loki h3maped object 56):
// the whole map layer scaled to a fixed client size with the main view's
// rectangle drawn over it. The Windows window draws into a 16-bit DIB
// section back buffer at +0x60 and tints the obstacle area like the map
// edit window; the layout follows Loki's with the obstacle area after the
// map (0x7c bytes).
#ifndef HOMM3_EDITOR_MINIMAPWND_H
#define HOMM3_EDITOR_MINIMAPWND_H

#include "editor/DIBSection.h"
#include "editor/MapViewingWnd.h"

class TGameMap;
class TGameMapMask;

class TMiniMapWnd : public TMapViewingWnd {
public:
    TMiniMapWnd(CWnd* pParent, TMapViewingWnd::TController* pController, const TGameMap* pMap,
                const TGameMapMask* pObstacleMask, bool bSecondLayer);
    virtual ~TMiniMapWnd();

    void clearMap();
    void setMapLayer(const TGameMap* pMap, const TGameMapMask* pObstacleMask, bool bSecondLayer);
    void moveViewRect(const CPoint& pos);
    void sizeViewRect(const CSize& size);
    void update(const CRect& rect);

protected:
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnCaptureChanged(CWnd* pWnd);
    DECLARE_MESSAGE_MAP()

private:
    void _processDrag(const CPoint& point);

    static const CSize s_kClientSize;

    TMapViewingWnd::TController* _m_pController;
    const TGameMap* _m_pMap;
    const TGameMapMask* _m_pObstacleMask;
    bool _m_bSecondLayer;
    CPoint _m_viewPos;
    CSize _m_viewSize;
    bool _m_bDragging;
    T16bppDIBSection _m_backBuffer;
};

#endif  /* HOMM3_EDITOR_MINIMAPWND_H */
