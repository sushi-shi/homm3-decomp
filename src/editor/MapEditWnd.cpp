// MapEditWnd.cpp - Loki h3maped object 55: the map edit window, the view
// of one map layer that the map frame embeds. It draws the layer into an
// off-screen 16-bit image, tracks the selection, the floating (grabbed)
// object, the terrain brush and the fill rectangle, scrolls through two
// GtkAdjustments and reports every edit to its controller. Copy and paste
// go through ~/.h3mapedclipboard. Assert and throw lines come from the
// retail immediates.
#include "editor/stdafx.h"

#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <fstream.h>
#include <memory>
#include <string>
#include <gtk/gtkmain.h>

#include "exceptions.h"
#include "editor/Clamp.h"
#include "editor/MapEditWnd.h"
#include "editor/GameMap.h"
#include "editor/GUIGameObject.h"
#include "editor/Tile.h"

// A brush drag repaints two tiles around the pointer; a pressed object is
// grabbed when the pointer leaves the 100-pixel square around the press or
// when the grab timer fires; the auto-scroll timer ticks every 50 ms.
const int kBrushRepaintMargin = 2;
const int kGrabDragSize = 100;
const int kGrabTimer = 4;
const int kAutoScrollTimer = 5;
const int kAutoScrollPeriod = 50;

namespace {

TMapEditWnd* mapwindow = NULL;
unsigned int objectClipboardFormat = 0;
GdkCursor* khArrowCursor = NULL;
GdkCursor* khNoCursor = NULL;
GdkCursor* hPointingHandCursor = NULL;
GdkCursor* hClosedHandCursor = NULL;

// The object on the clipboard: a clone of the copied object, streamed to
// the clipboard file as soon as it is made.
class TGameObjectDataSource {
public:
    TGameObjectDataSource(const TGameObject* pObject);
    ~TGameObjectDataSource();

    virtual bool OnRenderFileData();

private:
    TGameObject* _m_pObject;
};

TGameObjectDataSource::TGameObjectDataSource(const TGameObject* pObject)
    : _m_pObject(NULL)
{
#line 110
    assert(pObject != NULL);
    _m_pObject = pObject->clone(operator new);
    if (_m_pObject == NULL)
#line 114
        throw TAllocationFailure(__FILE__, __LINE__);
    if (!OnRenderFileData())
        doMessageBox("Error copying data to clipboard!");
}

TGameObjectDataSource::~TGameObjectDataSource()
{
    delete _m_pObject;
}

bool TGameObjectDataSource::OnRenderFileData()
{
    char* home = getenv("HOME");
    if (home == NULL)
        return false;
    size_t length = strlen(home) + 100;
    char path[length];
    snprintf(path, length, "%s/.h3mapedclipboard", home);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
        return false;
    filebuf buf(fd);
    try {
        TGameMap::streamObject(&buf, *_m_pObject);
    } catch (runtime_error& e) {
        close(fd);
        return false;
    }
    buf.sync();
    close(fd);
    return true;
}

// The mouse wheel's scroll line count (MFC's _AfxGetMouseScrollLines),
// not implemented on Loki.
class TGetMouseScrollLinesFunc {
public:
    TGetMouseScrollLinesFunc()
        : _m_bGotScrollLines(false),
          _m_bRegisteredMessage(false),
          _m_cachedScrollLines(0),
          _m_msgGetScrollLines(0),
          _m_registeredMessage(0)
    {
    }

    unsigned int operator()();

private:
    bool _m_bGotScrollLines;
    bool _m_bRegisteredMessage;
    unsigned int _m_cachedScrollLines;
    unsigned int _m_msgGetScrollLines;
    uword _m_registeredMessage;
};

unsigned int TGetMouseScrollLinesFunc::operator()()
{
    g_warning("TGetMouseScrollLinesFunc::operator() ...not implemented!\n");
    return 0;
}

TGetMouseScrollLinesFunc getMouseScrollLines;

// A cell's tile rectangle hatched with every third pixel of each row in
// color, clipped to pImage.
inline void drawCellHatchedRect(const CPoint& pos, uword color, TZoom zoom, GdkImage* pImage)
{
    unsigned int tileSize = akZoomTraits[zoom].m_tileSize;
    int left = pos.x;
    int top = pos.y;
    int right = left + tileSize;
    int bottom = top + tileSize;
    int phase = 3;
    if (left < 0)
        left = 0;
    if (top < 0)
        top = 0;
    if (right > pImage->width)
        right = pImage->width;
    if (bottom > pImage->height)
        bottom = pImage->height;
    right--;
    bottom--;
    guchar* pBits = (guchar*)pImage->mem;
    guchar* pRow = pBits + pImage->bpl * top + pImage->bpp * left;
    guchar* pEnd = pBits + pImage->bpl * bottom + pImage->bpp * right;
    uword* p;
    unsigned int span = pImage->bpp * (right - left);
    for (; pRow < pEnd; pRow += pImage->bpl) {
        for (p = (uword*)(pRow + span) - phase; p >= (uword*)pRow; p -= 3)
            *p = color;
        if (--phase <= 0)
            phase = 3;
    }
}

// The grid's one-pixel lines and the black beyond the map's edges, drawn
// into the 16-bit off-screen image.
static inline void fillRect(const CRect& rect, uword color, GdkImage* pImage)
{
    int left = rect.left < 0 ? 0 : rect.left;
    int top = rect.top < 0 ? 0 : rect.top;
    int right = (rect.right > pImage->width ? pImage->width : rect.right) - 1;
    int bottom = (rect.bottom > pImage->height ? pImage->height : rect.bottom) - 1;
    guchar* pBits = (guchar*)pImage->mem;
    guchar* pRow = pBits + pImage->bpl * top + pImage->bpp * left;
    guchar* pEnd = pBits + pImage->bpl * bottom + pImage->bpp * right;
    uword* p;
    unsigned int span = pImage->bpp * (right - left);
    for (; pRow < pEnd; pRow += pImage->bpl)
        for (p = (uword*)(pRow + span); p >= (uword*)pRow; p--)
            *p = color;
}

static inline void drawHLine(int y, uword color, GdkImage* pImage)
{
    if (!(y >= 0 && y < pImage->height))
        return;
    guchar* pBits = (guchar*)pImage->mem;
    uword* p = (uword*)(pBits + pImage->bpl * y);
    int width = pImage->width;
    for (int i = 0; i < width; i++) {
        *p = color;
        p++;
    }
}

static inline void drawVLine(int x, uword color, GdkImage* pImage)
{
    if (!(x >= 0 && x < pImage->width))
        return;
    guchar* pBits = (guchar*)pImage->mem;
    uword* p = (uword*)(pBits + pImage->bpp * x);
    int height = pImage->height;
    for (int i = 0; i < height; i++) {
        *p = color;
        p += pImage->width;
    }
}

}  // namespace

