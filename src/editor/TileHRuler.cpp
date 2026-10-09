// TileHRuler.cpp - the horizontal tile ruler (h3maped 0x4be7e0..0x4bf365;
// Loki h3maped object 58). /OPT:ICF folded setStartTile onto the vertical
// ruler's identical body; the vertical ruler's setZoom and setRange fold
// onto this ruler's.
#include "editor/stdafx.h"

#include <algorithm>

#include "va.h"
#include "editor/DCAttributeSelector.h"
#include "editor/GDIObjectSelector.h"
#include "editor/MemoryDC.h"
#include "editor/TileHRuler.h"

VA(0x004be9b4, 0x17b)
TTileHRuler::TTileHRuler(CWnd* pParent, unsigned int range, TZoom zoom)
    : _m_startTile(0),
      _m_range(range),
      _m_highlight(range),
      _m_zoom(zoom)
{
    DATA_COMPGEN_GUARD(0x005a4ff8, hRulerClassNameGuard, className)
    VA_COMPGEN(0x004beb2f, 0xa, STATIC_DTOR, className)
    DATA(0x005a4ff4) static CString className;
    if (className.IsEmpty())
        className = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW));
    if (!Create(className, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, CRect(0, 0, 0, 0), pParent, 0))
        throw TRuntimeError();
    CClientDC dc(this);
    if ((_m_font.m_hObject = ::GetStockObject(DEFAULT_GUI_FONT)) == NULL
        && (_m_font.m_hObject = ::GetStockObject(SYSTEM_FONT)) == NULL)
        throw TRuntimeError();
    TGDIObjectSelector<CFont> fontSelector(&dc, &_m_font);
    TEXTMETRIC tm;
    dc.GetTextMetrics(&tm);
    _m_height = tm.tmHeight + tm.tmExternalLeading;
}

VA_COMPGEN(0x004beb39, 0x1c, SCALAR_DELETING_DTOR, TTileHRuler)
VA_COMPGEN(0x004beb55, 0x69, IMPLICIT_DTOR, TTileHRuler)

VA(0x004bebbe, 0x1a)
void TTileHRuler::move(const CPoint& pos, unsigned int width)
{
    MoveWindow(pos.x, pos.y, width, _m_height);
}

void TTileHRuler::setStartTile(unsigned int startTile)
{
    if (startTile != _m_startTile) {
        _m_startTile = startTile;
        Invalidate(FALSE);
        UpdateWindow();
    }
}

VA(0x004bebd8, 0x29)
void TTileHRuler::setZoom(TZoom zoom)
{
    if (zoom != _m_zoom) {
        _m_zoom = zoom;
        Invalidate(FALSE);
        UpdateWindow();
    }
}

VA(0x004bec01, 0x14)
void TTileHRuler::setRange(unsigned int range)
{
    _m_range = range;
    _m_highlight = range;
    setStartTile(0);
}

VA(0x004bec15, 0x138)
void TTileHRuler::setHighlight(unsigned int tile)
{
    tile = min(tile, _m_range);
    if (tile == _m_highlight)
        return;
    CClientDC dc(this);
    TGDIObjectSelector<CFont> fontSelector(&dc, &_m_font);
    if (_m_highlight < _m_range) {
        unsigned int width = _computeHighlightWidth(&dc);
        unsigned int tileWidth = akZoomTraits[_m_zoom].m_tileSize;
        int pos = (_m_highlight - _m_startTile) * tileWidth - (width - tileWidth) / 2;
        CRect rect(pos, 0, pos + width, _m_height);
        InvalidateRect(&rect, FALSE);
    }
    _m_highlight = tile;
    _m_highlightText.Format("%d", tile);
    if (_m_highlight < _m_range) {
        unsigned int width = _computeHighlightWidth(&dc);
        unsigned int tileWidth = akZoomTraits[_m_zoom].m_tileSize;
        int pos = (_m_highlight - _m_startTile) * tileWidth - (width - tileWidth) / 2;
        CRect rect(pos, 0, pos + width, _m_height);
        InvalidateRect(&rect, FALSE);
    }
    UpdateWindow();
}

// The highlight is the tile's width, or wider when its number needs more.
VA(0x004bed4d, 0x5c)
unsigned int TTileHRuler::_computeHighlightWidth(CDC* pDC)
{
    TGDIObjectSelector<CFont> fontSelector(pDC, &_m_font);
    unsigned int textWidth = pDC->GetTextExtent(_m_highlightText).cx + 4;
    unsigned int tileWidth = akZoomTraits[_m_zoom].m_tileSize;
    return tileWidth < textWidth ? textWidth : tileWidth;
}

