// MapEditWnd.cpp - the map edit window (h3maped 0x4690fd..0x46f4ce; Loki
// h3maped object 55). The Windows window keeps Loki's model: the layer is
// drawn into an off-screen 16-bit image, here a DIB section selected into a
// memory DC, and the selection, brush and fill frames are XORed over it
// with a white pen. Scrolling goes through the window's own scroll bars,
// with deferred scroll repaints posted as WM_USER, and the middle button
// hands over to the panner. Objects are copied to the clipboard in a
// format registered per editor window id.
#include "editor/stdafx.h"

#include <memory>
#include <stdexcept>
#include <string>

#include <afxadv.h>

#include "va.h"
#include "editor/Clamp.h"
#include "editor/DCAttributeSelector.h"
#include "editor/GDIObjectSelector.h"
#include "editor/GUIGameObject.h"
#include "editor/MapEditWnd.h"
#include "editor/MapEditorText.h"
#include "editor/MemoryDC.h"
#include "editor/MFCFileBuf.h"
#include "editor/ObjectHelp.h"
#include "editor/ObstacleArea.h"
#include "editor/T16bppPalette.h"
#include "editor/resource.h"

// The window's timers: the mouse-leave poll, the brush's tile poll, the
// grab delay and auto-scrolling (Loki's names for the last two and the
// auto-scroll period; Windows polls the brush at the same 50 ms).
const int kMouseLeaveTimer = 2;
const int kMouseLeavePeriod = 100;
const int kBrushTimer = 3;
const int kBrushPeriod = 50;
const int kGrabTimer = 4;
const int kAutoScrollTimer = 5;
const int kAutoScrollPeriod = 50;

namespace {

DATA(0x005a1c20) UINT objectClipboardFormat = 0;
DATA(0x005a1c24) HCURSOR hPointingHandCursor = NULL;
DATA(0x005a1c28) HCURSOR hClosedHandCursor = NULL;

// The cursors the window shows over the map and over an invalid drop.
DATA(0x005a1bf4) HCURSOR khArrowCursor = ::LoadCursor(NULL, IDC_ARROW);
DATA(0x005a1bf0) HCURSOR khNoCursor = ::LoadCursor(NULL, IDC_NO);

// The mouse wheel's scroll line count: MFC's _AfxGetMouseScrollLines as a
// function object (Loki keeps the class with an empty operator()). The
// count is cached once read; a forced read asks again.
class TGetMouseScrollLinesFunc {
public:
    TGetMouseScrollLinesFunc()
        : _m_bForceFresh(false),
          _m_bGotScrollLines(false),
          _m_cachedScrollLines(0),
          _m_msgGetScrollLines(0),
          _m_registeredMessage(0)
    {
    }

    unsigned int operator()();
    void refresh() { _m_bForceFresh = true; }

private:
    bool _m_bForceFresh;
    bool _m_bGotScrollLines;
    unsigned int _m_cachedScrollLines;
    unsigned int _m_msgGetScrollLines;
    uword _m_registeredMessage;
};

VA(0x004692f7, 0x153)
unsigned int TGetMouseScrollLinesFunc::operator()()
{
    if (!_m_bForceFresh && _m_bGotScrollLines)
        return _m_cachedScrollLines;
    _m_bForceFresh = false;
    _m_bGotScrollLines = true;
    if (_m_registeredMessage == 0) {
        _m_msgGetScrollLines = ::RegisterWindowMessage(MSH_SCROLL_LINES);
        _m_registeredMessage = _m_msgGetScrollLines != 0 ? 2 : 1;
    }
    if (_m_registeredMessage == 2) {
        HWND hwMouseWheel = ::FindWindow(MSH_WHEELMODULE_CLASS, MSH_WHEELMODULE_TITLE);
        if (hwMouseWheel != NULL && _m_msgGetScrollLines != 0) {
            _m_cachedScrollLines = ::SendMessage(hwMouseWheel, _m_msgGetScrollLines, 0, 0);
            return _m_cachedScrollLines;
        }
    }
    OSVERSIONINFO ver;
    memset(&ver, 0, sizeof(ver));
    ver.dwOSVersionInfoSize = sizeof(ver);
    _m_cachedScrollLines = 3;
    if (!::GetVersionEx(&ver))
        return _m_cachedScrollLines;
    if ((ver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS || ver.dwPlatformId == VER_PLATFORM_WIN32_NT)
        && ver.dwMajorVersion < 4) {
        HKEY hKey;
        if (::RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_QUERY_VALUE, &hKey)
            == ERROR_SUCCESS) {
            TCHAR szData[128];
            DWORD dwKeyDataType;
            DWORD dwDataBufSize = sizeof(szData);
            if (::RegQueryValueEx(hKey, "WheelScrollLines", NULL, &dwKeyDataType, (LPBYTE)&szData, &dwDataBufSize)
                == ERROR_SUCCESS)
                _m_cachedScrollLines = strtoul(szData, NULL, 10);
            ::RegCloseKey(hKey);
        }
    } else if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT && ver.dwMajorVersion >= 4)
        ::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &_m_cachedScrollLines, FALSE);
    return _m_cachedScrollLines;
}

DATA(0x005a1be0) TGetMouseScrollLinesFunc getMouseScrollLines;

// The context menu's command UI: it records whether a command is enabled
// and ignores the check, radio and text updates.
class TContextMenuCmdUI : public CCmdUI {
public:
    TContextMenuCmdUI() : _m_bEnabled(false) {}

    virtual void Enable(BOOL bOn = TRUE);
    virtual void SetCheck(int nCheck = 1) {}
    virtual void SetRadio(BOOL bOn = TRUE) {}
    virtual void SetText(LPCTSTR lpszText) {}

    bool getBEnabled() const { return _m_bEnabled; }

private:
    bool _m_bEnabled;
};

VA(0x0046946c, 0x15)
void TContextMenuCmdUI::Enable(BOOL bOn)
{
    m_bEnableChanged = TRUE;
    _m_bEnabled = bOn != FALSE;
}

// The context menu's item texts from editor.txt, in menu order.
VA(0x00469481, 0x171)
void setContextMenuText(CMenu* pMenu)
{
    unsigned int index = getFirstMenuItem(pMenu);
    setMenuItemText(pMenu, index, SContextMenuText::kWhatsThisStr);
    setMenuItemText(pMenu, index, SContextMenuText::kUndoStr);
    setMenuItemText(pMenu, index, SContextMenuText::kRedoStr);
    setMenuItemText(pMenu, index, SContextMenuText::kCutStr);
    setMenuItemText(pMenu, index, SContextMenuText::kCopyStr);
    setMenuItemText(pMenu, index, SContextMenuText::kPasteStr);
    setMenuItemText(pMenu, index, SContextMenuText::kDeleteStr);
    setMenuItemText(pMenu, index, SContextMenuText::kFindStr);
    setMenuItemText(pMenu, index, SContextMenuText::kFindNextStr);
    setMenuItemText(pMenu, index, SContextMenuText::kFindPrevStr);
    setMenuItemText(pMenu, index, SContextMenuText::kPlaceObstaclesStr);
    setMenuItemText(pMenu, index, SContextMenuText::kPropertiesStr);
}

// Moves a DC's viewport origin for a scope.
class TViewportOrgOffsetter {
public:
    TViewportOrgOffsetter(CDC* pDC, int dx, int dy) : _m_pDC(pDC), _m_oldOrg(pDC->OffsetViewportOrg(dx, dy)) {}
    VA(0x0046b66b, 0x1b)
    ~TViewportOrgOffsetter() { _m_pDC->SetViewportOrg(_m_oldOrg); }

private:
    CDC* _m_pDC;
    CPoint _m_oldOrg;
};

typedef TDCAttributeSelector<int, &CDC::SetROP2> TROP2Selector;
typedef TDCAttributeSelector<int, &CDC::SetBkMode> TBkModeSelector;

}  // namespace

VA(0x004695f2, 0x373)
TMapEditWnd::TMapEditWnd(CWnd* pParent, TMapEditingWnd::TController* pController, int id, const TGameMap* pMap,
                         const TObstacleArea* pObstacleArea, bool bSecondLayer, TZoom zoom, bool bShowGrid,
                         bool bShowPassability)
    : _m_pController(pController),
      _m_id(id),
      _m_pMap(pMap),
      _m_pObstacleArea(pObstacleArea),
      _m_bSecondLayer(bSecondLayer),
      _m_pPanner(NULL),
      _m_pToolTip(NULL),
      _m_viewPos(0, 0),
      _m_frameNum(0),
      _m_bDragging(false),
      _m_hCursor(khArrowCursor),
      _m_bMouseInside(false),
      _m_cursorTilePos(pMap->getWidth(), pMap->getHeight()),
      _m_scrollDelta(0, 0),
      _m_bAutoScrollOn(false),
      _m_mode(_eModeSel),
      _m_zoom(zoom),
      _m_bShowGrid(bShowGrid),
      _m_bShowPassability(bShowPassability),
      _m_selectedObjID(TGameMap::TLayer::s_kInvalidObjID),
      _m_toolTipObjID(TGameMap::TLayer::s_kInvalidObjID),
      _m_pFloatingObj(NULL),
      _m_floatingObjPos(-1, -1),
      _m_potentialGrabID(TGameMap::TLayer::s_kInvalidObjID),
      _m_bPotentialCopy(false),
      _m_bBrushOn(false),
      _m_brushPos(-1, -1),
      _m_brushSize(0, 0),
      _m_fillRectAnchor(-1, -1),
      _m_fillRectDragPos(-1, -1),
      _m_pen(PS_SOLID, 1, RGB(255, 255, 255)),
      _m_hClipboardData(NULL)
{
    if (objectClipboardFormat == 0) {
        CString formatName;
        formatName.Format("Heroes of Might and Magic III Map Editor Object Clipboard Format %d", _m_id);
        objectClipboardFormat = ::RegisterClipboardFormat(formatName);
        if (objectClipboardFormat == 0)
            throw TRuntimeError();
    }
    if (hPointingHandCursor == NULL) {
        hPointingHandCursor = AfxGetApp()->LoadCursor(IDC_POINTING_HAND);
        hClosedHandCursor = AfxGetApp()->LoadCursor(IDC_CLOSED_HAND);
    }
    DATA_COMPGEN_GUARD(0x005a1bdc, mapEditWndClassNameGuard, className)
    VA_COMPGEN(0x0046998e, 0xa, STATIC_DTOR, className)
    static CString className;
    if (className.IsEmpty())
        className = AfxRegisterWndClass(CS_DBLCLKS, khArrowCursor);
    if (!CreateEx(WS_EX_CLIENTEDGE, className, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_HSCROLL | WS_VSCROLL,
                  CRect(0, 0, 0, 0), pParent, 0))
        throw TRuntimeError();
    _m_pPanner = new TPannerCtrl(this, this);
    if (_m_pPanner == NULL)
        throw TAllocationFailure();
    _m_pToolTip = new CToolTipCtrl;
    if (_m_pToolTip == NULL)
        throw TAllocationFailure();
    if (!_m_pToolTip->Create(this))
        throw TRuntimeError();
}

VA_COMPGEN(0x00469998, 0x1c, SCALAR_DELETING_DTOR, TMapEditWnd)

VA(0x004699b4, 0x7e)
TMapEditWnd::~TMapEditWnd()
{
    delete _m_pToolTip;
    delete _m_pPanner;
}

VA(0x00469a32, 0x43)
void TMapEditWnd::clearMap()
{
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID) {
        _m_selectedObjID = TGameMap::TLayer::s_kInvalidObjID;
        _m_pController->onEditObjectSelected(this, _m_selectedObjID);
    }
    if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
        _clearToolTipObj();
    _m_pMap = NULL;
    _m_pObstacleArea = NULL;
}

VA(0x00469a75, 0xdd)
void TMapEditWnd::setMapLayer(const TGameMap* pNewMap, const TObstacleArea* pNewObstacleArea, bool bNewSecondLayer)
{
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID) {
        _m_selectedObjID = TGameMap::TLayer::s_kInvalidObjID;
        _m_pController->onEditObjectSelected(this, _m_selectedObjID);
    }
    if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
        _clearToolTipObj();
    _m_pMap = pNewMap;
    _m_pObstacleArea = pNewObstacleArea;
    _m_bSecondLayer = bNewSecondLayer;
    _m_viewPos = CPoint(0, 0);
    SetScrollPos(SB_HORZ, 0);
    SetScrollPos(SB_VERT, 0);
    if (_m_bMouseInside) {
        KillTimer(kMouseLeaveTimer);
        _m_bMouseInside = false;
        _m_cursorTilePos = CPoint(_m_pMap->getWidth(), _m_pMap->getHeight());
    }
    CPoint origin(0, 0);
    CSize size(_m_pMap->getWidth(), _m_pMap->getHeight());
    update(CRect(origin, size));
}

