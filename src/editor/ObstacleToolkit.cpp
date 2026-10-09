// ObstacleToolkit.cpp - the obstacle toolkit (h3maped 0x492936..0x492cba;
// GOG only). The page stacks the obstacle tool bar at its label's place,
// then the brush label and the brush tool bar, 3 dialog units apart.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/MapEditorText.h"
#include "editor/ObstacleToolkit.h"
#include "editor/resource.h"

VA(0x00492952, 0x2db)
TObstacleToolkit::TObstacleToolkit(CWnd* pParent) : TToolkitBase(IDD_OBSTACLE_TOOLKIT, pParent)
{
    if (!Create(IDD_OBSTACLE_TOOLKIT, pParent))
        throw TRuntimeError();
    _m_brushStatic.SetWindowText(kBrushStr);
    _m_pToolBar = std::auto_ptr<TToolkitBaseToolBar>(new TToolkitBaseToolBar(this, IDR_OBSTACLE_TOOLBAR, 2));
    if (_m_pToolBar.get() == NULL)
        throw TAllocationFailure();
    _m_pBrushToolBar = std::auto_ptr<TToolkitBaseToolBar>(
        new TToolkitBaseToolBar(this, IDR_OBSTACLE_BRUSH_TOOLBAR, 6));
    if (_m_pBrushToolBar.get() == NULL)
        throw TAllocationFailure();
    CRect rect(0, 0, 0, 3);
    MapDialogRect(&rect);
    int margin = rect.Height();
    _m_brushStatic.GetWindowRect(&rect);
    ScreenToClient(&rect);
    int left = rect.left;
    int top = rect.top;
    int y = top;
    CSize toolBarSize = _m_pToolBar->getSize();
    _m_pToolBar->MoveWindow(left, y, toolBarSize.cx, toolBarSize.cy, FALSE);
    y += toolBarSize.cy + margin;
    int width = rect.Width();
    _m_brushStatic.MoveWindow(left, y, rect.Width(), rect.Height(), FALSE);
    if (rect.Width() > width)
        width = rect.Width();
    y += rect.Height() + margin;
    toolBarSize = _m_pBrushToolBar->getSize();
    _m_pBrushToolBar->MoveWindow(left, y, toolBarSize.cx, toolBarSize.cy, FALSE);
    if (toolBarSize.cx > width)
        width = toolBarSize.cx;
    _m_minSize.cx = width + left * 2;
    _m_minSize.cy = toolBarSize.cy + y + top;
    GetWindowRect(&rect);
    pParent->ScreenToClient(&rect);
    rect.right = rect.left + _m_minSize.cx;
    rect.bottom = rect.top + _m_minSize.cy;
    MoveWindow(&rect);
}

VA_COMPGEN(0x00492c2d, 0x1c, SCALAR_DELETING_DTOR, TObstacleToolkit)
VA_COMPGEN(0x00492c49, 0x56, IMPLICIT_DTOR, TObstacleToolkit)

VA(0x00492c9f, 0x15)
void TObstacleToolkit::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TOOLKIT_STATIC, _m_brushStatic);
}

VA(0x00492cb4, 0x6)
BEGIN_MESSAGE_MAP(TObstacleToolkit, TToolkitBase)
END_MESSAGE_MAP()
