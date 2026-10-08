// TileVRuler.h - the map's vertical tile ruler (Loki TileVRuler.cpp).
// Methods as the image declares them; return types from __PRETTY_FUNCTION__
// texts or the retail bodies. The members follow the constructor; the
// assert texts name _m_width, _m_startTileY, _m_range and _m_highlight;
// the others' names are not proven.
#ifndef HOMM3_EDITOR_TILEVRULER_H
#define HOMM3_EDITOR_TILEVRULER_H

#include "editor/stdafx.h"
#include "editor/Tile.h"

class TTileVRuler : public CWnd {
public:
    TTileVRuler(GtkWidget* thisWidget, unsigned int range, TZoom zoom, GdkFont* pFont);
    virtual ~TTileVRuler();

    void move(const CPoint& pos, unsigned int height);
    void setHeight(unsigned int newHeight);
    void setStartTile(int startTile);
    void setZoom(TZoom zoom);
    void setRange(unsigned int range);
    void setHighlight(unsigned int tile);
    int getWidth() const { return _m_width; }

    void OnPaint();
    void OnPaint(const CRect& rect);
    void OnSize(unsigned int type, int cx, int cy);

private:
    unsigned int _computeHighlightHeight() const;
    int _computeHighlightPos(unsigned int height) const;

    int _m_width;
    int _m_startTileY;
    unsigned int _m_range;
    unsigned int _m_highlight;
    char _m_highlightText[20];
    TZoom _m_zoom;
    GdkPixmap* _m_pBackBuffer;
    GdkFont* _m_pFont;
};

#endif  /* HOMM3_EDITOR_TILEVRULER_H */
