// MapDoc.cpp - the map document (h3maped 0x45d627..0x462881; Loki h3maped
// object 57). Loki's document asserts its operations' preconditions and
// saves through POSIX files; the Windows document is an MFC CDocument
// that serializes through CArchive's file, keeps a revision number with
// each map in its undo queue, generates random maps and autosaves.
//
// The terrain, river and road operations are the random map generator's
// (terrainplacement.h, lineplacement.h, lineerase.h); the document adapts
// its map to their map interfaces and reports what they change; the Tools
// menu repaint is editor-only code of the game's terrain source.
#include "editor/stdafx.h"

#include <algorithm>
#include <functional>
#include <istream>
#include <ostream>
#include <set>
#include <sstream>
#include <string>

#include "va.h"
#include "abstractfile.h"
#include "gzinflatebuf.h"
#include "lineerase.h"
#include "progress_bar.h"
#include "rmg_terrain_tile.h"
#include "terrainplacement.h"
#include "editor/Clamp.h"
#include "editor/GUIGameObject.h"
#include "editor/MapDoc.h"
#include "editor/MapEditorText.h"
#include "editor/MFCFileBuf.h"
#include "editor/NewMapDlg.h"
#include "editor/RawStream.h"
#include "editor/StringUtil.h"
#include "editor/resource.h"

namespace {

DATA(0x0059fe70) std::set<TMapDoc*> allMapDocs;

// Asks for a file name with the common file dialog, filtering by the
// file type the filter text names ("description\n.ext") and by all files.
// The name offered is the given one with the type's extension.
VA(0x0045d82d, 0x231)
bool promptForFileName(LPCTSTR pszName, BOOL bOpenFileDialog, DWORD flags, CString& fileName)
{
    CString filter;
    CString defExt;
    CString filterText(kTextFileFilterStr);
    int i = filterText.ReverseFind('\n');
    defExt = filterText.Right(filterText.GetLength() - i - 2);
    filter = filterText.Left(i);
    filter += "|*";
    filter += filterText.Right(filterText.GetLength() - i - 1);
    filter += "|";
    CString allFilter((LPCTSTR)AFX_IDS_ALLFILTER);
    filter += allFilter;
    filter += "|*.*||";
    CFileDialog dlg(bOpenFileDialog, defExt, NULL, flags, filter, NULL);
    CString name(pszName);
    int dot = name.ReverseFind('.');
    if (dot != -1 && name.GetLength() - dot <= 4)
        name.Delete(dot + 1, name.GetLength() - dot - 1);
    else
        name += '.';
    name += defExt;
    fileName = name;
    bool bResult;
    {
        TStringBuffer buffer(fileName, _MAX_PATH);
        dlg.m_ofn.lpstrFile = buffer;
        bResult = dlg.DoModal() == IDOK;
    }
    return bResult;
}

// The maps' object factory: the editor's GUI objects. Its one instance
// installs itself as the factory every map creates its objects with.
class TTheObjectFactory : public TGUIGameObjectFactory {
public:
    TTheObjectFactory();
};

VA(0x0045dae4, 0x31)
TTheObjectFactory::TTheObjectFactory()
{
    TGameMap::setObjectFactory(this);
}

DATA(0x0059fe68) TTheObjectFactory theObjectFactory;

// The terrain of a map layer as the terrain operations' map, for the
// repaint: it is pointed at a layer for each repaint and changes only the
// tiles' frames.
class TRepaintMapOp : public TTerrainPlacementOp::TAbstractMap {
public:
    void operator()(TGameMap* pMap, bool bSecondLayer, unsigned int specialTileFrequency)
    {
        _m_pMap = pMap;
        _m_bSecondLayer = bSecondLayer;
        TTerrainPlacementOp::repaintMap(this, specialTileFrequency);
        _m_pMap = NULL;
    }

    virtual void setTile(const TTilePoint& loc, const TRmgTerrainTile& tile);
    virtual void setFrame(const TTilePoint& loc, int frame);
    virtual TTilePoint& getSize(TTilePoint& size);
    virtual TRmgTerrainTile getTile(const TTilePoint& loc);
    virtual int getTerrain(const TTilePoint& loc);
    virtual int getFrame(const TTilePoint& loc);

private:
    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
};

// /OPT:ICF folds this empty body onto the map view's.
void TRepaintMapOp::setTile(const TTilePoint& loc, const TRmgTerrainTile& tile)
{
}

// This body and getTerrain and getFrame fold onto the map specifications'
// page's adapter (/OPT:ICF).
void TRepaintMapOp::setFrame(const TTilePoint& loc, int frame)
{
    TGameMap::TLayer& layer = _m_pMap->getLayer(_m_bSecondLayer);
    layer.getPCell(loc.x(), loc.y())->setTileNum(frame);
}

// The river and road maps' identical bodies fold onto this one (/OPT:ICF).
VA(0x0045db38, 0x26)
TTilePoint& TRepaintMapOp::getSize(TTilePoint& size)
{
    return size = TTilePoint(_m_pMap->getWidth(), _m_pMap->getHeight());
}

VA(0x0045db5e, 0x67)
TRmgTerrainTile TRepaintMapOp::getTile(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    const TGameMap::TLayer::TCell& cell = layer.getCell(loc.x(), loc.y());
    TRmgTerrainTile tile;
    tile.m_terrain = cell.getTerrainType();
    tile.m_frame = cell.getTileNum();
    tile.m_flipX = cell.getBHFlipped();
    tile.m_flipY = cell.getBVFlipped();
    return tile;
}

int TRepaintMapOp::getTerrain(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    return layer.getCell(loc.x(), loc.y()).getTerrainType();
}

int TRepaintMapOp::getFrame(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    return layer.getCell(loc.x(), loc.y()).getTileNum();
}

// The rivers of a map layer as the river operations' map. It keeps the
// extent of the cells it changes for the document's update.
class TRiverMap : public TRiverOp::TAbstractMap {
public:
    TRiverMap(TGameMap* pMap, bool bSecondLayer) : _m_pMap(pMap), _m_bSecondLayer(bSecondLayer)
    {
        reset();
    }

    virtual void setTile(const TTilePoint& loc, const TRmgTerrainTile& tile);
    virtual void setLineType(const TTilePoint& loc, int type);
    virtual TTilePoint getSize();
    virtual TRmgTerrainTile getTile(const TTilePoint& loc);
    virtual int getLineType(const TTilePoint& loc);
    virtual int getTerrain(const TTilePoint& loc);

    bool getUpdateExtent(TTileExtent* pExtent) const;
    void reset();

protected:
    void _touch(const TTilePoint& loc);

    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
    TTilePoint _m_topLeft;
    TTilePoint _m_bottomRight;
};

// The roads of a map layer as the road operations' map.
class TRoadMap : public TRoadOp::TAbstractMap {
public:
    TRoadMap(TGameMap* pMap, bool bSecondLayer) : _m_pMap(pMap), _m_bSecondLayer(bSecondLayer)
    {
        reset();
    }

    virtual void setTile(const TTilePoint& loc, const TRmgTerrainTile& tile);
    virtual void setLineType(const TTilePoint& loc, int type);
    virtual TTilePoint getSize();
    virtual TRmgTerrainTile getTile(const TTilePoint& loc);
    virtual int getLineType(const TTilePoint& loc);
    virtual int getTerrain(const TTilePoint& loc);

