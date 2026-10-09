// EraseToolkit.cpp - the erase toolkit (h3maped 0x418960..0x418be8; Loki
// h3maped object 63). The page lays itself out under its label: the brush
// tool bar a margin of 3 dialog units below it, and its own size to fit.
// Its DoDataExchange is ICF's twin of the obstacle toolkit's (0x492c9f),
// and its implicit base destructor is folded into another dialog's.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/EraseToolkit.h"
#include "editor/MapEditorText.h"
#include "editor/resource.h"

VA(0x0041897c, 0x1c9)
TEraseToolkit::TEraseToolkit(CWnd* pParent) : TToolkitBase(IDD_ERASE_TOOLKIT, pParent)
{
    if (!Create(IDD_ERASE_TOOLKIT, pParent))
        throw TRuntimeError();
    _m_brushStatic.SetWindowText(kBrushStr);
    _m_pToolBar = new TToolkitBaseToolBar(this, IDR_BRUSH_TOOLBAR, 1);
    if (_m_pToolBar == NULL)
        throw TAllocationFailure();
    CRect rect(0, 0, 0, 3);
    MapDialogRect(&rect);
    int margin = rect.Height();
    _m_brushStatic.GetWindowRect(&rect);
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

VA_COMPGEN(0x00418b70, 0x1c, SCALAR_DELETING_DTOR, TEraseToolkit)

VA(0x00418b91, 0x51)
TEraseToolkit::~TEraseToolkit()
{
    delete _m_pToolBar;
}

VA(0x00418be2, 0x6)
BEGIN_MESSAGE_MAP(TEraseToolkit, TToolkitBase)
END_MESSAGE_MAP()

void TEraseToolkit::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TOOLKIT_STATIC, _m_brushStatic);
}
