// PannerCtrl.h - the map window's middle-button panner (PannerCtrl.cpp;
// GOG only). A middle click shows the panner's bitmap under the cursor;
// while the button is held, or after a quick click that leaves it
// sticky, the distance of the cursor from the bitmap scrolls the client
// at a rate that grows with the square of that distance. RTTI names
// TPannerCtrl (46 CWnd slots) and its abstract TPannerCtrlClient (three
// pure slots, implemented by TMapEditWnd from its scroll bars). Layout
// from the image: the client at 0x3c, the mode, the sticky flag, the
// window focused before the panner, the bitmap, its size, the click and
// last tick times, the cursor position and the two axes' rates at 0x6c
// and 0x78 (0x84 bytes, TMapEditWnd's new).
#ifndef HOMM3_EDITOR_PANNERCTRL_H
#define HOMM3_EDITOR_PANNERCTRL_H

class TPannerCtrlClient {
public:
    virtual bool canPanHorizontally() = 0;
    virtual bool canPanVertically() = 0;
    virtual void pan(int dx, int dy) = 0;
};

class TPannerCtrl : public CWnd {
public:
    TPannerCtrl(CWnd* pParent, TPannerCtrlClient* pClient);
    virtual ~TPannerCtrl();

    void relayEvent(const MSG* pMsg);

protected:
    afx_msg void OnPaint();
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnCaptureChanged(CWnd* pWnd);
    DECLARE_MESSAGE_MAP()

private:
    enum EMode {
        eNone,
        eHorizontal,
        eVertical,
        eBoth
    };

    // A step's interval in milliseconds: the scale over the squared
    // distance, within the bounds.
    enum {
        s_kMinInterval = 30,
        s_kMaxInterval = 1200,
        s_kIntervalScale = 1440000
    };

    // One axis's scrolling: the cursor's distance past the bitmap, the
    // milliseconds per scrolled step and those left until the next step.
    struct TRate {
        int m_offset;
        unsigned int m_interval;
        unsigned int m_remaining;
    };

    void _draw(CDC* pDC);
    void _track(const CPoint& point);
    void _setOffset(const CSize& offset);

    TPannerCtrlClient* _m_pClient;
    EMode _m_mode;
    bool _m_bSticky;
    HWND _m_hPrevFocus;
    CBitmap _m_bitmap;
    CSize _m_size;
    DWORD _m_clickTime;
    DWORD _m_lastTime;
    CPoint _m_point;
    TRate _m_horz;
    TRate _m_vert;
};

#endif  /* HOMM3_EDITOR_PANNERCTRL_H */