#line 461
TMapEditWnd::TMapEditWnd(GtkWidget* thisWidget, TMapEditingWnd::TController* pController, int id,
                         const TGameMap* pMap, bool bSecondLayer, TZoom zoom, bool bShowGrid,
                         bool bShowPassability, GtkAdjustment* hAdjust, GtkAdjustment* vAdjust)
    : _m_hAdjust(hAdjust),
      _m_vAdjust(vAdjust),
      _m_pController(pController),
      _m_id(id),
      _m_pMap(pMap),
      _m_bSecondLayer(bSecondLayer),
      _m_viewPos(0, 0),
      _m_pImage(NULL),
      _m_frameNum(0),
      _m_bDragging(false),
      _m_hCursor(khArrowCursor),
      _m_bMouseInside(false),
      _m_cursorTilePos(pMap->getWidth(), pMap->getHeight()),
      _m_scrollDelta(0, 0),
      _m_bAutoScrollOn(false),
      _m_autoScrollPoint(),
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
      _m_potentialGrabPoint(),
      _m_bBrushOn(false),
      _m_brushPos(0, 0),
      _m_brushSize(0, 0),
      _m_fillRectAnchor(-1, -1),
      _m_fillRectDragPos(-1, -1),
      _m_bPanning(false),
      _m_panPoint()
{
#line 471
    assert(thisWidget != NULL);
    assert(pController != NULL);
    assert(_m_pMap != NULL);
    assert(!_m_bSecondLayer || _m_pMap->isTwoLayer());
    assert(zoom >= 0 && zoom < kNumZooms);
    assert(hAdjust != NULL);
    assert(vAdjust != NULL);
    assert(hAdjust != vAdjust);
    _m_bHasCapture = false;
    _m_hWnd = thisWidget;
    mapwindow = this;
    gtk_widget_add_events(_m_hWnd, GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_MOTION_MASK
                                   | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON2_MOTION_MASK
                                   | GDK_BUTTON3_MOTION_MASK | GDK_BUTTON_PRESS_MASK
                                   | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK
                                   | GDK_KEY_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK
                                   | GDK_LEAVE_NOTIFY_MASK);
    if (objectClipboardFormat == 0) {
        // The Windows editor registers its clipboard format here.
    }
    if (hPointingHandCursor == NULL) {
#line 501
        assert(hClosedHandCursor == NULL);
        assert(khArrowCursor == NULL);
        khArrowCursor = gdk_cursor_new(GDK_ARROW);
        hPointingHandCursor = gdk_cursor_new(GDK_HAND2);
        hClosedHandCursor = gdk_cursor_new(GDK_HAND2);
        khNoCursor = gdk_cursor_new(GDK_CIRCLE);
    }
#line 516
    assert(khArrowCursor != NULL);
    assert(hPointingHandCursor != NULL);
    assert(hClosedHandCursor != NULL);
}

TMapEditWnd::~TMapEditWnd()
{
}

void TMapEditWnd::dropUnexpectedFloater()
{
    if (_m_pFloatingObj != NULL) {
        _m_floatingObjPos.x = _m_floatingObjPos.y = -99999;
        if (GetCapture() == this)
            ReleaseCapture();
        else
            OnCaptureChanged(_m_hWnd);
    }
}

unsigned int TMapEditWnd::getCurrentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static gint timerCallback(gpointer data)
{
    if (mapwindow != NULL)
        mapwindow->OnTimer((unsigned int)data);
}

UINT TMapEditWnd::SetTimer(int id, unsigned int elapse, void* pTimerFunc)
{
#line 583
    assert(id < (sizeof (_m_timers) / sizeof (gint)));
    _m_timers[id] = gtk_timeout_add(elapse, timerCallback, (gpointer)id);
    return 1;
}

void TMapEditWnd::clearMap()
{
    dropUnexpectedFloater();
#line 592
    assert(_m_potentialGrabID == TGameMap::TLayer::s_kInvalidObjID);
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID) {
        _m_selectedObjID = TGameMap::TLayer::s_kInvalidObjID;
        _m_pController->onEditObjectSelected(this, _m_selectedObjID);
    }
    if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
        _clearToolTipObj();
    _m_pMap = NULL;
}

void TMapEditWnd::setMapLayer(const TGameMap* pNewMap, bool bNewSecondLayer)
{
    dropUnexpectedFloater();
#line 610
    assert(pNewMap != NULL);
    assert(!bNewSecondLayer || pNewMap->isTwoLayer());
    assert(_m_potentialGrabID == TGameMap::TLayer::s_kInvalidObjID);
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID) {
        _m_selectedObjID = TGameMap::TLayer::s_kInvalidObjID;
        _m_pController->onEditObjectSelected(this, _m_selectedObjID);
    }
    if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
        _clearToolTipObj();
    _m_pMap = pNewMap;
    _m_bSecondLayer = bNewSecondLayer;
#line 626
    assert(!_m_bDragging);
    _m_viewPos = CPoint(0, 0);
    gtk_adjustment_set_value(_m_hAdjust, 0);
    gtk_adjustment_set_value(_m_vAdjust, 0);
    if (_m_bMouseInside)
        _m_cursorTilePos = CPoint(_m_pMap->getWidth(), _m_pMap->getHeight());
    CPoint origin(0, 0);
    CSize size(_m_pMap->getWidth(), _m_pMap->getHeight());
    update(CRect(origin, size));
}

void TMapEditWnd::moveViewRect(const CPoint& pos)
{
    if (pos != _m_viewPos) {
        if (_m_bMouseInside) {
            CPoint newCursorTilePos = _m_cursorTilePos;
            if (pos.x != _m_viewPos.x)
                newCursorTilePos.x = _m_pMap->getWidth();
            if (pos.y != _m_viewPos.y)
                newCursorTilePos.y = _m_pMap->getHeight();
            if (newCursorTilePos != _m_cursorTilePos) {
#line 673
                assert(_m_pController != NULL);
                _m_pController->onEditCursorTilePosChanged(this, _m_cursorTilePos = newCursorTilePos);
            }
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
        gtk_adjustment_set_value(_m_hAdjust, _m_viewPos.x);
        gtk_adjustment_set_value(_m_vAdjust, _m_viewPos.y);
        OnPaint();
    }
}

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
        InvalidateRect(&invalidRect, false);
}

void TMapEditWnd::incZoom()
{
    if (_m_zoom > 0)
        setZoom(TZoom(_m_zoom - 1));
}

void TMapEditWnd::decZoom()
{
    if (_m_zoom + 1 < kNumZooms)
        setZoom(TZoom(_m_zoom + 1));
}

void TMapEditWnd::setZoom(TZoom newZoom)
{
#line 741
    assert(newZoom >= 0 && newZoom < kNumZooms);
    if (newZoom != _m_zoom) {
        CRect clientRect;
        GetClientRect(&clientRect);
        CSize oldViewRectSize = _computeViewRectSize(clientRect.Width(), clientRect.Height());
        _m_zoom = newZoom;
        CSize viewRectSize = _computeViewRectSize(clientRect.Width(), clientRect.Height());
        CPoint maxPos(_m_pMap->getWidth() - viewRectSize.cx, _m_pMap->getHeight() - viewRectSize.cy);
        CPoint newPos(0, 0);
        _m_hAdjust->lower = 0;
        _m_hAdjust->upper = _m_pMap->getWidth();
        _m_hAdjust->page_size = viewRectSize.cx;
        _m_hAdjust->value = newPos.x;
        gtk_adjustment_changed(_m_hAdjust);
        gtk_adjustment_value_changed(_m_hAdjust);
        _m_vAdjust->lower = 0;
        _m_vAdjust->upper = _m_pMap->getHeight();
        _m_vAdjust->page_size = viewRectSize.cy;
        _m_vAdjust->value = newPos.y;
        gtk_adjustment_changed(_m_vAdjust);
        gtk_adjustment_value_changed(_m_vAdjust);
        if (newPos != _m_viewPos) {
            _m_viewPos = newPos;
            _m_pController->onMoveMapViewRect(this, newPos);
        }
        _m_pController->onSizeMapViewRect(this, viewRectSize);
        OnPaint();
    }
}

void TMapEditWnd::showGrid(bool bShow)
{
    if (bShow != _m_bShowGrid) {
        _m_bShowGrid = bShow;
        Invalidate(false);
    }
}

void TMapEditWnd::showPassability(bool bShow)
{
    if (bShow != _m_bShowPassability) {
        _m_bShowPassability = bShow;
        Invalidate(false);
    }
}

void TMapEditWnd::selectObject(unsigned int objID)
{
    if (_m_mode == _eModeSel && objID != _m_selectedObjID) {
        GdkGC* gc = gdk_gc_new(_m_hWnd->window);
        if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID)
            _drawSelectionFrame(_m_hWnd->window, gc, false);
        _m_selectedObjID = objID;
        if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID)
            _drawSelectionFrame(_m_hWnd->window, gc, false);
        gdk_gc_unref(gc);
#line 845
        assert(_m_pController != NULL);
        _m_pController->onEditObjectSelected(this, _m_selectedObjID);
    }
}

