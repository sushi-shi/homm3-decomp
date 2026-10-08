// PannerCtrl.cpp - the map window's middle-button panner (h3maped
// 0x49381d..0x4942aa; GOG only). /OPT:ICF folded the three handlers
// that end a sticky pan onto one body; the message map names it three
// times.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/Clamp.h"
#include "editor/GDIObjectSelector.h"
#include "editor/MemoryDC.h"
#include "editor/PannerCtrl.h"
#include "editor/resource.h"

DATA(0x0058d2f0) static UINT g_timerID = 1;
DATA(0x0058d2f4) static UINT g_timerElapse = 50;

DATA(0x005a2439) static bool g_bCursorsLoaded;
DATA(0x005a243c) static HCURSOR g_hPanBothCursor;
DATA(0x005a2440) static HCURSOR g_hPanHorzCursor;
DATA(0x005a2444) static HCURSOR g_hPanVertCursor;
DATA(0x005a2448) static HCURSOR g_hPanNorthCursor;
DATA(0x005a244c) static HCURSOR g_hPanSouthCursor;
DATA(0x005a2450) static HCURSOR g_hPanEastCursor;
DATA(0x005a2454) static HCURSOR g_hPanWestCursor;
DATA(0x005a2458) static HCURSOR g_hPanNortheastCursor;
DATA(0x005a245c) static HCURSOR g_hPanNorthwestCursor;
DATA(0x005a2460) static HCURSOR g_hPanSoutheastCursor;
DATA(0x005a2464) static HCURSOR g_hPanSouthwestCursor;

VA(0x00493839, 0x109)
static void loadCursors()
{
    if (g_bCursorsLoaded)
        return;
    CWinApp* pApp = AfxGetApp();
    g_hPanBothCursor = pApp->LoadCursor(IDC_PAN_BOTH);
    g_hPanHorzCursor = pApp->LoadCursor(IDC_PAN_HORZ);
    g_hPanVertCursor = pApp->LoadCursor(IDC_PAN_VERT);
    g_hPanNorthCursor = pApp->LoadCursor(IDC_PAN_NORTH);
    g_hPanSouthCursor = pApp->LoadCursor(IDC_PAN_SOUTH);
    g_hPanEastCursor = pApp->LoadCursor(IDC_PAN_EAST);
    g_hPanWestCursor = pApp->LoadCursor(IDC_PAN_WEST);
    g_hPanNortheastCursor = pApp->LoadCursor(IDC_PAN_NORTHEAST);
    g_hPanNorthwestCursor = pApp->LoadCursor(IDC_PAN_NORTHWEST);
    g_hPanSoutheastCursor = pApp->LoadCursor(IDC_PAN_SOUTHEAST);
    g_hPanSouthwestCursor = pApp->LoadCursor(IDC_PAN_SOUTHWEST);
    g_bCursorsLoaded = true;
}

VA(0x00493942, 0xdc)
TPannerCtrl::TPannerCtrl(CWnd* pParent, TPannerCtrlClient* pClient)
    : _m_pClient(pClient),
      _m_mode(eNone)
{
    loadCursors();
    DATA_COMPGEN_GUARD(0x005a2438, pannerClassNameGuard, className)
    VA_COMPGEN(0x00493a1e, 0xa, STATIC_DTOR, className)
    DATA(0x005a2434) static CString className;
    if (className.IsEmpty())
        className = AfxRegisterWndClass(CS_SAVEBITS);
    if (!CreateEx(0, className, NULL, WS_CHILD | WS_CLIPSIBLINGS, CRect(0, 0, 0, 0), pParent, 0))
        throw TRuntimeError();
}

VA_COMPGEN(0x00493a28, 0x1c, SCALAR_DELETING_DTOR, TPannerCtrl)

VA(0x00493a44, 0x49)
TPannerCtrl::~TPannerCtrl()
{
}