    bool getUpdateExtent(TTileExtent* pExtent) const;
    void reset();

protected:
    void _touch(const TTilePoint& loc);

    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
    TTilePoint _m_topLeft;
    TTilePoint _m_bottomRight;
};

// Folds onto the repaint operation's body (/OPT:ICF).
TTilePoint TRiverMap::getSize()
{
    return TTilePoint(_m_pMap->getWidth(), _m_pMap->getHeight());
}

// The road map's identical body folds onto this one (/OPT:ICF).
VA(0x0045dbe2, 0x41)
bool TRiverMap::getUpdateExtent(TTileExtent* pExtent) const
{
    if (_m_bottomRight.x() > _m_topLeft.x()) {
        *pExtent = TTileExtent(_m_topLeft, _m_bottomRight - _m_topLeft);
        return true;
    }
    return false;
}

VA(0x0045dc23, 0x86)
void TRiverMap::setTile(const TTilePoint& loc, const TRmgTerrainTile& tile)
{
    TGameMap::TLayer& layer = _m_pMap->getLayer(_m_bSecondLayer);
    TGameMap::TLayer::TCell* pCell = layer.getPCell(loc.x(), loc.y());
    pCell->setRiverType(tile.m_terrain);
    pCell->setRiverTileNum(tile.m_frame);
    pCell->setBRiverHFlipped(tile.m_flipX);
    pCell->setBRiverVFlipped(tile.m_flipY);
    _touch(loc);
}

VA(0x0045dca9, 0x4b)
void TRiverMap::setLineType(const TTilePoint& loc, int type)
{
    TGameMap::TLayer& layer = _m_pMap->getLayer(_m_bSecondLayer);
    layer.getPCell(loc.x(), loc.y())->setRiverType(type);
    _touch(loc);
}

VA(0x0045dcf4, 0x6a)
TRmgTerrainTile TRiverMap::getTile(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    const TGameMap::TLayer::TCell& cell = layer.getCell(loc.x(), loc.y());
    TRmgTerrainTile tile;
    tile.m_terrain = cell.getRiverType();
    tile.m_frame = cell.getRiverTileNum();
    tile.m_flipX = cell.getBRiverHFlipped();
    tile.m_flipY = cell.getBRiverVFlipped();
    return tile;
}

VA(0x0045dd5e, 0x34)
int TRiverMap::getLineType(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    return layer.getCell(loc.x(), loc.y()).getRiverType();
}

int TRiverMap::getTerrain(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    return layer.getCell(loc.x(), loc.y()).getTerrainType();
}

void TRiverMap::reset()
{
    _m_topLeft = TTilePoint(_m_pMap->getWidth(), _m_pMap->getHeight());
    _m_bottomRight = TTilePoint(0, 0);
}

void TRiverMap::_touch(const TTilePoint& loc)
{
    if (loc.x() < _m_topLeft.x())
        _m_topLeft.x(loc.x());
    if (loc.y() < _m_topLeft.y())
        _m_topLeft.y(loc.y());
    if (loc.x() >= _m_bottomRight.x())
        _m_bottomRight.x(loc.x() + 1);
    if (loc.y() >= _m_bottomRight.y())
        _m_bottomRight.y(loc.y() + 1);
}

TTilePoint TRoadMap::getSize()
{
    return TTilePoint(_m_pMap->getWidth(), _m_pMap->getHeight());
}

bool TRoadMap::getUpdateExtent(TTileExtent* pExtent) const
{
    if (_m_bottomRight.x() > _m_topLeft.x()) {
        *pExtent = TTileExtent(_m_topLeft, _m_bottomRight - _m_topLeft);
        return true;
    }
    return false;
}

// The river map's identical body folds onto this one (/OPT:ICF).
VA(0x0045ddaf, 0x29)
void TRoadMap::reset()
{
    _m_topLeft = TTilePoint(_m_pMap->getWidth(), _m_pMap->getHeight());
    _m_bottomRight = TTilePoint(0, 0);
}

VA(0x0045ddd8, 0x86)
void TRoadMap::setTile(const TTilePoint& loc, const TRmgTerrainTile& tile)
{
    TGameMap::TLayer& layer = _m_pMap->getLayer(_m_bSecondLayer);
    TGameMap::TLayer::TCell* pCell = layer.getPCell(loc.x(), loc.y());
    pCell->setRoadType(tile.m_terrain);
    pCell->setRoadTileNum(tile.m_frame);
    pCell->setBRoadHFlipped(tile.m_flipX);
    pCell->setBRoadVFlipped(tile.m_flipY);
    _touch(loc);
}

VA(0x0045de5e, 0x4d)
void TRoadMap::setLineType(const TTilePoint& loc, int type)
{
    TGameMap::TLayer& layer = _m_pMap->getLayer(_m_bSecondLayer);
    layer.getPCell(loc.x(), loc.y())->setRoadType(type);
    _touch(loc);
}

VA(0x0045deab, 0x6a)
TRmgTerrainTile TRoadMap::getTile(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    const TGameMap::TLayer::TCell& cell = layer.getCell(loc.x(), loc.y());
    TRmgTerrainTile tile;
    tile.m_terrain = cell.getRoadType();
    tile.m_frame = cell.getRoadTileNum();
    tile.m_flipX = cell.getBRoadHFlipped();
    tile.m_flipY = cell.getBRoadVFlipped();
    return tile;
}

VA(0x0045df15, 0x34)
int TRoadMap::getLineType(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    return layer.getCell(loc.x(), loc.y()).getRoadType();
}

int TRoadMap::getTerrain(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    return layer.getCell(loc.x(), loc.y()).getTerrainType();
}

// The river map's identical body folds onto this one (/OPT:ICF).
VA(0x0045df49, 0x33)
void TRoadMap::_touch(const TTilePoint& loc)
{
    if (loc.x() < _m_topLeft.x())
        _m_topLeft.x(loc.x());
    if (loc.y() < _m_topLeft.y())
        _m_topLeft.y(loc.y());
    if (loc.x() >= _m_bottomRight.x())
        _m_bottomRight.x(loc.x() + 1);
    if (loc.y() >= _m_bottomRight.y())
        _m_bottomRight.y(loc.y() + 1);
}

// A stream buffer as the random map generator's output file.
class TStreamBufFile : public TAbstractFile {
public:
    TStreamBufFile(std::streambuf* pStreamBuf) : _m_pStreamBuf(pStreamBuf) {}

    virtual int read(void* data, int size);
    virtual int write(const void* data, int size);

private:
    std::streambuf* _m_pStreamBuf;
};

VA(0x0045df7c, 0x13)
int TStreamBufFile::read(void* data, int size)
{
    return _m_pStreamBuf->sgetn(static_cast<char*>(data), size);
}

VA(0x0045df8f, 0x13)
int TStreamBufFile::write(const void* data, int size)
{
    return _m_pStreamBuf->sputn(static_cast<const char*>(data), size);
}

// The modeless dialog that shows the random map generator's progress
// (RTTI names it with the generator's progress sink as its second base).
class TMapGenerationProgressDlg : public CDialog, public type_progress_bar {
public:
    enum { IDD = IDD_MAP_GENERATION_PROGRESS };

    TMapGenerationProgressDlg(CWnd* pParent);

