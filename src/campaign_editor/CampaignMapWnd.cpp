// CampaignMapWnd.cpp - the campaign map window (h3ccmped
// 0x419440..0x41b690). The map image and the region images come from the
// campaign map traits; the window scrolls over the map when it is smaller
// than the map and draws through an off-screen bitmap.
#include "campaign_editor/stdafx.h"

#include <string.h>

#include "va.h"
#include "campaignmap.h"
#include "resourcemanager.h"
#include "editor/GDIObjectSelector.h"
#include "editor/MemoryDC.h"
#include "campaign_editor/Campaign.h"
#include "campaign_editor/CampaignEditorText.h"
#include "campaign_editor/CampaignMapWnd.h"
#include "campaign_editor/resource.h"

// The size of the campaign map images.
VA_COMPGEN(0x00419480, 0x15, STATIC_CTOR, kMapSize)
DATA(0x004c7558) const CSize kMapSize(456, 600);

VA(0x004194a0, 0x2de)
TCampaignMapWnd::TCampaignMapWnd(CWnd* pParent, TCampaignMapWndClient* pClient)
    : _m_pClient(pClient), _m_selectedScenario(0), _m_toolScenario(0)
{
    DATA_COMPGEN_GUARD(0x004c7554, classNameGuard, className)
    VA_COMPGEN(0x00419780, 0xa, STATIC_DTOR, className)
    DATA(0x004c7550) static CString className;
    if (className.IsEmpty())
        className = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW));
    if (!CreateEx(WS_EX_CLIENTEDGE, className, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_HSCROLL | WS_VSCROLL,
                  CRect(0, 0, 0, 0), pParent, 0))
        throw TRuntimeError();
    _m_pToolTip = std::auto_ptr<CToolTipCtrl>(new CToolTipCtrl);
    if (_m_pToolTip.get() == NULL)
        throw TAllocationFailure();
    if (!_m_pToolTip->Create(this))
        throw TRuntimeError();
}

VA_COMPGEN(0x00419790, 0x1e, SCALAR_DELETING_DTOR, TCampaignMapWnd)
VA_COMPGEN(0x004197b0, 0x10b, IMPLICIT_DTOR, TCampaignMapWnd)
VA_COMPGEN(0x00419ea0, 0x78, IMPLICIT_DTOR, TRegionImages)

// Range-erase and implicit resource-conversion probes do not improve this
// body. The VC6 allocator model classifies the remaining receiver/value
// permutation as translation-unit handle state, not a local declaration knob.
VA(0x004198c0, 0x5e0)
void TCampaignMapWnd::initialUpdate()
{
    _m_regionImages.clear();
    if (_m_pMapImage.get() != NULL)
        ResourceManager::Dispose(_m_pMapImage.release());
    if (_m_pToolTip->GetToolCount() > 0)
        _m_pToolTip->DelTool(this, _m_toolScenario + 1);
    const TCampaign* pCampaign = _m_pClient->getCampaign();
    _m_toolScenario = pCampaign->getMapTraits().m_numRegions;
    _m_pMapImage = TResourcePtr<Bitmap16Bit>(
        ResourceManager::GetBitmap16(pCampaign->getMapTraits().m_imageName));
    if (_m_pMapImage.get() == NULL)
        throw TRuntimeError();
    unsigned int numScenarios = pCampaign->getMapTraits().m_numRegions;
    _m_regionImages.resize(numScenarios);
    for (unsigned int scenario = 0; scenario < numScenarios; scenario++) {
        const TCampaignMapTraits::TRegionTraits& traits =
            pCampaign->getMapTraits().m_regionTraits[scenario];
        TRegionImages& images = _m_regionImages[scenario];
        for (unsigned int color = 0; color < 8; color++) {
            images.m_selected[color] =
                TResourcePtr<Bitmap816>(ResourceManager::GetBitmap816(traits.m_selectedImageName[color]));
            if (images.m_selected[color].get() == NULL)
                throw TRuntimeError();
            images.m_enabled[color] =
                TResourcePtr<Bitmap816>(ResourceManager::GetBitmap816(traits.m_enabledImageName[color]));
            if (images.m_enabled[color].get() == NULL)
                throw TRuntimeError();
            images.m_conquered[color] =
                TResourcePtr<Bitmap816>(ResourceManager::GetBitmap816(traits.m_conqueredImageName[color]));
            if (images.m_conquered[color].get() == NULL)
                throw TRuntimeError();
        }
    }
    _m_selectedScenario = 0;
    _m_pClient->selectScenario(0);
    Invalidate();
}

