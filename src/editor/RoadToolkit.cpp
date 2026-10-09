// RoadToolkit.cpp - the road toolkit (h3maped 0x4b43a8..0x4b4615; Loki
// h3maped object 65). The page lays itself out as the erase toolkit
// does: the road type tool bar a margin of 3 dialog units below its label.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/RoadToolkit.h"
#include "editor/MapEditorText.h"
#include "editor/resource.h"

VA(0x004b43c4, 0x1c9)
TRoadToolkit::TRoadToolkit(CWnd* pParent) : TToolkitBase(IDD_ROAD_TOOLKIT, pParent)
{
    if (!Create(IDD_ROAD_TOOLKIT, pParent))
        throw TRuntimeError();
    _m_roadTypeStatic.SetWindowText(kRoadTypeStr);
    _m_pToolBar = new TToolkitBaseToolBar(this, IDR_ROAD_TOOLBAR, 1);
    if (_m_pToolBar == NULL)
        throw TAllocationFailure();
    CRect rect(0, 0, 0, 3);
    MapDialogRect(&rect);
    int margin = rect.Height();
    _m_roadTypeStatic.GetWindowRect(&rect);
    ScreenToClient(&rect);
    int left = rect.left;
    int width = rect.Width();
    int top = rect.top;
    margin += rect.bottom;
    CSize toolBarSize = _m_pToolBar->getSize();
    _m_pToolBar->MoveWindow(left, margin, toolBarSize.cx, toolBarSize.cy, FALSE);
    if (toolBarSize.cx > width)
        width = toolBarSize.cx;
    _m_minSize.cx = width + left * 2;
    _m_minSize.cy = toolBarSize.cy + margin + top;
    GetWindowRect(&rect);
    pParent->ScreenToClient(&rect);
    rect.right = rect.left + _m_minSize.cx;
    rect.bottom = rect.top + _m_minSize.cy;
    MoveWindow(&rect);
}

VA_COMPGEN(0x004b458d, 0x1c, SCALAR_DELETING_DTOR, TRoadToolkit)

VA(0x004b45a9, 0x51)
TRoadToolkit::~TRoadToolkit()
{
    delete _m_pToolBar;
}

VA(0x004b45fa, 0x15)
void TRoadToolkit::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_ROAD_TYPE_STATIC, _m_roadTypeStatic);
}

VA(0x004b460f, 0x6)
BEGIN_MESSAGE_MAP(TRoadToolkit, TToolkitBase)
END_MESSAGE_MAP()
