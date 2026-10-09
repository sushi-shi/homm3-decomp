// ObjectPaletteWnd.cpp - the object palette (h3maped 0x48bbbe..0x48d4da;
// Loki h3maped object 67). Three columns of object frames for the current
// slot's object types, scrolled by the window's vertical scroll bar. Each
// frame shows the object's shadow and sprite (with the player's flag for
// heroes) drawn into a DIB section and stretched into the frame; frames of
// objects the client cannot place are drawn on the button face. Each
// visible frame is a tool tip tool that names its object, and the context
// menu key shows the object's help. Without the Shadow of Death data the
// palette leaves out the Conflux hero classes and town.
#include "editor/stdafx.h"

#include <stdio.h>
#include <algorithm>
#include <functional>
#include <string>

#include "va.h"
#include "artifact.h"
#include "armygrp.h"
#include "csprite.h"
#include "gamecontext.h"
#include "objnames.h"
#include "retailobjecttype.h"
#include "videogamestate.h"
#include "editor/AfxPrivMessages.h"
#include "editor/Colors.h"
#include "editor/DCAttributeSelector.h"
#include "editor/GameResource.h"
#include "editor/GDIObjectSelector.h"
#include "editor/Generator.h"
#include "editor/Hero.h"
#include "editor/MemoryDC.h"
#include "editor/ObjectHelp.h"
#include "editor/ObjectPaletteWnd.h"
#include "editor/ObjectSpecializations.h"
#include "editor/ObjectSprites.h"
#include "editor/ObjectTypeTable.h"
#include "editor/Town.h"
#include "editor/resource.h"

namespace {

DATA(0x005a20e0) HCURSOR khArrowCursor = ::LoadCursor(NULL, IDC_ARROW);
DATA(0x005a210c) HCURSOR hOpenHandCursor = NULL;

} // namespace

VA(0x0048bda5, 0x6)
BEGIN_MESSAGE_MAP(TObjectPaletteWndToolTip, CToolTipCtrl)
    ON_MESSAGE(WM_DISABLEMODAL, OnDisableModal)
END_MESSAGE_MAP()

LRESULT TObjectPaletteWndToolTip::OnDisableModal(WPARAM wParam, LPARAM lParam)
{
    return 0;
}

DATA(0x005a20d8) const CSize TObjectPaletteWnd::s_kObjFrameSize(66, 66);

VA(0x0048bdb9, 0x23a)
TObjectPaletteWnd::TObjectPaletteWnd(CWnd* pParent, TObjectPaletteWndClient* pClient)
    : _m_pClient(pClient),
      _m_firstTool(0),
      _m_endTool(0),
      _m_slot(eSlotDirt),
      _m_player(ePlayerNone)
{
    if (hOpenHandCursor == NULL)
        hOpenHandCursor = AfxGetApp()->LoadCursor(IDC_OPEN_HAND);
    DATA_COMPGEN_GUARD(0x005a20d4, objectPaletteWndClassNameGuard, className)
    VA_COMPGEN(0x0048bff3, 0xa, STATIC_DTOR, className)
    DATA(0x005a20d0) static CString className;
    if (className.IsEmpty())
        className = AfxRegisterWndClass(CS_DBLCLKS, khArrowCursor);
    if (!CreateEx(WS_EX_CLIENTEDGE, className, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL,
                  CRect(0, 0, 0, 0), pParent, 0))
        throw TRuntimeError();
    if (!_m_toolTip.Create(this))
        throw TRuntimeError();

    for (int heroClass = 0; heroClass <= kNumHeroClasses; heroClass++) {
        if (g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH
            || THero::s_akClassTraits[heroClass].m_objType.getExtra() < classPlanesWalker)
            _m_aSlotInfo[eSlotHeroes].m_objTypes.push_back(&THero::s_akClassTraits[heroClass].m_objType);
    }
    for (unsigned int slot = 0; slot < kNumObjectSlots; slot++) {
        binder2nd<TObjectTypeInSlotPred> pred = bind2nd(TObjectTypeInSlotPred(), (TObjectSlot)slot);
        for (vector<TObjectType>::const_iterator it = kObjectTypeTable.m_objectTypes.begin();
             (it = find_if(it, kObjectTypeTable.m_objectTypes.end(), pred)) != kObjectTypeTable.m_objectTypes.end();
             it++) {
            if (g_videoGameState == VIDEO_GAME_STATE_FORCED_BINK_HIGH || it->getType() != TOWN
                || it->getExtra() < TOWN_CONFLUX)
                _m_aSlotInfo[slot].m_objTypes.push_back(&*it);
        }
    }
}

