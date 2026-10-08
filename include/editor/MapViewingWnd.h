// MapViewingWnd.h - the map viewing window interface (Loki h3maped; its
// type_info nodes are emitted by cppbridge.cpp): a viewing window (the
// mini map) reports view-rectangle moves to its controller. The window
// declares no destructor (its vtable holds CWnd::OnCaptureChanged alone).
// The controller's slot order is the vtables' (TMapView's thunks name
// them). The image's type names put TMapViewingWnd before its controller,
// so the controller is defined after the class. The file name is not
// proven.
#ifndef HOMM3_EDITOR_MAPVIEWINGWND_H
#define HOMM3_EDITOR_MAPVIEWINGWND_H

#include "editor/stdafx.h"

class TMapViewingWnd : public CWnd {
public:
    class TController;
};

class TMapViewingWnd::TController {
public:
    virtual void onMoveMapViewRect(TMapViewingWnd* pWnd, const CPoint& pos) = 0;
    virtual void onSizeMapViewRect(TMapViewingWnd* pWnd, const CSize& size) = 0;
};

#endif  /* HOMM3_EDITOR_MAPVIEWINGWND_H */
