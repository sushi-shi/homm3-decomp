// MapDoc.cpp - Loki h3maped object 57: the map document. It owns the map
// and its undo queue, runs the terrain, river, road and erase operations on
// it, loads and saves .h3m files (zlib-compressed raw streams, format
// version 14) and imports and exports the text form, and tells its view
// what changed. Every document is kept in the file-static allMapDocs set.
// Assert and throw lines come from the retail immediates.
#include "editor/stdafx.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fstream.h>
#include <string>

#include "exceptions.h"
#include "editor/MapDoc.h"
#include "editor/GameMap.h"
#include "editor/GUIGameObject.h"
#include "editor/MapEditorText.h"
#include "editor/MapView.h"
#include "editor/RawStream.h"
#include "GzBuf.h"

namespace {

set<TMapDoc*> allMapDocs;

}  // namespace

const unsigned int kMaxUndoQueueSize = 32;

unsigned int TMapDoc::_s_autosaveInterval = 0;
unsigned int TMapDoc::_s_specialTileFrequency = 4;
const TMapDoc::TNewMapParams TMapDoc::s_kDefaultParams = {TGameMap::TSize(1), true};

void TMapDoc::setAutosaveInterval(unsigned int interval)
{
    _s_autosaveInterval = interval;
}

void TMapDoc::setNewMapParams(TGameMap::TSize size, bool bTwoLayer)
{
    _m_newMapParams.m_size = size;
    _m_newMapParams.m_bTwoLayer = bTwoLayer;
}

void TMapDoc::setSpecialTileFrequency(unsigned int newFrequency)
{
#line 240
    assert(newFrequency <= s_kMaxSpecialTileFrequency);
    _s_specialTileFrequency = newFrequency;
}

TMapDoc::TMapDoc()
    : _m_pObjectFactory(NULL)
{
#line 293
    assert(false);
}

TMapDoc::TMapDoc(void* pParent, TMapView* mv)
    : m_bModified(false),
      m_pView(mv),
      _m_pObjectFactory(NULL),
      _m_pMap(NULL),
      _m_currentIndex(0),
      _m_savedIndex(kMaxUndoQueueSize),
      _m_bPromptForNewMapParams(false),
      _m_newMapParams(s_kDefaultParams),
      _m_bFlag5c(true),
      _m_bCreatingMap(false),
      _m_pTerrainPlacementOp(NULL),
      _m_pRiverPlacementOp(NULL),
      _m_pRiverEraseOp(NULL),
      _m_pRoadPlacementOp(NULL),
      _m_pRoadEraseOp(NULL),
      _m_bEraseUnderground(false)
{
#line 322
    assert(mv != NULL);
    _m_pObjectFactory = auto_ptr<TGameMap::TObjectFactory>(new TGUIGameObjectFactory);
    if (_m_pObjectFactory.get() == NULL)
#line 326
        throw TAllocationFailure(__FILE__, __LINE__);
    allMapDocs.insert(this);
    OnNewDocument();
}

TMapDoc::~TMapDoc()
{
#line 340
    assert(_m_pMap == NULL);
    assert(_m_pTerrainPlacementOp == NULL);
    assert(_m_pRiverPlacementOp == NULL);
    assert(_m_pRiverEraseOp == NULL);
    assert(_m_pRoadPlacementOp == NULL);
    assert(_m_pRoadEraseOp == NULL);
    allMapDocs.erase(this);
}

