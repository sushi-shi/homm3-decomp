// GameMap.cpp - the map model (h3maped object from 0x41e5c5; Loki h3maped
// object 12). TGameMap and TGameMap::TLayer forward to their copy-on-write
// implementations, which this file defines.
//
// Ported so far: the first anonymous helper and the layer's cell lookup.
#include "editor/stdafx.h"

#include <assert.h>

#include "va.h"
#include "editor/Array.h"
#include "editor/GameMap.h"
#include "editor/TilePoint.h"

namespace {

VA(0x0041e799, 0x92)
bool isBeachBorder(const TGameMap::TLayer& layer, const TTilePoint& loc)
{
    if (layer.getCell(loc).getTerrainType() == eTerrainWater)
        return false;
    bool abAdjacent[8];
    computeAdjacentDirs(layer.getWidth(), layer.getHeight(), loc.x(), loc.y(), abAdjacent);
    for (unsigned int dir = 0; dir < 8; dir++) {
        if (!abAdjacent[dir])
            continue;
        TTilePoint adjLoc = TPoint<int>(loc) + akAdjOffset[dir];
        if (layer.getCell(adjLoc).getTerrainType() == eTerrainWater)
            return true;
    }
    return false;
}

}  // namespace

// The map's implementation: so far only the dimension of each size.
class TGameMap::_TImpl {
public:
    static const unsigned int _s_akDimension[TGameMap::s_kNumSizes];
};

DATA(0x00535214)
const unsigned int TGameMap::_TImpl::_s_akDimension[TGameMap::s_kNumSizes] = { 36, 72, 108, 144 };

// A layer's implementation: its size and its cells.
class TGameMap::TLayer::_TImpl {
public:
    unsigned int getWidth() const { return TGameMap::_TImpl::_s_akDimension[_m_size]; }
    unsigned int getHeight() const { return TGameMap::_TImpl::_s_akDimension[_m_size]; }
    const TCell* getPCell(unsigned int x, unsigned int y) const { return _m_pCellGrid->getPCell(x, y); }

private:
    // The layer's cells in shared, copy-on-write segments of
    // s_kSegmentDim x s_kSegmentDim (six on Windows: the lookup at
    // 0x42a4d0 divides by 6; Loki's port uses 9).
    class _TCellGrid {
    public:
        const TCell* getPCell(unsigned int x, unsigned int y) const;

    private:
        enum { s_kSegmentDim = 6 };
        typedef TArray<TArray<TCell, s_kSegmentDim>, s_kSegmentDim> _TSegment;

        unsigned int _m_widthInSegments;
        vector<TRefCountingPtr<_TSegment> > _m_aSegment;
    };

    TGameMap::TSize _m_size;
    TRefCountingPtr<_TCellGrid> _m_pCellGrid;
};

unsigned int TGameMap::TLayer::getWidth() const
{
    return _m_pImpl->getWidth();
}

unsigned int TGameMap::TLayer::getHeight() const
{
    return _m_pImpl->getHeight();
}

VA(0x0042a4d0, 0x4c)
const TGameMap::TLayer::TCell* TGameMap::TLayer::_TImpl::_TCellGrid::getPCell(unsigned int x,
                                                                             unsigned int y) const
{
    return &(*_m_aSegment[y / s_kSegmentDim * _m_widthInSegments + x / s_kSegmentDim])
        [y % s_kSegmentDim][x % s_kSegmentDim];
}

VA(0x0042b60f, 0x1b)
const TGameMap::TLayer::TCell* TGameMap::TLayer::getPCell(unsigned int x, unsigned int y) const
{
    return _m_pImpl->getPCell(x, y);
}