    virtual void setTotal(int totalSteps);
    virtual void advance(int amount);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    bool _m_bInitialized;
    CProgressCtrl _m_progressCtrl;
};

VA(0x0045dfa2, 0x6)
BEGIN_MESSAGE_MAP(TMapGenerationProgressDlg, CDialog)
    ON_WM_DESTROY()
END_MESSAGE_MAP()

VA(0x0045dfa8, 0x16c)
TMapGenerationProgressDlg::TMapGenerationProgressDlg(CWnd* pParent)
    : type_progress_bar(1),
      _m_bInitialized(false)
{
    if (!Create(IDD, pParent))
        throw TRuntimeError();
    SetWindowText(kMapGenerationProgressCaptionStr);
    CRect rect;
    if (pParent != NULL) {
        pParent->GetClientRect(&rect);
        pParent->ClientToScreen(&rect);
    } else {
        rect = CRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }
    CSize size = rect.Size();
    CRect dlgRect;
    GetWindowRect(&dlgRect);
    CSize dlgSize = dlgRect.Size();
    dlgRect = CRect(CPoint(rect.left + (size.cx - dlgSize.cx) / 2, rect.top + (size.cy - dlgSize.cy) / 2), dlgSize);
    MoveWindow(&dlgRect);
    ShowWindow(SW_SHOW);
    UpdateWindow();
}

VA_COMPGEN(0x0045e114, 0x1c, SCALAR_DELETING_DTOR, TMapGenerationProgressDlg)
VA_COMPGEN(0x0045e130, 0x4c, IMPLICIT_DTOR, TMapGenerationProgressDlg)

VA(0x0045e17c, 0x2a)
void TMapGenerationProgressDlg::setTotal(int totalSteps)
{
    type_progress_bar::setTotal(totalSteps);
    if (_m_bInitialized)
        _m_progressCtrl.SetRange32(0, totalSteps);
}

VA(0x0045e1a6, 0x4c)
void TMapGenerationProgressDlg::advance(int amount)
{
    m_done = clamp<long>(0, m_done + amount, m_steps);
    if (_m_bInitialized)
        _m_progressCtrl.SetPos(m_done);
}

VA(0x0045e1f2, 0x15)
void TMapGenerationProgressDlg::DoDataExchange(CDataExchange* pDX)
{
    DDX_Control(pDX, IDC_GENERATION_PROGRESS, _m_progressCtrl);
}

VA(0x0045e207, 0x56)
BOOL TMapGenerationProgressDlg::OnInitDialog()
{
    GetDlgItem(IDC_GENERATING_MAP_STATIC)->SetWindowText(kGeneratingMapStr);
    CDialog::OnInitDialog();
    _m_progressCtrl.SetRange32(0, m_steps);
    _m_progressCtrl.SetPos(m_done);
    _m_bInitialized = true;
    return TRUE;
}

VA(0x0045e25d, 0x9)
void TMapGenerationProgressDlg::OnDestroy()
{
    _m_bInitialized = false;
    CDialog::OnDestroy();
}

}  // namespace

VA(0x0045e266, 0xb4)
TMapDoc::TNewMapParams::TNewMapParams(const TNewMapParams& other)
    : m_version(other.m_version),
      m_size(other.m_size),
      m_bTwoLayer(other.m_bTwoLayer)
{
    if (other.m_pRandomMapParams.get() != NULL) {
        m_pRandomMapParams = std::auto_ptr<TRandomMapParams>(new TRandomMapParams(*other.m_pRandomMapParams));
        if (m_pRandomMapParams.get() == NULL)
            throw TAllocationFailure();
    }
}

DATA(0x00539280)
IMPLEMENT_DYNAMIC(TMapDocLoadFailure, CException)

DATA(0x00539298)
IMPLEMENT_DYNAMIC(TMapDocInvalidFileVersion, TMapDocLoadFailure)

// The terrain placement operation on one map layer: the operation paints
// through the document's map and reports the changed cells in an update.
class TMapDoc::_TTerrainPlacementOp : public TTerrainPlacementOp::TAbstractMap {
public:
    _TTerrainPlacementOp(TMapDoc* pDoc, bool bSecondLayer, TTerrainType terrainType,
                         unsigned int specialTileFrequency);
    virtual ~_TTerrainPlacementOp();

    void operator()(unsigned int left, unsigned int top, unsigned int width, unsigned int height);

    virtual void setTile(const TTilePoint& loc, const TRmgTerrainTile& tile);
    virtual void setFrame(const TTilePoint& loc, int frame);
    virtual TTilePoint& getSize(TTilePoint& size);
    virtual TRmgTerrainTile getTile(const TTilePoint& loc);
    virtual int getTerrain(const TTilePoint& loc);
    virtual int getFrame(const TTilePoint& loc);

private:
    void _reset();
    void _touch(const TTilePoint& loc);

    TMapDoc* _m_pDoc;
    TGameMap* _m_pMap;
    bool _m_bSecondLayer;
    std::auto_ptr<TTerrainPlacementOp> _m_pOp;
    TTilePoint _m_topLeft;
    TTilePoint _m_bottomRight;
};

VA(0x0045e326, 0xcc)
TMapDoc::_TTerrainPlacementOp::_TTerrainPlacementOp(TMapDoc* pDoc, bool bSecondLayer,
                                                    TTerrainType terrainType,
                                                    unsigned int specialTileFrequency)
    : _m_pDoc(pDoc),
      _m_pMap(pDoc->_m_pMap),
      _m_bSecondLayer(bSecondLayer)
{
    _m_pOp = std::auto_ptr<TTerrainPlacementOp>(
        new TTerrainPlacementOp(this, terrainType, specialTileFrequency));
    if (_m_pOp.get() == NULL)
        throw TAllocationFailure();
    _reset();
}

VA(0x0045e416, 0x29)
void TMapDoc::_TTerrainPlacementOp::_reset()
{
    _m_topLeft = TTilePoint(_m_pMap->getWidth(), _m_pMap->getHeight());
    _m_bottomRight = TTilePoint(0, 0);
}

// The operation paints its last tiles as it is destroyed.
VA(0x0045e45b, 0x90)
TMapDoc::_TTerrainPlacementOp::~_TTerrainPlacementOp()
{
    delete _m_pOp.release();
    if (_m_bottomRight.x() > _m_topLeft.x())
        _m_pDoc->_onTerrainUpdated(_m_bSecondLayer, _m_topLeft.x(), _m_topLeft.y(),
                                   _m_bottomRight.x() - _m_topLeft.x(),
                                   _m_bottomRight.y() - _m_topLeft.y());
}

VA(0x0045e4eb, 0x57)
void TMapDoc::_TTerrainPlacementOp::operator()(unsigned int left, unsigned int top,
                                               unsigned int width, unsigned int height)
{
    _m_pOp->paintRectangle(left, top, width, height);
    if (_m_bottomRight.x() > _m_topLeft.x()) {
        _m_pDoc->_onTerrainUpdated(_m_bSecondLayer, _m_topLeft.x(), _m_topLeft.y(),
                                   _m_bottomRight.x() - _m_topLeft.x(),
                                   _m_bottomRight.y() - _m_topLeft.y());
        _reset();
    }
}

