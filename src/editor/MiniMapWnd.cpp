// MiniMapWnd.cpp - Loki h3maped object 56: the mini map, the whole map
// layer scaled to a fixed client size with the main view's rectangle drawn
// over it. Dragging inside it moves that rectangle through the controller.
// Every cell takes the colour of its terrain, or the mini map colour of the
// topmost impassable object there (owned objects win). Assert lines come
// from the retail immediates.
#include "editor/stdafx.h"

#include <string>

#include "editor/MiniMapWnd.h"
#include "editor/Clamp.h"
#include "editor/Colors.h"
#include "editor/GameMap.h"
#include "editor/GUIGameObject.h"

const CSize TMiniMapWnd::s_kClientSize(144, 144);

TMiniMapWnd::TMiniMapWnd(GtkWidget* thisWidget, TMapViewingWnd::TController* pController,
                         const TGameMap* pMap, bool bSecondLayer)
    : _m_pController(pController),
      _m_pMap(pMap),
      _m_bSecondLayer(bSecondLayer),
      _m_viewPos(0, 0),
      _m_viewSize(0, 0),
      _m_bDragging(false),
      _m_pBackBuffer(NULL)
{
#line 39
    assert(thisWidget != NULL);
    assert(_m_pController != NULL);
    assert(_m_pMap != NULL);
    assert(!_m_bSecondLayer || _m_pMap->isTwoLayer());
    _m_hWnd = thisWidget;
    gtk_widget_add_events(_m_hWnd, GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK
                                   | GDK_BUTTON_MOTION_MASK
                                   | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON2_MOTION_MASK
                                   | GDK_BUTTON3_MOTION_MASK | GDK_BUTTON_PRESS_MASK
                                   | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK
                                   | GDK_KEY_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK
                                   | GDK_LEAVE_NOTIFY_MASK);
}

TMiniMapWnd::~TMiniMapWnd()
{
}

void TMiniMapWnd::clearMap()
{
    _m_pMap = NULL;
}

void TMiniMapWnd::setMapLayer(const TGameMap* pMap, bool bSecondLayer)
{
#line 88
    assert(pMap != NULL);
    assert(!bSecondLayer || pMap->isTwoLayer());
    _m_pMap = pMap;
    _m_bSecondLayer = bSecondLayer;
    if (_m_bDragging)
        _m_bDragging = false;
    _m_viewPos = CPoint(0, 0);
    _m_viewSize = CSize(0, 0);
    Invalidate(false);
}