VA(0x004beda9, 0x6)
BEGIN_MESSAGE_MAP(TTileHRuler, CWnd)
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_PAINT()
    ON_WM_SIZE()
END_MESSAGE_MAP()

// The ruler keeps its height whatever its parent asks.
VA(0x004bedaf, 0x16)
void TTileHRuler::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    Default();
    lpwndpos->cy = _m_height;
}

VA(0x004bedc5, 0x4ce)
void TTileHRuler::OnPaint()
{
    CPaintDC paintDC(this);
    int tileWidth = akZoomTraits[_m_zoom].m_tileSize;
    TMemoryDC dc(&paintDC);
    TGDIObjectSelector<CBitmap> bitmapSelector(&dc, &_m_backBuffer);
    CRect clipRect;
    paintDC.GetClipBox(&clipRect);
    dc.SelectClipRgn(NULL);
    dc.IntersectClipRect(&clipRect);
    CBrush backgroundBrush(GetSysColor(COLOR_WINDOW));
    TGDIObjectSelector<CBrush> backgroundBrushSelector(&dc, &backgroundBrush);
    dc.PatBlt(clipRect.left, clipRect.top, clipRect.Width(), clipRect.Height(), PATCOPY);
    unsigned int fromX = _m_startTile + clipRect.left / tileWidth;
    if (fromX < _m_range) {
        unsigned int toX = _m_startTile + (clipRect.right + tileWidth - 1) / tileWidth;
        if (toX > _m_range)
            toX = _m_range;
        CPen pen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWTEXT));
        TGDIObjectSelector<CPen> penSelector(&dc, &pen);
        int x;
        for (x = fromX; x < (int)toX; x++) {
            int lineX = (x - _m_startTile + 1) * tileWidth - 1;
            dc.MoveTo(lineX, 0);
            dc.LineTo(lineX, _m_height + 1);
        }
        TGDIObjectSelector<CFont> fontSelector(&dc, &_m_font);
        TTextColorSelector textColorSelector(&dc, GetSysColor(COLOR_WINDOWTEXT));
        TBkColorSelector bkColorSelector(&dc, GetSysColor(COLOR_WINDOW));
        dc.SelectClipRgn(NULL);
        for (x = fromX; x < (int)toX; x++) {
            int left = (x - _m_startTile) * tileWidth;
            CRect tileRect(left, 0, left + tileWidth - 1, _m_height);
            dc.IntersectClipRect(&tileRect);
            CString text;
            text.Format("%d", x);
            CSize textSize = dc.GetTextExtent(text);
            dc.TextOut(left + (tileWidth - textSize.cx) / 2, (_m_height - textSize.cy) / 2, text);
            dc.SelectClipRgn(NULL);
        }
        dc.IntersectClipRect(&clipRect);
    }
    if (_m_highlight < _m_range) {
        unsigned int width = _computeHighlightWidth(&dc);
        unsigned int highlightTileWidth = akZoomTraits[_m_zoom].m_tileSize;
        int pos = (_m_highlight - _m_startTile) * highlightTileWidth - (width - highlightTileWidth) / 2;
        if (clipRect.right > pos && pos + width > (unsigned int)clipRect.left) {
            CBrush brush(GetSysColor(COLOR_WINDOWTEXT));
            TGDIObjectSelector<CBrush> brushSelector(&dc, &brush);
            dc.PatBlt(pos, 0, width, _m_height, PATCOPY);
            TGDIObjectSelector<CFont> fontSelector(&dc, &_m_font);
            TTextColorSelector textColorSelector(&dc, GetSysColor(COLOR_WINDOW));
            TBkColorSelector bkColorSelector(&dc, GetSysColor(COLOR_WINDOWTEXT));
            CSize textSize = dc.GetTextExtent(_m_highlightText);
            dc.TextOut(pos + (width - textSize.cx) / 2, (_m_height - textSize.cy) / 2, _m_highlightText);
        }
    }
    paintDC.BitBlt(clipRect.left, clipRect.top, clipRect.Width(), clipRect.Height(), &dc, clipRect.left,
                   clipRect.top, SRCCOPY);
}

VA(0x004bf293, 0x77)
void TTileHRuler::OnSize(UINT nType, int cx, int cy)
{
    Default();
    if (cx != 0 && cy != 0) {
        HGDIOBJ hOldBackBuffer = _m_backBuffer.Detach();
        if (hOldBackBuffer != NULL)
            ::DeleteObject(hOldBackBuffer);
        CClientDC dc(this);
        _m_backBuffer.Attach(::CreateCompatibleBitmap(dc.m_hDC, cx, cy));
    }
}
