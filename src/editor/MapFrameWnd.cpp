// MapFrameWnd.cpp - Loki h3maped object 54: the map frame, the map edit
// window between its two tile rulers. Requests go to the edit window and
// the rulers; the edit window's events go on to the frame's controller with
// the frame as their window. Assert and throw lines come from the retail
// immediates.
#include "editor/stdafx.h"

#include <assert.h>
#include <string>

#include "exceptions.h"
#include "editor/MapFrameWnd.h"
#include "editor/GameMap.h"
#include "editor/MapEditWnd.h"
#include "editor/TileHRuler.h"
#include "editor/TileVRuler.h"

TMapFrameWnd::TMapFrameWnd(GtkWidget* thisWidget, TMapEditingWnd::TController* pController, int id,
                           const TGameMap* pMap, bool bSecondLayer, TZoom zoom, bool bShowGrid,
                           bool bShowPassability, GtkAdjustment* pHAdjustment, GtkAdjustment* pVAdjustment)
    : _m_pController(pController),
      _m_pEditWnd(NULL),
      _m_pHRuler(NULL),
      _m_pVRuler(NULL)
{
#line 48
    assert(pController != NULL);
    assert(pMap != NULL);
    assert(!bSecondLayer || pMap->isTwoLayer());
    _m_hWnd = NULL;
    try {
        GtkWidget* editWidget = _widget("mapeditwnd");
        if ((_m_pEditWnd = new TMapEditWnd(editWidget, this, id, pMap, bSecondLayer, zoom, bShowGrid,
                                           bShowPassability, pHAdjustment, pVAdjustment)) == NULL)
#line 69
            throw TAllocationFailure(__FILE__, __LINE__);
        GtkWidget* statusBarWidget = _widget("statusbar");
        GtkStyle* s = gtk_widget_get_style(statusBarWidget);
#line 78
        assert(s != NULL);
        GdkFont* font = s->font;
        assert(font != NULL);
        GtkWidget* hRulerWidget = _widget("hruler");
        if ((_m_pHRuler = new TTileHRuler(hRulerWidget, pMap->getWidth(), zoom, font)) == NULL)
#line 85
            throw TAllocationFailure(__FILE__, __LINE__);
        GtkWidget* vRulerWidget = _widget("vruler");
        if ((_m_pVRuler = new TTileVRuler(vRulerWidget, pMap->getHeight(), zoom, font)) == NULL)
#line 90
            throw TAllocationFailure(__FILE__, __LINE__);
    } catch (...) {
        _deleteAll();
        throw;
    }
    _m_pHRuler->setStartTile(0);
    _m_pVRuler->setStartTile(0);
}

TMapFrameWnd::~TMapFrameWnd()
{
    _deleteAll();
}

void TMapFrameWnd::clearMap()
{
#line 111
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->clearMap();
}

void TMapFrameWnd::setMapLayer(const TGameMap* pMap, bool bSecondLayer)
{
#line 118
    assert(pMap != NULL);
    assert(!bSecondLayer || pMap->isTwoLayer());
#line 121
    assert(_m_pHRuler != NULL);
    _m_pHRuler->setRange(pMap->getWidth());
#line 124
    assert(_m_pVRuler != NULL);
    _m_pVRuler->setRange(pMap->getHeight());
#line 127
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->setMapLayer(pMap, bSecondLayer);
}

void TMapFrameWnd::moveViewRect(const CPoint& pos)
{
#line 134
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->moveViewRect(pos);
#line 137
    assert(_m_pHRuler != NULL);
    _m_pHRuler->setStartTile(pos.x);
#line 140
    assert(_m_pVRuler != NULL);
    _m_pVRuler->setStartTile(pos.y);
}

void TMapFrameWnd::update(const CRect& rect)
{
#line 147
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->update(rect);
}

void TMapFrameWnd::setZoom(TZoom zoom)
{
#line 154
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->setZoom(zoom);
#line 157
    assert(_m_pHRuler != NULL);
    _m_pHRuler->setZoom(zoom);
#line 160
    assert(_m_pVRuler != NULL);
    _m_pVRuler->setZoom(zoom);
}

void TMapFrameWnd::showGrid(bool bShow)
{
#line 167
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->showGrid(bShow);
}

void TMapFrameWnd::showPassability(bool bShow)
{
#line 174
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->showPassability(bShow);
}

void TMapFrameWnd::selectObject(unsigned int objID)
{
#line 181
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->selectObject(objID);
}

void TMapFrameWnd::grabObject(const TGUIGameObject& obj)
{
#line 188
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->grabObject(obj);
}

void TMapFrameWnd::onUndo()
{
#line 195
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->onUndo();
}

void TMapFrameWnd::onObjectRemoved(unsigned int objID)
{
#line 202
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->onObjectRemoved(objID);
}

void TMapFrameWnd::animate(unsigned int frameNum, bool bForce)
{
#line 209
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->animate(frameNum, bForce);
}

void TMapFrameWnd::resetAnimation()
{
#line 216
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->resetAnimation();
}

void TMapFrameWnd::makeVisible(unsigned int objID)
{
#line 223
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->makeVisible(objID);
}

void TMapFrameWnd::selectionMode()
{
#line 230
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->selectionMode();
}

void TMapFrameWnd::brushMode(const CSize& size)
{
#line 237
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->brushMode(size);
}

void TMapFrameWnd::fillMode()
{
#line 244
    assert(_m_pEditWnd != NULL);
    _m_pEditWnd->fillMode();
}