VA(0x0045e542, 0xa1)
void TMapDoc::_TTerrainPlacementOp::setTile(const TTilePoint& loc, const TRmgTerrainTile& tile)
{
    _touch(loc);
    TGameMap::TLayer& layer = _m_pMap->getLayer(_m_bSecondLayer);
    TGameMap::TLayer::TCell* pCell = layer.getPCell(loc.x(), loc.y());
    TTerrainType oldType = pCell->getTerrainType();
    pCell->setTerrainType(TTerrainType(tile.m_terrain));
    pCell->setTileNum(tile.m_frame);
    pCell->setBHFlipped(tile.m_flipX);
    pCell->setBVFlipped(tile.m_flipY);
    if (tile.m_terrain != oldType)
        _m_pDoc->_onTerrainTypeChanged(_m_bSecondLayer, loc.x(), loc.y(), oldType);
}

VA(0x0045e5e3, 0x33)
void TMapDoc::_TTerrainPlacementOp::_touch(const TTilePoint& loc)
{
    if (loc.x() < _m_topLeft.x())
        _m_topLeft.x(loc.x());
    if (loc.y() < _m_topLeft.y())
        _m_topLeft.y(loc.y());
    if (loc.x() >= _m_bottomRight.x())
        _m_bottomRight.x(loc.x() + 1);
    if (loc.y() >= _m_bottomRight.y())
        _m_bottomRight.y(loc.y() + 1);
}

VA(0x0045e616, 0x4a)
void TMapDoc::_TTerrainPlacementOp::setFrame(const TTilePoint& loc, int frame)
{
    _touch(loc);
    TGameMap::TLayer& layer = _m_pMap->getLayer(_m_bSecondLayer);
    layer.getPCell(loc.x(), loc.y())->setTileNum(frame);
}

VA(0x0045e660, 0x26)
TTilePoint& TMapDoc::_TTerrainPlacementOp::getSize(TTilePoint& size)
{
    return size = TTilePoint(_m_pMap->getWidth(), _m_pMap->getHeight());
}

VA(0x0045e686, 0x67)
TRmgTerrainTile TMapDoc::_TTerrainPlacementOp::getTile(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    const TGameMap::TLayer::TCell& cell = layer.getCell(loc.x(), loc.y());
    TRmgTerrainTile tile;
    tile.m_terrain = cell.getTerrainType();
    tile.m_frame = cell.getTileNum();
    tile.m_flipX = cell.getBHFlipped();
    tile.m_flipY = cell.getBVFlipped();
    return tile;
}

VA(0x0045e6ed, 0x31)
int TMapDoc::_TTerrainPlacementOp::getTerrain(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    return layer.getCell(loc.x(), loc.y()).getTerrainType();
}

VA(0x0045e71e, 0x34)
int TMapDoc::_TTerrainPlacementOp::getFrame(const TTilePoint& loc)
{
    const TGameMap* pMap = _m_pMap;
    const TGameMap::TLayer& layer = pMap->getLayer(_m_bSecondLayer);
    return layer.getCell(loc.x(), loc.y()).getTileNum();
}

// A river placement operation on one map layer, drawing from a starting
// cell to each next one.
class TMapDoc::_TRiverPlacementOp : public TRiverMap {
public:
    _TRiverPlacementOp(TMapDoc* pDoc, bool bSecondLayer, int riverType, const TTilePoint& start);

    void operator()(const TTilePoint& loc);

private:
    TMapDoc* _m_pDoc;
    std::auto_ptr<TRiverPlacementOp> _m_pOp;
};

VA(0x0045e752, 0xe7)
TMapDoc::_TRiverPlacementOp::_TRiverPlacementOp(TMapDoc* pDoc, bool bSecondLayer, int riverType,
                                                const TTilePoint& start)
    : TRiverMap(pDoc->_m_pMap, bSecondLayer),
      _m_pDoc(pDoc),
      _m_pOp(new TRiverPlacementOp(this, riverType, start))
{
    if (_m_pOp.get() == NULL)
        throw TAllocationFailure();
    TTileExtent extent;
    if (getUpdateExtent(&extent)) {
        _m_pDoc->_onRiversUpdated(_m_bSecondLayer, extent.left(), extent.top(), extent.width(),
                                  extent.height());
        reset();
    }
}

VA(0x0045e885, 0x53)
void TMapDoc::_TRiverPlacementOp::operator()(const TTilePoint& loc)
{
    _m_pOp->m_walker.drawTo(loc);
    TTileExtent extent;
    if (getUpdateExtent(&extent)) {
        _m_pDoc->_onRiversUpdated(_m_bSecondLayer, extent.left(), extent.top(), extent.width(),
                                  extent.height());
        reset();
    }
}

// A river erase operation on one map layer.
class TMapDoc::_TRiverEraseOp : public TRiverMap {
public:
    _TRiverEraseOp(TMapDoc* pDoc, bool bSecondLayer);

    void operator()(unsigned int left, unsigned int top, unsigned int width, unsigned int height);

private:
    TMapDoc* _m_pDoc;
    std::auto_ptr<TRiverEraseOp> _m_pOp;
};

VA(0x0045e8d8, 0xaa)
TMapDoc::_TRiverEraseOp::_TRiverEraseOp(TMapDoc* pDoc, bool bSecondLayer)
    : TRiverMap(pDoc->_m_pMap, bSecondLayer),
      _m_pDoc(pDoc),
      _m_pOp(new TRiverEraseOp(this))
{
    if (_m_pOp.get() == NULL)
        throw TAllocationFailure();
}

VA(0x0045e9ce, 0x5c)
void TMapDoc::_TRiverEraseOp::operator()(unsigned int left, unsigned int top, unsigned int width,
                                         unsigned int height)
{
    (*_m_pOp)(left, top, width, height);
    TTileExtent extent;
    if (getUpdateExtent(&extent)) {
        _m_pDoc->_onRiversUpdated(_m_bSecondLayer, extent.left(), extent.top(), extent.width(),
                                  extent.height());
        reset();
    }
}

// A road placement operation on one map layer.
class TMapDoc::_TRoadPlacementOp : public TRoadMap {
public:
    _TRoadPlacementOp(TMapDoc* pDoc, bool bSecondLayer, int roadType, const TTilePoint& start);

    void operator()(const TTilePoint& loc);

private:
    TMapDoc* _m_pDoc;
    std::auto_ptr<TRoadPlacementOp> _m_pOp;
};

VA(0x0045ea2a, 0xe7)
TMapDoc::_TRoadPlacementOp::_TRoadPlacementOp(TMapDoc* pDoc, bool bSecondLayer, int roadType,
                                              const TTilePoint& start)
    : TRoadMap(pDoc->_m_pMap, bSecondLayer),
      _m_pDoc(pDoc),
      _m_pOp(new TRoadPlacementOp(this, roadType, start))
{
    if (_m_pOp.get() == NULL)
        throw TAllocationFailure();
    TTileExtent extent;
    if (getUpdateExtent(&extent)) {
        _m_pDoc->_onRoadsUpdated(_m_bSecondLayer, extent.left(), extent.top(), extent.width(),
                                 extent.height());
        reset();
    }
}

VA(0x0045eb5d, 0x53)
void TMapDoc::_TRoadPlacementOp::operator()(const TTilePoint& loc)
{
    _m_pOp->m_walker.drawTo(loc);
    TTileExtent extent;
    if (getUpdateExtent(&extent)) {
        _m_pDoc->_onRoadsUpdated(_m_bSecondLayer, extent.left(), extent.top(), extent.width(),
                                 extent.height());
        reset();
    }
}