void TMiniMapWnd::moveViewRect(const CPoint& pos)
{
#line 105
    assert(_m_pMap != NULL);
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    CRect oldRect(_m_viewPos, _m_viewSize);
    CRect newRect(pos, _m_viewSize);
    _m_viewPos = pos;
    CRect dirtyRect;
    if (dirtyRect.IntersectRect(oldRect, newRect)) {
#line 120
        assert(dirtyRect.UnionRect( oldRect, newRect ) != false);
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
    OnPaint();
}

void TMiniMapWnd::sizeViewRect(const CSize& size)
{
#line 149
    assert(_m_pMap != NULL);
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    CRect oldRect(_m_viewPos, _m_viewSize);
    CRect newRect(_m_viewPos, size);
    _m_viewSize = size;
    CRect dirtyRect;
#line 161
    assert(dirtyRect.UnionRect( oldRect, newRect ) != false);
    dirtyRect.left = dirtyRect.left * s_kClientSize.cx / width;
    dirtyRect.top = dirtyRect.top * s_kClientSize.cy / height;
    dirtyRect.right = dirtyRect.right * s_kClientSize.cx / width;
    dirtyRect.bottom = dirtyRect.bottom * s_kClientSize.cy / height;
    InvalidateRect(dirtyRect, FALSE);
}

void TMiniMapWnd::update(const CRect& rect)
{
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    CRect dirtyRect(rect.left * s_kClientSize.cx / width, rect.top * s_kClientSize.cy / height,
                    rect.right * s_kClientSize.cx / width, rect.bottom * s_kClientSize.cy / height);
    InvalidateRect(dirtyRect, FALSE);
}

// Not exact: retail passes &pos.y to the second clamp as -0xc(%ebp)
// directly, where this spelling forms it as &pos + 4 (build_component_addr);
// only an indirection such as *&pos.y reproduces it, which is not source.
void TMiniMapWnd::_processDrag(const CPoint& point)
{
#line 189
    assert(_m_pMap != NULL);
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    CPoint pos(point.x * int(width) / s_kClientSize.cx, point.y * int(height) / s_kClientSize.cy);
    pos.x -= _m_viewSize.cx / 2;
    pos.y -= _m_viewSize.cy / 2;
    pos.x = clamp<int>(0, pos.x, width - _m_viewSize.cx);
    pos.y = clamp<int>(0, pos.y, height - _m_viewSize.cy);
    if (pos != _m_viewPos) {
#line 207
        assert(_m_pController != NULL);
        _m_pController->onMoveMapViewRect(this, pos);
    }
}

void TMiniMapWnd::OnPaint()
{
    if (_m_hWnd) {
        CRect rect(0, 0, _m_hWnd->allocation.width, _m_hWnd->allocation.height);
        OnPaint(rect);
    }
}

void TMiniMapWnd::paintTiles(const CRect& rect)
{
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    CRect dirtyRect(rect.left * s_kClientSize.cx / width, rect.top * s_kClientSize.cx / height,
                    rect.right * s_kClientSize.cx / height, rect.bottom * s_kClientSize.cx / height);
    OnPaint(dirtyRect);
}

void TMiniMapWnd::OnPaint(const CRect& rect)
{
    if (!_m_hWnd->window || !_m_pBackBuffer)
        return;

    if (_m_pMap == NULL) {
        GdkGC* gc = gdk_gc_new(_m_hWnd->window);
        gdk_gc_set_foreground(gc, &_m_white);
        gdk_draw_rectangle(_m_hWnd->window, gc, TRUE, 0, 0,
                           _m_hWnd->allocation.width, _m_hWnd->allocation.height);
        gdk_gc_set_foreground(gc, &_m_red);
        gdk_draw_line(_m_hWnd->window, gc, 0, 0,
                      _m_hWnd->allocation.width, _m_hWnd->allocation.height);
        gdk_draw_line(_m_hWnd->window, gc, 0, _m_hWnd->allocation.height,
                      _m_hWnd->allocation.width, 0);
        gdk_gc_unref(gc);
        return;
    }

    if (!_m_pBackBuffer)
        return;
    GdkGC* gc = gdk_gc_new(_m_pBackBuffer);
    if (!gc)
        return;

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
                           (tile.x + 1) * s_kClientSize.cx / width,
                           (tile.y + 1) * s_kClientSize.cy / height);
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
#line 364
                    assert(pThisObj != NULL);
                    bool bOwnable = pThisObj->isOwnable();
                    if (!bTopOwnable || bOwnable) {
                        pTopObj = pThisObj;
                        bTopOwnable = bOwnable;
                    }
                }
            }
            if (pTopObj)
                color = pTopObj->miniMapColor(terrainType);
            GdkColor gdkColor;
            TColorToGdkColor(color, &gdkColor);
            gdk_colormap_alloc_color(gdk_colormap_get_system(), &gdkColor, FALSE, TRUE);
            gdk_gc_set_foreground(gc, &gdkColor);
            gdk_draw_rectangle(_m_pBackBuffer, gc, TRUE, cellRect.left, cellRect.top,
                               cellRect.Width(), cellRect.Height());
        }
    }

    CRect viewRect(_m_viewPos.x * s_kClientSize.cx / width,
                   _m_viewPos.y * s_kClientSize.cx / height,
                   (_m_viewPos.x + _m_viewSize.cx) * s_kClientSize.cx / width - 1,
                   (_m_viewPos.y + _m_viewSize.cy) * s_kClientSize.cy / height - 1);
    GdkPoint points[5] = {
        { viewRect.left, viewRect.top },
        { viewRect.right, viewRect.top },
        { viewRect.right, viewRect.bottom },
        { viewRect.left, viewRect.bottom },
        { viewRect.left, viewRect.top },
    };
    GdkGCValues values;
    gdk_gc_get_values(gc, &values);
    gdk_gc_set_function(gc, GDK_XOR);
    gdk_gc_set_line_attributes(gc, values.line_width, GDK_LINE_ON_OFF_DASH, values.cap_style,
                               values.join_style);
    gdk_gc_set_foreground(gc, &_m_white);
    gdk_draw_lines(_m_pBackBuffer, gc, points, 5);
    gdk_gc_set_function(gc, values.function);
    gdk_draw_pixmap(_m_hWnd->window, gc, _m_pBackBuffer, rect.left, rect.top, rect.left, rect.top,
                    rect.Width(), rect.Height());
    gdk_gc_unref(gc);
}

void TMiniMapWnd::OnLButtonDown(unsigned int flags, CPoint point)
{
    _m_bDragging = true;
    _processDrag(point);
}

void TMiniMapWnd::OnLButtonUp(unsigned int flags, CPoint point)
{
    _m_bDragging = false;
    ReleaseCapture();
}

void TMiniMapWnd::OnMouseEnter()
{
    if (!button1Down())
        _m_bDragging = false;
}

void TMiniMapWnd::OnMouseMove(unsigned int flags, CPoint point)
{
    if (_m_bDragging) {
        gint x;
        gint y;
        gdk_window_get_pointer(_m_hWnd->window, &x, &y, NULL);
        if (x == point.x && y == point.y)
            _processDrag(point);
    }
}

void TMiniMapWnd::OnSize(unsigned int type, int cx, int cy)
{
    if (cx == 0 || cy == 0 || _m_hWnd == NULL || _m_hWnd->window == NULL)
        return;
    if (_m_pBackBuffer)
        gdk_pixmap_unref(_m_pBackBuffer);
    _m_pBackBuffer = gdk_pixmap_new(_m_hWnd->window, cx, cy, -1);
}

void TMiniMapWnd::OnCaptureChanged(CWnd* pWnd)
{
    _m_bDragging = false;
}
