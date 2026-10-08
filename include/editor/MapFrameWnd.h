// MapFrameWnd.h - the map frame (Loki MapFrameWnd.cpp): the map edit window
// and its two tile rulers. It is an editing window and its edit window's
// controller, forwarding the events to its own controller. Methods as the
// image declares them; return types from __PRETTY_FUNCTION__ texts or the
// retail bodies. The members are the controller (+0x10), the edit window
// (+0x14) and the rulers (+0x18, +0x1c), named by the asserts; the getters
// are in-class (strong after the static initialization).
#ifndef HOMM3_EDITOR_MAPFRAMEWND_H
#define HOMM3_EDITOR_MAPFRAMEWND_H

#include "editor/MapViewingWnd.h"
#include "editor/GameMap.h"
#include "editor/Tile.h"
#include "editor/MapEditingWnd.h"

class TMapEditWnd;
class TTileHRuler;
class TTileVRuler;
class TGUIGameObject;

class TMapFrameWnd : public TMapEditingWnd, private TMapEditingWnd::TController {
public:
    TMapFrameWnd(GtkWidget* thisWidget, TMapEditingWnd::TController* pController, int id,
                 const TGameMap* pMap, bool bSecondLayer, TZoom zoom, bool bShowGrid,
                 bool bShowPassability, GtkAdjustment* pHAdjustment, GtkAdjustment* pVAdjustment);
    virtual ~TMapFrameWnd();

    void clearMap();
    void setMapLayer(const TGameMap* pMap, bool bSecondLayer);
    void moveViewRect(const CPoint& pos);
    void update(const CRect& rect);
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
    void fillMode();
    CSize getMinWndSize() const;
    TMapLayerObjectID getSelectedObjectID() const;

    void onMoveMapViewRect(TMapViewingWnd* pWnd, const CPoint& pos);
    void onSizeMapViewRect(TMapViewingWnd* pWnd, const CSize& size);
    void onEditCursorTilePosChanged(TMapEditingWnd* pWnd, const CPoint& pos);
    void onEditObjectSelected(TMapEditingWnd* pWnd, unsigned int objID);
    void onEditEditObject(TMapEditingWnd* pWnd, unsigned int objID);
    void onEditDeleteObject(TMapEditingWnd* pWnd, unsigned int objID);
    void onEditGrabObject(TMapEditingWnd* pWnd, unsigned int objID);
    void onEditCopyObject(TMapEditingWnd* pWnd, unsigned int objID);
    void onEditPlaceObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj,
                           unsigned int x, unsigned int y);
    void onEditDiscardObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj);
    void onEditPasteObject(TMapEditingWnd* pWnd, const TGameObject& obj, const CRect& rect);
    void onEditBrushBeginDrag(TMapEditingWnd* pWnd, const CRect& rect);
    void onEditBrushEndDrag(TMapEditingWnd* pWnd);
    void onEditBrushDrag(TMapEditingWnd* pWnd, const CRect& rect);
    void onEditFillRectAnchor(TMapEditingWnd* pWnd, const CRect& rect);
    void onEditFillRectDrag(TMapEditingWnd* pWnd, const CRect& rect);
    void onEditFillRectEndDrag(TMapEditingWnd* pWnd, const CRect& rect);

    void OnSize(unsigned int type, int cx, int cy);

    TMapEditWnd* getMapEditWnd() { return _m_pEditWnd; }
    TTileHRuler* getHRuler() { return _m_pHRuler; }
    TTileVRuler* getVRuler() { return _m_pVRuler; }

private:
    void _deleteAll();

    TMapEditingWnd::TController* _m_pController;
    TMapEditWnd* _m_pEditWnd;
    TTileHRuler* _m_pHRuler;
    TTileVRuler* _m_pVRuler;
};

#endif  /* HOMM3_EDITOR_MAPFRAMEWND_H */