// A road erase operation on one map layer.
class TMapDoc::_TRoadEraseOp : public TRoadMap {
public:
    _TRoadEraseOp(TMapDoc* pDoc, bool bSecondLayer);

    void operator()(unsigned int left, unsigned int top, unsigned int width, unsigned int height);

private:
    TMapDoc* _m_pDoc;
    std::auto_ptr<TRoadEraseOp> _m_pOp;
};

VA(0x0045ebb0, 0xaa)
TMapDoc::_TRoadEraseOp::_TRoadEraseOp(TMapDoc* pDoc, bool bSecondLayer)
    : TRoadMap(pDoc->_m_pMap, bSecondLayer),
      _m_pDoc(pDoc),
      _m_pOp(new TRoadEraseOp(this))
{
    if (_m_pOp.get() == NULL)
        throw TAllocationFailure();
}

VA(0x0045eca6, 0x5c)
void TMapDoc::_TRoadEraseOp::operator()(unsigned int left, unsigned int top, unsigned int width,
                                        unsigned int height)
{
    (*_m_pOp)(left, top, width, height);
    TTileExtent extent;
    if (getUpdateExtent(&extent)) {
        _m_pDoc->_onRoadsUpdated(_m_bSecondLayer, extent.left(), extent.top(), extent.width(),
                                 extent.height());
        reset();
    }
}

DATA(0x0059fe50) const TMapDoc::TNewMapParams TMapDoc::s_kDefaultNewMapParams(
    GAME_VERSION_SOD, TGameMap::TSize(1), true, std::auto_ptr<TMapDoc::TRandomMapParams>());

VA(0x0045ed86, 0x35)
IMPLEMENT_DYNCREATE(TMapDoc, CDocument)

VA(0x0045edc1, 0x6)
BEGIN_MESSAGE_MAP(TMapDoc, CDocument)
    ON_COMMAND(ID_FILE_EXPORT_TEXT, OnFileExportText)
    ON_COMMAND(ID_FILE_IMPORT_TEXT, OnFileImportText)
    ON_COMMAND(ID_FILE_BATCH_CONVERT, OnFileBatchConvert)
    ON_COMMAND(ID_TOOLS_REPAINT_MAP, OnToolsRepaintMap)
END_MESSAGE_MAP()

DATA(0x0059fea8) unsigned int TMapDoc::_s_autosaveInterval;
DATA(0x0058b910) unsigned int TMapDoc::_s_specialTileFrequency = 4;

VA(0x0045edc7, 0x2f)
void TMapDoc::autosaveAll()
{
    std::for_each(allMapDocs.begin(), allMapDocs.end(), std::mem_fun(&TMapDoc::_autosave));
}

VA(0x0045edf6, 0xa)
void TMapDoc::setAutosaveInterval(unsigned int interval)
{
    _s_autosaveInterval = interval;
}

VA(0x0045ee00, 0xa)
void TMapDoc::setSpecialTileFrequency(unsigned int newFrequency)
{
    _s_specialTileFrequency = newFrequency;
}

namespace {

// The autosave file: "autosave" with the document type's extension, in
// the temporary directory.
VA(0x0045ee0a, 0xf9)
CString getAutosavePathName(CDocTemplate* pTemplate)
{
    CString ext;
    pTemplate->GetDocString(ext, CDocTemplate::filterExt);
    CString tempPath;
    DWORD length = GetTempPath(_MAX_PATH, tempPath.GetBuffer(_MAX_PATH));
    tempPath.ReleaseBuffer();
    if (length == 0)
        return CString("autosave") + ext;
    return tempPath + CString("autosave") + ext;
}

// The name of the mutex an editor holds while it owns the autosave file.
VA(0x0045ef03, 0x66)
CString getAutosaveMutexName(const CString& pathName)
{
    CString name = pathName + " mutex";
    name.Replace('\\', '/');
    name.MakeLower();
    return name;
}

}  // namespace

// Waits for the autosave mutex or for the document to close; the
// document owns the autosave file while the thread holds the mutex.
VA(0x0045ef69, 0x7b)
UINT TMapDoc::_autosaveThreadProc(LPVOID pParam)
{
    TMapDoc* pDoc = static_cast<TMapDoc*>(pParam);
    CSyncObject* apObjects[2] = { &pDoc->_m_autosaveMutex, &pDoc->_m_closeEvent };
    CMultiLock lock(apObjects, 2, FALSE);
    if (lock.Lock(INFINITE, FALSE, 0) != WAIT_OBJECT_0 + 1) {
        pDoc->_m_bOwnsAutosave = true;
        pDoc->_m_closeEvent.Lock(INFINITE);
        pDoc->_m_bOwnsAutosave = false;
    }
    return 0;
}

VA(0x0045efe4, 0x98)
TMapDoc::TMapDoc()
{
}

VA_COMPGEN(0x0045f08a, 0x1c, SCALAR_DELETING_DTOR, TMapDoc)

VA(0x0045f0a6, 0x176)
TMapDoc::TMapDoc(CDocTemplate* pTemplate)
    : _m_pMap(NULL),
      _m_revision(0),
      _m_lastRevision(0),
      _m_currentIndex(0),
      _m_savedIndex(s_kMaxUndoQueueSize),
      _m_bPromptForNewMapParams(false),
      _m_newMapParams(s_kDefaultNewMapParams),
      _m_bOfferAutosave(true),
      _m_bCreatingMap(false),
      _m_pTerrainPlacementOp(NULL),
      _m_pRiverPlacementOp(NULL),
      _m_pRiverEraseOp(NULL),
      _m_pRoadPlacementOp(NULL),
      _m_pRoadEraseOp(NULL),
      _m_bEraseUnderground(false),
      _m_bOwnsAutosave(false),
      _m_autosavePathName(getAutosavePathName(pTemplate)),
      _m_autosaveMutex(FALSE, getAutosaveMutexName(_m_autosavePathName)),
      _m_autosaveThread(_autosaveThreadProc, this),
      _m_closeEvent(FALSE, TRUE),
      _m_lastSaveTime(GetTickCount())
{
    _m_autosaveThread.m_bAutoDelete = FALSE;
    if (!_m_autosaveThread.CreateThread())
        throw TRuntimeError();
    allMapDocs.insert(this);
}

VA(0x0045f21c, 0x10d)
TMapDoc::~TMapDoc()
{
    allMapDocs.erase(this);
    if (_m_bOwnsAutosave) {
        try {
            CFile::Remove(_m_autosavePathName);
        } catch (CFileException* e) {
            e->Delete();
        }
    }
    _m_closeEvent.SetEvent();
    WaitForSingleObject(_m_autosaveThread.m_hThread, INFINITE);
}

// The new-map parameters' implicit assignment (setNewMapParams).
VA_COMPGEN(0x0045f52d, 0x29, IMPLICIT_COPY_ASSIGN, TNewMapParams)

VA(0x0045f556, 0x184)
void TMapDoc::Serialize(CArchive& ar)
{
    TMFCFileBuf buf(ar.GetFile());
    try {
        if (ar.IsStoring()) {
            TGzDeflateBuf deflateBuf(&buf);
            TRawOStream stream(&deflateBuf);
            long version = akMapFileVersion[_m_pMap->getVersion()];
            stream << version;
            _m_pMap->save(&deflateBuf);
        } else {
            {
                TGzInflateBuf inflateBuf(&buf);
                _m_pMap = _readMap(&inflateBuf).release();
            }
            if (_m_pMap == NULL)
                throw TAllocationFailure();
            _m_revision = 0;
            _m_lastRevision = 1;
        }
    } catch (...) {
        throw new TMapDocLoadFailure;
    }
}