VA(0x00469b52, 0x176)
void TMapEditWnd::moveViewRect(const CPoint& pos)
{
    if (pos != _m_viewPos) {
        CRect updateRect;
        if (GetUpdateRect(&updateRect)) {
            MSG msg;
            if (::PeekMessage(&msg, m_hWnd, WM_USER, WM_USER, PM_REMOVE | PM_NOYIELD))
                OnDeferredScroll(0, 0);
            UpdateWindow();
        }
        if (_m_bMouseInside) {
            CPoint newCursorTilePos = _m_cursorTilePos;
            if (pos.x != _m_viewPos.x)
                newCursorTilePos.x = _m_pMap->getWidth();
            if (pos.y != _m_viewPos.y)
                newCursorTilePos.y = _m_pMap->getHeight();
            if (newCursorTilePos != _m_cursorTilePos)
                _m_pController->onEditCursorTilePosChanged(this, _m_cursorTilePos = newCursorTilePos);
        }
        if (_m_bBrushOn) {
            if (_m_mode == _eModeBrush)
                _turnBrushOff();
            if (_m_mode == _eModeFill)
                _turnFillRectOff();
        }
        if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
            _clearToolTipObj();
        _m_scrollDelta += pos - _m_viewPos;
        _m_viewPos = pos;
        SetScrollPos(SB_HORZ, _m_viewPos.x);
        SetScrollPos(SB_VERT, _m_viewPos.y);
        MSG msg;
        if (!::PeekMessage(&msg, m_hWnd, WM_USER, WM_USER, PM_NOREMOVE | PM_NOYIELD))
            PostMessage(WM_USER);
    }
}

VA(0x00469cc8, 0x8c)
void TMapEditWnd::update(const CRect& rect)
{
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    CRect updateRect((rect.left - _m_viewPos.x) * tileSize, (rect.top - _m_viewPos.y) * tileSize,
                     (rect.right - _m_viewPos.x) * tileSize,
                     (rect.bottom - _m_viewPos.y) * tileSize + tileSize / 2);
    CRect clientRect;
    GetClientRect(&clientRect);
    CRect invalidRect;
    if (invalidRect.IntersectRect(&updateRect, &clientRect))
        InvalidateRect(&invalidRect, FALSE);
}

VA(0x00469d54, 0x19c)
void TMapEditWnd::setZoom(TZoom newZoom)
{
    if (newZoom != _m_zoom) {
        CRect clientRect;
        GetClientRect(&clientRect);
        CSize oldViewRectSize = _computeViewRectSize(clientRect.Width(), clientRect.Height());
        _m_zoom = newZoom;
        CSize viewRectSize = _computeViewRectSize(clientRect.Width(), clientRect.Height());
        CPoint maxPos(_m_pMap->getWidth() - viewRectSize.cx, _m_pMap->getHeight() - viewRectSize.cy);
        CPoint newPos(clamp(0, int(_m_viewPos.x + (oldViewRectSize.cx - viewRectSize.cx) / 2), int(maxPos.x)),
                      clamp(0, int(_m_viewPos.y + (oldViewRectSize.cy - viewRectSize.cy) / 2), int(maxPos.y)));
        SCROLLINFO scrollInfo;
        scrollInfo.cbSize = sizeof(scrollInfo);
        scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;
        scrollInfo.nMin = 0;
        scrollInfo.nMax = _m_pMap->getWidth() - 1;
        scrollInfo.nPage = viewRectSize.cx;
        scrollInfo.nPos = newPos.x;
        SetScrollInfo(SB_HORZ, &scrollInfo);
        scrollInfo.nMax = _m_pMap->getHeight() - 1;
        scrollInfo.nPage = viewRectSize.cy;
        scrollInfo.nPos = newPos.y;
        SetScrollInfo(SB_VERT, &scrollInfo);
        if (newPos != _m_viewPos) {
            _m_viewPos = newPos;
            _m_pController->onMoveMapViewRect(this, newPos);
        }
        _m_pController->onSizeMapViewRect(this, viewRectSize);
        Invalidate(FALSE);
        UpdateWindow();
    }
}

VA(0x00469ef0, 0x20)
void TMapEditWnd::showGrid(bool bShow)
{
    if (bShow != _m_bShowGrid) {
        _m_bShowGrid = bShow;
        Invalidate(FALSE);
    }
}

VA(0x00469f10, 0x20)
void TMapEditWnd::showPassability(bool bShow)
{
    if (bShow != _m_bShowPassability) {
        _m_bShowPassability = bShow;
        Invalidate(FALSE);
    }
}

VA(0x00469f30, 0x8f)
void TMapEditWnd::selectObject(unsigned int objID)
{
    if (_m_mode == _eModeSel && objID != _m_selectedObjID) {
        CClientDC dc(this);
        if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID)
            _drawSelectionFrame(&dc);
        _m_selectedObjID = objID;
        if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID)
            _drawSelectionFrame(&dc);
        _m_pController->onEditObjectSelected(this, _m_selectedObjID);
    }
}

VA(0x00469fbf, 0x1c1)
void TMapEditWnd::grabObject(const TGUIGameObject* pObj)
{
    if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
        _clearToolTipObj();
    _m_pFloatingObj = pObj;
    SetCapture();
    CPoint point;
    GetCursorPos(&point);
    ScreenToClient(&point);
    _m_floatingObjPos = _computeFloatingObjPos(point);
    CRect clientRect;
    GetClientRect(&clientRect);
    CSize viewSize = _computeViewRectSize(clientRect.Width(), clientRect.Height());
    CRect viewRect(_m_viewPos, viewSize);
    CRect floatingObjRect = _computeFloatingObjRect(_m_floatingObjPos);
    _setCursor(floatingObjRect.right > viewRect.left && floatingObjRect.bottom > viewRect.top
                       && floatingObjRect.left < viewRect.right && floatingObjRect.top < viewRect.bottom
                       && !_m_pMap->isValidPlacement(*pObj, _m_bSecondLayer, _m_floatingObjPos.x,
                                                     _m_floatingObjPos.y)
                   ? khNoCursor
                   : hClosedHandCursor);
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    floatingObjRect.left = (floatingObjRect.left - _m_viewPos.x) * tileSize;
    floatingObjRect.top = (floatingObjRect.top - _m_viewPos.y) * tileSize;
    floatingObjRect.right = (floatingObjRect.right - _m_viewPos.x) * tileSize;
    floatingObjRect.bottom = (floatingObjRect.bottom - _m_viewPos.y) * tileSize;
    CRect paintRect;
    if (paintRect.IntersectRect(&floatingObjRect, &clientRect)) {
        CClientDC dc(this);
        _paintRect(&dc, paintRect);
    }
}

VA(0x0046a180, 0x73)
void TMapEditWnd::onUndo()
{
    if (_m_mode == _eModeSel && _m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID) {
        _m_selectedObjID = TGameMap::TLayer::s_kInvalidObjID;
        _m_pController->onEditObjectSelected(this, _m_selectedObjID);
    }
    if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
        _clearToolTipObj();
    CRect rect(0, 0, _m_pMap->getWidth(), _m_pMap->getHeight());
    update(rect);
}

VA(0x0046a1f3, 0x3d)
void TMapEditWnd::onObjectRemoved(unsigned int removedObjID)
{
    if (_m_selectedObjID == removedObjID) {
        _m_selectedObjID = TGameMap::TLayer::s_kInvalidObjID;
        _m_pController->onEditObjectSelected(this, _m_selectedObjID);
    }
    if (_m_toolTipObjID == removedObjID)
        _clearToolTipObj();
}

VA(0x0046a230, 0x382)
void TMapEditWnd::animate(unsigned int frameNum, bool bForce)
{
    CRect clientRect;
    GetClientRect(&clientRect);
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    CRect cells(clientRect.left / tileSize + _m_viewPos.x, clientRect.top / tileSize + _m_viewPos.y,
                (clientRect.right + tileSize - 1) / tileSize + _m_viewPos.x,
                (clientRect.bottom + tileSize - 1) / tileSize + _m_viewPos.y);
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    if (cells.right > width)
        cells.right = width;
    if (cells.bottom > height)
        cells.bottom = height;
    const TGameMap::TLayer& layer = _getMapLayer();
    if (frameNum > 0) {
        _m_frameNum += frameNum;
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd(); ++iter) {
            TTileExtent extent = layer.getObjectExtent(*iter);
            if (!(extent.left() < cells.right && extent.top() < cells.bottom && extent.right() > cells.left
                  && extent.bottom() > cells.top))
                continue;
            const TGUIGameObject* pObject = dynamic_cast<const TGUIGameObject*>(layer.getPObject(*iter));
            if (!pObject->isAnimated())
                continue;
            CRect objRect((extent.left() - _m_viewPos.x) * tileSize, (extent.top() - _m_viewPos.y) * tileSize,
                          (extent.right() - _m_viewPos.x) * tileSize, (extent.bottom() - _m_viewPos.y) * tileSize);
            CRect invalidRect;
            if (invalidRect.IntersectRect(&objRect, &clientRect))
                InvalidateRect(&invalidRect, FALSE);
        }
    }
    if (bForce) {
        int y = 0;
        CPoint cell;
        for (cell.y = cells.top; cell.y < cells.bottom; cell.y++) {
            bool bInRun = false;
            int runStart;
            for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
                const TGameMap::TLayer::TCell& mapCell = layer.getCell(cell.x, cell.y);
                bool bAnimated = true;
                const TGroundTilesetTraits& groundTraits = akGroundTilesetTraits[mapCell.getTerrainType()];
                if (!(groundTraits.m_bAnimated && groundTraits.m_pTileTraits[mapCell.getTileNum()].m_bAnimated)) {
                    unsigned int riverType = mapCell.getRiverType();
                    if (!(riverType != 0 && akRiverTilesetTraits[riverType - 1].m_bAnimated))
                        bAnimated = false;
                }
                if (bAnimated) {
                    if (!bInRun) {
                        runStart = cell.x;
                        bInRun = true;
                    }
                } else if (bInRun) {
                    InvalidateRect(CRect(CPoint((runStart - _m_viewPos.x) * tileSize, y),
                                         CSize((cell.x - runStart) * tileSize, tileSize)),
                                   FALSE);
                    bInRun = false;
                }
            }
            if (bInRun)
                InvalidateRect(CRect(CPoint((runStart - _m_viewPos.x) * tileSize, y),
                                     CSize((cell.x - runStart) * tileSize, tileSize)),
                               FALSE);
            y += tileSize;
        }
    }
    if (_m_mode == _eModeSel && _m_pFloatingObj == NULL) {
        CPoint point;
        GetCursorPos(&point);
        if (GetCapture() == this || WindowFromPoint(point) == this) {
            ScreenToClient(&point);
            CRect rect;
            GetClientRect(&rect);
            if (rect.PtInRect(point)) {
                TMapLayerObjectID objID = _pickObject(point);
                if (objID != TGameMap::TLayer::s_kInvalidObjID) {
                    if (objID != _m_toolTipObjID) {
                        if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
                            _clearToolTipObj();
                        _setToolTipObj(objID);
                    }
                } else if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
                    _clearToolTipObj();
            }
        }
    }
}

VA(0x0046a5b2, 0x14)
const TGameMap::TLayer& TMapEditWnd::_getMapLayer() const
{
    return _m_pMap->getLayer(_m_bSecondLayer);
}

VA(0x0046a5c6, 0x14)
void TMapEditWnd::resetAnimation()
{
    _m_frameNum = 0;
    Invalidate(FALSE);
}

VA(0x0046a5da, 0x140)
void TMapEditWnd::makeVisible(unsigned int objID)
{
    CRect clientRect;
    GetClientRect(&clientRect);
    CSize viewSize = _computeViewRectSize(clientRect.Width(), clientRect.Height());
    CRect viewRect(_m_viewPos, viewSize);
    const TGameMap::TLayer& layer = _getMapLayer();
    TTileExtent extent = layer.getObjectExtent(objID);
    if (extent.left() < viewRect.left || extent.top() < viewRect.top || extent.right() > viewRect.right
        || extent.bottom() > viewRect.bottom) {
        CSize size = viewRect.Size();
        CPoint newPos(extent.left() + int(extent.width() - size.cx) / 2,
                      extent.top() + int(extent.height() - size.cy) / 2);
        newPos.x = clamp<int>(0, newPos.x, int(layer.getWidth() - size.cx));
        newPos.y = clamp<int>(0, newPos.y, int(layer.getHeight() - size.cy));
        if (newPos != _m_viewPos)
            _m_pController->onMoveMapViewRect(this, newPos);
    }
}

VA(0x0046a71a, 0x8)
void TMapEditWnd::selectionMode()
{
    _setMode(_eModeSel);
}

VA(0x0046a722, 0x19)
void TMapEditWnd::brushMode(const CSize& size)
{
    _setMode(_eModeBrush);
    _setBrushSize(size);
}

VA(0x0046a73b, 0x8)
void TMapEditWnd::fillMode()
{
    _setMode(_eModeFill);
}