BOOL TMapDoc::OnNewDocument()
{
    _m_bFlag5c = false;
    CWaitCursor waitCursor;
    DeleteContents();
#line 395
    assert(_m_pMap == NULL);
    assert(_m_pTerrainPlacementOp == NULL);
    assert(_m_pRiverPlacementOp == NULL);
    assert(_m_pRiverEraseOp == NULL);
    assert(_m_pRoadPlacementOp == NULL);
    assert(_m_pRoadEraseOp == NULL);
    _m_bCreatingMap = true;
    if ((_m_pMap = new TGameMap(this, _m_pObjectFactory.get(), _m_newMapParams.m_size,
                                _m_newMapParams.m_bTwoLayer)) == NULL)
#line 407
        throw TAllocationFailure(__FILE__, __LINE__);
    {
        TTerrainPlacementOp op(this, _m_pMap, false, eTerrainWater, _s_specialTileFrequency);
        op(0, 0, _m_pMap->getWidth(), _m_pMap->getHeight());
    }
    if (_m_pMap->isTwoLayer()) {
        TTerrainPlacementOp op(this, _m_pMap, true, eTerrainRock, _s_specialTileFrequency);
        op(0, 0, _m_pMap->getWidth(), _m_pMap->getHeight());
    }
    _m_newMapParams = s_kDefaultParams;
    _m_bCreatingMap = false;
    SetModifiedFlag(false);
    return TRUE;
}

void TMapDoc::Serialize(char* pathName, bool bStoring)
{
#line 434
    assert(_m_pTerrainPlacementOp == NULL);
    assert(_m_pRiverPlacementOp == NULL);
    assert(_m_pRiverEraseOp == NULL);
    assert(_m_pRoadPlacementOp == NULL);
    assert(_m_pRoadEraseOp == NULL);
    int flags = bStoring ? O_WRONLY | O_CREAT | O_TRUNC : O_RDONLY;
    int mode = 0666;
    int fd = open(pathName, flags, mode);
    if (fd == -1)
        throw new TMapDocLoadFailure(true);
    filebuf buf(fd);
    if (bStoring) {
#line 452
        assert(_m_pMap != NULL);
        TGzDeflateBuf deflateBuf(&buf, -1, 0);
        TRawOStream stream(&deflateBuf);
        stream << long(kMapFileVersion);
        _m_pMap->save(&deflateBuf);
        SetModifiedFlag(false);
    } else {
#line 464
        assert(_m_pMap == NULL);
        try {
            TGzInflateBuf inflateBuf(&buf);
            TRawIStream stream(&inflateBuf);
            long version;
            stream >> version;
            if (version != kMapFileVersion)
                throw new TMapDocInvalidFileVersion(version, kMapFileVersion, true);
            _m_pMap = new TGameMap(this, _m_pObjectFactory.get(), &inflateBuf, version);
        } catch (exception& e) {
            close(fd);
            throw new TMapDocLoadFailure(true);
        }
        close(fd);
        SetModifiedFlag(false);
        if (_m_pMap == NULL)
#line 492
            throw TAllocationFailure(__FILE__, __LINE__);
    }
}

void TMapDoc::DeleteContents()
{
#line 517
    assert(_m_pTerrainPlacementOp == NULL);
    assert(_m_pRiverPlacementOp == NULL);
    assert(_m_pRiverEraseOp == NULL);
    assert(_m_pRoadPlacementOp == NULL);
    assert(_m_pRoadEraseOp == NULL);
    if (_m_pMap != NULL) {
        TNotification notification;
        notification.m_type = TNotification::eClear;
        UpdateAllViews(NULL, (unsigned long)&notification);
        delete _m_pMap;
        _m_pMap = NULL;
    }
    _m_undoQueue.clear();
    _m_currentIndex = 0;
    _m_savedIndex = kMaxUndoQueueSize;
}

BOOL TMapDoc::OnOpenDocument(char* pathName)
{
#line 543
    assert(_m_pTerrainPlacementOp == NULL);
    assert(_m_pRiverPlacementOp == NULL);
    assert(_m_pRiverEraseOp == NULL);
    assert(_m_pRoadPlacementOp == NULL);
    assert(_m_pRoadEraseOp == NULL);
    DeleteContents();
    Serialize(pathName, false);
    return TRUE;
}

bool TMapDoc::OnSaveDocument(char* pathName)
{
    Serialize(pathName, true);
    return false;
}