VA(0x0045f6db, 0x7d)
void TMapDoc::DeleteContents()
{
    if (_m_pMap != NULL) {
        TNotification notification;
        notification.m_type = TNotification::eClear;
        UpdateAllViews(NULL, (LPARAM)&notification);
        delete _m_pMap;
        _m_pMap = NULL;
    }
    _m_undoQueue.clear();
    _m_currentIndex = 0;
    _m_savedIndex = s_kMaxUndoQueueSize;
    CDocument::DeleteContents();
}

VA(0x0045f758, 0x23)
BOOL TMapDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if (!CDocument::OnOpenDocument(lpszPathName))
        return FALSE;
    _m_lastSaveTime = GetTickCount();
    return TRUE;
}

VA(0x0045f77b, 0x23)
BOOL TMapDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    if (!CDocument::OnSaveDocument(lpszPathName))
        return FALSE;
    _m_lastSaveTime = GetTickCount();
    return TRUE;
}

VA(0x0045f79e, 0xc0)
void TMapDoc::undo()
{
    --_m_currentIndex;
    std::swap(*_m_pMap, _m_undoQueue[_m_currentIndex].second);
    std::swap(_m_revision, _m_undoQueue[_m_currentIndex].first);
    if (!IsModified()) {
        _m_savedIndex = _m_currentIndex;
        SetModifiedFlag(TRUE);
    } else if (_m_savedIndex == _m_currentIndex) {
        _m_savedIndex = s_kMaxUndoQueueSize;
        SetModifiedFlag(FALSE);
    }
    TNotification notification;
    notification.m_type = TNotification::eUndo;
    notification.m_revision = _m_revision;
    UpdateAllViews(NULL, (LPARAM)&notification);
}

VA(0x0045f85e, 0xbf)
void TMapDoc::redo()
{
    std::swap(*_m_pMap, _m_undoQueue[_m_currentIndex].second);
    std::swap(_m_revision, _m_undoQueue[_m_currentIndex].first);
    if (!IsModified()) {
        _m_savedIndex = _m_currentIndex;
        SetModifiedFlag(TRUE);
    } else if (_m_savedIndex == _m_currentIndex) {
        _m_savedIndex = s_kMaxUndoQueueSize;
        SetModifiedFlag(FALSE);
    }
    ++_m_currentIndex;
    TNotification notification;
    notification.m_type = TNotification::eRedo;
    notification.m_revision = _m_revision;
    UpdateAllViews(NULL, (LPARAM)&notification);
}

// Keeps a copy of the map, with its revision, where undo finds it. The
// maps redo could have restored are dropped first, then the oldest map
// when the queue is full; the views hear of each map dropped.
VA(0x0045f91d, 0x14b)
void TMapDoc::backupMap()
{
    while (_m_undoQueue.size() > _m_currentIndex) {
        TNotification notification;
        notification.m_type = TNotification::eDiscardBackup;
        notification.m_revision = (_m_undoQueue.end() - 1)->first;
        UpdateAllViews(NULL, (LPARAM)&notification);
        _m_undoQueue.pop_back();
        if (_m_savedIndex >= _m_undoQueue.size())
            _m_savedIndex = s_kMaxUndoQueueSize;
    }
    if (_m_undoQueue.size() == s_kMaxUndoQueueSize) {
        TNotification notification;
        notification.m_type = TNotification::eDiscardBackup;
        notification.m_revision = _m_undoQueue.begin()->first;
        UpdateAllViews(NULL, (LPARAM)&notification);
        _m_undoQueue.pop_front();
        --_m_currentIndex;
        if (_m_savedIndex == 0)
            _m_savedIndex = s_kMaxUndoQueueSize;
        else if (_m_savedIndex < s_kMaxUndoQueueSize)
            --_m_savedIndex;
    }
    if (!IsModified())
        _m_savedIndex = _m_currentIndex;
    _m_undoQueue.push_back(std::make_pair(_m_revision, *_m_pMap));
    ++_m_currentIndex;
    _m_revision = ++_m_lastRevision;
    TNotification notification;
    notification.m_type = TNotification::eBackup;
    notification.m_revision = _m_revision;
    UpdateAllViews(NULL, (LPARAM)&notification);
}

VA(0x0045fa70, 0x75)
TMapLayerObjectID TMapDoc::placeObject(bool bSecondLayer, std::auto_ptr<TGameObject> pObj, const TTilePoint& loc)
{
    TTileExtent extent;
    TMapLayerObjectID objID = _m_pMap->placeObject(bSecondLayer, pObj, loc, &extent);
    if (objID != TGameMap::TLayer::s_kInvalidObjID)
        _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
    return objID;
}

VA(0x0045fae5, 0x36)
void TMapDoc::removeObject(bool bSecondLayer, unsigned int objID)
{
    TTileExtent extent;
    _m_pMap->removeObject(bSecondLayer, objID, &extent);
    _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
}

VA(0x0045fb1b, 0x36)
void TMapDoc::floatObject(bool bSecondLayer, unsigned int objID)
{
    TTileExtent extent;
    _m_pMap->floatObject(bSecondLayer, objID, &extent);
    _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
}

VA(0x0045fb51, 0x39)
void TMapDoc::unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y)
{
    TTileExtent extent;
    _m_pMap->unfloatObject(bSecondLayer, x, y, &extent);
    _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
}

VA(0x0045fb8a, 0x50)
void TMapDoc::onObjectPropertiesChanged(bool bSecondLayer, unsigned int objID)
{
    TGameMap::TLayer& layer = _m_pMap->getLayer(bSecondLayer);
    TTileExtent extent = layer.getObjectExtent(objID);
    _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
}

VA(0x0045fbda, 0x7f)
void TMapDoc::startTerrainPlacementOp(bool bSecondLayer, TTerrainType terrainType)
{
    backupMap();
    _m_pTerrainPlacementOp = new _TTerrainPlacementOp(this, bSecondLayer, terrainType,
                                                      _s_specialTileFrequency);
    if (_m_pTerrainPlacementOp == NULL)
        throw TAllocationFailure();
}

VA(0x0045fc59, 0x1c)
void TMapDoc::endTerrainPlacementOp()
{
    delete _m_pTerrainPlacementOp;
    _m_pTerrainPlacementOp = NULL;
}

VA(0x0045fc75, 0x1e)
void TMapDoc::terrainFill(unsigned int left, unsigned int top, unsigned int width, unsigned int height)
{
    (*_m_pTerrainPlacementOp)(left, top, width, height);
}

VA(0x0045fc93, 0x89)
void TMapDoc::startRiverPlacementOp(bool bSecondLayer, int riverType, unsigned int x, unsigned int y)
{
    backupMap();
    _m_pRiverPlacementOp = new _TRiverPlacementOp(this, bSecondLayer, riverType, TTilePoint(x, y));
    if (_m_pRiverPlacementOp == NULL)
        throw TAllocationFailure();
}

VA(0x0045fd1c, 0x1c)
void TMapDoc::endRiverPlacementOp()
{
    delete _m_pRiverPlacementOp;
    _m_pRiverPlacementOp = NULL;
}

