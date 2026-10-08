// MapViewingWnd.h - the map window interfaces (Loki h3maped; their
// type_info nodes are emitted by cppbridge.cpp). A viewing window (the
// mini map) reports view-rectangle moves to its controller; an editing
// window (the map edit window) also reports cursor, selection, object and
// brush events. Neither window class declares a destructor (their vtables
// hold CWnd::OnCaptureChanged alone). The controllers' slot order is the
// vtables' (TMapView's and TMapFrameWnd's thunks name them). The file name
// is not proven.
#ifndef HOMM3_EDITOR_MAPVIEWINGWND_H
#define HOMM3_EDITOR_MAPVIEWINGWND_H

#include "editor/stdafx.h"

class TGameObject;
class TGUIGameObject;

class TMapViewingWnd : public CWnd {
public:
    class TController {
    public:
        virtual void onMoveMapViewRect(TMapViewingWnd* pWnd, const CPoint& pos) = 0;
        virtual void onSizeMapViewRect(TMapViewingWnd* pWnd, const CSize& size) = 0;
    };
};

class TMapEditingWnd : public TMapViewingWnd {
public:
    class TController : public TMapViewingWnd::TController {
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
};

#endif  /* HOMM3_EDITOR_MAPVIEWINGWND_H */