VA_COMPGEN(0x0048bffd, 0x1c, SCALAR_DELETING_DTOR, TObjectPaletteWnd)
VA_COMPGEN(0x0048c019, 0x5, IMPLICIT_DTOR, TObjectPaletteWndToolTip)
VA_COMPGEN(0x0048c01e, 0x1c, SCALAR_DELETING_DTOR, TObjectPaletteWndToolTip)

VA(0x0048c03a, 0x6b)
TObjectPaletteWnd::~TObjectPaletteWnd()
{
}

VA(0x0048c0a5, 0xdf)
void TObjectPaletteWnd::setSlot(TObjectSlot newSlot)
{
    if (newSlot == _m_slot)
        return;
    _removeAllTools();
    _m_slot = newSlot;
    CRect clientRect;
    GetClientRect(&clientRect);
    SCROLLINFO info;
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    info.nMin = 0;
    info.nMax = max(_computeRows() * s_kObjFrameSize.cy - 1, 0);
    info.nPage = clientRect.Height();
    info.nPos = min(_m_aSlotInfo[_m_slot].m_scrollPos, info.nMax - info.nPage + 1);
    SetScrollInfo(SB_VERT, &info);
    _m_aSlotInfo[_m_slot].m_scrollPos = info.nPos;
    _setupTools(clientRect.Height());
    Invalidate(FALSE);
    UpdateWindow();
}

VA(0x0048c1b2, 0x73)
void TObjectPaletteWnd::setPlayer(TPlayer newPlayer)
{
    if (_m_player == newPlayer)
        return;
    _m_player = newPlayer;
    for (unsigned int tool = _m_firstTool; tool < _m_endTool; tool++) {
        bool bEnabled = _m_pClient->onPaletteCanCreateObject(this, *_m_aSlotInfo[_m_slot].m_objTypes[tool]);
        if (bEnabled != _m_abToolEnabled[tool - _m_firstTool])
            _m_abToolEnabled[tool - _m_firstTool] = bEnabled;
    }
    Invalidate(FALSE);
    UpdateWindow();
}

VA(0x0048c225, 0x3c)
CSize TObjectPaletteWnd::getMinSize() const
{
    return CSize(s_kObjFrameSize.cx * 3 + ::GetSystemMetrics(SM_CXEDGE) * 2 + ::GetSystemMetrics(SM_CXVSCROLL),
                 s_kObjFrameSize.cy + ::GetSystemMetrics(SM_CYEDGE) * 2);
}

