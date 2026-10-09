// MapViewingWnd.h - the map viewing window interface (MapViewingWnd.cpp):
// a viewing window (the mini map) reports view-rectangle moves to its
// controller. The Windows class is an MFC window with an empty message map
// of its own (0x53ef08, CWnd's for a base); its vtable (0x53f0e8) differs
// from CWnd's in that slot alone. The controller's slot order is the
// vtables' (TMapView's thunks name them).
#ifndef HOMM3_EDITOR_MAPVIEWINGWND_H
#define HOMM3_EDITOR_MAPVIEWINGWND_H

#include "editor/stdafx.h"

class TMapViewingWnd : public CWnd {
public:
    class TController;

protected:
    DECLARE_MESSAGE_MAP()
};

class TMapViewingWnd::TController {
public:
    virtual void onMoveMapViewRect(TMapViewingWnd* pWnd, const CPoint& pos) = 0;
    virtual void onSizeMapViewRect(TMapViewingWnd* pWnd, const CSize& size) = 0;
};

#endif  /* HOMM3_EDITOR_MAPVIEWINGWND_H */