VA(0x0045fd38, 0x24)
void TMapDoc::placeRiver(unsigned int x, unsigned int y)
{
    (*_m_pRiverPlacementOp)(TTilePoint(x, y));
}

VA(0x0045fd5c, 0x89)
void TMapDoc::startRoadPlacementOp(bool bSecondLayer, int roadType, unsigned int x, unsigned int y)
{
    backupMap();
    _m_pRoadPlacementOp = new _TRoadPlacementOp(this, bSecondLayer, roadType, TTilePoint(x, y));
    if (_m_pRoadPlacementOp == NULL)
        throw TAllocationFailure();
}

VA(0x0045fde5, 0x1c)
void TMapDoc::endRoadPlacementOp()
{
    delete _m_pRoadPlacementOp;
    _m_pRoadPlacementOp = NULL;
}

VA(0x0045fe01, 0x24)
void TMapDoc::placeRoad(unsigned int x, unsigned int y)
{
    (*_m_pRoadPlacementOp)(TTilePoint(x, y));
}

// Erasing clears the rivers, the roads and the objects of a rectangle.
VA(0x0045fe25, 0x103)
void TMapDoc::startEraseOp(bool bSecondLayer)
{
    backupMap();
    _m_bEraseUnderground = bSecondLayer;
    _m_pRiverEraseOp = new _TRiverEraseOp(this, bSecondLayer);
    if (_m_pRiverEraseOp == NULL)
        throw TAllocationFailure();
    try {
        _m_pRoadEraseOp = new _TRoadEraseOp(this, bSecondLayer);
        if (_m_pRoadEraseOp == NULL)
            throw TAllocationFailure();
    } catch (...) {
        delete _m_pRiverEraseOp;
        _m_pRiverEraseOp = NULL;
        throw;
    }
}

VA(0x0045ff28, 0x39)
void TMapDoc::endEraseOp()
{
    delete _m_pRoadEraseOp;
    _m_pRoadEraseOp = NULL;
    delete _m_pRiverEraseOp;
    _m_pRiverEraseOp = NULL;
}

VA(0x0045ff61, 0xae)
void TMapDoc::erase(unsigned int left, unsigned int top, unsigned int width, unsigned int height)
{
    (*_m_pRiverEraseOp)(left, top, width, height);
    (*_m_pRoadEraseOp)(left, top, width, height);
    TGameMap::TLayer& layer = _m_pMap->getLayer(_m_bEraseUnderground);
    for (unsigned int y = top; y < top + height; y++) {
        for (unsigned int x = left; x < left + width; x++) {
            while (layer.getNumObjectIDsAtCell(x, y) > 0)
                removeObject(_m_bEraseUnderground, layer.getObjectIDAtCell(x, y, 0));
        }
    }
}

VA(0x0046000f, 0x76)
void TMapDoc::startRiverEraseOp(bool bSecondLayer)
{
    backupMap();
    _m_pRiverEraseOp = new _TRiverEraseOp(this, bSecondLayer);
    if (_m_pRiverEraseOp == NULL)
        throw TAllocationFailure();
}

VA(0x00460085, 0x1c)
void TMapDoc::endRiverEraseOp()
{
    delete _m_pRiverEraseOp;
    _m_pRiverEraseOp = NULL;
}

VA(0x004600a1, 0x1a)
void TMapDoc::eraseRiver(unsigned int x, unsigned int y)
{
    (*_m_pRiverEraseOp)(x, y, 1, 1);
}

VA(0x004600bb, 0x76)
void TMapDoc::startRoadEraseOp(bool bSecondLayer)
{
    backupMap();
    _m_pRoadEraseOp = new _TRoadEraseOp(this, bSecondLayer);
    if (_m_pRoadEraseOp == NULL)
        throw TAllocationFailure();
}

VA(0x00460131, 0x1c)
void TMapDoc::endRoadEraseOp()
{
    delete _m_pRoadEraseOp;
    _m_pRoadEraseOp = NULL;
}

VA(0x0046014d, 0x1a)
void TMapDoc::eraseRoad(unsigned int x, unsigned int y)
{
    (*_m_pRoadEraseOp)(x, y, 1, 1);
}

VA(0x00460167, 0x33)
void TMapDoc::onMapObjectRemoved(bool bSecondLayer, TMapLayerObjectID objID)
{
    TObjectRemovedParams params;
    params.m_bSecondLayer = bSecondLayer;
    params.m_objID = objID;
    TNotification notification;
    notification.m_type = TNotification::eObjectRemoved;
    notification.m_pObjectRemovedParams = &params;
    UpdateAllViews(NULL, (LPARAM)&notification);
}

VA(0x0046019a, 0x127)
std::auto_ptr<TGameMap> TMapDoc::_readMap(std::streambuf* pStreamBuf)
{
    TRawIStream stream(pStreamBuf);
    long fileVersion;
    stream >> fileVersion;
    const unsigned int kNumVersions = sizeof(akMapFileVersion) / sizeof(akMapFileVersion[0]);
    unsigned int version;
    for (version = 0; version < kNumVersions; version++)
        if (fileVersion == akMapFileVersion[version])
            break;
    if (version == kNumVersions)
        throw new TMapDocInvalidFileVersion(fileVersion, kMapFileVersion);
    std::auto_ptr<TGameMap> pMap(new TGameMap(this, EGameVersion(version), pStreamBuf, fileVersion));
    if (pMap.get() == NULL)
        throw TAllocationFailure();
    return pMap;
}

// Generates the new map's random map into memory and reads it back.
VA(0x004602c1, 0x29c)
bool TMapDoc::_generateRandomMap()
{
    const TRandomMapParams* pParams = _m_newMapParams.m_pRandomMapParams.get();
    std::string mapData;
    TRandomMapRequest request(TGameMap::getDimension(_m_newMapParams.m_size),
                              TGameMap::getDimension(_m_newMapParams.m_size),
                              _m_newMapParams.m_bTwoLayer ? 2 : 1);
    switch (_m_newMapParams.m_version) {
    case GAME_VERSION_ROE:
        request.m_mapVersion = RMG_MAP_RESTORATION_OF_ERATHIA;
        break;
    case GAME_VERSION_AB:
        request.m_mapVersion = RMG_MAP_ARMAGEDDONS_BLADE;
        break;
    case GAME_VERSION_SOD:
        request.m_mapVersion = RMG_MAP_SHADOW_OF_DEATH;
        break;
    }
    request.m_humanPlayerCount = pParams->m_humanPlayerCount;
    request.m_humanTeamCount = pParams->m_humanTeamCount;
    request.m_computerPlayerCount = pParams->m_computerPlayerCount;
    request.m_computerTeamCount = pParams->m_computerTeamCount;
    request.m_waterContent = pParams->m_waterContent;
    request.m_monsterStrength = pParams->m_monsterStrength;
    {
        std::stringbuf buf(std::ios_base::out);
        ERandomMapResult result;
        {
            TGzDeflateBuf deflateBuf(&buf);
            TStreamBufFile file(&deflateBuf);
            TMapGenerationProgressDlg dlg(AfxGetMainWnd());
            result = request.generateToFile(&file, &dlg);
        }
        if (result != RANDOM_MAP_OK) {
            AfxMessageBox(kUnableToGenerateRandomMapStr);
            return false;
        }
        mapData = buf.str();
    }
    std::stringbuf buf(mapData, std::ios_base::in);
    TGzInflateBuf inflateBuf(&buf);
    _m_pMap = _readMap(&inflateBuf).release();
    _m_revision = 0;
    _m_lastRevision = 1;
    SetModifiedFlag(TRUE);
    return true;
}

