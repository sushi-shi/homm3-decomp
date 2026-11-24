// MainFrame.h - the editor's main window (MainFrame.cpp; GOG only, Loki's
// GTK+ port has no frame): an AppWizard MFC frame with the tool bar, the
// mode bar and a four-pane status bar. It is the map view's status bar
// interface (RTTI ??_7TMainFrame@@6BTStatusUI@TMapView@@@ at +0xbc), keeps
// the frame's placement and the bars' layout in the registry, and gives
// every command its status bar prompt and tool tip from "editrcmd.txt".
// Layout from the constructor (0x27c bytes, DECLARE_DYNCREATE): the bars
// at +0xc0, +0x13c and +0x1d8, then the object name pane's width and the
// first activation flag. The member names are not recorded.
#ifndef HOMM3_EDITOR_MAINFRAME_H
#define HOMM3_EDITOR_MAINFRAME_H

#include "editor/stdafx.h"

#include "editor/MapView.h"

// RTTI: the interface base is private (attributes 9).
class TMainFrame : public CFrameWnd, private TMapView::TStatusUI {
    DECLARE_DYNCREATE(TMainFrame)

protected:
    TMainFrame();

public:
    virtual ~TMainFrame();
    virtual BOOL DestroyWindow();
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void GetMessageString(UINT nID, CString& rMessage) const;
    virtual void ActivateFrame(int nCmdShow = -1);

protected:
    virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDisplayChange(WPARAM wParam, LPARAM lParam);
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg BOOL OnQueryOpen();
    afx_msg BOOL OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()

private:
    virtual void showObjectName();
    virtual void hideObjectName();
    virtual void setObjectName(const char* name);
    virtual void setCurPlayer(TPlayer player);
    virtual void setMapSize(unsigned int width, unsigned int height, bool bTwoLayer);

    CStatusBar _m_statusBar;
    CToolBar _m_toolBar;
    CToolBar _m_modeBar;
    int _m_objectNamePaneWidth;
    bool _m_bFirstActivation;
};

#endif  /* HOMM3_EDITOR_MAINFRAME_H */
