// MapFrameWnd.h - the map frame (MapFrameWnd.cpp; Loki h3maped object 56):
// the map edit window between its two tile rulers. The frame is itself an
// editing window to the map view and the edit window's controller, so it
// passes every edit window event on as its own and keeps the rulers'
// start tiles, zoom and highlight in step. Layout from the constructor
// (0x50 bytes): the controller interface at +0x3c, then Loki's members.
#ifndef HOMM3_EDITOR_MAPFRAMEWND_H
#define HOMM3_EDITOR_MAPFRAMEWND_H

#include "editor/GameMap.h"
#include "editor/MapEditingWnd.h"
#include "editor/Tile.h"

class TGUIGameObject;
class TMapEditWnd;
class TGameMapMask;
class TTileHRuler;
class TTileVRuler;

class TMapFrameWnd : public TMapEditingWnd, private TMapEditingWnd::TController {
public:
    TMapFrameWnd(CWnd* pParent, TMapEditingWnd::TController* pController, int id, const TGameMap* pMap,
                 const TGameMapMask* pObstacleMask, bool bSecondLayer, TZoom zoom, bool bShowGrid,
                 bool bShowPassability);
    virtual ~TMapFrameWnd();
    virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

    void clearMap();
    void setMapLayer(const TGameMap* pMap, const TGameMapMask* pObstacleMask, bool bSecondLayer);
    void moveViewRect(const CPoint& pos);
    void update(const CRect& rect);
    void setZoom(TZoom zoom);
    void showGrid(bool bShow);
    void showPassability(bool bShow);
    void selectObject(unsigned int objID);
    void grabObject(const TGUIGameObject* pObj);
    void onUndo();
    void onObjectRemoved(unsigned int objID);
    void animate(unsigned int frameNum, bool bForce);
    void resetAnimation();
    void makeVisible(unsigned int objID);
    void selectionMode();
    void brushMode(const CSize& size);
    void fillMode();
    TMapLayerObjectID getSelectedObjectID() const;

protected:
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    DECLARE_MESSAGE_MAP()

private:
    virtual void onMoveMapViewRect(TMapViewingWnd* pWnd, const CPoint& pos);
    virtual void onSizeMapViewRect(TMapViewingWnd* pWnd, const CSize& size);
    virtual void onEditCursorTilePosChanged(TMapEditingWnd* pWnd, const CPoint& pos);
    virtual void onEditObjectSelected(TMapEditingWnd* pWnd, unsigned int objID);
    virtual void onEditEditObject(TMapEditingWnd* pWnd, unsigned int objID);
    virtual void onEditDeleteObject(TMapEditingWnd* pWnd, unsigned int objID);
    virtual void onEditGrabObject(TMapEditingWnd* pWnd, unsigned int objID);
    virtual void onEditCopyObject(TMapEditingWnd* pWnd, unsigned int objID);
    virtual void onEditPlaceObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj, unsigned int x,
                                   unsigned int y);
    virtual void onEditDiscardObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj);
    virtual void onEditPasteObject(TMapEditingWnd* pWnd, const TGameObject& obj, const CRect& rect);
    virtual void onEditBrushBeginDrag(TMapEditingWnd* pWnd, const CRect& rect);
    virtual void onEditBrushEndDrag(TMapEditingWnd* pWnd);
    virtual void onEditBrushDrag(TMapEditingWnd* pWnd, const CRect& rect);
    virtual void onEditFillRectAnchor(TMapEditingWnd* pWnd, const CRect& rect);
    virtual void onEditFillRectDrag(TMapEditingWnd* pWnd, const CRect& rect);
    virtual void onEditFillRectEndDrag(TMapEditingWnd* pWnd, const CRect& rect);

    void _deleteAll();

    TMapEditingWnd::TController* _m_pController;
    TMapEditWnd* _m_pEditWnd;
    TTileHRuler* _m_pHRuler;
    TTileVRuler* _m_pVRuler;
};

#endif  /* HOMM3_EDITOR_MAPFRAMEWND_H */