VA(0x0046a743, 0x52)
void TMapEditWnd::_turnAutoScrollOn()
{
    if (SetTimer(kAutoScrollTimer, kAutoScrollPeriod, NULL)) {
        GetCursorPos(&_m_autoScrollPoint);
        ScreenToClient(&_m_autoScrollPoint);
        _m_autoScrollTime = ::GetMessageTime();
        _m_hAutoScrollDir = 0;
        _m_vAutoScrollDir = 0;
        _m_bAutoScrollOn = true;
    }
}

inline void TMapEditWnd::_turnAutoScrollOff()
{
    KillTimer(kAutoScrollTimer);
    _m_bAutoScrollOn = false;
}

VA(0x0046a795, 0x247)
void TMapEditWnd::_handleAutoScroll(const CPoint& point)
{
    if (point == _m_autoScrollPoint)
        return;
    _m_autoScrollPoint = point;
    CRect rect;
    GetClientRect(&rect);
    rect.DeflateRect(16, 16);
    CPoint dir(0, 0);
    if (point.x < rect.left)
        dir.x = point.x - rect.left - 32;
    else if (point.x >= rect.right)
        dir.x = point.x - rect.right + 33;
    if (point.y < rect.top)
        dir.y = point.y - rect.top - 32;
    else if (point.y >= rect.bottom)
        dir.y = point.y - rect.bottom + 33;
    DWORD time = ::GetMessageTime();
    unsigned int dx = 0;
    unsigned int dy = 0;
    int oldHDir = _m_hAutoScrollDir;
    _m_hAutoScrollDir = dir.x;
    if (_m_hAutoScrollDir != 0) {
        int dist = dir.x < 0 ? -dir.x : dir.x;
        unsigned int oldInterval = _m_hAutoScrollInterval;
        _m_hAutoScrollInterval = clamp(30u, unsigned(1440000 / (dist * dist)), 1200u);
        if (oldHDir == 0 || (oldHDir < 0 && dir.x > 0) || (oldHDir > 0 && dir.x < 0))
            _m_hAutoScrollDelay = time - _m_autoScrollTime;
        else {
            _m_hAutoScrollDelay += _m_hAutoScrollInterval;
            if (oldInterval >= _m_hAutoScrollDelay) {
                dx = (oldInterval - _m_hAutoScrollDelay) / _m_hAutoScrollInterval + 1;
                _m_hAutoScrollDelay += dx * _m_hAutoScrollInterval;
            }
            _m_hAutoScrollDelay -= oldInterval;
        }
    }
    int oldVDir = _m_vAutoScrollDir;
    _m_vAutoScrollDir = dir.y;
    if (_m_vAutoScrollDir != 0) {
        int dist = dir.y < 0 ? -dir.y : dir.y;
        unsigned int oldInterval = _m_vAutoScrollInterval;
        _m_vAutoScrollInterval = clamp(30u, unsigned(1440000 / (dist * dist)), 1200u);
        if (oldVDir == 0 || (oldVDir < 0 && dir.y > 0) || (oldVDir > 0 && dir.y < 0))
            _m_vAutoScrollDelay = time - _m_autoScrollTime;
        else {
            _m_vAutoScrollDelay += _m_vAutoScrollInterval;
            if (oldInterval >= _m_vAutoScrollDelay) {
                dy = (oldInterval - _m_vAutoScrollDelay) / _m_vAutoScrollInterval + 1;
                _m_vAutoScrollDelay += dy * _m_vAutoScrollInterval;
            }
            _m_vAutoScrollDelay -= oldInterval;
        }
    }
    pan(dir.x < 0 ? -dx : dx, dir.y < 0 ? -dy : dy);
    _generateMouseMove();
}

VA(0x0046a9dc, 0x91)
void TMapEditWnd::_setMode(_TMode newMode)
{
    if (newMode == _m_mode)
        return;
    switch (_m_mode) {
    case _eModeSel:
        if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID)
            selectObject(TGameMap::TLayer::s_kInvalidObjID);
        if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
            _clearToolTipObj();
        break;
    case _eModeBrush:
        if (_m_bBrushOn)
            _turnBrushOff();
        break;
    case _eModeFill:
        if (_m_bBrushOn)
            _turnFillRectOff();
        break;
    }
    _m_mode = newMode;
    switch (_m_mode) {
    case _eModeBrush:
        _turnBrushOn();
        if (_m_bBrushOn)
            OnTimer(kBrushTimer);
        break;
    }
}

VA(0x0046aa6d, 0xaa)
void TMapEditWnd::_setToolTipObj(unsigned int objID)
{
    const TGameMap::TLayer& layer = _getMapLayer();
    const TGameObject& obj = layer.getObject(objID);
    TTileExtent extent = layer.getObjectExtent(objID);
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    CRect rect((extent.left() - _m_viewPos.x) * tileSize, (extent.top() - _m_viewPos.y) * tileSize,
               (extent.right() - _m_viewPos.x) * tileSize, (extent.bottom() - _m_viewPos.y) * tileSize);
    _m_pToolTip->AddTool(this, LPSTR_TEXTCALLBACK, &rect, objID);
    _m_toolTipObjID = objID;
    _setCursor(hPointingHandCursor);
}

VA(0x0046ab17, 0x35)
void TMapEditWnd::_clearToolTipObj()
{
    _setCursor(khArrowCursor);
    _m_pToolTip->DelTool(this, _m_toolTipObjID);
    _m_toolTipObjID = TGameMap::TLayer::s_kInvalidObjID;
}

VA(0x0046ab4c, 0x5b)
void TMapEditWnd::_turnBrushOn()
{
    if (SetTimer(kBrushTimer, kBrushPeriod, NULL)) {
        _m_bBrushOn = true;
        CClientDC dc(this);
        _drawBrush(&dc);
    }
}

VA(0x0046aba7, 0x53)
void TMapEditWnd::_turnBrushOff()
{
    KillTimer(kBrushTimer);
    CClientDC dc(this);
    _drawBrush(&dc);
    _m_bBrushOn = false;
}

VA(0x0046abfa, 0x81)
void TMapEditWnd::_setBrushPos(const CPoint& pos)
{
    if (_m_bBrushOn) {
        CClientDC dc(this);
        _drawBrush(&dc);
        _m_brushPos = pos;
        _drawBrush(&dc);
    } else
        _m_brushPos = pos;
}

VA(0x0046ac7b, 0x81)
void TMapEditWnd::_setBrushSize(const CSize& brushSize)
{
    if (_m_bBrushOn) {
        CClientDC dc(this);
        _drawBrush(&dc);
        _m_brushSize = brushSize;
        _drawBrush(&dc);
    } else
        _m_brushSize = brushSize;
}

VA(0x0046acfc, 0x48)
void TMapEditWnd::_turnFillRectOff()
{
    CClientDC dc(this);
    _drawBrush(&dc);
    _m_bBrushOn = false;
}

VA(0x0046ad44, 0x6f)
void TMapEditWnd::_setFillRectAnchor(const CPoint& pos)
{
    _m_bBrushOn = true;
    _m_fillRectAnchor = pos;
    _m_fillRectDragPos = pos;
    CClientDC dc(this);
    _drawBrush(&dc);
}

VA(0x0046adb3, 0x76)
void TMapEditWnd::_setFillRectDragPos(const CPoint& pos)
{
    CClientDC dc(this);
    if (_m_bBrushOn)
        _drawBrush(&dc);
    _m_bBrushOn = true;
    _m_fillRectDragPos = pos;
    _drawBrush(&dc);
}

VA(0x0046ae29, 0x127)
void TMapEditWnd::_drawSelectionFrame(CDC* pDC)
{
    const TGameMap::TLayer& layer = _getMapLayer();
    const TGameObject& obj = layer.getObject(_m_selectedObjID);
    TTilePoint loc = layer.getObjectLoc(_m_selectedObjID);
    CRect rect;
    rect.left = loc.x() + 1 - obj.getWidth() - _m_viewPos.x;
    rect.top = loc.y() + 1 - obj.getHeight() - _m_viewPos.y;
    rect.right = rect.left + obj.getWidth();
    rect.bottom = rect.top + obj.getHeight();
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    CRect frameRect(rect.left * tileSize, rect.top * tileSize, rect.right * tileSize, rect.bottom * tileSize);
    TGDIObjectSelector<CPen> penSelector(pDC, &_m_pen);
    TROP2Selector rop2Selector(pDC, R2_XORPEN);
    pDC->MoveTo(frameRect.left, frameRect.top);
    pDC->LineTo(frameRect.right - 1, frameRect.top);
    pDC->LineTo(frameRect.right - 1, frameRect.bottom - 1);
    pDC->LineTo(frameRect.left, frameRect.bottom - 1);
    pDC->LineTo(frameRect.left, frameRect.top);
}

VA(0x0046af50, 0x1d2)
void TMapEditWnd::_drawBrush(CDC* pDC)
{
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    if (_m_mode == _eModeBrush) {
        CRect rect;
        rect.left = (_m_brushPos.x - _m_viewPos.x) * tileSize;
        rect.top = (_m_brushPos.y - _m_viewPos.y) * tileSize;
        rect.right = rect.left + _m_brushSize.cx * tileSize;
        rect.bottom = rect.top + _m_brushSize.cy * tileSize;
        TGDIObjectSelector<CPen> penSelector(pDC, &_m_pen);
        TROP2Selector rop2Selector(pDC, R2_XORPEN);
        pDC->MoveTo(rect.left, rect.top);
        pDC->LineTo(rect.right - 1, rect.top);
        pDC->LineTo(rect.right - 1, rect.bottom - 1);
        pDC->LineTo(rect.left, rect.bottom - 1);
        pDC->LineTo(rect.left, rect.top);
    } else {
        CRect rect = _computeFillRect();
        rect.left = (rect.left - _m_viewPos.x) * tileSize;
        rect.top = (rect.top - _m_viewPos.y) * tileSize;
        rect.right = (rect.right - _m_viewPos.x) * tileSize;
        rect.bottom = (rect.bottom - _m_viewPos.y) * tileSize;
        TGDIObjectSelector<CPen> penSelector(pDC, &_m_pen);
        TROP2Selector rop2Selector(pDC, R2_XORPEN);
        pDC->MoveTo(rect.left, rect.top);
        pDC->LineTo(rect.right - 1, rect.top);
        pDC->LineTo(rect.right - 1, rect.bottom - 1);
        pDC->LineTo(rect.left, rect.bottom - 1);
        pDC->LineTo(rect.left, rect.top);
    }
}

// A cell's tile rectangle drawn with the pen and brush (the passability
// overlay's frames; Loki's hatches the cell instead).
VA(0x0046c309, 0x7d)
inline void drawCellHatchedRect(CDC* pDC, CPen* pPen, CBrush* pBrush, const CPoint& pos, TZoom zoom)
{
    TGDIObjectSelector<CPen> penSelector(pDC, pPen);
    TGDIObjectSelector<CBrush> brushSelector(pDC, pBrush);
    unsigned int tileSize = akZoomTraits[zoom].m_tileSize;
    pDC->Rectangle(CRect(pos, CSize(tileSize, tileSize)));
}