VA(0x00493a8d, 0x1b4)
void TPannerCtrl::relayEvent(const MSG* pMsg)
{
    if (pMsg->message != WM_MBUTTONDOWN || _m_mode != eNone)
        return;
    UINT flags = pMsg->wParam;
    DWORD time = pMsg->time;
    if ((GetKeyState(VK_CONTROL) & ~1) || flags != MK_MBUTTON)
        return;
    CWnd* pParent = GetParent();
    bool bHorz = _m_pClient->canPanHorizontally();
    bool bVert = _m_pClient->canPanVertically();
    if (!bHorz && !bVert)
        return;
    HCURSOR hCursor;
    if (!bVert) {
        _m_mode = eHorizontal;
        _m_bitmap.LoadBitmap(IDB_PANNER_HORZ);
        hCursor = g_hPanHorzCursor;
    } else if (!bHorz) {
        _m_mode = eVertical;
        _m_bitmap.LoadBitmap(IDB_PANNER_VERT);
        hCursor = g_hPanVertCursor;
    } else {
        _m_mode = eBoth;
        _m_bitmap.LoadBitmap(IDB_PANNER_BOTH);
        hCursor = g_hPanBothCursor;
    }
    if (_m_bitmap.m_hObject != NULL) {
        try {
            if (!SetTimer(g_timerID, g_timerElapse, NULL))
                throw false;
            _m_bSticky = false;
            _m_clickTime = _m_lastTime = time;
            BITMAP bitmap;
            _m_bitmap.GetBitmap(&bitmap);
            _m_size = CSize(bitmap.bmWidth, bitmap.bmHeight);
            _m_point = CPoint(_m_size.cx / 2, _m_size.cy / 2);
            MoveWindow(LOWORD(pMsg->lParam) - _m_point.x, HIWORD(pMsg->lParam) - _m_point.y,
                       _m_size.cx, _m_size.cy);
            ShowWindow(SW_SHOW);
            _m_hPrevFocus = SetFocus()->GetSafeHwnd();
            SetCapture();
            ::SetCursor(hCursor);
            _m_horz.m_offset = 0;
            _m_vert.m_offset = 0;
        } catch (...) {
            _m_bitmap.DeleteObject();
            _m_mode = eNone;
        }
    } else {
        _m_mode = eNone;
    }
}

VA(0x00493c6c, 0xe4)
void TPannerCtrl::_draw(CDC* pDC)
{
    if (_m_mode == eNone)
        return;
    CBrush brush;
    brush.CreateStockObject(WHITE_BRUSH);
    TGDIObjectSelector<CBrush> brushSelector(pDC, &brush);
    CPen pen;
    pen.CreateStockObject(BLACK_PEN);
    TGDIObjectSelector<CPen> penSelector(pDC, &pen);
    pDC->Ellipse(0, 0, _m_size.cx, _m_size.cy);
    drawTransparentBitmap(pDC, &_m_bitmap, 0, 0);
}

