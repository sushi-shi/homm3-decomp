// TileHRuler.h - the map's horizontal tile ruler (Loki TileHRuler.cpp).
// Methods as the image declares them; return types from __PRETTY_FUNCTION__
// texts or the retail bodies. The members follow the constructor; the
// assert texts name _m_height, _m_startTileX, _m_range and _m_highlight;
// the others' names are not proven.
#ifndef HOMM3_EDITOR_TILEHRULER_H
#define HOMM3_EDITOR_TILEHRULER_H

#include "editor/stdafx.h"
#include "editor/Tile.h"

class TTileHRuler : public CWnd {
public:
    TTileHRuler(GtkWidget* thisWidget, unsigned int range, TZoom zoom, GdkFont* pFont);
    virtual ~TTileHRuler();

    void move(const CPoint& pos, unsigned int width);
    void setWidth(unsigned int newWidth);
    void setStartTile(int startTile);
    void setZoom(TZoom zoom);
    void setRange(unsigned int range);
    void setHighlight(unsigned int tile);
    int getHeight() const { return _m_height; }

    void OnPaint();
    void OnPaint(const CRect& rect);
    void OnSize(unsigned int type, int cx, int cy);

private:
    unsigned int _computeHighlightWidth() const;
    int _computeHighlightPos(unsigned int width) const;

    int _m_height;
    int _m_startTileX;
    unsigned int _m_range;
    unsigned int _m_highlight;
    char _m_highlightText[20];
    TZoom _m_zoom;
    GdkPixmap* _m_pBackBuffer;
    GdkFont* _m_pFont;
};

#endif  /* HOMM3_EDITOR_TILEHRULER_H */