void TMapDoc::undo()
{
#line 577
    assert(canUndo());
    assert(_m_pMap != NULL);
    assert(_m_undoQueue.size() <= kMaxUndoQueueSize);
    assert(_m_currentIndex <= _m_undoQueue.size());
    assert(_m_pTerrainPlacementOp == NULL);
    assert(_m_pRiverPlacementOp == NULL);
    assert(_m_pRiverEraseOp == NULL);
    assert(_m_pRoadPlacementOp == NULL);
    assert(_m_pRoadEraseOp == NULL);
    swap(*_m_pMap, _m_undoQueue[--_m_currentIndex]);
    if (!IsModified()) {
        _m_savedIndex = _m_currentIndex;
        SetModifiedFlag();
    } else if (_m_savedIndex == _m_currentIndex) {
        _m_savedIndex = kMaxUndoQueueSize;
        SetModifiedFlag(false);
    }
    TNotification notification;
    notification.m_type = TNotification::eReplace;
    UpdateAllViews(NULL, (unsigned long)&notification);
}

void TMapDoc::redo()
{
#line 608
    assert(canRedo());
    assert(_m_pMap != NULL);
    assert(_m_undoQueue.size() <= kMaxUndoQueueSize);
    assert(_m_pTerrainPlacementOp == NULL);
    assert(_m_pRiverPlacementOp == NULL);
    assert(_m_pRiverEraseOp == NULL);
    assert(_m_pRoadPlacementOp == NULL);
    assert(_m_pRoadEraseOp == NULL);
    swap(*_m_pMap, _m_undoQueue[_m_currentIndex]);
    if (!IsModified()) {
        _m_savedIndex = _m_currentIndex;
        SetModifiedFlag();
    } else if (_m_savedIndex == _m_currentIndex) {
        _m_savedIndex = kMaxUndoQueueSize;
        SetModifiedFlag(false);
    }
    _m_currentIndex++;
    TNotification notification;
    notification.m_type = TNotification::eReplace;
    UpdateAllViews(NULL, (unsigned long)&notification);
}

void TMapDoc::backupMap()
{
#line 640
    assert(_m_pMap != NULL);
    backupMap(*_m_pMap);
}

void TMapDoc::backupMap(const TGameMap& map)
{
#line 648
    assert(_m_undoQueue.size() <= kMaxUndoQueueSize);
    assert(_m_currentIndex <= _m_undoQueue.size());
    assert(_m_pTerrainPlacementOp == NULL);
    assert(_m_pRiverPlacementOp == NULL);
    assert(_m_pRiverEraseOp == NULL);
    assert(_m_pRoadPlacementOp == NULL);
    assert(_m_pRoadEraseOp == NULL);
    if (_m_currentIndex < _m_undoQueue.size()) {
        _m_undoQueue.erase(_m_undoQueue.begin() + _m_currentIndex, _m_undoQueue.end());
        if (_m_savedIndex >= _m_currentIndex)
            _m_savedIndex = kMaxUndoQueueSize;
    }
    if (_m_undoQueue.size() == kMaxUndoQueueSize) {
        _m_undoQueue.pop_front();
        _m_currentIndex--;
        if (_m_savedIndex == 0)
            _m_savedIndex = kMaxUndoQueueSize;
        else if (_m_savedIndex < kMaxUndoQueueSize)
            _m_savedIndex--;
    }
    if (!IsModified())
        _m_savedIndex = _m_currentIndex;
    _m_undoQueue.push_back(map);
    _m_currentIndex++;
}

TMapLayerObjectID TMapDoc::placeObject(bool bSecondLayer, const TGameObject& obj, unsigned int x,
                                       unsigned int y)
{
#line 684
    assert(_m_pMap != NULL);
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    TMapLayerObjectID objID;
    TTileExtent extent;
    objID = _m_pMap->placeObject(bSecondLayer, obj, x, y, &extent);
    if (objID != TGameMap::TLayer::s_kInvalidObjID)
        _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
    return objID;
}