void TMapEditWnd::grabObject(const TGUIGameObject& obj)
{
    if (_m_pFloatingObj != NULL) {
        delete _m_pFloatingObj;
        _m_pFloatingObj = NULL;
    }
    if (GetCapture() == this)
        ReleaseCapture();
#line 862
    assert(_m_mode == _eModeSel);
    dropUnexpectedFloater();
    assert(GetCapture() == NULL);
    if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
        _clearToolTipObj();
    _m_pFloatingObj = &obj;
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
    GdkCursor* pCursor = floatingObjRect.right > viewRect.left && floatingObjRect.bottom > viewRect.top
                                 && floatingObjRect.left < viewRect.right
                                 && floatingObjRect.top < viewRect.bottom
                                 && !_m_pMap->isValidPlacement(obj, _m_bSecondLayer, _m_floatingObjPos.x,
                                                               _m_floatingObjPos.y)
                             ? khNoCursor
                             : hClosedHandCursor;
    _setCursor(pCursor);
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    floatingObjRect.left = (floatingObjRect.left - _m_viewPos.x) * tileSize;
    floatingObjRect.top = (floatingObjRect.top - _m_viewPos.y) * tileSize;
    floatingObjRect.right = (floatingObjRect.right - _m_viewPos.x) * tileSize;
    floatingObjRect.bottom = (floatingObjRect.bottom - _m_viewPos.y) * tileSize;
    CRect paintRect;
    if (paintRect.IntersectRect(&floatingObjRect, &clientRect))
        _paintRect(paintRect);
}

void TMapEditWnd::onUndo()
{
    if (_m_mode == _eModeSel && _m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID) {
        _m_selectedObjID = TGameMap::TLayer::s_kInvalidObjID;
#line 915
        assert(_m_pController != NULL);
        _m_pController->onEditObjectSelected(this, _m_selectedObjID);
    }
    CRect rect(0, 0, _m_pMap->getWidth(), _m_pMap->getHeight());
    update(rect);
}

void TMapEditWnd::onObjectRemoved(unsigned int removedObjID)
{
#line 926
    assert(removedObjID != TGameMap::TLayer::s_kInvalidObjID);
    if (_m_selectedObjID == removedObjID) {
        _m_selectedObjID = TGameMap::TLayer::s_kInvalidObjID;
#line 932
        assert(_m_pController != NULL);
        _m_pController->onEditObjectSelected(this, _m_selectedObjID);
    }
    if (_m_toolTipObjID == removedObjID)
        _clearToolTipObj();
}

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
    if (frameNum != 0) {
        _m_frameNum += frameNum;
        for (TGameMap::TLayer::TObjectIDIter iter = layer.objectIDBegin(); iter != layer.objectIDEnd();
             ++iter) {
            TTileExtent extent = layer.getObjectExtent(*iter);
            if (!(extent.left() < cells.right && extent.top() < cells.bottom
                  && extent.right() > cells.left && extent.bottom() > cells.top))
                continue;
            const TGUIGameObject* pObject = dynamic_cast<const TGUIGameObject*>(layer.getPObject(*iter));
#line 981
            assert(pObject != NULL);
            if (!pObject->isAnimated())
                continue;
            CRect objRect((extent.left() - _m_viewPos.x) * tileSize, (extent.top() - _m_viewPos.y) * tileSize,
                          (extent.right() - _m_viewPos.x) * tileSize,
                          (extent.bottom() - _m_viewPos.y) * tileSize);
            CRect invalidRect;
            if (invalidRect.IntersectRect(&objRect, &clientRect))
                InvalidateRect(&invalidRect, false);
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
                    TRiverType riverType = mapCell.getRiverType();
                    if (!(riverType != 0 && akRiverTilesetTraits[riverType - 1].m_bAnimated))
                        bAnimated = false;
                }
                if (bAnimated) {
                    if (!bInRun) {
                        runStart = cell.x;
                        bInRun = true;
                    }
                } else if (bInRun) {
                    CPoint topLeft((runStart - _m_viewPos.x) * tileSize, y);
                    CSize size((cell.x - runStart) * tileSize, tileSize);
                    CRect rect(topLeft, size);
                    InvalidateRect(&rect, false);
                    bInRun = false;
                }
            }
            if (bInRun) {
                CPoint topLeft((runStart - _m_viewPos.x) * tileSize, y);
                CSize size((cell.x - runStart) * tileSize, tileSize);
                CRect rect(topLeft, size);
                InvalidateRect(&rect, false);
            }
            y += tileSize;
        }
    }
}

void TMapEditWnd::resetAnimation()
{
    _m_frameNum = 0;
    Invalidate(false);
}

