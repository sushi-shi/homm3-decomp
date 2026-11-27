// TileHRuler.h - the map's horizontal tile ruler (TileHRuler.cpp; Loki
// h3maped object 58): the numbered tile columns above the map edit window
// with the cursor's column highlighted. The Windows ruler is a child window
// that paints through a back buffer bitmap in the system colours with the
// default GUI font. Layout from the constructor (0x64 bytes, the map
// frame's new): the height, the first visible tile, the tile count, the
// highlighted tile and its text, the zoom, the back buffer and the font.
// Loki's asserts name _m_height, _m_startTileX, _m_range and _m_highlight.
// Loki declares the height and start tile int (setStartTile(int)); the
// Windows rulers compare and divide them unsigned (int extents drop both
// OnPaints to 95.95/85.74, an int start tile the vertical one to 98.40).
#ifndef HOMM3_EDITOR_TILEHRULER_H
#define HOMM3_EDITOR_TILEHRULER_H

#include "editor/Tile.h"

class TTileHRuler : public CWnd {
public:
    TTileHRuler(CWnd* pParent, unsigned int range, TZoom zoom);
    ~TTileHRuler();

    void move(const CPoint& pos, unsigned int width);
    void setStartTile(unsigned int startTile);
    void setZoom(TZoom zoom);
    void setRange(unsigned int range);
    void setHighlight(unsigned int tile);
    unsigned int getHeight() const { return _m_height; }

protected:
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    DECLARE_MESSAGE_MAP()

private:
    unsigned int _computeHighlightWidth(CDC* pDC);
    int _computeHighlightPos(unsigned int width) const;

    unsigned int _m_height;
    unsigned int _m_startTileX;
    unsigned int _m_range;
    unsigned int _m_highlight;
    CString _m_highlightText;
    TZoom _m_zoom;
    CBitmap _m_backBuffer;
    CFont _m_font;
};

#endif  /* HOMM3_EDITOR_TILEHRULER_H */