// Draws the layer into the back buffer, the floating object and the panner
// over it, and blits the rectangle to the DC; the back buffer then gets back
// what lay under the floating object and the panner, and the XOR frames are
// redrawn into it so that it holds the plain layer with the frames.
VA(0x0046b122, 0x549)
void TMapEditWnd::_paintRect(CDC* pDC, CRect& rect)
{
    TMemoryDC memDC(pDC);
    TGDIObjectSelector<CBitmap> bitmapSelector(&memDC, &_m_backBuffer);
    memDC.SelectClipRgn(NULL);
    memDC.IntersectClipRect(&rect);
    _drawMap(&memDC, rect);
    bool bSelectionFrameDrawn = false;
    bool bFloatingObjDrawn = false;
    CRect floatingObjDrawRect;
    CBitmap floatingObjBackground;
    bool bBrushDrawn = false;
    if (_m_mode == _eModeSel) {
        if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID
            && _getMapLayer().getFloatingObjID() != _m_selectedObjID) {
            _drawSelectionFrame(&memDC);
            bSelectionFrameDrawn = true;
        }
        if (_m_pFloatingObj != NULL) {
            unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
            CRect floatingObjRect = _computeFloatingObjRect(_m_floatingObjPos);
            floatingObjRect.left = (floatingObjRect.left - _m_viewPos.x) * tileSize;
            floatingObjRect.right = (floatingObjRect.right - _m_viewPos.x) * tileSize;
            floatingObjRect.top = (floatingObjRect.top - _m_viewPos.y) * tileSize;
            floatingObjRect.bottom = (floatingObjRect.bottom - _m_viewPos.y) * tileSize;
            if (floatingObjDrawRect.IntersectRect(&floatingObjRect, &rect)) {
                TMemoryDC backgroundDC(&memDC);
                floatingObjBackground.CreateCompatibleBitmap(&memDC, floatingObjDrawRect.Width(),
                                                             floatingObjDrawRect.Height());
                TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &floatingObjBackground);
                backgroundDC.BitBlt(0, 0, floatingObjDrawRect.Width(), floatingObjDrawRect.Height(), &memDC,
                                    floatingObjDrawRect.left, floatingObjDrawRect.top, SRCCOPY);
                int x = (_m_floatingObjPos.x + 1 - _m_viewPos.x) * tileSize - 1;
                int y = (_m_floatingObjPos.y + 1 - _m_viewPos.y) * tileSize - 1;
                _m_pFloatingObj->draw(0, &memDC, &_m_backBuffer, x, y, _m_zoom);
                bFloatingObjDrawn = true;
            }
        }
    } else if (_m_bBrushOn) {
        _drawBrush(&memDC);
        bBrushDrawn = true;
    }
    bool bPannerDrawn = false;
    CRect pannerDrawRect;
    CBitmap pannerBackground;
    if (_m_pPanner != NULL && _m_pPanner->IsWindowVisible()) {
        CRect pannerRect;
        _m_pPanner->GetClientRect(&pannerRect);
        _m_pPanner->ClientToScreen(&pannerRect);
        ScreenToClient(&pannerRect);
        if (pannerDrawRect.IntersectRect(&pannerRect, &rect)) {
            TMemoryDC backgroundDC(&memDC);
            pannerBackground.CreateCompatibleBitmap(&memDC, pannerDrawRect.Width(), pannerDrawRect.Height());
            TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &pannerBackground);
            backgroundDC.BitBlt(0, 0, pannerDrawRect.Width(), pannerDrawRect.Height(), &memDC, pannerDrawRect.left,
                                pannerDrawRect.top, SRCCOPY);
            TViewportOrgOffsetter viewportOrgOffsetter(&memDC, pannerRect.left, pannerRect.top);
            _m_pPanner->_draw(&memDC);
            CRect validRect = pannerDrawRect;
            ClientToScreen(&validRect);
            _m_pPanner->ScreenToClient(&validRect);
            _m_pPanner->ValidateRect(&validRect);
            bPannerDrawn = true;
        }
    }
    pDC->BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &memDC, rect.left, rect.top, SRCCOPY);
    if (bPannerDrawn) {
        TMemoryDC backgroundDC(&memDC);
        TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &pannerBackground);
        memDC.BitBlt(pannerDrawRect.left, pannerDrawRect.top, pannerDrawRect.Width(), pannerDrawRect.Height(),
                     &backgroundDC, 0, 0, SRCCOPY);
    }
    if (bBrushDrawn)
        _drawBrush(&memDC);
    if (bFloatingObjDrawn) {
        TMemoryDC backgroundDC(&memDC);
        TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &floatingObjBackground);
        memDC.BitBlt(floatingObjDrawRect.left, floatingObjDrawRect.top, floatingObjDrawRect.Width(),
                     floatingObjDrawRect.Height(), &backgroundDC, 0, 0, SRCCOPY);
    }
    if (bSelectionFrameDrawn)
        _drawSelectionFrame(&memDC);
}

VA(0x0046b686, 0xc4b)
void TMapEditWnd::_drawMap(CDC* pDC, CRect& rect)
{
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    CRect cells(rect.left / tileSize + _m_viewPos.x, rect.top / tileSize + _m_viewPos.y,
                (rect.right + tileSize - 1) / tileSize + _m_viewPos.x,
                (rect.bottom + tileSize - 1) / tileSize + _m_viewPos.y);
    unsigned int width = _m_pMap->getWidth();
    unsigned int height = _m_pMap->getHeight();
    if (cells.right > width)
        cells.right = width;
    if (cells.bottom > height)
        cells.bottom = height;
    const TGameMap::TLayer& layer = _getMapLayer();
    CPoint cell;
    for (cell.y = cells.top; cell.y < cells.bottom; cell.y++)
        for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
            CPoint pos((cell.x - _m_viewPos.x) * tileSize - rect.left, (cell.y - _m_viewPos.y) * tileSize - rect.top);
            const TGameMap::TLayer::TCell& mapCell = layer.getCell(cell.x, cell.y);
            (*akZoomTraits[_m_zoom].m_pDrawTile)(akGroundTilesetTraits[mapCell.getTerrainType()].m_pSprite,
                                              mapCell.getTileNum(), pos.x, pos.y,
                                              (uword*)((ubyte*)_m_backBuffer.getPixels()
                                                       + rect.top * _m_backBuffer.getPitch())
                                                  + rect.left,
                                              rect.Width(), rect.Height(), _m_backBuffer.getPitch(),
                                              mapCell.getBHFlipped(), mapCell.getBVFlipped());
        }
    for (cell.y = cells.top; cell.y < cells.bottom; cell.y++)
        for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
            CPoint pos((cell.x - _m_viewPos.x) * tileSize - rect.left, (cell.y - _m_viewPos.y) * tileSize - rect.top);
            const TGameMap::TLayer::TCell& mapCell = layer.getCell(cell.x, cell.y);
            if (mapCell.getRiverType() != 0)
                (*akZoomTraits[_m_zoom].m_pDrawTile)(akRiverTilesetTraits[mapCell.getRiverType() - 1].m_pSprite,
                                                  mapCell.getRiverTileNum(), pos.x, pos.y,
                                                  (uword*)((ubyte*)_m_backBuffer.getPixels()
                                                           + rect.top * _m_backBuffer.getPitch())
                                                      + rect.left,
                                                  rect.Width(), rect.Height(), _m_backBuffer.getPitch(),
                                                  mapCell.getBRiverHFlipped(), mapCell.getBRiverVFlipped());
        }
    CRect roadCells = cells;
    roadCells.top--;
    roadCells.top = roadCells.top < 0 ? 0 : roadCells.top;
    for (cell.y = roadCells.top; cell.y < roadCells.bottom; cell.y++)
        for (cell.x = roadCells.left; cell.x < roadCells.right; cell.x++) {
            CPoint pos((cell.x - _m_viewPos.x) * tileSize - rect.left,
                       (cell.y - _m_viewPos.y) * tileSize + tileSize / 2 - rect.top);
            const TGameMap::TLayer::TCell& mapCell = layer.getCell(cell.x, cell.y);
            if (mapCell.getRoadType() != 0)
                (*akZoomTraits[_m_zoom].m_pDrawTile)(akRoadTilesetTraits[mapCell.getRoadType() - 1].m_pSprite,
                                                  mapCell.getRoadTileNum(), pos.x, pos.y,
                                                  (uword*)((ubyte*)_m_backBuffer.getPixels()
                                                           + rect.top * _m_backBuffer.getPitch())
                                                      + rect.left,
                                                  rect.Width(), rect.Height(), _m_backBuffer.getPitch(),
                                                  mapCell.getBRoadHFlipped(), mapCell.getBRoadVFlipped());
        }
    for (cell.y = cells.top; cell.y < cells.bottom; cell.y++)
        for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
            CPoint pos((cell.x - _m_viewPos.x + 1) * tileSize - 1, (cell.y - _m_viewPos.y + 1) * tileSize - 1);
            unsigned int numIDs = layer.getNumObjectIDsAtCell(cell.x, cell.y);
            for (unsigned int i = 0; i < numIDs; i++) {
                TMapLayerObjectID objID = layer.getObjectIDAtCell(cell.x, cell.y, i);
                const TGUIGameObject* pObject = dynamic_cast<const TGUIGameObject*>(layer.getPObject(objID));
                if (pObject->getBUnderlay()) {
                    TTilePoint loc = layer.getObjectLoc(objID);
                    pObject->drawCell(_m_frameNum, loc.x() - cell.x, loc.y() - cell.y, pDC, &_m_backBuffer, pos.x,
                                      pos.y, _m_zoom);
                } else
                    break;
            }
        }
    for (cell.y = cells.top; cell.y < cells.bottom; cell.y++)
        for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
            CPoint pos((cell.x - _m_viewPos.x + 1) * tileSize - 1, (cell.y - _m_viewPos.y + 1) * tileSize - 1);
            unsigned int numIDs = layer.getNumShadowIDsAtCell(cell.x, cell.y);
            for (unsigned int i = 0; i < numIDs; i++) {
                TMapLayerObjectID objID = layer.getShadowIDAtCell(cell.x, cell.y, i);
                const TGUIGameObject* pObject = dynamic_cast<const TGUIGameObject*>(layer.getPObject(objID));
                TTilePoint loc = layer.getObjectLoc(objID);
                pObject->drawCellShadow(_m_frameNum, loc.x() - cell.x, loc.y() - cell.y, pDC, &_m_backBuffer,
                                        pos.x, pos.y, _m_zoom);
            }
        }
    for (cell.y = cells.top; cell.y < cells.bottom; cell.y++)
        for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
            CPoint pos((cell.x - _m_viewPos.x + 1) * tileSize - 1, (cell.y - _m_viewPos.y + 1) * tileSize - 1);
            unsigned int numIDs = layer.getNumObjectIDsAtCell(cell.x, cell.y);
            for (unsigned int i = 0; i < numIDs; i++) {
                TMapLayerObjectID objID = layer.getObjectIDAtCell(cell.x, cell.y, i);
                const TGUIGameObject* pObject = dynamic_cast<const TGUIGameObject*>(layer.getPObject(objID));
                if (!pObject->getBUnderlay()) {
                    TTilePoint loc = layer.getObjectLoc(objID);
                    pObject->drawCell(_m_frameNum, loc.x() - cell.x, loc.y() - cell.y, pDC, &_m_backBuffer, pos.x,
                                      pos.y, _m_zoom);
                }
            }
        }
    if (_m_pObstacleArea->hasTiles(_m_bSecondLayer))
        for (cell.y = cells.top; cell.y < cells.bottom; cell.y++) {
            int top = (cell.y - _m_viewPos.y) * tileSize;
            unsigned int cellHeight = tileSize;
            if (top < rect.top) {
                cellHeight = top - rect.top + tileSize;
                top = rect.top;
            }
            if (top + cellHeight > rect.bottom)
                cellHeight = rect.bottom - top;
            for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
                int left = (cell.x - _m_viewPos.x) * tileSize;
                unsigned int cellWidth = tileSize;
                if (left < rect.left) {
                    cellWidth = left - rect.left + tileSize;
                    left = rect.left;
                }
                if (left + cellWidth > rect.right)
                    cellWidth = rect.right - left;
                int state = _m_pObstacleArea->getTileState(cell.x, cell.y, _m_bSecondLayer);
                if (state == 2) {
                    TRGB color = {0, 0, 0xff};
                    _m_backBuffer.blendRect(left, top, cellWidth, cellHeight, T16bppPalette::rgbToEntry(color));
                } else if (state == 1) {
                    TRGB color = {0, 0, 0xff};
                    _m_backBuffer.tintRect(left, top, cellWidth, cellHeight, T16bppPalette::rgbToEntry(color));
                }
            }
        }
    if (_m_bShowGrid) {
        CPen pen;
        pen.CreateStockObject(BLACK_PEN);
        TGDIObjectSelector<CPen> penSelector(pDC, &pen);
        for (unsigned int x = cells.left, xPos = (x - _m_viewPos.x) * tileSize; x < cells.right;
             x++, xPos += tileSize) {
            pDC->MoveTo(xPos, rect.top);
            pDC->LineTo(xPos, rect.bottom);
            pDC->MoveTo(xPos + tileSize - 1, rect.top);
            pDC->LineTo(xPos + tileSize - 1, rect.bottom);
        }
        for (unsigned int y = cells.top, yPos = (y - _m_viewPos.y) * tileSize; y < cells.bottom;
             y++, yPos += tileSize) {
            pDC->MoveTo(rect.left, yPos);
            pDC->LineTo(rect.right, yPos);
            pDC->MoveTo(rect.left, yPos + tileSize - 1);
            pDC->LineTo(rect.right, yPos + tileSize - 1);
        }
    }
    if (_m_bShowPassability) {
        CPen impassablePen(PS_SOLID, 1, RGB(255, 0, 0));
        CPen triggerPen(PS_SOLID, 1, RGB(255, 255, 0));
        CBrush brush;
        brush.CreateStockObject(NULL_BRUSH);
        TBkModeSelector bkModeSelector(pDC, TRANSPARENT);
        for (cell.y = cells.top; cell.y < cells.bottom; cell.y++) {
            int posY = (cell.y - _m_viewPos.y) * tileSize;
            int top = posY;
            unsigned int cellHeight = tileSize;
            if (top < rect.top) {
                cellHeight = top - rect.top + tileSize;
                top = rect.top;
            }
            if (top + cellHeight > rect.bottom)
                cellHeight = rect.bottom - top;
            for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
                CPoint pos((cell.x - _m_viewPos.x) * tileSize, posY);
                int left = pos.x;
                unsigned int cellWidth = tileSize;
                if (left < rect.left) {
                    cellWidth = left - rect.left + tileSize;
                    left = rect.left;
                }
                if (left + cellWidth > rect.right)
                    cellWidth = rect.right - left;
                unsigned int numIDs = layer.getNumObjectIDsAtCell(cell.x, cell.y);
                for (unsigned int i = 0; i < numIDs; i++) {
                    TMapLayerObjectID objID = layer.getObjectIDAtCell(cell.x, cell.y, i);
                    const TGameObject& obj = layer.getObject(objID);
                    TTilePoint loc = layer.getObjectLoc(objID);
                    int cellX = loc.x() - cell.x;
                    int cellY = loc.y() - cell.y;
                    if (obj.getBCellTrigger(cellX, cellY)) {
                        TRGB color = {0xff, 0xff, 0};
                        _m_backBuffer.tintRect(left, top, cellWidth, cellHeight, T16bppPalette::rgbToEntry(color));
                        drawCellHatchedRect(pDC, &triggerPen, &brush, pos, _m_zoom);
                        break;
                    } else if (!obj.getBCellPassable(cellX, cellY)) {
                        TRGB color = {0xff, 0, 0};
                        _m_backBuffer.tintRect(left, top, cellWidth, cellHeight, T16bppPalette::rgbToEntry(color));
                        drawCellHatchedRect(pDC, &impassablePen, &brush, pos, _m_zoom);
                        break;
                    }
                }
            }
        }
    }
    CBrush backgroundBrush(::GetSysColor(COLOR_WINDOW));
    TGDIObjectSelector<CBrush> brushSelector(pDC, &backgroundBrush);
    if ((rect.right + tileSize - 1) / tileSize + _m_viewPos.x > width) {
        CRect outside = rect;
        outside.left = (width - _m_viewPos.x) * tileSize;
        pDC->PatBlt(outside.left, outside.top, outside.Width(), outside.Height(), PATCOPY);
    }
    if ((rect.bottom + tileSize - 1) / tileSize + _m_viewPos.y > height) {
        CRect outside = rect;
        outside.top = (height - _m_viewPos.y) * tileSize;
        pDC->PatBlt(outside.left, outside.top, outside.Width(), outside.Height(), PATCOPY);
    }
}