void TMapEditWnd::makeVisible(unsigned int objID)
{
#line 1105
    assert(objID != TGameMap::TLayer::s_kInvalidObjID);
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

void TMapEditWnd::selectionMode()
{
    _setMode(_eModeSel);
}

void TMapEditWnd::brushMode(const CSize& size)
{
    _setMode(_eModeBrush);
    _setBrushSize(size);
}

void TMapEditWnd::brushMode()
{
    _setMode(_eModeBrush);
}

void TMapEditWnd::fillMode()
{
    _setMode(_eModeFill);
}

CSize TMapEditWnd::getMinWndSize() const
{
    return CSize(100, 100);
}

void TMapEditWnd::_turnAutoScrollOn()
{
#line 1173
    assert(!_m_bAutoScrollOn);
    if (SetTimer(kAutoScrollTimer, kAutoScrollPeriod, NULL)) {
        GetCursorPos(&_m_autoScrollPoint);
        ScreenToClient(&_m_autoScrollPoint);
        _m_autoScrollTime = getCurrentTime();
        _m_hAutoScrollDir = 0;
        _m_vAutoScrollDir = 0;
        _m_bAutoScrollOn = true;
    }
}

void TMapEditWnd::_turnAutoScrollOff()
{
#line 1192
    assert(_m_bAutoScrollOn);
    gtk_timeout_remove(_m_timers[kAutoScrollTimer]);
    _m_bAutoScrollOn = false;
}

void TMapEditWnd::_handleAutoScroll(const CPoint& point)
{
#line 1202
    assert(_m_bAutoScrollOn);
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
    unsigned int time = getCurrentTime();
    unsigned int dx = 0;
    unsigned int dy = 0;
    int oldHDir = _m_hAutoScrollDir;
    _m_hAutoScrollDir = dir.x;
    if (_m_hAutoScrollDir != 0) {
        int dist = dir.x < 0 ? -dir.x : dir.x;
        unsigned int oldInterval = _m_hAutoScrollInterval;
        _m_hAutoScrollInterval = clamp(30u, unsigned(1440000 / (dist * dist)), 1200u);
        if (oldHDir == 0 || (oldHDir < 0 && _m_hAutoScrollDir > 0) || (oldHDir > 0 && _m_hAutoScrollDir < 0))
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
        if (oldVDir == 0 || (oldVDir < 0 && _m_vAutoScrollDir > 0) || (oldVDir > 0 && _m_vAutoScrollDir < 0))
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
    onPan(_m_hAutoScrollDir < 0 ? -dx : dx, _m_vAutoScrollDir < 0 ? -dy : dy);
    _generateMouseMove();
}

void TMapEditWnd::_setMode(_TMode newMode)
{
#line 1303
    assert(newMode >= 0 && newMode < _s_kNumModes);
    if (newMode == _m_mode)
        return;
    switch (_m_mode) {
    case _eModeSel:
        dropUnexpectedFloater();
#line 1314
        assert(_m_potentialGrabID == TGameMap::TLayer::s_kInvalidObjID);
        if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID)
            selectObject(TGameMap::TLayer::s_kInvalidObjID);
        if (_m_toolTipObjID != TGameMap::TLayer::s_kInvalidObjID)
            _clearToolTipObj();
        break;
    case _eModeBrush:
#line 1322
        assert(!_m_bDragging);
        if (_m_bBrushOn)
            _turnBrushOff();
        break;
    case _eModeFill:
#line 1328
        assert(!_m_bDragging);
        if (_m_bBrushOn)
            _turnFillRectOff();
        break;
    }
    _m_mode = newMode;
    switch (_m_mode) {
    case _eModeBrush:
        _turnBrushOn();
        break;
    }
}

void TMapEditWnd::_setCursor(GdkCursor* pCursor)
{
    gdk_window_set_cursor(_m_hWnd->window, pCursor);
}

void TMapEditWnd::_setToolTipObj(unsigned int objID)
{
    g_warning("TMapEditWnd::_setToolTipObj(): stub.\n");
}

void TMapEditWnd::_clearToolTipObj()
{
    g_print("TMapEditWnd::_clearToolTipObj() not implemented.\n");
}

void TMapEditWnd::_turnBrushOn()
{
#line 1403
    assert(_m_mode == _eModeBrush);
    if (_m_bBrushOn)
        return;
    _m_bBrushOn = true;
    GdkGC* gc;
    if (_m_hWnd->window != NULL && (gc = gdk_gc_new(_m_hWnd->window)) != NULL) {
        _drawBrush(_m_hWnd->window, gc);
        gdk_gc_unref(gc);
    }
}

void TMapEditWnd::_turnBrushOff()
{
#line 1429
    assert(_m_mode == _eModeBrush);
    if (!_m_bBrushOn)
        return;
    GdkGC* gc;
    if (_m_hWnd->window != NULL && (gc = gdk_gc_new(_m_hWnd->window)) != NULL) {
        _drawBrush(_m_hWnd->window, gc);
        gdk_gc_unref(gc);
    }
    _m_bBrushOn = false;
}

void TMapEditWnd::_setBrushPos(const CPoint& pos)
{
#line 1453
    assert(_m_mode == _eModeBrush);
    if (_m_bBrushOn) {
        GdkGC* gc = gdk_gc_new(_m_hWnd->window);
        _drawBrush(_m_hWnd->window, gc);
        _m_brushPos = pos;
        _drawBrush(_m_hWnd->window, gc);
        gdk_gc_unref(gc);
    } else
        _m_brushPos = pos;
}

void TMapEditWnd::_setBrushSize(const CSize& brushSize)
{
#line 1473
    assert(_m_mode == _eModeBrush);
    assert(brushSize.cx > 0 && brushSize.cy > 0);
    if (_m_bBrushOn && _m_hWnd->window != NULL) {
        GdkGC* gc = gdk_gc_new(_m_hWnd->window);
        if (gc != NULL) {
            _drawBrush(_m_hWnd->window, gc);
            _m_brushSize = brushSize;
            _drawBrush(_m_hWnd->window, gc);
            gdk_gc_unref(gc);
        }
    }
    _m_brushSize = brushSize;
}

void TMapEditWnd::_turnFillRectOff()
{
#line 1501
    assert(_m_mode == _eModeFill);
    assert(_m_bBrushOn);
    GdkGC* gc = gdk_gc_new(_m_hWnd->window);
    _drawBrush(_m_hWnd->window, gc);
    gdk_gc_unref(gc);
    _m_bBrushOn = false;
}

void TMapEditWnd::_setFillRectAnchor(const CPoint& pos)
{
#line 1515
    assert(_m_mode == _eModeFill);
    assert(!_m_bBrushOn);
    _m_bBrushOn = true;
    _m_fillRectAnchor = pos;
    _m_fillRectDragPos = pos;
    GdkGC* gc = gdk_gc_new(_m_hWnd->window);
    _drawBrush(_m_hWnd->window, gc);
    gdk_gc_unref(gc);
}

void TMapEditWnd::_setFillRectDragPos(const CPoint& pos)
{
#line 1532
    assert(_m_mode == _eModeFill);
    GdkGC* gc = gdk_gc_new(_m_hWnd->window);
    if (_m_bBrushOn)
        _drawBrush(_m_hWnd->window, gc);
    _m_bBrushOn = true;
    _m_fillRectDragPos = pos;
    _drawBrush(_m_hWnd->window, gc);
    gdk_gc_unref(gc);
}

void TMapEditWnd::_drawSelectionFrame(GdkDrawable* drawable, GdkGC* gc, bool bErase)
{
#line 1553
    assert(drawable != NULL);
    assert(gc != NULL);
    assert(_m_mode == _eModeSel);
    assert(_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID);
    assert(_getMapLayer().getFloatingObjID() != _m_selectedObjID);
    const TGameMap::TLayer& layer = _getMapLayer();
    const TGameObject& obj = layer.getObject(_m_selectedObjID);
    TTilePoint loc = layer.getObjectLoc(_m_selectedObjID);
    CRect rect;
    rect.left = loc.x() + 1 - obj.getWidth() - _m_viewPos.x;
    rect.top = loc.y() + 1 - obj.getHeight() - _m_viewPos.y;
    rect.right = rect.left + obj.getWidth();
    rect.bottom = rect.top + obj.getHeight();
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    CRect frameRect(rect.left * tileSize, rect.top * tileSize, rect.right * tileSize,
                    rect.bottom * tileSize);
    GdkGCValues values;
    gdk_gc_get_values(gc, &values);
    gdk_gc_set_function(gc, GDK_XOR);
    gdk_gc_set_foreground(gc, &_m_white);
    GdkPoint points[5];
    points[0].x = frameRect.left;
    points[0].y = frameRect.top;
    points[1].x = frameRect.right - 1;
    points[1].y = frameRect.top;
    points[2].x = frameRect.right - 1;
    points[2].y = frameRect.bottom - 1;
    points[3].x = frameRect.left;
    points[3].y = frameRect.bottom - 1;
    points[4].x = frameRect.left;
    points[4].y = frameRect.top;
    gdk_draw_lines(drawable, gc, points, 5);
    gdk_gc_set_function(gc, values.function);
}

void TMapEditWnd::_drawBrush(GdkDrawable* pWindow, GdkGC* pGC)
{
#line 1617
    assert(_m_bBrushOn);
    if (pWindow == NULL || pGC == NULL)
        return;
    gdk_gc_set_foreground(pGC, &_m_white);
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    GdkGCValues values;
    gdk_gc_get_values(pGC, &values);
    gdk_gc_set_function(pGC, GDK_XOR);
    if (_m_mode == _eModeBrush) {
        CRect rect;
        rect.left = (_m_brushPos.x - _m_viewPos.x) * tileSize;
        rect.top = (_m_brushPos.y - _m_viewPos.y) * tileSize;
        rect.right = rect.left + _m_brushSize.cx * tileSize;
        rect.bottom = rect.top + _m_brushSize.cy * tileSize;
        GdkPoint points[5];
        points[0].x = rect.left;
        points[0].y = rect.top;
        points[1].x = rect.right - 1;
        points[1].y = rect.top;
        points[2].x = rect.right - 1;
        points[2].y = rect.bottom - 1;
        points[3].x = rect.left;
        points[3].y = rect.bottom - 1;
        points[4].x = rect.left;
        points[4].y = rect.top;
        gdk_draw_lines(pWindow, pGC, points, 5);
    } else {
#line 1658
        assert(_m_mode == _eModeFill);
        CRect rect = _computeFillRect();
        rect.left = (rect.left - _m_viewPos.x) * tileSize;
        rect.top = (rect.top - _m_viewPos.y) * tileSize;
        rect.right = (rect.right - _m_viewPos.x) * tileSize;
        rect.bottom = (rect.bottom - _m_viewPos.y) * tileSize;
        GdkPoint points[5];
        points[0].x = rect.left;
        points[0].y = rect.top;
        points[1].x = rect.right - 1;
        points[1].y = rect.top;
        points[2].x = rect.right - 1;
        points[2].y = rect.bottom - 1;
        points[3].x = rect.left;
        points[3].y = rect.bottom - 1;
        points[4].x = rect.left;
        points[4].y = rect.top;
        gdk_draw_lines(pWindow, pGC, points, 5);
    }
    gdk_gc_set_function(pGC, values.function);
}

void TMapEditWnd::_paintRect(CRect& rect)
{
    int width = _m_hWnd->allocation.width;
    int height = _m_hWnd->allocation.height;
    if (width % 2)
        width--;
    if (rect.left < 0)
        rect.left = 0;
    if (rect.left > width)
        rect.left = width;
    if (rect.top < 0)
        rect.top = 0;
    if (rect.top > height)
        rect.top = height;
    if (rect.right < 0)
        rect.right = 0;
    if (rect.right > width)
        rect.right = width;
    if (rect.bottom < 0)
        rect.bottom = 0;
    if (rect.bottom > height)
        rect.bottom = height;
    GdkGC* gc = gdk_gc_new(_m_hWnd->window);
    GdkRectangle clipRect;
    CRect::CRect_to_gdk_rectangle(&rect, &clipRect);
    gdk_gc_set_clip_origin(gc, 0, 0);
    gdk_gc_set_clip_rectangle(gc, &clipRect);
    _drawMap(gc, rect);
    if (_m_mode == _eModeSel && _m_pFloatingObj != NULL) {
        CRect objRect;
        unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
        CRect floatingObjRect = _computeFloatingObjRect(_m_floatingObjPos);
        floatingObjRect.left = (floatingObjRect.left - _m_viewPos.x) * tileSize;
        floatingObjRect.right = (floatingObjRect.right - _m_viewPos.x) * tileSize;
        floatingObjRect.top = (floatingObjRect.top - _m_viewPos.y) * tileSize;
        floatingObjRect.bottom = (floatingObjRect.bottom - _m_viewPos.y) * tileSize;
        if (objRect.IntersectRect(&floatingObjRect, &rect)) {
            int x = (_m_floatingObjPos.x + 1 - _m_viewPos.x) * tileSize - 1;
            int y = (_m_floatingObjPos.y + 1 - _m_viewPos.y) * tileSize - 1;
            _m_pFloatingObj->draw(0, gc, _m_pImage, x, y, _m_zoom);
        }
    }
    GdkVisual* pVisual = gdk_visual_get_system();
    if (pVisual->depth == 24) {
        GdkImage* pImage = gdk_image_new(GDK_IMAGE_FASTEST, gdk_visual_get_system(), _m_pImage->width,
                                         _m_pImage->height);
        if (pImage != NULL) {
            int x;
            int y;
            uword* pSrc = (uword*)_m_pImage->mem;
            guint32* pDst = (guint32*)pImage->mem;
            for (y = 0; y < height; y++)
                for (x = 0; x < width; x++) {
                    unsigned int pixel = pSrc[y * width + x];
                    pDst[y * width + x] = ((pixel & 0xf800) << 8) + ((pixel & 0x7e0) << 5)
                                          + ((pixel & 0x1f) << 3);
                }
            gdk_draw_image(_m_hWnd->window, gc, pImage, rect.left, rect.top, rect.left, rect.top,
                           rect.Width(), rect.Height());
            gdk_image_destroy(pImage);
        }
    } else
        gdk_draw_image(_m_hWnd->window, gc, _m_pImage, rect.left, rect.top, rect.left, rect.top,
                       rect.Width(), rect.Height());
    if (_m_mode == _eModeSel) {
        if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID
            && _getMapLayer().getFloatingObjID() != _m_selectedObjID)
            _drawSelectionFrame(_m_hWnd->window, gc, false);
    } else if ((_m_mode == _eModeBrush || _m_mode == _eModeFill) && _m_bBrushOn)
        _drawBrush(_m_hWnd->window, gc);
    gdk_gc_unref(gc);
}