void TMapDoc::removeObject(bool bSecondLayer, unsigned int objID)
{
#line 701
    assert(_m_pMap != NULL);
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    TTileExtent extent;
    _m_pMap->removeObject(bSecondLayer, objID, &extent);
    _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
}

void TMapDoc::floatObject(bool bSecondLayer, unsigned int objID)
{
#line 714
    assert(_m_pMap != NULL);
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    TTileExtent extent;
    _m_pMap->floatObject(bSecondLayer, objID, &extent);
    _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
}

void TMapDoc::unfloatObject(bool bSecondLayer, unsigned int x, unsigned int y)
{
#line 727
    assert(_m_pMap != NULL);
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    TTileExtent extent;
    _m_pMap->unfloatObject(bSecondLayer, x, y, &extent);
    _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
}

void TMapDoc::onObjectPropertiesChanged(bool bSecondLayer, unsigned int objID)
{
#line 740
    assert(_m_pMap != NULL);
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    TGameMap::TLayer* pLayer = _m_pMap->getPLayer(bSecondLayer);
    TTileExtent extent = pLayer->getObjectExtent(objID);
    _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
}

void TMapDoc::startTerrainPlacementOp(bool bSecondLayer, TTerrainType terrainType)
{
#line 752
    assert(_m_pMap != NULL);
    assert(_m_pTerrainPlacementOp == NULL);
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    backupMap();
    _m_pTerrainPlacementOp
        = new TTerrainPlacementOp(this, _m_pMap, bSecondLayer, terrainType, _s_specialTileFrequency);
    if (_m_pTerrainPlacementOp == NULL)
#line 760
        throw TAllocationFailure(__FILE__, __LINE__);
}

void TMapDoc::endTerrainPlacementOp()
{
#line 766
    assert(_m_pMap != NULL);
    assert(_m_pTerrainPlacementOp != NULL);
    delete _m_pTerrainPlacementOp;
    _m_pTerrainPlacementOp = NULL;
}

void TMapDoc::terrainFill(unsigned int left, unsigned int top, unsigned int width, unsigned int height)
{
#line 776
    assert(_m_pMap != NULL);
    assert(_m_pTerrainPlacementOp != NULL);
    (*_m_pTerrainPlacementOp)(left, top, width, height);
}

void TMapDoc::startRiverPlacementOp(bool bSecondLayer, TRiverType riverType, unsigned int x,
                                    unsigned int y)
{
#line 785
    assert(_m_pMap != NULL);
    assert(_m_pRiverPlacementOp == NULL);
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    backupMap();
    _m_pRiverPlacementOp = new TRiverPlacementOp(this, _m_pMap, bSecondLayer, riverType, x, y);
    if (_m_pRiverPlacementOp == NULL)
#line 793
        throw TAllocationFailure(__FILE__, __LINE__);
}

void TMapDoc::endRiverPlacementOp()
{
#line 799
    assert(_m_pMap != NULL);
    assert(_m_pRiverPlacementOp != NULL);
    delete _m_pRiverPlacementOp;
    _m_pRiverPlacementOp = NULL;
}

void TMapDoc::placeRiver(unsigned int x, unsigned int y)
{
#line 809
    assert(_m_pMap != NULL);
    assert(_m_pRiverPlacementOp != NULL);
    (*_m_pRiverPlacementOp)(x, y);
}

void TMapDoc::startRoadPlacementOp(bool bSecondLayer, TRoadType roadType, unsigned int x,
                                   unsigned int y)
{
#line 818
    assert(_m_pMap != NULL);
    assert(_m_pRoadPlacementOp == NULL);
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    backupMap();
    _m_pRoadPlacementOp = new TRoadPlacementOp(this, _m_pMap, bSecondLayer, roadType, x, y);
    if (_m_pRoadPlacementOp == NULL)
#line 826
        throw TAllocationFailure(__FILE__, __LINE__);
}

