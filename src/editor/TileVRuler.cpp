// TileVRuler.cpp - Loki h3maped object 59: the vertical tile ruler, the
// numbered tile rows beside the map (each number drawn digit above digit)
// with the highlighted cursor row, drawn through a back-buffer pixmap.
// Assert lines come from the retail immediates.
#include "editor/stdafx.h"

#include <stdio.h>
#include <string>

#include "editor/TileVRuler.h"

TTileVRuler::TTileVRuler(GtkWidget* thisWin, unsigned int range, TZoom zoom, GdkFont* font)
    : _m_width(0),
      _m_startTileY(0),
      _m_range(range),
      _m_highlight(range),
      _m_zoom(zoom),
      _m_pBackBuffer(NULL),
      _m_pFont(font)
{
#line 45
    assert(thisWin != NULL);
    assert(font != NULL);
    assert(_m_range > 0);
    assert(zoom >= 0 && zoom < kNumZooms);
    _m_hWnd = thisWin;

    gint ascent;
    gint descent;
    int textWidth;
    int margin = 3;
    gdk_string_extents(_m_pFont, "0123456789", NULL, NULL, NULL, &ascent, &descent);
    textWidth = descent + ascent;
    _m_width = textWidth + margin;
    gtk_widget_set_usize(_m_hWnd, _m_width, -2);
}

TTileVRuler::~TTileVRuler()
{
    if (_m_pBackBuffer)
        gdk_pixmap_unref(_m_pBackBuffer);
}

void TTileVRuler::move(const CPoint& pos, unsigned int height)
{
    MoveWindow(pos.x, pos.y, _m_width, height);
}

void TTileVRuler::setHeight(unsigned int newHeight)
{
    CRect wndRect;
    GetWindowRect(wndRect);
#line 107
    assert(wndRect.Width() == _m_width);
    gint x;
    gint y;
    gdk_window_get_deskrelative_origin(gdk_window_get_parent(_m_hWnd->window), &x, &y);
    wndRect.top -= y;
    wndRect.bottom -= y;
    wndRect.left -= x;
    wndRect.right -= x;
    move(wndRect.TopLeft(), newHeight);
}

void TTileVRuler::setStartTile(int startTileY)
{
#line 129
    assert(startTileY >= 0);
    assert(startTileY < _m_range);
    if (startTileY != _m_startTileY) {
        _m_startTileY = startTileY;
        OnPaint();
    }
}

void TTileVRuler::setZoom(TZoom newZoom)
{
#line 143
    assert(newZoom >= 0 && newZoom < kNumZooms);
    if (newZoom != _m_zoom) {
        _m_zoom = newZoom;
        OnPaint();
    }
}

void TTileVRuler::setRange(unsigned int newRange)
{
#line 158
    assert(newRange > 0);
    _m_range = newRange;
    _m_highlight = newRange;
    setStartTile(0);
}

void TTileVRuler::setHighlight(unsigned int tile)
{
    tile = tile < _m_range ? tile : _m_range;
    if (tile != _m_highlight) {
        if (_m_highlight < _m_range && _m_highlight >= _m_startTileY) {
            unsigned int height = _computeHighlightHeight();
            int pos = _computeHighlightPos(height);
            OnPaint();
        }
        _m_highlight = tile;
        snprintf(_m_highlightText, 20, "%d", _m_highlight);
        if (_m_highlight < _m_range && _m_highlight >= _m_startTileY) {
            unsigned int height = _computeHighlightHeight();
            int pos = _computeHighlightPos(height);
            OnPaint();
        }
    }
}

unsigned int TTileVRuler::_computeHighlightHeight() const
{
#line 205
    assert(_m_highlight < _m_range);
    assert(_m_highlight >= _m_startTileY);
    unsigned int textHeight = gdk_string_width(_m_pFont, _m_highlightText) + 4;
    unsigned int tileHeight = akZoomTraits[_m_zoom].m_tileSize;
    return tileHeight < textHeight ? textHeight : tileHeight;
}

int TTileVRuler::_computeHighlightPos(unsigned int height) const
{
#line 220
    assert(_m_highlight < _m_range);
    assert(_m_highlight >= _m_startTileY);
    unsigned int tileHeight = akZoomTraits[_m_zoom].m_tileSize;
#line 224
    assert(height >= tileHeight);
    int tilePos = (_m_highlight - _m_startTileY) * tileHeight;
    return tilePos - (height - tileHeight) / 2;
}