VA(0x00419f20, 0xf)
void TCampaignMapWnd::update()
{
    Invalidate();
}

VA(0x00419f30, 0x34)
CSize TCampaignMapWnd::getSize() const
{
    return kMapSize + CSize(2 * ::GetSystemMetrics(SM_CXEDGE), 2 * ::GetSystemMetrics(SM_CYEDGE));
}

VA(0x00419f70, 0x36)
void TCampaignMapWnd::_removeTool()
{
    const TCampaign* pCampaign = _m_pClient->getCampaign();
    _m_pToolTip->DelTool(this, _m_toolScenario + 1);
    _m_toolScenario = pCampaign->getMapTraits().m_numRegions;
}

VA(0x00419fb0, 0xbe)
void TCampaignMapWnd::_addTool(unsigned int scenario)
{
    const TCampaign* pCampaign = _m_pClient->getCampaign();
    SCROLLINFO info;
    info.cbSize = sizeof(info);
    GetScrollInfo(SB_HORZ, &info, SIF_POS);
    int x = info.nPos;
    GetScrollInfo(SB_VERT, &info, SIF_POS);
    int y = info.nPos;
    const TCampaignMapTraits::TRegionTraits& traits =
        pCampaign->getMapTraits().m_regionTraits[scenario];
    const Bitmap816* pImage = _m_regionImages[scenario].m_selected[0].get();
    CRect rect(CPoint(traits.m_offsetX - x, traits.m_offsetY - y), CSize(pImage->GetWidth(), pImage->GetHeight()));
    _m_pToolTip->AddTool(this, LPSTR_TEXTCALLBACK, &rect, scenario + 1);
    _m_toolScenario = scenario;
}

VA(0x0041a070, 0x13d)
unsigned int TCampaignMapWnd::_getScenario(const CPoint& point)
{
    const TCampaign* pCampaign = _m_pClient->getCampaign();
    SCROLLINFO info;
    info.cbSize = sizeof(info);
    GetScrollInfo(SB_HORZ, &info, SIF_POS);
    int x = info.nPos;
    GetScrollInfo(SB_VERT, &info, SIF_POS);
    int y = info.nPos;
    unsigned int scenario;
    for (scenario = 0; scenario < pCampaign->getMapTraits().m_numRegions; scenario++) {
        const TCampaignMapTraits::TRegionTraits& traits =
            pCampaign->getMapTraits().m_regionTraits[scenario];
        int imageX = point.x - traits.m_offsetX + x;
        int imageY = point.y - traits.m_offsetY + y;
        const Bitmap816* pImage = _m_regionImages[scenario].m_selected[0].get();
        if (imageX >= 0 && imageX < pImage->GetWidth() && imageY >= 0 && imageY < pImage->GetHeight()
            && *pImage->GetMap(imageX, imageY) >= 10)
            break;
    }
    return scenario;
}

VA(0x0041a1b0, 0x6)
BEGIN_MESSAGE_MAP(TCampaignMapWnd, CWnd)
    ON_WM_WINDOWPOSCHANGED()
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_SIZE()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_COMMAND(ID_SCENARIO_PROPERTIES, OnScenarioProperties)
    ON_WM_CONTEXTMENU()
    ON_WM_RBUTTONDOWN()
    ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNeedText)
END_MESSAGE_MAP()