VA(0x0048c261, 0x226)
void TObjectPaletteWnd::_setupTools(int height)
{
    unsigned int scrollPos = _m_aSlotInfo[_m_slot].m_scrollPos;
    unsigned int firstRow = scrollPos / s_kObjFrameSize.cy;
    unsigned int numRows = _computeRows();
    unsigned int lastRow = (scrollPos + height + s_kObjFrameSize.cy - 1) / s_kObjFrameSize.cy;
    if (firstRow >= numRows)
        return;
    CRect toolRect;
    unsigned int tool = _m_firstTool = firstRow * 3;
    toolRect.left = toolRect.top = 0;
    toolRect.bottom = s_kObjFrameSize.cy - scrollPos % s_kObjFrameSize.cy;
    unsigned int col;
    for (col = 0; col < 3 && tool < _m_aSlotInfo[_m_slot].m_objTypes.size(); col++) {
        toolRect.right = toolRect.left + s_kObjFrameSize.cx;
        _m_toolTip.AddTool(this, LPSTR_TEXTCALLBACK, &toolRect, tool + 1);
        tool++;
        toolRect.left += s_kObjFrameSize.cx;
    }
    unsigned int row;
    for (row = firstRow + 1; row < lastRow - 1 && row < numRows - 1; row++) {
        toolRect.top = toolRect.bottom;
        toolRect.bottom += s_kObjFrameSize.cy;
        toolRect.left = 0;
        for (col = 0; col < 3; col++) {
            toolRect.right = toolRect.left + s_kObjFrameSize.cx;
            _m_toolTip.AddTool(this, LPSTR_TEXTCALLBACK, &toolRect, tool + 1);
            tool++;
            toolRect.left += s_kObjFrameSize.cx;
        }
    }
    toolRect.top = toolRect.bottom;
    toolRect.left = 0;
    if (row == lastRow - 1)
        toolRect.bottom = height;
    else
        toolRect.bottom += s_kObjFrameSize.cy;
    for (col = 0; col < 3 && tool < _m_aSlotInfo[_m_slot].m_objTypes.size(); col++) {
        toolRect.right = toolRect.left + s_kObjFrameSize.cx;
        _m_toolTip.AddTool(this, LPSTR_TEXTCALLBACK, &toolRect, tool + 1);
        tool++;
        toolRect.left += s_kObjFrameSize.cx;
    }
    _m_endTool = tool;
    _m_abToolEnabled.resize(_m_endTool - _m_firstTool, false);
    for (tool = _m_firstTool; tool < _m_endTool; tool++)
        _m_abToolEnabled[tool - _m_firstTool] =
            _m_pClient->onPaletteCanCreateObject(this, *_m_aSlotInfo[_m_slot].m_objTypes[tool]);
}

VA(0x0048c487, 0x5e)
void TObjectPaletteWnd::_removeAllTools()
{
    for (; _m_firstTool < _m_endTool; _m_firstTool++)
        _m_toolTip.DelTool(this, _m_firstTool + 1);
    _m_endTool = 0;
    _m_firstTool = 0;
    _m_abToolEnabled.clear();
}

VA(0x0048c4e5, 0x6)
BEGIN_MESSAGE_MAP(TObjectPaletteWnd, CWnd)
    ON_WM_SIZE()
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_PAINT()
    ON_WM_VSCROLL()
    ON_WM_LBUTTONDOWN()
    ON_WM_CONTEXTMENU()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNeedText)
    ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
END_MESSAGE_MAP()

VA(0x0048c4eb, 0xb0)
void TObjectPaletteWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    if (cx == 0 || cy == 0)
        return;
    _removeAllTools();
    _m_backBuffer.create(cx, cy);
    SCROLLINFO info;
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    info.nMin = 0;
    info.nMax = _computeRows() * s_kObjFrameSize.cy - 1;
    info.nPage = cy;
    info.nPos = min(_m_aSlotInfo[_m_slot].m_scrollPos, info.nMax - info.nPage + 1);
    SetScrollInfo(SB_VERT, &info);
    _m_aSlotInfo[_m_slot].m_scrollPos = info.nPos;
    _setupTools(cy);
}

// The palette is exactly three frames wide and at least one frame high.
VA(0x0048c59b, 0x4f)
void TObjectPaletteWnd::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
    lpwndpos->cx = s_kObjFrameSize.cx * 3 + ::GetSystemMetrics(SM_CXEDGE) * 2 + ::GetSystemMetrics(SM_CXVSCROLL);
    int minHeight = s_kObjFrameSize.cy + ::GetSystemMetrics(SM_CYEDGE) * 2;
    lpwndpos->cy = max(lpwndpos->cy, minHeight);
    CWnd::OnWindowPosChanging(lpwndpos);
}