void TMapEditWnd::_drawMap(GdkGC* pGC, CRect& rect)
{
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    if (rect.right > _m_hWnd->allocation.width)
        rect.right = _m_hWnd->allocation.width;
    if (rect.bottom > _m_hWnd->allocation.height)
        rect.bottom = _m_hWnd->allocation.height;
    if (rect.Width() % 2)
        rect.right--;
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
    int imageWidth = _m_hWnd->allocation.width;
    if (imageWidth % 2)
        imageWidth--;
    CPoint cell;
    for (cell.y = cells.top; cell.y < cells.bottom; cell.y++)
        for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
            CPoint pos((cell.x - _m_viewPos.x) * tileSize - rect.left,
                       (cell.y - _m_viewPos.y) * tileSize - rect.top);
            const TGameMap::TLayer::TCell& mapCell = layer.getCell(cell.x, cell.y);
            (*akZoomTraits[_m_zoom].m_pDrawTile)(akGroundTilesetTraits[mapCell.getTerrainType()].m_pSprite,
                                              mapCell.getTileNum(), pos.x, pos.y,
                                              (uword*)getImgLine(_m_pImage, rect.top) + rect.left,
                                              rect.Width(), rect.Height(), getImgPitch(_m_pImage),
                                              mapCell.getBHFlipped(), mapCell.getBVFlipped());
        }
    for (cell.y = cells.top; cell.y < cells.bottom; cell.y++)
        for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
            CPoint pos((cell.x - _m_viewPos.x) * tileSize - rect.left,
                       (cell.y - _m_viewPos.y) * tileSize - rect.top);
            const TGameMap::TLayer::TCell& mapCell = layer.getCell(cell.x, cell.y);
            if (mapCell.getRiverType() != TRiverType(0))
                (*akZoomTraits[_m_zoom].m_pDrawTile)(akRiverTilesetTraits[mapCell.getRiverType() - 1].m_pSprite,
                                                  mapCell.getRiverTileNum(), pos.x, pos.y,
                                                  (uword*)getImgLine(_m_pImage, rect.top) + rect.left,
                                                  rect.Width(), rect.Height(), getImgPitch(_m_pImage),
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
            if (mapCell.getRoadType() != TRoadType(0))
                (*akZoomTraits[_m_zoom].m_pDrawTile)(akRoadTilesetTraits[mapCell.getRoadType() - 1].m_pSprite,
                                                  mapCell.getRoadTileNum(), pos.x, pos.y,
                                                  (uword*)getImgLine(_m_pImage, rect.top) + rect.left,
                                                  rect.Width(), rect.Height(), getImgPitch(_m_pImage),
                                                  mapCell.getBRoadHFlipped(), mapCell.getBRoadVFlipped());
        }
    for (cell.y = cells.top; cell.y < cells.bottom; cell.y++)
        for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
            CPoint pos((cell.x - _m_viewPos.x + 1) * tileSize - 1, (cell.y - _m_viewPos.y + 1) * tileSize - 1);
            unsigned int numIDs = layer.getNumObjectIDsAtCell(cell.x, cell.y);
            for (unsigned int i = 0; i < numIDs; i++) {
                TMapLayerObjectID objID = layer.getObjectIDAtCell(cell.x, cell.y, i);
                const TGUIGameObject* pObject = dynamic_cast<const TGUIGameObject*>(layer.getPObject(objID));
#line 1980
                assert(pObject != NULL);
                if (pObject->getBUnderlay()) {
                    TTilePoint loc = layer.getObjectLoc(objID);
                    pObject->drawCell(_m_frameNum, loc.x() - cell.x, loc.y() - cell.y, pGC, _m_pImage,
                                      pos.x, pos.y, _m_zoom);
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
#line 2025
                assert(pObject != NULL);
                TTilePoint loc = layer.getObjectLoc(objID);
                pObject->drawCellShadow(_m_frameNum, loc.x() - cell.x, loc.y() - cell.y, pGC, _m_pImage,
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
#line 2060
                assert(pObject != NULL);
                if (!pObject->getBUnderlay()) {
                    TTilePoint loc = layer.getObjectLoc(objID);
                    pObject->drawCell(_m_frameNum, loc.x() - cell.x, loc.y() - cell.y, pGC, _m_pImage,
                                      pos.x, pos.y, _m_zoom);
                }
            }
        }
    if (_m_bShowPassability)
        for (cell.y = cells.top; cell.y < cells.bottom; cell.y++)
            for (cell.x = cells.left; cell.x < cells.right; cell.x++) {
                CPoint pos((cell.x - _m_viewPos.x) * tileSize, (cell.y - _m_viewPos.y) * tileSize);
                unsigned int numIDs = layer.getNumObjectIDsAtCell(cell.x, cell.y);
                for (unsigned int i = 0; i < numIDs; i++) {
                    TMapLayerObjectID objID = layer.getObjectIDAtCell(cell.x, cell.y, i);
                    const TGameObject& obj = layer.getObject(objID);
                    TTilePoint loc = layer.getObjectLoc(objID);
                    int cellX = loc.x() - cell.x;
                    int cellY = loc.y() - cell.y;
                    if (obj.getBCellTrigger(cellX, cellY)) {
                        drawCellHatchedRect(pos, 0xffe0, _m_zoom, _m_pImage);
                        break;
                    } else if (!obj.getBCellPassable(cellX, cellY)) {
                        drawCellHatchedRect(pos, 0xf800, _m_zoom, _m_pImage);
                        break;
                    }
                }
            }
    if (_m_bShowGrid) {
        for (unsigned int x = cells.left, xPos = (x - _m_viewPos.x) * tileSize; x < cells.right;
             x++, xPos += tileSize) {
            drawVLine(xPos, 0, _m_pImage);
            drawVLine(xPos + tileSize - 1, 0, _m_pImage);
        }
        for (unsigned int y = cells.top, yPos = (y - _m_viewPos.y) * tileSize; y < cells.bottom;
             y++, yPos += tileSize) {
            drawHLine(yPos, 0, _m_pImage);
            drawHLine(yPos + tileSize - 1, 0, _m_pImage);
        }
    }
    if ((rect.right + tileSize - 1) / tileSize + _m_viewPos.x > width) {
        CRect outside = rect;
        outside.left = (width - _m_viewPos.x) * tileSize;
        fillRect(outside, 0, _m_pImage);
    }
    if ((rect.bottom + tileSize - 1) / tileSize + _m_viewPos.y > height) {
        CRect outside = rect;
        outside.top = (height - _m_viewPos.y) * tileSize;
        fillRect(outside, 0, _m_pImage);
    }
}