// A cell turned to water or rock loses its river and road; the map then
// reworks the terrain transitions around it.
VA(0x00460594, 0x121)
void TMapDoc::_onTerrainTypeChanged(bool bSecondLayer, unsigned int x, unsigned int y,
                                    TTerrainType oldType)
{
    TTileExtent extent;
    {
        TRiverMap riverMap(_m_pMap, bSecondLayer);
        TRiverEraseOp::onTerrainTypeChanged(&riverMap, TTilePoint(x, y));
        if (riverMap.getUpdateExtent(&extent))
            _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
    }
    {
        TRoadMap roadMap(_m_pMap, bSecondLayer);
        TRoadEraseOp::onTerrainTypeChanged(&roadMap, TTilePoint(x, y));
        if (roadMap.getUpdateExtent(&extent))
            _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
    }
    if (_m_pMap->onTerrainTypeChanged(bSecondLayer, x, y, oldType, &extent))
        _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
}

VA(0x004606b5, 0x5d)
void TMapDoc::_sendUpdate(bool bSecondLayer, unsigned int left, unsigned int top, unsigned int width,
                          unsigned int height)
{
    SetModifiedFlag(TRUE);
    CPoint topLeft(left, top);
    CSize size(width, height);
    TUpdateParams params(CRect(topLeft, size), bSecondLayer);
    TNotification notification;
    notification.m_type = TNotification::eUpdate;
    notification.m_pUpdateParams = &params;
    UpdateAllViews(NULL, (LPARAM)&notification);
}

// Saves an unchanging document to the autosave file once the autosave
// interval (in minutes) has passed, keeping its modified flag.
VA(0x00460712, 0x11e)
int TMapDoc::_autosave()
{
    if (_s_autosaveInterval > 0 && _m_pTerrainPlacementOp == NULL && _m_pRiverPlacementOp == NULL
        && _m_pRiverEraseOp == NULL && _m_pRoadPlacementOp == NULL && _m_pRoadEraseOp == NULL) {
        DWORD time = GetTickCount();
        DWORD interval = _s_autosaveInterval * 60000;
        DWORD elapsed = time - _m_lastSaveTime;
        if (elapsed >= interval) {
            _m_lastSaveTime += elapsed - elapsed % interval;
            if (_m_bOwnsAutosave) {
                int i = _m_autosavePathName.ReverseFind('\\');
                if (i != -1) {
                    bool bDirectory;
                    CString directory = _m_autosavePathName.Left(i);
                    WIN32_FIND_DATA findData;
                    HANDLE hFind = FindFirstFile(directory, &findData);
                    if (hFind != INVALID_HANDLE_VALUE) {
                        FindClose(hFind);
                        bDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    } else {
                        bDirectory = CreateDirectory(directory, NULL) != FALSE;
                    }
                    if (!bDirectory)
                        return 0;
                }
                BOOL bModified = IsModified();
                CDocument::OnSaveDocument(_m_autosavePathName);
                SetModifiedFlag(bModified);
            }
        }
    }
    return 0;
}

VA(0x00460830, 0xa5)
void TMapDoc::ReportSaveLoadException(LPCTSTR lpszPathName, CException* e, BOOL bSaving, UINT nIDPDefault)
{
    CString message;
    if (e->IsKindOf(RUNTIME_CLASS(TMapDocInvalidFileVersion))) {
        TMapDocInvalidFileVersion* pFailure = static_cast<TMapDocInvalidFileVersion*>(e);
        message.Format(kInvalidMapVersionFmtStr, pFailure->getVersion(), pFailure->getExpectedVersion());
    } else if (e->IsKindOf(RUNTIME_CLASS(TMapDocLoadFailure))) {
        message = kInvalidMapFileStr;
    } else {
        CDocument::ReportSaveLoadException(lpszPathName, e, bSaving, nIDPDefault);
        return;
    }
    AfxMessageBox(message, MB_ICONEXCLAMATION);
}

VA(0x004608d5, 0x17f)
void TMapDoc::OnFileExportText()
{
    CString pathName;
    if (!promptForFileName(m_strTitle, FALSE, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
                           pathName))
        return;
    CFileException fe;
    CStdioFile file;
    if (!file.Open(pathName, CFile::modeCreate | CFile::modeWrite | CFile::typeText, &fe)) {
        CDocument::ReportSaveLoadException(pathName, &fe, TRUE, AFX_IDP_INVALID_FILENAME);
        return;
    }
    TMFCFileBuf buf(&file);
    std::ostream stream(&buf);
    try {
        _m_pMap->exportText(&stream);
    } catch (CException* e) {
        CDocument::ReportSaveLoadException(pathName, e, TRUE, AFX_IDP_FAILED_TO_SAVE_DOC);
        e->Delete();
    }
}

VA(0x00460ab1, 0x2bb)
void TMapDoc::OnFileImportText()
{
    CString pathName;
    if (!promptForFileName(m_strTitle, TRUE, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, pathName))
        return;
    CFileException fe;
    CStdioFile file;
    if (!file.Open(pathName, CFile::modeRead | CFile::typeText, &fe)) {
        CDocument::ReportSaveLoadException(pathName, &fe, FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
        return;
    }
    TMFCFileBuf buf(&file);
    std::istream stream(&buf);
    TGameMap map(*_m_pMap);
    try {
        map.importText(&stream);
    } catch (TGameMap::TImportTextFailure&) {
        CString message;
        message.Format(kUnableToImportTextFmtStr, (LPCTSTR)pathName);
        AfxMessageBox(message, MB_ICONSTOP);
        return;
    } catch (CException* e) {
        CDocument::ReportSaveLoadException(pathName, e, FALSE, AFX_IDP_FAILED_TO_OPEN_DOC);
        e->Delete();
        return;
    }
    backupMap();
    *_m_pMap = map;
    SetModifiedFlag(TRUE);
    CString message;
    message.Format(kTextImportedSuccessfullyFmtStr, (LPCTSTR)pathName);
    AfxMessageBox(message);
}

VA(0x00460d81, 0x8a)
void TMapDoc::OnToolsRepaintMap()
{
    backupMap();
    TRepaintMapOp repaintOp;
    repaintOp(_m_pMap, false, _s_specialTileFrequency);
    if (_m_pMap->isTwoLayer())
        repaintOp(_m_pMap, true, _s_specialTileFrequency);
    SetModifiedFlag();
    UpdateAllViews(NULL);
}

// A new document asks for the new map's version, size and levels when
// the application wants it to (File New).
VA(0x00460e0b, 0xb1)
BOOL TMapDoc::SaveModified()
{
    bool bPrompt = _m_bPromptForNewMapParams;
    _m_bPromptForNewMapParams = false;
    if (!CDocument::SaveModified())
        return FALSE;
    if (bPrompt) {
        CWnd* pMainWnd = AfxGetMainWnd();
        TNewMapDlg dlg(pMainWnd, s_kDefaultNewMapParams);
        if (dlg.DoModal() != IDOK)
            return FALSE;
        setNewMapParams(dlg.getParams());
        if (pMainWnd != NULL)
            pMainWnd->UpdateWindow();
    }
    return TRUE;
}

void TMapDoc::OnFileBatchConvert()
{
}
