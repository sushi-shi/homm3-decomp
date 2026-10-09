// MapFrameWnd.cpp - the map frame (h3maped 0x46f4ce..0x46fd3c; Loki h3maped
// object 56): the map edit window with the horizontal ruler above it and
// the vertical ruler at its left. The frame forwards the map view's calls
// to the edit window and the edit window's events to its own controller,
// with itself as the window. The view-rectangle move forwarder is ICF's
// twin of another window's (0x4c164c) and so carries no claim.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/MapEditWnd.h"
#include "editor/MapFrameWnd.h"
#include "editor/TileHRuler.h"
#include "editor/TileVRuler.h"

VA(0x0046f6a2, 0x25e)
TMapFrameWnd::TMapFrameWnd(CWnd* pParent, TMapEditingWnd::TController* pController, int id, const TGameMap* pMap,
                           const TGameMapMask* pObstacleMask, bool bSecondLayer, TZoom zoom, bool bShowGrid,
                           bool bShowPassability)
    : _m_pController(pController),
      _m_pEditWnd(NULL),
      _m_pHRuler(NULL),
      _m_pVRuler(NULL)
{
    DATA_COMPGEN_GUARD(0x005a1c30, mapFrameWndClassNameGuard, className)
    VA_COMPGEN(0x0046f900, 0xa, STATIC_DTOR, className)
    static CString className;
    if (className.IsEmpty())
        className = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_BTNFACE + 1));
    if (!Create(className, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, CRect(0, 0, 0, 0), pParent, 0))
        throw TRuntimeError();
    _m_pEditWnd = new TMapEditWnd(this, this, id, pMap, pObstacleMask, bSecondLayer, zoom, bShowGrid,
                                  bShowPassability);
    if (_m_pEditWnd == NULL)
        throw TAllocationFailure();
    try {
        if (GetFocus() == this)
            _m_pEditWnd->SetFocus();
        _m_pHRuler = new TTileHRuler(this, pMap->getWidth(), zoom);
        if (_m_pHRuler == NULL)
            throw TAllocationFailure();
        _m_pVRuler = new TTileVRuler(this, pMap->getHeight(), zoom);
        if (_m_pVRuler == NULL)
            throw TAllocationFailure();
    } catch (...) {
        _deleteAll();
        throw;
    }
    _m_pHRuler->setStartTile(0);
    _m_pVRuler->setStartTile(0);
}

VA_COMPGEN(0x0046f90a, 0x1c, SCALAR_DELETING_DTOR, TMapFrameWnd)

VA(0x0046f926, 0x3f)
TMapFrameWnd::~TMapFrameWnd()
{
    _deleteAll();
}

VA(0x0046f965, 0x8)
void TMapFrameWnd::clearMap()
{
    _m_pEditWnd->clearMap();
}

VA(0x0046f96d, 0x3e)
void TMapFrameWnd::setMapLayer(const TGameMap* pMap, const TGameMapMask* pObstacleMask, bool bSecondLayer)
{
    _m_pHRuler->setRange(pMap->getWidth());
    _m_pVRuler->setRange(pMap->getHeight());
    _m_pEditWnd->setMapLayer(pMap, pObstacleMask, bSecondLayer);
}

VA(0x0046f9ab, 0x2b)
void TMapFrameWnd::moveViewRect(const CPoint& pos)
{
    _m_pEditWnd->moveViewRect(pos);
    _m_pHRuler->setStartTile(pos.x);
    _m_pVRuler->setStartTile(pos.y);
}

VA(0x0046f9d6, 0xf)
void TMapFrameWnd::update(const CRect& rect)
{
    _m_pEditWnd->update(rect);
}

VA(0x0046f9e5, 0x28)
void TMapFrameWnd::setZoom(TZoom zoom)
{
    _m_pEditWnd->setZoom(zoom);
    _m_pHRuler->setZoom(zoom);
    _m_pVRuler->setZoom(zoom);
}

VA(0x0046fa0d, 0xf)
void TMapFrameWnd::showGrid(bool bShow)
{
    _m_pEditWnd->showGrid(bShow);
}

VA(0x0046fa1c, 0xf)
void TMapFrameWnd::showPassability(bool bShow)
{
    _m_pEditWnd->showPassability(bShow);
}

VA(0x0046fa2b, 0xf)
void TMapFrameWnd::selectObject(unsigned int objID)
{
    _m_pEditWnd->selectObject(objID);
}

VA(0x0046fa3a, 0xf)
void TMapFrameWnd::grabObject(const TGUIGameObject* pObj)
{
    _m_pEditWnd->grabObject(pObj);
}

VA(0x0046fa49, 0x8)
void TMapFrameWnd::onUndo()
{
    _m_pEditWnd->onUndo();
}

VA(0x0046fa51, 0xf)
void TMapFrameWnd::onObjectRemoved(unsigned int objID)
{
    _m_pEditWnd->onObjectRemoved(objID);
}

VA(0x0046fa60, 0x13)
void TMapFrameWnd::animate(unsigned int frameNum, bool bForce)
{
    _m_pEditWnd->animate(frameNum, bForce);
}

VA(0x0046fa73, 0x8)
void TMapFrameWnd::resetAnimation()
{
    _m_pEditWnd->resetAnimation();
}

VA(0x0046fa7b, 0xf)
void TMapFrameWnd::makeVisible(unsigned int objID)
{
    _m_pEditWnd->makeVisible(objID);
}

VA(0x0046fa8a, 0x8)
void TMapFrameWnd::selectionMode()
{
    _m_pEditWnd->selectionMode();
}

VA(0x0046fa92, 0xf)
void TMapFrameWnd::brushMode(const CSize& size)
{
    _m_pEditWnd->brushMode(size);
}

