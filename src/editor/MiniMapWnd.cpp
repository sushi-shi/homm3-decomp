// MiniMapWnd.cpp - the mini map (h3maped 0x487715..0x488543; Loki h3maped
// object 56). Every cell takes the colour of its terrain, or the mini map
// colour of the topmost impassable object there (owned objects win), and
// the obstacle area is tinted over it; the view rectangle is a dotted XOR
// frame. Dragging inside the window moves the view through the controller.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Clamp.h"
#include "editor/Colors.h"
#include "editor/DCAttributeSelector.h"
#include "editor/GameMap.h"
#include "editor/GDIObjectSelector.h"
#include "editor/GUIGameObject.h"
#include "editor/MemoryDC.h"
#include "editor/MiniMapWnd.h"
#include "editor/GameMapMask.h"
#include "editor/T16bppPalette.h"

DATA(0x005a1ed0) const CSize TMiniMapWnd::s_kClientSize(144, 144);

VA(0x004878ee, 0x12e)
TMiniMapWnd::TMiniMapWnd(CWnd* pParent, TMapViewingWnd::TController* pController, const TGameMap* pMap,
                         const TGameMapMask* pObstacleMask, bool bSecondLayer)
    : _m_pController(pController),
      _m_pMap(pMap),
      _m_pObstacleMask(pObstacleMask),
      _m_bSecondLayer(bSecondLayer),
      _m_viewPos(0, 0),
      _m_viewSize(0, 0),
      _m_bDragging(false)
{
    DATA_COMPGEN_GUARD(0x005a1edc, miniMapWndClassNameGuard, className)
    VA_COMPGEN(0x00487a1c, 0xa, STATIC_DTOR, className)
    static CString className;
    if (className.IsEmpty())
        className = AfxRegisterWndClass(0, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1));
    int cxEdge = ::GetSystemMetrics(SM_CXEDGE);
    int cyEdge = ::GetSystemMetrics(SM_CYEDGE);
    if (!CreateEx(WS_EX_CLIENTEDGE, className, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                  CRect(0, 0, s_kClientSize.cx + cxEdge * 2, s_kClientSize.cy + cyEdge * 2), pParent, 0))
        throw TRuntimeError();
}

VA_COMPGEN(0x00487a26, 0x1c, SCALAR_DELETING_DTOR, TMiniMapWnd)
VA_COMPGEN(0x00487a42, 0x5, IMPLICIT_DTOR, TMapViewingWnd)
VA_COMPGEN(0x00487a47, 0x1c, SCALAR_DELETING_DTOR, TMapViewingWnd)

VA(0x00487a63, 0x3b)
TMiniMapWnd::~TMiniMapWnd()
{
}

VA(0x00487aae, 0x9)
void TMiniMapWnd::clearMap()
{
    _m_pMap = NULL;
    _m_pObstacleMask = NULL;
}

VA(0x00487ab7, 0x33)
void TMiniMapWnd::setMapLayer(const TGameMap* pMap, const TGameMapMask* pObstacleMask, bool bSecondLayer)
{
    _m_pMap = pMap;
    _m_pObstacleMask = pObstacleMask;
    _m_bSecondLayer = bSecondLayer;
    _m_viewPos = CPoint(0, 0);
    _m_viewSize = CSize(0, 0);
    Invalidate(FALSE);
}

VA(0x00487aea, 0x198)
void TMiniMapWnd::moveViewRect(const CPoint& pos)
{
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    CRect oldRect(_m_viewPos, _m_viewSize);
    CRect newRect(pos, _m_viewSize);
    _m_viewPos = pos;
    CRect dirtyRect;
    if (dirtyRect.IntersectRect(oldRect, newRect)) {
        dirtyRect.UnionRect(oldRect, newRect);
        dirtyRect.left = dirtyRect.left * s_kClientSize.cx / width;
        dirtyRect.top = dirtyRect.top * s_kClientSize.cy / height;
        dirtyRect.right = dirtyRect.right * s_kClientSize.cx / width;
        dirtyRect.bottom = dirtyRect.bottom * s_kClientSize.cy / height;
        InvalidateRect(dirtyRect, FALSE);
    } else {
        dirtyRect.left = oldRect.left * s_kClientSize.cx / width;
        dirtyRect.top = oldRect.top * s_kClientSize.cy / height;
        dirtyRect.right = oldRect.right * s_kClientSize.cx / width;
        dirtyRect.bottom = oldRect.bottom * s_kClientSize.cy / height;
        InvalidateRect(dirtyRect, FALSE);
        dirtyRect.left = newRect.left * s_kClientSize.cx / width;
        dirtyRect.top = newRect.top * s_kClientSize.cy / height;
        dirtyRect.right = newRect.right * s_kClientSize.cx / width;
        dirtyRect.bottom = newRect.bottom * s_kClientSize.cy / height;
        InvalidateRect(dirtyRect, FALSE);
    }
    UpdateWindow();
}

