// TerrainToolkit.cpp - the terrain toolkit (h3maped 0x4bdbf1..0x4bdfe4;
// Loki h3maped object 66). The page stacks the brush tool bar under its
// label and the terrain type label and tool bar under it, 3 dialog units
// apart, and sizes itself to fit.
#include "editor/stdafx.h"

#include "va.h"
#include "editor/MapEditorText.h"
#include "editor/TerrainToolkit.h"
#include "editor/resource.h"

VA(0x004bdc0d, 0x31d)
TTerrainToolkit::TTerrainToolkit(CWnd* pParent)
    : TToolkitBase(IDD_TERRAIN_TOOLKIT, pParent),
      _m_pBrushToolBar(NULL),
      _m_pTerrainToolBar(NULL)
{
    if (!Create(IDD_TERRAIN_TOOLKIT, pParent))
        throw TRuntimeError();
    _m_brushStatic.SetWindowText(kBrushStr);
    _m_terrainTypeStatic.SetWindowText(kTerrainTypeStr);
    _m_pBrushToolBar = new TToolkitBaseToolBar(this, IDR_BRUSH_TOOLBAR, 1);
    if (_m_pBrushToolBar == NULL)
        throw TAllocationFailure();
    try {
        _m_pTerrainToolBar = new TToolkitBaseToolBar(this, IDR_TERRAIN_TOOLBAR, 4);
        if (_m_pTerrainToolBar == NULL)
            throw TAllocationFailure();
    } catch (...) {
        _deleteAll();
        throw;
    }
    CRect rect(0, 0, 0, 3);
    MapDialogRect(&rect);
    int margin = rect.Height();
    _m_brushStatic.GetWindowRect(&rect);
    ScreenToClient(&rect);
    int left = rect.left;
    int width = rect.Width();
    int top = rect.top;
    int y = rect.bottom + margin;
    CSize toolBarSize = _m_pBrushToolBar->getSize();
    _m_pBrushToolBar->MoveWindow(left, y, toolBarSize.cx, toolBarSize.cy, FALSE);
    if (toolBarSize.cx > width)
        width = toolBarSize.cx;
    y += toolBarSize.cy + margin;
    _m_terrainTypeStatic.GetWindowRect(&rect);
    _m_terrainTypeStatic.MoveWindow(left, y, rect.Width(), rect.Height(), FALSE);
    if (rect.Width() > width)
        width = rect.Width();
    y += rect.Height() + margin;
    toolBarSize = _m_pTerrainToolBar->getSize();
    _m_pTerrainToolBar->MoveWindow(left, y, toolBarSize.cx, toolBarSize.cy, FALSE);
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

VA_COMPGEN(0x004bdf0e, 0x1c, SCALAR_DELETING_DTOR, TTerrainToolkit)

VA(0x004bdf2a, 0x56)
TTerrainToolkit::~TTerrainToolkit()
{
    _deleteAll();
}

VA(0x004bdf80, 0x27)
void TTerrainToolkit::_deleteAll()
{
    delete _m_pTerrainToolBar;
    delete _m_pBrushToolBar;
}

VA(0x004bdfa7, 0x2e)
void TTerrainToolkit::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_TERRAIN_TYPE_STATIC, _m_terrainTypeStatic);
    DDX_Control(pDX, IDC_TOOLKIT_STATIC, _m_brushStatic);
}

VA(0x004bdfd5, 0x6)
BEGIN_MESSAGE_MAP(TTerrainToolkit, TToolkitBase)
END_MESSAGE_MAP()

VA(0x004bdfdb, 0x9)
BOOL TTerrainToolkit::OnInitDialog()
{
    CDialog::OnInitDialog();
    return TRUE;
}
