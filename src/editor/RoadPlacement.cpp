// RoadPlacement.cpp - Loki h3maped object 28: road placement. TRoadOp
// shows a map layer's roads to the line engine as a TMapLineFilter (road
// type, tile number and flips per cell; water and rock block roads); the
// placement and erase operations drive TLinePlacementOp and TLineEraseOp
// over it and report the changed rectangle to their client. Assert and
// throw lines come from the retail immediates; the tileset's name is not
// proven.
#include <assert.h>
#include <vector>

#include "editor/RoadPlacement.h"
#include "exceptions.h"

// The road tileset: the line shape of each of its 17 tiles.
static const TLineShape akRoadTileLineShape[] = {
    eLS_es, eLS_es, eLS_esDiag, eLS_esDiag, eLS_esDiag, eLS_esDiag, eLS_nes, eLS_nes, eLS_esw,
    eLS_esw, eLS_ns, eLS_ns, eLS_ew, eLS_ew, eLS_s, eLS_e, eLS_nesw
};

static TLineTilesetTraits roadTilesetTraits(
    sizeof(akRoadTileLineShape) / sizeof(akRoadTileLineShape[0]), akRoadTileLineShape);

TRoadOp::TRoadOp(TGameMap* pMap, bool bSecondLayer)
    : TMapLineFilter(pMap->getWidth(), pMap->getHeight()), _m_pMap(pMap), _m_bSecondLayer(bSecondLayer)
{
#line 61
    assert(_m_pMap != NULL);
    assert(!_m_bSecondLayer || _m_pMap->isTwoLayer());
}

TRoadOp::~TRoadOp()
{
}

const TLineTilesetTraits& TRoadOp::getTilesetTraits(unsigned int type) const
{
    return roadTilesetTraits;
}

void TRoadOp::_setCellInfo(const TTilePoint& loc, const TCellInfo& cellInfo)
{
#line 79
    assert(cellInfo.m_type >= 0 && cellInfo.m_type < kNumRoadTypes);
    TGameMap::TLayer::TCell* pCell = _getPMapLayer()->getPCell(loc);
    pCell->setRoadType(TRoadType(cellInfo.m_type));
    pCell->setRoadTileNum(cellInfo.m_tileNum);
    pCell->setBRoadHFlipped(cellInfo.m_bHFlipped);
    pCell->setBRoadVFlipped(cellInfo.m_bVFlipped);
}

void TRoadOp::_setCellType(const TTilePoint& loc, unsigned int type)
{
#line 90
    assert(type >= 0 && type < kNumRoadTypes);
    _getPMapLayer()->getPCell(loc)->setRoadType(TRoadType(type));
}

void TRoadOp::_setCellTileNum(const TTilePoint& loc, unsigned int tileNum)
{
    _getPMapLayer()->getPCell(loc)->setRoadTileNum(tileNum);
}

void TRoadOp::_setCellBHFlipped(const TTilePoint& loc, bool bHFlipped)
{
    _getPMapLayer()->getPCell(loc)->setBRoadHFlipped(bHFlipped);
}

void TRoadOp::_setCellBVFlipped(const TTilePoint& loc, bool bVFlipped)
{
    _getPMapLayer()->getPCell(loc)->setBRoadVFlipped(bVFlipped);
}

bool TRoadOp::_isCellBlocked(const TTilePoint& loc) const
{
    TTerrainType terrainType = _getMapLayer().getCell(loc).getTerrainType();
    return terrainType == eTerrainWater || terrainType == eTerrainRock;
}

void TRoadOp::_getCellInfo(const TTilePoint& loc, TCellInfo* pCellInfo) const
{
#line 122
    assert(pCellInfo != NULL);
    const TGameMap::TLayer::TCell& cell = _getMapLayer().getCell(loc);
    pCellInfo->m_type = cell.getRoadType();
    pCellInfo->m_tileNum = cell.getRoadTileNum();
    pCellInfo->m_bHFlipped = cell.getBRoadHFlipped();
    pCellInfo->m_bVFlipped = cell.getBRoadVFlipped();
}

unsigned int TRoadOp::_getCellType(const TTilePoint& loc) const
{
    return _getMapLayer().getCell(loc).getRoadType();
}

unsigned int TRoadOp::_getCellTileNum(const TTilePoint& loc) const
{
    return _getMapLayer().getCell(loc).getRoadTileNum();
}

bool TRoadOp::_getCellBHFlipped(const TTilePoint& loc) const
{
    return _getMapLayer().getCell(loc).getBRoadHFlipped();
}

bool TRoadOp::_getCellBVFlipped(const TTilePoint& loc) const
{
    return _getMapLayer().getCell(loc).getBRoadHFlipped();
}

TRoadPlacementOp::TRoadPlacementOp(TRoadPlacementOpClient* pClient, TGameMap* pMap,
                                     bool bSecondLayer, TRoadType roadType, unsigned int x,
                                     unsigned int y)
    : TRoadOp(pMap, bSecondLayer), _m_pClient(pClient)
{
#line 159
    assert(pClient != NULL);
    if ((_m_pLinePlacementOp = new TLinePlacementOp(this, _getMapLineFilter(), roadType, x, y))
        == NULL)
#line 164
        throw TAllocationFailure(__FILE__, __LINE__);
}

TRoadPlacementOp::~TRoadPlacementOp()
{
#line 170
    assert(_m_pLinePlacementOp != NULL);
    delete _m_pLinePlacementOp;
}

void TRoadPlacementOp::onLinesUpdated(unsigned int left, unsigned int top, unsigned int width,
                                       unsigned int height)
{
    _m_pClient->onRoadsUpdated(_m_bSecondLayer, left, top, width, height);
}

void TRoadPlacementOp::onPlacingLine(unsigned int x, unsigned int y)
{
    _m_pClient->onPlacingRoad(_m_bSecondLayer, x, y);
}

void TRoadEraseOp::onTerrainTypeChanged(TRoadOpClient* pClient, TGameMap* pMap, bool bSecondLayer,
                                         unsigned int x, unsigned int y)
{
    const TGameMap::TLayer::TCell& cell = pMap->getLayer(bSecondLayer).getCell(x, y);
    TRoadType roadType = cell.getRoadType();
    TTerrainType terrainType = cell.getTerrainType();
    if (roadType != TRoadType(kLineTypeNone)
        && (terrainType == eTerrainWater || terrainType == eTerrainRock)) {
        TRoadEraseOp eraseOp(pClient, pMap, bSecondLayer);
        eraseOp(x, y, 1, 1);
    }
}

TRoadEraseOp::TRoadEraseOp(TRoadOpClient* pClient, TGameMap* pMap, bool bSecondLayer)
    : TRoadOp(pMap, bSecondLayer), _m_pClient(pClient), _m_lineEraseOp(this, _getMapLineFilter())
{
#line 212
    assert(_m_pClient != NULL);
}

void TRoadEraseOp::onLinesUpdated(unsigned int left, unsigned int top, unsigned int width,
                                   unsigned int height)
{
    _m_pClient->onRoadsUpdated(_m_bSecondLayer, left, top, width, height);
}