VA(0x0046c386, 0xfe)
void TMapEditWnd::_generateMouseMove()
{
    CPoint point;
    if (GetCursorPos(&point)) {
        CPoint clientPoint = point;
        ScreenToClient(&clientPoint);
        CRect clientRect;
        GetClientRect(&clientRect);
        if (GetCapture() == this || (WindowFromPoint(point) == this && clientRect.PtInRect(clientPoint))) {
            UINT flags = ((::GetKeyState(VK_CONTROL) & ~1) ? MK_CONTROL : 0)
                         | ((::GetKeyState(VK_MBUTTON) & ~1) ? MK_MBUTTON : 0)
                         | ((::GetKeyState(VK_SHIFT) & ~1) ? MK_SHIFT : 0)
                         | ((::GetKeyState(VK_RBUTTON) & ~1) ? MK_RBUTTON : 0)
                         | ((::GetKeyState(VK_LBUTTON) & ~1) ? MK_LBUTTON : 0);
            SendMessage(WM_MOUSEMOVE, flags, MAKELPARAM(clientPoint.x, clientPoint.y));
        }
    }
}

// Streams the selected object to the clipboard in the editor's format.
VA(0x0046c484, 0x16c)
bool TMapEditWnd::_copySelectedObject()
{
    if (!OpenClipboard())
        return false;
    try {
        try {
            if (!::EmptyClipboard())
                throw std::runtime_error(std::string());
            CSharedFile file(GMEM_MOVEABLE | GMEM_DDESHARE, 0x1000);
            {
                TMFCFileBuf buf(&file);
                TGameMap::streamObject(&buf, *_m_pMap->getLayer(_m_bSecondLayer).getPObject(_m_selectedObjID));
            }
            _m_hClipboardData = file.Detach();
            if (_m_hClipboardData == NULL)
                throw std::runtime_error(std::string());
            ::SetClipboardData(objectClipboardFormat, _m_hClipboardData);
        } catch (...) {
            ::CloseClipboard();
            throw;
        }
    } catch (CException* e) {
        e->Delete();
        return false;
    }
    ::CloseClipboard();
    return true;
}

VA(0x0046c5f0, 0x12e)
TMapLayerObjectID TMapEditWnd::_pickObject(const CPoint& point) const
{
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    CPoint cell(point.x / tileSize + _m_viewPos.x, point.y / tileSize + _m_viewPos.y);
    if (!(cell.x < _m_pMap->getWidth() && cell.y < _m_pMap->getHeight()))
        return TGameMap::TLayer::s_kInvalidObjID;
    const TGameMap::TLayer& layer = _getMapLayer();
    TMapLayerObjectID pickedID = TGameMap::TLayer::s_kInvalidObjID;
    unsigned int numIDs = layer.getNumObjectIDsAtCell(cell.x, cell.y);
    for (unsigned int i = 0; i < numIDs; i++) {
        TMapLayerObjectID objID = layer.getObjectIDAtCell(cell.x, cell.y, i);
        const TGUIGameObject* pObject = dynamic_cast<const TGUIGameObject*>(layer.getPObject(objID));
        TTilePoint loc = layer.getObjectLoc(objID);
        CPoint objPos((loc.x() + 1 - _m_viewPos.x) * tileSize - 1, (loc.y() + 1 - _m_viewPos.y) * tileSize - 1);
        if (pObject->hitTest(_m_frameNum, objPos.x - point.x, objPos.y - point.y, _m_zoom))
            pickedID = objID;
    }
    return pickedID;
}

VA(0x0046c71e, 0x63)
CPoint TMapEditWnd::_computeFloatingObjPos(const CPoint& point) const
{
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    return CPoint((point.x + ((_m_pFloatingObj->getWidth() - 1) * tileSize >> 1)) / tileSize + _m_viewPos.x,
                  (point.y + ((_m_pFloatingObj->getHeight() - 1) * tileSize >> 1)) / tileSize + _m_viewPos.y);
}

VA(0x0046c781, 0x4e)
CRect TMapEditWnd::_computeFloatingObjRect(const CPoint& point) const
{
    return CRect(CPoint(point.x + 1 - _m_pFloatingObj->getWidth(), point.y + 1 - _m_pFloatingObj->getHeight()),
                 CSize(_m_pFloatingObj->getWidth(), _m_pFloatingObj->getHeight()));
}

VA(0x0046c7cf, 0x90)
CSize TMapEditWnd::_computeViewRectSize(int cx, int cy) const
{
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    return CSize(clamp(1, int(cx / tileSize), int(_m_pMap->getWidth())),
                 clamp(1, int(cy / tileSize), int(_m_pMap->getHeight())));
}

VA(0x0046c85f, 0x1a3)
CPoint TMapEditWnd::_computeBrushPos(const CPoint& point) const
{
    int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    CPoint pos((point.x - (_m_brushSize.cx - 1) * tileSize / 2) / tileSize + _m_viewPos.x,
               (point.y - (_m_brushSize.cy - 1) * tileSize / 2) / tileSize + _m_viewPos.y);
    CRect clientRect;
    GetClientRect(&clientRect);
    CRect viewRect;
    viewRect.left = _m_viewPos.x;
    viewRect.top = _m_viewPos.y;
    viewRect.right = viewRect.left + (clientRect.Width() + tileSize - 1) / tileSize < _m_pMap->getWidth()
                         ? viewRect.left + (clientRect.Width() + tileSize - 1) / tileSize
                         : _m_pMap->getWidth();
    viewRect.bottom = viewRect.top + (clientRect.Height() + tileSize - 1) / tileSize < _m_pMap->getHeight()
                          ? viewRect.top + (clientRect.Height() + tileSize - 1) / tileSize
                          : _m_pMap->getHeight();
    if (_m_brushSize.cx < viewRect.Width())
        pos.x = clamp<int>(viewRect.left, pos.x, int(viewRect.right - _m_brushSize.cx));
    else
        pos.x = clamp<int>(0, pos.x, int(_m_pMap->getWidth() - _m_brushSize.cx));
    if (_m_brushSize.cy < viewRect.Height())
        pos.y = clamp<int>(viewRect.top, pos.y, int(viewRect.bottom - _m_brushSize.cy));
    else
        pos.y = clamp<int>(0, pos.y, int(_m_pMap->getHeight() - _m_brushSize.cy));
    return pos;
}

VA(0x0046ca02, 0x5a)
CRect TMapEditWnd::_computeFillRect() const
{
    CRect rect;
    if (_m_fillRectAnchor.x <= _m_fillRectDragPos.x) {
        rect.left = _m_fillRectAnchor.x;
        rect.right = _m_fillRectDragPos.x + 1;
    } else {
        rect.left = _m_fillRectDragPos.x;
        rect.right = _m_fillRectAnchor.x + 1;
    }
    if (_m_fillRectAnchor.y <= _m_fillRectDragPos.y) {
        rect.top = _m_fillRectAnchor.y;
        rect.bottom = _m_fillRectDragPos.y + 1;
    } else {
        rect.top = _m_fillRectDragPos.y;
        rect.bottom = _m_fillRectAnchor.y + 1;
    }
    return rect;
}

VA(0x0046ca5c, 0x105)
CPoint TMapEditWnd::_computeFillRectDragPos(const CPoint& point) const
{
    int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    CPoint pos(point.x / tileSize + _m_viewPos.x, point.y / tileSize + _m_viewPos.y);
    CRect clientRect;
    GetClientRect(&clientRect);
    CRect viewRect;
    viewRect.left = _m_viewPos.x;
    viewRect.top = _m_viewPos.y;
    viewRect.right = viewRect.left + (clientRect.Width() + tileSize - 1) / tileSize < _m_pMap->getWidth()
                         ? viewRect.left + (clientRect.Width() + tileSize - 1) / tileSize
                         : _m_pMap->getWidth();
    viewRect.bottom = viewRect.top + (clientRect.Height() + tileSize - 1) / tileSize < _m_pMap->getHeight()
                          ? viewRect.top + (clientRect.Height() + tileSize - 1) / tileSize
                          : _m_pMap->getHeight();
    pos.x = clamp<int>(viewRect.left, pos.x, int(viewRect.right - 1));
    pos.y = clamp<int>(viewRect.top, pos.y, int(viewRect.bottom - 1));
    return pos;
}

VA(0x0046cb61, 0x35)
bool TMapEditWnd::canPanHorizontally()
{
    SCROLLINFO scrollInfo;
    scrollInfo.cbSize = sizeof(scrollInfo);
    return GetScrollInfo(SB_HORZ, &scrollInfo, SIF_RANGE | SIF_PAGE)
           && scrollInfo.nPage <= unsigned(scrollInfo.nMax - scrollInfo.nMin);
}

VA(0x0046cb96, 0x35)
bool TMapEditWnd::canPanVertically()
{
    SCROLLINFO scrollInfo;
    scrollInfo.cbSize = sizeof(scrollInfo);
    return GetScrollInfo(SB_VERT, &scrollInfo, SIF_RANGE | SIF_PAGE)
           && scrollInfo.nPage <= unsigned(scrollInfo.nMax - scrollInfo.nMin);
}

VA(0x0046cbcb, 0xd0)
void TMapEditWnd::pan(int dx, int dy)
{
    SCROLLINFO scrollInfo;
    scrollInfo.cbSize = sizeof(scrollInfo);
    GetScrollInfo(SB_HORZ, &scrollInfo, SIF_PAGE);
    int hPageSize = scrollInfo.nPage;
    GetScrollInfo(SB_VERT, &scrollInfo, SIF_PAGE);
    int vPageSize = scrollInfo.nPage;
    CPoint newPos = _m_viewPos + CPoint(dx, dy);
    newPos.x = clamp<int>(0, newPos.x, int(_m_pMap->getWidth() - hPageSize));
    newPos.y = clamp<int>(0, newPos.y, int(_m_pMap->getHeight() - vPageSize));
    if (newPos != _m_viewPos)
        _m_pController->onMoveMapViewRect(this, newPos);
}

VA(0x0046cc9b, 0x6)
BEGIN_MESSAGE_MAP(TMapEditWnd, TMapEditingWnd)
    ON_WM_SIZE()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_PAINT()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_TIMER()
    ON_WM_LBUTTONDBLCLK()
    ON_COMMAND(ID_EDIT_PROPERTIES, OnEditProperties)
    ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
    ON_COMMAND(ID_LEFT, OnLeft)
    ON_COMMAND(ID_RIGHT, OnRight)
    ON_COMMAND(ID_UP, OnUp)
    ON_COMMAND(ID_DOWN, OnDown)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateSelectionCommand)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateSelectionCommand)
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateSelectionCommand)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PROPERTIES, OnUpdateSelectionCommand)
    ON_WM_CONTEXTMENU()
    ON_WM_RBUTTONDOWN()
    ON_WM_MOUSEWHEEL()
    ON_WM_MBUTTONDOWN()
    ON_WM_SETTINGCHANGE()
    ON_WM_CAPTURECHANGED()
    ON_COMMAND(ID_HELP_WHATS_THIS, OnHelpWhatsThis)
    ON_UPDATE_COMMAND_UI(ID_HELP_WHATS_THIS, OnUpdateSelectionCommand)
    ON_WM_SETCURSOR()
    ON_WM_DESTROYCLIPBOARD()
    ON_MESSAGE(WM_USER, OnDeferredScroll)
    ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNeedText)