CSize TMapFrameWnd::getMinWndSize() const
{
#line 251
    assert(_m_pEditWnd != NULL);
    CSize size = _m_pEditWnd->getMinWndSize();
#line 254
    assert(_m_pHRuler != NULL);
    size.cy += _m_pHRuler->getHeight();
#line 257
    assert(_m_pVRuler != NULL);
    size.cx += _m_pVRuler->getWidth();
    return size;
}

TMapLayerObjectID TMapFrameWnd::getSelectedObjectID() const
{
#line 266
    assert(_m_pEditWnd != NULL);
    return _m_pEditWnd->getSelectedObjectID();
}

void TMapFrameWnd::onMoveMapViewRect(TMapViewingWnd* pWnd, const CPoint& pos)
{
#line 273
    assert(_m_pController != NULL);
    _m_pController->onMoveMapViewRect(this, pos);
}

void TMapFrameWnd::onSizeMapViewRect(TMapViewingWnd* pWnd, const CSize& size)
{
#line 280
    assert(_m_pController != NULL);
    _m_pController->onSizeMapViewRect(this, size);
}

void TMapFrameWnd::onEditCursorTilePosChanged(TMapEditingWnd* pWnd, const CPoint& pos)
{
#line 287
    assert(_m_pController != NULL);
    _m_pController->onEditCursorTilePosChanged(this, pos);
#line 290
    assert(_m_pHRuler != NULL);
    _m_pHRuler->setHighlight(pos.x);
#line 293
    assert(_m_pVRuler != NULL);
    _m_pVRuler->setHighlight(pos.y);
}

void TMapFrameWnd::onEditObjectSelected(TMapEditingWnd* pWnd, unsigned int objID)
{
#line 300
    assert(_m_pController != NULL);
    _m_pController->onEditObjectSelected(this, objID);
}

void TMapFrameWnd::onEditEditObject(TMapEditingWnd* pWnd, unsigned int objID)
{
#line 307
    assert(_m_pController != NULL);
    _m_pController->onEditEditObject(this, objID);
}

void TMapFrameWnd::onEditDeleteObject(TMapEditingWnd* pWnd, unsigned int objID)
{
#line 314
    assert(_m_pController != NULL);
    _m_pController->onEditDeleteObject(this, objID);
}

void TMapFrameWnd::onEditGrabObject(TMapEditingWnd* pWnd, unsigned int objID)
{
#line 321
    assert(_m_pController != NULL);
    _m_pController->onEditGrabObject(this, objID);
}

void TMapFrameWnd::onEditCopyObject(TMapEditingWnd* pWnd, unsigned int objID)
{
#line 328
    assert(_m_pController != NULL);
    _m_pController->onEditCopyObject(this, objID);
}

void TMapFrameWnd::onEditPlaceObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj, unsigned int x, unsigned int y)
{
#line 335
    assert(_m_pController != NULL);
    _m_pController->onEditPlaceObject(this, obj, x, y);
}

void TMapFrameWnd::onEditDiscardObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj)
{
#line 342
    assert(_m_pController != NULL);
    _m_pController->onEditDiscardObject(this, obj);
}

void TMapFrameWnd::onEditPasteObject(TMapEditingWnd* pWnd, const TGameObject& obj, const CRect& rect)
{
#line 349
    assert(_m_pController != NULL);
    _m_pController->onEditPasteObject(this, obj, rect);
}

void TMapFrameWnd::onEditBrushBeginDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
#line 356
    assert(_m_pController != NULL);
    _m_pController->onEditBrushBeginDrag(this, rect);
}

void TMapFrameWnd::onEditBrushEndDrag(TMapEditingWnd* pWnd)
{
#line 363
    assert(_m_pController != NULL);
    _m_pController->onEditBrushEndDrag(this);
}

void TMapFrameWnd::onEditBrushDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
#line 370
    assert(_m_pController != NULL);
    _m_pController->onEditBrushDrag(this, rect);
}

void TMapFrameWnd::onEditFillRectAnchor(TMapEditingWnd* pWnd, const CRect& rect)
{
#line 377
    assert(_m_pController != NULL);
    _m_pController->onEditFillRectAnchor(this, rect);
}

void TMapFrameWnd::onEditFillRectDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
#line 384
    assert(_m_pController != NULL);
    _m_pController->onEditFillRectDrag(this, rect);
}

void TMapFrameWnd::onEditFillRectEndDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
#line 391
    assert(_m_pController != NULL);
    _m_pController->onEditFillRectEndDrag(this, rect);
}

void TMapFrameWnd::_deleteAll()
{
    delete _m_pVRuler;
    delete _m_pHRuler;
    delete _m_pEditWnd;
}

void TMapFrameWnd::OnSize(unsigned int type, int cx, int cy)
{
    if (_m_pEditWnd) {
#line 429
        assert(_m_pEditWnd != NULL);
        assert(_m_pHRuler != NULL);
        assert(_m_pVRuler != NULL);
        _m_pEditWnd->MoveWindow(_m_pVRuler->getWidth(), _m_pHRuler->getHeight(),
                                cx - _m_pVRuler->getWidth(), cy - _m_pHRuler->getHeight());
        CRect rect;
        _m_pEditWnd->GetClientRect(&rect);
        _m_pEditWnd->ClientToScreen(&rect);
        ScreenToClient(&rect);
        _m_pHRuler->move(CPoint(rect.left, 0), rect.Width());
        _m_pVRuler->move(CPoint(0, rect.top), rect.Height());
    }
}