VA(0x00493d50, 0x18d)
void TPannerCtrl::_track(const CPoint& point)
{
    if (point == _m_point)
        return;
    _m_point = point;
    switch (_m_mode) {
    case eHorizontal: {
        int dx;
        if (point.x < 0) {
            dx = point.x;
            ::SetCursor(g_hPanWestCursor);
        } else if (point.x >= _m_size.cx) {
            dx = point.x - _m_size.cy + 1;
            ::SetCursor(g_hPanEastCursor);
        } else {
            dx = 0;
            ::SetCursor(g_hPanHorzCursor);
        }
        _setOffset(CSize(dx, 0));
        break;
    }
    case eVertical: {
        int dy;
        if (point.y < 0) {
            dy = point.y;
            ::SetCursor(g_hPanNorthCursor);
        } else if (point.y >= _m_size.cy) {
            dy = point.y - _m_size.cy + 1;
            ::SetCursor(g_hPanSouthCursor);
        } else {
            dy = 0;
            ::SetCursor(g_hPanVertCursor);
        }
        _setOffset(CSize(0, dy));
        break;
    }
    case eBoth: {
        CSize offset;
        if (point.x < 0) {
            offset.cx = point.x;
            if (point.y < 0) {
                offset.cy = point.y;
                ::SetCursor(g_hPanNorthwestCursor);
            } else if (point.y >= _m_size.cy) {
                offset.cy = point.y - _m_size.cy + 1;
                ::SetCursor(g_hPanSouthwestCursor);
            } else {
                offset.cy = 0;
                ::SetCursor(g_hPanWestCursor);
            }
        } else if (point.x >= _m_size.cx) {
            offset.cx = point.x - _m_size.cx + 1;
            if (point.y < 0) {
                offset.cy = point.y;
                ::SetCursor(g_hPanNortheastCursor);
            } else if (point.y >= _m_size.cy) {
                offset.cy = point.y - _m_size.cy + 1;
                ::SetCursor(g_hPanSoutheastCursor);
            } else {
                offset.cy = 0;
                ::SetCursor(g_hPanEastCursor);
            }
        } else {
            offset.cx = 0;
            if (point.y < 0) {
                offset.cy = point.y;
                ::SetCursor(g_hPanNorthCursor);
            } else if (point.y >= _m_size.cy) {
                offset.cy = point.y - _m_size.cy + 1;
                ::SetCursor(g_hPanSouthCursor);
            } else {
                offset.cy = 0;
                ::SetCursor(g_hPanBothCursor);
            }
        }
        _setOffset(offset);
        break;
    }
    }
}

VA(0x00493edd, 0x18e)
void TPannerCtrl::_setOffset(const CSize& offset)
{
    DWORD time = GetMessageTime();
    int dx = 0;
    int dy = 0;
    int oldOffset = _m_horz.m_offset;
    _m_horz.m_offset = offset.cx;
    if (_m_horz.m_offset != 0) {
        int distance = offset.cx < 0 ? -offset.cx : offset.cx;
        unsigned int oldInterval = _m_horz.m_interval;
        _m_horz.m_interval = clamp<unsigned int>(s_kMinInterval, s_kIntervalScale / (distance * distance), s_kMaxInterval);
        if (oldOffset == 0 || oldOffset < 0 && _m_horz.m_offset > 0 || oldOffset > 0 && _m_horz.m_offset < 0) {
            _m_horz.m_remaining = time - _m_lastTime;
        } else {
            _m_horz.m_remaining += _m_horz.m_interval;
            if (oldInterval >= _m_horz.m_remaining) {
                dx = (oldInterval - _m_horz.m_remaining) / _m_horz.m_interval + 1;
                _m_horz.m_remaining += _m_horz.m_interval * dx;
            }
            _m_horz.m_remaining -= oldInterval;
        }
    }
    oldOffset = _m_vert.m_offset;
    _m_vert.m_offset = offset.cy;
    if (_m_vert.m_offset != 0) {
        int distance = offset.cy < 0 ? -offset.cy : offset.cy;
        unsigned int oldInterval = _m_vert.m_interval;
        _m_vert.m_interval = clamp<unsigned int>(s_kMinInterval, s_kIntervalScale / (distance * distance), s_kMaxInterval);
        if (oldOffset == 0 || oldOffset < 0 && _m_vert.m_offset > 0 || oldOffset > 0 && _m_vert.m_offset < 0) {
            _m_vert.m_remaining = time - _m_lastTime;
        } else {
            _m_vert.m_remaining += _m_vert.m_interval;
            if (oldInterval >= _m_vert.m_remaining) {
                dy = (oldInterval - _m_vert.m_remaining) / _m_vert.m_interval + 1;
                _m_vert.m_remaining += _m_vert.m_interval * dy;
            }
            _m_vert.m_remaining -= oldInterval;
        }
    }
    _m_pClient->pan(_m_horz.m_offset < 0 ? -dx : dx, _m_vert.m_offset < 0 ? -dy : dy);
}

