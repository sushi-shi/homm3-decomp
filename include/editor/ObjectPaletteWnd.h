// ObjectPaletteWnd.h - the object palette (ObjectPaletteWnd.cpp; Loki
// h3maped object 67). Three columns of object frames for the current
// slot's object types in a vertically scrolled child window; each visible
// frame is a tool of the window's tool tip, which names its object. The
// client decides which object types can be placed and takes the one the
// user grabs.
//
// Layout from the constructor (0x200 bytes): the client, the tool tip,
// the visible tool range [first, end) and per-tool enabled flags, the back
// buffer, the current slot, one _TSlotInfo per slot (its saved scroll
// position and the object types the constructor collects from
// kObjectTypeTable) and the player. Names follow Loki's; Windows keeps the
// scroll position in the window's scroll bar and draws with GDI.
#ifndef HOMM3_EDITOR_OBJECTPALETTEWND_H
#define HOMM3_EDITOR_OBJECTPALETTEWND_H

#include "editor/stdafx.h"

#include <vector>

#include "va.h"
#include "objecttype.h"
#include "editor/ObjectTypeTable.h"
#include "editor/Array.h"
#include "editor/DIBSection.h"
#include "editor/Player.h"

class TObjectPaletteWndClient;

// The palette's tool tip (RTTI TObjectPaletteWndToolTip): it stays active
// while a modal dialog disables its owner.
class TObjectPaletteWndToolTip : public CToolTipCtrl {
protected:
    afx_msg LRESULT OnDisableModal(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()
};

class TObjectPaletteWnd : public CWnd {
public:
    TObjectPaletteWnd(CWnd* pParent, TObjectPaletteWndClient* pClient);
    virtual ~TObjectPaletteWnd();

    void setSlot(TObjectSlot newSlot);
    void setPlayer(TPlayer newPlayer);
    CSize getMinSize() const;

    virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
    afx_msg void OnPaint();
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg BOOL OnToolTipNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

private:
    // A slot's saved scroll position (pixels) and its object types.
    struct _TSlotInfo {
        _TSlotInfo() : m_scrollPos(0) {}

        unsigned int m_scrollPos;
        vector<const TObjectType*> m_objTypes;
    };

    void _setupTools(int height);
    void _removeAllTools();
    VA(0x0048c184, 0x2e)
    int _computeRows() const
    {
        return (_m_aSlotInfo[_m_slot].m_objTypes.size() + 2) / 3;
    }

    static const CSize s_kObjFrameSize;

    TObjectPaletteWndClient* _m_pClient;
    TObjectPaletteWndToolTip _m_toolTip;
    unsigned int _m_firstTool;
    unsigned int _m_endTool;
    vector<bool> _m_abToolEnabled;
    T16bppDIBSection _m_backBuffer;
    TObjectSlot _m_slot;
    TArray<_TSlotInfo, kNumObjectSlots> _m_aSlotInfo;
    TPlayer _m_player;
};

class TObjectPaletteWndClient {
public:
    virtual bool onPaletteCanCreateObject(TObjectPaletteWnd* pPaletteWnd, const TObjectType& objType) = 0;
    virtual void onPaletteGrabObject(TObjectPaletteWnd* pPaletteWnd, const TObjectType& objType) = 0;
};

#endif  /* HOMM3_EDITOR_OBJECTPALETTEWND_H */