END_MESSAGE_MAP()

VA(0x0046cca1, 0x105)
void TMapEditWnd::OnSize(UINT nType, int cx, int cy)
{
    if (cx == 0 || cy == 0)
        return;
    _m_backBuffer.create(cx, cy);
    CSize viewRectSize = _computeViewRectSize(cx, cy);
    CPoint maxPos(_m_pMap->getWidth() - viewRectSize.cx, _m_pMap->getHeight() - viewRectSize.cy);
    SCROLLINFO scrollInfo;
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_DISABLENOSCROLL;
    scrollInfo.nMin = 0;
    scrollInfo.nMax = _m_pMap->getWidth() - 1;
    scrollInfo.nPage = viewRectSize.cx;
    SetScrollInfo(SB_HORZ, &scrollInfo);
    scrollInfo.nMax = _m_pMap->getHeight() - 1;
    scrollInfo.nPage = viewRectSize.cy;
    SetScrollInfo(SB_VERT, &scrollInfo);
    Invalidate(FALSE);
    if (_m_viewPos.x > maxPos.x || _m_viewPos.y > maxPos.y) {
        CPoint newPos(_m_viewPos.x < maxPos.x ? _m_viewPos.x : maxPos.x,
                      _m_viewPos.y < maxPos.y ? _m_viewPos.y : maxPos.y);
        _m_pController->onMoveMapViewRect(this, newPos);
    }
    _m_pController->onSizeMapViewRect(this, viewRectSize);
}

VA(0x0046cda6, 0xb6)
void TMapEditWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SCROLLINFO scrollInfo;
    scrollInfo.cbSize = sizeof(scrollInfo);
    GetScrollInfo(SB_HORZ, &scrollInfo, SIF_PAGE);
    unsigned int newPos = _m_viewPos.x;
    switch (nSBCode) {
    case SB_LINELEFT:
        if ((int)newPos > 0)
            newPos--;
        break;
    case SB_LINERIGHT:
        if (newPos < _m_pMap->getWidth() - scrollInfo.nPage)
            newPos++;
        break;
    case SB_PAGELEFT:
        newPos -= scrollInfo.nPage;
        if ((int)newPos < 0)
            newPos = 0;
        break;
    case SB_PAGERIGHT:
        newPos += scrollInfo.nPage;
        if (newPos > _m_pMap->getWidth() - scrollInfo.nPage)
            newPos = _m_pMap->getWidth() - scrollInfo.nPage;
        break;
    case SB_LEFT:
        newPos = 0;
        break;
    case SB_RIGHT:
        newPos = _m_pMap->getWidth() - scrollInfo.nPage;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        newPos = nPos;
        break;
    }
    if (newPos != _m_viewPos.x)
        _m_pController->onMoveMapViewRect(this, CPoint(newPos, _m_viewPos.y));
}

VA(0x0046ce5c, 0xb6)
void TMapEditWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    SCROLLINFO scrollInfo;
    scrollInfo.cbSize = sizeof(scrollInfo);
    GetScrollInfo(SB_VERT, &scrollInfo, SIF_PAGE);
    unsigned int newPos = _m_viewPos.y;
    switch (nSBCode) {
    case SB_LINEUP:
        if ((int)newPos > 0)
            newPos--;
        break;
    case SB_LINEDOWN:
        if (newPos < _m_pMap->getHeight() - scrollInfo.nPage)
            newPos++;
        break;
    case SB_PAGEUP:
        newPos -= scrollInfo.nPage;
        if ((int)newPos < 0)
            newPos = 0;
        break;
    case SB_PAGEDOWN:
        newPos += scrollInfo.nPage;
        if (newPos > _m_pMap->getHeight() - scrollInfo.nPage)
            newPos = _m_pMap->getHeight() - scrollInfo.nPage;
        break;
    case SB_TOP:
        newPos = 0;
        break;
    case SB_BOTTOM:
        newPos = _m_pMap->getHeight() - scrollInfo.nPage;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        newPos = nPos;
        break;
    }
    if (newPos != _m_viewPos.y)
        _m_pController->onMoveMapViewRect(this, CPoint(_m_viewPos.x, newPos));
}

// Paints the update region's rectangles one by one through the back buffer.
VA(0x0046cf12, 0x16a)
void TMapEditWnd::OnPaint()
{
    if (_m_pMap == NULL) {
        CPaintDC dc(this);
        return;
    }
    CRgn updateRgn;
    if (updateRgn.CreateRectRgn(0, 0, 0, 0)) {
        int type = GetUpdateRgn(&updateRgn);
        if (type != NULLREGION && type != ERROR) {
            CPaintDC dc(this);
            int size = updateRgn.GetRegionData(NULL, 0);
            std::auto_ptr<RGNDATA> pRgnData((RGNDATA*)new char[size]);
            if (pRgnData.get() == NULL)
                throw TAllocationFailure();
            pRgnData->rdh.dwSize = sizeof(RGNDATAHEADER);
            updateRgn.GetRegionData(pRgnData.get(), size);
            for (unsigned int i = 0; i < pRgnData->rdh.nCount; i++) {
                CRect rect(&((const RECT*)pRgnData->Buffer)[i]);
                _paintRect(&dc, rect);
            }
        }
    }
}

VA_COMPGEN(0x0046d07c, 0x1c, SCALAR_DELETING_DTOR, CRgn)
VA_COMPGEN(0x0046d098, 0x29, IMPLICIT_DTOR, CRgn)

VA(0x0046d0c1, 0xacd)
void TMapEditWnd::OnMouseMove(UINT nFlags, CPoint point)
{
    CRect clientRect;
    GetClientRect(&clientRect);
    if (clientRect.PtInRect(point)) {
        if (_m_bMouseInside)
            KillTimer(kMouseLeaveTimer);
        if (SetTimer(kMouseLeaveTimer, kMouseLeavePeriod, NULL)) {
            _m_bMouseInside = true;
            unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
            CPoint cursorTilePos(point.x / tileSize + _m_viewPos.x, point.y / tileSize + _m_viewPos.y);
            if (cursorTilePos != _m_cursorTilePos)
                _m_pController->onEditCursorTilePosChanged(this, _m_cursorTilePos = cursorTilePos);
        } else
            _m_bMouseInside = false;
    }
    switch (_m_mode) {
    case _eModeSel:
        if (_m_pFloatingObj != NULL) {
            CPoint newPos = _computeFloatingObjPos(point);
            if (newPos != _m_floatingObjPos) {
                CSize viewSize = _computeViewRectSize(clientRect.Width(), clientRect.Height());
                CRect viewRect(_m_viewPos, viewSize);
                CRect oldRect = _computeFloatingObjRect(_m_floatingObjPos);
                CRect newRect = _computeFloatingObjRect(newPos);
                _m_floatingObjPos = newPos;
                _setCursor(newRect.right > viewRect.left && newRect.bottom > viewRect.top
                                   && newRect.left < viewRect.right && newRect.top < viewRect.bottom
                                   && !_m_pMap->isValidPlacement(*_m_pFloatingObj, _m_bSecondLayer,
                                                                 _m_floatingObjPos.x, _m_floatingObjPos.y)
                               ? khNoCursor
                               : hClosedHandCursor);
                unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
                CRect updateRect;
                if (updateRect.IntersectRect(&oldRect, &newRect)) {
                    updateRect.UnionRect(&oldRect, &newRect);
                    updateRect.left = (updateRect.left - _m_viewPos.x) * tileSize;
                    updateRect.top = (updateRect.top - _m_viewPos.y) * tileSize;
                    updateRect.right = (updateRect.right - _m_viewPos.x) * tileSize;
                    updateRect.bottom = (updateRect.bottom - _m_viewPos.y) * tileSize;
                    CRect paintRect;
                    if (paintRect.IntersectRect(&updateRect, &clientRect)) {
                        CClientDC dc(this);
                        TMemoryDC memDC(&dc);
                        TGDIObjectSelector<CBitmap> bitmapSelector(&memDC, &_m_backBuffer);
                        memDC.SelectClipRgn(NULL);
                        memDC.IntersectClipRect(&paintRect);
                        bool bSelectionFrameDrawn = false;
                        if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID
                            && _getMapLayer().getFloatingObjID() != _m_selectedObjID) {
                            _drawSelectionFrame(&memDC);
                            bSelectionFrameDrawn = true;
                        }
                        bool bFloatingObjDrawn = false;
                        CBitmap background;
                        newRect.left = (newRect.left - _m_viewPos.x) * tileSize;
                        newRect.top = (newRect.top - _m_viewPos.y) * tileSize;
                        newRect.right = (newRect.right - _m_viewPos.x) * tileSize;
                        newRect.bottom = (newRect.bottom - _m_viewPos.y) * tileSize;
                        CRect drawRect;
                        if (drawRect.IntersectRect(&newRect, &paintRect)) {
                            TMemoryDC backgroundDC(&memDC);
                            background.CreateCompatibleBitmap(&memDC, drawRect.Width(), drawRect.Height());
                            TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &background);
                            backgroundDC.BitBlt(0, 0, drawRect.Width(), drawRect.Height(), &memDC, drawRect.left,
                                                drawRect.top, SRCCOPY);
                            int x = (_m_floatingObjPos.x + 1 - _m_viewPos.x) * tileSize - 1;
                            int y = (_m_floatingObjPos.y + 1 - _m_viewPos.y) * tileSize - 1;
                            _m_pFloatingObj->draw(0, &memDC, &_m_backBuffer, x, y, _m_zoom);
                            bFloatingObjDrawn = true;
                        }
                        dc.BitBlt(paintRect.left, paintRect.top, paintRect.Width(), paintRect.Height(), &memDC,
                                  paintRect.left, paintRect.top, SRCCOPY);
                        if (bFloatingObjDrawn) {
                            TMemoryDC backgroundDC(&memDC);
                            TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &background);
                            memDC.BitBlt(drawRect.left, drawRect.top, drawRect.Width(), drawRect.Height(),
                                         &backgroundDC, 0, 0, SRCCOPY);
                        }
                        if (bSelectionFrameDrawn)
                            _drawSelectionFrame(&memDC);
                    }
                } else {
                    CClientDC dc(this);
                    TMemoryDC memDC(&dc);
                    TGDIObjectSelector<CBitmap> bitmapSelector(&memDC, &_m_backBuffer);
                    oldRect.left = (oldRect.left - _m_viewPos.x) * tileSize;
                    oldRect.top = (oldRect.top - _m_viewPos.y) * tileSize;
                    oldRect.right = (oldRect.right - _m_viewPos.x) * tileSize;
                    oldRect.bottom = (oldRect.bottom - _m_viewPos.y) * tileSize;
                    CRect paintRect;
                    if (paintRect.IntersectRect(&oldRect, &clientRect)) {
                        dc.SelectClipRgn(NULL);
                        dc.IntersectClipRect(&paintRect);
                        try {
                            dc.BitBlt(paintRect.left, paintRect.top, paintRect.Width(), paintRect.Height(), &memDC,
                                      paintRect.left, paintRect.top, SRCCOPY);
                            if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID
                                && _getMapLayer().getFloatingObjID() != _m_selectedObjID)
                                _drawSelectionFrame(&dc);
                        } catch (...) {
                            dc.SelectClipRgn(NULL);
                            throw;
                        }
                        dc.SelectClipRgn(NULL);
                    }
                    newRect.left = (newRect.left - _m_viewPos.x) * tileSize;
                    newRect.top = (newRect.top - _m_viewPos.y) * tileSize;
                    newRect.right = (newRect.right - _m_viewPos.x) * tileSize;
                    newRect.bottom = (newRect.bottom - _m_viewPos.y) * tileSize;
                    if (paintRect.IntersectRect(&newRect, &clientRect)) {
                        memDC.SelectClipRgn(NULL);
                        memDC.IntersectClipRect(&paintRect);
                        bool bSelectionFrameDrawn = false;
                        if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID
                            && _getMapLayer().getFloatingObjID() != _m_selectedObjID) {
                            _drawSelectionFrame(&memDC);
                            bSelectionFrameDrawn = true;
                        }
                        TMemoryDC backgroundDC(&memDC);
                        CBitmap background;
                        background.CreateCompatibleBitmap(&memDC, paintRect.Width(), paintRect.Height());
                        TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &background);
                        backgroundDC.BitBlt(0, 0, paintRect.Width(), paintRect.Height(), &memDC, paintRect.left,
                                            paintRect.top, SRCCOPY);
                        int x = (_m_floatingObjPos.x + 1 - _m_viewPos.x) * tileSize - 1;
                        int y = (_m_floatingObjPos.y + 1 - _m_viewPos.y) * tileSize - 1;
                        _m_pFloatingObj->draw(0, &memDC, &_m_backBuffer, x, y, _m_zoom);
                        dc.BitBlt(paintRect.left, paintRect.top, paintRect.Width(), paintRect.Height(), &memDC,
                                  paintRect.left, paintRect.top, SRCCOPY);
                        memDC.BitBlt(paintRect.left, paintRect.top, paintRect.Width(), paintRect.Height(),
                                     &backgroundDC, 0, 0, SRCCOPY);
                        if (bSelectionFrameDrawn)
                            _drawSelectionFrame(&memDC);
                    }
                }
            }
        } else if (_m_potentialGrabID != TGameMap::TLayer::s_kInvalidObjID) {
            bool bGrab = false;
            int dragWidth = ::GetSystemMetrics(SM_CXDRAG);
            int left = _m_potentialGrabPoint.x - dragWidth / 2;
            if (point.x < left || point.x >= left + dragWidth)
                bGrab = true;
            else {
                int dragHeight = ::GetSystemMetrics(SM_CYDRAG);
                int top = _m_potentialGrabPoint.y - dragHeight / 2;
                if (point.y < top || point.y >= top + dragHeight)
                    bGrab = true;
            }
            if (bGrab)
                OnTimer(kGrabTimer);
        }
        break;
    case _eModeBrush:
        if (_m_bBrushOn) {
            KillTimer(kBrushTimer);
            if (!SetTimer(kBrushTimer, kBrushPeriod, NULL))
                _turnBrushOff();
        } else
            _turnBrushOn();
        if (_m_bBrushOn) {
            CPoint brushPos = _computeBrushPos(point);
            if (brushPos != _m_brushPos) {
                _setBrushPos(brushPos);
                if (_m_bDragging)
                    _m_pController->onEditBrushDrag(this, CRect(_m_brushPos, _m_brushSize));
            }
        }
        break;
    case _eModeFill:
        if (_m_bDragging) {
            CPoint dragPos = _computeFillRectDragPos(point);
            if (dragPos != _m_fillRectDragPos) {
                _setFillRectDragPos(dragPos);
                _m_pController->onEditFillRectDrag(this, _computeFillRect());
            } else if (!_m_bBrushOn)
                _setFillRectDragPos(dragPos);
        }
        break;
    }
}