void TMapDoc::endRoadPlacementOp()
{
#line 832
    assert(_m_pMap != NULL);
    assert(_m_pRoadPlacementOp != NULL);
    delete _m_pRoadPlacementOp;
    _m_pRoadPlacementOp = NULL;
}

void TMapDoc::placeRoad(unsigned int x, unsigned int y)
{
#line 842
    assert(_m_pMap != NULL);
    assert(_m_pRoadPlacementOp != NULL);
    (*_m_pRoadPlacementOp)(x, y);
}

void TMapDoc::startEraseOp(bool bSecondLayer)
{
#line 851
    assert(_m_pMap != NULL);
    assert(_m_pRiverEraseOp == NULL);
    assert(_m_pRoadEraseOp == NULL);
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    backupMap();
    _m_bEraseUnderground = bSecondLayer;
    _m_pRiverEraseOp = new TRiverEraseOp(this, _m_pMap, bSecondLayer);
    if (_m_pRiverEraseOp == NULL)
#line 862
        throw TAllocationFailure(__FILE__, __LINE__);
    try {
        _m_pRoadEraseOp = new TRoadEraseOp(this, _m_pMap, bSecondLayer);
        if (_m_pRoadEraseOp == NULL)
#line 868
            throw TAllocationFailure(__FILE__, __LINE__);
    } catch (...) {
        delete _m_pRiverEraseOp;
        _m_pRiverEraseOp = NULL;
        throw;
    }
}

void TMapDoc::endEraseOp()
{
#line 881
    assert(_m_pMap != NULL);
    assert(_m_pRiverEraseOp != NULL);
    assert(_m_pRoadEraseOp != NULL);
    delete _m_pRoadEraseOp;
    _m_pRoadEraseOp = NULL;
    delete _m_pRiverEraseOp;
    _m_pRiverEraseOp = NULL;
}

void TMapDoc::erase(unsigned int left, unsigned int top, unsigned int width, unsigned int height)
{
#line 895
    assert(_m_pMap != NULL);
    assert(_m_pRiverEraseOp != NULL);
    assert(_m_pRoadEraseOp != NULL);
    (*_m_pRiverEraseOp)(left, top, width, height);
    (*_m_pRoadEraseOp)(left, top, width, height);
    TGameMap::TLayer* pLayer = _m_pMap->getPLayer(_m_bEraseUnderground);
    for (unsigned int y = top; y < top + height; y++)
        for (unsigned int x = left; x < left + width; x++)
            while (pLayer->getNumObjectIDsAtCell(x, y) != 0)
                removeObject(_m_bEraseUnderground, pLayer->getObjectIDAtCell(x, y, 0));
}

void TMapDoc::onMapObjectRemoved(bool bSecondLayer, TMapLayerObjectID objID)
{
#line 918
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    TObjectRemovedParams params;
    params.m_bSecondLayer = bSecondLayer;
    params.m_objID = objID;
    TNotification notification;
    notification.m_type = TNotification::eObjectRemoved;
    notification.m_pObjectRemovedParams = &params;
    UpdateAllViews(NULL, (unsigned long)&notification);
}

void TMapDoc::onTerrainTypeChanged(bool bSecondLayer, unsigned int x, unsigned int y,
                                   TTerrainType newType)
{
#line 933
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    TRiverEraseOp::onTerrainTypeChanged(this, _m_pMap, bSecondLayer, x, y);
    TRoadEraseOp::onTerrainTypeChanged(this, _m_pMap, bSecondLayer, x, y);
    TTileExtent extent;
    if (_m_pMap->onTerrainTypeChanged(bSecondLayer, x, y, newType, &extent))
        _sendUpdate(bSecondLayer, extent.left(), extent.top(), extent.width(), extent.height());
}