VA(0x0041a1c0, 0x151)
void TCampaignMapWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
    CWnd::OnWindowPosChanged(lpwndpos);
    if (!(lpwndpos->flags & SWP_NOSIZE) && lpwndpos->cx > 0 && lpwndpos->cy > 0) {
        int edgeX = ::GetSystemMetrics(SM_CXEDGE);
        int edgeY = ::GetSystemMetrics(SM_CYEDGE);
        int width = max(lpwndpos->cx - 2 * edgeX, 0);
        int height = max(lpwndpos->cy - 2 * edgeY, 0);
        if (width < kMapSize.cx || height < kMapSize.cy) {
            int scrollBarWidth = ::GetSystemMetrics(SM_CXVSCROLL);
            int scrollBarHeight = ::GetSystemMetrics(SM_CYHSCROLL);
            width = max(width - scrollBarWidth, 0);
            height = max(height - scrollBarHeight, 0);
        }
        SCROLLINFO info;
        info.cbSize = sizeof(info);
        GetScrollInfo(SB_HORZ, &info, SIF_POS);
        int x = info.nPos;
        GetScrollInfo(SB_VERT, &info, SIF_POS);
        int y = info.nPos;
        info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        info.nMin = 0;
        info.nMax = kMapSize.cx - 1;
        info.nPage = width;
        info.nPos = min(x, kMapSize.cx - width);
        SetScrollInfo(SB_HORZ, &info);
        info.nMax = kMapSize.cy - 1;
        info.nPage = height;
        info.nPos = min(y, kMapSize.cy - height);
        SetScrollInfo(SB_VERT, &info);
    }
}

VA(0x0041a320, 0x60)
void TCampaignMapWnd::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    CWnd::OnWindowPosChanging(lpwndpos);
    if (!(lpwndpos->flags & SWP_NOSIZE) && lpwndpos->cx > 0 && lpwndpos->cy > 0) {
        int edgeX = ::GetSystemMetrics(SM_CXEDGE);
        int edgeY = ::GetSystemMetrics(SM_CYEDGE);
        lpwndpos->cx = min(lpwndpos->cx, kMapSize.cx + 2 * edgeX);
        lpwndpos->cy = min(lpwndpos->cy, kMapSize.cy + 2 * edgeY);
    }
}

VA(0x0041a380, 0x64)
void TCampaignMapWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    if (cx > 0 && cy > 0)
        _m_bitmap.create(cx, cy);
}

VA(0x0041a3f0, 0x11c)
void TCampaignMapWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SCROLLINFO info;
    info.cbSize = sizeof(info);
    GetScrollInfo(SB_HORZ, &info, SIF_PAGE | SIF_POS);
    int pos;
    switch (nSBCode) {
    case SB_LINELEFT:
        pos = max(info.nPos - 10, 0);
        break;
    case SB_LINERIGHT:
        pos = min(info.nPos + 10, kMapSize.cx - info.nPage);
        break;
    case SB_PAGELEFT:
        pos = max(info.nPos - info.nPage, 0);
        break;
    case SB_PAGERIGHT:
        pos = min(info.nPos + info.nPage, kMapSize.cx - info.nPage);
        break;
    case SB_LEFT:
        pos = 0;
        break;
    case SB_RIGHT:
        pos = kMapSize.cx - info.nPage;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        pos = nPos;
        break;
    default:
        return;
    }
    if (pos == info.nPos)
        return;
    const TCampaign* pCampaign = _m_pClient->getCampaign();
    if (pCampaign != NULL && _m_toolScenario < pCampaign->getMapTraits().m_numRegions)
        _removeTool();
    info.fMask = SIF_POS;
    info.nPos = pos;
    SetScrollInfo(SB_HORZ, &info);
    Invalidate();
}

