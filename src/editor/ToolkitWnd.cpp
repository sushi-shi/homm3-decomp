// ToolkitWnd.cpp - the toolkit window (h3maped 0x4c0f14..0x4c16f0; Loki
// h3maped object 61). It creates the five toolkits and the object palette
// as its children, takes the largest of their minimum sizes as its own and
// shows one of them at a time, sized to its client area.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/EraseToolkit.h"
#include "editor/ObjectPaletteWnd.h"
#include "editor/ObstacleToolkit.h"
#include "editor/RiverToolkit.h"
#include "editor/RoadToolkit.h"
#include "editor/TerrainToolkit.h"
#include "editor/ToolkitWnd.h"

VA(0x004c10e8, 0x464)
TToolkitWnd::TToolkitWnd(CWnd* pParent, TToolkitWndClient* pClient)
    : _m_pClient(pClient),
      _m_pShownPane(NULL)
{
    DATA_COMPGEN_GUARD(0x005a50a0, toolkitWndClassNameGuard, className)
    VA_COMPGEN(0x004c154c, 0xa, STATIC_DTOR, className)
    DATA(0x005a509c) static CString className;
    if (className.IsEmpty())
        className = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_BTNFACE + 1));
    if (!Create(className, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, CRect(0, 0, 0, 0), pParent, 0))
        throw TRuntimeError();
    _m_pTerrainToolkit = auto_ptr<TTerrainToolkit>(new TTerrainToolkit(this));
    if (_m_pTerrainToolkit.get() == NULL)
        throw TAllocationFailure();
    _m_pRiverToolkit = auto_ptr<TRiverToolkit>(new TRiverToolkit(this));
    if (_m_pRiverToolkit.get() == NULL)
        throw TAllocationFailure();
    _m_pRoadToolkit = auto_ptr<TRoadToolkit>(new TRoadToolkit(this));
    if (_m_pRoadToolkit.get() == NULL)
        throw TAllocationFailure();
    _m_pEraseToolkit = auto_ptr<TEraseToolkit>(new TEraseToolkit(this));
    if (_m_pEraseToolkit.get() == NULL)
        throw TAllocationFailure();
    _m_pObstacleToolkit = auto_ptr<TObstacleToolkit>(new TObstacleToolkit(this));
    if (_m_pObstacleToolkit.get() == NULL)
        throw TAllocationFailure();
    _m_pObjectPaletteWnd = auto_ptr<TObjectPaletteWnd>(new TObjectPaletteWnd(this, this));
    if (_m_pObjectPaletteWnd.get() == NULL)
        throw TAllocationFailure();

    _m_minSize = _m_pTerrainToolkit->getMinSize();
    CSize size = _m_pRiverToolkit->getMinSize();
    _m_minSize.cx = max(_m_minSize.cx, size.cx);
    _m_minSize.cy = max(_m_minSize.cy, size.cy);
    size = _m_pRoadToolkit->getMinSize();
    _m_minSize.cx = max(_m_minSize.cx, size.cx);
    _m_minSize.cy = max(_m_minSize.cy, size.cy);
    size = _m_pEraseToolkit->getMinSize();
    _m_minSize.cx = max(_m_minSize.cx, size.cx);
    _m_minSize.cy = max(_m_minSize.cy, size.cy);
    size = _m_pObstacleToolkit->getMinSize();
    _m_minSize.cx = max(_m_minSize.cx, size.cx);
    _m_minSize.cy = max(_m_minSize.cy, size.cy);
    size = _m_pObjectPaletteWnd->getMinSize();
    _m_minSize.cx = max(_m_minSize.cx, size.cx);
    _m_minSize.cy = max(_m_minSize.cy, size.cy);
}

VA_COMPGEN(0x004c1556, 0x1c, SCALAR_DELETING_DTOR, TToolkitWnd)

VA(0x004c1572, 0x81)
TToolkitWnd::~TToolkitWnd()
{
}

VA(0x004c15f3, 0x9)
void TToolkitWnd::showTerrainToolkit()
{
    _showPane(_m_pTerrainToolkit.get());
}

VA(0x004c15fc, 0x9)
void TToolkitWnd::showRiverToolkit()
{
    _showPane(_m_pRiverToolkit.get());
}

VA(0x004c1605, 0x9)
void TToolkitWnd::showRoadToolkit()
{
    _showPane(_m_pRoadToolkit.get());
}

VA(0x004c160e, 0x9)
void TToolkitWnd::showEraseToolkit()
{
    _showPane(_m_pEraseToolkit.get());
}

VA(0x004c1617, 0x9)
void TToolkitWnd::showObstacleToolkit()
{
    _showPane(_m_pObstacleToolkit.get());
}

VA(0x004c1620, 0x1d)
void TToolkitWnd::showObjectPalette(TObjectSlot slot)
{
    _m_pObjectPaletteWnd->setSlot(slot);
    _showPane(_m_pObjectPaletteWnd.get());
}

VA(0x004c163d, 0xf)
void TToolkitWnd::setPlayer(TPlayer player)
{
    _m_pObjectPaletteWnd->setPlayer(player);
}

// The map frame's view-rect forwarder is this body's /OPT:ICF twin, and
// onPaletteGrabObject folds into the frame's (0x46fab3).
VA(0x004c164c, 0x14)
bool TToolkitWnd::onPaletteCanCreateObject(TObjectPaletteWnd* pPaletteWnd, const TObjectType& objType)
{
    return _m_pClient->onToolkitCanCreateObject(this, objType);
}

void TToolkitWnd::onPaletteGrabObject(TObjectPaletteWnd* pPaletteWnd, const TObjectType& objType)
{
    _m_pClient->onToolkitGrabObject(this, objType);
}

VA(0x004c1660, 0x5f)
void TToolkitWnd::_showPane(CWnd* pPane)
{
    if (pPane == _m_pShownPane)
        return;
    if (_m_pShownPane != NULL)
        _m_pShownPane->ShowWindow(SW_HIDE);
    _m_pShownPane = pPane;
    if (_m_pShownPane != NULL) {
        CRect rect;
        GetWindowRect(&rect);
        CSize size = rect.Size();
        _m_pShownPane->MoveWindow(0, 0, size.cx, size.cy);
        _m_pShownPane->ShowWindow(SW_SHOW);
    }
}

VA(0x004c16bf, 0x6)
BEGIN_MESSAGE_MAP(TToolkitWnd, CWnd)
    ON_WM_SIZE()
END_MESSAGE_MAP()

VA(0x004c16c5, 0x2b)
void TToolkitWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);
    if (_m_pTerrainToolkit.get() != NULL && _m_pShownPane != NULL)
        _m_pShownPane->MoveWindow(0, 0, cx, cy);
}
