// TileVRuler.h - the map's vertical tile ruler (Loki TileVRuler.cpp).
// Methods as the image declares them; return types from __PRETTY_FUNCTION__
// texts or the retail bodies. Data members are not declared yet.
#ifndef HOMM3_EDITOR_TILEVRULER_H
#define HOMM3_EDITOR_TILEVRULER_H

#include "editor/stdafx.h"
#include "editor/Tile.h"

class TTileVRuler : public CWnd {
public:
    TTileVRuler(GtkWidget* thisWidget, unsigned int range, TZoom zoom, GdkFont* pFont);
    virtual ~TTileVRuler();

    void move(const CPoint& pos, unsigned int length);
    void setHeight(unsigned int newHeight);
    void setStartTile(int startTile);
    void setZoom(TZoom zoom);
    void setRange(unsigned int range);
    void setHighlight(unsigned int tile);
    unsigned int getWidth() const;

    void OnPaint();
    void OnPaint(const CRect& rect);
    void OnSize(unsigned int type, int cx, int cy);

private:
    unsigned int _computeHighlightHeight() const;
    int _computeHighlightPos(unsigned int tile) const;
};

#endif  /* HOMM3_EDITOR_TILEVRULER_H */
