// TileVRuler.h - the map's vertical tile ruler (TileVRuler.cpp; Loki
// h3maped object 59): the numbered tile rows left of the map edit window
// with the cursor's row highlighted. Like the horizontal ruler, but its
// numbers read upwards: each is drawn into a bitmap of its own and copied
// into the back buffer turned a quarter left, so the back buffer is a
// 16-bit DIB section. Layout from the constructor (0x78 bytes, the map
// frame's new): the width, the first visible tile, the tile count, the
// highlighted tile and its text, the zoom, the back buffer and the font.
#ifndef HOMM3_EDITOR_TILEVRULER_H
#define HOMM3_EDITOR_TILEVRULER_H

#include "editor/DIBSection.h"
#include "editor/Tile.h"

class TTileVRuler : public CWnd {
public:
    TTileVRuler(CWnd* pParent, unsigned int range, TZoom zoom);

    void move(const CPoint& pos, unsigned int height);
    void setStartTile(unsigned int startTile);
    void setZoom(TZoom zoom);
    void setRange(unsigned int range);
    void setHighlight(unsigned int tile);
    unsigned int getWidth() const { return _m_width; }

protected:
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    DECLARE_MESSAGE_MAP()

private:
    unsigned int _computeHighlightHeight(CDC* pDC);

    unsigned int _m_width;
    unsigned int _m_startTile;
    unsigned int _m_range;
    unsigned int _m_highlight;
    CString _m_highlightText;
    TZoom _m_zoom;
    T16bppDIBSection _m_backBuffer;
    CFont _m_font;
};

#endif  /* HOMM3_EDITOR_TILEVRULER_H */