VA(0x0046db8e, 0x1bb)
void TMapEditWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
    switch (_m_mode) {
    case _eModeSel: {
        TMapLayerObjectID objID = _pickObject(point);
        selectObject(objID);
        if (objID != TGameMap::TLayer::s_kInvalidObjID) {
            UINT delay = ::GetDoubleClickTime();
            DWORD elapsed = ::GetTickCount() - ::GetMessageTime();
            delay = delay > elapsed ? delay - elapsed : 1;
            if (SetTimer(kGrabTimer, delay, NULL)) {
                SetCapture();
                _m_potentialGrabID = objID;
                _m_bPotentialCopy = (nFlags & MK_CONTROL) != 0;
                _m_potentialGrabPoint = point;
            }
        }
        break;
    }
    case _eModeBrush:
        _setBrushPos(_computeBrushPos(point));
        SetCapture();
        _turnAutoScrollOn();
        _m_bDragging = true;
        _m_pController->onEditBrushBeginDrag(this, CRect(_m_brushPos, _m_brushSize));
        break;
    case _eModeFill: {
        unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
        CPoint cell(point.x / tileSize + _m_viewPos.x, point.y / tileSize + _m_viewPos.y);
        if (!(cell.x < _m_pMap->getWidth() && cell.y < _m_pMap->getHeight()))
            break;
        _setFillRectAnchor(cell);
        SetCapture();
        _turnAutoScrollOn();
        _m_bDragging = true;
        _m_pController->onEditFillRectAnchor(this, _computeFillRect());
        break;
    }
    }
}

VA(0x0046dd49, 0x3a)
void TMapEditWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
    switch (_m_mode) {
    case _eModeSel:
        if (_m_pFloatingObj != NULL || _m_potentialGrabID != TGameMap::TLayer::s_kInvalidObjID)
            ReleaseCapture();
        break;
    case _eModeBrush:
    case _eModeFill:
        if (_m_bDragging)
            ReleaseCapture();
        break;
    }
}

VA(0x0046dd83, 0x8)
void TMapEditWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    OnEditProperties();
}

VA(0x0046dd8b, 0x34)
void TMapEditWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
    if (_m_mode == _eModeSel && GetCapture() != this)
        selectObject(_pickObject(point));
}

// The middle button hands over to the panner (PreTranslateMessage): the
// mouse-leave and brush polls run once now.
VA(0x0046ddbf, 0x29)
void TMapEditWnd::OnMButtonDown(UINT nFlags, CPoint point)
{
    if (_m_bMouseInside)
        OnTimer(kMouseLeaveTimer);
    if (_m_bBrushOn)
        OnTimer(kBrushTimer);
}

VA(0x0046dde8, 0x8d)
BOOL TMapEditWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    int scrollLines = getMouseScrollLines();
    if (scrollLines > 0 || scrollLines == WHEEL_PAGESCROLL) {
        int bar = (nFlags & MK_SHIFT) ? SB_HORZ : SB_VERT;
        SCROLLINFO scrollInfo;
        scrollInfo.cbSize = sizeof(scrollInfo);
        GetScrollInfo(bar, &scrollInfo, SIF_PAGE);
        int delta = scrollLines != WHEEL_PAGESCROLL && (UINT)scrollLines < scrollInfo.nPage
                        ? zDelta / -WHEEL_DELTA * scrollLines
                        : zDelta / -WHEEL_DELTA * (int)scrollInfo.nPage;
        if (bar == SB_HORZ)
            pan(delta, 0);
        else
            pan(0, delta);
        _generateMouseMove();
    }
    return TRUE;
}

VA(0x0046de75, 0x27f)
void TMapEditWnd::OnTimer(UINT nIDEvent)
{
    switch (nIDEvent) {
    case kMouseLeaveTimer: {
        CWnd* pCaptureWnd = GetCapture();
        if (pCaptureWnd != NULL && pCaptureWnd != this)
            _m_bMouseInside = false;
        else {
            CPoint point;
            ::GetCursorPos(&point);
            if (::WindowFromPoint(point) != m_hWnd)
                _m_bMouseInside = false;
            else {
                CRect rect;
                GetClientRect(&rect);
                ClientToScreen(&rect);
                if (!rect.PtInRect(point))
                    _m_bMouseInside = false;
            }
        }
        if (!_m_bMouseInside) {
            KillTimer(kMouseLeaveTimer);
            CPoint cursorTilePos(_m_pMap->getWidth(), _m_pMap->getHeight());
            if (cursorTilePos != _m_cursorTilePos)
                _m_pController->onEditCursorTilePosChanged(this, _m_cursorTilePos = cursorTilePos);
        }
        break;
    }
    case kBrushTimer:
        if (!_m_bDragging) {
            if (GetCapture() != NULL)
                _turnBrushOff();
            else {
                CPoint point;
                ::GetCursorPos(&point);
                if (::WindowFromPoint(point) != m_hWnd)
                    _turnBrushOff();
                else {
                    CRect rect;
                    GetClientRect(&rect);
                    ClientToScreen(&rect);
                    if (!rect.PtInRect(point))
                        _turnBrushOff();
                }
            }
        }
        break;
    case kGrabTimer: {
        TMapLayerObjectID grabID = _m_potentialGrabID;
        bool bCopy = _m_bPotentialCopy;
        ReleaseCapture();
        if (bCopy)
            _m_pController->onEditCopyObject(this, grabID);
        else
            _m_pController->onEditGrabObject(this, grabID);
        break;
    }
    case kAutoScrollTimer:
        if (_m_bAutoScrollOn) {
            CPoint point;
            if (GetCursorPos(&point)) {
                ScreenToClient(&point);
                _handleAutoScroll(point);
            }
            DWORD time = ::GetMessageTime();
            unsigned int dx = 0;
            unsigned int dy = 0;
            unsigned int elapsed = time - _m_autoScrollTime;
            _m_autoScrollTime = time;
            if (_m_hAutoScrollDir != 0) {
                unsigned int remainder = elapsed;
                if (remainder >= _m_hAutoScrollDelay) {
                    remainder -= _m_hAutoScrollDelay;
                    dx = remainder / _m_hAutoScrollInterval + 1;
                    remainder %= _m_hAutoScrollInterval;
                    _m_hAutoScrollDelay = _m_hAutoScrollInterval;
                }
                _m_hAutoScrollDelay -= remainder;
            }
            if (_m_vAutoScrollDir != 0) {
                unsigned int remainder = elapsed;
                if (remainder >= _m_vAutoScrollDelay) {
                    remainder -= _m_vAutoScrollDelay;
                    dy = remainder / _m_vAutoScrollInterval + 1;
                    remainder %= _m_vAutoScrollInterval;
                    _m_vAutoScrollDelay = _m_vAutoScrollInterval;
                }
                _m_vAutoScrollDelay -= remainder;
            }
            pan(_m_hAutoScrollDir < 0 ? -dx : dx, _m_vAutoScrollDir < 0 ? -dy : dy);
            _generateMouseMove();
        }
        break;
    }
    CWnd::OnTimer(nIDEvent);
}

VA(0x0046e0f4, 0x1d)
void TMapEditWnd::OnEditProperties()
{
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID)
        _m_pController->onEditEditObject(this, _m_selectedObjID);
}

VA(0x0046e111, 0x1d)
void TMapEditWnd::OnEditDelete()
{
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID)
        _m_pController->onEditDeleteObject(this, _m_selectedObjID);
}

VA(0x0046e12e, 0x16)
void TMapEditWnd::OnLeft()
{
    OnHScroll(SB_LINELEFT, 0, NULL);
    _generateMouseMove();
}

VA(0x0046e144, 0x17)
void TMapEditWnd::OnRight()
{
    OnHScroll(SB_LINERIGHT, 0, NULL);
    _generateMouseMove();
}

VA(0x0046e15b, 0x16)
void TMapEditWnd::OnUp()
{
    OnVScroll(SB_LINEUP, 0, NULL);
    _generateMouseMove();
}

VA(0x0046e171, 0x17)
void TMapEditWnd::OnDown()
{
    OnVScroll(SB_LINEDOWN, 0, NULL);
    _generateMouseMove();
}

VA(0x0046e188, 0x2f)
void TMapEditWnd::OnEditCut()
{
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID && _copySelectedObject())
        _m_pController->onEditDeleteObject(this, _m_selectedObjID);
}

VA(0x0046e1b7, 0x14)
void TMapEditWnd::OnEditCopy()
{
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID)
        _copySelectedObject();
}

// Pastes the clipboard's object into the view (the controller floats it).
VA(0x0046e1cb, 0x17c)
void TMapEditWnd::OnEditPaste()
{
    if (::IsClipboardFormatAvailable(objectClipboardFormat) && OpenClipboard()) {
        try {
            HGLOBAL hData = ::GetClipboardData(objectClipboardFormat);
            {
                CSharedFile file(GMEM_MOVEABLE | GMEM_DDESHARE, 0x1000);
                file.SetHandle(hData, FALSE);
                {
                    TMFCFileBuf buf(&file);
                    std::auto_ptr<TGameObject> pObject = _m_pMap->reconstructObject(&buf, _m_id);
                    if (pObject.get() == NULL)
                        throw TAllocationFailure();
                    CRect clientRect;
                    GetClientRect(&clientRect);
                    CSize viewSize = _computeViewRectSize(clientRect.Width(), clientRect.Height());
                    _m_pController->onEditPasteObject(this, *pObject, CRect(_m_viewPos, viewSize));
                }
                file.Detach();
                ::GlobalUnlock(hData);
            }
            ::CloseClipboard();
        } catch (...) {
            ::CloseClipboard();
            throw;
        }
    }
}

VA(0x0046e347, 0x1e)
void TMapEditWnd::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(::IsClipboardFormatAvailable(objectClipboardFormat));
}

VA(0x0046e365, 0x1f)
void TMapEditWnd::OnDestroyClipboard()
{
    CWnd::OnDestroyClipboard();
    ::GlobalFree(_m_hClipboardData);
    _m_hClipboardData = NULL;
}

// The context menu shows the resource menu's commands that are enabled for
// this window, with separators only between shown groups.
VA(0x0046e384, 0x1ba)
void TMapEditWnd::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if (GetCapture() == this)
        return;
    CMenu menu;
    if (menu.CreatePopupMenu()) {
        CMenu resourceMenu;
        if (resourceMenu.LoadMenu(IDR_CONTEXT_MENU)) {
            setContextMenuText(&resourceMenu);
            CFrameWnd* pFrame = GetParentFrame();
            bool bSeparator = false;
            unsigned int numItems = 0;
            unsigned int count = resourceMenu.GetMenuItemCount();
            for (unsigned int i = 0; i < count; i++) {
                UINT state = resourceMenu.GetMenuState(i, MF_BYPOSITION);
                if (state & MF_SEPARATOR) {
                    bSeparator = true;
                    continue;
                }
                UINT id = resourceMenu.GetMenuItemID(i);
                TContextMenuCmdUI cmdUI;
                cmdUI.m_nID = id;
                cmdUI.DoUpdate(pFrame, TRUE);
                if (cmdUI.getBEnabled()) {
                    if (bSeparator) {
                        if (numItems > 0)
                            menu.AppendMenu(MF_SEPARATOR);
                        bSeparator = false;
                    }
                    CString text;
                    resourceMenu.GetMenuString(i, text, MF_BYPOSITION);
                    menu.AppendMenu(state, id, text);
                    numItems++;
                }
            }
            resourceMenu.DestroyMenu();
            if (numItems > 0)
                menu.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, pFrame);
        }
    }
}

