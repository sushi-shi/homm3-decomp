// ToolkitBase.cpp - the toolkit and tool bar bases (h3maped
// 0x4c097e..0x4c0f14; Loki h3maped object 62). The tool bar loads its
// buttons as MFC's CToolBar::LoadToolBar does and updates them through its
// own command UI while the application idles.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/ToolkitBase.h"

// afxpriv.h's idle command-UI message: afxpriv.h includes ATL's
// atlconv.h, which the toolchain does not carry.
const UINT WM_IDLEUPDATECMDUI = 0x0363;

namespace {

// A tool bar resource (MFC's CToolBarData): the bitmap size and the
// buttons' command ids, 0 for a separator.
struct TToolBarData {
    WORD m_version;
    WORD m_width;
    WORD m_height;
    WORD m_itemCount;

    WORD* items() { return (WORD*)(this + 1); }
};

// The command UI of a tool bar button (MFC's CToolCmdUI over the common
// control).
class TToolBarCmdUI : public CCmdUI {
public:
    virtual void Enable(BOOL bOn = TRUE);
    virtual void SetCheck(int nCheck = 1);
    virtual void SetRadio(BOOL bOn = TRUE);
    virtual void SetText(LPCTSTR lpszText) {}
};

}  // namespace

VA(0x004c099a, 0x1c)
TToolkitBase::TToolkitBase(UINT nIDTemplate, CWnd* pParent) : CDialog(nIDTemplate, pParent)
{
}

VA(0x004c09b6, 0x6)
BEGIN_MESSAGE_MAP(TToolkitBase, CDialog)
    ON_NOTIFY_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
    ON_NOTIFY_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()

// Commands go to the top-level frame; a control that sent one updates its
// command UI at once.
VA(0x004c09bc, 0x59)
BOOL TToolkitBase::OnCommand(WPARAM wParam, LPARAM lParam)
{
    CFrameWnd* pFrame = GetTopLevelFrame();
    if (pFrame == NULL)
        return CDialog::OnCommand(wParam, lParam);
    BOOL bResult = pFrame->SendMessage(WM_COMMAND, wParam, lParam) != 0;
    if (lParam != 0)
        ::SendMessage((HWND)lParam, WM_IDLEUPDATECMDUI, TRUE, 0);
    return bResult;
}

VA(0x004c0a15, 0x2e)
void TToolkitBase::OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
    CFrameWnd* pFrame = GetTopLevelFrame();
    if (pFrame != NULL)
        *pResult = pFrame->SendMessage(WM_NOTIFY, id, (LPARAM)pNMHDR);
    else
        *pResult = 0;
}

namespace {

VA(0x004c0a43, 0x28)
void TToolBarCmdUI::Enable(BOOL bOn)
{
    ((CToolBarCtrl*)m_pOther)->EnableButton(m_nID, bOn);
    m_bEnableChanged = TRUE;
}

VA(0x004c0a6b, 0x4b)
void TToolBarCmdUI::SetCheck(int nCheck)
{
    CToolBarCtrl* pToolBar = (CToolBarCtrl*)m_pOther;
    UINT state = pToolBar->GetState(m_nID) & ~(TBSTATE_CHECKED | TBSTATE_INDETERMINATE);
    if (nCheck == 2)
        state |= TBSTATE_INDETERMINATE;
    else if (nCheck == 1)
        state |= TBSTATE_CHECKED;
    pToolBar->SetState(m_nID, state);
}

VA(0x004c0ab6, 0x1d)
void TToolBarCmdUI::SetRadio(BOOL bOn)
{
    ((CToolBarCtrl*)m_pOther)->CheckButton(m_nID, bOn);
}

}  // namespace

VA(0x004c0ad3, 0xca)
TToolkitBaseToolBar::TToolkitBaseToolBar(CWnd* pParent, UINT id, int numRows)
    : _m_hInst(NULL),
      _m_hRsrc(NULL)
{
    if (!Create(WS_CHILD | WS_VISIBLE | CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN | TBSTYLE_TOOLTIPS
                    | TBSTYLE_WRAPABLE,
                CRect(0, 0, 0, 0), pParent, 0)
        || !_loadToolBar(id))
        throw TRuntimeError();
    CRect rect;
    SetRows(numRows, TRUE, &rect);
    MoveWindow(rect.left, rect.top, rect.Width(), rect.Height(), FALSE);
}

VA_COMPGEN(0x004c0b9d, 0x1c, SCALAR_DELETING_DTOR, TToolkitBaseToolBar)
VA_COMPGEN(0x004c0bb9, 0x43, IMPLICIT_DTOR, TToolkitBaseToolBar)

