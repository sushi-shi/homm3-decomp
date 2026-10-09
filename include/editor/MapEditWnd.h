// MapEditWnd.h - the map edit window (MapEditWnd.cpp; Loki h3maped object
// 55), the editing window over one map layer that the map frame embeds. It
// draws the layer into a 16-bit DIB section back buffer, tracks the
// selection, the floating (grabbed) object, the terrain brush and the fill
// rectangle, scrolls through its own scroll bars and the middle-button
// panner, shows object tool tips and reports every edit to its controller.
// Copy and paste go through a registered clipboard format.
//
// The Windows class (0x128 bytes, the map frame's new) adds the panner's
// client interface at +0x3c (vtable 0x539d14) and overrides
// PreTranslateMessage and WindowProc. Layout from the constructor: the
// controller, the window id, the map, the obstacle area, the layer flag,
// the panner, the tool tip control, the view position, the back buffer
// and then Loki's members in Loki's order; the selection and brush frames
// are drawn with a white XOR pen at +0x11c, and +0x124 keeps the clipboard
// data handle until the clipboard is destroyed. The _TMode enumerators are
// Loki's.
#ifndef HOMM3_EDITOR_MAPEDITWND_H
#define HOMM3_EDITOR_MAPEDITWND_H

#include "editor/DIBSection.h"
#include "editor/GameMap.h"
#include "editor/MapEditingWnd.h"
#include "editor/PannerCtrl.h"
#include "editor/Tile.h"

class TGUIGameObject;
class TGameMapMask;

class TMapEditWnd : public TMapEditingWnd, private TPannerCtrlClient {
public:
    enum _TMode {
        _eModeSel,
        _eModeBrush,
        _eModeFill
    };

    TMapEditWnd(CWnd* pParent, TMapEditingWnd::TController* pController, int id, const TGameMap* pMap,
                const TGameMapMask* pObstacleMask, bool bSecondLayer, TZoom zoom, bool bShowGrid,
                bool bShowPassability);
    virtual ~TMapEditWnd();
    virtual BOOL PreTranslateMessage(MSG* pMsg);

    void clearMap();
    void setMapLayer(const TGameMap* pMap, const TGameMapMask* pObstacleMask, bool bSecondLayer);
    void moveViewRect(const CPoint& pos);
    void update(const CRect& rect);
    void setZoom(TZoom zoom);
    void showGrid(bool bShow);
    void showPassability(bool bShow);
    void selectObject(unsigned int objID);
    void grabObject(const TGUIGameObject* pObj);
    void onUndo();
    void onObjectRemoved(unsigned int objID);
    void animate(unsigned int frameNum, bool bForce);
    void resetAnimation();
    void makeVisible(unsigned int objID);
    void selectionMode();
    void brushMode(const CSize& size);
    void fillMode();
    TMapLayerObjectID getSelectedObjectID() const { return _m_selectedObjID; }

protected:
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnPaint();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnEditProperties();
    afx_msg void OnEditDelete();
    afx_msg void OnLeft();
    afx_msg void OnRight();
    afx_msg void OnUp();
    afx_msg void OnDown();
    afx_msg void OnEditCut();
    afx_msg void OnEditCopy();
    afx_msg void OnEditPaste();
    afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
    afx_msg void OnDestroyClipboard();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg void OnCaptureChanged(CWnd* pWnd);
    afx_msg LRESULT OnDeferredScroll(WPARAM wParam, LPARAM lParam);
    afx_msg BOOL OnToolTipNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnHelpWhatsThis();
    afx_msg void OnUpdateSelectionCommand(CCmdUI* pCmdUI);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    DECLARE_MESSAGE_MAP()

private:
    virtual bool canPanHorizontally();
    virtual bool canPanVertically();
    virtual void pan(int dx, int dy);

    void _turnAutoScrollOn();
    void _turnAutoScrollOff();
    void _handleAutoScroll(const CPoint& point);
    void _setMode(_TMode mode);
    void _setToolTipObj(unsigned int objID);
    void _clearToolTipObj();
    void _turnBrushOn();
    void _turnBrushOff();
    void _setBrushPos(const CPoint& pos);
    void _setBrushSize(const CSize& size);
    void _turnFillRectOff();
    void _setFillRectAnchor(const CPoint& pos);
    void _setFillRectDragPos(const CPoint& pos);
    void _drawSelectionFrame(CDC* pDC);
    void _drawBrush(CDC* pDC);
    void _paintRect(CDC* pDC, CRect& rect);
    void _drawMap(CDC* pDC, CRect& rect);
    void _generateMouseMove();
    bool _copySelectedObject();
    TMapLayerObjectID _pickObject(const CPoint& point) const;
    CPoint _computeFloatingObjPos(const CPoint& point) const;
    CRect _computeFloatingObjRect(const CPoint& point) const;
    CSize _computeViewRectSize(int cx, int cy) const;
    CPoint _computeBrushPos(const CPoint& point) const;
    CRect _computeFillRect() const;
    CPoint _computeFillRectDragPos(const CPoint& point) const;
    const TGameMap::TLayer& _getMapLayer() const;
    void _setCursor(HCURSOR hCursor)
    {
        _m_hCursor = hCursor;
        ::SetCursor(hCursor);
    }

    TMapEditingWnd::TController* _m_pController;
    int _m_id;
    const TGameMap* _m_pMap;
    const TGameMapMask* _m_pObstacleMask;
    bool _m_bSecondLayer;
    TPannerCtrl* _m_pPanner;
    CToolTipCtrl* _m_pToolTip;
    CPoint _m_viewPos;
    T16bppDIBSection _m_backBuffer;
    unsigned int _m_frameNum;
    bool _m_bDragging;
    HCURSOR _m_hCursor;
    bool _m_bMouseInside;
    CPoint _m_cursorTilePos;
    CPoint _m_scrollDelta;
    bool _m_bAutoScrollOn;
    CPoint _m_autoScrollPoint;
    DWORD _m_autoScrollTime;
    int _m_hAutoScrollDir;
    unsigned int _m_hAutoScrollInterval;
    unsigned int _m_hAutoScrollDelay;
    int _m_vAutoScrollDir;
    unsigned int _m_vAutoScrollInterval;
    unsigned int _m_vAutoScrollDelay;
    _TMode _m_mode;
    TZoom _m_zoom;
    bool _m_bShowGrid;
    bool _m_bShowPassability;
    TMapLayerObjectID _m_selectedObjID;
    TMapLayerObjectID _m_toolTipObjID;
    const TGUIGameObject* _m_pFloatingObj;
    CPoint _m_floatingObjPos;
    TMapLayerObjectID _m_potentialGrabID;
    bool _m_bPotentialCopy;
    CPoint _m_potentialGrabPoint;
    bool _m_bBrushOn;
    CPoint _m_brushPos;
    CSize _m_brushSize;
    CPoint _m_fillRectAnchor;
    CPoint _m_fillRectDragPos;
    CPen _m_pen;
    HGLOBAL _m_hClipboardData;
};

#endif  /* HOMM3_EDITOR_MAPEDITWND_H */
