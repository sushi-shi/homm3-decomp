// TileVRuler.cpp - the vertical tile ruler (h3maped 0x4bf365..0x4c016c,
// after TilePoint.cpp's neighbour bodies; Loki h3maped object 59).
// /OPT:ICF folded setZoom and setRange onto the horizontal ruler's
// identical bodies, and that ruler's setStartTile onto this one's.
#include "editor/stdafx.h"

#include <algorithm>

#include "va.h"
#include "editor/DCAttributeSelector.h"
#include "editor/GDIObjectSelector.h"
#include "editor/MemoryDC.h"
#include "editor/TileVRuler.h"

VA(0x004bf61b, 0x17d)
TTileVRuler::TTileVRuler(CWnd* pParent, unsigned int range, TZoom zoom)
    : _m_startTile(0),
      _m_range(range),
      _m_highlight(range),
      _m_zoom(zoom)
{
    DATA_COMPGEN_GUARD(0x005a506c, vRulerClassNameGuard, className)
    VA_COMPGEN(0x004bf798, 0xa, STATIC_DTOR, className)
    DATA(0x005a5068) static CString className;
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
    _m_width = tm.tmHeight + tm.tmExternalLeading;
}

VA_COMPGEN(0x004bf7a2, 0x1c, SCALAR_DELETING_DTOR, TTileVRuler)
VA_COMPGEN(0x004bf7be, 0x61, IMPLICIT_DTOR, TTileVRuler)

VA(0x004bf81f, 0x1a)
void TTileVRuler::move(const CPoint& pos, unsigned int height)
{
    MoveWindow(pos.x, pos.y, _m_width, height);
}

VA(0x004bf839, 0x29)
void TTileVRuler::setStartTile(unsigned int startTile)
{
    if (startTile != _m_startTile) {
        _m_startTile = startTile;
        Invalidate(FALSE);
        UpdateWindow();
    }
}

void TTileVRuler::setZoom(TZoom zoom)
{
    if (zoom != _m_zoom) {
        _m_zoom = zoom;
        Invalidate(FALSE);
        UpdateWindow();
    }
}

void TTileVRuler::setRange(unsigned int range)
{
    _m_range = range;
    _m_highlight = range;
    setStartTile(0);
}

VA(0x004bf862, 0x138)
void TTileVRuler::setHighlight(unsigned int tile)
{
    tile = min(tile, _m_range);
    if (tile == _m_highlight)
        return;
    CClientDC dc(this);
    TGDIObjectSelector<CFont> fontSelector(&dc, &_m_font);
    if (_m_highlight < _m_range) {
        unsigned int height = _computeHighlightHeight(&dc);
        unsigned int tileHeight = akZoomTraits[_m_zoom].m_tileSize;
        int pos = (_m_highlight - _m_startTile) * tileHeight - (height - tileHeight) / 2;
        CRect rect(0, pos, _m_width, pos + height);
        InvalidateRect(&rect, FALSE);
    }
    _m_highlight = tile;
    _m_highlightText.Format("%d", tile);
    if (_m_highlight < _m_range) {
        unsigned int height = _computeHighlightHeight(&dc);
        unsigned int tileHeight = akZoomTraits[_m_zoom].m_tileSize;
        int pos = (_m_highlight - _m_startTile) * tileHeight - (height - tileHeight) / 2;
        CRect rect(0, pos, _m_width, pos + height);
        InvalidateRect(&rect, FALSE);
    }
    UpdateWindow();
}

VA(0x004bf99a, 0x5c)
unsigned int TTileVRuler::_computeHighlightHeight(CDC* pDC)
{
    TGDIObjectSelector<CFont> fontSelector(pDC, &_m_font);
    unsigned int textHeight = pDC->GetTextExtent(_m_highlightText).cx + 4;
    unsigned int tileHeight = akZoomTraits[_m_zoom].m_tileSize;
    return tileHeight < textHeight ? textHeight : tileHeight;
}

VA(0x004bf9f6, 0x6)
BEGIN_MESSAGE_MAP(TTileVRuler, CWnd)
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_PAINT()
    ON_WM_SIZE()
END_MESSAGE_MAP()

VA(0x004bf9fc, 0x16)
void TTileVRuler::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    Default();
    lpwndpos->cx = _m_width;
}