VA(0x0046faa1, 0x8)
void TMapFrameWnd::fillMode()
{
    _m_pEditWnd->fillMode();
}

VA(0x0046faa9, 0xa)
TMapLayerObjectID TMapFrameWnd::getSelectedObjectID() const
{
    return _m_pEditWnd->getSelectedObjectID();
}

void TMapFrameWnd::onMoveMapViewRect(TMapViewingWnd* pWnd, const CPoint& pos)
{
    _m_pController->onMoveMapViewRect(this, pos);
}

VA(0x0046fab3, 0x15)
void TMapFrameWnd::onSizeMapViewRect(TMapViewingWnd* pWnd, const CSize& size)
{
    _m_pController->onSizeMapViewRect(this, size);
}

VA(0x0046fac8, 0x2f)
void TMapFrameWnd::onEditCursorTilePosChanged(TMapEditingWnd* pWnd, const CPoint& pos)
{
    _m_pController->onEditCursorTilePosChanged(this, pos);
    _m_pHRuler->setHighlight(pos.x);
    _m_pVRuler->setHighlight(pos.y);
}

VA(0x0046faf7, 0x15)
void TMapFrameWnd::onEditObjectSelected(TMapEditingWnd* pWnd, unsigned int objID)
{
    _m_pController->onEditObjectSelected(this, objID);
}

VA(0x0046fb0c, 0x15)
void TMapFrameWnd::onEditEditObject(TMapEditingWnd* pWnd, unsigned int objID)
{
    _m_pController->onEditEditObject(this, objID);
}

VA(0x0046fb21, 0x15)
void TMapFrameWnd::onEditDeleteObject(TMapEditingWnd* pWnd, unsigned int objID)
{
    _m_pController->onEditDeleteObject(this, objID);
}

VA(0x0046fb36, 0x15)
void TMapFrameWnd::onEditGrabObject(TMapEditingWnd* pWnd, unsigned int objID)
{
    _m_pController->onEditGrabObject(this, objID);
}

VA(0x0046fb4b, 0x15)
void TMapFrameWnd::onEditCopyObject(TMapEditingWnd* pWnd, unsigned int objID)
{
    _m_pController->onEditCopyObject(this, objID);
}

VA(0x0046fb60, 0x1d)
void TMapFrameWnd::onEditPlaceObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj, unsigned int x,
                                     unsigned int y)
{
    _m_pController->onEditPlaceObject(this, obj, x, y);
}

VA(0x0046fb7d, 0x15)
void TMapFrameWnd::onEditDiscardObject(TMapEditingWnd* pWnd, const TGUIGameObject& obj)
{
    _m_pController->onEditDiscardObject(this, obj);
}

VA(0x0046fb92, 0x19)
void TMapFrameWnd::onEditPasteObject(TMapEditingWnd* pWnd, const TGameObject& obj, const CRect& rect)
{
    _m_pController->onEditPasteObject(this, obj, rect);
}

VA(0x0046fbab, 0x15)
void TMapFrameWnd::onEditBrushBeginDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
    _m_pController->onEditBrushBeginDrag(this, rect);
}

VA(0x0046fbc0, 0x11)
void TMapFrameWnd::onEditBrushEndDrag(TMapEditingWnd* pWnd)
{
    _m_pController->onEditBrushEndDrag(this);
}

VA(0x0046fbd1, 0x15)
void TMapFrameWnd::onEditBrushDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
    _m_pController->onEditBrushDrag(this, rect);
}

VA(0x0046fbe6, 0x15)
void TMapFrameWnd::onEditFillRectAnchor(TMapEditingWnd* pWnd, const CRect& rect)
{
    _m_pController->onEditFillRectAnchor(this, rect);
}

VA(0x0046fbfb, 0x15)
void TMapFrameWnd::onEditFillRectDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
    _m_pController->onEditFillRectDrag(this, rect);
}

VA(0x0046fc10, 0x15)
void TMapFrameWnd::onEditFillRectEndDrag(TMapEditingWnd* pWnd, const CRect& rect)
{
    _m_pController->onEditFillRectEndDrag(this, rect);
}

VA(0x0046fc25, 0x2f)
void TMapFrameWnd::_deleteAll()
{
    delete _m_pVRuler;
    delete _m_pHRuler;
    delete _m_pEditWnd;
}

VA(0x0046fc54, 0x6)
BEGIN_MESSAGE_MAP(TMapFrameWnd, TMapEditingWnd)
    ON_WM_SIZE()
    ON_WM_SETFOCUS()
END_MESSAGE_MAP()

// Commands go to the edit window first; the frame handles none itself.
VA(0x0046fc5a, 0x2a)
BOOL TMapFrameWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
    if (_m_pEditWnd != NULL && _m_pEditWnd->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;
    return FALSE;
}

VA(0x0046fc84, 0xa0)
void TMapFrameWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    if (_m_pEditWnd != NULL) {
        _m_pEditWnd->MoveWindow(_m_pVRuler->getWidth(), _m_pHRuler->getHeight(), cx - _m_pVRuler->getWidth(),
                                cy - _m_pHRuler->getHeight());
        CRect rect;
        _m_pEditWnd->GetClientRect(&rect);
        _m_pEditWnd->ClientToScreen(&rect);
        ScreenToClient(&rect);
        _m_pHRuler->move(CPoint(rect.left, 0), rect.Width());
        _m_pVRuler->move(CPoint(0, rect.top), rect.Height());
    }
}

VA(0x0046fd24, 0x18)
void TMapFrameWnd::OnSetFocus(CWnd* pOldWnd)
{
    if (_m_pEditWnd != NULL)
        _m_pEditWnd->SetFocus();
    else
        CWnd::OnSetFocus(pOldWnd);
}
