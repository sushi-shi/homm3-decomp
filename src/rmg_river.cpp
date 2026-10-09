// rmg_river.cpp - the random-map generator's river placement operation.
//
// Retail keeps the river pattern table's initializer and cleanup and the
// river operation together at 0x55ed70..0x55eef1, after resourcemanager's
// code: one object of its own in the original link order. The original file
// name is unknown. The pattern table's shared constructor and destructor stay
// out of line in rmg_support.cpp, so the cleanup keeps its destructor call.
#include "va.h"

#include "rmg.h"
#include "rmg_terrain.h"
#include "tiles.h"

// Complete-only pattern globals: retail cinit 0x55ed70/0x55f2f0 passes the
// array and count to the shared support constructor, then registers cleanup
// at 0x55ed90/0x55f310. Both cleanups retain the support destructor. Defining
// these beside that destructor instead expands delete into the callbacks;
// keep the ordinary library boundary, with no inline controls or new helpers.
DATA(0x00641140)
static const int g_rmgRiverPatterns[13] = {
    4, 4, 4, 4, 8, 7, 7, 6, 6, 2, 2, 3, 3
};
DATA(0x0069E5D0)
TRmgLinePatternTable g_rmgRiverPatternTable(13, g_rmgRiverPatterns);

VA_COMPGEN(0x0055ED70, 0x1D, STATIC_CTOR, g_rmgRiverPatternTable)

VA_COMPGEN(0x0055ED90, 0x0A, STATIC_DTOR, g_rmgRiverPatternTable)

VA(0x0055eda0, 0x07)
MAC_ADDRESS(0x253ccc, 0x60)
TRiverPlacementOp::~TRiverPlacementOp()
{
}

// All river types use the same pattern table, so the argument is ignored.
VA(0x0055edb0, 0x08)
MAC_ADDRESS(0x253ad8, 0x8)  // vtables 0x641174/0x641190; Complete-only
TRmgLinePatternTable* TRiverOp::getPatternTable(s32)
{
    return &g_rmgRiverPatternTable;
}

VA(0x0055edc0, 0x36)
MAC_ADDRESS(0x253ae0, 0x54) // anchor-vtable 0x641174/0x641190/0x6411f0/0x64120c +4
void TRiverOp::setTile(const TRmgGridPoint& point, const TRmgTerrainTile& tile)
{
    TRmgTerrainTile snapshot(tile.m_terrain, tile.m_frame);
    snapshot.m_flipX = tile.m_flipX;
    snapshot.m_flipY = tile.m_flipY;
    m_adapter->setTile(point, snapshot);
}

MAC_ADDRESS(0x253b34, 0x30)
void TRiverOp::setLineType(const TRmgGridPoint& point, s32 value)
{
    m_adapter->setLineType(point, value);
}

MAC_ADDRESS(0x253ba8, 0x88)
void TRiverOp::getTile(const TRmgGridPoint& point, TRmgTerrainTile& tile)
{
    TRmgTerrainTile snapshot = m_adapter->getTile(point);
    tile = snapshot;
}

// Roads and rivers cannot be painted over water or rock terrain.
VA(0x0055ee00, 0x28)
MAC_ADDRESS(0x253b64, 0x44)  // vtables 0x641174/0x641190/0x6411f0/0x64120c
s32 TRiverOp::isBlocked(const TRmgGridPoint& point)
{
    s32 terrain = m_adapter->getTerrain(point);
    if (terrain == eTerrainWater || terrain == eTerrainRock)
        return 1;
    return 0;
}

VA(0x0055ee30, 0x13)
MAC_ADDRESS(0x253c30, 0x30)
s32 TRiverOp::getLineType(const TRmgGridPoint& point)
{
    return m_adapter->getLineType(point);
}

VA(0x0055ee50, 0x76)
MAC_ADDRESS(0x253c60, 0x6c)
TRiverPlacementOp::TRiverPlacementOp(
    TRiverOp::TAbstractMap* newAdapter,
    s32 newRiverType,
    const TRmgGridPoint& newStart)
    : TRiverOp(newAdapter),
      m_walker(this, newRiverType, newStart)
{
}

VA_COMPGEN(0x0055eed0, 0x21, SCALAR_DELETING_DTOR, TRiverPlacementOp)