void TMapEditWnd::_generateMouseMove()
{
    CPoint point;
    if (GetCursorPos(&point)) {
        ScreenToClient(&point);
        if (GetCapture() == this)
            OnMouseMove(1, point);
    }
}

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

CPoint TMapEditWnd::_computeFloatingObjPos(const CPoint& point) const
{
#line 2210
    assert(_m_pFloatingObj != NULL);
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    return CPoint((point.x + ((_m_pFloatingObj->getWidth() - 1) * tileSize >> 1)) / tileSize + _m_viewPos.x,
                  (point.y + ((_m_pFloatingObj->getHeight() - 1) * tileSize >> 1)) / tileSize + _m_viewPos.y);
}

CRect TMapEditWnd::_computeFloatingObjRect(const CPoint& point) const
{
#line 2221
    assert(_m_pFloatingObj != NULL);
    CPoint topLeft(point.x + 1 - _m_pFloatingObj->getWidth(), point.y + 1 - _m_pFloatingObj->getHeight());
    CSize size(_m_pFloatingObj->getWidth(), _m_pFloatingObj->getHeight());
    return CRect(topLeft, size);
}

CSize TMapEditWnd::_computeViewRectSize(int cx, int cy) const
{
#line 2231
    assert(_m_pMap != NULL);
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    return CSize(clamp(1, int(cx / tileSize), int(_m_pMap->getWidth())),
                 clamp(1, int(cy / tileSize), int(_m_pMap->getHeight())));
}

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