VA(0x0049406b, 0x6)
BEGIN_MESSAGE_MAP(TPannerCtrl, CWnd)
    ON_WM_PAINT()
    ON_WM_MOUSEMOVE()
    ON_WM_MBUTTONUP()
    ON_WM_TIMER()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONUP()
    ON_WM_KEYUP()
    ON_WM_MOUSEWHEEL()
    ON_WM_CAPTURECHANGED()
END_MESSAGE_MAP()

VA(0x00494071, 0x41)
void TPannerCtrl::OnPaint()
{
    CPaintDC dc(this);
    _draw(&dc);
}

VA(0x004940b2, 0xd)
void TPannerCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
    _track(point);
}

VA(0x004940bf, 0x8c)
void TPannerCtrl::OnMButtonUp(UINT nFlags, CPoint point)
{
    if (_m_mode == eNone)
        return;
    if (!_m_bSticky && GetMessageTime() - _m_clickTime <= GetDoubleClickTime()) {
        int width = GetSystemMetrics(SM_CXDOUBLECLK);
        int left = _m_size.cx / 2 - width / 2;
        if (point.x >= left && point.x < left + width) {
            int height = GetSystemMetrics(SM_CYDOUBLECLK);
            int top = _m_size.cy / 2 - height / 2;
            if (point.y >= top && point.y < top + height) {
                _m_bSticky = true;
                return;
            }
        }
    }
    ReleaseCapture();
}

VA(0x0049414b, 0xf7)
void TPannerCtrl::OnTimer(UINT nIDEvent)
{
    if (_m_mode == eNone)
        return;
    CPoint point;
    if (GetCursorPos(&point)) {
        ScreenToClient(&point);
        _track(point);
    }
    DWORD time = GetMessageTime();
    if (!_m_bSticky && !(GetKeyState(VK_MBUTTON) & ~1)) {
        ReleaseCapture();
        return;
    }
    DWORD elapsed = time - _m_lastTime;
    int dx = 0;
    int dy = 0;
    _m_lastTime = time;
    if (_m_horz.m_offset != 0) {
        DWORD rest = elapsed;
        if (rest >= _m_horz.m_remaining) {
            rest -= _m_horz.m_remaining;
            _m_horz.m_remaining = _m_horz.m_interval;
            dx = rest / _m_horz.m_interval + 1;
            rest %= _m_horz.m_interval;
        }
        _m_horz.m_remaining -= rest;
    }
    if (_m_vert.m_offset != 0) {
        DWORD rest = elapsed;
        if (rest >= _m_vert.m_remaining) {
            rest -= _m_vert.m_remaining;
            _m_vert.m_remaining = _m_vert.m_interval;
            dy = rest / _m_vert.m_interval + 1;
            rest %= _m_vert.m_interval;
        }
        _m_vert.m_remaining -= rest;
    }
    _m_pClient->pan(_m_horz.m_offset < 0 ? -dx : dx, _m_vert.m_offset < 0 ? -dy : dy);
}

VA(0x00494242, 0x15)
void TPannerCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (_m_mode != eNone && _m_bSticky)
        ReleaseCapture();
}

void TPannerCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
    if (_m_mode != eNone && _m_bSticky)
        ReleaseCapture();
}

void TPannerCtrl::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if (_m_mode != eNone && _m_bSticky)
        ReleaseCapture();
}

VA(0x00494257, 0x18)
BOOL TPannerCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (_m_mode != eNone && _m_bSticky)
        ReleaseCapture();
    return TRUE;
}

VA(0x0049426f, 0x3b)
void TPannerCtrl::OnCaptureChanged(CWnd* pWnd)
{
    ::SetFocus(_m_hPrevFocus);
    ShowWindow(SW_HIDE);
    KillTimer(g_timerID);
    _m_bitmap.DeleteObject();
    _m_mode = eNone;
    CWnd::OnCaptureChanged(pWnd);
}