VA(0x0041a510, 0x11c)
void TCampaignMapWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SCROLLINFO info;
    info.cbSize = sizeof(info);
    GetScrollInfo(SB_VERT, &info, SIF_PAGE | SIF_POS);
    int pos;
    switch (nSBCode) {
    case SB_LINEUP:
        pos = max(info.nPos - 10, 0);
        break;
    case SB_LINEDOWN:
        pos = min(info.nPos + 10, kMapSize.cy - info.nPage);
        break;
    case SB_PAGEUP:
        pos = max(info.nPos - info.nPage, 0);
        break;
    case SB_PAGEDOWN:
        pos = min(info.nPos + info.nPage, kMapSize.cy - info.nPage);
        break;
    case SB_TOP:
        pos = 0;
        break;
    case SB_BOTTOM:
        pos = kMapSize.cy - info.nPage;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        pos = nPos;
        break;
    default:
        return;
    }
    if (pos == info.nPos)
        return;
    const TCampaign* pCampaign = _m_pClient->getCampaign();
    if (pCampaign != NULL && _m_toolScenario < pCampaign->getMapTraits().m_numRegions)
        _removeTool();
    info.fMask = SIF_POS;
    info.nPos = pos;
    SetScrollInfo(SB_VERT, &info);
    Invalidate();
}

// TMemoryDC's CDC::CreateCompatibleDC call reproduces retail's zero-register
// comparison (99.7845 -> 99.9965%). All 23 blocks and 27 calls agree. The
// remaining difference is the entry's handler pointer: retail push 0x4821be
// has no relocation paired to the compiler's local EH label at +3.
VA(0x0041a630, 0x399)
void TCampaignMapWnd::OnPaint()
{
    CPaintDC dc(this);
    const TCampaign* pCampaign = _m_pClient->getCampaign();
    if (pCampaign == NULL)
        return;
    SCROLLINFO info;
    info.cbSize = sizeof(info);
    GetScrollInfo(SB_HORZ, &info, SIF_POS);
    int x = info.nPos;
    GetScrollInfo(SB_VERT, &info, SIF_POS);
    int y = info.nPos;
    CRect clipRect;
    dc.GetClipBox(&clipRect);
    _m_pMapImage->Draw(clipRect.left + x, clipRect.top + y, clipRect.Width(), clipRect.Height(), _m_bitmap.getPixels(),
                       clipRect.left, clipRect.top, _m_bitmap.getWidth(), _m_bitmap.getHeight(), _m_bitmap.getPitch(),
                       false);
    for (unsigned int scenario = 0; scenario < pCampaign->getMapTraits().m_numRegions;
         scenario++) {
        const TCampaignMapTraits::TRegionTraits& traits =
            pCampaign->getMapTraits().m_regionTraits[scenario];
        int color = pCampaign->getScenario(scenario).getRegionColor();
        const Bitmap816* pImage;
        if (scenario == _m_selectedScenario)
            pImage = _m_regionImages[scenario].m_selected[color].get();
        else if (pCampaign->getScenario(scenario).getMap() != NULL)
            pImage = _m_regionImages[scenario].m_enabled[color].get();
        else
            pImage = _m_regionImages[scenario].m_conquered[color].get();
        CRect imageRect(CPoint(traits.m_offsetX - x, traits.m_offsetY - y),
                        CSize(pImage->GetWidth(), pImage->GetHeight()));
        CRect drawRect;
        if (drawRect.IntersectRect(&clipRect, &imageRect))
            pImage->Draw(drawRect.left - traits.m_offsetX + x, drawRect.top - traits.m_offsetY + y, drawRect.Width(),
                         drawRect.Height(), _m_bitmap.getPixels(), drawRect.left, drawRect.top, _m_bitmap.getWidth(),
                         _m_bitmap.getHeight(), _m_bitmap.getPitch(), true);
    }
    TMemoryDC memoryDC(&dc);
    TGDIObjectSelector<CBitmap> selector(&memoryDC, &_m_bitmap);
    dc.BitBlt(clipRect.left, clipRect.top, clipRect.Width(), clipRect.Height(), &memoryDC, clipRect.left, clipRect.top,
              SRCCOPY);
}

