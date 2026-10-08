// TileHRuler.h - the map's horizontal tile ruler (Loki TileHRuler.cpp).
// Methods as the image declares them; return types from __PRETTY_FUNCTION__
// texts or the retail bodies. Data members are not declared yet.
#ifndef HOMM3_EDITOR_TILEHRULER_H
#define HOMM3_EDITOR_TILEHRULER_H

#include "editor/stdafx.h"
#include "editor/Tile.h"

class TTileHRuler : public CWnd {
public:
    TTileHRuler(GtkWidget* thisWidget, unsigned int range, TZoom zoom, GdkFont* pFont);
    virtual ~TTileHRuler();

    void move(const CPoint& pos, unsigned int length);
    void setWidth(unsigned int newWidth);
    void setStartTile(int startTile);
    void setZoom(TZoom zoom);
    void setRange(unsigned int range);
    void setHighlight(unsigned int tile);
    unsigned int getHeight() const;

    void OnPaint();
    void OnPaint(const CRect& rect);
    void OnSize(unsigned int type, int cx, int cy);

private:
    unsigned int _computeHighlightWidth() const;
    int _computeHighlightPos(unsigned int tile) const;
};

#endif  /* HOMM3_EDITOR_TILEHRULER_H */
