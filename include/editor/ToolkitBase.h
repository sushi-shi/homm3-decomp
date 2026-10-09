// ToolkitBase.h - the toolkit and tool bar bases (ToolkitBase.cpp; Loki
// h3maped object 62). A Windows toolkit is a modeless dialog page that
// passes its commands and tool tip requests on to the top-level frame; its
// tool bar is a common-control tool bar loaded from a tool bar resource and
// its system-colour bitmap, with MFC's command UI routed to its buttons.
// Layout from the tool bar's constructor (0x50 bytes, the toolkits' new):
// the bitmap at +0x3c, the bitmap count, the resource's instance and
// handle (kept to reload the bitmap when the system colours change).
#ifndef HOMM3_EDITOR_TOOLKITBASE_H
#define HOMM3_EDITOR_TOOLKITBASE_H

#include "editor/stdafx.h"

class TToolkitBase : public CDialog {
public:
    TToolkitBase(UINT nIDTemplate, CWnd* pParent);

protected:
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

    afx_msg void OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()
};

class TToolkitBaseToolBar : public CToolBarCtrl {
public:
    TToolkitBaseToolBar(CWnd* pParent, UINT id, int numRows);

    CSize getSize() const
    {
        CRect rect;
        GetWindowRect(&rect);
        return rect.Size();
    }

protected:
    afx_msg void OnPaint();
    afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam);
    afx_msg void OnSysColorChange();
    DECLARE_MESSAGE_MAP()

private:
    bool _loadToolBar(UINT id);

    CBitmap _m_bitmap;
    int _m_numBitmaps;
    HINSTANCE _m_hInst;
    HRSRC _m_hRsrc;
};

#endif  /* HOMM3_EDITOR_TOOLKITBASE_H */