VA(0x0041a9d0, 0x5c)
void TCampaignMapWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    const TCampaign* pCampaign = _m_pClient->getCampaign();
    if (pCampaign == NULL)
        return;
    unsigned int scenario = _getScenario(point);
    if (scenario < pCampaign->getMapTraits().m_numRegions
        && scenario != _m_selectedScenario) {
        _m_selectedScenario = scenario;
        _m_pClient->selectScenario(scenario);
        Invalidate();
    }
}

VA(0x0041aa30, 0x8)
void TCampaignMapWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    OnScenarioProperties();
}

VA(0x0041aa40, 0x149)
LRESULT TCampaignMapWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    const TCampaign* pCampaign = _m_pClient->getCampaign();
    if (pCampaign != NULL && message == WM_MOUSEMOVE) {
        CPoint point(lParam);
        unsigned int scenario = _getScenario(point);
        if (scenario < pCampaign->getMapTraits().m_numRegions) {
            if (scenario != _m_toolScenario) {
                if (_m_toolScenario < pCampaign->getMapTraits().m_numRegions)
                    _removeTool();
                _addTool(scenario);
            }
        } else if (_m_toolScenario < pCampaign->getMapTraits().m_numRegions)
            _removeTool();
    }
    switch (message) {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP: {
        MSG msg;
        msg.hwnd = m_hWnd;
        msg.message = message;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.time = ::GetMessageTime();
        msg.pt = CPoint(::GetMessagePos());
        _m_pToolTip->RelayEvent(&msg);
        break;
    }
    }
    return CWnd::WindowProc(message, wParam, lParam);
}

VA(0x0041ab90, 0x163)
BOOL TCampaignMapWnd::OnToolTipNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
    DATA(0x004c7488) static char text[200];
    if (pNMHDR->hwndFrom != _m_pToolTip->m_hWnd)
        return FALSE;
    const TCampaign* pCampaign = _m_pClient->getCampaign();
    const TCampaignMapTraits::TRegionTraits& traits =
        pCampaign->getMapTraits().m_regionTraits[_m_toolScenario];
    const TCampaignScenarioMap* pMap = pCampaign->getScenario(_m_toolScenario).getMap();
    if (pMap != NULL)
        strncpy(text, (pMap->m_name + " - " + traits.m_name).c_str(), sizeof(text));
    else
        strncpy(text, traits.m_name, sizeof(text));
    text[sizeof(text) - 1] = '\0';
    TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
    pTTT->hinst = NULL;
    pTTT->lpszText = text;
    return TRUE;
}

VA(0x0041ad00, 0x1c)
void TCampaignMapWnd::OnScenarioProperties()
{
    if (_m_pClient->getCampaign() != NULL)
        _m_pClient->editScenario(_m_selectedScenario);
}

VA(0x0041ad20, 0x1e7)
void TCampaignMapWnd::OnContextMenu(CWnd* pWnd, CPoint point)
{
    CMenu popupMenu;
    if (popupMenu.CreatePopupMenu()) {
        CMenu menu;
        if (menu.LoadMenu(IDR_SCENARIO_MENU)) {
            unsigned int numItems = menu.GetMenuItemCount();
            for (unsigned int item = 0; item < numItems; item++) {
                UINT id = menu.GetMenuItemID(item);
                UINT state = menu.GetMenuState(item, MF_BYPOSITION);
                CString text;
                menu.GetMenuString(item, text, MF_BYPOSITION);
                popupMenu.AppendMenu(state, id, text);
            }
            menu.DestroyMenu();
            unsigned int index = getFirstMenuItem(&popupMenu);
            setMenuItemText(&popupMenu, index, kPropertiesStr);
            popupMenu.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, GetParentFrame());
        }
    }
}

VA(0x0041af70, 0x17)
void TCampaignMapWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
    OnLButtonDown(nFlags, point);
}