// The middle button press goes to the panner before the window sees it.
VA(0x0046e604, 0x2b)
BOOL TMapEditWnd::PreTranslateMessage(MSG* pMsg)
{
    if (_m_pPanner != NULL && pMsg->message == WM_MBUTTONDOWN)
        _m_pPanner->relayEvent(pMsg);
    return CWnd::PreTranslateMessage(pMsg);
}

VA(0x0046e62f, 0xa)
void TMapEditWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    getMouseScrollLines.refresh();
}

VA(0x0046e639, 0x264)
void TMapEditWnd::OnCaptureChanged(CWnd* pWnd)
{
    switch (_m_mode) {
    case _eModeSel:
        if (_m_pFloatingObj != NULL) {
            CRect clientRect;
            GetClientRect(&clientRect);
            CRect viewRect(_m_viewPos, _computeViewRectSize(clientRect.Width(), clientRect.Height()));
            CRect floatingObjRect = _computeFloatingObjRect(_m_floatingObjPos);
            if (floatingObjRect.left < viewRect.right && floatingObjRect.top < viewRect.bottom
                && floatingObjRect.right > viewRect.left && floatingObjRect.bottom > viewRect.top)
                _m_pController->onEditPlaceObject(this, *_m_pFloatingObj, _m_floatingObjPos.x, _m_floatingObjPos.y);
            else
                _m_pController->onEditDiscardObject(this, *_m_pFloatingObj);
            _m_pFloatingObj = NULL;
            unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
            floatingObjRect.left = (floatingObjRect.left - _m_viewPos.x) * tileSize;
            floatingObjRect.top = (floatingObjRect.top - _m_viewPos.y) * tileSize;
            floatingObjRect.right = (floatingObjRect.right - _m_viewPos.x) * tileSize;
            floatingObjRect.bottom = (floatingObjRect.bottom - _m_viewPos.y) * tileSize;
            CRect paintRect;
            if (paintRect.IntersectRect(&floatingObjRect, &clientRect)) {
                InvalidateRect(&floatingObjRect, FALSE);
                UpdateWindow();
            }
            _setCursor(khArrowCursor);
        } else if (_m_potentialGrabID != TGameMap::TLayer::s_kInvalidObjID) {
            KillTimer(kGrabTimer);
            _m_potentialGrabID = TGameMap::TLayer::s_kInvalidObjID;
        }
        break;
    case _eModeBrush:
        if (_m_bDragging) {
            if (_m_bAutoScrollOn)
                _turnAutoScrollOff();
            _m_bDragging = false;
            _m_pController->onEditBrushEndDrag(this);
        }
        break;
    case _eModeFill:
        if (_m_bDragging) {
            if (_m_bAutoScrollOn)
                _turnAutoScrollOff();
            _m_bDragging = false;
            CPoint point;
            GetCursorPos(&point);
            ScreenToClient(&point);
            _setFillRectDragPos(_computeFillRectDragPos(point));
            _m_pController->onEditFillRectEndDrag(this, _computeFillRect());
            _turnFillRectOff();
        }
        break;
    }
    CWnd::OnCaptureChanged(pWnd);
}

// Repaints after the view moved: what stayed visible is scrolled within the
// back buffer and only the uncovered region is drawn again, unless the move
// is a page or more, which invalidates the window.
VA(0x0046e89d, 0x7ae)
LRESULT TMapEditWnd::OnDeferredScroll(WPARAM wParam, LPARAM lParam)
{
    if (_m_scrollDelta != CPoint(0, 0)) {
        CRect clientRect;
        GetClientRect(&clientRect);
        ClientToScreen(&clientRect);
        CRect screenRect(CPoint(0, 0), CSize(::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN)));
        CRect visibleRect;
        if (visibleRect.IntersectRect(&clientRect, &screenRect)) {
            ScreenToClient(&visibleRect);
            int visibleWidth = visibleRect.Width();
            int visibleHeight = visibleRect.Height();
            unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
            _m_scrollDelta.x *= tileSize;
            _m_scrollDelta.y *= tileSize;
            if (_m_scrollDelta.x < visibleWidth && _m_scrollDelta.x > -visibleWidth
                && _m_scrollDelta.y < visibleHeight && _m_scrollDelta.y > -visibleHeight) {
                CClientDC dc(this);
                CRgn updateRgn;
                updateRgn.CreateRectRgn(0, 0, 0, 0);
                TMemoryDC memDC(&dc);
                TGDIObjectSelector<CBitmap> bitmapSelector(&memDC, &_m_backBuffer);
                CRect updateRect;
                memDC.ScrollDC(-_m_scrollDelta.x, -_m_scrollDelta.y, &visibleRect, &visibleRect, &updateRgn,
                               &updateRect);
                int size = updateRgn.GetRegionData(NULL, 0);
                {
                    std::auto_ptr<RGNDATA> pRgnData((RGNDATA*)new char[size]);
                    if (pRgnData.get() == NULL)
                        throw TAllocationFailure();
                    pRgnData->rdh.dwSize = sizeof(RGNDATAHEADER);
                    updateRgn.GetRegionData(pRgnData.get(), size);
                    for (unsigned int i = 0; i < pRgnData->rdh.nCount; i++) {
                        memDC.IntersectClipRect(&((const RECT*)pRgnData->Buffer)[i]);
                        CRect rect(&((const RECT*)pRgnData->Buffer)[i]);
                        _drawMap(&memDC, rect);
                        memDC.SelectClipRgn(NULL);
                    }
                }
                bool bSelectionFrameDrawn = false;
                bool bBrushDrawn = false;
                bool bFloatingObjDrawn = false;
                CRect floatingObjDrawRect;
                CBitmap floatingObjBackground;
                bool bPannerDrawn = false;
                CRect pannerDrawRect;
                CBitmap pannerBackground;
                if (_m_mode == _eModeSel) {
                    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID
                        && _getMapLayer().getFloatingObjID() != _m_selectedObjID) {
                        _drawSelectionFrame(&memDC);
                        bSelectionFrameDrawn = true;
                    }
                    if (_m_pFloatingObj != NULL) {
                        CRect floatingObjRect = _computeFloatingObjRect(_m_floatingObjPos);
                        floatingObjRect.left = (floatingObjRect.left - _m_viewPos.x) * tileSize;
                        floatingObjRect.right = (floatingObjRect.right - _m_viewPos.x) * tileSize;
                        floatingObjRect.top = (floatingObjRect.top - _m_viewPos.y) * tileSize;
                        floatingObjRect.bottom = (floatingObjRect.bottom - _m_viewPos.y) * tileSize;
                        if (floatingObjDrawRect.IntersectRect(&floatingObjRect, &visibleRect)) {
                            TMemoryDC backgroundDC(&memDC);
                            floatingObjBackground.CreateCompatibleBitmap(&memDC, floatingObjDrawRect.Width(),
                                                                         floatingObjDrawRect.Height());
                            TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &floatingObjBackground);
                            backgroundDC.BitBlt(0, 0, floatingObjDrawRect.Width(), floatingObjDrawRect.Height(),
                                                &memDC, floatingObjDrawRect.left, floatingObjDrawRect.top, SRCCOPY);
                            int x = (_m_floatingObjPos.x + 1 - _m_viewPos.x) * tileSize - 1;
                            int y = (_m_floatingObjPos.y + 1 - _m_viewPos.y) * tileSize - 1;
                            _m_pFloatingObj->draw(0, &memDC, &_m_backBuffer, x, y, _m_zoom);
                            bFloatingObjDrawn = true;
                        }
                    }
                } else if (_m_bBrushOn) {
                    _drawBrush(&memDC);
                    bBrushDrawn = true;
                }
                if (_m_pPanner != NULL && _m_pPanner->IsWindowVisible()) {
                    CRect pannerRect;
                    _m_pPanner->GetClientRect(&pannerRect);
                    _m_pPanner->ClientToScreen(&pannerRect);
                    ScreenToClient(&pannerRect);
                    if (pannerDrawRect.IntersectRect(&pannerRect, &visibleRect)) {
                        TMemoryDC backgroundDC(&memDC);
                        pannerBackground.CreateCompatibleBitmap(&memDC, pannerDrawRect.Width(), pannerDrawRect.Height());
                        TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &pannerBackground);
                        backgroundDC.BitBlt(0, 0, pannerDrawRect.Width(), pannerDrawRect.Height(), &memDC,
                                            pannerDrawRect.left, pannerDrawRect.top, SRCCOPY);
                        TViewportOrgOffsetter viewportOrgOffsetter(&memDC, pannerRect.left, pannerRect.top);
                        _m_pPanner->_draw(&memDC);
                        CRect validRect = pannerDrawRect;
                        ClientToScreen(&validRect);
                        _m_pPanner->ScreenToClient(&validRect);
                        _m_pPanner->ValidateRect(&validRect);
                        bPannerDrawn = true;
                    }
                }
                dc.BitBlt(visibleRect.left, visibleRect.top, visibleRect.Width(), visibleRect.Height(), &memDC,
                          visibleRect.left, visibleRect.top, SRCCOPY);
                if (bPannerDrawn) {
                    TMemoryDC backgroundDC(&memDC);
                    TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &pannerBackground);
                    memDC.BitBlt(pannerDrawRect.left, pannerDrawRect.top, pannerDrawRect.Width(),
                                 pannerDrawRect.Height(), &backgroundDC, 0, 0, SRCCOPY);
                }
                if (bBrushDrawn)
                    _drawBrush(&memDC);
                if (bFloatingObjDrawn) {
                    TMemoryDC backgroundDC(&memDC);
                    TGDIObjectSelector<CBitmap> backgroundSelector(&backgroundDC, &floatingObjBackground);
                    memDC.BitBlt(floatingObjDrawRect.left, floatingObjDrawRect.top, floatingObjDrawRect.Width(),
                                 floatingObjDrawRect.Height(), &backgroundDC, 0, 0, SRCCOPY);
                }
                if (bSelectionFrameDrawn)
                    _drawSelectionFrame(&memDC);
            } else {
                Invalidate(FALSE);
                UpdateWindow();
            }
            _m_scrollDelta = CPoint(0, 0);
        }
    }
    return 0;
}

// Object tool tips: the window relays its mouse messages to the tool tip
// control and picks the tool tip's object as the mouse moves.
VA(0x0046f04b, 0x116)
LRESULT TMapEditWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_MOUSEMOVE && _m_mode == _eModeSel && _m_pFloatingObj == NULL) {
        CPoint point(lParam);
        TMapLayerObjectID objID = _pickObject(point);
        if (objID != TGameMap::TLayer::s_kInvalidObjID) {
            if (objID != _m_toolTipObjID) {
                if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
                    _clearToolTipObj();
                _setToolTipObj(objID);
            }
        } else if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
            _clearToolTipObj();
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
        DWORD pos = ::GetMessagePos();
        msg.pt.x = (short)LOWORD(pos);
        msg.pt.y = (short)HIWORD(pos);
        _m_pToolTip->RelayEvent(&msg);
        break;
    }
    }
    return CWnd::WindowProc(message, wParam, lParam);
}

VA(0x0046f161, 0x78)
BOOL TMapEditWnd::OnToolTipNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
    if (pNMHDR->hwndFrom != _m_pToolTip->m_hWnd)
        return FALSE;
    TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
    DATA(0x005a1b10) static char text[200];
    TMapLayerObjectID objID = _m_toolTipObjID;
    const TGameMap::TLayer& layer = _getMapLayer();
    strncpy(text, layer.getPObject(objID)->getTypeName().c_str(), sizeof(text));
    text[sizeof(text) - 1] = '\0';
    pTTT->hinst = NULL;
    pTTT->lpszText = text;
    return TRUE;
}

VA(0x0046f1d9, 0x2b)
void TMapEditWnd::OnHelpWhatsThis()
{
    TMapLayerObjectID objID = _m_selectedObjID;
    if (objID != TGameMap::TLayer::s_kInvalidObjID) {
        const TGameMap::TLayer& layer = _getMapLayer();
        displayObjectHelp(layer.getPObject(objID)->getObjectType());
    }
}

// The selection commands' state (one body for each of them in the image).
VA(0x0046f204, 0x21)
void TMapEditWnd::OnUpdateSelectionCommand(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID);
}

VA(0x0046f225, 0x25)
BOOL TMapEditWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (pWnd == this && nHitTest == HTCLIENT) {
        ::SetCursor(_m_hCursor);
        return FALSE;
    }
    return CWnd::OnSetCursor(pWnd, nHitTest, message);
}
