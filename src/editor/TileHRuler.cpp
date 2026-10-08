// TileHRuler.cpp - Loki h3maped object 58: the horizontal tile ruler, the
// numbered tile columns above the map with the highlighted cursor column,
// drawn through a back-buffer pixmap. Assert lines come from the retail
// immediates.
#include "editor/stdafx.h"

#include <algorithm>
#include <stdio.h>
#include <string>

#include "editor/TileHRuler.h"

TTileHRuler::TTileHRuler(GtkWidget* thisWidget, unsigned int range, TZoom zoom, GdkFont* font)
    : _m_height(-1),
      _m_startTileX(0),
      _m_range(range),
      _m_highlight(range),
      _m_zoom(zoom),
      _m_pBackBuffer(NULL),
      _m_pFont(font)
{
#line 41
    assert(thisWidget != NULL);
    assert(font != NULL);
    assert(_m_range > 0);
    assert(zoom >= 0 && zoom < kNumZooms);
    _m_hWnd = thisWidget;
    _m_highlightText[0] = '\0';

    gint ascent;
    gint descent;
    int textHeight;
    int margin = 3;
    gdk_string_extents(_m_pFont, "0123456789", NULL, NULL, NULL, &ascent, &descent);
    textHeight = descent + ascent;
    _m_height = textHeight + margin;
    gtk_widget_set_usize(_m_hWnd, -2, _m_height);
}

TTileHRuler::~TTileHRuler()
{
    if (_m_pBackBuffer)
        gdk_pixmap_unref(_m_pBackBuffer);
}

void TTileHRuler::move(const CPoint& pos, unsigned int width)
{
    MoveWindow(pos.x, pos.y, width, _m_height);
}

void TTileHRuler::setWidth(unsigned int newWidth)
{
    CRect wndRect;
    GetWindowRect(wndRect);
#line 104
    assert(wndRect.Height() == _m_height);
    gint x;
    gint y;
    gdk_window_get_deskrelative_origin(gdk_window_get_parent(_m_hWnd->window), &x, &y);
    wndRect.top -= y;
    wndRect.bottom -= y;
    wndRect.left -= x;
    wndRect.right -= x;
    move(wndRect.TopLeft(), newWidth);
}

void TTileHRuler::setStartTile(int startTileX)
{
#line 126
    assert(startTileX >= 0);
    assert(startTileX < _m_range);
    if (startTileX != _m_startTileX) {
        _m_startTileX = startTileX;
        OnPaint();
    }
}

void TTileHRuler::setZoom(TZoom newZoom)
{
#line 140
    assert(newZoom >= 0 && newZoom < kNumZooms);
    if (newZoom != _m_zoom) {
        _m_zoom = newZoom;
        OnPaint();
    }
}

void TTileHRuler::setRange(unsigned int newRange)
{
#line 155
    assert(newRange > 0);
    _m_range = newRange;
    _m_highlight = _m_range;
    setStartTile(0);
}

void TTileHRuler::setHighlight(unsigned int tile)
{
    tile = min(tile, _m_range);
    if (tile != _m_highlight) {
        GdkGC* gc = gdk_gc_new(_m_hWnd->window);
        if (_m_highlight < _m_range && _m_highlight >= _m_startTileX) {
            unsigned int width = _computeHighlightWidth();
            int pos = _computeHighlightPos(width);
            CRect rect(pos, 0, pos + width, _m_height);
            OnPaint(rect);
        }
        _m_highlight = tile;
        snprintf(_m_highlightText, 20, "%d", _m_highlight);
        if (_m_highlight < _m_range && _m_highlight >= _m_startTileX) {
            unsigned int width = _computeHighlightWidth();
            int pos = _computeHighlightPos(width);
            CRect rect(pos, 0, pos + width, _m_height);
            OnPaint();
        }
        gdk_gc_unref(gc);
    }
}

unsigned int TTileHRuler::_computeHighlightWidth() const
{
#line 205
    assert(_m_highlight < _m_range);
    assert(_m_highlight >= _m_startTileX);
    unsigned int textWidth = gdk_string_width(_m_pFont, _m_highlightText) + 4;
    unsigned int tileWidth = akZoomTraits[_m_zoom].m_tileSize;
    return tileWidth < textWidth ? textWidth : tileWidth;
}

int TTileHRuler::_computeHighlightPos(unsigned int width) const
{
#line 220
    assert(_m_highlight < _m_range);
    assert(_m_highlight >= _m_startTileX);
    unsigned int tileWidth = akZoomTraits[_m_zoom].m_tileSize;
#line 224
    assert(width >= tileWidth);
    int tilePos = (_m_highlight - _m_startTileX) * tileWidth;
    return tilePos - (width - tileWidth) / 2;
}