void TMapDoc::onTerrainUpdated(bool bSecondLayer, unsigned int left, unsigned int top,
                               unsigned int width, unsigned int height)
{
#line 946
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    if (!_m_bCreatingMap)
        _sendUpdate(bSecondLayer, left, top, width, height);
}

void TMapDoc::onRiversUpdated(bool bSecondLayer, unsigned int left, unsigned int top,
                              unsigned int width, unsigned int height)
{
#line 955
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    if (!_m_bCreatingMap)
        _sendUpdate(bSecondLayer, left, top, width, height);
}

void TMapDoc::onPlacingRiver(bool bSecondLayer, unsigned int x, unsigned int y)
{
#line 964
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
}

void TMapDoc::onRoadsUpdated(bool bSecondLayer, unsigned int left, unsigned int top,
                             unsigned int width, unsigned int height)
{
#line 970
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    if (!_m_bCreatingMap)
        _sendUpdate(bSecondLayer, left, top, width, height);
}

void TMapDoc::onPlacingRoad(bool bSecondLayer, unsigned int x, unsigned int y)
{
#line 979
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
}

void TMapDoc::_sendUpdate(bool bSecondLayer, unsigned int left, unsigned int top, unsigned int width,
                          unsigned int height)
{
#line 985
    assert(!bSecondLayer || _m_pMap->isTwoLayer());
    SetModifiedFlag();
    CPoint topLeft(left, top);
    CSize size(width, height);
    TUpdateParams params(CRect(topLeft, size), bSecondLayer);
    TNotification notification;
    notification.m_type = TNotification::eUpdate;
    notification.m_pUpdateParams = &params;
    UpdateAllViews(NULL, (unsigned long)&notification);
}

void TMapDoc::ReportSaveLoadException(char* pathName)
{
    char message[512];
    strcpy(message, "Save/Load exception: ");
    strcat(message, pathName);
    doMessageBox(message);
}

void TMapDoc::OnFileExportText(char* pathName)
{
    int fd = open(pathName, O_WRONLY | O_CREAT, 0600);
    if (fd < 0) {
        ReportSaveLoadException(pathName);
        return;
    }
    filebuf buf(fd);
    ostream stream(&buf);
    try {
        _m_pMap->exportText(&stream);
    } catch (...) {
        ReportSaveLoadException(pathName);
    }
    flush(stream);
    close(fd);
}

void TMapDoc::OnFileImportText(char* pathName)
{
    int fd = open(pathName, O_RDONLY);
    if (fd < 0) {
        ReportSaveLoadException(pathName);
        return;
    }
    filebuf buf(fd);
    istream stream(&buf);
    TGameMap map = *_m_pMap;
    try {
        map.importText(&stream);
    } catch (TGameMap::TImportTextFailure& e) {
        char message[512];
        snprintf(message, sizeof(message), kUnableToImportTextFmtStr, pathName);
        close(fd);
        doMessageBox(message);
        return;
    } catch (...) {
        ReportSaveLoadException(pathName);
        close(fd);
        return;
    }
    close(fd);
    backupMap();
    *_m_pMap = map;
    SetModifiedFlag();
    char message[512];
    snprintf(message, sizeof(message), kTextImportedSuccessfullyFmtStr, pathName);
    doMessageBox(message);
}

void TMapDoc::OnToolsRepaintMap()
{
    backupMap();
    TTerrainPlacementOp::repaintMap(_m_pMap, _s_specialTileFrequency);
    SetModifiedFlag();
    UpdateAllViews(NULL);
}

void TMapDoc::OnFileBatchConvert()
{
}

bool TMapDoc::SaveModified()
{
    bool bPrompt = _m_bPromptForNewMapParams;
    _m_bPromptForNewMapParams = false;
    if (bPrompt)
        g_warning("TMapDoc::SaveModified(); We should fire up a TNewMapDlg here...\n");
    return true;
}