VA(0x00487c82, 0xdb)
void TMiniMapWnd::sizeViewRect(const CSize& size)
{
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    CRect oldRect(_m_viewPos, _m_viewSize);
    CRect newRect(_m_viewPos, size);
    _m_viewSize = size;
    CRect dirtyRect;
    dirtyRect.UnionRect(oldRect, newRect);
    dirtyRect.left = dirtyRect.left * s_kClientSize.cx / width;
    dirtyRect.top = dirtyRect.top * s_kClientSize.cy / height;
    dirtyRect.right = dirtyRect.right * s_kClientSize.cx / width;
    dirtyRect.bottom = dirtyRect.bottom * s_kClientSize.cy / height;
    InvalidateRect(dirtyRect, FALSE);
    UpdateWindow();
}

VA(0x00487d5d, 0x86)
void TMiniMapWnd::update(const CRect& rect)
{
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    CRect dirtyRect(rect.left * s_kClientSize.cx / width, rect.top * s_kClientSize.cy / height,
                    rect.right * s_kClientSize.cx / width, rect.bottom * s_kClientSize.cy / height);
    InvalidateRect(dirtyRect, FALSE);
    UpdateWindow();
}

VA(0x00487de3, 0xc8)
void TMiniMapWnd::_processDrag(const CPoint& point)
{
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    CPoint pos(point.x * int(width) / s_kClientSize.cx, point.y * int(height) / s_kClientSize.cy);
    pos.x -= _m_viewSize.cx / 2;
    pos.y -= _m_viewSize.cy / 2;
    pos.x = clamp<int>(0, pos.x, width - _m_viewSize.cx);
    pos.y = clamp<int>(0, pos.y, height - _m_viewSize.cy);
    if (pos != _m_viewPos)
        _m_pController->onMoveMapViewRect(this, pos);
}

VA(0x00487eab, 0x6)
BEGIN_MESSAGE_MAP(TMiniMapWnd, TMapViewingWnd)
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SIZE()
    ON_WM_CAPTURECHANGED()
END_MESSAGE_MAP()

// The mini map keeps its size whatever its parent asks.
VA(0x00487eb1, 0x10)
void TMiniMapWnd::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    lpwndpos->flags |= SWP_NOSIZE;
    CWnd::OnWindowPosChanging(lpwndpos);
}