VA(0x0048c5ea, 0x6d3)
void TObjectPaletteWnd::OnPaint()
{
    CPaintDC dc(this);
    TMemoryDC memDC(&dc);
    TGDIObjectSelector<CBitmap> bitmapSelector(&memDC, &_m_backBuffer);
    CRect rect;
    dc.GetClipBox(&rect);
    memDC.SelectClipRgn(NULL);
    memDC.IntersectClipRect(&rect);
    CPen pen(PS_SOLID, 1, ::GetSysColor(COLOR_WINDOWFRAME));
    TGDIObjectSelector<CPen> penSelector(&memDC, &pen);
    TDCAttributeSelector<int, &CDC::SetStretchBltMode> stretchBltModeSelector(&memDC, COLORONCOLOR);
    unsigned int scrollPos = GetScrollPos(SB_VERT);
    unsigned int firstRow = (rect.top + scrollPos) / s_kObjFrameSize.cy;
    unsigned int lastRow = (rect.bottom + scrollPos + s_kObjFrameSize.cy - 1) / s_kObjFrameSize.cy;
    CDC imageDC;
    imageDC.Attach(::CreateCompatibleDC(dc.GetSafeHdc()));
    T16bppDIBSection image(256, 192);
    TGDIObjectSelector<CBitmap> imageSelector(&imageDC, &image);
    CBrush enabledBrush(::GetSysColor(COLOR_WINDOW));
    CBrush disabledBrush(::GetSysColor(COLOR_BTNFACE));
    TColor flagColor = akPlayerColor[_m_player + 1];

    for (unsigned int row = firstRow; row < lastRow; row++) {
        unsigned int tool = row * 3;
        CRect toolRect;
        toolRect.top = row * s_kObjFrameSize.cy - scrollPos;
        toolRect.bottom = toolRect.top + s_kObjFrameSize.cy;
        for (unsigned int col = 0; col < 3; tool++, col++) {
            toolRect.left = col * s_kObjFrameSize.cx;
            toolRect.right = toolRect.left + s_kObjFrameSize.cx;
            if (tool < _m_aSlotInfo[_m_slot].m_objTypes.size()) {
                {
                    TGDIObjectSelector<CBrush> brushSelector(
                        &memDC, _m_abToolEnabled[tool - _m_firstTool] ? &enabledBrush : &disabledBrush);
                    memDC.Rectangle(toolRect);
                }
                void (CSprite::*pfnDraw)(int, int, int, int, int, unsigned short*, int, int, int, int, int,
                                         unsigned short, bool) const;
                if (_m_abToolEnabled[tool - _m_firstTool])
                    pfnDraw = &CSprite::DrawAdvObjWithFlag;
                else
                    pfnDraw = &CSprite::DrawAdvObjWithFlagAlpha;
                const TObjectType& objType = *_m_aSlotInfo[_m_slot].m_objTypes[tool];
                bool bHero = _m_player != ePlayerNone
                             && (objType.getType() == HERO || objType.getType() == RANDOM_HERO
                                 || objType.getType() == HERO_PLACEHOLDER);
                int srcWidth;
                int srcHeight;
                {
                    TObjectSpritePtr pObjSprite(objType);
                    srcWidth = pObjSprite->GetWidth();
                    srcHeight = pObjSprite->GetHeight();
                    {
                        TGDIObjectSelector<CBrush> brushSelector(
                            &imageDC, _m_abToolEnabled[tool - _m_firstTool] ? &enabledBrush : &disabledBrush);
                        imageDC.PatBlt(0, 0, srcWidth, srcHeight, PATCOPY);
                    }
                    pObjSprite->DrawAdvObjShadow(0, 0, 0, srcWidth, srcHeight, image.getPixels(), 0, 0,
                                                 image.getWidth(), image.getHeight(), image.getPitch(), false);
                }
                if (bHero) {
                    THeroFlagSpritePtr pFlagSprite(_m_player);
                    pFlagSprite->DrawAdvObjShadow(0, 0, 0, srcWidth, srcHeight, image.getPixels(), 0, 0,
                                                  image.getWidth(), image.getHeight(), image.getPitch(), false);
                }
                {
                    TObjectSpritePtr pObjSprite(objType);
                    (pObjSprite.get()->*pfnDraw)(0, 0, 0, srcWidth, srcHeight, image.getPixels(), 0, 0,
                                                 image.getWidth(), image.getHeight(), image.getPitch(),
                                                 flagColor, false);
                }
                if (bHero) {
                    THeroFlagSpritePtr pFlagSprite(_m_player);
                    (pFlagSprite.get()->*pfnDraw)(0, 0, 0, srcWidth, srcHeight, image.getPixels(), 0, 0,
                                                  image.getWidth(), image.getHeight(), image.getPitch(), 0,
                                                  false);
                }

                CRect frameRect = toolRect;
                frameRect.DeflateRect(1, 1);
                int dstWidth = min(srcWidth, frameRect.Width());
                int dstHeight = min(srcHeight, frameRect.Height());
                if (srcWidth >= srcHeight)
                    dstHeight = srcHeight * dstWidth / srcWidth;
                else
                    dstWidth = srcWidth * dstHeight / srcHeight;
                memDC.StretchBlt(frameRect.left + (frameRect.Width() - dstWidth) / 2,
                                 frameRect.top + (frameRect.Height() - dstHeight) / 2, dstWidth, dstHeight,
                                 &imageDC, 0, 0, srcWidth, srcHeight, SRCCOPY);
            } else {
                TGDIObjectSelector<CBrush> brushSelector(&memDC, &disabledBrush);
                memDC.PatBlt(toolRect.left, toolRect.top, toolRect.Width(), toolRect.Height(), PATCOPY);
            }
        }
    }

    dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &memDC, rect.left, rect.top, SRCCOPY);
}