void TTileHRuler::OnPaint()
{
    if (_m_hWnd) {
        CRect rect(0, 0, _m_hWnd->allocation.width, _m_hWnd->allocation.height);
        OnPaint(rect);
    }
}

void TTileHRuler::OnPaint(const CRect& rect)
{
    if (!_m_hWnd || !_m_pBackBuffer)
        return;
    GdkGC* gc = gdk_gc_new(_m_hWnd->window);
    if (!gc)
        return;

    int tileWidth = akZoomTraits[_m_zoom].m_tileSize;
    GdkRectangle clipRect;
    CRect::CRect_to_gdk_rectangle(&rect, &clipRect);
    gdk_gc_set_clip_origin(gc, 0, 0);
    gdk_gc_set_clip_rectangle(gc, &clipRect);
    gdk_gc_set_foreground(gc, &_m_white);
    gdk_gc_set_background(gc, &_m_white);
    gdk_draw_rectangle(_m_pBackBuffer, gc, TRUE, rect.left, rect.top, rect.Width(), rect.Height());

    int fromX = _m_startTileX + rect.left / tileWidth;
#line 310
    assert(fromX >= 0);
    if (fromX < _m_range) {
        int toX = _m_startTileX + (rect.right + tileWidth - 1) / tileWidth;
        if (toX > _m_range)
            toX = _m_range;
#line 316
        assert(toX >= fromX);
        gdk_gc_set_foreground(gc, &_m_black);
        int x;
        for (x = fromX; x < toX; x++) {
            int lineX = (x - _m_startTileX) * tileWidth + tileWidth - 1;
            gdk_draw_line(_m_pBackBuffer, gc, lineX, 0, lineX, _m_height + 1);
        }
        gdk_gc_set_font(gc, _m_pFont);
        for (x = fromX; x < toX; x++) {
            CPoint pos((x - _m_startTileX) * tileWidth, 0);
            CRect tileRect(pos, CSize(tileWidth - 1, _m_height));
            tileRect.IntersectRect(rect, tileRect);
            GdkRectangle tileClipRect;
            CRect::CRect_to_gdk_rectangle(&tileRect, &tileClipRect);
            gdk_gc_set_clip_origin(gc, 0, 0);
            gdk_gc_set_clip_rectangle(gc, &tileClipRect);
            char text[20];
            snprintf(text, 20, "%d", x);
            CSize textSize(gdk_string_width(_m_pFont, text), gdk_string_height(_m_pFont, text));
            CPoint textPos = pos + CPoint((tileWidth - textSize.cx) / 2,
                                          (_m_height - textSize.cy) / 2 + textSize.cy);
            gdk_draw_string(_m_pBackBuffer, _m_pFont, gc, textPos.x, textPos.y, text);
            gdk_gc_set_clip_rectangle(gc, NULL);
        }
    }

    if (_m_highlight < _m_range && _m_highlight >= _m_startTileX) {
        unsigned int width = _computeHighlightWidth();
        int pos = _computeHighlightPos(width);
        if (rect.right > pos && pos + width > rect.left) {
            gdk_gc_set_foreground(gc, &_m_black);
            gdk_gc_set_background(gc, &_m_black);
            gdk_draw_rectangle(_m_pBackBuffer, gc, TRUE, pos, 0, width, _m_height);
            gdk_gc_set_foreground(gc, &_m_white);
            CSize textSize(gdk_string_width(_m_pFont, _m_highlightText),
                           gdk_string_height(_m_pFont, _m_highlightText));
            CPoint textPos(pos + (width - textSize.cx) / 2,
                           (_m_height - textSize.cy) / 2 + textSize.cy);
            gdk_draw_string(_m_pBackBuffer, _m_pFont, gc, textPos.x, textPos.y, _m_highlightText);
        }
    }

    gdk_draw_pixmap(_m_hWnd->window, gc, _m_pBackBuffer, rect.left, rect.top,
                    rect.left, rect.top, rect.Width(), rect.Height());
    gdk_gc_unref(gc);
}

void TTileHRuler::OnSize(unsigned int type, int cx, int cy)
{
    if (cx == 0 || cy == 0 || _m_hWnd == NULL || _m_hWnd->window == NULL)
        return;
    if (_m_pBackBuffer)
        gdk_pixmap_unref(_m_pBackBuffer);
    _m_pBackBuffer = gdk_pixmap_new(_m_hWnd->window, cx, cy, -1);
}
