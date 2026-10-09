// RiverToolkit.cpp - the river toolkit (h3maped 0x4b3db3..0x4b4020; Loki
// h3maped object 64). The page lays itself out as the erase toolkit
// does: the river type tool bar a margin of 3 dialog units below its label.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/RiverToolkit.h"
#include "editor/MapEditorText.h"
#include "editor/resource.h"

VA(0x004b3dcf, 0x1c9)
TRiverToolkit::TRiverToolkit(CWnd* pParent) : TToolkitBase(IDD_RIVER_TOOLKIT, pParent)
{
    if (!Create(IDD_RIVER_TOOLKIT, pParent))
        throw TRuntimeError();
    _m_riverTypeStatic.SetWindowText(kRiverTypeStr);
    _m_pToolBar = new TToolkitBaseToolBar(this, IDR_RIVER_TOOLBAR, 1);
    if (_m_pToolBar == NULL)
        throw TAllocationFailure();
    CRect rect(0, 0, 0, 3);
    MapDialogRect(&rect);
    int margin = rect.Height();
    _m_riverTypeStatic.GetWindowRect(&rect);
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

VA_COMPGEN(0x004b3f98, 0x1c, SCALAR_DELETING_DTOR, TRiverToolkit)

VA(0x004b3fb4, 0x51)
TRiverToolkit::~TRiverToolkit()
{
    delete _m_pToolBar;
}

VA(0x004b4005, 0x15)
void TRiverToolkit::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_RIVER_TYPE_STATIC, _m_riverTypeStatic);
}

VA(0x004b401a, 0x6)
BEGIN_MESSAGE_MAP(TRiverToolkit, TToolkitBase)
END_MESSAGE_MAP()