CPoint TMapEditWnd::_computeFillRectDragPos(const CPoint& point) const
{
    int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    CPoint pos(point.x / tileSize + _m_viewPos.x, point.y / tileSize + _m_viewPos.y);
#line 2301
    assert(_m_pMap != NULL);
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

bool TMapEditWnd::getBDoHScroll() const
{
#line 2328
    assert(_m_hAdjust != NULL);
    return _m_hAdjust->page_size <= _m_hAdjust->upper - _m_hAdjust->lower;
}

bool TMapEditWnd::getBDoVScroll() const
{
#line 2342
    assert(_m_vAdjust != NULL);
    return _m_vAdjust->page_size <= _m_vAdjust->upper - _m_vAdjust->lower;
}

void TMapEditWnd::onPan(int dx, int dy)
{
#line 2349
    assert(_m_hAdjust != NULL);
    assert(_m_vAdjust != NULL);
    int hPageSize = (int)_m_hAdjust->page_size;
    int vPageSize = (int)_m_vAdjust->page_size;
    CPoint newPos = _m_viewPos + CPoint(dx, dy);
    newPos.x = clamp<int>(0, newPos.x, int(_m_pMap->getWidth() - hPageSize));
    newPos.y = clamp<int>(0, newPos.y, int(_m_pMap->getHeight() - vPageSize));
    if (newPos != _m_viewPos)
        _m_pController->onMoveMapViewRect(this, newPos);
}

void TMapEditWnd::OnSize(unsigned int type, int cx, int cy)
{
    if (cx == 0 || cy == 0 || _m_hWnd == NULL || _m_hWnd->window == NULL)
        return;
    if (type != 1) {
        if (_m_pImage != NULL)
            gdk_image_destroy(_m_pImage);
        unsigned int imageWidth = cx;
        if (imageWidth % 2)
            imageWidth++;
        _m_pImage = gdk_image_new(GDK_IMAGE_FASTEST, gdk_visual_get_system(), imageWidth, cy);
    }
#line 2445
    assert(_m_pMap != NULL);
    CSize viewRectSize = _computeViewRectSize(cx, cy);
    CPoint maxPos(_m_pMap->getWidth() - viewRectSize.cx, _m_pMap->getHeight() - viewRectSize.cy);
    if (type != 1) {
        _m_hAdjust->lower = 0;
        _m_hAdjust->upper = _m_pMap->getWidth();
        _m_hAdjust->page_size = viewRectSize.cx;
        gtk_adjustment_changed(_m_hAdjust);
        _m_vAdjust->lower = 0;
        _m_vAdjust->upper = _m_pMap->getHeight();
        _m_vAdjust->page_size = viewRectSize.cy;
        gtk_adjustment_changed(_m_vAdjust);
        Invalidate(false);
    }
#line 2479
    assert(_m_pController != NULL);
    if (_m_viewPos.x > maxPos.x || _m_viewPos.y > maxPos.y) {
        CPoint newPos(_m_viewPos.x < maxPos.x ? _m_viewPos.x : maxPos.x,
                      _m_viewPos.y < maxPos.y ? _m_viewPos.y : maxPos.y);
        _m_pController->onMoveMapViewRect(this, newPos);
    }
    _m_pController->onSizeMapViewRect(this, viewRectSize);
}

void TMapEditWnd::OnHScroll(unsigned int code)
{
#line 2493
    assert(_m_pMap != NULL);
    assert(_m_hAdjust != NULL);
    unsigned int pageSize = (unsigned int)_m_hAdjust->page_size;
    unsigned int thumbPos = (unsigned int)_m_hAdjust->value;
    unsigned int newPos = _m_viewPos.x;
    switch (code) {
    case SB_LINELEFT:
        if ((int)newPos > 0)
            newPos--;
        break;
    case SB_LINERIGHT:
        if (newPos < _m_pMap->getWidth() - pageSize)
            newPos++;
        break;
    case SB_PAGELEFT:
        newPos -= pageSize;
        if ((int)newPos < 0)
            newPos = 0;
        break;
    case SB_PAGERIGHT:
        newPos += pageSize;
        if (newPos > _m_pMap->getWidth() - pageSize)
            newPos = _m_pMap->getWidth() - pageSize;
        break;
    case SB_LEFT:
        newPos = 0;
        break;
    case SB_RIGHT:
        newPos = _m_pMap->getWidth() - pageSize;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        newPos = thumbPos;
        break;
    }
    if (newPos != _m_viewPos.x) {
#line 2543
        assert(_m_pController != NULL);
        _m_pController->onMoveMapViewRect(this, CPoint(newPos, _m_viewPos.y));
    }
}

void TMapEditWnd::OnVScroll(unsigned int code)
{
#line 2551
    assert(_m_pMap != NULL);
    assert(_m_vAdjust != NULL);
    unsigned int pageSize = (unsigned int)_m_vAdjust->page_size;
    unsigned int thumbPos = (unsigned int)_m_vAdjust->value;
    unsigned int newPos = _m_viewPos.y;
    switch (code) {
    case SB_LINEUP:
        if ((int)newPos > 0)
            newPos--;
        break;
    case SB_LINEDOWN:
        if (newPos < _m_pMap->getHeight() - pageSize)
            newPos++;
        break;
    case SB_PAGEUP:
        newPos -= pageSize;
        if ((int)newPos < 0)
            newPos = 0;
        break;
    case SB_PAGEDOWN:
        newPos += pageSize;
        if (newPos > _m_pMap->getHeight() - pageSize)
            newPos = _m_pMap->getHeight() - pageSize;
        break;
    case SB_TOP:
        newPos = 0;
        break;
    case SB_BOTTOM:
        newPos = _m_pMap->getHeight() - pageSize;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        newPos = thumbPos;
        break;
    }
    if (newPos != _m_viewPos.y) {
#line 2601
        assert(_m_pController != NULL);
        _m_pController->onMoveMapViewRect(this, CPoint(_m_viewPos.x, newPos));
    }
}

void TMapEditWnd::OnPaint()
{
    if (_m_hWnd != NULL) {
        CRect rect(0, 0, _m_hWnd->allocation.width, _m_hWnd->allocation.height);
        OnPaint(rect);
    }
}

void TMapEditWnd::OnPaint(CRect& rect)
{
    if (_m_pMap == NULL || _m_pImage == NULL)
        return;
    if (rect.left > 0)
        rect.left--;
    rect.right++;
    _paintRect(rect);
}

void TMapEditWnd::OnMouseEnter()
{
#line 2667
    assert(_m_hWnd != NULL);
    _m_bMouseInside = true;
    if (_m_bAutoScrollOn)
        _turnAutoScrollOff();
    if (_m_mode == _eModeBrush)
        _turnBrushOn();
}

void TMapEditWnd::OnMouseLeave()
{
    _m_bMouseInside = false;
    if (_m_bDragging && !_m_bAutoScrollOn)
        _turnAutoScrollOn();
    if (_m_mode == _eModeBrush)
        _turnBrushOff();
}

void TMapEditWnd::OnMouseMove(unsigned int flags, CPoint point)
{
    if (_m_bAutoScrollOn && flags == 0)
        return;
    if (!button2Down())
        _m_bPanning = false;
    else if (_m_bPanning == true) {
        if (point.x < _m_panPoint.x)
            OnHScroll(SB_LINELEFT);
        else if (point.x > _m_panPoint.x)
            OnHScroll(SB_LINERIGHT);
        if (point.y < _m_panPoint.y)
            OnVScroll(SB_LINEUP);
        else if (point.y > _m_panPoint.y)
            OnVScroll(SB_LINEDOWN);
        _m_panPoint = point;
        return;
    }
    if (_m_mode == _eModeSel && _m_pFloatingObj != NULL) {
        int x;
        int y;
        gdk_window_get_pointer(_m_hWnd->window, &x, &y, NULL);
        if (!(x == point.x && y == point.y))
            return;
    }
    int width = _m_hWnd->allocation.width;
    int height = _m_hWnd->allocation.height;
    if (width % 2)
        width--;
    CRect clientRect;
    GetClientRect(&clientRect);
    unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
    if (_m_bMouseInside) {
        CPoint cursorTilePos(point.x / tileSize + _m_viewPos.x, point.y / tileSize + _m_viewPos.y);
        if (cursorTilePos != _m_cursorTilePos) {
#line 2741
            assert(_m_pController != NULL);
            _m_pController->onEditCursorTilePosChanged(this, _m_cursorTilePos = cursorTilePos);
        }
    }
    switch (_m_mode) {
    case _eModeSel:
        if (_m_pFloatingObj != NULL) {
#line 2751
            assert(GetCapture() == this);
            CPoint newPos = _computeFloatingObjPos(point);
            if (newPos != _m_floatingObjPos) {
                CSize viewSize = _computeViewRectSize(clientRect.Width(), clientRect.Height());
                CRect viewRect(_m_viewPos, viewSize);
                CRect oldRect = _computeFloatingObjRect(_m_floatingObjPos);
                CRect newRect = _computeFloatingObjRect(newPos);
                _m_floatingObjPos = newPos;
                GdkCursor* pCursor = newRect.right > viewRect.left && newRect.bottom > viewRect.top
                                             && newRect.left < viewRect.right && newRect.top < viewRect.bottom
                                             && !_m_pMap->isValidPlacement(*_m_pFloatingObj, _m_bSecondLayer,
                                                                           _m_floatingObjPos.x,
                                                                           _m_floatingObjPos.y)
                                         ? khNoCursor
                                         : hClosedHandCursor;
                _setCursor(pCursor);
                unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
                CRect clipRect;
                CRect updateRect;
                updateRect.UnionRect(&oldRect, &newRect);
                updateRect.left = (updateRect.left - _m_viewPos.x) * tileSize;
                updateRect.top = (updateRect.top - _m_viewPos.y) * tileSize;
                updateRect.right = (updateRect.right - _m_viewPos.x) * tileSize;
                updateRect.bottom = (updateRect.bottom - _m_viewPos.y) * tileSize;
                if (clipRect.IntersectRect(&updateRect, &clientRect)) {
                    newRect.left = (newRect.left - _m_viewPos.x) * tileSize;
                    newRect.top = (newRect.top - _m_viewPos.y) * tileSize;
                    newRect.right = (newRect.right - _m_viewPos.x) * tileSize;
                    newRect.bottom = (newRect.bottom - _m_viewPos.y) * tileSize;
                    CRect paintRect;
                    if (paintRect.UnionRect(&newRect, &clipRect))
                        _paintRect(paintRect);
                }
            }
        } else if (_m_potentialGrabID != TGameMap::TLayer::s_kInvalidObjID) {
            bool bGrab = false;
            int dragWidth = kGrabDragSize;
            int left = _m_potentialGrabPoint.x - dragWidth / 2;
            if (point.x < left || point.x >= left + dragWidth)
                bGrab = true;
            else {
                int dragHeight = kGrabDragSize;
                int top = _m_potentialGrabPoint.y - dragHeight / 2;
                if (point.y < top || point.y >= top + dragHeight)
                    bGrab = true;
            }
            if (bGrab)
                OnTimer(kGrabTimer);
        }
        break;
    case _eModeBrush: {
        CPoint brushPos = _computeBrushPos(point);
        if (brushPos != _m_brushPos) {
            _setBrushPos(brushPos);
            if (_m_bDragging) {
#line 2852
                assert(GetCapture() == this);
                assert(_m_pController != NULL);
                _m_pController->onEditBrushDrag(this, CRect(_m_brushPos, _m_brushSize));
                int margin = tileSize * kBrushRepaintMargin;
                CRect dirtyRect(point.x - margin, point.y - margin, point.x + margin, point.y + margin);
                OnPaint(dirtyRect);
            }
        }
        break;
    }
    case _eModeFill:
        if (_m_bDragging) {
#line 2868
            assert(GetCapture() == this);
            CPoint dragPos = _computeFillRectDragPos(point);
            if (dragPos != _m_fillRectDragPos) {
                _setFillRectDragPos(dragPos);
#line 2875
                assert(_m_pController != NULL);
                _m_pController->onEditFillRectDrag(this, _computeFillRect());
            } else if (!_m_bBrushOn)
                _setFillRectDragPos(dragPos);
        }
        break;
    }
}

void TMapEditWnd::OnLButtonDown(unsigned int flags, CPoint point)
{
    if (_m_bDragging)
        return;
    switch (_m_mode) {
    case _eModeSel: {
        if (_m_pFloatingObj != NULL) {
            ReleaseCapture();
            return;
        }
        TMapLayerObjectID objID = _pickObject(point);
        selectObject(objID);
        if (objID != TGameMap::TLayer::s_kInvalidObjID) {
            unsigned int delay = 500;
            _m_potentialGrabID = objID;
            _m_bPotentialCopy = (flags & 4) != 0;
            _m_potentialGrabPoint = point;
            if (SetTimer(kGrabTimer, delay, NULL))
                SetCapture();
        }
        break;
    }
    case _eModeBrush: {
        _setBrushPos(_computeBrushPos(point));
        SetCapture();
        _m_bDragging = true;
#line 2939
        assert(_m_pController != NULL);
        _m_pController->onEditBrushBeginDrag(this, CRect(_m_brushPos, _m_brushSize));
        break;
    }
    case _eModeFill: {
#line 2945
        assert(_m_pMap != NULL);
        unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
        CPoint cell(point.x / tileSize + _m_viewPos.x, point.y / tileSize + _m_viewPos.y);
        if (!(cell.x < _m_pMap->getWidth() && cell.y < _m_pMap->getHeight()))
            break;
        _setFillRectAnchor(cell);
        SetCapture();
        _m_bDragging = true;
#line 2961
        assert(_m_pController != NULL);
        _m_pController->onEditFillRectAnchor(this, _computeFillRect());
        break;
    }
    }
}

void TMapEditWnd::OnLButtonUp(unsigned int flags, CPoint point)
{
    switch (_m_mode) {
    case _eModeSel:
        if (_m_pFloatingObj != NULL) {
#line 2976
            assert(GetCapture() == this);
            ReleaseCapture();
        } else if (_m_potentialGrabID != TGameMap::TLayer::s_kInvalidObjID) {
#line 2983
            assert(GetCapture() == this);
            ReleaseCapture();
        }
        break;
    case _eModeBrush:
        if (_m_bDragging) {
#line 2992
            assert(GetCapture() == this);
            ReleaseCapture();
        }
        break;
    case _eModeFill:
        if (_m_bDragging) {
            _m_pController->onEditFillRectEndDrag(this, _computeFillRect());
#line 3001
            assert(GetCapture() == this);
            ReleaseCapture();
        }
        break;
    }
}

void TMapEditWnd::OnLButtonDblClk(unsigned int flags, CPoint point)
{
    OnEditProperties();
}

int TMapEditWnd::OnRButtonDown(unsigned int flags, CPoint point)
{
}

void TMapEditWnd::OnRButtonUp(unsigned int flags, CPoint point)
{
    dropUnexpectedFloater();
}

void TMapEditWnd::OnMButtonDown(unsigned int flags, CPoint point)
{
    _m_bPanning = true;
    _m_panPoint = point;
}

gint TMapEditWnd::OnTimer(unsigned int id)
{
    switch (id) {
    case kGrabTimer: {
        TMapLayerObjectID grabID = _m_potentialGrabID;
        bool bCopy = _m_bPotentialCopy;
        if (GetCapture() != this) {
            gtk_timeout_remove(_m_timers[kGrabTimer]);
            return 1;
        }
        ReleaseCapture();
        if (grabID == TGameMap::TLayer::s_kInvalidObjID)
            return 1;
#line 3165
        assert(_m_pController != NULL);
        if (bCopy)
            _m_pController->onEditCopyObject(this, grabID);
        else
            _m_pController->onEditGrabObject(this, grabID);
        break;
    }
    case kAutoScrollTimer:
        if (_m_bAutoScrollOn) {
            if (!button1Down()) {
                CPoint point;
                GetCursorPos(&point);
                OnLButtonUp(0, point);
                return 1;
            }
            CPoint point;
            if (GetCursorPos(&point)) {
                ScreenToClient(&point);
                _handleAutoScroll(point);
            }
            unsigned int time = getCurrentTime();
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
            onPan(_m_hAutoScrollDir < 0 ? -dx : dx, _m_vAutoScrollDir < 0 ? -dy : dy);
            _generateMouseMove();
        }
        break;
    default:
#line 3236
        assert(false);
    }
    return 1;
}

void TMapEditWnd::OnEditProperties()
{
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID) {
#line 3248
        assert(_m_mode == _eModeSel);
        assert(_m_pController != NULL);
        _m_pController->onEditEditObject(this, _m_selectedObjID);
    }
}