VA(0x0048ccbd, 0x141)
void TObjectPaletteWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SCROLLINFO info;
    info.cbSize = sizeof(info);
    GetScrollInfo(SB_VERT, &info, SIF_PAGE | SIF_POS);
    int delta = 0;
    switch (nSBCode) {
    case SB_LINEUP:
        delta = info.nPos > s_kObjFrameSize.cy ? s_kObjFrameSize.cy : info.nPos;
        break;
    case SB_LINEDOWN: {
        int maxPos = _computeRows() * s_kObjFrameSize.cy - info.nPage;
        delta = maxPos - info.nPos > s_kObjFrameSize.cy ? -s_kObjFrameSize.cy : info.nPos - maxPos;
        break;
    }
    case SB_PAGEUP:
        delta = info.nPos > info.nPage ? info.nPage : info.nPos;
        break;
    case SB_PAGEDOWN: {
        int maxPos = _computeRows() * s_kObjFrameSize.cy - info.nPage;
        delta = maxPos - info.nPos > info.nPage ? -info.nPage : info.nPos - maxPos;
        break;
    }
    case SB_TOP:
        delta = info.nPos;
        break;
    case SB_BOTTOM:
        delta = info.nPos - (_computeRows() * s_kObjFrameSize.cy - info.nPage);
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        delta = info.nPos - nPos;
        break;
    }
    if (delta != 0) {
        _removeAllTools();
        info.nPos -= delta;
        SetScrollInfo(SB_VERT, &info);
        _m_aSlotInfo[_m_slot].m_scrollPos = info.nPos;
        ScrollWindow(0, delta);
        _setupTools(info.nPage);
        UpdateWindow();
    }
    CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

VA(0x0048cdfe, 0x75)
void TObjectPaletteWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    CPoint pos = point;
    pos.y += GetScrollPos(SB_VERT);
    unsigned int tool = pos.y / s_kObjFrameSize.cy * 3 + pos.x / s_kObjFrameSize.cx;
    if (tool < _m_aSlotInfo[_m_slot].m_objTypes.size())
        _m_pClient->onPaletteGrabObject(this, *_m_aSlotInfo[_m_slot].m_objTypes[tool]);
    CWnd::OnLButtonDown(nFlags, point);
}

VA(0x0048ce73, 0x53)
BOOL TObjectPaletteWnd::PreTranslateMessage(MSG* pMsg)
{
    switch (pMsg->message) {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        _m_toolTip.RelayEvent(pMsg);
        break;
    }
    return CWnd::PreTranslateMessage(pMsg);
}