VA(0x004c0bfc, 0x205)
bool TToolkitBaseToolBar::_loadToolBar(UINT id)
{
    if (GetButtonCount() > 0)
        return false;
    HINSTANCE hInst = AfxFindResourceHandle(MAKEINTRESOURCE(id), RT_TOOLBAR);
    HRSRC hRsrc = ::FindResource(hInst, MAKEINTRESOURCE(id), RT_TOOLBAR);
    if (hRsrc == NULL)
        return false;
    HGLOBAL hGlobal = ::LoadResource(hInst, hRsrc);
    if (hGlobal == NULL)
        return false;
    try {
        TToolBarData* pData = (TToolBarData*)::LockResource(hGlobal);
        if (pData == NULL)
            throw false;
        try {
            TBBUTTON button;
            memset(&button, 0, sizeof(button));
            _m_numBitmaps = 0;
            for (int i = 0; i < pData->m_itemCount; i++) {
                button.fsState = TBSTATE_ENABLED;
                button.idCommand = pData->items()[i];
                if (button.idCommand == 0) {
                    button.fsStyle = TBSTYLE_SEP;
                    button.iBitmap = 8;
                } else {
                    button.fsStyle = TBSTYLE_BUTTON;
                    button.iBitmap = _m_numBitmaps++;
                }
                AddButtons(1, &button);
            }
            SetBitmapSize(CSize(pData->m_width, pData->m_height));
            SetButtonSize(CSize(pData->m_width + 7, pData->m_height + 7));
        } catch (...) {
            UnlockResource(hGlobal);
            throw;
        }
        UnlockResource(hGlobal);
    } catch (bool) {
        return false;
    } catch (...) {
        throw;
    }
    hInst = AfxFindResourceHandle(MAKEINTRESOURCE(id), RT_BITMAP);
    hRsrc = ::FindResource(hInst, MAKEINTRESOURCE(id), RT_BITMAP);
    if (hRsrc == NULL)
        return false;
    HBITMAP hBitmap = AfxLoadSysColorBitmap(hInst, hRsrc);
    if (hBitmap == NULL)
        return false;
    try {
        if (!_m_bitmap.Attach(hBitmap))
            throw false;
        try {
            if (AddBitmap(_m_numBitmaps, &_m_bitmap) == -1)
                throw false;
        } catch (...) {
            _m_bitmap.Detach();
            throw;
        }
    } catch (bool) {
        ::DeleteObject(hBitmap);
        return false;
    }
    _m_hInst = hInst;
    _m_hRsrc = hRsrc;
    return true;
}

VA(0x004c0e01, 0x6)
BEGIN_MESSAGE_MAP(TToolkitBaseToolBar, CToolBarCtrl)
    ON_WM_PAINT()
    ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
    ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

VA(0x004c0e07, 0x15)
void TToolkitBaseToolBar::OnPaint()
{
    OnIdleUpdateCmdUI(TRUE, 0);
    CToolBarCtrl::OnPaint();
}

VA(0x004c0e1c, 0x84)
LRESULT TToolkitBaseToolBar::OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam)
{
    CFrameWnd* pTarget = GetTopLevelFrame();
    if (pTarget != NULL) {
        TToolBarCmdUI state;
        state.m_pOther = this;
        state.m_nIndexMax = GetButtonCount();
        for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; state.m_nIndex++) {
            TBBUTTON button;
            GetButton(state.m_nIndex, &button);
            if (!(button.fsStyle & TBSTYLE_SEP)) {
                state.m_nID = button.idCommand;
                state.DoUpdate(pTarget, (BOOL)wParam);
            }
        }
    }
    return 0;
}

// Reloads the buttons' bitmap in the new system colours.
VA(0x004c0ea0, 0x74)
void TToolkitBaseToolBar::OnSysColorChange()
{
    if (_m_hInst != NULL && _m_hRsrc != NULL) {
        HBITMAP hBitmap = AfxLoadSysColorBitmap(_m_hInst, _m_hRsrc);
        if (hBitmap != NULL) {
            TBREPLACEBITMAP replace;
            replace.hInstOld = NULL;
            replace.nIDOld = (UINT)_m_bitmap.m_hObject;
            replace.hInstNew = NULL;
            replace.nIDNew = (UINT)hBitmap;
            replace.nButtons = _m_numBitmaps;
            if (DefWindowProc(TB_REPLACEBITMAP, 0, (LPARAM)&replace)) {
                ::DeleteObject(_m_bitmap.Detach());
                _m_bitmap.Attach(hBitmap);
            }
        }
    }
}
