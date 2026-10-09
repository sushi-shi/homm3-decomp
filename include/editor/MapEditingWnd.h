// MapEditingWnd.h - the map editing window interface (MapEditingWnd.cpp):
// an editing window (the map edit window) reports cursor, selection,
// object and brush events to its controller besides the view-rectangle
// moves. The Windows class adds an empty message map (0x539988, the
// viewing window's for a base). The controller's slot order is the
// vtables' (TMapView's and TMapFrameWnd's thunks name them).
#ifndef HOMM3_EDITOR_MAPEDITINGWND_H
#define HOMM3_EDITOR_MAPEDITINGWND_H

#include "editor/MapViewingWnd.h"

class TGameObject;
class TGUIGameObject;

class TMapEditingWnd : public TMapViewingWnd {
public:
    class TController;

protected:
    DECLARE_MESSAGE_MAP()
};

class TMapEditingWnd::TController : public TMapViewingWnd::TController {
public:
    virtual void onEditCursorTilePosChanged(TMapEditingWnd* pWnd, const CPoint& pos) = 0;
    virtual void onEditObjectSelected(TMapEditingWnd* pWnd, unsigned int objID) = 0;
    virtual void onEditEditObject(TMapEditingWnd* pWnd, unsigned int objID) = 0;
    virtual void onEditDeleteObject(TMapEditingWnd* pWnd, unsigned int objID) = 0;
    virtual void onEditGrabObject(TMapEditingWnd* pWnd, unsigned int objID) = 0;
    virtual void onEditCopyObject(TMapEditingWnd* pWnd, unsigned int objID) = 0;
    virtual void onEditPlaceObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj, unsigned int x,
                                   unsigned int y) = 0;
    virtual void onEditDiscardObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj) = 0;
    virtual void onEditPasteObject(TMapEditingWnd* pWnd, const TGameObject& obj, const CRect& rect) = 0;
    virtual void onEditBrushBeginDrag(TMapEditingWnd* pWnd, const CRect& rect) = 0;
    virtual void onEditBrushEndDrag(TMapEditingWnd* pWnd) = 0;
    virtual void onEditBrushDrag(TMapEditingWnd* pWnd, const CRect& rect) = 0;
    virtual void onEditFillRectAnchor(TMapEditingWnd* pWnd, const CRect& rect) = 0;
    virtual void onEditFillRectDrag(TMapEditingWnd* pWnd, const CRect& rect) = 0;
    virtual void onEditFillRectEndDrag(TMapEditingWnd* pWnd, const CRect& rect) = 0;
};

#endif  /* HOMM3_EDITOR_MAPEDITINGWND_H */