void TMapEditWnd::OnEditDelete()
{
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID) {
#line 3266
        assert(_m_mode == _eModeSel);
        assert(_m_pController != NULL);
        _m_pController->onEditDeleteObject(this, _m_selectedObjID);
    }
}

void TMapEditWnd::OnLeft()
{
    OnHScroll(SB_LINELEFT);
    _generateMouseMove();
}

void TMapEditWnd::OnRight()
{
    OnHScroll(SB_LINERIGHT);
    _generateMouseMove();
}

void TMapEditWnd::OnUp()
{
    OnVScroll(SB_LINEUP);
    _generateMouseMove();
}

void TMapEditWnd::OnDown()
{
    OnVScroll(SB_LINEDOWN);
    _generateMouseMove();
}

void TMapEditWnd::OnEditCut()
{
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID) {
        OnEditCopy();
        _m_pController->onEditDeleteObject(this, _m_selectedObjID);
    }
}

void TMapEditWnd::OnEditCopy()
{
    if (_m_selectedObjID != TGameMap::TLayer::s_kInvalidObjID) {
        TGameObjectDataSource* pDataSource
            = new TGameObjectDataSource(_m_pMap->getLayer(_m_bSecondLayer).getPObject(_m_selectedObjID));
        if (pDataSource == NULL)
#line 3332
            throw TAllocationFailure(__FILE__, __LINE__);
    }
}

void TMapEditWnd::OnEditPaste()
{
    char* home = getenv("HOME");
    if (home == NULL) {
        doMessageBox("error accessing clipboard.");
        return;
    }
    size_t length = strlen(home) + 100;
    char path[length];
    snprintf(path, length, "%s/.h3mapedclipboard", home);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        doMessageBox("error accessing clipboard.");
        return;
    }
    {
        filebuf buf(fd);
        auto_ptr<TGameObject> pObject(_m_pMap->reconstructObject(&buf, _m_id, operator new));
        close(fd);
        if (pObject.get() == NULL)
#line 3378
            throw TAllocationFailure(__FILE__, __LINE__);
        CRect clientRect;
        GetClientRect(&clientRect);
        CSize viewSize = _computeViewRectSize(clientRect.Width(), clientRect.Height());
        _m_pController->onEditPasteObject(this, *pObject, CRect(_m_viewPos, viewSize));
    }
}

void TMapEditWnd::OnContextMenu(GtkWidget* pWidget, CPoint point)
{
    g_warning("TMapEditWnd::OnContextMenu() is not implemented.\n");
}

void TMapEditWnd::OnCaptureChanged(GtkWidget* pWidget)
{
    switch (_m_mode) {
    case _eModeSel:
        if (_m_pFloatingObj != NULL) {
            CRect clientRect;
            GetClientRect(&clientRect);
            CRect viewRect(_m_viewPos, _computeViewRectSize(clientRect.Width(), clientRect.Height()));
            CRect floatingObjRect = _computeFloatingObjRect(_m_floatingObjPos);
#line 3468
            assert(_m_pController != NULL);
            if (!(floatingObjRect.left < viewRect.right && floatingObjRect.top < viewRect.bottom
                  && floatingObjRect.right > viewRect.left && floatingObjRect.bottom > viewRect.top))
                _m_pController->onEditDiscardObject(this, *_m_pFloatingObj);
            else
                _m_pController->onEditPlaceObject(this, *_m_pFloatingObj, _m_floatingObjPos.x,
                                                  _m_floatingObjPos.y);
            _m_pFloatingObj = NULL;
            unsigned int tileSize = akZoomTraits[_m_zoom].m_tileSize;
            floatingObjRect.left = (floatingObjRect.left - _m_viewPos.x) * tileSize;
            floatingObjRect.top = (floatingObjRect.top - _m_viewPos.y) * tileSize;
            floatingObjRect.right = (floatingObjRect.right - _m_viewPos.x) * tileSize;
            floatingObjRect.bottom = (floatingObjRect.bottom - _m_viewPos.y) * tileSize;
            CRect paintRect;
            if (paintRect.IntersectRect(&floatingObjRect, &clientRect))
                OnPaint(floatingObjRect);
            _setCursor(khArrowCursor);
        } else if (_m_potentialGrabID != TGameMap::TLayer::s_kInvalidObjID) {
            gtk_timeout_remove(_m_timers[kGrabTimer]);
            _m_potentialGrabID = TGameMap::TLayer::s_kInvalidObjID;
        }
        break;
    case _eModeBrush:
        if (_m_bDragging) {
            if (_m_bAutoScrollOn)
                _turnAutoScrollOff();
            _m_bDragging = false;
#line 3511
            assert(_m_pController != NULL);
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
#line 3529
            assert(_m_pController != NULL);
            _m_pController->onEditFillRectEndDrag(this, _computeFillRect());
            _turnFillRectOff();
        }
        break;
    }
    OnPaint();
}

void TMapEditWnd::OnDeferredScroll(unsigned long hPos, unsigned long vPos)
{
    g_warning("TMapEditWnd::OnDeferredScroll() not implemented.\n");
}
