// rmg_road.cpp - the random-map generator's road placement operation.
//
// Retail keeps the road pattern table's initializer and cleanup and the road
// operation together at 0x55f2f0..0x55f467, the object after the river
// operation's in the original link order. The original file name is unknown.
#include "va.h"

#include "rmg.h"
#include "rmg_terrain.h"
#include "tiles.h"

DATA(0x006411AC)
static const int g_rmgRoadPatterns[17] = {
    4, 4, 5, 5, 5, 5, 6, 6, 7, 7, 2, 2, 3, 3, 0, 1, 8
};
DATA(0x0069E650)
TRmgLinePatternTable g_rmgRoadPatternTable(17, g_rmgRoadPatterns);

VA_COMPGEN(0x0055F2F0, 0x1D, STATIC_CTOR, g_rmgRoadPatternTable)

VA_COMPGEN(0x0055F310, 0x0A, STATIC_DTOR, g_rmgRoadPatternTable)

// Cinit 0x55f2f0 builds the seventeen-entry road pattern table from the ids
// at 0x6411ac. The road painter's first virtual slot returns that table.
VA(0x0055f320, 0x08)
MAC_ADDRESS(0x253fc0, 0x8)  // vtables 0x6411f0/0x64120c; Complete-only
TRmgLinePatternTable* TRoadOp::getPatternTable(s32)
{
    return &g_rmgRoadPatternTable;
}

MAC_ADDRESS(0x253fc8, 0x54)
void TRoadOp::setTile(const TTilePoint& point, const TRmgTerrainTile& tile)
{
    TRmgTerrainTile snapshot(tile.m_terrain, tile.m_frame);
    snapshot.m_flipX = tile.m_flipX;
    snapshot.m_flipY = tile.m_flipY;
    m_adapter->setTile(point, snapshot);
}

MAC_ADDRESS(0x25404c, 0x44)
s32 TRoadOp::isBlocked(const TTilePoint& point)
{
    s32 terrain = m_adapter->getTerrain(point);
    if (terrain == eTerrainWater || terrain == eTerrainRock)
        return 1;
    return 0;
}

VA(0x0055f330, 0x17)
MAC_ADDRESS(0x25401c, 0x30)  // vtables 0x641174/0x641190/0x6411f0/0x64120c; Complete-only
void TRoadOp::setLineType(const TTilePoint& point, s32 value)
{
    m_adapter->setLineType(point, value);
}

VA(0x0055f350, 0x34)
MAC_ADDRESS(0x254090, 0x88) // anchor-vtable 0x641174/0x641190/0x6411f0/0x64120c +0x10
void TRoadOp::getTile(const TTilePoint& point, TRmgTerrainTile& tile)
{
    TRmgTerrainTile snapshot = m_adapter->getTile(point);
    tile = snapshot;
}

// The road hierarchy's parallel vtables 0x6411f0/0x64120c use the same
// adapter getLineType forwarding shape in slot 5.
VA(0x0055f390, 0x13)
MAC_ADDRESS(0x254118, 0x30)  // Complete-only road painter
s32 TRoadOp::getLineType(const TTilePoint& point)
{
    return m_adapter->getLineType(point);
}

// The road builder constructs adapter vtable 0x640a04 at 0x548120 and passes
// it here at 0x548143. As in the river constructor, the common painter prefix
// is passed unchanged to walker 0x4fa280, whose subobject begins at +0x10.
VA(0x0055f3b0, 0x76)
MAC_ADDRESS(0x254148, 0x6c) // anchor-callee 0x548143; Complete-only, thiscall ret 0xc
TRoadPlacementOp::TRoadPlacementOp(
    TRoadOp::TAbstractMap* newAdapter,
    s32 newRoadType,
    const TTilePoint& newStart)
    : TRoadOp(newAdapter),
      m_walker(this, newRoadType, newStart)
{
}

// Recovering the real constructor emits the final vtable and this wrapper
// naturally. Its 33 bytes call the retained destructor, test the deleting
// flag, conditionally release this, and return the original object pointer.
VA_COMPGEN(0x0055f430, 0x21, SCALAR_DELETING_DTOR, TRoadPlacementOp)

// The road painter's empty derived destructor restores its distinct base
// vtable at 0x6411f0. The road builder at 0x548040 constructs this parallel
// hierarchy; its scalar deleting destructor is retained at 0x55f430.
VA(0x0055f460, 0x07)
MAC_ADDRESS(0x2541b4, 0x60)  // road painter cleanup; Complete-only RMG helper
TRoadPlacementOp::~TRoadPlacementOp()
{
}
