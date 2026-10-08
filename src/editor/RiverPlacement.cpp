// RiverPlacement.cpp - Loki h3maped object 27: river placement. TRiverOp
// shows a map layer's rivers to the line engine as a TMapLineFilter (river
// type, tile number and flips per cell; water and rock block rivers); the
// placement and erase operations drive TLinePlacementOp and TLineEraseOp
// over it and report the changed rectangle to their client. Assert and
// throw lines come from the retail immediates; the tileset's name is not
// proven.
#include <assert.h>
#include <vector>

#include "editor/RiverPlacement.h"
#include "exceptions.h"

// The river tileset: the line shape of each of its 13 tiles.
static const TLineShape akRiverTileLineShape[] = {
    eLS_es, eLS_es, eLS_es, eLS_es, eLS_nesw, eLS_esw, eLS_esw, eLS_nes, eLS_nes,
    eLS_ns, eLS_ns, eLS_ew, eLS_ew
};

static TLineTilesetTraits riverTilesetTraits(
    sizeof(akRiverTileLineShape) / sizeof(akRiverTileLineShape[0]), akRiverTileLineShape);

TRiverOp::TRiverOp(TGameMap* pMap, bool bSecondLayer)
    : TMapLineFilter(pMap->getWidth(), pMap->getHeight()), _m_pMap(pMap), _m_bSecondLayer(bSecondLayer)
{
#line 57
    assert(_m_pMap != NULL);
    assert(!_m_bSecondLayer || _m_pMap->isTwoLayer());
}

TRiverOp::~TRiverOp()
{
}

const TLineTilesetTraits& TRiverOp::getTilesetTraits(unsigned int type) const
{
    return riverTilesetTraits;
}

void TRiverOp::_setCellInfo(const TTilePoint& loc, const TCellInfo& cellInfo)
{
#line 75
    assert(cellInfo.m_type >= 0 && cellInfo.m_type < kNumRiverTypes);
    TGameMap::TLayer::TCell* pCell = _getPMapLayer()->getPCell(loc);
    pCell->setRiverType(TRiverType(cellInfo.m_type));
    pCell->setRiverTileNum(cellInfo.m_tileNum);
    pCell->setBRiverHFlipped(cellInfo.m_bHFlipped);
    pCell->setBRiverVFlipped(cellInfo.m_bVFlipped);
}

void TRiverOp::_setCellType(const TTilePoint& loc, unsigned int type)
{
#line 86
    assert(type >= 0 && type < kNumRiverTypes);
    _getPMapLayer()->getPCell(loc)->setRiverType(TRiverType(type));
}

void TRiverOp::_setCellTileNum(const TTilePoint& loc, unsigned int tileNum)
{
    _getPMapLayer()->getPCell(loc)->setRiverTileNum(tileNum);
}

void TRiverOp::_setCellBHFlipped(const TTilePoint& loc, bool bHFlipped)
{
    _getPMapLayer()->getPCell(loc)->setBRiverHFlipped(bHFlipped);
}

void TRiverOp::_setCellBVFlipped(const TTilePoint& loc, bool bVFlipped)
{
    _getPMapLayer()->getPCell(loc)->setBRiverVFlipped(bVFlipped);
}

bool TRiverOp::_isCellBlocked(const TTilePoint& loc) const
{
    TTerrainType terrainType = _getMapLayer().getCell(loc).getTerrainType();
    return terrainType == eTerrainWater || terrainType == eTerrainRock;
}

void TRiverOp::_getCellInfo(const TTilePoint& loc, TCellInfo* pCellInfo) const
{
#line 118
    assert(pCellInfo != NULL);
    const TGameMap::TLayer::TCell& cell = _getMapLayer().getCell(loc);
    pCellInfo->m_type = cell.getRiverType();
    pCellInfo->m_tileNum = cell.getRiverTileNum();
    pCellInfo->m_bHFlipped = cell.getBRiverHFlipped();
    pCellInfo->m_bVFlipped = cell.getBRiverVFlipped();
}

unsigned int TRiverOp::_getCellType(const TTilePoint& loc) const
{
    return _getMapLayer().getCell(loc).getRiverType();
}

unsigned int TRiverOp::_getCellTileNum(const TTilePoint& loc) const
{
    return _getMapLayer().getCell(loc).getRiverTileNum();
}

bool TRiverOp::_getCellBHFlipped(const TTilePoint& loc) const
{
    return _getMapLayer().getCell(loc).getBRiverHFlipped();
}

bool TRiverOp::_getCellBVFlipped(const TTilePoint& loc) const
{
    return _getMapLayer().getCell(loc).getBRiverHFlipped();
}

TRiverPlacementOp::TRiverPlacementOp(TRiverPlacementOpClient* pClient, TGameMap* pMap,
                                     bool bSecondLayer, TRiverType riverType, unsigned int x,
                                     unsigned int y)
    : TRiverOp(pMap, bSecondLayer), _m_pClient(pClient)
{
#line 155
    assert(pClient != NULL);
    if ((_m_pLinePlacementOp = new TLinePlacementOp(this, _getMapLineFilter(), riverType, x, y))
        == NULL)
#line 160
        throw TAllocationFailure(__FILE__, __LINE__);
}

TRiverPlacementOp::~TRiverPlacementOp()
{
#line 166
    assert(_m_pLinePlacementOp != NULL);
    delete _m_pLinePlacementOp;
}

void TRiverPlacementOp::onLinesUpdated(unsigned int left, unsigned int top, unsigned int width,
                                       unsigned int height)
{
    _m_pClient->onRiversUpdated(_m_bSecondLayer, left, top, width, height);
}

void TRiverPlacementOp::onPlacingLine(unsigned int x, unsigned int y)
{
    _m_pClient->onPlacingRiver(_m_bSecondLayer, x, y);
}

void TRiverEraseOp::onTerrainTypeChanged(TRiverOpClient* pClient, TGameMap* pMap, bool bSecondLayer,
                                         unsigned int x, unsigned int y)
{
    const TGameMap::TLayer::TCell& cell = pMap->getLayer(bSecondLayer).getCell(x, y);
    TRiverType riverType = cell.getRiverType();
    TTerrainType terrainType = cell.getTerrainType();
    if (riverType != TRiverType(kLineTypeNone)
        && (terrainType == eTerrainWater || terrainType == eTerrainRock)) {
        TRiverEraseOp eraseOp(pClient, pMap, bSecondLayer);
        eraseOp(x, y, 1, 1);
    }
}

TRiverEraseOp::TRiverEraseOp(TRiverOpClient* pClient, TGameMap* pMap, bool bSecondLayer)
    : TRiverOp(pMap, bSecondLayer), _m_pClient(pClient), _m_lineEraseOp(this, _getMapLineFilter())
{
#line 208
    assert(_m_pClient != NULL);
}

void TRiverEraseOp::onLinesUpdated(unsigned int left, unsigned int top, unsigned int width,
                                   unsigned int height)
{
    _m_pClient->onRiversUpdated(_m_bSecondLayer, left, top, width, height);
}