void TTileVRuler::OnPaint()
{
    if (_m_hWnd) {
        CRect rect(0, 0, _m_hWnd->allocation.width, _m_hWnd->allocation.height);
        OnPaint(rect);
    }
}

void TTileVRuler::OnPaint(const CRect& rect)
{
    if (!_m_pBackBuffer)
        return;
    CRect wndRect(0, 0, _m_hWnd->allocation.width, _m_hWnd->allocation.height);
    int tileHeight = akZoomTraits[_m_zoom].m_tileSize;
    GdkGC* gc = gdk_gc_new(_m_hWnd->window);
    if (!gc)
        return;
    gdk_gc_set_foreground(gc, &_m_white);
    gdk_gc_set_background(gc, &_m_white);
    gdk_draw_rectangle(_m_pBackBuffer, gc, TRUE, wndRect.left, wndRect.top,
                       wndRect.Width(), wndRect.Height());

    int fromY = _m_startTileY + wndRect.top / tileHeight;
#line 298
    assert(fromY >= 0);
    if (fromY < _m_range) {
        int toY = _m_startTileY + (wndRect.bottom + tileHeight - 1) / tileHeight;
        if (toY > _m_range)
            toY = _m_range;
#line 304
        assert(toY >= fromY);
        gdk_gc_set_foreground(gc, &_m_black);
        if (_m_highlight < _m_range && _m_highlight >= _m_startTileY) {
            unsigned int height = _computeHighlightHeight();
            int pos = _computeHighlightPos(height);
            if (wndRect.bottom > pos && pos + height > wndRect.top)
                gdk_draw_rectangle(_m_pBackBuffer, gc, TRUE, 0, pos, _m_width, height);
        }
        int posY = 0;
        for (int y = fromY; y < toY; y++) {
            int lineY = (y - _m_startTileY) * tileHeight + tileHeight - 1;
            gdk_draw_line(_m_pBackBuffer, gc, 0, lineY, _m_width + 1, lineY);

            char tensText[2] = { '\0' };
            char onesText[2] = { '\0' };
            int tensHeight = 0;
            int onesHeight = 0;
            CSize textSize(0, 0);
            tensText[0] = '0' + y / 10;
            if (tensText[0] != '0') {
                textSize.cx += gdk_string_width(_m_pFont, tensText);
                tensHeight = gdk_string_height(_m_pFont, tensText);
                textSize.cy += tensHeight;
            }
            onesText[0] = '0' + y % 10;
            textSize.cx = textSize.cx > gdk_string_width(_m_pFont, onesText)
                ? textSize.cx : gdk_string_width(_m_pFont, onesText);
            onesHeight = gdk_string_height(_m_pFont, onesText);
            textSize.cy += onesHeight;

            if (y == _m_highlight)
                gdk_gc_set_foreground(gc, &_m_white);
            if (tensText[0] != '0')
                gdk_draw_string(_m_pBackBuffer, _m_pFont, gc,
                                (_m_hWnd->allocation.width - textSize.cx) / 2,
                                (tileHeight - textSize.cy) / 2 + posY + tensHeight, tensText);
            gdk_draw_string(_m_pBackBuffer, _m_pFont, gc,
                            (_m_hWnd->allocation.width - textSize.cx) / 2,
                            (tileHeight - textSize.cy) / 2 + (posY + 1) + tensHeight + onesHeight,
                            onesText);
            if (y == _m_highlight)
                gdk_gc_set_foreground(gc, &_m_black);
            posY += tileHeight;
        }
    }

    gdk_draw_pixmap(_m_hWnd->window, gc, _m_pBackBuffer, 0, 0, 0, 0,
                    _m_hWnd->allocation.width, _m_hWnd->allocation.height);
    gdk_gc_unref(gc);
}

void TTileVRuler::OnSize(unsigned int type, int cx, int cy)
{
    if (cx == 0 || cy == 0 || _m_hWnd == NULL || _m_hWnd->window == NULL)
        return;
    if (_m_pBackBuffer)
        gdk_pixmap_unref(_m_pBackBuffer);
    _m_pBackBuffer = gdk_pixmap_new(_m_hWnd->window, cx, cy, -1);
}