VA(0x00487ec1, 0x533)
void TMiniMapWnd::OnPaint()
{
    CPaintDC dc(this);
    if (_m_pMap == NULL)
        return;
    TMemoryDC memDC(&dc);
    TGDIObjectSelector<CBitmap> bitmapSelector(&memDC, &_m_backBuffer);
    CRect rect;
    dc.GetClipBox(&rect);
    memDC.SelectClipRgn(NULL);
    memDC.IntersectClipRect(&rect);
    CBrush brush(::GetSysColor(COLOR_WINDOW));
    TGDIObjectSelector<CBrush> brushSelector(&memDC, &brush);
    CPen pen(PS_DOT, 1, RGB(255, 255, 255));
    TGDIObjectSelector<CPen> penSelector(&memDC, &pen);
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    CRect tileRect(rect.left * width / s_kClientSize.cx, rect.top * height / s_kClientSize.cy,
                   (rect.right * width + s_kClientSize.cx - 1) / s_kClientSize.cx,
                   (rect.bottom * height + s_kClientSize.cy - 1) / s_kClientSize.cy);
    const TGameMap::TLayer& layer = _m_pMap->getLayer(_m_bSecondLayer);
    CPoint tile;
    for (tile.y = tileRect.top; tile.y < tileRect.bottom; tile.y++) {
        for (tile.x = tileRect.left; tile.x < tileRect.right; tile.x++) {
            CRect cellRect(tile.x * s_kClientSize.cx / width, tile.y * s_kClientSize.cy / height,
                           (tile.x + 1) * s_kClientSize.cx / width, (tile.y + 1) * s_kClientSize.cy / height);
            TTerrainType terrainType = layer.getCell(tile.x, tile.y).getTerrainType();
            TColor color = akTerrainColors[terrainType].m_color;
            const TGUIGameObject* pTopObj = NULL;
            bool bTopOwnable = false;
            unsigned int numObjects = layer.getNumObjectIDsAtCell(tile.x, tile.y);
            for (unsigned int i = 0; i < numObjects; i++) {
                TMapLayerObjectID objID = layer.getObjectIDAtCell(tile.x, tile.y, i);
                TTilePoint objLoc = layer.getObjectLoc(objID);
                const TGameObject& obj = layer.getObject(objID);
                if (!obj.getBCellPassable(objLoc.x() - tile.x, objLoc.y() - tile.y)) {
                    const TGUIGameObject* pThisObj = dynamic_cast<const TGUIGameObject*>(&obj);
                    bool bOwnable = pThisObj->isOwnable();
                    if (!bTopOwnable || bOwnable) {
                        pTopObj = pThisObj;
                        bTopOwnable = bOwnable;
                    }
                }
            }
            if (pTopObj != NULL)
                color = pTopObj->miniMapColor(terrainType);
            _m_backBuffer.fillRect(cellRect.left, cellRect.top, cellRect.Width(), cellRect.Height(), color);
            if (_m_pObstacleMask->hasTiles(_m_bSecondLayer)) {
                int state = _m_pObstacleMask->getTileState(tile.x, tile.y, _m_bSecondLayer);
                if (state == 2) {
                    TRGB obstacleColor = {0, 0, 0xff};
                    _m_backBuffer.blendRect(cellRect.left, cellRect.top, cellRect.Width(), cellRect.Height(),
                                            T16bppPalette::rgbToEntry(obstacleColor));
                } else if (state == 1) {
                    TRGB obstacleColor = {0, 0, 0xff};
                    _m_backBuffer.tintRect(cellRect.left, cellRect.top, cellRect.Width(), cellRect.Height(),
                                           T16bppPalette::rgbToEntry(obstacleColor));
                }
            }
        }
    }
    CRect viewRect(_m_viewPos.x * s_kClientSize.cx / width, _m_viewPos.y * s_kClientSize.cy / height,
                   (_m_viewPos.x + _m_viewSize.cx) * s_kClientSize.cx / width - 1,
                   (_m_viewPos.y + _m_viewSize.cy) * s_kClientSize.cy / height - 1);
    POINT points[5] = {
        { viewRect.left, viewRect.top },
        { viewRect.right, viewRect.top },
        { viewRect.right, viewRect.bottom },
        { viewRect.left, viewRect.bottom },
        { viewRect.left, viewRect.top },
    };
    {
        TDCAttributeSelector<int, &CDC::SetBkMode> bkModeSelector(&memDC, TRANSPARENT);
        int oldROP2 = memDC.SetROP2(R2_XORPEN);
        memDC.Polyline(points, 5);
        memDC.SetROP2(oldROP2);
    }
    dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &memDC, rect.left, rect.top, SRCCOPY);
}

VA(0x004883f4, 0x2d)
void TMiniMapWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    SetCapture();
    _m_bDragging = true;
    _processDrag(point);
    CWnd::OnLButtonDown(nFlags, point);
}

VA(0x00488421, 0x1a)
void TMiniMapWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (_m_bDragging)
        ReleaseCapture();
    CWnd::OnLButtonUp(nFlags, point);
}

VA(0x0048843b, 0x1e)
void TMiniMapWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    if (_m_bDragging)
        _processDrag(point);
    CWnd::OnMouseMove(nFlags, point);
}

VA(0x00488459, 0x21)
void TMiniMapWnd::OnSize(UINT nType, int cx, int cy)
{
    if (cx != 0 && cy != 0)
        _m_backBuffer.create(cx, cy);
}

VA(0x0048847a, 0xc)
void TMiniMapWnd::OnCaptureChanged(CWnd* pWnd)
{
    _m_bDragging = false;
    CWnd::OnCaptureChanged(pWnd);
}
