// MapEditingWnd.h - the map editing window interface (Loki h3maped; its
// type_info nodes are emitted by cppbridge.cpp): an editing window (the
// map edit window) reports cursor, selection, object and brush events to
// its controller besides the view-rectangle moves. The window declares no
// destructor (its vtable holds CWnd::OnCaptureChanged alone). The
// controller's slot order is the vtables' (TMapView's and TMapFrameWnd's
// thunks name them). The image's type names put this class after GameMap.h's
// in the map windows' objects, and the mini map's object has none of it,
// so it has a header of its own; the file name is not proven.
#ifndef HOMM3_EDITOR_MAPEDITINGWND_H
#define HOMM3_EDITOR_MAPEDITINGWND_H

#include "editor/stdafx.h"

#include "editor/MapViewingWnd.h"
#include "editor/GameMap.h"
#include "editor/Tile.h"

class TGameObject;
class TGUIGameObject;

class TMapEditingWnd : public TMapViewingWnd {
public:
    class TController;
};

class TMapEditingWnd::TController : public TMapViewingWnd::TController {
public:
    virtual void onEditCursorTilePosChanged(TMapEditingWnd* pWnd, const CPoint& pos) = 0;
    virtual void onEditObjectSelected(TMapEditingWnd* pWnd, unsigned int objID) = 0;
    virtual void onEditEditObject(TMapEditingWnd* pWnd, unsigned int objID) = 0;
    virtual void onEditDeleteObject(TMapEditingWnd* pWnd, unsigned int objID) = 0;
    virtual void onEditGrabObject(TMapEditingWnd* pWnd, unsigned int objID) = 0;
    virtual void onEditCopyObject(TMapEditingWnd* pWnd, unsigned int objID) = 0;
    virtual void onEditPlaceObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj,
                                   unsigned int x, unsigned int y) = 0;
    virtual void onEditDiscardObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj) = 0;
    virtual void onEditPasteObject(TMapEditingWnd* pWnd, const TGameObject& obj,
                                   const CRect& rect) = 0;
    virtual void onEditBrushBeginDrag(TMapEditingWnd* pWnd, const CRect& rect) = 0;
    virtual void onEditBrushEndDrag(TMapEditingWnd* pWnd) = 0;
    virtual void onEditBrushDrag(TMapEditingWnd* pWnd, const CRect& rect) = 0;
    virtual void onEditFillRectAnchor(TMapEditingWnd* pWnd, const CRect& rect) = 0;
    virtual void onEditFillRectDrag(TMapEditingWnd* pWnd, const CRect& rect) = 0;
    virtual void onEditFillRectEndDrag(TMapEditingWnd* pWnd, const CRect& rect) = 0;
};

#endif  /* HOMM3_EDITOR_MAPEDITINGWND_H */