// Each number is drawn left to right into a bitmap of its own; its pixels
// go into the back buffer column by column from the bottom up.
VA(0x004bfa12, 0x730)
void TTileVRuler::OnPaint()
{
    CPaintDC paintDC(this);
    int tileHeight = akZoomTraits[_m_zoom].m_tileSize;
    TMemoryDC dc(&paintDC);
    TGDIObjectSelector<CBitmap> bitmapSelector(&dc, &_m_backBuffer);
    CRect clipRect;
    paintDC.GetClipBox(&clipRect);
    dc.SelectClipRgn(NULL);
    dc.IntersectClipRect(&clipRect);
    CBrush backgroundBrush(GetSysColor(COLOR_WINDOW));
    TGDIObjectSelector<CBrush> backgroundBrushSelector(&dc, &backgroundBrush);
    dc.PatBlt(clipRect.left, clipRect.top, clipRect.Width(), clipRect.Height(), PATCOPY);
    unsigned int fromY = _m_startTile + clipRect.top / tileHeight;
    if (fromY < _m_range) {
        unsigned int toY = _m_startTile + (clipRect.bottom + tileHeight - 1) / tileHeight;
        if (toY > _m_range)
            toY = _m_range;
        CPen pen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWTEXT));
        TGDIObjectSelector<CPen> penSelector(&dc, &pen);
        int y;
        for (y = fromY; y < (int)toY; y++) {
            int lineY = (y - _m_startTile + 1) * tileHeight - 1;
            dc.MoveTo(0, lineY);
            dc.LineTo(_m_width + 1, lineY);
        }
        T16bppDIBSection labelBitmap(tileHeight, _m_width);
        TMemoryDC labelDC(&dc);
        TGDIObjectSelector<CBitmap> labelBitmapSelector(&labelDC, &labelBitmap);
        TGDIObjectSelector<CFont> fontSelector(&labelDC, &_m_font);
        TTextColorSelector textColorSelector(&labelDC, GetSysColor(COLOR_WINDOWTEXT));
        TBkColorSelector bkColorSelector(&labelDC, GetSysColor(COLOR_WINDOW));
        for (y = fromY; y < (int)toY; y++) {
            CString text;
            text.Format("%d", y);
            CSize textSize = labelDC.GetTextExtent(text);
            int textX = (tileHeight - textSize.cx) / 2;
            unsigned int textY = (_m_width - textSize.cy) / 2;
            labelDC.TextOut(textX, textY, text);
            int top = (int)textY > 0 ? textY : 0;
            int left = textX > 1 ? textX : 1;
            unsigned int bottom = min(textY + textSize.cy, _m_width);
            int right = min(textX + textSize.cx, tileHeight);
            int destY = (y - _m_startTile + 1) * tileHeight - left;
            int first = left;
            if (destY > clipRect.bottom)
                first = destY - clipRect.bottom + left;
            if (first < right && top < (int)bottom) {
                for (int row = top; row < (int)bottom; row++) {
                    const uword* pSrc = (const uword*)((const ubyte*)labelBitmap.getPixels()
                                                       + labelBitmap.getPitch() * row) + first;
                    const uword* pSrcEnd = pSrc + (right - first);
                    uword* pDest = (uword*)((ubyte*)_m_backBuffer.getPixels()
                                            + _m_backBuffer.getPitch() * (destY - 1)) + row;
                    while (pSrc < pSrcEnd) {
                        *pDest = *pSrc++;
                        pDest = (uword*)((ubyte*)pDest - _m_backBuffer.getPitch());
                    }
                }
            }
        }
    }
    if (_m_highlight < _m_range) {
        unsigned int height = _computeHighlightHeight(&dc);
        unsigned int highlightTileHeight = akZoomTraits[_m_zoom].m_tileSize;
        int pos = (_m_highlight - _m_startTile) * highlightTileHeight - (height - highlightTileHeight) / 2;
        if (clipRect.bottom > pos && pos + height > (unsigned int)clipRect.top) {
            {
                CBrush brush(GetSysColor(COLOR_WINDOWTEXT));
                TGDIObjectSelector<CBrush> brushSelector(&dc, &brush);
                dc.PatBlt(0, pos, _m_width, height, PATCOPY);
            }
            TMemoryDC labelDC(&dc);
            CSize textSize;
            {
                TGDIObjectSelector<CFont> fontSelector(&labelDC, &_m_font);
                textSize = labelDC.GetTextExtent(_m_highlightText);
            }
            T16bppDIBSection labelBitmap(textSize.cx, textSize.cy);
            {
                TGDIObjectSelector<CBitmap> labelBitmapSelector(&labelDC, &labelBitmap);
                TGDIObjectSelector<CFont> fontSelector(&labelDC, &_m_font);
                TTextColorSelector textColorSelector(&labelDC, GetSysColor(COLOR_WINDOW));
                TBkColorSelector bkColorSelector(&labelDC, GetSysColor(COLOR_WINDOWTEXT));
                labelDC.TextOut(0, 0, _m_highlightText);
            }
            unsigned int column = (_m_width - textSize.cy) / 2;
            int destY = pos + height - (height - textSize.cx) / 2;
            int skip = 0;
            if (destY > clipRect.bottom) {
                skip = destY - clipRect.bottom;
                destY = clipRect.bottom;
            }
            int end = textSize.cx;
            if (destY - textSize.cx + skip < clipRect.top)
                end = destY - clipRect.top + skip;
            if (skip < end) {
                for (int row = 0; row < textSize.cy; row++) {
                    const uword* pSrc = (const uword*)((const ubyte*)labelBitmap.getPixels()
                                                       + labelBitmap.getPitch() * row) + skip;
                    const uword* pSrcEnd = pSrc + (end - skip);
                    uword* pDest = (uword*)((ubyte*)_m_backBuffer.getPixels()
                                            + _m_backBuffer.getPitch() * (destY - 1)) + column + row;
                    while (pSrc < pSrcEnd) {
                        *pDest = *pSrc++;
                        pDest = (uword*)((ubyte*)pDest - _m_backBuffer.getPitch());
                    }
                }
            }
        }
    }
    paintDC.BitBlt(clipRect.left, clipRect.top, clipRect.Width(), clipRect.Height(), &dc, clipRect.left,
                   clipRect.top, SRCCOPY);
}

VA(0x004c0142, 0x2a)
void TTileVRuler::OnSize(UINT nType, int cx, int cy)
{
    Default();
    if (cx != 0 && cy != 0)
        _m_backBuffer.create(cx, cy);
}