VA(0x0048cec6, 0x193)
BOOL TObjectPaletteWnd::OnToolTipNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
    TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
    unsigned int tool = pNMHDR->idFrom - 1;
    const TObjectType* pObjType = _m_aSlotInfo[_m_slot].m_objTypes[tool];
    string text;
    switch (pObjType->getType()) {
    case ARTIFACT:
        text = akArtifactTraits[pObjType->getExtra()].m_name;
        break;
    case HERO:
        text = THero::s_akClassTraits[pObjType->getExtra()].m_name;
        break;
    case TOWN:
        text = TTown::s_akTypeTraits[pObjType->getExtra()].m_pName;
        break;
    case MONSTER:
        text = akCreatureTypeTraits[pObjType->getExtra()].m_name;
        break;
    case MINE:
    case ABANDONED_MINE:
        text = TMine::s_akMineTypeTraits[pObjType->getExtra()].m_name;
        break;
    case CREATURE_BANK:
        text = akCreatureBankTypeTraits[pObjType->getExtra()].m_name;
        break;
    case CREATURE_GENERATOR_1:
        text = TGenerator::s_akGenerator1TypeTraits[pObjType->getExtra()].m_name;
        break;
    case CREATURE_GENERATOR_4:
        text = TGenerator::s_akGenerator4TypeTraits[pObjType->getExtra()].m_name;
        break;
    case RANDOM_DWELLING_LVL:
        text = TRandomlyAlignedGenerator::s_akTypeTraits[pObjType->getExtra()].m_name;
        break;
    case RANDOM_DWELLING_FACTION:
        text = TRandomlyLeveledGenerator::s_akTypeTraits[pObjType->getExtra()].m_name;
        break;
    case RESOURCE:
        text = akGameResourceTypeTraits[pObjType->getExtra()].m_name;
        break;
    case GARRISON:
    case GARRISON2:
        text = TGarrison::s_akTypeTraits[pObjType->getExtra()].m_name;
        break;
    default:
        text = akAdvObjectTypeTraits[pObjType->getType()].m_name;
    }
    DATA(0x005a2008) static char szText[200];
    sprintf(szText, "%s", text.c_str());
    pTTT->hinst = NULL;
    pTTT->lpszText = szText;
    return TRUE;
}

VA(0x0048d059, 0x74)
void TObjectPaletteWnd::OnContextMenu(CWnd* pWnd, CPoint point)
{
    ScreenToClient(&point);
    CPoint pos = point;
    pos.y += GetScrollPos(SB_VERT);
    unsigned int tool = pos.y / s_kObjFrameSize.cy * 3 + pos.x / s_kObjFrameSize.cx;
    if (tool < _m_aSlotInfo[_m_slot].m_objTypes.size())
        displayObjectHelp(*_m_aSlotInfo[_m_slot].m_objTypes[tool]);
}

// Redraws the frames whose objects the client can or can no longer place.
VA(0x0048d0cd, 0xc1)
LRESULT TObjectPaletteWnd::OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam)
{
    if (_m_firstTool >= _m_endTool)
        return 0;
    int scrollPos = GetScrollPos(SB_VERT);
    for (unsigned int tool = _m_firstTool; tool < _m_endTool; tool++) {
        bool bEnabled = _m_pClient->onPaletteCanCreateObject(this, *_m_aSlotInfo[_m_slot].m_objTypes[tool]);
        if (bEnabled != _m_abToolEnabled[tool - _m_firstTool]) {
            _m_abToolEnabled[tool - _m_firstTool] = bEnabled;
            CRect toolRect;
            toolRect.top = tool / 3 * s_kObjFrameSize.cy - scrollPos;
            toolRect.bottom = toolRect.top + s_kObjFrameSize.cy;
            toolRect.left = tool % 3 * s_kObjFrameSize.cx;
            toolRect.right = toolRect.left + s_kObjFrameSize.cx;
            InvalidateRect(&toolRect, FALSE);
        }
    }
    UpdateWindow();
    return 0;
}

VA(0x0048d18e, 0x75)
void TObjectPaletteWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    CPoint pos = point;
    pos.y += GetScrollPos(SB_VERT);
    unsigned int tool = pos.y / s_kObjFrameSize.cy * 3 + pos.x / s_kObjFrameSize.cx;
    ::SetCursor(tool < _m_aSlotInfo[_m_slot].m_objTypes.size() ? hOpenHandCursor : khArrowCursor);
    CWnd::OnMouseMove(nFlags, point);
}

// OnMouseMove sets the cursor over the client area.
VA(0x0048d203, 0x19)
BOOL TObjectPaletteWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (pWnd == this && nHitTest == HTCLIENT)
        return FALSE;
    return CWnd::OnSetCursor(pWnd, nHitTest, message);
}
