// rmg.cpp - Complete-only random-map generator support.

// The Dreamcast build has no RMG compiland. Retail's direct caller graph
// reaches this library from TSingleSelectionWindow::GenerateRandomMap, and
// the tree node layout proves an eight-byte TPoint value ordered by y, then x.
// Helper names are inferred from retail; Dreamcast attests no RMG spelling.
#include "va.h"
#include "includes.h"
#include "bitset_iterator.h"
#include "homm3_minmax.h"

#include <algorithm>
#include <bitset>
#include <ctype.h>
#include <functional>
#include <list>
#include <math.h>
#include <queue>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <time.h>

#include "rmg.h"

#include "abstractfile.h"
#include "advmgr_objects.h"
#include "armygrp.h"
#include "artifact.h"
#include "hero.h"
#include "mapcell.h"
#include "objnames.h"
#include "resourcemanager.h"
#include "rmg_request.h"
#include "rmg_terrain.h"
#include "savegame.h"
#include "textresource.h"
#include "town.h"

// Mac 0x22cef0 retains the shared two-point integer distance calculation.
// Its by-value point arguments are spilled as two adjacent coordinate pairs.
// Euclidean distance truncated to an integer; keep the integer squares before
// conversion to double, as in retail, rather than changing rounding/overflow.
MAC_ADDRESS(0x22cef0, 0x84)
int getRmgDistance(TPoint first, TPoint second)
{
    return static_cast<int>(sqrt(static_cast<double>(
        getRmgSquaredDistance(first, second))));
}

typedef std::set<TPoint> TRmgPointSet;

// Complete-only creature reward values: seven signed dwords at 0x6824e0.
DATA(0x006824E0)
int g_rmgCreatureValueByLevel[7] = {5000, 7000, 9000, 12000, 16000, 21000, 27000};

// Vtable 0x640a74 slots 1/2 retain the shared empty/true bodies at
// 0x5bc690/0x484620. These are real base-class defaults, not missing hooks.
void type_object::releaseReservation() {}
b8 type_object::isWritable() { return true; }

// Vtable 0x640b64 slot 2 is the shared false body at 0x484d50.
// Quest vtables 0x640c00/0x640c0c/0x640c18 and key tent 0x640c30
// replace it with the shared true body at 0x484620. No DC RMG counterpart.
b8 type_treasure_def::isTerrainDependent() { return false; }
b8 type_quest_creature_def::isTerrainDependent() { return true; }
b8 type_quest_experience_def::isTerrainDependent() { return true; }
b8 type_quest_gold_def::isTerrainDependent() { return true; }
b8 type_key_tent_def::isTerrainDependent() { return true; }

// Retail constructor defaults and overrides; 0x546257 compares map counts,
// while 0x546270 compares per-zone counts. Names are role-derived.
DATA(0x0069CE4C)
int g_rmgMapObjectLimits[232];
DATA(0x0069D1F4)
int g_rmgZoneObjectLimits[232];
DATA(0x00640718)
static const TRmgObjectLimit g_rmgMapObjectLimitOverrides[30] = {
    { EVENT, 200 },
    { BLACK_BOX, 200 },
    { OBELISK, 48 },
    { BOAT, 64 },
    { TRAINING_GROUNDS, 32 },
    { DEFENSE_TOWER, 32 },
    { GARDEN_OF_REVELATION, 32 },
    { MERC_CAMP, 32 },
    { POWER_SCHOOL, 32 },
    { TREE_OF_KNOWLEDGE, 32 },
    { LIBRARY, 32 },
    { ARENA, 32 },
    { MAGIC_SCHOOL, 32 },
    { WAR_SCHOOL, 32 },
    { UNIVERSITY, 32 },
    { WITCH_HUT, 32 },
    { SHRINE1, 32 },
    { SHRINE2, 32 },
    { SHRINE3, 32 },
    { SIREN, 32 },
    { MYSTICAL_GARDEN, 32 },
    { WATER_WHEEL, 32 },
    { WINDMILL, 32 },
    { MAGIC_SPRING, 32 },
    { DEAD_GUY, 32 },
    { LEAN_TO, 32 },
    { WARRIOR_TOMB, 32 },
    { WAGON, 32 },
    { SEER, 48 },
    { BLACK_MARKET, 32 },
};
DATA(0x00640808)
static const TRmgObjectLimit g_rmgZoneObjectLimitOverrides[24] = {
    { ALTAR_OF_SACRIFICE, 1 },
    { CARTOGRAPHER, 1 },
    { CLOVER_FIELD, 1 },
    { COVER_OF_DARKNESS, 1 },
    { EYE_OF_MAGI, 1 },
    { FAERIE_RING, 1 },
    { FOUNTAIN_OF_FORTUNE, 1 },
    { FOUNTAIN_OF_YOUTH, 1 },
    { HILL_FORT, 1 },
    { IDOL_OF_FORTUNE, 1 },
    { LIGHTHOUSE, 1 },
    { MAGIC_SPRING, 1 },
    { MAGIC_WELL, 1 },
    { OASIS, 1 },
    { OBSERVATORY, 1 },
    { PILLAR_OF_FIRE, 1 },
    { RALLY_FLAG, 1 },
    { SANCTUARY, 1 },
    { STABLES, 1 },
    { TEMPLE, 1 },
    { TRADING_POST, 1 },
    { WAR_MACHINE_FACTORY, 1 },
    { WATERING_HOLE, 1 },
    { WITCH_HUT, 3 },
};

// Complete-only pattern globals: retail cinit 0x55ed70/0x55f2f0 passes the
// array and count to the shared support constructor, then registers cleanup
// at 0x55ed90/0x55f310. Both cleanups retain the support destructor.
DATA(0x00641140)
static const int g_rmgRiverPatterns[13] = {
    4, 4, 4, 4, 8, 7, 7, 6, 6, 2, 2, 3, 3
};
DATA(0x0069E5D0)
TRmgLinePatternTable g_rmgRiverPatternTable(13, g_rmgRiverPatterns);

VA_COMPGEN(0x0055ED70, 0x1D, STATIC_CTOR, g_rmgRiverPatternTable)

VA_COMPGEN(0x0055ED90, 0x0A, STATIC_DTOR, g_rmgRiverPatternTable)

DATA(0x006411AC)
static const int g_rmgRoadPatterns[17] = {
    4, 4, 5, 5, 5, 5, 6, 6, 7, 7, 2, 2, 3, 3, 0, 1, 8
};
DATA(0x0069E650)
TRmgLinePatternTable g_rmgRoadPatternTable(17, g_rmgRoadPatterns);

VA_COMPGEN(0x0055F2F0, 0x1D, STATIC_CTOR, g_rmgRoadPatternTable)

VA_COMPGEN(0x0055F310, 0x0A, STATIC_DTOR, g_rmgRoadPatternTable)

VA(0x00530E20, 0x1C)
MAC_ADDRESS(0x22cf74, 0x18)
TProgressSink::TProgressSink(int totalSteps)
{
    m_steps = totalSteps;
    m_done = 0;
}

VA_COMPGEN(0x00530E40, 0x23, SCALAR_DELETING_DTOR, TProgressSink)

VA(0x00530E70, 0x07)
MAC_ADDRESS(0x22cf8c, 0x48)
TProgressSink::~TProgressSink()
{
}

// Vtable 0x6409c0 slot 1 stores the new total at +4. The derived progress
// dialog overrides the same interface while the base Advance slot stays pure.
VA(0x00530E80, 0x0D)
MAC_ADDRESS(0x22cfd4, 0x8)  // Complete-only RMG progress base
void TProgressSink::setTotal(int totalSteps)
{
    m_steps = totalSteps;
}

namespace {

// The cinit at 0x530da0 writes these eight clockwise neighbours.  The river
// search advances by two entries, selecting only the four cardinal offsets.
DATA(0x0069CDC0)
TPoint g_rmgDirections[RMG_DIRECTION_COUNT] = {
    TPoint(1, 0),
    TPoint(1, 1),
    TPoint(0, 1),
    TPoint(-1, 1),
    TPoint(-1, 0),
    TPoint(-1, -1),
    TPoint(0, -1),
    TPoint(1, -1)
};

// Shipyards are three tiles wide.  The connection repair pass probes the
// four water-facing squares beside their upper and lower edges before it
// floods the reachable water region.
DATA(0x0069CE00)
TPoint g_rmgShipyardWaterOffsets[RMG_SHIPYARD_WATER_OFFSET_COUNT] = {
    TPoint(-3, 0),
    TPoint(1, 0),
    TPoint(-3, 1),
    TPoint(1, 1)
};

DATA(0x006409A0)
static const int g_landRiverDeltaIndex[4] = {2, 0, 3, 1};

DATA(0x006409B0)
static const int g_snowRiverDeltaIndex[4] = {7, 5, 4, 6};

// Terrain-indexed town preferences consumed by initialization 0x53bf85.
// Nine four-entry rows read from the SHA-256-pinned retail image. The
// unusual sentinel/version scan is retained in the caller as retail code.
DATA(0x00682450)
int g_rmgTerrainTownChoices[9][4] = {
    {0, 1, 4, -1}, {6, -1, 0, 0}, {0, 1, -1, 0},
    {2, -1, 0, 0}, {7, 4, -1, 0}, {6, 8, -1, 0},
    {5, 3, 4, -1}, {3, -1, 0, 0}, {-1, 0, 0, 0}
};

// Town-alignment preference used by chooseTerrain 0x532ace..0x532ad5.
// Nine dwords from the SHA-256-pinned Complete image; this is a distinct
// RMG table, not armygrp's native-terrain table at 0x643698. Role-derived
// name: no Dreamcast RMG compiland or original global spelling exists.
DATA(0x006408C8)
static const int g_rmgTownNativeTerrains[9] = {
    eTerrainGrass, eTerrainGrass, eTerrainSnow, eTerrainLava, eTerrainDirt,
    eTerrainDirt, eTerrainRough, eTerrainSwamp, eTerrainGrass
};

// Thirty-two radial directions used by the placement and boundary passes.
DATA(0x00682500)
double g_rmgDirectionCosines[32] = {
    1.0, 0.9807, 0.9239, 0.8315, 0.7071, 0.5556, 0.3827, 0.1951,
    0.0, -0.1951, -0.3827, -0.5556, -0.7071, -0.8315, -0.9239, -0.9807,
    -1.0, -0.9807, -0.9239, -0.8315, -0.7071, -0.5556, -0.3827, -0.1951,
    0.0, 0.1951, 0.3827, 0.5556, 0.7071, 0.8315, 0.9239, 0.9807
};
DATA(0x00682600)
double g_rmgDirectionSines[32] = {
    0.0, 0.1951, 0.3827, 0.5556, 0.7071, 0.8315, 0.9239, 0.9807,
    1.0, 0.9807, 0.9239, 0.8315, 0.7071, 0.5556, 0.3827, 0.1951,
    0.0, -0.1951, -0.3827, -0.5556, -0.7071, -0.8315, -0.9239, -0.9807,
    -1.0, -0.9807, -0.9239, -0.8315, -0.7071, -0.5556, -0.3827, -0.1951
};

// Four six-entry tables drive Complete's guarded-zone connection strength.
// Six dwords per row in the pinned Complete image; original names unknown.
DATA(0x006823F0) int g_rmgGuardThresholdLow[6] = {50000, 2500, 1500, 1000, 500, 0};
DATA(0x00682408) int g_rmgGuardThresholdHigh[6] = {50000, 7500, 7500, 7500, 5000, 5000};
DATA(0x00682420) int g_rmgGuardScaleLow[6] = {0, 2, 3, 4, 6, 6};
DATA(0x00682438) int g_rmgGuardScaleHigh[6] = {0, 2, 3, 4, 4, 6};

DATA(0x00682700)
static const char* g_rmgWaterNames[3] = {
    DATA_COMPGEN(0x006827EC, rmgWaterNone, "None"),
    DATA_COMPGEN(0x006827E4, rmgWaterNormal, "normal"),
    DATA_COMPGEN(0x006827DC, rmgWaterIslands, "islands")
};

DATA(0x0068270C)
static const char* g_rmgPlayerNames[8] = {
    DATA_COMPGEN(0x006827D8, rmgPlayerRed, "red"),
    DATA_COMPGEN(0x006827D0, rmgPlayerBlue, "blue"),
    DATA_COMPGEN(0x006827CC, rmgPlayerTan, "tan"),
    DATA_COMPGEN(0x006827C4, rmgPlayerGreen, "green"),
    DATA_COMPGEN(0x006827BC, rmgPlayerOrange, "orange"),
    DATA_COMPGEN(0x006827B4, rmgPlayerPurple, "purple"),
    DATA_COMPGEN(0x006827AC, rmgPlayerTeal, "teal"),
    DATA_COMPGEN(0x006827A4, rmgPlayerPink, "pink")
};

DATA(0x0068272C)
static const char* g_rmgTownNames[9] = {
    DATA_COMPGEN(0x0068279C, rmgTownCastle, "castle"),
    DATA_COMPGEN(0x00682794, rmgTownRampart, "rampart"),
    DATA_COMPGEN(0x0068278C, rmgTownTower, "tower"),
    DATA_COMPGEN(0x00682784, rmgTownInferno, "inferno"),
    DATA_COMPGEN(0x00682778, rmgTownNecropolis, "necropolis"),
    DATA_COMPGEN(0x00682770, rmgTownDungeon, "dungeon"),
    DATA_COMPGEN(0x00682764, rmgTownStronghold, "stronghold"),
    DATA_COMPGEN(0x00682758, rmgTownFortress, "fortress"),
    DATA_COMPGEN(0x00682750, rmgTownConflux, "conflux")
};

// Ordinary helper used by ReadRmgTemplateZones; original name unknown.
// The connection reader and template-row scan share the same blank test.
static bool isRmgTemplateFieldSet(const char* value)
{
    return value && value[0] && value[0] != ' ';
}

} // namespace

static void __fastcall assignRmgTeams(
    int teamCount,
    int playerCount,
    int firstTeam,
    const b8* players,
    char* teams);

// Retail retains bitset<156>::set and bitset<128>::set in the two expansions.
// The wrapper name is inferred.
template <unsigned int N>
static void setAvailableRmgHeroes(
    std::bitset<N>* availableHeroes,
    b8* heroFlag,
    b8* end)
{
    std::transform(heroFlag, end,
        bitset_iterator<N>(*availableHeroes, 0),
        std::logical_not<b8>());
}

// Vtable 0x6409cc slot 0 and the 0x14-byte concrete map layout identify this
// scalar deleting wrapper. Its non-deleting half destroys the owned array of
// 0x30-byte TRmgMapItem elements before restoring the abstract map vtable.
VA_COMPGEN(0x00530F80, 0x21, SCALAR_DELETING_DTOR, type_random_map)

VA(0x00530E90, 0x4A)
MAC_ADDRESS(0x22cfdc, 0x50)
TRmgMapItem::TRmgMapItem()
{
    clear();
}

VA_COMPGEN(0x00530EE0, 0x26, IMPLICIT_DTOR, TRmgMapItem)
MAC_COMPGEN_ADDRESS(0x22d02c, 0x84, IMPLICIT_DTOR, TRmgMapItem)

// Called by the array constructor at 0x530e90 after m_objects construction.
// Retail clears the pointer vector before taking the packed-field snapshots;
// fields absent from these writes, including previous Y/Z, remain unchanged.
VA(0x00530F10, 0x6F)
MAC_ADDRESS(0x22d110, 0x15c)
void TRmgMapItem::clear()
{
    m_objects.clear();
    TRmgConnectionDecoration connection = m_connection;
    TRmgGroundTileData tileData = m_tileData;

    connection.m_present = false;
    m_tile.m_landType = eTerrainWater;
    m_tile.m_terrainFrame = 21;
    m_tile.m_riverType = 0;
    m_tile.m_riverFrame = 0;
    m_tile.m_roadType = 0;
    tileData.m_roadFrame = 0;
    tileData.m_blockedDirections = 0;
    tileData.m_connectionDirection = 0;
    tileData.m_terrainFlipX = false;
    tileData.m_terrainFlipY = false;
    tileData.m_riverFlipX = false;
    tileData.m_riverFlipY = false;
    tileData.m_roadFlipX = false;
    tileData.m_roadFlipY = false;
    tileData.m_coastal = false;
    tileData.m_roadEntrance = false;
    tileData.m_placementOutline = false;
    tileData.m_roadPassable = true;
    tileData.m_borderObject = false;
    tileData.m_pathClearance = true;
    tileData.m_paintZoneTerrain = false;
    tileData.m_hasRiver = false;
    tileData.m_riverTarget = false;
    tileData.m_impassable = false;
    m_connection = connection;
    m_movement.m_cost = 32700;
    m_movement.m_zonePathCost = 32700;
    m_zoneState.m_score = 32700;
    m_zoneState.m_zone = -1;
    m_zoneState.m_connectionEligibility = -1;
    m_previousTile.m_x = -1;
    m_tileData = tileData;
}

// Retail retains this constructor call in TRmgGeneratorBase::TRmgGeneratorBase
// (0x536070).
VA(0x00530FB0, 0xA0)
MAC_ADDRESS(0x22d394, 0xbc)
type_random_map::type_random_map(int width, int height, int levels)
{
    m_mapWidth = width;
    m_mapHeight = height;
    m_numberLevels = levels;
    m_ownsMapItems = true;
    m_mapItems = new TRmgMapItem[m_mapWidth * m_mapHeight * m_numberLevels];
}

// The array-delete helper for TRmgMapItem uses the recovered 0x30-byte stride
// and delegates every element to the implicit destructor above.
VA_COMPGEN(0x00531050, 0x58, VECTOR_DELETING_DTOR, TRmgMapItem)

VA(0x005310B0, 0x83)
MAC_ADDRESS(0x22d450, 0x84)
type_random_map::~type_random_map()
{
    if (m_ownsMapItems)
        delete[] m_mapItems;
}

VA(0x00531140, 0x2A)
MAC_ADDRESS(0x22d4d4, 0x60)
void type_random_map::clear()
{
    TRmgMapItem* mapItem = m_mapItems;
    int mapItemCount = m_mapWidth * m_mapHeight * m_numberLevels;
    while (mapItemCount--) {
        mapItem->clear();
        ++mapItem;
    }
}

// Shared by the footprint and surrounding-outline placement tests. Preserve
// land, entrance, then zone query order; border, water and path clearance
// policies stay with each caller.
static inline bool isRmgPlacementCellBlocked(TRmgMapItem* item, int zoneIndex)
{
    return !item->isPassableLand() || item->isRoadEntrance()
        || item->m_zoneState.m_zone != zoneIndex;
}

VA(0x00531170, 0x19C)
MAC_ADDRESS(0x22d534, 0x244)
b8 type_random_map::hasConnectedOutline(
    const std::vector<TPoint>& outline, TRmgMapPosition position,
    b8 allowEntrances, TRmgZone* zone, b8 requirePathClearance)
{
    // Circular run counting: the repeated first point closes the final run.
    // Retail requires a nonempty outline; preserve its empty-outline behavior.
    b8 blocked = true;
    b8 foundBoundary = false;
    b8 waterZone = zone->m_terrain == eTerrainWater;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    for (unsigned int index = 0; index < outline.size() + 1; ++index) {
        b8 previouslyBlocked = blocked;
        TPoint offset(outline[index % outline.size()]);
        int x = position.m_x + offset.m_x;
        int y = position.m_y + offset.m_y;
        if (!containsXY(TPoint(x, y))) {
            blocked = true;
        } else {
            TRmgMapItem* item = getMapItem(x, y, position.m_z);
            if (!allowEntrances && item->isRoadEntrance())
                return false;
            blocked = isRmgPlacementCellBlocked(item, zoneIndex);
            if (requirePathClearance && !item->hasPathClearance())
                blocked = true;
            if ((item->getLandType() == eTerrainWater) != waterZone)
                blocked = true;
        }
        if (blocked && !previouslyBlocked) {
            if (foundBoundary)
                return false;
            foundBoundary = true;
        }
    }
    if (blocked && !foundBoundary)
        return false;
    return true;
}

// Preserve the callers' Y-before-X bound updates.
static inline void setRmgNeighborhoodBounds(TRmgZoneBounds& bounds,
    const TPoint& position, const type_random_map& map, int radius)
{
    int row = position.m_y;
    TPoint lower(position.m_x - radius, row - radius);
    bounds.m_minimumY = max(lower.m_y, 0);
    bounds.m_minimumX = max(lower.m_x, 0);
    int height = map.m_mapHeight;
    bounds.m_maximumY = min(row + (radius + 1), height);
    int width = map.m_mapWidth;
    bounds.m_maximumX = min(position.m_x + (radius + 1), width);
}

VA(0x00531310, 0x14B)
MAC_ADDRESS(0x22d778, 0x234)
void type_random_map::markCoastalTiles()
{
    TRmgMapPosition position;
    TRmgMapItem* item = m_mapItems;
    for (position.m_z = 0; position.m_z < m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_mapWidth; ++position.m_x, ++item) {
                if (item->getLandType() == eTerrainWater) {
                    TRmgZoneBounds bounds;
                    setRmgNeighborhoodBounds(bounds, position, *this, 1);
                    for (int nearY = bounds.m_minimumY; nearY < bounds.m_maximumY; ++nearY) {
                        for (int nearX = bounds.m_minimumX; nearX < bounds.m_maximumX; ++nearX) {
                            TRmgMapItem* neighbor = getMapItem(nearX, nearY, position.m_z);
                            if (neighbor->getLandType() != eTerrainWater
                                && neighbor->getLandType() != eTerrainRock)
                                neighbor->m_tileData.m_coastal = true;
                        }
                    }
                }
            }
        }
    }
}

// Equal-cost entries precede older entries, so back removal processes ties
// oldest first. Retail uses one top test with two unconditional back edges.
static int findRmgWorkItemInsertionIndex(
    const std::vector<int>& costs, int count, int cost)
{
    int first = 0;
    int last = count;
    int middle;
    while (1) {
        middle = (first + last) >> 1;
        if (first >= last)
            break;
        if (cost < costs[middle])
            first = middle + 1;
        else
            last = middle;
    }

    return middle;
}

// BuildRoadCostMap and CreateRiver materialize a separate by-value position
// before this search. No standalone body survives; name/linkage are inferred.
static void insertRmgWorkItem(
    std::vector<TRmgMapPosition>& positions,
    std::vector<int>& costs,
    TRmgMapPosition position,
    int cost)
{
    int middle = findRmgWorkItemInsertionIndex(costs, positions.size(), cost);
    positions.insert(positions.begin() + middle, position);
    costs.insert(costs.begin() + middle, cost);
}

// Copy the position before either erase, and preserve cost-before-position
// removal. Searches that consume the queued cost use their own extraction.
static inline void popRmgMovementPosition(TRmgMapPosition& position,
    std::vector<TRmgMapPosition>& positions, std::vector<int>& costs)
{
    position = positions.back();
    costs.pop_back();
    positions.pop_back();
}

// Retail 0x5407dd/0x5408a2 pass a by-value position and a water byte; name unknown.
// This flood inserts costs before positions (+0x3c6, stride 4; +0x3de, stride 12).
// Other position worklists insert positions first. Keep the distinct order and
// this caller's direct neighbor argument rather than an extra by-value copy.
VA(0x00531460, 0x441)
MAC_ADDRESS(0x22d9ac, 0x5cc)
void type_random_map::floodConnectionCosts(TRmgMapPosition position, b8 waterZone)
{
    // Retail applies the zero-cost clearance override after the improvement
    // test and reads the queued cost even though it is not subsequently used.
    std::vector<int> costs;
    std::vector<TRmgMapPosition> positions;
    TRmgMapItem* seed = getMapItem(position);
    int zone = seed->m_zoneState.m_zone;
    positions.push_back(position);
    costs.push_back(0);
    seed->setMovementCost(0, TRmgMapPosition(-1, -1, -1));
    while (positions.size()) {
        TRmgMapPosition currentPosition = positions.back();
        int queuedCost = costs.back();
        TRmgMapItem* current = getMapItem(currentPosition);
        positions.pop_back();
        costs.pop_back();
        int currentZone = current->m_zoneState.m_zone;
        int currentCost = currentZone == zone
            ? current->m_movement.m_cost : current->m_movement.m_zonePathCost;
        int direction = RMG_DIRECTION_COUNT;
        if (current->isRoadEntrance()) {
            int objectType = current->m_objects[0]->m_properties->m_prototype->getObjectType();
            if (!g_adventureObjectTraits[objectType].m_trait1)
                direction = 5;
        }
        while (direction--) {
            int nextCost = currentCost + 1;
            TRmgMapPosition nextPosition = currentPosition;
            nextPosition += g_rmgDirections[direction];
            if (!containsXY(nextPosition))
                continue;
            TRmgMapItem* next = getMapItem(nextPosition);
            if (next->m_zoneState.m_zone < 0 || !next->isPassableLand())
                continue;
            if (next->isRoadEntrance()) {
                int objectType = next->m_objects[0]->m_properties->m_prototype->getObjectType();
                const TAdvObjectTraits& traits = g_adventureObjectTraits[objectType];
                if (traits.m_blocksLanding && !traits.m_trait2)
                    continue;
                if (!traits.m_trait1 && direction > 0 && direction < 4)
                    continue;
            }
            if (next->m_zoneState.m_zone != zone) {
                nextCost = currentCost + 10;
                if (currentZone != zone && currentZone != next->m_zoneState.m_zone)
                    continue;
                if (next->m_movement.m_zonePathCost <= nextCost)
                    continue;
                next->m_movement.m_zonePathCost = nextCost;
                next->m_tileData.m_connectionDirection = direction - 4;
                next->m_zoneState.m_connectionEligibility = zone;
            } else {
                if (currentZone != zone)
                    continue;
                if (next->getLandType() == eTerrainWater)
                    nextCost = currentCost + 10;
                if (next->m_movement.m_cost <= nextCost)
                    continue;
                if (!currentCost && next->hasPathClearance()
                    && (next->getLandType() != eTerrainWater || waterZone))
                    nextCost = 0;
                next->setMovementCost(nextCost, currentPosition);
            }
            int middle = findRmgWorkItemInsertionIndex(
                costs, positions.size(), nextCost);
            costs.insert(costs.begin() + middle, nextCost);
            positions.insert(positions.begin() + middle, nextPosition);
        }
    }
}

// The category guard precedes the bitset query.
static inline bool isRmgWaterOnlyPrototype(const TObjectType& prototype)
{
    return prototype.m_slotCategory == TObjectType::SLOT_CATEGORY_0
        && prototype.m_recommendedTerrainMask.test(eTerrainWater);
}

// Retail checks trigger and blocked-mask cells separately, even when both
// select the same tile. Only the trigger arm observes rejectBorder. The
// category-zero water rule belongs to the blocked-mask arm, not the whole
// footprint. Bounds are signed; the two mask loops use unsigned indices.
VA(0x005318B0, 0x212)
MAC_ADDRESS(0x22dfe0, 0x2a4) // anchor-callee 0x531d29; thiscall, ret 0x18; retail-only
b8 type_random_map::isPlacementBlocked(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position,
    int zoneIndex, b8 rejectBorder)
{
    TObjectType& prototype = *properties->m_prototype;
    if (position.m_x < prototype.getWidth() - 1 || position.m_x >= m_mapWidth
        || position.m_y < prototype.getHeight() - 1 || position.m_y >= m_mapHeight)
        return true;
    TRmgMapPosition nearby = position;
    for (unsigned int y = 0; y < prototype.getHeight(); ++y, --nearby.m_y) {
        nearby.m_x = position.m_x;
        for (unsigned int x = 0; x < prototype.getWidth(); ++x, --nearby.m_x) {
            TRmgGridPoint maskPoint(x, y);
            TRmgMapItem* item = getMapItem(nearby);
            if (prototype.isTriggerCell(maskPoint.m_x, maskPoint.m_y)) {
                if (isRmgPlacementCellBlocked(item, zoneIndex))
                    return true;
                if (rejectBorder && item->hasBorderObject())
                    return true;
            }
            if (!prototype.isPassableCell(maskPoint.m_x, maskPoint.m_y)) {
                if (isRmgPlacementCellBlocked(item, zoneIndex))
                    return true;
                if (item->getLandType() == eTerrainWater) {
                    if (!isRmgWaterOnlyPrototype(prototype))
                        return true;
                } else if (isRmgWaterOnlyPrototype(prototype)) {
                    return true;
                }
            }
        }
    }
    return false;
}

VA(0x00531AD0, 0x100)
MAC_ADDRESS(0x22e284, 0x1d0) // anchor-callee 0x5441a1; Complete-only, ret 0xc
void type_random_map::openPathPatch(int x, int y, int level)
{
    TRmgMapItem* item = getMapItem(x, y, level);
    item->openPath();
    TRmgZoneBounds bounds;
    setRmgNeighborhoodBounds(bounds, TPoint(x, y), *this, 1);
    for (int row = bounds.m_minimumY; row < bounds.m_maximumY; ++row) {
        for (int column = bounds.m_minimumX; column < bounds.m_maximumX; ++column) {
            TRmgMapItem* nearby = getMapItem(column, row, level);
            nearby->openPath();
        }
    }
}

VA(0x00531BD0, 0x11F)
MAC_ADDRESS(0x22e454, 0x254) // anchor-callee 0x544343; Complete-only, ret 0xc
void type_random_map::markBorderPatch(TRmgMapPosition position)
{
    TRmgMapItem* item = getMapItem(position);
    item->markBorderObject();
    TRmgZoneBounds bounds;
    setRmgNeighborhoodBounds(bounds, position, *this, 1);
    for (int row = bounds.m_minimumY; row < bounds.m_maximumY; ++row) {
        for (int column = bounds.m_minimumX; column < bounds.m_maximumX; ++column) {
            TRmgMapItem* nearby = getMapItem(column, row, position.m_z);
            if (!nearby->isRoadEntrance() && nearby->isPassableLand()
                && nearby->getLandType() != eTerrainWater)
                nearby->releasePathClearance();
        }
    }
}

// Keep the native point construction and compound subtraction nested.
// Callers pass either the live prototype trigger or their earlier snapshot;
// town placement must keep its captured offset across virtual object insertion.
static inline TRmgMapPosition getRmgObjectTriggerPosition(
    TRmgMapPosition position, const TObjectType::TPoint& trigger)
{
    TPoint triggerOffset(trigger.m_x, trigger.m_y);
    position -= triggerOffset;
    return position;
}

// Translate a placed object's stored position by its live prototype trigger.
static inline TRmgMapPosition getRmgPlacedObjectEntrance(const type_object* object)
{
    return getRmgObjectTriggerPosition(object->getPosition(),
        object->m_properties->m_prototype->m_triggerCell);
}

// Keep trait2 before trait1; direction limits use trait1 alone elsewhere.
static inline bool allowsRmgSharedObjectEntrance(int objectType)
{
    return g_adventureObjectTraits[objectType].m_trait2
        && g_adventureObjectTraits[objectType].m_trait1;
}

// The footprint/outline helper calls are retained, followed by an expanded
// entrance lookup one row below the trigger. Retail checks a negative zone
// separately from a different zone, then compares water membership as ints.
VA(0x00531CF0, 0x1A5)
MAC_ADDRESS(0x22e6a8, 0x270) // anchor-callee 0x541c73; thiscall, ret 0x14; retail-only
b8 type_random_map::canPlaceObject(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position, TRmgZone* zone)
{
    TObjectType& prototype = *properties->m_prototype;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    if (isPlacementBlocked(properties, position, zoneIndex, false))
        return false;
    int objectType = prototype.getObjectType();
    properties->buildOutline();
    if (!hasConnectedOutline(properties->m_outline, position,
            allowsRmgSharedObjectEntrance(objectType),
            zone, false))
        return false;
    if (!prototype.m_hasTrigger)
        return true;
    TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, prototype.m_triggerCell);
    ++entrance.m_y;
    if (entrance.m_y >= m_mapHeight)
        return false;
    TRmgMapItem* item = getMapItem(entrance);
    if (!item->isPassableLand())
        return false;
    if (item->m_zoneState.m_zone < 0)
        return false;
    if (item->m_zoneState.m_zone != zoneIndex)
        return false;
    if (item->isRoadEntrance()) {
        int entranceType = item->m_objects[0]->m_properties->m_prototype->getObjectType();
        if (!g_adventureObjectTraits[entranceType].m_trait2)
            return false;
    }
    b8 result = (item->getLandType() == eTerrainWater) == (zone->m_terrain == eTerrainWater);
    return result;
}

// Treasure fill and guard placement pass a map in ECX, followed by an
// object address and a by-value position. Retail first stores that position
// into the object, then updates the footprint's cells and object vectors.
// At both STL insertion sites retail creates a pointer temporary at [ebp-4].
// A type_object reference supplies &object prvalues naturally; a pointer
// parameter binds directly from [ebp+8] instead. This source contract remains
// a retail-supported inference: there is no Dreamcast RMG declaration.
// Mask coordinates are unsigned, world coordinates signed; width/height
// queries remain live throughout both footprint walkers.
VA(0x00531EA0, 0x2E6)
MAC_ADDRESS(0x22e918, 0x1c4) // anchor-callee 0x5465d9/0x535400; thiscall, ret 0x10
void type_random_map::addObject(type_object& object, TRmgMapPosition position)
{
    TObjectType& prototype = *object.m_properties->m_prototype;
    object.m_position = position;
    TRmgMapPosition nearby = position;
    for (unsigned int y = 0; y < prototype.getHeight(); ++y, --nearby.m_y) {
        if (nearby.m_y < 0 || nearby.m_y >= m_mapHeight)
            continue;
        nearby.m_x = position.m_x;
        for (unsigned int x = 0; x < prototype.getWidth(); ++x, --nearby.m_x) {
            if (nearby.m_x < 0 || nearby.m_x >= m_mapWidth)
                continue;
            TRmgGridPoint maskPoint(x, y);
            TRmgMapItem* item = getMapItem(nearby);
            if (prototype.isTriggerCell(maskPoint.m_x, maskPoint.m_y)) {
                item->m_tileData.m_roadEntrance = true;
                item->openPath();
                item->m_objects.push_back(&object);
            } else if (!prototype.isPassableCell(maskPoint.m_x, maskPoint.m_y)) {
                item->m_tileData.m_roadPassable = false;
                item->m_objects.push_back(&object);
            }
        }
    }
}

VA(0x00532190, 0x6D)
MAC_ADDRESS(0x22eadc, 0x70)
void type_random_map::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    TRmgMapItem& item = *getMapItem(point.m_x, point.m_y);
    b8 flipY = tile.m_flipY;
    b8 flipX = tile.m_flipX;
    int frame = tile.m_frame;
    int terrain = tile.m_terrain;
    item.setTerrain(terrain, frame, flipX, flipY);
}

// Slot 2 updates only the packed eight-bit terrain frame.
VA(0x00532200, 0x3C)
MAC_ADDRESS(0x22eb4c, 0x38)
void type_random_map::setFrame(const TRmgGridPoint& point, int value)
{
    TRmgMapItem& item = *getMapItem(point.m_x, point.m_y);
    item.m_tile.m_terrainFrame = value;
}

// Slot 3 writes the explicit output and returns its address. This body alone
// also fits a hidden value result; retail adapter 0x532790 distinguishes the
// contracts by copying from the returned reference, not the named temporary.
VA(0x00532240, 0x15)
MAC_ADDRESS(0x22eb84, 0x14)
#if defined(HOMM3_TARGET_MAC)
// Mac 0x22eb84 returns the size through a hidden result pointer.
TRmgGridPoint type_random_map::getSize()
{
    return TRmgGridPoint(m_mapWidth, m_mapHeight);
}
#else
TRmgGridPoint& type_random_map::getSize(TRmgGridPoint& output)
{
    output = TRmgGridPoint(m_mapWidth, m_mapHeight);
    return output;
}
#endif

// Slot 4 expands the packed terrain fields into the adapter's three-dword
// value, including the two independent flip bytes.
VA(0x00532260, 0x60)
MAC_ADDRESS(0x22eb98, 0x80)
rmgTerrainTile type_random_map::getTile(const TRmgGridPoint& point)
{
    TRmgMapItem& item = *getMapItem(point.m_x, point.m_y);
    rmgTerrainTile tile;
    tile.m_terrain = item.getLandType();
    tile.m_frame = item.m_tile.m_terrainFrame;
    tile.m_flipX = item.m_tileData.m_terrainFlipX;
    tile.m_flipY = item.m_tileData.m_terrainFlipY;
    return tile;
}

// Vtable 0x6409cc slots 5 and 6 index the 0x30-byte cell array with the
// supplied x/y point. The two signed extracts select the six-bit land kind
// and its adjacent eight-bit terrain frame from TRmgGroundTile.
VA(0x005322C0, 0x2A)
MAC_ADDRESS(0x22ec18, 0x34)
int type_random_map::getTerrain(const TRmgGridPoint& point)
{
    return getMapItem(point.m_x, point.m_y)->getLandType();
}

VA(0x005322F0, 0x2A)
MAC_ADDRESS(0x22ec4c, 0x34)
int type_random_map::getFrame(const TRmgGridPoint& point)
{
    return getMapItem(point.m_x, point.m_y)->m_tile.m_terrainFrame;
}

VA(0x00532350, 0x07)
MAC_ADDRESS(0x22ec98, 0x48)
TRmgRoadMapAdapterInterface::~TRmgRoadMapAdapterInterface()
{
}

VA_COMPGEN(0x00537940, 0x23, SCALAR_DELETING_DTOR, TRmgRoadMapAdapterInterface)

VA(0x00532360, 0x6E)
MAC_ADDRESS(0x22ece0, 0x6c)
void TRmgRoadMapAdapter::setTile(
    const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    TRmgMapItem& item = *m_map->getMapItem(point.m_x, point.m_y);
    b8 flipY = tile.m_flipY;
    int frame = tile.m_frame;
    b8 flipX = tile.m_flipX;
    int terrain = tile.m_terrain;
    item.m_tile.m_roadType = terrain;
    item.m_tileData.m_roadFrame = frame;
    item.m_tileData.m_roadFlipX = flipX;
    item.m_tileData.m_roadFlipY = flipY;
}

VA(0x005323D0, 0x3C)
MAC_ADDRESS(0x22ed4c, 0x3c)
void TRmgRoadMapAdapter::setLineType(const TRmgGridPoint& point, int value)
{
    TRmgMapItem& item = *m_map->getMapItem(point.m_x, point.m_y);
    item.m_tile.m_roadType = value;
}

VA(0x00532410, 0x62)
MAC_ADDRESS(0x22edc4, 0xa0)
rmgTerrainTile TRmgRoadMapAdapter::getTile(const TRmgGridPoint& point)
{
    TRmgMapItem& item = *m_map->getMapItem(point.m_x, point.m_y);
    rmgTerrainTile tile;
    tile.m_terrain = item.m_tile.m_roadType;
    tile.m_frame = item.m_tileData.m_roadFrame;
    tile.m_flipX = item.m_tileData.m_roadFlipX;
    tile.m_flipY = item.m_tileData.m_roadFlipY;
    return tile;
}

VA(0x00532480, 0x2D)
MAC_ADDRESS(0x22ee64, 0x38)
int TRmgRoadMapAdapter::getLineType(const TRmgGridPoint& point)
{
    return m_map->getMapItem(point.m_x, point.m_y)->m_tile.m_roadType;
}

VA(0x005324B0, 0x2D)
MAC_ADDRESS(0x22ee9c, 0x38)
int TRmgRoadMapAdapter::getTerrain(const TRmgGridPoint& point)
{
    return m_map->getMapItem(point.m_x, point.m_y)->getLandType();
}

// The two concrete adapter vtables share retail 0x532790. Keep both source
// methods; the river method below owns their joint ICF representative.
// VC6 accepts the temporary output at the non-const-reference slot; copy
// its returned reference before the full expression ends, as in the painter.
TRmgGridPoint TRmgRoadMapAdapter::getSize()
{
#if defined(HOMM3_TARGET_MAC)
    return m_map->getSize();  // Mac 0x22f2e4
#else
    return m_map->getSize(TRmgGridPoint());
#endif
}

// The real road-painting stack construction at 0x548120 retains the
// concrete adapter vtable and this ordinary deleting wrapper naturally.
VA_COMPGEN(0x00532320, 0x21, SCALAR_DELETING_DTOR, TRmgRoadMapAdapter)

VA(0x00532510, 0x07)
MAC_ADDRESS(0x22eeec, 0x48)
TRmgRiverMapAdapterInterface::~TRmgRiverMapAdapterInterface()
{
}

VA_COMPGEN(0x00537910, 0x23, SCALAR_DELETING_DTOR, TRmgRiverMapAdapterInterface)

// Vtable 0x640a3c slot 0 and the 0x08 concrete adapter layout identify this
// scalar deleting wrapper. The retained body delegates to the adapter-interface
// destructor at 0x532510 before conditionally releasing the object.
VA_COMPGEN(0x005324E0, 0x21, SCALAR_DELETING_DTOR, TRmgRiverMapAdapter)

VA(0x00532520, 0x205)
MAC_ADDRESS(0x22ef34, 0x35c)
void TRmgRiverMapAdapter::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    TRmgMapItem& item = *m_map->getMapItem(point.m_x, point.m_y);
    b8 flipX = tile.m_flipX;
    int terrain = tile.m_terrain;
    b8 flipY = tile.m_flipY;
    int frame = tile.m_frame;
    item.m_tile.m_riverType = terrain;
    item.m_tile.m_riverFrame = frame;
    item.m_tileData.m_riverFlipX = flipX;
    item.m_tileData.m_riverFlipY = flipY;
    b8 present = tile.m_terrain != 0;
    item.m_tileData.m_hasRiver = present;
    if (tile.m_terrain != 0) {
        {
            TRmgZoneBounds bounds;
            setRmgNeighborhoodBounds(bounds, point, *m_map, 1);
            for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
                for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                    TRmgMapItem& neighbour = *m_map->getMapItem(x, y);
                    neighbour.m_tileData.m_impassable = true;
                }
            }
        }
        {
            TRmgZoneBounds bounds;
            setRmgNeighborhoodBounds(bounds, point, *m_map, 2);
            for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
                for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                    TRmgMapItem& neighbour = *m_map->getMapItem(x, y);
                    if (neighbour.m_tile.m_riverType == 0) {
                        neighbour.m_tileData.m_riverTarget = false;
                    }
                }
            }
        }
    }
}

VA(0x00532730, 0x57)
MAC_ADDRESS(0x22f290, 0x54) // anchor-vtable + packed-field evidence; Complete-only
void TRmgRiverMapAdapter::setLineType(const TRmgGridPoint& point, int value)
{
    TRmgMapItem& item = *m_map->getMapItem(point.m_x, point.m_y);
    item.m_tile.m_riverType = value;
    b8 present = value != 0;
    item.m_tileData.m_hasRiver = present;
}

// Both adapter vtables share this body. Retail copies from the map query's
// returned reference into its value result. The temporary output lives
// through both the query and the copy.
VA(0x00532790, 0x27)
MAC_ADDRESS(0x22f2e4, 0x3c) // vtable 0x640a3c slot 3, ICF with road slot 3
TRmgGridPoint TRmgRiverMapAdapter::getSize()
{
#if defined(HOMM3_TARGET_MAC)
    return m_map->getSize();  // Mac 0x22f2e4
#else
    return m_map->getSize(TRmgGridPoint());
#endif
}

VA(0x005327C0, 0x63)
MAC_ADDRESS(0x22f320, 0xa4) // anchor-vtable + packed-field evidence; Complete-only
rmgTerrainTile TRmgRiverMapAdapter::getTile(const TRmgGridPoint& point)
{
    const TRmgMapItem& item = *m_map->getMapItem(point.m_x, point.m_y);
    rmgTerrainTile tile;
    tile.m_terrain = item.m_tile.m_riverType;
    tile.m_frame = item.m_tile.m_riverFrame;
    tile.m_flipX = item.m_tileData.m_riverFlipX;
    tile.m_flipY = item.m_tileData.m_riverFlipY;
    return tile;
}

// Concrete river-adapter vtable 0x640a3c slots 5 and 6 read the packed river
// kind and underlying land kind from the wrapped map's 0x30-byte cell array.
VA(0x00532830, 0x2D)
MAC_ADDRESS(0x22f3c4, 0x38)
int TRmgRiverMapAdapter::getLineType(const TRmgGridPoint& point)
{
    return m_map->getMapItem(point.m_x, point.m_y)->m_tile.m_riverType;
}

VA(0x00532860, 0x2D)
MAC_ADDRESS(0x22f3fc, 0x38)
int TRmgRiverMapAdapter::getTerrain(const TRmgGridPoint& point)
{
    return m_map->getMapItem(point.m_x, point.m_y)->getLandType();
}

VA(0x00532890, 0x104)
MAC_ADDRESS(0x22f434, 0x1d4)
void TRmgMapItem::write(TAbstractFile* outputFile)
{
    writeValue<char>(outputFile, m_tile.m_landType);
    writeValue<char>(outputFile, m_tile.m_terrainFrame);
    writeValue<char>(outputFile, m_tile.m_riverType);
    writeValue<char>(outputFile, m_tile.m_riverFrame);
    writeValue<char>(outputFile, m_tile.m_roadType);
    writeValue<char>(outputFile, m_tileData.m_roadFrame);
    char flags = 0;
    if (m_tileData.m_terrainFlipX) flags |= 1;
    if (m_tileData.m_terrainFlipY) flags |= 2;
    if (m_tileData.m_riverFlipX) flags |= 4;
    if (m_tileData.m_riverFlipY) flags |= 8;
    if (m_tileData.m_roadFlipX) flags |= 16;
    if (m_tileData.m_roadFlipY) flags |= 32;
    if (m_tileData.m_coastal) flags |= 64;
    writeValue<char>(outputFile, flags);
}

VA_COMPGEN(0x005329A0, 0x32, IMPLICIT_DTOR, TRmgTemplateZone)
MAC_COMPGEN_ADDRESS(0x22f660, 0x68, IMPLICIT_DTOR, TRmgTemplateZone)

// Native 0x22f6c8 counts enabled town types and uniformly selects one.
// It returns -1 for an empty set. Both the zone constructor (0x22f834)
// and additional-town placement (0x248878) retain this shared selection.
MAC_ADDRESS(0x22f6c8, 0xb4)
int TRmgTemplateZone::selectAllowedTown()
{
    int allowedTownCount = 0;
    for (int town = 0; town < 9; ++town) {
        if (m_allowedTowns[town])
            ++allowedTownCount;
    }
    if (!allowedTownCount)
        return -1;
    int selectedIndex = rand() % allowedTownCount;
    for (town = 0; town < 9; ++town) {
        if (m_allowedTowns[town] && --selectedIndex < 0)
            return town;
    }
    return -1;
}

VA(0x005329E0, 0xCF)
MAC_ADDRESS(0x22f7c4, 0xc0) // anchor-callee 0x53e149/0x53e45c; thiscall, ret 4
TRmgZone::TRmgZone(TRmgTemplateZone* newSlot)
{
    m_templateZone = newSlot;
    m_alignment = newSlot->selectAllowedTown();
    m_boundaryRoughness = newSlot->m_size;
    m_bounds.resetEmpty();
    m_active = false;
    memset(m_objectCountByType, 0, sizeof(m_objectCountByType));
}

// Native 0x22f9d8 retains this selection; initializeZones calls it at
// 0x23d1b4. Both builds retain the unusual OR condition in the town scan.
MAC_ADDRESS(0x22f9d8, 0xcc)
void TRmgZone::chooseTownType(b8 expanded)
{
    if (m_alignment != -1) {
        m_townType2 = m_alignment;
    } else {
        int count = 0;
        // Retail 0x53bf8c..0x53bf98 continues on != -1, then on
        // expanded, then on != 8. Preserve this observed condition,
        // even though no table entry can satisfy both equalities.
        while (count < 4 &&
            (g_rmgTerrainTownChoices[m_terrain][count] != -1 || expanded ||
             g_rmgTerrainTownChoices[m_terrain][count] != TOWN_CONFLUX))
            ++count;
        if (count == 0)
            m_townType2 = -1;
        else
            m_townType2 = g_rmgTerrainTownChoices[m_terrain][rand() % count];
    }
}

// Count and selection must use the same allowed-terrain and level policy.
static inline bool isRmgZoneTerrainAllowed(const TRmgZone& zone, int terrain)
{
    return zone.m_templateZone->m_allowedTerrain[terrain]
        && (terrain != eTerrainSubterranean || zone.m_levelPosition.m_z == 1);
}

VA(0x00532AB0, 0x96)
MAC_ADDRESS(0x22faa4, 0x130)
void TRmgZone::chooseTerrain()
{
    if (m_templateZone->m_useNativeTerrain && m_alignment != -1) {
        m_terrain = g_rmgTownNativeTerrains[m_alignment];
    } else {
        int count = 0;
        for (int terrain = 0; terrain < eTerrainWater; ++terrain) {
            if (isRmgZoneTerrainAllowed(*this, terrain))
                ++count;
        }
        if (!count) {
            m_terrain = eTerrainDirt;
        } else {
            int selected = rand() % count;
            int terrain;
            for (terrain = 0; terrain < eTerrainWater; ++terrain) {
                if (isRmgZoneTerrainAllowed(*this, terrain)) {
                    if (selected-- <= 0)
                        break;
                }
            }
            m_terrain = terrain;
        }
    }
    if (m_levelPosition.m_z == 1 && m_terrain != eTerrainLava)
        m_terrain = eTerrainSubterranean;
}

// Retail 0x532b62/0x532b85/0x532ba6 destroys entrances (+0x404), boundary
// (+0x3f4), then distances (+0x3e4), with no pointee or user cleanup.
// The body is visible before the derived generator destructor at 0x537df0;
// whether the original declaration was explicit remains inferred.
VA(0x00532B50, 0x76)
MAC_ADDRESS(0x22fbd4, 0xa0)
TRmgZone::~TRmgZone()
{
}

// Provisional Complete-only bookkeeping boundary, inferred from the
// expansion in removeObject (0x54bc50); no separate retained body is known.
void TRmgZone::decrementObjectCount(TAdventureObjectType objectType)
{
    --m_objectCountByType[objectType];
}

// Both the level-occupancy pass and the bounds pass in FilterZonePositions
// copy the whole coordinate before selecting a component. That retained
// value-copy shape motivates this ordinary accessor; no DC name is known.
TRmgMapPosition TRmgZone::getLevelPosition() const
{
    TRmgMapPosition result;
    result = m_levelPosition;
    return result;
}

// Candidate placement loads all three coordinates before writing the zone,
// consistent with passing the coordinate value through an ordinary setter.
void TRmgZone::setLevelPosition(TRmgMapPosition position)
{
    m_levelPosition = position;
}

// FilterZonePositions calls this predicate at 0x53b4b7 and 0x53b5ae.
// The two center coordinates, template sizes and map-level comparison prove
// its role independently of the provisional name. Return value is in al.
// Mac 0x22fdec calls the shared two-point distance helper and reads this
// zone's size before the other zone's size.
VA(0x00532BD0, 0xA8)
MAC_ADDRESS(0x22fdc0, 0xc8) // anchor-callee 0x53b4b7/0x53b5ae; thiscall, ret 4
b8 TRmgZone::canConnect(const TRmgZone* other) const
{
    int distance = getRmgDistance(m_levelPosition, other->m_levelPosition);
    int thisSize = m_templateZone->m_size;
    int otherSize = other->m_templateZone->m_size;
    int combinedSize = thisSize + otherSize;
    if (other->m_levelPosition.m_z != m_levelPosition.m_z) {
        if (combinedSize < distance)
            return false;
        int minimumSize = thisSize;
        if (otherSize < minimumSize)
            minimumSize = otherSize;
        return combinedSize - distance > minimumSize / 2;
    }
    return 11 * combinedSize >= 10 * distance;
}

// Query passability first so trigger-mask access remains short-circuited.
static inline bool isRmgObjectFootprintCell(
    const TObjectType* prototype, unsigned int x, unsigned int y)
{
    return !prototype->isPassableCell(x, y) || prototype->isTriggerCell(x, y);
}

// Update the direction before copying its offset and adding XY.
static inline TPoint nextRmgOutlineProbe(const TPoint& position, int& direction)
{
    direction = (direction - 2) & 7;
    TPoint offset = g_rmgDirections[direction];
    return TPoint(position.m_x + offset.m_x, position.m_y + offset.m_y);
}

// Preserve the canonical point/vector addition before reversing direction.
static inline void advanceRmgOutlineWalk(TPoint& position, int& direction)
{
    position = position + TRmgVector(g_rmgDirections[direction].m_x,
        g_rmgDirections[direction].m_y);
    direction = (direction - 4) & 7;
}

VA(0x00532C80, 0x1BA)
MAC_ADDRESS(0x22fe88, 0x208)
void TRmgObjectPropertiesRef::buildOutline()
{
    // Cardinal wall following around the footprint. The world offsets are
    // nonpositive; their negations, not negative array indices, address masks.
    if (m_outline.size() > 0)
        return;
    TPoint position;
    position.m_y = 0;
    position.m_x = 0;
    while (static_cast<unsigned int>(-position.m_x) < m_prototype->getWidth()) {
        if (isRmgObjectFootprintCell(m_prototype, -position.m_x, 0))
            break;
        --position.m_x;
    }
    if (position.m_x == -m_prototype->getWidth())
        return;
    position.m_y = 1;
    TPoint start = position;
    int direction = 6;
    do {
        m_outline.push_back(position);
        int attempts = 0;
        do {
            TPoint nearby = nextRmgOutlineProbe(position, direction);
            if (nearby.m_x > 0 || static_cast<unsigned int>(-nearby.m_x) >= m_prototype->getWidth()
                || nearby.m_y > 0 || static_cast<unsigned int>(-nearby.m_y) >= m_prototype->getHeight())
                break;
            if (!isRmgObjectFootprintCell(m_prototype, -nearby.m_x, -nearby.m_y))
                break;
        } while (++attempts < 4);
        advanceRmgOutlineWalk(position, direction);
    } while (start != position);
}

VA(0x00532E40, 0x19E)
MAC_ADDRESS(0x230090, 0x16c)
void TRmgObjectPropertiesRef::buildOverlapPriorities()
{
    if (m_prioritiesInitialized)
        return;
    m_prioritiesInitialized = true;
    for (unsigned int x = 0; x < m_prototype->getWidth(); ++x) {
        int priority = !m_prototype->isUnderlay();
        unsigned int y = 0;
        for (;;) {
            if (m_prototype->isDrawCell(x, y))
                m_overlapPriorities[x][y] = priority;
            if (++y >= m_prototype->getHeight())
                break;
            if (!m_prototype->isUnderlay()) {
                if (m_prototype->isPassableCell(x, y)) {
                    if (x > 0 && !m_prototype->isPassableCell(x - 1, y))
                        priority = m_overlapPriorities[x - 1][y];
                    else
                        ++priority;
                } else {
                    if (m_prototype->isPassableCell(x, y - 1))
                        priority = 1;
                    else
                        ++priority;
                }
            }
        }
    }
}

// The generator destructor calls this body at 0x537e84, then frees the
// template. It deletes every owned slot, destroys zones, and finally name;
// the member offsets agree with the rmg.txt coordinator and zone reader.
VA(0x00532FE0, 0xB4)
MAC_ADDRESS(0x230254, 0xb0)
TRmgTemplate::~TRmgTemplate()
{
    for (int zone = 0; zone < m_zones.size(); ++zone)
        delete m_zones[zone];
}

// The rmg.txt connection reader calls this for both endpoint identifiers.
// It searches the template's pointer vector and compares each slot's first
// field; ret 4 fixes the member's one integer argument.
VA(0x005330A0, 0x3E)
MAC_ADDRESS(0x230304, 0x48)
TRmgTemplateZone* TRmgTemplate::findZone(int zoneIndex)
{
    for (int zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->m_zoneIndex == zoneIndex)
            return m_zones[zone];
    }
    return 0;
}

// Shipyard water probing copies all three fields from the object before
// adding offsets. This ordinary value accessor models that copy boundary;
// the source name is inferred from the Complete-only retail use.
TRmgMapPosition type_object::getPosition() const
{
    return m_position;
}

VA(0x005330E0, 0x39)
MAC_ADDRESS(0x2303ec, 0x5c)
type_object::type_object(TRmgObjectPropertiesRef* newProperties)
{
    m_properties = newProperties;
    ++m_properties->m_refCount;
    m_position.m_x = -1;
    m_position.m_y = -1;
    m_position.m_z = -1;
    clearPlacementMarks();
}

// The base-sized default-payload classes retain separate serialization
// vtables. Their ordinary constructors expand the same canonical base call.
// These Complete generator classes have no Dreamcast RMG compiland or
// class/procedure counterparts. The factories 0x534870/0x534970/0x534a00/
// 0x534a90 allocate only the base's 0x1c bytes and install 0x640ac4/0x640b24/
// 0x640b34/0x640b54 respectively; writer slot 3 proves each payload role.
rmgResourceObject::rmgResourceObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

rmgScholarObject::rmgScholarObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

rmgShrineObject::rmgShrineObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

rmgSpellScrollObject::rmgSpellScrollObject(TRmgObjectPropertiesRef* properties, int spell)
    : type_object(properties), m_spell(spell)
{
}

rmgWitchHutObject::rmgWitchHutObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

// The three simple reward factories expand this same constructor. The
// vector's automatic construction precedes these scalar/default writes.
// Complete's generation classes have no Dreamcast RMG compiland, class or
// procedure counterparts. DC's BlackBoxData and NewfullMap::readBlackBox
// are the gameplay payload and reader, not these generator definitions.
// Retail 0x534380/0x534410/0x534490 allocate 0x54 bytes, install 0x640ad4,
// clear vector words +0x48/+0x4c/+0x50, and set the reward defaults below.
rmgBlackBoxObject::rmgBlackBoxObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
    m_creatureType = -1;
    m_creatureCount = 0;
    m_experience = 0;
    memset(m_resources, 0, sizeof(m_resources));
}

// Base-object vtable 0x640a74 slot 0 retains the generated deleting wrapper;
// its non-deleting half is the shared refcount release at 0x5338d0.
VA_COMPGEN(0x00533120, 0x2D, SCALAR_DELETING_DTOR, type_object)

VA(0x00533150, 0x12)
MAC_ADDRESS(0x2304a0, 0x1c)
void type_object::clearPlacementMarks()
{
    m_candidateCovers = false;
    m_candidateBehind = false;
    m_adjacentToCandidate = false;
    m_overlapsCandidate = false;
    m_blockedByCandidate = false;
}

// Keep coordinate reads interleaved with their one-byte writes.
static inline void writeRmgMapPosition(
    TAbstractFile* outputFile, const TRmgMapPosition& position)
{
    writeValue<char>(outputFile, position.m_x);
    writeValue<char>(outputFile, position.m_y);
    writeValue<char>(outputFile, position.m_z);
}

// One write per fixed-size block.
template <int N>
static inline void writeRmgReservedBytes(TAbstractFile* outputFile)
{
    char reserved[N];
    memset(reserved, 0, sizeof(reserved));
    outputFile->write(reserved, sizeof(reserved));
}

// Retail stages the two reserved bytes in a zeroed int slot and writes only
// its low short.
static inline void writeRmgReservedWord(TAbstractFile* outputFile)
{
    int reserved = 0;
    outputFile->write(&reserved, sizeof(short));
}

VA(0x00533170, 0x79)
MAC_ADDRESS(0x2304c8, 0xfc)
void type_object::write(TAbstractFile* outputFile, int version)
{
    writeRmgMapPosition(outputFile, m_position);
    writeValue<int>(outputFile, m_properties->m_prototypeIndex);
    writeRmgReservedBytes<5>(outputFile);
}

VA(0x005331F0, 0xFD)
MAC_ADDRESS(0x23062c, 0x144) // anchor-vtable 0x640a84+0x0c; thiscall, ret 8
void rmgMonsterObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<int>(outputFile, m_objectId);
    }
    writeValue<short>(outputFile, m_count);
    writeValue<char>(outputFile, m_disposition);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, 0);
    writeRmgReservedWord(outputFile);
}

VA(0x005332F0, 0x16A)
MAC_ADDRESS(0x2307d8, 0x20c)
void rmgTownObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<int>(outputFile, m_objectId);
    }
    writeValue<char>(outputFile, m_player);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, m_townOption);
    char spells[9];
    memset(spells, 0, sizeof(spells));
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE)
        outputFile->write(spells, sizeof(spells));
    outputFile->write(spells, sizeof(spells));
    writeValue<int>(outputFile, 0);
    if (version >= RMG_MAP_SHADOW_OF_DEATH) {
        writeValue<char>(outputFile, -1);
    }
    writeRmgReservedBytes<3>(outputFile);
}

VA(0x00533460, 0xA0)
MAC_ADDRESS(0x230a1c, 0x78) // base serialization plus unowned player and reserved bytes
void rmgOwnableObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, -1); // player
    writeRmgReservedBytes<3>(outputFile);
}

// Artifact vtable 0x640ab4 is the only change from the base constructor.
// Its factory at 0x5341f0 expands this ordinary constructor and the canonical
// base body; no independent retained artifact-constructor address is claimed.
rmgArtifactObject::rmgArtifactObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

rmgSeerHutObject::rmgSeerHutObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
    m_experience = 0;
    m_artifact = -1;
    m_resourceType = GOLD;
    m_resourceCount = 0;
    m_creatureType = -1;
    m_creatureCount = 0;
}

rmgQuestArtifactObject::rmgQuestArtifactObject(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, rmgSeerHutObject* seerHut,
    type_treasure_def* definition)
    : rmgArtifactObject(properties), m_generator(generator),
      m_seerHut(seerHut), m_definition(definition)
{
}

rmgKeyTentObject::rmgKeyTentObject(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, int value)
    : type_object(properties), m_generator(generator), m_value(value)
{
}

VA(0x00533500, 0x8A)
MAC_ADDRESS(0x230acc, 0x50)
void rmgArtifactObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, 0); // has custom treasure
}

VA_COMPGEN(0x00533590, 0x21, SCALAR_DELETING_DTOR, rmgOwnableObject)

VA(0x005335C0, 0xB2)
MAC_ADDRESS(0x230bb4, 0x78) // anchor-vtable 0x640ac4 slot 3; thiscall ret 8
void rmgResourceObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, 0); // has custom treasure
    writeValue<int>(outputFile, 0); // amount
    writeValue<int>(outputFile, 0);
}

VA_COMPGEN(0x00533680, 0x21, SCALAR_DELETING_DTOR, rmgBlackBoxObject)

VA_COMPGEN(0x005336B0, 0x36, IMPLICIT_DTOR, rmgBlackBoxObject)
MAC_COMPGEN_ADDRESS(0x251488, 0x7c, IMPLICIT_DTOR, rmgBlackBoxObject)

// Read the count only after the id write.
static inline void writeRmgCreatureReward(
    TAbstractFile* outputFile, int version, int creature, const int& count)
{
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE)
        writeValue<short>(outputFile, creature);
    else
        writeValue<char>(outputFile, creature);
    writeValue<short>(outputFile, count);
}

VA(0x005336F0, 0x1E0)
MAC_ADDRESS(0x230cac, 0x32c)
void rmgBlackBoxObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, 0); // has custom treasure
    writeValue<int>(outputFile, m_experience);
    writeValue<int>(outputFile, 0); // mana
    writeValue<char>(outputFile, 0); // morale
    writeValue<char>(outputFile, 0); // luck
    outputFile->write(m_resources, sizeof(m_resources));
    writeValue<int>(outputFile, 0); // primary skills
    writeValue<char>(outputFile, 0); // secondary skill count
    writeValue<char>(outputFile, 0); // artifact count
    writeValue<char>(outputFile, m_spells.size()); // spell count
    for (unsigned int spellIndex = 0; spellIndex < m_spells.size(); ++spellIndex) {
        writeValue<char>(outputFile, m_spells[spellIndex]);
    }
    if (m_creatureType == -1) {
        writeValue<char>(outputFile, 0); // creature count
    } else {
        writeValue<char>(outputFile, 1); // creature count
        writeRmgCreatureReward(outputFile, version, m_creatureType, m_creatureCount);
    }
    writeValue<int>(outputFile, 0);
    writeValue<int>(outputFile, 0);
}

// Base and ownable destructors share this 13-byte body and base-vtable
// relocation at 0x5338d0. The base cleanup is visible to the ownable destructor.
VA(0x005338D0, 0x0D)
MAC_ADDRESS(0x230448, 0x58)
type_object::~type_object()
{
    --m_properties->m_refCount;
}

// Callers snapshot value/position before removal; query the owning zone afterward.
static inline void replaceRmgObjectWithTreasure(type_random_map_generator* generator,
    type_object* object, int value, TRmgMapPosition position)
{
    generator->removeObject(object);
    TRmgZone* zone = generator->m_zones[
        generator->m_map.getMapItem(position)->m_zoneState.m_zone];
    int actualValue;
    type_object* replacement = generator->createTreasureObject(
        zone, value, value * 3 / 2, &actualValue, false, false, false, position);
    if (replacement)
        generator->addObject(replacement, position);
}

VA(0x005338E0, 0xD4)
MAC_ADDRESS(0x231030, 0x68)
b8 rmgKeyTentObject::isWritable()
{
    if (m_generator->placeKeyTentGuard(this, m_value * 3 / 2))
        return true;
    type_random_map_generator* generator = m_generator;
    int value = m_value;
    TRmgMapPosition position = m_position;
    replaceRmgObjectWithTreasure(generator, this, value, position);
    return false;
}

// Vtable 0x640af4 owns a pending polymorphic seer-hut object. The retained
// destructor deletes it before the ordinary artifact/base property release.
VA_COMPGEN(0x005339C0, 0x21, SCALAR_DELETING_DTOR, rmgQuestArtifactObject)

VA(0x005339F0, 0x58)
MAC_ADDRESS(0x231100, 0x9c)
rmgQuestArtifactObject::~rmgQuestArtifactObject()
{
    delete m_seerHut;
}

// The generator places the seer hut on success and takes ownership. On
// failure the wrapper destroys its pending hut. Both paths clear ownership.
VA(0x00533A50, 0x33)
MAC_ADDRESS(0x2311fc, 0x78) // vtable 0x640af4 slot 2 + retained callee 0x54b490
b8 rmgQuestArtifactObject::isWritable()
{
    if (m_generator->placeQuestArtifact(this)) {
        m_seerHut = 0;
        return true;
    }
    delete m_seerHut;
    m_seerHut = 0;
    return false;
}

// Vtable 0x640b04 serializes the artifact quest followed by exactly one
// reward: experience, creatures, or resources, in that precedence order.
// AB adds the quest kind, artifact count, deadline and three empty strings.
VA(0x00533A90, 0x1E0)
MAC_ADDRESS(0x2312d0, 0x31c)
void rmgSeerHutObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<char>(outputFile, 5); // quest kind
        writeValue<char>(outputFile, 1); // artifact count
        writeValue<short>(outputFile, m_artifact);
        writeValue<int>(outputFile, -1); // deadline
        writeValue<int>(outputFile, 0); // first visit length
        writeValue<int>(outputFile, 0); // next visit length
        writeValue<int>(outputFile, 0); // completion length
    } else {
        writeValue<char>(outputFile, m_artifact);
    }
    if (m_experience > 0) {
        writeValue<char>(outputFile, 1); // reward kind
        writeValue<int>(outputFile, m_experience);
    } else if (m_creatureType != -1) {
        writeValue<char>(outputFile, 10); // reward kind
        writeRmgCreatureReward(outputFile, version, m_creatureType, m_creatureCount);
    } else {
        writeValue<char>(outputFile, 5); // reward kind
        writeValue<char>(outputFile, m_resourceType);
        writeValue<int>(outputFile, m_resourceCount);
    }
    writeRmgReservedWord(outputFile);
}

// The hero-object factory marks the selected index in disabledHeroes before
// construction. Vtable 0x640b14 slot 1 clears that byte when the reservation
// is released.
rmgHeroObject::rmgHeroObject(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, const int& objectId, int heroIndex,
    int experience)
    : type_object(properties)
{
    m_generator = generator;
    m_heroIndex = heroIndex;
    m_objectId = objectId;
    m_experience = experience;
}

VA(0x00533C70, 0x0F)
MAC_ADDRESS(0x231644, 0x18)  // factory 0x5348d0; Complete-only RMG object
void rmgHeroObject::releaseReservation()
{
    m_generator->m_disabledHeroes[m_heroIndex] = false;
}

VA(0x00533C80, 0x1E4)
MAC_ADDRESS(0x23165c, 0x318) // anchor-vtable + ordered versioned H3M writes; ret 8
void rmgHeroObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<int>(outputFile, m_objectId);
    }
    writeValue<char>(outputFile, -1); // owner
    writeValue<char>(outputFile, m_heroIndex);
    writeValue<char>(outputFile, 0); // custom name
    if (version >= RMG_MAP_SHADOW_OF_DEATH) {
        writeValue<char>(outputFile, m_experience != 0); // custom experience
        if (m_experience != 0) {
            writeValue<int>(outputFile, m_experience);
        }
    } else {
        writeValue<int>(outputFile, m_experience);
    }
    writeValue<char>(outputFile, 0); // custom portrait
    writeValue<char>(outputFile, 0); // custom secondary skills
    writeValue<char>(outputFile, 0); // custom armies
    writeValue<char>(outputFile, 0); // group formation
    writeValue<char>(outputFile, 0); // custom artifacts
    writeValue<char>(outputFile, -1); // patrol radius
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<char>(outputFile, 0); // custom biography
        writeValue<char>(outputFile, -1); // sex
        if (version >= RMG_MAP_SHADOW_OF_DEATH) {
            writeValue<char>(outputFile, 0); // custom spells
            writeValue<char>(outputFile, 0); // custom primary skills
        } else {
            writeValue<char>(outputFile, -2); // spell
        }
    }
    writeRmgReservedBytes<16>(outputFile);
}

VA(0x00533E70, 0xC3)
MAC_ADDRESS(0x2319ac, 0xbc) // anchor-vtable + default serialization bytes; ret 8
void rmgScholarObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, -1); // reward kind
    writeValue<char>(outputFile, 0); // reward value
    writeValue<int>(outputFile, 0);
    writeRmgReservedWord(outputFile);
}

VA(0x00533F40, 0xAF)
MAC_ADDRESS(0x231aa0, 0x9c) // anchor-vtable + ordered write sizes; ret 8
void rmgShrineObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, -1); // spell
    writeRmgReservedWord(outputFile);
    writeValue<char>(outputFile, 0); // reserved byte
}

VA(0x00533FF0, 0xC2)
MAC_ADDRESS(0x231b84, 0xc8)
void rmgSpellScrollObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, 0); // message
    writeValue<char>(outputFile, m_spell);
    writeRmgReservedWord(outputFile);
    writeValue<char>(outputFile, 0);
}

VA(0x005340C0, 0x93)
MAC_ADDRESS(0x231c84, 0x68) // anchor-vtable + version guard and mask 0xefdf; ret 8
void rmgWitchHutObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<unsigned int>(outputFile, 0xefdf); // allowed skills
    }
}

// Complete-only helper called by InitializeObjectGenerators at 0x538b10.
// The four argument loads, five stores, vtable relocation, and `ret 0x10`
// independently prove this constructor and the shared 0x14-byte prefix.
VA(0x00534160, 0x27)
MAC_ADDRESS(0x231cec, 0x1c)
type_treasure_def::type_treasure_def(
    int newObjectType, int newSubtype, int newValue, int newDensity)
{
    m_objectType = newObjectType;
    m_subtype = newSubtype;
    m_value = newValue;
    m_density = newDensity;
}

VA(0x00534190, 0x06)
MAC_ADDRESS(0x231d10, 0x8)
int type_treasure_def::getValue(TRmgZone*, type_random_map_generator*)
{
    return m_value;
}

VA(0x005341A0, 0x4D)
MAC_ADDRESS(0x231d18, 0x50)
type_object* type_treasure_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new type_object(properties);
}

VA(0x005341F0, 0x53)
MAC_ADDRESS(0x231dac, 0x50)
type_object* type_artifact_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgArtifactObject(properties);
}

VA(0x00534250, 0xB5)
MAC_ADDRESS(0x231dfc, 0x108)
type_black_box_creature_def::type_black_box_creature_def(int newCreatureType)
    : type_treasure_def(BLACK_BOX, 0, -1, 3),
      m_creatureType(newCreatureType)
{
    m_creatureCount =
        g_rmgCreatureValueByLevel[g_creatureTypeTraits[newCreatureType].m_level]
        / g_creatureTypeTraits[newCreatureType].m_aiValue;

    if (m_creatureCount > 50)
        m_creatureCount = ((m_creatureCount + 5) / 10) * 10;
    else if (m_creatureCount > 12)
        m_creatureCount = ((m_creatureCount + 2) / 5) * 5;
    else if (m_creatureCount > 5)
        m_creatureCount = ((m_creatureCount + 1) / 2) * 2;
}

// Keep integer multiplication before division and the zero-zone guard.
static inline int adjustRmgValueForAlignment(int value, int alignmentCount, int zoneCount)
{
    if (zoneCount > 0)
        value += alignmentCount * value / zoneCount;
    return value;
}

// Creature-definition vtable 0x640b7c slot 1. The zone test reads +8
// (townType2), and the alignment weighting uses the generator's active-zone
// counts.
VA(0x00534310, 0x64)
MAC_ADDRESS(0x231f04, 0x6c)
int type_black_box_creature_def::getValue(
    TRmgZone* zone, type_random_map_generator* generator)
{
    int alignment = g_creatureTypeTraits[m_creatureType].m_townType;
    if (alignment != zone->m_townType2)
        return -1;
    int value = g_creatureTypeTraits[m_creatureType].m_aiValue * m_creatureCount;
    int alignmentCount = 0;
    if (alignment != -1)
        alignmentCount = generator->m_activeZoneCountsByAlignment[alignment];
    int zoneCount = generator->m_activeZoneCount;
    return adjustRmgValueForAlignment(value, alignmentCount, zoneCount);
}

VA(0x00534380, 0x85)
MAC_ADDRESS(0x231f70, 0x6c)
type_object* type_black_box_creature_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    rmgBlackBoxObject* object = new rmgBlackBoxObject(properties);
    int count = m_creatureCount;
    object->m_creatureType = m_creatureType;
    object->m_creatureCount = count;
    return object;
}

VA(0x00534410, 0x7F)
MAC_ADDRESS(0x23204c, 0x64)
type_object* type_black_box_experience_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    rmgBlackBoxObject* object = new rmgBlackBoxObject(properties);
    object->m_experience = m_experience;
    return object;
}

VA(0x00534490, 0x84)
MAC_ADDRESS(0x232108, 0x6c)
type_object* type_black_box_gold_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    rmgBlackBoxObject* object = new rmgBlackBoxObject(properties);
    object->m_resources[GOLD] += m_gold;
    return object;
}

VA(0x00534520, 0x267)
MAC_ADDRESS(0x2321ec, 0xc0) // anchor-vtable 0x640ba0 slot 0; object vptr 0x640ad4; ret 0xc
type_object* type_black_box_spells_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    rmgBlackBoxObject* object = new rmgBlackBoxObject(properties);
    for (int level = m_maximumLevel; level >= m_minimumLevel; --level) {
        for (long spell = 0; spell < 70; ++spell) {
            if (!(g_spellTraits[spell].m_flags & 0x2000)
                && g_spellTraits[spell].m_level == level
                && (g_spellTraits[spell].m_school & m_schoolMask))
                object->m_spells.push_back(spell);
        }
    }
    return object;
}

// Both dwelling-definition tables share this ownable-object factory.
VA(0x00534790, 0x53)
MAC_ADDRESS(0x2322e4, 0x50)
type_object* type_dwelling_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgOwnableObject(properties);
}

VA(0x005347F0, 0x79)
MAC_ADDRESS(0x23237c, 0x88)
int type_map_dwelling_def::getValue(TRmgZone* zone, type_random_map_generator* generator)
{
    const TCreatureTypeTraits& creature =
        g_creatureTypeTraits[g_creatureGenerator1Types[m_subtype]];
    if (creature.m_townType != zone->m_townType2)
        return -1;

    int value = creature.m_growthRate * creature.m_aiValue;
    int alignmentZoneCount = 0;
    if (creature.m_townType != -1)
        alignmentZoneCount = generator->m_activeZoneCountsByAlignment[creature.m_townType];
    value = adjustRmgValueForAlignment(value, alignmentZoneCount, generator->m_activeZoneCount);
    return value + creature.m_aiValue * alignmentZoneCount / 2;
}

// Resource-definition table 0x640bc4 constructs the base-sized resource
// object and replaces its vptr with 0x640ac4 after the canonical base call.
VA(0x00534870, 0x53)
MAC_ADDRESS(0x23243c, 0x50) // anchor-definition table + allocated-object vptr; ret 0xc
type_object* type_resource_lump_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgResourceObject(properties);
}

// Prison-definition table 0x640bd0 selects this factory. Retail reserves
// a hero, returns null on exhaustion, and expands the 0x2c-byte object's
// constructor with the definition's experience and the next generator id.
// Complete-only: no Dreamcast counterpart; constructor spelling provisional.
// The post-incremented id's const-reference temporary lives through construction.
VA(0x005348D0, 0x93)
MAC_ADDRESS(0x2324e4, 0x84) // anchor-definition/object vtables + selectPrisonHero
type_object* type_prison_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    int heroIndex = generator->selectPrisonHero();
    if (heroIndex == -1)
        return 0;
    return new rmgHeroObject(properties, generator,
        generator->m_nextObjectId++, heroIndex, m_experience);
}

// Scholar-definition table 0x640bdc selects object vtable 0x640b24.
VA(0x00534970, 0x53)
MAC_ADDRESS(0x2325b0, 0x50) // anchor-definition and object vtables; ret 0xc
type_object* type_scholar_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgScholarObject(properties);
}

VA(0x005349D0, 0x29)
MAC_ADDRESS(0x232600, 0x44)
type_shrine_def::type_shrine_def(int newObjectType, int newValue)
    : type_treasure_def(newObjectType, 0, newValue, 100)
{
}

// Shrine-definition table 0x640be8 selects object vtable 0x640b34.
VA(0x00534A00, 0x53)
MAC_ADDRESS(0x232644, 0x50) // anchor-definition and object vtables; ret 0xc
type_object* type_shrine_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgShrineObject(properties);
}

VA(0x00534A60, 0x25)
MAC_ADDRESS(0x232694, 0x48)
type_witch_hut_def::type_witch_hut_def()
    : type_treasure_def(WITCH_HUT, 0, 1500, 80)
{
}

// Witch-hut definition 0x640bf4 selects object vtable 0x640b54.
VA(0x00534A90, 0x53)
MAC_ADDRESS(0x2326dc, 0x50) // anchor-definition and object vtables; ret 0xc
type_object* type_witch_hut_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgWitchHutObject(properties);
}

// Check the prototype before the pool; creature alignment/value policy follows.
static inline bool canUseRmgSeerHutPrototype(
    const type_random_map_generator* generator, int prototypeIndex)
{
    return generator->m_nextSeerHutPrototypeIndex == prototypeIndex
        && !generator->m_questArtifactPoolLow;
}

// Callers allocate the hut before this operation and write its payload afterward,
// even if wrapper allocation returns null.
static inline rmgQuestArtifactObject* createRmgQuestArtifactForHut(
    type_random_map_generator* generator, rmgSeerHutObject* seerHut,
    type_treasure_def* definition)
{
    TRmgObjectPropertiesRef* artifact = generator->selectObjectPrototype(
        eTerrainDirt, RANDOM_ARTIFACT, 0);
    return new rmgQuestArtifactObject(artifact, generator, seerHut, definition);
}

VA(0x00534AF0, 0x9E)
MAC_ADDRESS(0x23277c, 0x68)
int type_quest_creature_def::getValue(TRmgZone* zone, type_random_map_generator* generator)
{
    if (!canUseRmgSeerHutPrototype(generator, m_subtype))
        return -1;
    int value = type_black_box_creature_def::getValue(zone, generator);
    return (2 * value - 4000) / 3;
}

VA(0x00534B90, 0xE7)
MAC_ADDRESS(0x2327ec, 0xa4) // anchor-vtable + canonical hut/wrapper allocations; ret 0xc
type_object* type_quest_creature_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    rmgSeerHutObject* seerHut = new rmgSeerHutObject(properties);
    rmgQuestArtifactObject* object = createRmgQuestArtifactForHut(generator, seerHut, this);
    int count = m_creatureCount;
    seerHut->m_creatureType = m_creatureType;
    seerHut->m_creatureCount = count;
    return object;
}

// Seer-hut definition tables 0x640c0c and 0x640c18 share this ICF body.
// Both classes exist independently and use the same availability checks:
// current prototype at +0xf58, then the exhausted-artifact flag at +0x10b4.
VA(0x00534C80, 0x34)
MAC_ADDRESS(0x232900, 0x34)
int type_quest_experience_def::getValue(TRmgZone*, type_random_map_generator* generator)
{
    if (!canUseRmgSeerHutPrototype(generator, m_subtype))
        return -1;
    return m_value;
}

int type_quest_gold_def::getValue(TRmgZone*, type_random_map_generator* generator)
{
    if (!canUseRmgSeerHutPrototype(generator, m_subtype))
        return -1;
    return m_value;
}

VA(0x00534CC0, 0xE1)
MAC_ADDRESS(0x23293c, 0x9c)
type_object* type_quest_experience_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    rmgSeerHutObject* seerHut = new rmgSeerHutObject(properties);
    rmgQuestArtifactObject* object = createRmgQuestArtifactForHut(generator, seerHut, this);
    seerHut->m_experience = m_experience;
    return object;
}

VA(0x00534DB0, 0xE8)
MAC_ADDRESS(0x232a84, 0xa4)
type_object* type_quest_gold_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    rmgSeerHutObject* seerHut = new rmgSeerHutObject(properties);
    rmgQuestArtifactObject* object = createRmgQuestArtifactForHut(generator, seerHut, this);
    int amount = m_gold;
    seerHut->m_resourceType = GOLD;
    seerHut->m_resourceCount = amount;
    return object;
}

VA(0x00534EA0, 0x30)
MAC_ADDRESS(0x232b28, 0x58)
type_spell_scroll_def::type_spell_scroll_def(int newSpellLevel, int newValue)
    : type_treasure_def(SPELL_SCROLL, 0, newValue, 30)
{
    m_spellLevel = newSpellLevel;
}

// Query the excluded flag, school membership, then level in that order.
static inline bool isRmgScrollSpell(int spell, int level)
{
    return !(g_spellTraits[spell].m_flags & 0x2000)
        && g_spellTraits[spell].m_schoolBits
        && g_spellTraits[spell].m_level == level;
}

VA(0x00534ED0, 0xC3)
MAC_ADDRESS(0x232b80, 0x108)
type_object* type_spell_scroll_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    // Count then select the nth eligible spell: one random draw, in spell order.
    // Retail assumes at least one eligible spell for the configured level.
    int eligibleSpellCount = 0;
    int spell;
    for (spell = 0; spell < 70; ++spell) {
        if (isRmgScrollSpell(spell, m_spellLevel))
            ++eligibleSpellCount;
    }
    int selectedIndex = rand() % eligibleSpellCount;
    for (spell = 0; spell < 70; ++spell) {
        if (isRmgScrollSpell(spell, m_spellLevel)) {
            if (selectedIndex-- <= 0)
                break;
        }
    }
    return new rmgSpellScrollObject(properties, spell);
}

VA(0x00534FA0, 0x21)
MAC_ADDRESS(0x232cd4, 0x20)
int type_key_tent_def::getValue(TRmgZone*, type_random_map_generator* generator)
{
    if (generator->m_nextKeyTentColor != m_subtype)
        return -1;
    return m_value;
}

// Definition vtable 0x640c30 slot 0 constructs the concrete 0x24-byte tent.
// The generator is the second factory argument; +0x20 receives m_value,
// not the definition subtype/color (that is already in its properties).
VA(0x00534FD0, 0x64)
MAC_ADDRESS(0x232cfc, 0x70)
type_object* type_key_tent_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    return new rmgKeyTentObject(properties, generator, m_value);
}

// Called after map/vector construction. Clears entries without deleting objects;
// ownership is established by 0x547360 and 0x5466e0. Retail reset copies at
// 0x547625/0x547717 call the ordinary two-coordinate lookup.
VA(0x00535040, 0xC6)
MAC_ADDRESS(0x232e70, 0xa8) // anchor-callee 0x5473d2; thiscall, ret 0; retail-only
void TRmgTreasureGroup::reset()
{
    m_objects.clear();
    m_outline.clear();
    m_map.clear();
    m_hasGuard = false;
    m_ready = false;
    type_random_map& map = m_map;
    TRmgMapItem* item = map.getMapItem(0, 0);
    int width = map.m_mapWidth;
    int height = map.m_mapHeight;
    int count = width * height;
    while (count--) {
        item->setTerrain(eTerrainDirt, 0, false, false);
        ++item;
    }
}

// Native 0x232fc4 marks the group ready and flags every surface-outline cell.
// Its retained callers are assembleTreasureGroup, placeQuestArtifact and
// placeKeyTentGuard; Windows expands the same shared loop in those callers.
MAC_ADDRESS(0x232fc4, 0x64)
void TRmgTreasureGroup::markPlacementOutline()
{
    m_ready = true;
    for (unsigned int index = 0; index < m_outline.size(); ++index)
        m_map.getMapItem(m_outline[index].m_x,
            m_outline[index].m_y)->m_tileData.m_placementOutline = true;
}

// Native 0x233d18 tests each group's object trait before the caller checks
// its separate guard flag. The only retained caller is 0x24b22c.
MAC_ADDRESS(0x233d18, 0x60)
b8 TRmgTreasureGroup::objectsAllowEntrances() const
{
    for (unsigned int index = 0; index < m_objects.size(); ++index) {
        int objectType = m_objects[index]->m_properties->m_prototype->getObjectType();
        if (!g_adventureObjectTraits[objectType].m_trait2)
            return false;
    }
    return true;
}

// Mac keeps this shared group insertion at 0x233d78. Its four callers
// pass one object and a two-dimensional point; map storage uses level zero.
MAC_ADDRESS(0x233d78, 0xd4)
void TRmgTreasureGroup::addObject(type_object* object, TPoint point)
{
    m_objects.push_back(object);
    TRmgMapPosition position(point.m_x, point.m_y, 0);
    m_map.addObject(*object, position);
}

// Complete-only group helpers: assembly passes the guard pointer in one
// stack dword to 0x535110 (AL result); its first and last calls trace the
// group's closed outline. 0x535ee0 appends points at +0x38 until closure.
// Provisional names describe these retail roles.
// Native 0x233234/0x23333c/0x2334a0/0x2335e0 uses the surface
// lookup overload. Its three materialized land predicates and copied point
// sums at 0x233458/0x23356c use the shared isPassableLand and operator+.
VA(0x00535110, 0x4AB)
MAC_ADDRESS(0x233028, 0x6a8) // anchor-callee 0x546843; thiscall, ret 4
b8 TRmgTreasureGroup::addGuard(type_object* guard)
{
    traceOutline();
    // Retail 0x535150 saves the existing object's prototype; 0x535405
    // still reads that last prototype's trigger after inserting the guard.
    // Preserve that retail quirk: substituting the guard prototype changes
    // generated maps. This path also assumes the group already owns an object.
    TObjectType* prototype;
    for (unsigned int objectIndex = 0; objectIndex < m_objects.size(); ++objectIndex) {
        type_object* object = m_objects[objectIndex];
        prototype = object->m_properties->m_prototype;
        TRmgMapPosition entrance = getRmgPlacedObjectEntrance(object);
        unsigned int direction = g_adventureObjectTraits[prototype->getObjectType()].m_trait1
            ? RMG_DIRECTION_COUNT : 5;
        while (direction--) {
            TRmgMapPosition position = entrance + g_rmgDirections[direction];
            TRmgMapItem* item = m_map.getMapItem(position);
            if (item->isRoadEntrance() || !item->isPassableLand())
                continue;
            item->markBorderObject();
            for (int x = position.m_x - 1; x <= position.m_x + 1; ++x) {
                for (int y = position.m_y - 1; y <= position.m_y + 1; ++y) {
                    TRmgMapItem* nearby = m_map.getMapItem(x, y);
                    if (nearby->isPassableLand()
                        && !nearby->isRoadEntrance())
                        nearby->releasePathClearance();
                }
            }
        }
    }
    TRmgObjectPropertiesRef* guardProperties = guard->m_properties;
    for (unsigned int index = m_outline.size(); index--;) {
        TPoint point = m_outline[index];
        TRmgMapPosition position;
        position.m_x = point.m_x;
        position.m_y = point.m_y;
        position.m_z = 0;
        if (!m_map.getMapItem(point.m_x, point.m_y)->hasBorderObject()
            || !canFitObject(guardProperties, position))
            m_outline.erase(m_outline.begin() + index);
    }
    if (!m_outline.size())
        return false;
    unsigned int outlineCount = m_outline.size();
    TPoint guardPosition = m_outline[rand() % outlineCount];
    addObject(guard, guardPosition);
    guardPosition.m_x -= prototype->m_triggerCell.m_x;
    guardPosition.m_y -= prototype->m_triggerCell.m_y;
    int guardType = guardProperties->m_prototype->getObjectType();
    for (int direction = 0; direction < RMG_DIRECTION_COUNT; ++direction) {
        TPoint point = g_rmgDirections[direction]
            + TRmgVector(guardPosition.m_x, guardPosition.m_y);
        TRmgMapItem* item = m_map.getMapItem(point.m_x, point.m_y);
        if (!item->isPassableLand())
            continue;
        if (guardType == BORDER_GUARD && item->hasBorderObject())
            continue;
        item->openPath();
        int fanDirection;
        unsigned int count;
        if (direction & 1) {
            fanDirection = (direction - 1) & 7;
            count = 3;
        } else {
            fanDirection = direction;
            count = 1;
        }
        while (count--) {
            TPoint nearby = g_rmgDirections[fanDirection]
                + TRmgVector(point.m_x, point.m_y);
            if (m_map.containsXY(nearby)) {
                TRmgMapItem* next = m_map.getMapItem(nearby.m_x, nearby.m_y);
                if (!next->hasBorderObject() && !next->hasPathClearance()
                    && next->isPassableLand()) {
                    next->openPath();
                }
            }
            fanDirection = (fanDirection + 1) & 7;
        }
    }
    m_guardPosition = guardPosition;
    m_hasGuard = true;
    m_outline.clear();
    updateBounds();
    traceOutline();
    return true;
}

// Retained immediately before canFitObject. Three stores and ret 12 prove
// the value ABI and 12-byte layout; original source-file ownership is inferred.
VA(0x005355C0, 0x1A)
TRmgMapPosition::TRmgMapPosition(int newX, int newY, int newZ)
    : TPoint(newX, newY), m_z(newZ)
{
}

// Mac 0x2337b4, 0x233858 and 0x233978 query the surface XY plane using a
// shared translated trigger snapshot. Keep the native first-failure join;
// placement checking receives the original by-value position.
VA(0x005355E0, 0x1F9)
MAC_ADDRESS(0x2336d0, 0x374) // anchor-callee 0x535ab9; thiscall, ret 0x10
b8 TRmgTreasureGroup::canFitObject(TRmgObjectPropertiesRef* properties,
    TRmgMapPosition position)
{
    TObjectType* prototype = properties->m_prototype;
    int objectType = prototype->getObjectType();
    TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, prototype->m_triggerCell);
    TRmgVector origin(entrance.m_x, entrance.m_y);
    if (!g_adventureObjectTraits[objectType].m_trait1) {
        for (int direction = 5; direction < RMG_DIRECTION_COUNT; ++direction) {
            TPoint nearby = g_rmgDirections[direction] + origin;
            if (m_map.getMapItem(nearby.m_x, nearby.m_y)->isRoadEntrance())
                goto placementFailure;
        }
    }
    {
        for (int direction = 0; direction < 5; ++direction) {
            TPoint nearby = g_rmgDirections[direction] + origin;
            TRmgMapItem* item = m_map.getMapItem(nearby.m_x, nearby.m_y);
            if (item->isRoadEntrance()) {
                int neighborType = item->m_objects[0]->m_properties->m_prototype->getObjectType();
                if (!allowsRmgSharedObjectEntrance(neighborType))
                    goto placementFailure;
            }
        }
    }
    if (objectType != MONSTER && objectType != BORDER_GUARD) {
        if (m_map.isPlacementBlocked(properties, position, -1, true)) {
placementFailure:
            return false;
        }
    } else {
        if (m_map.isPlacementBlocked(properties, position, -1, false))
            return false;
        int direction;
        for (direction = 0; direction < RMG_DIRECTION_COUNT; ++direction) {
            TPoint nearby = g_rmgDirections[direction] + origin;
            TRmgMapItem* item = m_map.getMapItem(nearby.m_x, nearby.m_y);
            if (!item->isRoadEntrance() && item->isPassableLand()
                && !item->hasBorderObject())
                break;
        }
        if (direction == RMG_DIRECTION_COUNT)
            return false;
    }
    return true;
}

// Mac retains addObject at 0x233cf0 and reuses the candidate position for
// selection. Native 0x233aa0..0x233ab4 snapshots the trigger before conversion;
// 0x233b70..0x233c18 uses the ordinary -= and two returned point sums.
VA(0x00535970, 0x240)
MAC_ADDRESS(0x233a44, 0x2d4) // anchor-callee 0x546680; thiscall, ret 4
b8 TRmgTreasureGroup::tryAddObject(type_object* object)
{
    TRmgObjectPropertiesRef* properties = object->m_properties;
    TObjectType* prototype = properties->m_prototype;
    TRmgZoneBounds bounds;
    bounds.m_minimumX = prototype->getWidth() + 2;
    bounds.m_minimumY = prototype->getHeight() + 2;
    bounds.m_maximumX = m_map.getWidth() - 3;
    bounds.m_maximumY = m_map.getHeight() - 3;
    TObjectType::TPoint triggerCell = prototype->m_triggerCell;
    TPoint trigger(triggerCell.m_x, triggerCell.m_y);
    std::vector<TRmgMapPosition> candidates;
    TRmgMapPosition position;
    unsigned index;
    for (index = 0; index < m_objects.size(); ++index) {
        type_object* existing = m_objects[index];
        TObjectType* existingPrototype = existing->m_properties->m_prototype;
        TRmgMapPosition entrance = getRmgPlacedObjectEntrance(existing);
        int endDirection;
        int firstDirection;
        if (g_adventureObjectTraits[existingPrototype->getObjectType()].m_trait1) {
            endDirection = RMG_DIRECTION_COUNT;
            firstDirection = 0;
        } else {
            endDirection = 4;
            firstDirection = 1;
        }
        for (int direction = endDirection; direction-- > firstDirection; ) {
            position = entrance + g_rmgDirections[direction] + trigger;
            if (bounds.contains(position) && canFitObject(properties, position))
                candidates.push_back(position);
        }
    }
    unsigned count = candidates.size();
    if (!count)
        return false;
    index = rand() % count;
    position = candidates[index];
    addObject(object, position);
    return true;
}

VA(0x00535DF0, 0xEA)
MAC_ADDRESS(0x233e4c, 0x16c) // anchor-callee 0x5466c6/0x5355a4; thiscall, ret 0
void TRmgTreasureGroup::updateBounds()
{
    m_bounds.resetEmpty();
    TRmgMapItem* item = m_map.m_mapItems;
    for (int y = 0; y < m_map.m_mapHeight; ++y) {
        for (int x = 0; x < m_map.m_mapWidth; ++x, ++item) {
            if (!item->isClearOutlineCell()) {
                m_bounds.includeCell(x, y);
            }
        }
    }
}

// The cell predicates match the two retail expansions at 0x535f39 and
// 0x536000, including the byte-return entrance/clearance queries.
// Keep the x == width exhaustion test, including retail's zero-height behavior.
// The retained vector<TPoint>::insert matches all 582 bytes of the folded
// vector<type_artifact> body at 0x54d330, with identical new/delete calls.
// Direction-table references resolve to g_rmgDirections (0x69cdc0) and +4.
// The native perimeter uses the surface lookup overload.
VA(0x00535EE0, 0x18F)
MAC_ADDRESS(0x233fb8, 0x274) // anchor-callee 0x5468ea/0x53511b; thiscall, ret 0
void TRmgTreasureGroup::traceOutline()
{
    if (m_outline.size() > 0)
        return;
    TPoint position;
    position.m_x = 0;
    for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
        for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position.m_x, position.m_y);
            if (!item->isClearOutlineCell())
                break;
        }
        if (position.m_x < m_map.m_mapWidth)
            break;
    }
    if (position.m_x == m_map.m_mapWidth)
        return;
    --position.m_y;
    TPoint start = position;
    int direction = 2;
    do {
        m_outline.push_back(position);
        int attempts = 0;
        do {
            TPoint nearby = nextRmgOutlineProbe(position, direction);
            if (!m_map.containsXY(nearby))
                break;
            TRmgMapItem* item = m_map.getMapItem(nearby.m_x, nearby.m_y);
            if (item->isClearOutlineCell())
                break;
        } while (++attempts < 4);
        advanceRmgOutlineWalk(position, direction);
    } while (start != position);
}

VA(0x00536070, 0xFB)
MAC_ADDRESS(0x23422c, 0xec)
TRmgGeneratorBase::TRmgGeneratorBase(int width, int height, int levels,
    TProgressSink* progress, int additionalSteps, int version)
    : m_map(width, height, levels)
{
    m_progress = progress;
    m_mapVersion = version;
    if (progress)
        progress->setTotal(progress->m_steps + additionalSteps + 0x3bc4);
    time(&m_randomSeed);
    srand(m_randomSeed);
    loadObjectPrototypes();
}

VA_COMPGEN(0x00536170, 0x21, SCALAR_DELETING_DTOR, TRmgGeneratorBase)

VA(0x005361A0, 0x07)
MAC_ADDRESS(0x22d34c, 0x48)
TRmgMapInterface::~TRmgMapInterface()
{
}

VA_COMPGEN(0x005361B0, 0x23, SCALAR_DELETING_DTOR, TRmgMapInterface)

VA_COMPGEN(0x005361E0, 0x18, DEFAULT_CTOR_CLOSURE, TRmgObjectPropertiesRef)

// The loader inlines construction of each reference. Its owned outline
// vector is constructed before the scalar stores below; the 8x6 priority
// table is left uninitialized until the lazy builder sets its flag.
TRmgObjectPropertiesRef::TRmgObjectPropertiesRef(TObjectType* prototype)
{
    m_prototype = prototype;
    m_placementRule = 0;
    m_refCount = 0;
    m_prototypeIndex = 0;
    m_preferredTerrain = -1;
    m_prioritiesInitialized = false;
}

static inline bool isRmgObjectAvailableInVersion(int objectType, int version)
{
    if (version < RMG_MAP_SHADOW_OF_DEATH && objectType >= CLOVER_FIELD_2)
        return false;
    if (version < RMG_MAP_ARMAGEDDONS_BLADE && objectType >= MAX_EVENT_TYPE)
        return false;
    return true;
}

VA(0x00536200, 0x1AC)
MAC_ADDRESS(0x234440, 0x24c)
void TRmgGeneratorBase::loadObjectPrototypes()
{
    m_objectsTxt.load("objects.txt");
    unsigned int index = 0;
    for (; index < m_objectsTxt.m_objectTypes.size(); ++index) {
        int type = m_objectsTxt.m_objectTypes[index].getObjectType();
        if (!isRmgObjectAvailableInVersion(type, m_mapVersion))
            continue;
        if (m_mapVersion < RMG_MAP_SHADOW_OF_DEATH && (type == LITH_TWOWAY || type == LITH_ONEWAY_ENTRANCE || type == LITH_ONEWAY_EXIT)
            && m_objectsTxt.m_objectTypes[index].getSubtype() >= 3)
            continue;
        if (type < 0 || type >= 232)
            continue;
        TRmgObjectPropertiesRef* properties =
            new TRmgObjectPropertiesRef(&m_objectsTxt.m_objectTypes[index]);
        memcpy(&type, &g_adventureObjectTraits[type].m_nameRow, sizeof(type));
        m_objectPrototypes[type].push_back(properties);
    }
    // Exchange sort by subtype, swapping prototype pointers within the refs.
    // Keep its exact swaps and the retail nonempty-monster-list assumption;
    // replacing the unsigned size()-1 condition would repair retail behavior.
    for (index = 0; index < m_objectPrototypes[MONSTER].size() - 1; ++index) {
        for (unsigned int second = index + 1; second < m_objectPrototypes[MONSTER].size(); ++second) {
            if (m_objectPrototypes[MONSTER][index]->m_prototype->getSubtype() > m_objectPrototypes[MONSTER][second]->m_prototype->getSubtype()) {
                std::swap(m_objectPrototypes[MONSTER][index]->m_prototype, m_objectPrototypes[MONSTER][second]->m_prototype);
            }
        }
    }
    readObjectPlacementRules();
    if (m_progress)
        m_progress->advance(15300);
}

VA(0x005363B0, 0x1A9)
MAC_ADDRESS(0x2346a0, 0x18c)
TRmgGeneratorBase::~TRmgGeneratorBase()
{
    for (unsigned int object = 0; object < m_objects.size(); ++object)
        delete m_objects[object];
    for (int type = 0; type < 232; ++type)
        for (unsigned int prototype = 0; prototype < m_objectPrototypes[type].size(); ++prototype)
            delete m_objectPrototypes[type][prototype];
}

// Sixteen metadata columns precede scores. Resize before parsing and retain
// left-to-right reads and index + preceding-score-count + metadata order.
static inline void readRmgPlacementScores(std::vector<int>& scores,
    const TSpreadsheetResource::TStringVector& fields, int precedingScores, int count)
{
    scores.resize(count, 0);
    for (int index = 0; index < count; ++index)
        scores[index] = atoi(fields[index + precedingScores + 16]);
}

// The final lookup's temporary reverse iterator preserves the signed match
// sentinel and last-duplicate precedence. Construct it only on the guarded
// RHS: an empty group never forms a pointer offset.
// Mac 0x234998/0x2349a4 use separate object/terrain enum-vector families,
// distinct from the subtype integer/POD vector at 0x2348e4. The object list
// indexes the adventure-object traits/prototypes, establishing its domain.
VA(0x00536560, 0x5F2)
MAC_ADDRESS(0x23486c, 0x6a0) // anchor-string rand_trn.txt; thiscall, ret 0; retail-only
void TRmgGeneratorBase::readObjectPlacementRules()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x006827F4, rmgPlacementRulesFilename, "rand_trn.txt"));
    int row = 3;
    std::vector<TAdventureObjectType> objectTypes;
    std::vector<TTerrainType> terrains;
    std::vector<int> subtypes;
    TAdventureObjectType objectType;
    int subtype;
    TTerrainType terrain;
    for (; row < sheet->getNumberOfRows();) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        if (values[0][0] == ' ' || values[0][0] == 0)
            break;
        TRmgObjectPlacementRule rule;
        rule.m_index = row - 3;
        objectType = H3_ENUM_DECODE(TAdventureObjectType, atoi(values[3]));
        subtype = atoi(values[4]);
        terrain = H3_ENUM_DECODE(TTerrainType, atoi(values[6]));
        objectTypes.push_back(objectType);
        terrains.push_back(terrain);
        subtypes.push_back(subtype);
        for (terrain = eTerrainDirt; terrain <= eTerrainWater;
             terrain = H3_ENUM_DECODE(TTerrainType, terrain + 1))
            rule.m_terrainScores[terrain] = atoi(values[terrain + 7]);
        for (; terrain < 10; terrain = H3_ENUM_DECODE(TTerrainType, terrain + 1))
            rule.m_terrainScores[terrain] = RMG_PLACEMENT_INVALID;
        m_placementRules.push_back(rule);
        ++row;
    }
    int ruleCount = m_placementRules.size();
    for (row = 3; row < ruleCount + 3; ++row) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        TRmgObjectPlacementRule& rule = m_placementRules[row - 3];
        readRmgPlacementScores(rule.m_adjacentScores, values, 0, ruleCount);
        readRmgPlacementScores(rule.m_blockedScores, values, ruleCount, ruleCount);
    }
    sheet->dispose();

#if defined(HOMM3_TARGET_MAC)
    // CodeWarrior's 32K frame limit: Mac 0x23486c news both tables in one 0xd980 block.
    struct TPlacementTables {
        std::vector<TRmgObjectPlacementRule*> rulesByType[ADVENTURE_OBJECT_TRAIT_COUNT][10];
        std::vector<int> subtypesByType[ADVENTURE_OBJECT_TRAIT_COUNT][10];
    };
    TPlacementTables* tables = new TPlacementTables;
    std::vector<TRmgObjectPlacementRule*> (&rulesByType)[ADVENTURE_OBJECT_TRAIT_COUNT][10] =
        tables->rulesByType;
    std::vector<int> (&subtypesByType)[ADVENTURE_OBJECT_TRAIT_COUNT][10] =
        tables->subtypesByType;
#else
    std::vector<TRmgObjectPlacementRule*> rulesByType[ADVENTURE_OBJECT_TRAIT_COUNT][10];
    std::vector<int> subtypesByType[ADVENTURE_OBJECT_TRAIT_COUNT][10];
#endif
    for (int index = 0; index < ruleCount; ++index) {
        TRmgObjectPlacementRule* rule = &m_placementRules[index];
        rulesByType[objectTypes[index]][terrains[index]].push_back(rule);
        subtypesByType[objectTypes[index]][terrains[index]].push_back(subtypes[index]);
    }
    for (objectType = NOTHING; objectType < ADVENTURE_OBJECT_TRAIT_COUNT;
         objectType = H3_ENUM_DECODE(TAdventureObjectType, objectType + 1)) {
        for (int index = 0; index < m_objectPrototypes[objectType].size();
             ++index) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[objectType][index];
            TObjectType* prototype = properties->m_prototype;
            properties->m_placementRule = 0;
            for (terrain = eTerrainDirt; terrain < eTerrainRock;
                 terrain = H3_ENUM_DECODE(TTerrainType, terrain + 1)) {
                if (prototype->m_recommendedTerrainMask[terrain])
                    break;
            }
            properties->m_preferredTerrain = terrain;
            if (terrain != eTerrainRock) {
                subtype = prototype->getSubtype();
                int mappedType;
                // Same canonical byte table used by readObjectType: the
                // dword at +8 remaps aliases to their objnames.txt row.
                memcpy(&mappedType, &g_adventureObjectTraits[objectType].m_nameRow,
                       sizeof(mappedType));
                int match = rulesByType[mappedType][terrain].size();
                while (match-- && *std::vector<int>::reverse_iterator(
                    subtypesByType[mappedType][terrain].begin() + match + 1) != subtype)
                    ;
                if (match >= 0)
                    properties->m_placementRule = rulesByType[mappedType][terrain][match];
            }
        }
    }
#if defined(HOMM3_TARGET_MAC)
    delete tables;
#endif
}

// Caller 0x5375ff admits only positive scores. Retail retains bitset<48>::test
// at 0x536ca3/0x536cc1 and bitset<10>::test at 0x536d06. The insert callee's
// widget* name is an ICF alias of this pointer-vector instantiation.
VA(0x00536BC0, 0x5F4)
MAC_ADDRESS(0x23515c, 0x7a0) // anchor-callee 0x5375ff; thiscall, ret 0x10; retail-only
int TRmgGeneratorBase::scoreObjectPlacement(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position)
{
    TObjectType* prototype = properties->m_prototype;
    std::vector<type_object*> affected;
    b8 terrainSeen[10];
    memset(terrainSeen, 0, sizeof(terrainSeen));
    unsigned int marks[10][8];
    memset(marks, 0, sizeof(marks));
    for (unsigned int row = 0; row < prototype->getHeight(); ++row) {
        int y = position.m_y - row;
        if (y < 0 || y >= m_map.m_mapHeight)
            continue;
        for (unsigned int column = 0; column < prototype->getWidth(); ++column) {
            int x = position.m_x - column;
            if (x < 0 || x >= m_map.m_mapWidth)
                continue;
            if (!prototype->isDrawCell(column, row))
                continue;
            marks[column + 1][row + 1] |= RMG_PLACEMENT_OVERLAP;
            if (!prototype->isPassableCell(column, row)) {
                marks[column + 1][row + 1] |= RMG_PLACEMENT_BLOCKED;
                TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
                if (!prototype->m_terrainMask[item->getLandType()])
                    return RMG_PLACEMENT_INVALID;
                if (item->hasPathClearance())
                    return RMG_PLACEMENT_INVALID;

                // Retail 0x536d34 overwrites the complete mark with one
                // before marking the surrounding area; retain that store.
                marks[column + 1][row + 1] = RMG_PLACEMENT_ADJACENT;
                terrainSeen[item->getLandType()] = true;
                int firstRow = position.m_y - min(y + 1, m_map.m_mapHeight) + 1;
                int lastRow = position.m_y - max(y - 2, 0) + 1;
                int firstColumn = position.m_x - min(x + 1, m_map.m_mapWidth) + 1;
                int lastColumn = position.m_x - max(x - 2, 0) + 1;
                for (int nearColumn = firstColumn; nearColumn < lastColumn;
                     ++nearColumn) {
                    for (int nearRow = firstRow; nearRow < lastRow; ++nearRow)
                        marks[nearColumn][nearRow] |= RMG_PLACEMENT_ADJACENT;
                }
            }
        }
    }

    TRmgObjectPlacementRule* rule = properties->m_placementRule;
    int score = 0;
    b8 hasPositiveTerrain = false;
    for (int terrain = 0; terrain < 10; ++terrain) {
        if (terrainSeen[terrain]) {
            score += rule->m_terrainScores[terrain];
            if (rule->m_terrainScores[terrain] > 0)
                hasPositiveTerrain = true;
        }
    }
    if (score < RMG_PLACEMENT_MINIMUM_TERRAIN_SCORE)
        return score;
    if (!hasPositiveTerrain)
        return RMG_PLACEMENT_NO_TERRAIN_PREFERENCE;

    properties->buildOverlapPriorities();
    for (row = 0; row < prototype->getHeight() + 2; ++row) {
        int y = position.m_y + 1 - row;
        if (y < 0 || y >= m_map.m_mapHeight)
            continue;
        for (unsigned int column = 0; column < prototype->getWidth() + 2;
             ++column) {
            int x = position.m_x + 1 - column;
            if (x < 0 || x >= m_map.m_mapWidth)
                continue;
            unsigned int mark = marks[column][row];
            if (!mark)
                continue;
            TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
            if (item->isPassableLand())
                continue;
            int priority;
            // Only interior cells receive OVERLAP above (column+1, row+1),
            // so these subtractions cannot underflow for an overlap cell.
            if (mark & RMG_PLACEMENT_OVERLAP)
                priority = properties->m_overlapPriorities[column - 1][row - 1];
            for (int index = 0; index < static_cast<int>(item->m_objects.size());
                 ++index) {
                type_object* object = item->m_objects[index];
                b8 wasTouched = object->isPlacementTouched();
                if (mark & RMG_PLACEMENT_OVERLAP) {
                    object->m_properties->buildOverlapPriorities();
                    if (object->m_properties->m_overlapPriorities
                            [object->m_position.m_x - x][object->m_position.m_y - y]
                        <= priority)
                        object->m_candidateCovers = true;
                    else
                        object->m_candidateBehind = true;
                    object->m_overlapsCandidate = true;
                }
                if (mark & RMG_PLACEMENT_ADJACENT)
                    object->m_adjacentToCandidate = true;
                if (mark & RMG_PLACEMENT_BLOCKED)
                    object->m_blockedByCandidate = true;
                if (!wasTouched && object->isPlacementTouched())
                    affected.push_back(object);
                if (object->hasConflictingPlacementOrder())
                    break;
            }
        }
    }

    for (unsigned int index = 0; index < affected.size(); ++index) {
        type_object* object = affected[index];
        if (object->m_blockedByCandidate) {
            if (!object->m_properties->m_placementRule)
                score = RMG_PLACEMENT_INVALID;
            else
                score += rule->m_blockedScores[object->m_properties->m_placementRule->m_index];
        } else if (object->m_adjacentToCandidate) {
            if (object->m_properties->m_placementRule)
                score += rule->m_adjacentScores[object->m_properties->m_placementRule->m_index];
        }
        if (object->hasConflictingPlacementOrder())
            score = RMG_PLACEMENT_INVALID;
        object->clearPlacementMarks();
    }
    return score;
}

VA(0x005371C0, 0x1DA)
MAC_ADDRESS(0x2358fc, 0xb8)
void TRmgGeneratorBase::addObject(type_object* object, TRmgMapPosition position)
{
    m_map.addObject(*object, position);
    m_objects.push_back(object);
}

// Map-decoration caller 0x537a59 passes a position value and progress share.
// The body uses base fields and virtual object insertion; ownership/name provisional.
// Pinned retail 0x6408ec..0x64099f: decoration type ordinals, excluding
// terrain holes, rivers and roads. The final six Complete-only ids have
// no admitted semantic names. Role-derived table name.
DATA(0x006408EC)
static const int g_rmgDecorationTypes[45] = {
    TERRAIN_BRUSH, TERRAIN_BUSH, TERRAIN_CACTUS, TERRAIN_CANYON,
    TERRAIN_CRATER, TERRAIN_DEAD_VEGETATION, TERRAIN_FLOWER,
    TERRAIN_FROZEN_LAKE, TERRAIN_HEDGE, TERRAIN_HILL, TERRAIN_KELP,
    TERRAIN_LAKE, TERRAIN_LAVA_FLOW, TERRAIN_LAVA_LAKE, TERRAIN_MUSHROOM,
    TERRAIN_LOG, TERRAIN_MANDRAKE, TERRAIN_MOSS, TERRAIN_MOUND,
    TERRAIN_MOUNTAIN, TERRAIN_OAK_TREE, TERRAIN_OUTCROPPING,
    TERRAIN_PINE_TREE, TERRAIN_PLANT, TERRAIN_ROCK, TERRAIN_SAND_DUNE,
    TERRAIN_SAND_PIT, TERRAIN_SHRUB, TERRAIN_SKULL, TERRAIN_STALAGMITE,
    TERRAIN_STUMP, TERRAIN_TAR_PIT, TERRAIN_TREE, TERRAIN_VINE,
    TERRAIN_VOLCANIC_VENT, TERRAIN_VOLCANO, TERRAIN_WILLOW_TREE,
    TERRAIN_YUCCA_TREE, TERRAIN_REEF, 206, 207, 208, 209, 210, 211
};

// Retail retains the initial single insert, worklist erase and first
// by-value getMapItem call.
VA(0x005373A0, 0x53D)
MAC_ADDRESS(0x2359b4, 0x76c)
void TRmgGeneratorBase::decorateMapCell(TRmgMapPosition position, int progressSteps)
{
    std::vector<TRmgMapPosition> pending;
    pending.push_back(position);
    while (pending.size()) {
        position = pending.back();
        pending.pop_back();
        if (m_progress)
            m_progress->advance(progressSteps);
        TRmgMapItem* item = m_map.getMapItem(position);
        if (!item->isPassableLand())
            continue;
        int terrain = item->getLandType();
        std::vector<TRmgObjectPropertiesRef*> candidates;
        std::vector<TRmgMapPosition> positions;
        std::vector<int> weights;
        // Retail constructs a fourth vector at frame -0x84 and destroys it
        // before weights (-0x48), positions (-0x58), candidates (-0x38).
        // No element use survives; object-pointer element type/name provisional.
        std::vector<type_object*> unusedObjects;
        int totalWeight = 0;
        for (const int* type = g_rmgDecorationTypes;
            type < g_rmgDecorationTypes + sizeof(g_rmgDecorationTypes) / sizeof(g_rmgDecorationTypes[0]); ++type) {
            std::vector<TRmgObjectPropertiesRef*>& prototypes = m_objectPrototypes[*type];
            for (unsigned int index = 0; index < prototypes.size(); ++index) {
                TRmgObjectPropertiesRef* properties = prototypes[index];
                TObjectType* prototype = properties->m_prototype;
                if (!properties->m_placementRule
                    || properties->m_placementRule->m_terrainScores[terrain] <= RMG_PLACEMENT_INVALID)
                    continue;
                if (!isRmgObjectAvailableInVersion(prototype->getObjectType(), m_mapVersion))
                    continue;
                TRmgZoneBounds bounds;
                bounds.m_minimumX = position.m_x;
                bounds.m_minimumY = position.m_y;
                bounds.m_maximumX = position.m_x + prototype->getWidth();
                bounds.m_maximumY = position.m_y + prototype->getHeight();
                TRmgMapPosition candidatePosition;
                candidatePosition.m_z = position.m_z;
                for (candidatePosition.m_y = bounds.m_minimumY;
                    candidatePosition.m_y < bounds.m_maximumY; ++candidatePosition.m_y) {
                    for (candidatePosition.m_x = bounds.m_minimumX;
                        candidatePosition.m_x < bounds.m_maximumX; ++candidatePosition.m_x) {
                        if (prototype->isPassableCell(
                                candidatePosition.m_x - position.m_x, candidatePosition.m_y - position.m_y))
                            continue;
                        int score = scoreObjectPlacement(properties, candidatePosition);
                        if (score > 0) {
                            totalWeight += score;
                            candidates.push_back(properties);
                            positions.push_back(candidatePosition);
                            weights.push_back(score);
                        }
                    }
                }
            }
        }
        if (candidates.size()) {
            // Candidate traversal and subtraction order determine seed identity.
            int selected = rand() % totalWeight;
            unsigned int index;
            for (index = 0; index < candidates.size(); ++index) {
                selected -= weights[index];
                if (selected < 0)
                    break;
            }
            TRmgObjectPropertiesRef* properties = candidates[index];
            TRmgMapPosition candidatePosition = positions[index];
            TObjectType* prototype = properties->m_prototype;
            addObject(new type_object(properties), candidatePosition);
            TRmgZoneBounds bounds;
            bounds.m_minimumX = max(candidatePosition.m_x - prototype->getWidth(), 0);
            bounds.m_minimumY = max(candidatePosition.m_y - prototype->getHeight(), 0);
            bounds.m_maximumX = min(candidatePosition.m_x + 2, m_map.m_mapWidth);
            bounds.m_maximumY = min(candidatePosition.m_y + 2, m_map.m_mapHeight);
            candidatePosition.m_z = position.m_z;
            for (candidatePosition.m_y = bounds.m_minimumY;
                candidatePosition.m_y < bounds.m_maximumY; ++candidatePosition.m_y) {
                for (candidatePosition.m_x = bounds.m_minimumX;
                    candidatePosition.m_x < bounds.m_maximumX; ++candidatePosition.m_x) {
                    TRmgMapItem* nearby = m_map.getMapItem(candidatePosition);
                    if (nearby->hasBorderObject() && nearby->isPassableLand()) {
                        nearby->clearBorderObject();
                        pending.push_back(candidatePosition);
                    }
                }
            }
        }
    }
}

VA(0x005378E0, 0x27)
TRmgMapItem* type_random_map::getMapItem(TRmgMapPosition point)
{
    return &m_mapItems[(point.m_z * m_mapHeight + point.m_y) * m_mapWidth + point.m_x];
}

// Retail 0x549c91 calls this base-prefix pass after coastal marking.
VA(0x00537970, 0x199)
MAC_ADDRESS(0x236120, 0x2c4)
void TRmgGeneratorBase::decorateMap()
{
    TRmgMapPosition position;
    int count = 0;
    TRmgMapItem* item = m_map.m_mapItems;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z)
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y)
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item)
                if (item->hasBorderObject())
                    ++count;
    if (!count)
        return;
    int progressSteps = 276300 / count;
    item = m_map.m_mapItems;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (item->hasBorderObject()) {
                    if (!item->isPassableLand()) {
                        if (m_progress)
                            m_progress->advance(progressSteps);
                        continue;
                    }
                    decorateMapCell(position, progressSteps);
                }
            }
        }
    }
    item = m_map.m_mapItems;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (!item->hasPathClearance() && item->isPassableLand()) {
                    item->openPath();
                }
            }
        }
    }
}

// Reverse traversal makes an earlier record win if an object type repeats.
static inline void applyRmgObjectLimitOverrides(int* limits,
    const TRmgObjectLimit* overrides, int count)
{
    while (count--)
        limits[overrides[count].m_objectType] = overrides[count].m_limit;
}

// Hero eligibility uses bytes within THeroTraits' attributes word:
// 0x537d11/+0x3a excludes special heroes; +0x38/+0x39 selects map-version availability.
VA(0x00537B10, 0x2A8)
MAC_ADDRESS(0x2363e4, 0x2ec)
type_random_map_generator::type_random_map_generator(
    int width, int height, int levels, int humanPlayers, int humanTeams,
    int computerPlayers, int computerTeams, int waterContent,
    int monsterStrength, TProgressSink* progress, int mapVersion)
    : TRmgGeneratorBase(width, height, levels, progress,
        width * height + 326900, mapVersion)
{
    m_nextObjectId = 1;
    m_questArtifactPoolLow = false;
    m_waterContent = waterContent;
    m_monsterStrength = monsterStrength;
    m_humanPlayerCount = humanPlayers;
    m_humanTeamCount = humanTeams;
    m_computerPlayerCount = computerPlayers;
    m_computerTeamCount = computerTeams;
    if (m_waterContent == RMG_WATER_RANDOM)
        m_waterContent = rand() % 3;
    loadTemplates();
    if (m_templates.size()) {
        m_nextSeerHutPrototypeIndex = 0;
        memset(m_usedQuestArtifacts, 0, sizeof(m_usedQuestArtifacts));
        memset(m_disabledHeroes, 0, sizeof(m_disabledHeroes));
        memset(m_objectCountByType, 0, sizeof(m_objectCountByType));
        initializeObjectGenerators();
        for (int hero = 0; hero < 156; ++hero) {
            if (g_heroTraits[hero].m_availability.m_special)
                m_disabledHeroes[hero] = true;
            else if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
                if (!g_heroTraits[hero].m_availability.m_availableInExpansion)
                    m_disabledHeroes[hero] = true;
            } else if (!g_heroTraits[hero].m_availability.m_availableInOriginal)
                m_disabledHeroes[hero] = true;
        }
        for (int zoneObjectType = 0; zoneObjectType < 232; ++zoneObjectType)
            g_rmgZoneObjectLimits[zoneObjectType] = 32000;
        for (int mapObjectType = 0; mapObjectType < 232; ++mapObjectType)
            g_rmgMapObjectLimits[mapObjectType] = 32000;
        applyRmgObjectLimitOverrides(g_rmgMapObjectLimits, g_rmgMapObjectLimitOverrides, 30);
        applyRmgObjectLimitOverrides(g_rmgZoneObjectLimits, g_rmgZoneObjectLimitOverrides, 24);
        memset(m_fixedHumanPlayers, 0, sizeof(m_fixedHumanPlayers));
    }
}

VA_COMPGEN(0x00537DC0, 0x21, SCALAR_DELETING_DTOR, type_random_map_generator)

VA(0x00537DF0, 0x200)
MAC_ADDRESS(0x23685c, 0x1cc)
type_random_map_generator::~type_random_map_generator()
{
    for (unsigned int zone = 0; zone < m_zones.size(); ++zone)
        delete m_zones[zone];
    for (unsigned int mapTemplate = 0; mapTemplate < m_templates.size(); ++mapTemplate)
        delete m_templates[mapTemplate];
    for (unsigned int definition = 0; definition < m_objectGenerators.size(); ++definition)
        delete m_objectGenerators[definition];
}

template<class Record>
static inline void readRmgTemplatePlayerLimits(Record& record,
    const TSpreadsheetResource::TStringVector& fields, int firstField)
{
    record.m_minimumHumanPlayers = atoi(fields[firstField]);
    record.m_maximumHumanPlayers = atoi(fields[firstField + 1]);
    record.m_minimumPlayers = atoi(fields[firstField + 2]);
    record.m_maximumPlayers = atoi(fields[firstField + 3]);
}

// Keep the short-circuit order and the two signed total-count additions.
template<class Record>
static inline bool allowsRmgTemplatePlayerCounts(const Record& record,
    int humanPlayers, int computerPlayers)
{
    return record.m_minimumHumanPlayers <= humanPlayers
        && record.m_maximumHumanPlayers >= humanPlayers
        && record.m_minimumPlayers <= humanPlayers + computerPlayers
        && record.m_maximumPlayers >= humanPlayers + computerPlayers;
}

// The retail coordinator snapshots player counts separately before each pass.
static void readRmgTemplateConnections(const TSpreadsheetResource* sheet,
    TRmgTemplate* mapTemplate, int firstRow, int endRow,
    int humanPlayers, int computerPlayers)
{
    for (int connectionRow = firstRow; connectionRow < endRow; ++connectionRow) {
        const TSpreadsheetResource::TStringVector& fields = sheet->getRow(connectionRow);
        if (fields.size() > 84 && isRmgTemplateFieldSet(fields[76])
            && fields[77][0]) {
            int firstZone = atoi(fields[76]);
            int secondZone = atoi(fields[77]);
            TRmgTemplateZone* first = mapTemplate->findZone(firstZone);
            TRmgTemplateZone* second = mapTemplate->findZone(secondZone);
            if (first && second) {
                TRmgZoneConnection connection;
                connection.m_destination = second;
                connection.m_value = atoi(fields[78]);
                connection.m_unguarded = isRmgTemplateFieldSet(fields[79]);
                connection.m_placeBorderObjects = isRmgTemplateFieldSet(fields[80]);
                readRmgTemplatePlayerLimits(connection, fields, 81);
                connection.m_connected = false;
                if (allowsRmgTemplatePlayerCounts(connection, humanPlayers, computerPlayers)) {
                    first->m_connections.push_back(connection);
                    connection.m_destination = first;
                    second->m_connections.push_back(connection);
                }
            }
        }
    }
}

static bool hasRmgTemplatePlayerSlots(TRmgTemplate* mapTemplate,
    int humanPlayers, int computerPlayers)
{
    int playerSlots = 0;
    for (unsigned int slot = 0; slot < mapTemplate->m_zones.size(); ++slot)
        if (mapTemplate->m_zones[slot]->m_kind == RMG_TEMPLATE_HUMAN)
            ++playerSlots;
    if (playerSlots < humanPlayers)
        return false;
    for (slot = 0; slot < mapTemplate->m_zones.size(); ++slot)
        if (mapTemplate->m_zones[slot]->m_kind == RMG_TEMPLATE_COMPUTER)
            ++playerSlots;
    return playerSlots >= humanPlayers + computerPlayers;
}

// Template size units are 36*36 cells. Retail uses one rejection cleanup
// before the acceptance arm and retains separate zone/connection reader calls.
VA(0x00537FF0, 0x482)
MAC_ADDRESS(0x2372ec, 0x304)
void type_random_map_generator::loadTemplates()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00682804, rmgTemplatesFilename, "rmg.txt"));
    int mapSize = m_map.getWidth() * m_map.getHeight() * m_map.m_numberLevels / 1296;
    int row = 3;
    if (m_waterContent == RMG_WATER_ISLANDS)
        mapSize = max(mapSize / 2, 1);
    for (; row < sheet->getNumberOfRows();) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        // Retail's guard permits a two-field row even though it reads [2].
        // Preserve that malformed-template behavior rather than widening it.
        if (values.size() < 2) {
            ++row;
            continue;
        }
        TRmgTemplate* mapTemplate = new TRmgTemplate;
        mapTemplate->m_minimumSize = atoi(values[1]);
        mapTemplate->m_maximumSize = atoi(values[2]);
        mapTemplate->m_name = values[0];
        int endRow = row + 1;
        while (endRow < sheet->getNumberOfRows()
            && !isRmgTemplateFieldSet(sheet->getRow(endRow)[0]))
            ++endRow;
        bool accepted = mapSize >= mapTemplate->m_minimumSize
            && mapSize <= mapTemplate->m_maximumSize;
        if (accepted) {
            readRmgTemplateZones(sheet, mapTemplate, row, endRow,
                m_humanPlayerCount, m_computerPlayerCount, m_mapVersion);
            readRmgTemplateConnections(sheet, mapTemplate, row, endRow,
                m_humanPlayerCount, m_computerPlayerCount);
            accepted = hasRmgTemplatePlayerSlots(mapTemplate,
                m_humanPlayerCount, m_computerPlayerCount);
        }
        if (!accepted) {
            delete mapTemplate;
        } else {
            for (unsigned int zone = 0; zone < mapTemplate->m_zones.size(); ++zone)
                mapTemplate->m_zones[zone]->m_zoneIndex = zone;
            m_templates.push_back(mapTemplate);
        }
        row = endRow;
    }
    sheet->dispose();
}

VA(0x00538480, 0x687)
MAC_ADDRESS(0x236a28, 0x694)
void readRmgTemplateZones(
    const TSpreadsheetResource* sheet, TRmgTemplate* mapTemplate,
    int firstRow, int endRow, int humanPlayers, int computerPlayers,
    int mapVersion)
{
    for (int row = firstRow; row < endRow; ++row) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        // RETAIL BUG: a three-field row reads values[3] before the full
        // row-length check. Preserve this evaluation order.
        if (values.size() >= 3 && isRmgTemplateFieldSet(values[3]) &&
            values.size() > 75) {

            TRmgTemplateZone* slot = new TRmgTemplateZone;
            slot->m_zoneIndex = atoi(values[3]);
            slot->m_kind = RMG_TEMPLATE_TREASURE;
            if (isRmgTemplateFieldSet(values[4]))
                slot->m_kind = RMG_TEMPLATE_HUMAN;
            if (isRmgTemplateFieldSet(values[5]))
                slot->m_kind = RMG_TEMPLATE_COMPUTER;
            if (isRmgTemplateFieldSet(values[6]))
                slot->m_kind = RMG_TEMPLATE_TREASURE;
            if (isRmgTemplateFieldSet(values[7]))
                slot->m_kind = RMG_TEMPLATE_JUNCTION;
            slot->m_size = atoi(values[8]);
            readRmgTemplatePlayerLimits(*slot, values, 9);
            if (!allowsRmgTemplatePlayerCounts(*slot, humanPlayers, computerPlayers)) {
                delete slot;
            } else {
                slot->m_playerIndex = atoi(values[13]) - 1;
                slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT] = atoi(values[14]);
                slot->m_townPlacement[RMG_TOWN_PLAYER_OPTION_COUNT] = atoi(values[15]);
                slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_DENSITY] = atoi(values[16]);
                slot->m_townPlacement[RMG_TOWN_PLAYER_OPTION_DENSITY] = atoi(values[17]);
                slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT] = atoi(values[18]);
                slot->m_townPlacement[RMG_TOWN_NEUTRAL_OPTION_COUNT] = atoi(values[19]);
                slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_DENSITY] = atoi(values[20]);
                slot->m_townPlacement[RMG_TOWN_NEUTRAL_OPTION_DENSITY] = atoi(values[21]);
                slot->m_neutralTownsMatchZone = false;
                if (isRmgTemplateFieldSet(values[22]))
                    slot->m_neutralTownsMatchZone = true;
                int townCount;
                if (mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE)
                    townCount = 9;
                else {
                    townCount = 8;
                    slot->m_allowedTowns[8] = false;
                }
                while (townCount--) {
                    if (isRmgTemplateFieldSet(values[23 + townCount]))
                        slot->m_allowedTowns[townCount] = true;
                    else
                        slot->m_allowedTowns[townCount] = false;
                }
                for (int mine = 0; mine < 7; ++mine)
                    slot->m_mineCounts[mine] = atoi(values[32 + mine]);
                for (int resource = 0; resource < 7; ++resource)
                    slot->m_mineDensities[resource] = atoi(values[39 + resource]);
                slot->m_useNativeTerrain = isRmgTemplateFieldSet(values[46]);
                b8 anyTerrain = false;
                for (int terrain = 0; terrain < 8; ++terrain) {
                    slot->m_allowedTerrain[terrain] =
                        isRmgTemplateFieldSet(values[47 + terrain]);
                    if (slot->m_allowedTerrain[terrain])
                        anyTerrain = true;
                }
                if (!anyTerrain)
                    slot->m_allowedTerrain[0] = true;
                switch (tolower(values[55][0])) {
                case 'n': slot->m_monsterStrength = 0; break;
                case 'w': slot->m_monsterStrength = 2; break;
                case 's': slot->m_monsterStrength = 4; break;
                case 'a': slot->m_monsterStrength = 3; break;
                default: slot->m_monsterStrength = 3; break;
                }
                slot->m_guardsMatchZone = isRmgTemplateFieldSet(values[56]);
                for (int monster = 0; monster < 10; ++monster)
                    slot->m_allowedMonsters[monster] =
                        isRmgTemplateFieldSet(values[57 + monster]);
                if (mapVersion < RMG_MAP_ARMAGEDDONS_BLADE)
                    slot->m_allowedMonsters[8] = false;
                for (int treasure = 0; treasure < 3; ++treasure) {
                    slot->m_treasure[treasure].m_minimum = atoi(values[67 + 3 * treasure]);
                    slot->m_treasure[treasure].m_maximum = atoi(values[68 + 3 * treasure]);
                    slot->m_treasure[treasure].m_density = atoi(values[69 + 3 * treasure]);
                }
                mapTemplate->m_zones.push_back(slot);
            }
        }
    }
}

// Retail registers each generator through push_back; the differing retained
// vector overloads are compiler expansions of that operation.
VA(0x00538B10, 0x2241)
MAC_ADDRESS(0x2375f0, 0x4880)
void type_random_map_generator::initializeObjectGenerators()
{
    m_objectGenerators.push_back(new type_treasure_def(ALTAR_OF_SACRIFICE, 0, 100, 20));
    m_objectGenerators.push_back(new type_treasure_def(ARENA, 0, 3000, 50));

    {
        int creatureCount = m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE ? 145 : 118;
        for (int creature = creatureCount; creature--;) {
            if (g_creatureTypeTraits[creature].m_level >= 0)
                m_objectGenerators.push_back(
                    new type_black_box_creature_def(creature));
        }
    }

    m_objectGenerators.push_back(
        new type_black_box_experience_def(6000, 5000));
    m_objectGenerators.push_back(
        new type_black_box_experience_def(12000, 10000));
    m_objectGenerators.push_back(
        new type_black_box_experience_def(18000, 15000));
    m_objectGenerators.push_back(
        new type_black_box_experience_def(24000, 20000));

    m_objectGenerators.push_back(new type_black_box_gold_def(5000, 5000));
    m_objectGenerators.push_back(new type_black_box_gold_def(10000, 10000));
    m_objectGenerators.push_back(new type_black_box_gold_def(15000, 15000));
    m_objectGenerators.push_back(new type_black_box_gold_def(20000, 20000));

    m_objectGenerators.push_back(new type_black_box_spells_def(5000, 1, 1, 15));
    m_objectGenerators.push_back(new type_black_box_spells_def(7500, 2, 2, 15));
    m_objectGenerators.push_back(new type_black_box_spells_def(10000, 3, 3, 15));
    m_objectGenerators.push_back(new type_black_box_spells_def(12500, 4, 4, 15));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 5, 5, 15));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 1));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 2));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 4));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, 8));
    m_objectGenerators.push_back(new type_black_box_spells_def(30000, 1, 5, 15));

    {
        int tentIndex = m_objectPrototypes[BORDER_TENT].size();
        m_disabledKeyTents.resize(tentIndex);
        for (; tentIndex--;) {
            m_disabledKeyTents[tentIndex] = false;
            m_objectGenerators.push_back(new type_key_tent_def(tentIndex, 5000));
            m_objectGenerators.push_back(new type_key_tent_def(tentIndex, 7500));
            m_objectGenerators.push_back(new type_key_tent_def(tentIndex, 10000));
            m_objectGenerators.push_back(new type_key_tent_def(tentIndex, 15000));
            m_objectGenerators.push_back(new type_key_tent_def(tentIndex, 20000));
        }
    }

    m_objectGenerators.push_back(new type_treasure_def(BLACK_MARKET, 0, 8000, 20));
    m_objectGenerators.push_back(new type_treasure_def(BUOY, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(CAMPFIRE, 0, 2000, 500));
    m_objectGenerators.push_back(new type_treasure_def(CARTOGRAPHER, 0, 5000, 20));
    m_objectGenerators.push_back(new type_treasure_def(CARTOGRAPHER, 1, 10000, 20));
    m_objectGenerators.push_back(new type_treasure_def(CARTOGRAPHER, 2, 7500, 20));
    m_objectGenerators.push_back(new type_treasure_def(CLOVER_FIELD, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, 0, 3000, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, 1, 2000, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, 2, 2000, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, 3, 5000, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, 4, 1500, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, 5, 3000, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, 6, 9000, 100));

    int dwelling = 80;
    if (m_mapVersion < RMG_MAP_ARMAGEDDONS_BLADE)
        dwelling = 58;
    for (; dwelling--;)
        m_objectGenerators.push_back(new type_map_dwelling_def(dwelling));

    m_objectGenerators.push_back(new type_treasure_def(DEAD_GUY, 0, 500, 100));
    m_objectGenerators.push_back(new type_treasure_def(DEFENSE_TOWER, 0, 1500, 100));
    m_objectGenerators.push_back(new type_treasure_def(DERELICT_SHIP, 0, 4000, 20));
    m_objectGenerators.push_back(new type_treasure_def(DRAGON_CITY, 0, 10000, 100));
    m_objectGenerators.push_back(new type_treasure_def(FAERIE_RING, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(FLOTSAM, 0, 500, 1000));
    m_objectGenerators.push_back(new type_treasure_def(FOUNTAIN_OF_FORTUNE, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(FOUNTAIN_OF_YOUTH, 0, 100, 50));
    m_objectGenerators.push_back(new type_treasure_def(GARDEN_OF_REVELATION, 0, 1500, 100));
    m_objectGenerators.push_back(new type_treasure_def(HILL_FORT, 0, 7000, 20));
    m_objectGenerators.push_back(new type_treasure_def(IDOL_OF_FORTUNE, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(LEAN_TO, 0, 500, 100));
    m_objectGenerators.push_back(new type_treasure_def(LIBRARY, 0, 12000, 20));
    m_objectGenerators.push_back(new type_treasure_def(MAGIC_SCHOOL, 0, 1000, 50));
    m_objectGenerators.push_back(new type_treasure_def(MAGIC_SPRING, 0, 500, 50));
    m_objectGenerators.push_back(new type_treasure_def(MAGIC_WELL, 0, 250, 100));
    m_objectGenerators.push_back(new type_treasure_def(MERC_CAMP, 0, 1500, 100));
    m_objectGenerators.push_back(new type_treasure_def(MERMAID, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(MYSTICAL_GARDEN, 0, 500, 50));
    m_objectGenerators.push_back(new type_treasure_def(OASIS, 0, 100, 50));
    m_objectGenerators.push_back(new type_treasure_def(OBELISK, 0, 3500, 200));
    m_objectGenerators.push_back(new type_treasure_def(OBSERVATORY, 0, 750, 100));
    m_objectGenerators.push_back(new type_treasure_def(PILLAR_OF_FIRE, 0, 750, 100));
    m_objectGenerators.push_back(new type_treasure_def(POWER_SCHOOL, 0, 1500, 100));

    m_objectGenerators.push_back(new type_prison_def(2500, 0));
    m_objectGenerators.push_back(new type_prison_def(5000, 5000));
    m_objectGenerators.push_back(new type_prison_def(10000, 15000));
    m_objectGenerators.push_back(new type_prison_def(20000, 90000));
    m_objectGenerators.push_back(new type_prison_def(30000, 500000));
    m_objectGenerators.push_back(new type_treasure_def(PYRAMID, 0, 5000, 20));
    m_objectGenerators.push_back(new type_treasure_def(RALLY_FLAG, 0, 100, 100));

    m_objectGenerators.push_back(new type_artifact_def(RANDOM_ARTIFACT_1, 2000));
    m_objectGenerators.push_back(new type_artifact_def(RANDOM_ARTIFACT_2, 5000));
    m_objectGenerators.push_back(new type_artifact_def(RANDOM_ARTIFACT_3, 10000));
    m_objectGenerators.push_back(new type_artifact_def(RANDOM_ARTIFACT_4, 20000));

    m_objectGenerators.push_back(new type_resource_lump_def(RANDOM_RESOURCE, 0, 1500, 2000));
    m_objectGenerators.push_back(new type_treasure_def(REFUGEE_CAMP, 0, 5000, 20));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, 0, 1400, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, 2, 1400, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, 1, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, 3, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, 4, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, 5, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, 6, 750, 300));
    m_objectGenerators.push_back(new type_treasure_def(SANCTUARY, 0, 100, 50));
    m_objectGenerators.push_back(new type_scholar_def());
    m_objectGenerators.push_back(new type_treasure_def(SEA_CHEST, 0, 1500, 500));

    for (int quest = 0; quest < m_objectPrototypes[SEER].size(); ++quest) {
        int creatureCount = m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE ? 145 : 118;
        for (int creature = creatureCount; creature--;) {
            if (g_creatureTypeTraits[creature].m_level >= 0)
                m_objectGenerators.push_back(
                    new type_quest_creature_def(creature, quest));
        }

        m_objectGenerators.push_back(
            new type_quest_experience_def(quest, 2000, 5000));
        m_objectGenerators.push_back(
            new type_quest_experience_def(quest, 5333, 10000));
        m_objectGenerators.push_back(
            new type_quest_experience_def(quest, 8666, 15000));
        m_objectGenerators.push_back(
            new type_quest_experience_def(quest, 12000, 20000));
        m_objectGenerators.push_back(new type_quest_gold_def(quest, 2000, 5000));
        m_objectGenerators.push_back(new type_quest_gold_def(quest, 5333, 10000));
        m_objectGenerators.push_back(new type_quest_gold_def(quest, 8666, 15000));
        m_objectGenerators.push_back(new type_quest_gold_def(quest, 12000, 20000));
    }

    m_objectGenerators.push_back(new type_treasure_def(SEPULCHER, 0, 1000, 100));
    m_objectGenerators.push_back(new type_treasure_def(SHIPWRECK, 0, 2000, 100));
    m_objectGenerators.push_back(new type_treasure_def(SHIPWRECK_SURVIVOR, 0, 1500, 50));
    m_objectGenerators.push_back(new type_shrine_def(SHRINE1, 500));
    m_objectGenerators.push_back(new type_shrine_def(SHRINE2, 2000));
    m_objectGenerators.push_back(new type_shrine_def(SHRINE3, 3000));
    m_objectGenerators.push_back(new type_treasure_def(SIREN, 0, 100, 20));
    m_objectGenerators.push_back(new type_spell_scroll_def(1, 500));
    m_objectGenerators.push_back(new type_spell_scroll_def(2, 2000));
    m_objectGenerators.push_back(new type_spell_scroll_def(3, 3000));
    m_objectGenerators.push_back(new type_spell_scroll_def(4, 4000));
    m_objectGenerators.push_back(new type_spell_scroll_def(5, 5000));
    m_objectGenerators.push_back(new type_treasure_def(STABLES, 0, 200, 40));
    m_objectGenerators.push_back(new type_treasure_def(TAVERN, 0, 100, 20));
    m_objectGenerators.push_back(new type_treasure_def(TEMPLE, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(THIEVES_DEN, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(TRADING_POST, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(TRAINING_GROUNDS, 0, 1500, 200));
    m_objectGenerators.push_back(new type_treasure_def(TREASURE_CHEST, 0, 1500, 1000));
    m_objectGenerators.push_back(new type_treasure_def(TREE_OF_KNOWLEDGE, 0, 2500, 50));
    m_objectGenerators.push_back(new type_treasure_def(UNIVERSITY, 0, 2500, 20));
    m_objectGenerators.push_back(new type_treasure_def(WAGON, 0, 500, 50));
    m_objectGenerators.push_back(new type_treasure_def(WAR_MACHINE_FACTORY, 0, 1500, 50));
    m_objectGenerators.push_back(new type_treasure_def(WAR_SCHOOL, 0, 1000, 50));
    m_objectGenerators.push_back(new type_treasure_def(WARRIOR_TOMB, 0, 6000, 20));
    m_objectGenerators.push_back(new type_treasure_def(WATER_WHEEL, 0, 750, 50));
    m_objectGenerators.push_back(new type_treasure_def(WATERING_HOLE, 0, 500, 50));
    m_objectGenerators.push_back(new type_treasure_def(WINDMILL, 0, 2500, 150));
    m_objectGenerators.push_back(new type_witch_hut_def());
}

// Mac 0x23bf6c retains getRmgDistance on the XY bases of owned position copies,
// as does TRmgZone::canConnect.
VA(0x0053AD60, 0x113)
MAC_ADDRESS(0x23be70, 0x14c) // anchor-callee 0x53e2ea/0x53af04; thiscall, ret 4
b8 type_random_map_generator::canPlaceZone(TRmgZone* zone)
{
    TRmgTemplateZone* slot = zone->m_templateZone;
    TRmgMapPosition position = zone->getLevelPosition();
    int size = slot->m_size;
    if ((slot->m_kind == RMG_TEMPLATE_HUMAN ||
         slot->m_kind == RMG_TEMPLATE_COMPUTER) &&
        position.m_z == 1 && zone->m_alignment != TOWN_INFERNO &&
        zone->m_alignment != TOWN_NECROPOLIS && zone->m_alignment != TOWN_DUNGEON)
        return false;
    int zoneIndex = slot->m_zoneIndex;
    for (int other = 0; other < m_zones.size(); ++other) {
        TRmgZone* otherZone = m_zones[other];
        if (otherZone->getLevelPosition().m_z != position.m_z ||
            otherZone->m_templateZone->m_zoneIndex == zoneIndex)
            continue;
        TRmgMapPosition otherPosition = otherZone->getLevelPosition();
        int distance = getRmgDistance(otherPosition, position);
        if (10 * distance < 8 * (otherZone->m_templateZone->m_size + size))
            return false;
    }
    return true;
}

// Sample one of the 32 radial zone candidates. Keep the Y conversion before
// the X expression and truncate only after adding the scaled direction.
static inline TRmgMapPosition getRmgRadialZonePosition(
    const TRmgMapPosition& center, int radius, int direction, int level)
{
    const int& y = static_cast<int>(center.m_y + radius * g_rmgDirectionSines[direction]);
    return TRmgMapPosition(
        static_cast<int>(center.m_x + radius * g_rmgDirectionCosines[direction]),
        y, level);
}

// Trial placement leaves the zone at the candidate even when rejected.
// Snapshot accepted positions after the predicate, which can observe or
// alter the zone; do not append the incoming coordinate directly.
static inline void appendRmgZoneCandidate(type_random_map_generator* generator,
    TRmgZone* zone, const TRmgMapPosition& candidate,
    std::vector<TRmgMapPosition>& candidates)
{
    zone->setLevelPosition(candidate);
    if (generator->canPlaceZone(zone))
        candidates.push_back(zone->getLevelPosition());
}

VA(0x0053AE80, 0x36A)
MAC_ADDRESS(0x23bfbc, 0x28c) // anchor-callee 0x53bab9/0x53bb23; thiscall, ret 0xc
void type_random_map_generator::appendZonePositions(TRmgZone* center,
    TRmgZone* zone, std::vector<TRmgMapPosition>& candidates)
{
    int radius = center->m_templateZone->m_size + zone->m_templateZone->m_size;
    TRmgMapPosition position = center->getLevelPosition();
    TRmgMapPosition candidate;
    for (int direction = 0; direction < 32; ++direction) {
        candidate = getRmgRadialZonePosition(position, radius, direction, position.m_z);
        appendRmgZoneCandidate(this, zone, candidate, candidates);
    }
    if (m_map.m_numberLevels == 1)
        return;
    int level = 1 - position.m_z;
    candidate = TRmgMapPosition(position.m_x, position.m_y, level);
    appendRmgZoneCandidate(this, zone, candidate, candidates);
    radius = center->m_templateZone->m_size;
    if (radius < zone->m_templateZone->m_size)
        radius = zone->m_templateZone->m_size;
    for (direction = 0; direction < 32; ++direction) {
        candidate = getRmgRadialZonePosition(position, radius, direction, level);
        appendRmgZoneCandidate(this, zone, candidate, candidates);
    }
}

// Both filtering passes retain the same size and canConnect calls.
MAC_ADDRESS(0x23c248, 0x9c)
int type_random_map_generator::countPlacedZoneConnections(TRmgZone* zone) const
{
    int connectionCount = 0;
    TRmgTemplateZone* slot = zone->m_templateZone;
    for (int connection = 0; connection < slot->m_connections.size(); ++connection) {
        int destination = slot->m_connections[connection].m_destination->m_zoneIndex;
        if (destination < m_zones.size() && m_zones[destination]->canConnect(zone))
            ++connectionCount;
    }
    return connectionCount;
}

// Grow a half-open bounding rectangle around a zone's nominal radius.
// Keep Y-before-X updates and the inclusive far cell's +1 adjustment.
static inline void includeRmgZoneFootprint(int& minimumY, int& minimumX,
    int& maximumY, int& maximumX, const TRmgMapPosition& position, int size)
{
    minimumY = min(minimumY, position.m_y - size);
    minimumX = min(minimumX, position.m_x - size);
    maximumY = max(maximumY, position.m_y + size + 1);
    maximumX = max(maximumX, position.m_x + size + 1);
}

VA(0x0053B1F0, 0xFE)
MAC_ADDRESS(0x23c2e4, 0x170)
void type_random_map_generator::getInitialZoneBounds(int& minimumY, int& minimumX,
    int& maximumY, int& maximumX) const
{
    minimumY = 0;
    minimumX = 0;
    maximumY = 0;
    maximumX = 0;
    for (int zone = 0; zone < m_zones.size(); ++zone) {
        TRmgMapPosition position;
        position = m_zones[zone]->getLevelPosition();
        int size = m_zones[zone]->m_templateZone->m_size;
        includeRmgZoneFootprint(minimumY, minimumX, maximumY, maximumX, position, size);
    }
}

// Square-map extent needed to enclose a candidate zone and the bounds of
// all other zones. Both ranking and rejection use this same integer measure.
// Keep the Y-before-X calculations and minimum requested map size.
static inline int getRmgCandidateMapSize(const TRmgZoneBounds& bounds,
    const TRmgMapPosition& position, int zoneSize, int mapSize)
{
    TRmgZoneBounds candidate = bounds;
    includeRmgZoneFootprint(candidate.m_minimumY, candidate.m_minimumX,
        candidate.m_maximumY, candidate.m_maximumX, position, zoneSize);
    int candidateSize = max(mapSize, candidate.m_maximumY - candidate.m_minimumY);
    candidateSize = max(candidateSize, candidate.m_maximumX - candidate.m_minimumX);
    return candidateSize;
}

// Prefer unused levels, then maximum connections, then the smallest enclosing
// square. Keep owned position snapshots and the by-value setter's load-before-
// store boundary.
VA(0x0053B2F0, 0x678)
MAC_ADDRESS(0x23c454, 0x62c) // anchor-callee 0x53bb38; thiscall, ret 0xc
void type_random_map_generator::filterZonePositions(
    TRmgZone* zone, std::vector<TRmgMapPosition>& candidates, int mapSize)
{
    int bestConnections = 0;
    if (m_map.m_numberLevels > 1) {
        b8 occupiedLevels[2] = {false, false};
        for (int other = 0; other < m_zones.size(); ++other) {
            if (m_zones[other] != zone)
                occupiedLevels[m_zones[other]->getLevelPosition().m_z] = true;
        }
        if (!occupiedLevels[0] || !occupiedLevels[1]) {
            int candidate = candidates.size();
            while (candidate--) {
                if (!occupiedLevels[candidates[candidate].m_z])
                    break;
            }
            // RETAIL BUG: an unused-level candidate at index zero alone
            // does not trigger filtering. Preserve the strict > 0 test.
            if (candidate > 0) {
                candidate = candidates.size();
                while (candidate--) {
                    if (occupiedLevels[candidates[candidate].m_z])
                        candidates.erase(candidates.begin() + candidate);
                }
            }
        }
    }

    for (int candidate = 0; candidate < candidates.size(); ++candidate) {
        zone->setLevelPosition(candidates[candidate]);
        int connections = countPlacedZoneConnections(zone);
        if (connections > bestConnections)
            bestConnections = connections;
    }
    for (candidate = candidates.size() - 1; candidate >= 0; --candidate) {
        zone->setLevelPosition(candidates[candidate]);
        if (countPlacedZoneConnections(zone) < bestConnections)
            candidates.erase(candidates.begin() + candidate);
    }

    int bestSize = 32000;
    TRmgZoneBounds bounds;
    bounds.m_minimumY = 0;
    bounds.m_minimumX = 0;
    bounds.m_maximumY = 0;
    bounds.m_maximumX = 0;
    for (int other = 0; other < m_zones.size(); ++other) {
        if (m_zones[other] != zone) {
            TRmgMapPosition position;
            position = m_zones[other]->getLevelPosition();
            int size = m_zones[other]->getSize();
            includeRmgZoneFootprint(bounds.m_minimumY, bounds.m_minimumX,
                bounds.m_maximumY, bounds.m_maximumX, position, size);
        }
    }
    int size = zone->getSize();
    for (candidate = 0; candidate < candidates.size(); ++candidate) {
        int candidateSize = getRmgCandidateMapSize(
            bounds, candidates[candidate], size, mapSize);
        bestSize = min(bestSize, candidateSize);
    }
    for (candidate = candidates.size() - 1; candidate >= 0; --candidate) {
        int candidateSize = getRmgCandidateMapSize(
            bounds, candidates[candidate], size, mapSize);
        if (bestSize < candidateSize)
            candidates.erase(candidates.begin() + candidate);
    }
}

// The first zone may start on either eligible level. Later zones sample
// template neighbours, falling back to all placed zones before filtering.
// Preserve the owned snapshot of the selected coordinate.
VA(0x0053B970, 0x232)
MAC_ADDRESS(0x23ca80, 0x20c) // anchor-callee 0x53bde2/0x53be39; thiscall, ret 8
void type_random_map_generator::positionZone(TRmgZone* zone, int mapSize)
{
    std::vector<TRmgMapPosition> candidates;
    if (m_zones.size() == 0) {
        zone->m_levelPosition.m_y = 0;
        zone->m_levelPosition.m_z = 0;
        zone->m_levelPosition.m_x = 0;
        candidates.push_back(zone->getLevelPosition());
        if (m_map.m_numberLevels > 1)
            appendRmgZoneCandidate(this, zone, TRmgMapPosition(0, 0, 1), candidates);
    } else {
        TRmgTemplateZone* slot = zone->m_templateZone;
        for (int connection = 0; connection < slot->m_connections.size(); ++connection) {
            TRmgTemplateZone* destination = slot->m_connections[connection].m_destination;
            if (destination->m_zoneIndex < m_zones.size())
                appendZonePositions(m_zones[destination->m_zoneIndex], zone, candidates);
        }
        if (candidates.size() == 0) {
            for (int other = 0; other < m_zones.size(); ++other)
                appendZonePositions(m_zones[other], zone, candidates);
        }
        filterZonePositions(zone, candidates, mapSize);
    }
    // Retail assumes a legal candidate exists; an empty set reaches rand()%0.
    // Do not add a fallback here: it would change placement and RNG behavior.
    unsigned int count = candidates.size();
    unsigned int selected = rand() % count;
    TRmgMapPosition selectedPosition;
    selectedPosition = candidates[selected];
    zone->setLevelPosition(selectedPosition);
}

VA(0x0053BBB0, 0xFD)
MAC_ADDRESS(0x23cc8c, 0xf8)
void type_random_map_generator::calculateZoneBounds()
{
    TRmgMapItem* item = m_map.m_mapItems;
    TRmgMapPosition position;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (item->m_zoneState.m_zone >= 0) {
                    TRmgZone* zone = m_zones[item->m_zoneState.m_zone];
                    zone->m_bounds.includeCell(position.m_x, position.m_y);
                }
            }
        }
    }
}

// The two placement passes precede normalization to a centered square.
// Preserve the town table's sentinel/version traversal.
VA(0x0053BCB0, 0x33B)
MAC_ADDRESS(0x23cd84, 0x458)
void type_random_map_generator::initializeZones(TRmgTemplate* mapTemplate)
{
    m_zones.clear();
    int minimumSize = 32000;
    for (int slotIndex = 0; slotIndex < mapTemplate->m_zones.size(); ++slotIndex)
        minimumSize = min(minimumSize, mapTemplate->m_zones[slotIndex]->m_size);
    int mapSize = min(minimumSize * m_map.m_mapWidth,
        minimumSize * m_map.m_mapHeight);
    switch (m_waterContent) {
    case RMG_WATER_NONE: mapSize /= 5; break;
    case RMG_WATER_NORMAL: mapSize /= 6; break;
    default: mapSize /= 7; break;
    }
    for (slotIndex = 0; slotIndex < mapTemplate->m_zones.size(); ++slotIndex) {
        TRmgTemplateZone* slot = mapTemplate->m_zones[slotIndex];
        TRmgZone* zone = new TRmgZone(slot);
        if (slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT] + slot->m_townPlacement[RMG_TOWN_PLAYER_OPTION_COUNT] > 0
            && slot->m_playerIndex >= 0
            && m_playerIndexMap[slot->m_playerIndex + 1] >= 0
            && m_townChoices[m_playerIndexMap[slot->m_playerIndex + 1]] != -1)
            zone->m_alignment = m_townChoices[m_playerIndexMap[slot->m_playerIndex + 1]];
        positionZone(zone, mapSize);
        m_zones.push_back(zone);
    }
    for (int pass = 0; pass < 2; ++pass) {
        for (slotIndex = 0; slotIndex < mapTemplate->m_zones.size(); ++slotIndex)
            positionZone(m_zones[slotIndex], mapSize);
    }
    int minimumY, minimumX, maximumY, maximumX;
    getInitialZoneBounds(minimumY, minimumX, maximumY, maximumX);
    int span = max(maximumY - minimumY, maximumX - minimumX);
    int size = max(m_map.m_mapWidth, m_map.m_mapHeight);
    minimumY = (minimumY - span + maximumY) / 2;
    minimumX = (minimumX - span + maximumX) / 2;
    for (int zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgMapPosition position = m_zones[zoneIndex]->getLevelPosition();
        position.m_x = (position.m_x - minimumX) * size / span;
        position.m_y = (position.m_y - minimumY) * size / span;
        m_zones[zoneIndex]->setLevelPosition(position);
        m_zones[zoneIndex]->m_boundaryRoughness = m_zones[zoneIndex]->m_templateZone->m_size * size / span;
        m_zones[zoneIndex]->chooseTerrain();
        m_zones[zoneIndex]->chooseTownType(m_mapVersion >= 0);
    }
}

// Subdivision uses +1 before signed division on both axes. This is retail's
// rounding rule, including for negative coordinates, rather than a plain average.
static inline TPoint getRmgSubdivisionMidpoint(const TPoint& from, const TPoint& to)
{
    return TPoint((from.m_x + to.m_x + 1) / 2, (from.m_y + to.m_y + 1) / 2);
}

// Center one modulo-distributed draw on zero. Preserve modulo bias, asymmetric
// even ranges, signed division and one RNG draw; callers own range validity.
static inline int getRmgCenteredRandomOffset(int range)
{
    return rand() % range - range / 2;
}

// Displace an interior midpoint across its segment. Island outlines use
// half the segment length as their displacement limit; the other boundary
// drawers use the full length. Keep the single RNG draw after the length test.
static inline void displaceRmgBoundaryMidpoint(TPoint& midpoint,
    const TPoint& from, const TPoint& to, int roughness, int lengthDivisor)
{
    TRmgVector perpendicular;
    {
        TRmgVector delta = to - from;
        perpendicular = TRmgVector(-delta.m_y, delta.m_x);
    }
    int length = perpendicular.length();
    if (length > 1) {
        // Retail requires positive roughness here; zero reaches rand()%0.
        int limit = cppMin<long>(length / lengthDivisor, roughness);
        int displacement = getRmgCenteredRandomOffset(limit);
        perpendicular = perpendicular * displacement / length;
        midpoint += perpendicular;
    }
}

// One step of the depth-first subdivision worklist. A segment with an interior
// rounded midpoint is displaced and replaced by its two halves: pushing the
// far endpoint before the midpoint keeps the near half next. Otherwise the
// caller handles the segment's start cell and advances to its end.
static inline bool splitRmgBoundarySegment(std::vector<TPoint>& pending,
    const TPoint& from, const TPoint& to, int roughness, int lengthDivisor)
{
    TPoint midpoint = getRmgSubdivisionMidpoint(from, to);
    if (midpoint == from || midpoint == to)
        return false;
    displaceRmgBoundaryMidpoint(midpoint, from, to, roughness, lengthDivisor);
    pending.push_back(to);
    pending.push_back(midpoint);
    return true;
}

// Clamp each boundary coordinate in X-then-Y order before querying or
// opening the map cell. Keep the long-reference selectors used by retail.
static inline TPoint clampRmgBoundaryToMap(
    const TPoint& point, const type_random_map& map)
{
    long x = cppMax<long>(point.m_x, 0);
    x = cppMin<long>(x, map.m_mapWidth - 1);
    long y = cppMax<long>(point.m_y, 0);
    y = cppMin<long>(y, map.m_mapHeight - 1);
    return TPoint(x, y);
}

// Assign a cell to its zone and, under the caller's captured policy, mark it
// for zone-terrain painting. Zone outlines and the scanline fill share this;
// on the islands-mode surface only the coast and island interior are marked.
// The straight drawer's final cell receives only the zone assignment.
static inline void assignRmgZoneCell(
    TRmgMapItem* item, int zoneIndex, b8 markForTerrain)
{
    item->m_zoneState.m_zone = zoneIndex;
    if (markForTerrain)
        item->m_tileData.m_paintZoneTerrain = true;
}

// Depth-first midpoint displacement: rounding, displacement bounds and RNG
// order are observable in generated maps.
VA(0x0053BFF0, 0x22B)
MAC_ADDRESS(0x23d684, 0x408)
void type_random_map_generator::drawIrregularZoneBoundary(
    TPoint from, TPoint to, int zoneIndex, int level, int roughness)
{
    std::vector<TPoint> pending;
    b8 markForTerrain = level == 1 || m_waterContent != RMG_WATER_ISLANDS;
    pending.push_back(to);
    while (pending.size() > 0) {
        to = pending.back();
        pending.pop_back();
        if (!splitRmgBoundarySegment(pending, from, to, roughness, 1)) {
            TPoint clamped = clampRmgBoundaryToMap(from, m_map);
            TRmgMapItem* item = m_map.getMapItem(clamped.m_x, clamped.m_y, level);
            assignRmgZoneCell(item, zoneIndex, markForTerrain);
            from = to;
        }
    }
}

// Bresenham line rasterization with an accumulated half-major-axis error.
// The final cell receives its zone but deliberately not the terrain mark.
VA(0x0053C220, 0x16A)
MAC_ADDRESS(0x23da90, 0x2a4)
void type_random_map_generator::drawStraightZoneBoundary(
    TPoint from, TPoint to, int zoneIndex, int level)
{
    if (from.m_x > to.m_x)
        std::swap(from, to);
    int deltaX = to.m_x - from.m_x;
    int deltaY = to.m_y - from.m_y;
    int verticalDistance = abs(deltaY);
    int majorDistance;
    int minorDistance;
    TPoint axialStep;
    TPoint diagonalStep;
    if (deltaX > verticalDistance) {
        majorDistance = deltaX;
        minorDistance = verticalDistance;
        axialStep.m_x = 1;
        axialStep.m_y = 0;
        diagonalStep.m_y = deltaY > 0 ? 1 : -1;
    } else {
        majorDistance = verticalDistance;
        minorDistance = deltaX;
        axialStep = TPoint(0, deltaY > 0 ? 1 : -1);
        diagonalStep = axialStep;
    }
    diagonalStep.m_x = 1;
    b8 markForTerrain = level == 1 || m_waterContent != RMG_WATER_ISLANDS;
    int error = majorDistance / 2;
    while (from.m_x != to.m_x || from.m_y != to.m_y) {
        TRmgMapItem* item = m_map.getMapItem(from.m_x, from.m_y, level);
        assignRmgZoneCell(item, zoneIndex, markForTerrain);
        error += minorDistance;
        if (error < majorDistance) {
            from += axialStep;
        } else {
            error -= majorDistance;
            from += diagonalStep;
        }
    }
    TRmgMapItem* lastItem = m_map.getMapItem(from.m_x, from.m_y, level);
    lastItem->m_zoneState.m_zone = zoneIndex;
}

// Clip both endpoints toward the original opposite endpoint. The second
// clipping must not use the already-clipped first result as its target.
static inline void clipRmgBoundarySegment(const TRmgZoneBounds& bounds,
    const TPoint& originalFrom, const TPoint& originalTo, TPoint& from, TPoint& to)
{
    from = clipRmgBoundaryPoint(bounds, originalFrom, originalTo);
    to = clipRmgBoundaryPoint(bounds, originalTo, originalFrom);
}

// Clip each Voronoi edge, assign its map cells and record the zone polygon.
// Keep the owning-zone snapshot outside the neighbour branch; bounds and
// neighbour-dependent minimum temporaries retain separate lifetimes.
VA(0x0053C390, 0x730)
MAC_ADDRESS(0x23dd34, 0x614) // caller 0x53e5f4/0x53e602, ret 8; retail-only
void type_random_map_generator::traceZoneBoundary(
    TRmgHalfEdge* first, b8 irregular)
{
    TRmgHalfEdge* vertex = first;
    TRmgZone* zone = vertex->m_zone;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition zonePosition = zone->m_levelPosition;
    TRmgZoneBounds bounds;
    bounds.m_maximumY = m_map.m_mapHeight;
    bounds.m_minimumX = bounds.m_minimumY = 0;
    bounds.m_maximumX = m_map.m_mapWidth;
    TRmgHalfEdge* next;
    TPoint originalFrom;
    TPoint originalTo;
    TPoint from;
    TPoint to;

    bool found = false;
    do {
        next = vertex->m_next;
        originalFrom = vertex->m_position;
        originalTo = next->m_position;
        clipRmgBoundarySegment(bounds, originalFrom, originalTo, from, to);
        if (bounds.contains(from) && from != to) {
            found = true;
            break;
        }
        vertex = next;
    } while (vertex != first);
    if (!found) {
        TPoint upperLeft(bounds.m_minimumX, bounds.m_minimumY);
        TPoint upperRight(bounds.m_maximumX - 1, bounds.m_minimumY);
        TPoint lowerLeft(bounds.m_minimumX, bounds.m_maximumY - 1);
        TPoint lowerRight(bounds.m_maximumX - 1, bounds.m_maximumY - 1);
        drawStraightZoneBoundary(lowerRight, upperRight, zoneIndex, zonePosition.m_z);
        drawStraightZoneBoundary(upperRight, upperLeft, zoneIndex, zonePosition.m_z);
        drawStraightZoneBoundary(upperLeft, lowerLeft, zoneIndex, zonePosition.m_z);
        drawStraightZoneBoundary(lowerLeft, lowerRight, zoneIndex, zonePosition.m_z);
        zone->m_boundary.push_back(TPoint(lowerRight));
        zone->m_boundary.push_back(TPoint(upperRight));
        zone->m_boundary.push_back(TPoint(upperLeft));
        zone->m_boundary.push_back(TPoint(lowerLeft));
        return;
    }

    first = vertex;
    do {
        next = vertex->m_next;
        TRmgZone* neighbour = next->getOppositeZone();
        originalFrom = vertex->m_position;
        originalTo = next->m_position;
        clipRmgBoundarySegment(bounds, originalFrom, originalTo, from, to);
        zone->m_boundary.push_back(TPoint(from));

        if (!neighbour || neighbour->m_templateZone->m_zoneIndex > zoneIndex) {
            int roughness = zone->m_boundaryRoughness;
            if (neighbour) {
                int ownRoughness = roughness;
                int neighbourRoughness = neighbour->m_boundaryRoughness;
                roughness = min(ownRoughness, neighbourRoughness);
            }
            if (irregular)
                drawIrregularZoneBoundary(from, to, zoneIndex, zonePosition.m_z, roughness);
            else
                drawStraightZoneBoundary(from, to, zoneIndex, zonePosition.m_z);
        }

        vertex = next;
        if (to != originalTo) {
            from = to;
            for (;;) {
                next = next->m_next;
                to = clipRmgBoundaryPoint(bounds, vertex->m_position, next->m_position);
                if (bounds.contains(to))
                    break;
                vertex = next;
            }
            while (from.m_x != to.m_x && from.m_y != to.m_y) {
                TPoint corner;
                if (from.m_x == bounds.m_minimumX && from.m_y != bounds.m_minimumY)
                    corner = TPoint(bounds.m_minimumX, bounds.m_minimumY);
                else if (from.m_y == bounds.m_minimumY && from.m_x != bounds.m_maximumX - 1)
                    corner = TPoint(bounds.m_maximumX - 1, bounds.m_minimumY);
                else if (from.m_x == bounds.m_maximumX - 1 && from.m_y != bounds.m_maximumY - 1)
                    corner = TPoint(bounds.m_maximumX - 1, bounds.m_maximumY - 1);
                else
                    corner = TPoint(bounds.m_minimumX, bounds.m_maximumY - 1);
                drawStraightZoneBoundary(from, corner, zoneIndex, zonePosition.m_z);
                zone->m_boundary.push_back(TPoint(from));
                from = corner;
            }
            drawStraightZoneBoundary(from, to, zoneIndex, zonePosition.m_z);
            zone->m_boundary.push_back(TPoint(from));
        }
    } while (vertex != first);
}

// Reject clipping across an axis limit from its permitted side. The
// lower-bound test precedes the upper-bound test.
static inline bool crossesRmgBoundaryAxis(
    int original, int clipped, int minimum, int maximum)
{
    return (original >= minimum && clipped < minimum)
        || (original < maximum && clipped >= maximum);
}

// Ordered integer line/rectangle clipping (left, top, right, bottom).
// Each rounded intersection and early rejection is observable in maps; this
// is not a direct implementation of Cohen-Sutherland or Liang-Barsky clipping.
VA(0x0053CAC0, 0x266)
MAC_ADDRESS(0x23d1dc, 0x4a8)
TPoint clipRmgBoundaryPoint(
    const TRmgZoneBounds& bounds, TPoint point, TPoint toward)
{
    if (bounds.contains(point))
        return point;

    TRmgVector delta = toward - point;
    TPoint clipped = point;
    if (clipped.m_x < bounds.m_minimumX && delta.m_x) {
        clipped = clipped + delta * (bounds.m_minimumX - clipped.m_x) / delta.m_x;
        if (crossesRmgBoundaryAxis(point.m_y, clipped.m_y,
                bounds.m_minimumY, bounds.m_maximumY))
            return point;
    }
    if (clipped.m_y < bounds.m_minimumY && delta.m_y) {
        clipped = clipped + delta * (bounds.m_minimumY - clipped.m_y) / delta.m_y;
        if (crossesRmgBoundaryAxis(point.m_x, clipped.m_x,
                bounds.m_minimumX, bounds.m_maximumX))
            return point;
    }
    if (clipped.m_x >= bounds.m_maximumX && delta.m_x) {
        clipped = clipped + delta * (bounds.m_maximumX - clipped.m_x - 1) / delta.m_x;
        if (crossesRmgBoundaryAxis(point.m_y, clipped.m_y,
                bounds.m_minimumY, bounds.m_maximumY))
            return point;
    }
    if (clipped.m_y >= bounds.m_maximumY && delta.m_y) {
        clipped = clipped + delta * (bounds.m_maximumY - clipped.m_y - 1) / delta.m_y;
        if (crossesRmgBoundaryAxis(point.m_x, clipped.m_x,
                bounds.m_minimumX, bounds.m_maximumX))
            return point;
    }
    return clipped;
}

// Equal costs insert before existing equals, so older equal-cost work pops first.
// Unlike the position overload, this overload inserts the cost before the zone.
// Retail 0x53da1b..0x53da6f also snapshots the pointer argument separately.
static void insertRmgWorkItem(
    std::vector<TRmgZone*>& zones, std::vector<int>& costs,
    TRmgZone* zone, int cost)
{
    int insertionIndex = findRmgWorkItemInsertionIndex(costs, zones.size(), cost);
    costs.insert(costs.begin() + insertionIndex, cost);
    zones.insert(zones.begin() + insertionIndex, 1, zone);
}

// Random midpoint displacement, with half-length displacement bounds for
// the island coast. The zone-border variant uses the full segment length.
VA(0x0053CD30, 0x212)
MAC_ADDRESS(0x23e348, 0x3f0) // anchor-callee 0x53d34e; thiscall, ret 0x1c
void type_random_map_generator::drawIslandBoundary(TPoint from, TPoint to,
    int zoneIndex, int level, int roughness)
{
    std::vector<TPoint> pending;
    pending.push_back(to);
    while (pending.size() > 0) {
        to = pending.back();
        pending.pop_back();
        if (!splitRmgBoundarySegment(pending, from, to, roughness, 2)) {
            TPoint clamped = clampRmgBoundaryToMap(from, m_map);
            TRmgMapItem* item = m_map.getMapItem(clamped.m_x, clamped.m_y, level);
            if (item->m_zoneState.m_zone == zoneIndex)
                item->m_tileData.m_paintZoneTerrain = true;
            from = to;
        }
    }
}

// Four-connected depth-first flood fill marking the island for zone terrain;
// cells already marked, including the coast, stop growth.
VA(0x0053CF50, 0x177)
MAC_ADDRESS(0x23e738, 0x1f0)
void type_random_map_generator::fillIslandInterior(TRmgZone* zone)
{
    std::vector<TRmgMapPosition> pending;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition position = zone->getLevelPosition();
    pending.push_back(position);
    while (pending.size()) {
        position = pending.back();
        pending.pop_back();
        for (int direction = 0; direction < 8; direction += 2) {
            TRmgMapPosition next;
            next.m_x = position.m_x;
            next.m_y = position.m_y;
            TPoint offset = g_rmgDirections[direction];
            next += offset;
            next.m_z = position.m_z;
            if (!m_map.containsXY(next))
                continue;
            TRmgMapItem* item = m_map.getMapItem(next.m_x, next.m_y, next.m_z);
            if (item->shouldPaintZoneTerrain() || item->m_zoneState.m_zone != zoneIndex)
                continue;
            item->m_tileData.m_paintZoneTerrain = true;
            pending.push_back(next);
        }
    }
}

VA(0x0053D0D0, 0xE3)
MAC_ADDRESS(0x23e928, 0x184) // anchor-callee 0x53e6e8; thiscall, ret 4
void type_random_map_generator::recenterZone(TRmgZone* zone)
{
    TRmgZoneBounds bounds = zone->m_bounds;
    TRmgMapPosition position;
    position = zone->getLevelPosition();
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    int cellCount = 0;
    TRmgMapPosition coordinateTotal;
    coordinateTotal.m_x = 0;
    coordinateTotal.m_y = 0;
    coordinateTotal.m_z = position.m_z;
    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
        for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
            if (m_map.getMapItem(x, y, position.m_z)->m_zoneState.m_zone == zoneIndex) {
                ++cellCount;
                coordinateTotal.m_x += x;
                coordinateTotal.m_y += y;
            }
        }
    }
    if (cellCount) {
        coordinateTotal.m_x /= cellCount;
        coordinateTotal.m_y /= cellCount;
        zone->setLevelPosition(coordinateTotal);
    }
}

// Move one polygon vertex inward using the same clamped radius for the
// closing vertex and every subsequently visited vertex.
static inline void insetRmgIslandBoundaryPoint(
    TPoint& point, const TRmgMapPosition& center)
{
    TRmgVector delta(center.m_x - point.m_x, center.m_y - point.m_y);
    int length = delta.length();
    if (length > 0) {
        // Retail binds both clamps through long references.
        long displacement = std::max<long>(4, length / 4);
        displacement = std::min<long>(displacement, length / 2);
        delta = delta * displacement / length;
        point += delta;
    }
}

VA(0x0053D1C0, 0x1B9)
MAC_ADDRESS(0x23eaac, 0x2b0) // anchor-callee 0x53e70f; thiscall, ret 4
void type_random_map_generator::insetIslandZone(TRmgZone* zone)
{
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition center = zone->getLevelPosition();
    int count = zone->m_boundary.size();
    TPoint point = zone->m_boundary[0];
    insetRmgIslandBoundaryPoint(point, center);
    while (count--) {
        TPoint previous = point;
        point = zone->m_boundary[count];
        insetRmgIslandBoundaryPoint(point, center);
        drawIslandBoundary(point, previous, zoneIndex, center.m_z, zone->m_boundaryRoughness / 2);
    }
    fillIslandInterior(zone);
}

// Four-connected scanline fill of unassigned cells. An out-of-bounds center is
// clipped toward the interior ring site with the greatest edge clearance.
// X/Y guards keep left/up pointer offsets inside the contiguous map-item array.
// Mac 0x23ef84..0x23efb8 identifies the pending-seed removal as pop_back.
VA(0x0053D380, 0x551)
MAC_ADDRESS(0x23ed74, 0x4a4) // anchor-caller 0x53e050; Complete-only, thiscall ret 8
void type_random_map_generator::fillZoneArea(TRmgZone* zone, TRmgHalfEdge* first)
{
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    std::vector<TRmgMapPosition> pending;
    TRmgMapPosition position;
    position = zone->getLevelPosition();
    TRmgMapPosition upperSeed;
    TRmgMapPosition lowerSeed;
    if (!m_map.containsXY(position)) {
        int bestClearance = 0;
        TPoint best;
        best.m_x = -1;
        TRmgHalfEdge* edge = first;
        do {
            edge = edge->m_next;
            TPoint point = edge->getOppositeSitePosition();
            if (point.m_x >= 1 && point.m_x < m_map.getWidth() - 1
                && point.m_y >= 1 && point.m_y < m_map.getHeight() - 1) {
                int clearance = min(min(min(point.m_x,
                    m_map.getWidth() - point.m_x - 1), point.m_y),
                    m_map.getHeight() - point.m_y - 1);
                if (clearance > bestClearance) {
                    bestClearance = clearance;
                    best = point;
                }
            }
        } while (edge != first);
        if (best.m_x < 0)
            return;
        TRmgZoneBounds bounds = {0, 0, m_map.getWidth(), m_map.getHeight()};
        TPoint clipped = clipRmgBoundaryPoint(bounds,
            TPoint(position.m_x, position.m_y), best);
        position.m_x = clipped.m_x;
        position.m_y = clipped.m_y;
    }
    pending.push_back(position);
    while (pending.size()) {
        position = pending.back();
        pending.pop_back();
        TRmgMapItem* item = m_map.getMapItem(position);
        b8 hasUpperSpan = false;
        b8 hasLowerSpan = false;
        while (position.m_x > 0 && (item - 1)->m_zoneState.m_zone == -1) {
            --item;
            --position.m_x;
        }
        while (position.m_x < m_map.getWidth() && item->m_zoneState.m_zone == -1) {
            assignRmgZoneCell(item, zoneIndex,
                m_waterContent != RMG_WATER_ISLANDS || position.m_z == 1);
            if (position.m_y > 0) {
                if ((item - m_map.getWidth())->m_zoneState.m_zone == -1) {
                    if (!hasUpperSpan) {
                        upperSeed = position;
                        --upperSeed.m_y;
                        hasUpperSpan = true;
                    }
                } else if (hasUpperSpan) {
                    hasUpperSpan = false;
                    pending.push_back(upperSeed);
                }
            }
            if (position.m_y < m_map.getHeight() - 1) {
                if ((item + m_map.getWidth())->m_zoneState.m_zone == -1) {
                    if (!hasLowerSpan) {
                        lowerSeed = position;
                        ++lowerSeed.m_y;
                        hasLowerSpan = true;
                    }
                } else if (hasLowerSpan) {
                    hasLowerSpan = false;
                    pending.push_back(lowerSeed);
                }
            }
            ++item;
            ++position.m_x;
        }
        if (hasUpperSpan)
            pending.push_back(upperSeed);
        if (hasLowerSpan)
            pending.push_back(lowerSeed);
    }
}

// Unit-weight distance relaxation uses a sorted worklist, not FIFO traversal.
// Read the next distance from the per-zone short table, not the queued priority.
VA(0x0053D8E0, 0x1EC)
MAC_ADDRESS(0x23f218, 0x2f0) // anchor-callee 0x53dcd2/0x53e020; Complete-only, ret 4
void type_random_map_generator::propagateZoneDistances(TRmgZone* zone)
{
    std::vector<TRmgZone*> pending;
    std::vector<int> costs;
    int distanceCount = zone->m_zoneDistances.size();
    for (int column = 0; column < distanceCount; ++column) {
        pending.push_back(zone);
        costs.push_back(0);
        while (pending.size()) {
            TRmgZone* current = pending.back();
            pending.pop_back();
            TRmgTemplateZone* slot = current->m_templateZone;
            costs.pop_back();
            int distance = current->m_zoneDistances[column] + 1;
            for (unsigned int connection = 0; connection < slot->m_connections.size(); ++connection) {
                TRmgZone* next = m_zones[slot->m_connections[connection].m_destination->m_zoneIndex];
                if (next->m_zoneDistances[column] > distance) {
                    next->m_zoneDistances[column] = distance;
                    insertRmgWorkItem(pending, costs, next, distance);
                }
            }
        }
    }
}

VA_COMPGEN(0x0054C1E0, 0x1E9, VECTOR_RESIZE, Short)

VA_COMPGEN(0x0054C3D0, 0x12, VECTOR_SIZE, Short)

static void initializeRmgZoneDistances(
    type_random_map_generator* generator, int originalZones)
{
    for (int index = 0; index < generator->m_zones.size(); ++index) {
        TRmgZone* zone = generator->m_zones[index];
        zone->m_zoneDistances.resize(originalZones);
        for (int column = originalZones; column--;)
            zone->m_zoneDistances[column] = 32000;
        if (zone->m_templateZone->m_zoneIndex < originalZones)
            zone->m_zoneDistances[zone->m_templateZone->m_zoneIndex] = 0;
    }
}

// Search a zone's closed boundary ring for an edge adjoining another zone.
// Both extra-zone connection passes advance before testing, so keep that
// first-next order and test the starting edge last.
static inline TRmgHalfEdge* findRmgBoundaryWithZone(
    TRmgHalfEdge* first, const TRmgZone* destination)
{
    TRmgHalfEdge* edge = first;
    do {
        edge = edge->m_next;
        if (edge->getOppositeZone() == destination)
            return edge;
    } while (edge != first);
    return 0;
}

// Add both directed records for one unguarded extra-zone connection.
// Keep the player-filter limits uninitialized, as in retail: these generated
// records are consumed after template filtering. Append source first, then
// retarget the same caller-owned record for the reverse edge. The caller
// keeps the record storage/lifetime, including its untouched filter fields.
static inline void appendRmgExtraZoneConnection(
    TRmgZoneConnection& connection, TRmgZone* source, TRmgZone* destination,
    b8 connected)
{
    connection.m_destination = destination->m_templateZone;
    connection.m_value = 0;
    connection.m_unguarded = true;
    connection.m_placeBorderObjects = false;
    connection.m_connected = connected;
    source->m_templateZone->m_connections.push_back(connection);
    connection.m_destination = source->m_templateZone;
    destination->m_templateZone->m_connections.push_back(connection);
}

// Extra-to-extra edges are completed unguarded connections when their boundary
// intersects the map. Extra-to-original edges must not shorten another original
// zone's distance; accepted changes trigger another graph relaxation.
VA(0x0053DAD0, 0x57F)
MAC_ADDRESS(0x23f518, 0x40c) // anchor-callee buildZoneBoundaries; Complete-only, ret 8
void type_random_map_generator::joinExtraZones(int originalZones, TRmgVoronoi* diagram)
{
    TRmgZoneBounds bounds = {0, 0, m_map.m_mapWidth, m_map.m_mapHeight};
    for (int index = originalZones; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        TRmgMapPosition position = zone->getLevelPosition();
        TRmgHalfEdge* first = diagram->locate(TPoint(position.m_x, position.m_y));
        for (int other = index + 1; other < m_zones.size(); ++other) {
            TRmgZone* destination = m_zones[other];
            if (destination->getLevelPosition().m_z != zone->getLevelPosition().m_z)
                continue;
            TRmgHalfEdge* edge = findRmgBoundaryWithZone(first, destination);
            if (!edge)
                continue;
            TPoint clipped = clipRmgBoundaryPoint(bounds, edge->m_position, edge->m_previous->m_position);
            if (bounds.contains(clipped)) {
                TRmgZoneConnection connection;
                appendRmgExtraZoneConnection(connection, zone, destination, true);
            }
        }
    }
    initializeRmgZoneDistances(this, originalZones);
    for (index = 0; index < originalZones; ++index)
        propagateZoneDistances(m_zones[index]);

    for (index = originalZones; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        TRmgMapPosition position = zone->getLevelPosition();
        TRmgHalfEdge* first = diagram->locate(TPoint(position.m_x, position.m_y));
        for (int other = 0; other < originalZones; ++other) {
            TRmgZone* destination = m_zones[other];
            if (destination->getLevelPosition().m_z != zone->getLevelPosition().m_z)
                continue;
            TRmgHalfEdge* edge = findRmgBoundaryWithZone(first, destination);
            if (!edge)
                continue;
            int column = 0;
            for (; column < originalZones; ++column) {
                if (column != other
                    && destination->m_zoneDistances[column] > zone->m_zoneDistances[column] + 1)
                    break;
            }
            if (column < originalZones)
                continue;
            TRmgZoneConnection connection;
            appendRmgExtraZoneConnection(connection, zone, destination, false);
            propagateZoneDistances(destination);
        }
    }
}

VA_COMPGEN(0x0054DE90, 0x14, STD_CONSTRUCT, TRmgZoneConnection)

// Existing zones seed the subdivision; radial sites add surface water zones
// and underground boundaries. Keep one subdivision lifetime and the nested
// temporary slot/zone pair. The selected zone stays live across radial inserts;
// copy each center before scaling and keep bound temporaries inside their guards.
VA(0x0053E050, 0x64D)
MAC_ADDRESS(0x23f924, 0x798) // anchor-callee 0x549af9; thiscall, ret 8
void type_random_map_generator::buildZoneBoundaries(
    TRmgTemplate* mapTemplate, int level)
{
    TPoint query;
    TRmgVoronoi diagram;
    for (int zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->getLevelPosition().m_z == level) {
            TRmgMapPosition position = m_zones[zone]->getLevelPosition();
            diagram.addSite(TPoint(position.m_x, position.m_y), m_zones[zone]);
        }
    }
    int originalZones = m_zones.size();
    if (level == 1 || m_waterContent != RMG_WATER_NONE) {
        TRmgTemplateZone testSlot;
        testSlot.m_zoneIndex = -1;
        testSlot.m_kind = RMG_TEMPLATE_JUNCTION;
        testSlot.m_size = 0;
        TRmgZone testZone(&testSlot);
        TRmgZone* addedZone = 0;
        for (zone = 0; zone < originalZones; ++zone) {
            TRmgZone* current = m_zones[zone];
            if (current->getLevelPosition().m_z != level)
                continue;
            int radius = current->m_boundaryRoughness;
            testSlot.m_size = radius;
            TRmgMapPosition position = current->getLevelPosition();
            for (int direction = 0; direction < 32; direction += 4) {
                TRmgMapPosition horizontalCenter;
                horizontalCenter = current->getLevelPosition();
                double dx = radius * g_rmgDirectionCosines[direction];
                position.m_x = static_cast<int>(horizontalCenter.m_x + dx * 2);
                TRmgMapPosition verticalCenter;
                verticalCenter = current->getLevelPosition();
                double dy = radius * g_rmgDirectionSines[direction];
                position.m_y = static_cast<int>(verticalCenter.m_y + dy * 2);
                if (position.m_x < 0 && position.m_x < dx)
                    continue;
                if (position.m_x >= m_map.m_mapWidth) {
                    int maximumWidth = m_map.m_mapWidth;
                    if (position.m_x >= maximumWidth + dx)
                        continue;
                }
                if (position.m_y < 0 && position.m_y < dy)
                    continue;
                if (position.m_y >= m_map.m_mapHeight) {
                    int maximumHeight = m_map.m_mapHeight;
                    if (position.m_y >= maximumHeight + dy)
                        continue;
                }
                testZone.setLevelPosition(position);
                if (!canPlaceZone(&testZone))
                    continue;
                if (position.m_z == 0) {
                    // RETAIL BUG: m_allowedTowns is untouched before the constructor
                    // reads it at 0x53e45c; recycled heap contents affect RNG consumption.
                    TRmgTemplateZone* slot = new TRmgTemplateZone;
                    slot->m_zoneIndex = mapTemplate->m_zones.size();
                    slot->m_size = radius;
                    memset(slot->m_allowedMonsters, 0, sizeof(slot->m_allowedMonsters));
                    memset(slot->m_allowedTerrain, 0, sizeof(slot->m_allowedTerrain));
                    memset(slot->m_mineCounts, 0, sizeof(slot->m_mineCounts));
                    memset(slot->m_mineDensities, 0, sizeof(slot->m_mineDensities));
                    slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT] = 0;
                    slot->m_townPlacement[RMG_TOWN_PLAYER_OPTION_COUNT] = 0;
                    slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_DENSITY] = 0;
                    slot->m_townPlacement[RMG_TOWN_PLAYER_OPTION_DENSITY] = 0;
                    slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT] = 0;
                    slot->m_townPlacement[RMG_TOWN_NEUTRAL_OPTION_COUNT] = 0;
                    slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_DENSITY] = 0;
                    slot->m_townPlacement[RMG_TOWN_NEUTRAL_OPTION_DENSITY] = 0;
                    slot->m_monsterStrength = 0;
                    slot->m_playerIndex = -1;
                    memset(slot->m_treasure, 0, sizeof(slot->m_treasure));
                    slot->m_treasure[0].m_density = 5;
                    slot->m_treasure[0].m_maximum = 1000;
                    slot->m_treasure[0].m_minimum = 100;
                    slot->m_treasure[1].m_density = 1;
                    slot->m_treasure[1].m_maximum = 6000;
                    slot->m_treasure[1].m_minimum = 2000;
                    slot->m_kind = RMG_TEMPLATE_JUNCTION;
                    addedZone = new TRmgZone(slot);
                    addedZone->m_terrain = eTerrainWater;
                    addedZone->setLevelPosition(position);
                    mapTemplate->m_zones.push_back(slot);
                    m_zones.push_back(addedZone);
                }
                diagram.addSite(TPoint(position.m_x, position.m_y), addedZone);
            }
        }
    }
    diagram.buildVertices();
    for (zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->getLevelPosition().m_z == level) {
            TRmgMapPosition position = m_zones[zone]->getLevelPosition();
            query.m_y = position.m_y;
            query.m_x = position.m_x;
            TRmgHalfEdge* first = diagram.locate(query);
            traceZoneBoundary(first,
                zone < originalZones && (m_waterContent != RMG_WATER_ISLANDS || level == 1));
        }
    }
    for (zone = 0; zone < m_zones.size(); ++zone) {
        TRmgZone* current = m_zones[zone];
        if (current->getLevelPosition().m_z == level) {
            TRmgMapPosition position = current->getLevelPosition();
            query.m_y = position.m_y;
            query.m_x = position.m_x;
            fillZoneArea(current, diagram.locate(query));
        }
    }
    joinExtraZones(originalZones, &diagram);
}

// Mac retains the view-map constructor at 0x22d320 and expands the initial lookup.
VA(0x0053E6A0, 0x337)
MAC_ADDRESS(0x2400bc, 0x354)
void type_random_map_generator::paintZoneTerrain()
{
    calculateZoneBounds();
    for (int zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        recenterZone(zone);
        if (m_waterContent == RMG_WATER_ISLANDS && zone->getLevelPosition().m_z == 0)
            insetIslandZone(zone);
    }
    if (m_map.getNumberLevels() > 1) {
        type_random_map levelMap(m_map.getMapItem(0, 0, 1),
            m_map.getWidth(), m_map.getHeight());
        TRmgTerrainBrush brush(&levelMap, eTerrainRock, 4);
        brush.paintRectangle(0, 0, m_map.getWidth(), m_map.getHeight());
        if (m_progress)
            m_progress->advance(12500);
    }
    {
        TRmgTerrainBrush brush(&m_map, eTerrainWater, 4);
        brush.paintRectangle(0, 0, m_map.getWidth(), m_map.getHeight());
    }
    int progressSteps = 15800 / m_zones.size();
    for (zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgZoneBounds bounds = zone->getBounds();
        TRmgMapPosition position = zone->getLevelPosition();
        if (zone->getTerrain() != eTerrainWater) {
            type_random_map levelMap(m_map.getMapItem(0, 0, position.m_z),
                m_map.getWidth(), m_map.getHeight());
            TRmgTerrainBrush brush(&levelMap, zone->getTerrain(), 4);
            for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
                for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                    TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
                    if (item->m_zoneState.m_zone == zoneIndex && item->shouldPaintZoneTerrain())
                        brush.paintRectangle(x, y, 1, 1);
                }
            }
            if (m_progress)
                m_progress->advance(progressSteps);
        }
    }
}

// Omit collapsed dimensions, but preserve reversed bounds.
static inline void appendRmgNoiseQuadrant(
    std::vector<TRmgNoiseRegion>& pending, const TRmgNoiseRegion& quadrant)
{
    if (quadrant.m_bounds.m_minimumX != quadrant.m_bounds.m_maximumX
        && quadrant.m_bounds.m_minimumY != quadrant.m_bounds.m_maximumY)
        pending.push_back(quadrant);
}

// Each nondegenerate subdivision quadrant inherits the original variation.
// Preserve Y-before-X midpoint evaluation. The center is last in the source
// signature despite being passed in EDX before the two by-value records.
VA(0x0053E9E0, 0x31E)
MAC_ADDRESS(0x240410, 0x2ac) // anchor-callee 0x53ed91; Complete-only, fastcall ret 0x34
void subdivideRmgNoiseRegion(std::vector<TRmgNoiseRegion>& pending,
    TRmgNoiseRegion region,
    TRmgNoiseMidpoints midpoints,
    int centerValue)
{
    int midpointCoordinates[2] = {
        (region.m_bounds.m_minimumY + region.m_bounds.m_maximumY) / 2,
        (region.m_bounds.m_minimumX + region.m_bounds.m_maximumX) / 2
    };
    TRmgNoiseRegion quadrant = region;
    quadrant.m_bounds.m_minimumX = midpointCoordinates[1];
    quadrant.m_bounds.m_minimumY = midpointCoordinates[0];
    quadrant.m_corners[0] = centerValue;
    quadrant.m_corners[1] = midpoints.m_maxYValue;
    quadrant.m_corners[2] = midpoints.m_maxXValue;
    appendRmgNoiseQuadrant(pending, quadrant);

    quadrant = region;
    quadrant.m_bounds.m_minimumX = midpointCoordinates[1];
    quadrant.m_bounds.m_maximumY = midpointCoordinates[0];
    quadrant.m_corners[0] = midpoints.m_minYValue;
    quadrant.m_corners[1] = centerValue;
    quadrant.m_corners[3] = midpoints.m_maxXValue;
    appendRmgNoiseQuadrant(pending, quadrant);

    quadrant = region;
    quadrant.m_bounds.m_maximumX = midpointCoordinates[1];
    quadrant.m_bounds.m_minimumY = midpointCoordinates[0];
    quadrant.m_corners[0] = midpoints.m_minXValue;
    quadrant.m_corners[2] = centerValue;
    quadrant.m_corners[3] = midpoints.m_maxYValue;
    appendRmgNoiseQuadrant(pending, quadrant);

    quadrant = region;
    quadrant.m_bounds.m_maximumX = midpointCoordinates[1];
    quadrant.m_bounds.m_maximumY = midpointCoordinates[0];
    quadrant.m_corners[1] = midpoints.m_minXValue;
    quadrant.m_corners[2] = midpoints.m_minYValue;
    quadrant.m_corners[3] = centerValue;
    appendRmgNoiseQuadrant(pending, quadrant);
}

// The nine-dword copy stride identifies this vector specialization.
VA_COMPGEN(0x0054C670, 0x21, VECTOR_SIZE, TRmgNoiseRegion)

VA_COMPGEN(0x0054D5C0, 0x2E4, VECTOR_INSERT_COUNT, TRmgNoiseRegion)

VA_COMPGEN(0x0054D960, 0x3B, VECTOR_UCOPY, TRmgNoiseRegion)

VA_COMPGEN(0x0054D9A0, 0x31, VECTOR_UFILL, TRmgNoiseRegion)

// The inlined search at 0x53fe7a returns an element pointer and its caller
// then tests that pointer, even on the found arm. Preserve that ordinary
// helper boundary rather than reducing the search to a boolean.
TRmgZoneConnection* TRmgTemplateZone::findConnection(int destinationZone)
{
    for (unsigned int connectionIndex = 0; connectionIndex < m_connections.size(); ++connectionIndex) {
        if (m_connections[connectionIndex].m_destination->m_zoneIndex == destinationZone)
            return &m_connections[connectionIndex];
    }
    return 0;
}

// Retail 0x53f048 passes the mask in ECX, width in EDX and height on the stack.
void __fastcall generateRmgIslandMask(unsigned char* mask, int width, int height);

// Initialize the Y edge pair before X. Each random displacement keeps its own
// range / 2 expression and lifetime.
VA(0x0053ED00, 0x29B)
MAC_ADDRESS(0x2406bc, 0x484)
void __fastcall generateRmgIslandMask(unsigned char* mask, int width, int height)
{
    std::vector<TRmgNoiseRegion> patches;
    // This noise grid deliberately uses bounds X for rows and Y for columns:
    // subdivision receives (height, width), and output uses X*width + Y.
    TRmgNoiseRegion patch = {
        { 0, 0, height, width }, { 0, 0, 0, 0 }, (height + width) / 4 + 1
    };
    TRmgNoiseMidpoints edges;
    edges.m_minYValue = 0;
    edges.m_maxYValue = 0;
    edges.m_minXValue = 0;
    edges.m_maxXValue = 0;
    subdivideRmgNoiseRegion(patches, patch, edges, patch.m_variation / 2);
    while (patches.size()) {
        patch = patches.back();
        patches.pop_back();
        if (patch.m_bounds.m_maximumY == patch.m_bounds.m_minimumY + 1
            && patch.m_bounds.m_maximumX == patch.m_bounds.m_minimumX + 1) {
            if (patch.m_bounds.m_minimumX < 0 || patch.m_bounds.m_minimumX >= height
                || patch.m_bounds.m_minimumY < 0 || patch.m_bounds.m_minimumY >= width)
                continue;
            patch.m_corners[0] = min(max(patch.m_corners[0], 0), 255);
            mask[patch.m_bounds.m_minimumX * width + patch.m_bounds.m_minimumY] = patch.m_corners[0];
            continue;
        }
        if (patch.m_bounds.m_maximumX < 0 || patch.m_bounds.m_maximumY < 0
            || patch.m_bounds.m_minimumX >= height || patch.m_bounds.m_minimumY >= width)
            continue;
        edges.m_minXValue = (patch.m_corners[1] + patch.m_corners[0]) / 2;
        edges.m_minYValue = (patch.m_corners[2] + patch.m_corners[0]) / 2;
        edges.m_maxXValue = (patch.m_corners[3] + patch.m_corners[2]) / 2;
        edges.m_maxYValue = (patch.m_corners[3] + patch.m_corners[1]) / 2;
        int center = (patch.m_corners[3] + patch.m_corners[2]
            + patch.m_corners[1] + patch.m_corners[0]) / 4;
        int range = patch.m_variation;
        if (range > 1) {
            edges.m_minXValue += getRmgCenteredRandomOffset(range);
            edges.m_minYValue += getRmgCenteredRandomOffset(range);
            edges.m_maxXValue += getRmgCenteredRandomOffset(range);
            edges.m_maxYValue += getRmgCenteredRandomOffset(range);
            center += getRmgCenteredRandomOffset(range);
        }
        patch.m_variation = (range - 1) / 2 + 1;
        subdivideRmgNoiseRegion(patches, patch, edges, center);
    }
}

VA(0x0053EFA0, 0x1F2)
MAC_ADDRESS(0x240ba8, 0x270)
void type_random_map_generator::createWaterZoneIsland(const TRmgZoneBounds& bounds, int level)
{
    int width = bounds.m_maximumX - bounds.m_minimumX;
    int height = bounds.m_maximumY - bounds.m_minimumY;
    unsigned char* mask = new unsigned char[width * height];
    TRmgMapPosition point;
    int terrain = rand() % 6;
    {
        type_random_map map(m_map.getMapItem(0, 0, level),
            m_map.m_mapWidth, m_map.m_mapHeight);
        TRmgTerrainBrush brush(&map, terrain, 4);
        generateRmgIslandMask(mask, width, height);
        for (point.m_y = bounds.m_minimumY; point.m_y < bounds.m_maximumY; ++point.m_y) {
            for (point.m_x = bounds.m_minimumX; point.m_x < bounds.m_maximumX; ++point.m_x) {
                if (mask[(point.m_y - bounds.m_minimumY) * width + point.m_x - bounds.m_minimumX] > 0)
                    brush.paintRectangle(point.m_x, point.m_y, 1, 1);
            }
        }
    }
    point.m_z = level;
    for (point.m_y = bounds.m_minimumY; point.m_y < bounds.m_maximumY; ++point.m_y) {
        for (point.m_x = bounds.m_minimumX; point.m_x < bounds.m_maximumX; ++point.m_x) {
            TRmgMapItem* item = m_map.getMapItem(point.m_x, point.m_y, point.m_z);
            if (item->getLandType() != eTerrainWater) {
                item->markBorderObject();
            }
        }
    }
    delete[] mask;
    if (m_progress)
        m_progress->advance(1000);
}

// Eight-neighbour chamfer distances use cardinal/diagonal costs 2/3. Only the
// supplied zone is traversed; accepting a shorter distance resets connection
// metadata without checking terrain or objects. Copy both direction components
// before reading the current position.
VA(0x0053F1A0, 0x2C6)
MAC_ADDRESS(0x240ee4, 0x358)
void type_random_map_generator::floodWaterZoneDistances(TRmgMapPosition position, int zoneIndex)
{
    std::vector<TRmgMapPosition> positions;
    std::vector<int> costs;
    positions.push_back(position);
    costs.push_back(0);
    TRmgMapItem* seed = m_map.getMapItem(position);
    seed->setWaterZoneDistance(0, 0);
    while (positions.size()) {
        popRmgMovementPosition(position, positions, costs);
        unsigned currentCost = m_map.getMapItem(position)->m_movement.m_zonePathCost;
        for (int direction = 0; direction < 8; ++direction) {
            TPoint offset = g_rmgDirections[direction];
            TRmgMapPosition next;
            next.m_x = position.m_x + offset.m_x;
            next.m_y = position.m_y + offset.m_y;
            next.m_z = position.m_z;
            if (!m_map.containsXY(next))
                continue;
            TRmgMapItem* item = m_map.getMapItem(next);
            if (item->m_zoneState.m_zone != zoneIndex)
                continue;
            unsigned nextCost;
            if (direction & 1)
                nextCost = currentCost + 3;
            else
                nextCost = currentCost + 2;
            if (nextCost >= item->m_movement.m_zonePathCost)
                continue;
            item->setWaterZoneDistance(nextCost, direction);
            insertRmgWorkItem(positions, costs, next, nextCost);
        }
    }
}

// Seed islands at least 20 distance units from the current coast, rebuilding
// the candidate list after every island.
VA(0x0053F470, 0x409)
MAC_ADDRESS(0x24123c, 0x638)
void type_random_map_generator::prepareWaterZoneConnections(TRmgZone* zone)
{
    if (zone->m_terrain != eTerrainWater)
        return;
    TRmgZoneBounds bounds = zone->m_bounds;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition position;
    position = zone->m_levelPosition;
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position);
            item->setWaterZoneDistance(32000, 0);
        }
    }
    TRmgZoneBounds surrounding;
    surrounding.m_minimumX = max(bounds.m_minimumX - 1, 0);
    surrounding.m_minimumY = max(bounds.m_minimumY - 1, 0);
    surrounding.m_maximumX = min(bounds.m_maximumX + 1, m_map.getWidth());
    surrounding.m_maximumY = min(bounds.m_maximumY + 1, m_map.getHeight());
    for (position.m_y = surrounding.m_minimumY; position.m_y < surrounding.m_maximumY; ++position.m_y) {
        for (position.m_x = surrounding.m_minimumX; position.m_x < surrounding.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position);
            if (item->m_zoneState.m_zone != zoneIndex)
                floodWaterZoneDistances(position, zoneIndex);
        }
    }
    bounds.m_minimumX = max(bounds.m_minimumX, 3);
    bounds.m_minimumY = max(bounds.m_minimumY, 3);
    bounds.m_maximumX = min(bounds.m_maximumX, m_map.getWidth() - 4);
    bounds.m_maximumY = min(bounds.m_maximumY, m_map.getHeight() - 4);
    while (1) {
        std::vector<TRmgMapPosition> candidates;
        for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
            for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
                if (m_map.getMapItem(position)->m_movement.m_zonePathCost >= 20)
                    candidates.push_back(position);
            }
        }
        if (!candidates.size())
            break;
        position = candidates[rand() % candidates.size()];
        TRmgMapItem* selectedItem = m_map.getMapItem(position);
        int range = selectedItem->m_movement.m_zonePathCost / 3 - 5;
        int radius = rand() % range + 3;
        if (radius > 6)
            radius = 6;
        TRmgZoneBounds island;
        island.m_minimumX = max(position.m_x - radius, 0);
        island.m_minimumY = max(position.m_y - radius, 0);
        island.m_maximumX = min(position.m_x + radius, m_map.getWidth());
        island.m_maximumY = min(position.m_y + radius, m_map.getHeight());
        createWaterZoneIsland(island, position.m_z);
        floodWaterZoneDistances(position, zoneIndex);
    }
}

// Separate dry assigned cells near water or another zone according to the
// connection, level and guard policy. Keep the two distinct connection-policy
// tests at 0x53fa3f..0x53fa58.
VA(0x0053F880, 0x429)
MAC_ADDRESS(0x241874, 0x628)
void type_random_map_generator::expandObstacleClearance()
{
    TRmgMapItem* current = m_map.m_mapItems;
    TRmgMapPosition position;
    TRmgMapPosition nearby;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++current) {
                int zoneIndex = current->m_zoneState.m_zone;
                if (zoneIndex < 0 || current->getLandType() == eTerrainWater)
                    continue;
                TRmgZoneBounds bounds;
                setRmgNeighborhoodBounds(bounds, position, m_map, 1);
                TRmgZone* zone = m_zones[zoneIndex];
                b8 needsClearance = false;
                nearby.m_z = position.m_z;
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        int otherZone = item->m_zoneState.m_zone;
                        if (otherZone < 0) {
                            if (item->getLandType() == eTerrainWater)
                                needsClearance = true;
                        } else if (otherZone != zoneIndex) {
                            TRmgZoneConnection* connection = zone->m_templateZone->findConnection(otherZone);
                            if (!connection || position.m_z == 1)
                                needsClearance = true;
                            if (connection && !connection->m_unguarded)
                                needsClearance = true;
                        }
                    }
                }
                if (!needsClearance)
                    continue;
                current->markBorderObject();
                setRmgNeighborhoodBounds(bounds, position, m_map, 0);
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        if (item->getLandType() != eTerrainWater
                            && static_cast<int>(item->m_objects.size()) <= 0) {
                            item->markBorderObject();
                        }
                    }
                }
                setRmgNeighborhoodBounds(bounds, position, m_map, 1);
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        item->releaseUnoccupiedPathClearance();
                    }
                }
            }
        }
    }
    if (m_progress)
        m_progress->advance(1600);
}

// Mac terrain push_back 0x251f80 and clear 0x252374 preserve the terrain enum
// domain; the subtype vector in placement-rule loading is a distinct POD type.
VA(0x0053FCB0, 0x5EC)
MAC_ADDRESS(0x241e9c, 0x770)
void type_random_map_generator::repairWaterZoneBorders()
{
    TRmgMapItem* current = m_map.m_mapItems;
    TTerrainType terrain;
    TRmgMapPosition position;
    TRmgMapPosition nearby;
    std::vector<TRmgMapPosition> positions;
    std::vector<TTerrainType> terrains;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++current) {
                int zoneIndex = current->m_zoneState.m_zone;
                if (zoneIndex < 0 || current->getLandType() != eTerrainWater)
                    continue;
                int destinationZone = current->m_zoneState.m_connectionEligibility;
                if (destinationZone < 0)
                    continue;

                b8 foundLandTerrain = false;
                TRmgZoneBounds bounds;
                setRmgNeighborhoodBounds(bounds, position, m_map, 1);
                nearby.m_z = position.m_z;
                TRmgZone* zone = m_zones[zoneIndex];
                for (nearby.m_y = bounds.m_minimumY;
                     nearby.m_y < bounds.m_maximumY && !foundLandTerrain; ++nearby.m_y) {
                    nearby.m_x = bounds.m_minimumX;
                    if (nearby.m_x < bounds.m_maximumX) {
                        do {
                            TRmgMapItem* item = m_map.getMapItem(nearby);
                            if (item->getLandType() != eTerrainWater
                                && item->isPassableLand()
                                && !item->hasBorderObject()) {
                                terrain = H3_ENUM_DECODE(TTerrainType, item->getLandType());
                                foundLandTerrain = true;
                                break;
                            }
                            ++nearby.m_x;
                            if (nearby.m_x >= bounds.m_maximumX)
                                break;
                        } while (1);
                    }
                }
                if (!foundLandTerrain || zone->m_templateZone->findConnection(destinationZone))
                    continue;

                setRmgNeighborhoodBounds(bounds, position, m_map, 1);
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        item->markBorderObject();
                        if (item->getLandType() == eTerrainWater) {
                            positions.push_back(nearby);
                            terrains.push_back(terrain);
                        }
                    }
                }

                setRmgNeighborhoodBounds(bounds, position, m_map, 2);
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        item->releaseUnoccupiedPathClearance();
                    }
                }
            }
            if (m_progress)
                m_progress->advance(20);
        }
        if (positions.size()) {
            TTerrainType lastTerrain = terrains[0];
            type_random_map levelMap(m_map.getMapItem(0, 0, position.m_z),
                m_map.m_mapWidth, m_map.m_mapHeight);
            TRmgTerrainBrush brush(&levelMap, lastTerrain, 4);
            for (unsigned int paintIndex = 0; paintIndex < positions.size(); ++paintIndex) {
                terrain = terrains[paintIndex];
                if (terrain != lastTerrain) {
                    brush.changeTerrain(terrain, 4);
                    lastTerrain = terrain;
                }
                position = positions[paintIndex];
                brush.paintRectangle(position.m_x, position.m_y, 1, 1);
            }
            positions.clear();
            terrains.clear();
        }
    }
}

// Generator vtable 0x640c44 slot 1 uses the base object's by-value position
// interface. Registration precedes type counting and entrance-score propagation.
// Keep the canonical base, position-addition and sorted-worklist helpers.
VA(0x005402A0, 0x32A)
MAC_ADDRESS(0x24260c, 0x434) // anchor-vtable + generator/map layouts; retail-only
void type_random_map_generator::addObject(type_object* object, TRmgMapPosition position)
{
    TRmgGeneratorBase::addObject(object, position);
    TObjectType* prototype = object->m_properties->m_prototype;
    int objectType = prototype->getObjectType();
    ++m_objectCountByType[objectType];
    if (prototype->m_hasTrigger) {
        TObjectType::TPoint trigger = prototype->m_triggerCell;
        std::vector<TRmgMapPosition> positions;
        std::vector<int> costs;
        TRmgMapPosition currentPosition = getRmgObjectTriggerPosition(position, trigger);
        TRmgMapItem* seed = m_map.getMapItem(currentPosition);
        int zoneIndex = seed->m_zoneState.m_zone;
        if (zoneIndex >= 0)
            ++m_zones[zoneIndex]->m_objectCountByType[objectType];
        seed->m_zoneState.m_score = 0;
        positions.push_back(currentPosition);
        costs.push_back(0);
        while (positions.size()) {
            popRmgMovementPosition(currentPosition, positions, costs);
            int cost = m_map.getMapItem(currentPosition)->m_zoneState.m_score + 2;
            for (int direction = 0; direction < RMG_DIRECTION_COUNT; ++direction) {
                int nextCost = cost;
                if (direction & 1)
                    ++nextCost;
                TRmgMapPosition nextPosition = currentPosition + g_rmgDirections[direction];
                if (!m_map.containsXY(nextPosition))
                    continue;
                TRmgMapItem* next = m_map.getMapItem(nextPosition);
                if (nextCost >= next->m_zoneState.m_score)
                    continue;
                next->m_zoneState.m_score = nextCost;
                insertRmgWorkItem(positions, costs, nextPosition, nextCost);
            }
        }
    }
}

// Preserve separate movement-cost resets and predecessor construction within
// each cell iteration. Zone bounds are captured before flooding; its level is
// read again afterward because the flood can observe or mutate the zone.
VA(0x005405D0, 0x304)
MAC_ADDRESS(0x242a40, 0x448)
void type_random_map_generator::buildZoneConnectionPaths()
{
    int count = m_map.m_numberLevels * m_map.m_mapHeight * m_map.m_mapWidth;
    TRmgMapItem* item = m_map.m_mapItems;
    while (count--) {
        item->m_movement.m_zonePathCost = 32000;
        item->m_tileData.m_connectionDirection = 0;
        item->m_zoneState.m_connectionEligibility = -1;
        TRmgMapPosition previous;
        previous.m_x = -1;
        previous.m_y = -1;
        previous.m_z = -1;
        item->resetMovement(previous);
        ++item;
    }
    for (unsigned int zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgZoneBounds bounds = zone->m_bounds;
        TRmgMapPosition position = zone->m_levelPosition;
        // RETAIL BUG: if the zone contains no eligible empty cell, the scan
        // never assigns seed, yet the fallback below still uses it.
        TRmgMapPosition seed;
        TRmgMapPosition pathPosition;
        b8 foundClearPath = false;
        for (pathPosition.m_y = bounds.m_minimumY;
             pathPosition.m_y < bounds.m_maximumY && !foundClearPath; ++pathPosition.m_y) {
            int x = bounds.m_minimumX;
            if (x < bounds.m_maximumX) {
                do {
                    TRmgMapItem* current = m_map.getMapItem(x, pathPosition.m_y, position.m_z);
                    if (current->m_zoneState.m_zone == zoneIndex) {
                        unsigned terrain = current->getLandType();
                        if ((terrain != eTerrainWater || zone->m_terrain == terrain)
                            && static_cast<int>(current->m_objects.size()) <= 0) {
                            seed.m_x = x;
                            seed.m_y = pathPosition.m_y;
                            seed.m_z = position.m_z;
                            if (current->hasPathClearance() && current->m_tileData.m_roadPassable
                                && terrain != eTerrainRock) {
                                foundClearPath = true;
                                break;
                            }
                        }
                    }
                    ++x;
                    if (x >= bounds.m_maximumX)
                        break;
                } while (1);
            }
        }
        if (!foundClearPath) {
            TRmgMapItem* current = m_map.getMapItem(seed);
            current->openPath();
        }
        m_map.floodConnectionCosts(seed, zone->m_terrain == eTerrainWater);
        pathPosition = zone->m_levelPosition;
        for (pathPosition.m_y = bounds.m_minimumY; pathPosition.m_y < bounds.m_maximumY; ++pathPosition.m_y) {
            for (pathPosition.m_x = bounds.m_minimumX; pathPosition.m_x < bounds.m_maximumX; ++pathPosition.m_x) {
                TRmgMapItem* current = m_map.getMapItem(pathPosition);
                if (current->m_zoneState.m_zone == zoneIndex
                    && current->hasPathClearance() && current->isPassableLand()
                    && current->m_movement.m_cost
                    && current->getLandType() != eTerrainWater) {
                    openConnectionPath(pathPosition, false);
                    m_map.floodConnectionCosts(pathPosition, zone->m_terrain == eTerrainWater);
                }
            }
        }
    }
}

// Widen an opened route by removing same-zone border obstacles, retaining
// cells reserved for a connection. Bounds calculation stays with each caller
// to preserve its integer types; visit the rectangle in row-major order.
static inline void clearRmgZonePathBorders(type_random_map& map,
    const TRmgZoneBounds& bounds, int level, int zoneIndex)
{
    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
        for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
            TRmgMapItem* nearby = map.getMapItem(x, y, level);
            if (nearby->m_zoneState.m_zone == zoneIndex)
                nearby->clearBorderObject();
        }
    }
}

// Follow predecessors to the zero-cost seed without changing costs. After
// virtual addObject, recheck connection presence. Widened routes clear only
// borderObject in nearby same-zone cells; those neighbours are not route cells.
VA(0x005408E0, 0x23F)
MAC_ADDRESS(0x242e88, 0x380) // anchor-callee createGroundConnection; thiscall, ret 0x10
void type_random_map_generator::openConnectionPath(
    TRmgMapPosition position, b8 narrow)
{
    TRmgMapItem* item = m_map.getMapItem(position);
    int zone = item->m_zoneState.m_zone;
    if (item->m_movement.m_cost >= 30000)
        return;
    while (item->m_movement.m_cost > 0) {
        if (item->m_connection.m_present) {
            TRmgObjectPropertiesRef* properties = selectObjectPrototype(
                eTerrainDirt, BORDER_GUARD, item->m_connection.m_direction);
            type_object* object = new type_object(properties);
            item->clearBorderConnection();
            addObject(object, position);
        }
        item->openPath();
        TRmgMapPosition previous = item->m_previousTile;
        if (!narrow) {
            TRmgZoneBounds bounds;
            setRmgNeighborhoodBounds(bounds, position, m_map, 1);
            clearRmgZonePathBorders(m_map, bounds, position.m_z, zone);
        }
        position = previous;
        item = m_map.getMapItem(position);
    }
}

// Retail filters creature traits in reverse, then selects across all 145 slots,
// including for RoE. Its exclusion ends at 117 and the next predecrement starts
// eligibility at 116 without clearing slot 117. The two variation draws retain
// first-draw-minus-second-draw order.
VA(0x00540B20, 0x240)
MAC_ADDRESS(0x243208, 0x290) // anchor-callee 0x54203b; thiscall, ret 8; retail-only
type_object* type_random_map_generator::createGuard(int value, TRmgZone* zone)
{
    b8 allowedFactions[10];
    if (zone->m_templateZone->m_guardsMatchZone && zone->m_alignment != -1) {
        memset(allowedFactions, 0, sizeof(allowedFactions));
        allowedFactions[zone->m_alignment + 1] = true;
    } else {
        memcpy(allowedFactions, zone->m_templateZone->m_allowedMonsters, sizeof(allowedFactions));
    }
    int prototypeIndices[RMG_GUARD_CREATURE_COUNT];
    memset(prototypeIndices, -1, sizeof(prototypeIndices));
    for (unsigned int index = 0; index < m_objectPrototypes[MONSTER].size(); ++index) {
        TRmgObjectPropertiesRef* properties = m_objectPrototypes[MONSTER][index];
        prototypeIndices[properties->m_prototype->getSubtype()] = index;
    }
    int eligibleCreatureCount = 0;
    int creature = RMG_GUARD_CREATURE_COUNT;
    if (m_mapVersion < RMG_MAP_ARMAGEDDONS_BLADE) {
        while (--creature >= RMG_GUARD_ROE_EXCLUDED_FIRST)
            prototypeIndices[creature] = -1;
    }
    for (--creature; creature >= 0; --creature) {
        const TCreatureTypeTraits& traits = g_creatureTypeTraits[creature];
        if ((traits.m_wanderingHigh + traits.m_wanderingLow) / 2 * traits.m_aiValue <= value
            && value <= traits.m_aiValue * RMG_GUARD_MAXIMUM_COUNT
            && traits.m_level >= 0 && allowedFactions[traits.m_townType + 1]) {
            // RETAIL BUG: eligibility counts traits even without a loaded
            // prototype. Selection below only consumes actual prototypes.
            ++eligibleCreatureCount;
        } else {
            prototypeIndices[creature] = -1;
        }
    }
    if (!eligibleCreatureCount)
        return 0;
    int selectionRank = rand() % eligibleCreatureCount;
    for (creature = RMG_GUARD_CREATURE_COUNT - 1; creature >= 0; --creature) {
        if (prototypeIndices[creature] >= 0 && --selectionRank < 0)
            break;
    }
    // RETAIL BUG: if the eligibility/prototype counts disagree, creature can
    // reach -1. Preserve the unchecked lookup rather than changing selection.
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[MONSTER][prototypeIndices[creature]];
    int aiValue = g_creatureTypeTraits[creature].m_aiValue;
    int creatureCount = (value + aiValue / 2) / aiValue;
    int countVariation = creatureCount / 4 + 1;
    if (countVariation > 1) {
        creatureCount = creatureCount + (rand() % countVariation - rand() % countVariation);
    }
    return new rmgMonsterObject(properties, m_nextObjectId++, creatureCount);
}

// Locate the first prototype of a requested subtype in stored order. Preserve
// each caller's index type and read the live count on every iteration. Callers
// retain their distinct missing-prototype behavior, including unchecked use.
template<class Index>
static inline Index findRmgPrototypeSubtypeIndex(
    std::vector<TRmgObjectPropertiesRef*>& prototypes, int subtype)
{
    Index index = 0;
    while (index < prototypes.size()
        && prototypes[index]->m_prototype->getSubtype() != subtype)
        ++index;
    return index;
}

VA(0x00540D60, 0x256)
MAC_ADDRESS(0x24356c, 0x2b8) // anchor-callee createShipyardConnection; thiscall, ret 0x14
int type_random_map_generator::placeBorderObject(
    TRmgMapPosition position, int count, TRmgZone* zone)
{
    int color = m_nextKeyTentColor;
    int index = findRmgPrototypeSubtypeIndex<int>(m_objectPrototypes[BORDER_TENT], color);
    if (index == m_objectPrototypes[BORDER_TENT].size())
        return -1;
    TRmgObjectPropertiesRef* tentProperties = m_objectPrototypes[BORDER_TENT][index];

    index = findRmgPrototypeSubtypeIndex<int>(m_objectPrototypes[BORDER_GUARD], color);
    // RETAIL BUG: missing guard art returns color zero, which callers treat
    // as successful placement, unlike the -1 used for missing tent art.
    if (index == m_objectPrototypes[BORDER_GUARD].size())
        return 0;
    TRmgObjectPropertiesRef* guardProperties = m_objectPrototypes[BORDER_GUARD][index];
    type_object* tent = new type_object(tentProperties);
    if (!placeObjectInZone(tent, zone)) {
        delete tent;
        return -1;
    }

    for (index = 0; index < count; ++index) {
        type_object* guard = new type_object(guardProperties);
        TRmgMapItem* item = m_map.getMapItem(position);
        item->clearBorderConnection();
        addObject(guard, position);
        ++position.m_x;
    }

    setKeyTentColorDisabled(color, true);
    return color;
}

// Border-area and subterranean-gate marking share the same empty-cell
// policy. An existing connection keeps its tile flags but receives the new
// direction. Monolith marking intentionally has no empty-cell check and
// must remain separate.
static inline void markRmgBorderObjectCell(TRmgMapItem* item, int direction)
{
    if (item->m_objects.size() == 0) {
        item->markBorderConnection(direction);
    }
}

// A placed gate border extends to the empty cells west then east of its
// entrance. The caller's guard positions remain unshifted.
static inline void markRmgGateSideBorderCells(type_random_map& map,
    TRmgMapPosition entrance, int direction)
{
    --entrance.m_x;
    markRmgBorderObjectCell(map.getMapItem(entrance), direction);
    entrance.m_x += 2;
    markRmgBorderObjectCell(map.getMapItem(entrance), direction);
}

VA(0x00540FC0, 0x172)
MAC_ADDRESS(0x243824, 0x2d4) // anchor-callee createGroundConnection; thiscall, ret 0x10
void type_random_map_generator::markBorderObjectArea(
    TRmgMapPosition position, int direction)
{
    TRmgZoneBounds bounds;
    setRmgNeighborhoodBounds(bounds, position, m_map, 1);
    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
        for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
            TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
            markRmgBorderObjectCell(item, direction);
        }
    }
    TRmgMapPosition previous = m_map.getMapItem(position)->m_previousTile;
    if (m_map.containsXY(previous)) {
        TRmgMapItem* item = m_map.getMapItem(previous);
        item->clearBorderConnection();
    }
}

// Native ground (0x243c40), road (0x24c18c), object-score (0x24288c) and coast
// (0x24ced0) callers copy the full position and planar offset before addition.
// Preserve both by-value operands and the ordinary position constructor.
TRmgMapPosition operator+(TRmgMapPosition position, TPoint offset)
{
    return TRmgMapPosition(position.m_x + offset.m_x, position.m_y + offset.m_y, position.m_z);
}

// Preserve X-before-Y updates.
TRmgMapPosition& TRmgMapPosition::operator+=(const TPoint& offset)
{
    m_x += offset.m_x;
    m_y += offset.m_y;
    return *this;
}

TRmgMapPosition& TRmgMapPosition::operator-=(const TPoint& offset)
{
    m_x -= offset.m_x;
    m_y -= offset.m_y;
    return *this;
}

// Native 0x243498 retains this helper in ground, shipyard, gate, monolith and
// mine placement. Value precedes the copied position in the native interface.
MAC_ADDRESS(0x243498, 0xd4)
void type_random_map_generator::placeGuard(int value, TRmgMapPosition position)
{
    TRmgMapItem* item = m_map.getMapItem(position);
    TRmgZone* zone = m_zones[item->m_zoneState.m_zone];
    if (static_cast<int>(item->m_objects.size()) > 0)
        return;
    type_object* guard = createGuard(value, zone);
    if (guard)
        addObject(guard, position);
}

// A successful ground border placement marks its surrounding area and
// suppresses guards for this and all remaining crossings. The guard value
// belongs to the caller's whole connection, not just this endpoint.
static inline void placeRmgGroundConnectionBorder(type_random_map_generator& generator,
    TRmgMapPosition position, TRmgZone* keyTentZone, int& guardValue)
{
    int direction = generator.placeBorderObject(position, 1, keyTentZone);
    if (direction >= 0) {
        generator.markBorderObjectArea(position, direction);
        guardValue = 0;
    }
}

// Unguarded template connections bypass valuation entirely. Keep the native
// scalar value helper on the guarded path. Preserve the shipyard's consecutive
// strength-before-value snapshots, after the unguarded early return.
static inline int getRmgConnectionGuardValue(const TRmgZoneConnection* connection,
    const type_random_map_generator& generator)
{
    if (connection->m_unguarded)
        return 0;
    int strength = generator.m_monsterStrength;
    int value = connection->m_value;
    return getRmgGuardValue(value, strength);
}

// All equally cheap empty crossings are eligible. Open their predecessor
// paths and record both entrances before placing borders or guards. Native
// 0x243b24..0x243b38 captures both input slots before reading their zone indices.
VA(0x00541140, 0x63A)
MAC_ADDRESS(0x243af8, 0x53c) // anchor-callee ConnectZones 0x543550; retail-only
b8 type_random_map_generator::createGroundConnection(
    TRmgZone* source,
    TRmgZoneConnection* connection,
    std::vector<TRmgMapItem*>* borderItems,
    std::vector<TRmgMapPosition>* borderPositions)
{
    TRmgTemplateZone* sourceSlot = source->m_templateZone;
    TRmgTemplateZone* destinationSlot = connection->m_destination;
    int sourceZone = sourceSlot->m_zoneIndex;
    TRmgZone* destination = m_zones[destinationSlot->m_zoneIndex];
    int destinationZone = destination->m_templateZone->m_zoneIndex;
    if (source->getLevelPosition().m_z != destination->getLevelPosition().m_z)
        return false;
    if (source->m_terrain == eTerrainWater)
        return false;
    if (destination->m_terrain == eTerrainWater)
        return false;

    std::vector<TRmgMapPosition> candidates;
    int eligibleCount = 0;
    int bestCost = 100;
    for (int index = 0; index < borderItems->size(); ++index) {
        TRmgMapItem* item = (*borderItems)[index];
        if (item->m_zoneState.m_zone == sourceZone
            && item->m_zoneState.m_connectionEligibility == destinationZone
            && static_cast<int>(item->m_objects.size()) <= 0) {
            TRmgMapPosition other = (*borderPositions)[index]
                + g_rmgDirections[item->m_tileData.m_connectionDirection];
            if (static_cast<int>(m_map.getMapItem(other)->m_objects.size()) <= 0) {
                ++eligibleCount;
                int cost = item->m_movement.m_zonePathCost;
                if (cost <= bestCost) {
                    if (cost < bestCost) {
                        candidates.clear();
                        bestCost = cost;
                    }
                    candidates.push_back((*borderPositions)[index]);
                }
            }
        }
    }

    if (candidates.size() == 0)
        return false;

    int guardValue = getRmgConnectionGuardValue(connection, *this);

    if (bestCost == 1 && guardValue == 0 && !connection->m_placeBorderObjects)
        return true;

    int count = min(candidates.size(), (eligibleCount + 39) / 40);
    // Retail shares guardValue across crossings: either successful border
    // placement suppresses the current guard and all remaining crossings' guards.
    for (int crossing = 0; crossing < count; ++crossing) {
        int selected = rand() % candidates.size();
        TRmgMapPosition position = candidates[selected];
        TPoint direction = g_rmgDirections[
            m_map.getMapItem(position)->m_tileData.m_connectionDirection];
        TRmgMapPosition otherPosition = candidates[selected] + direction;

        openConnectionPath(candidates[selected], connection->m_placeBorderObjects);
        source->m_entrances.push_back(TPoint(position.m_x, position.m_y));
        openConnectionPath(otherPosition, connection->m_placeBorderObjects);
        destination->m_entrances.push_back(TPoint(otherPosition.m_x, otherPosition.m_y));
        candidates.erase(candidates.begin() + selected);

        if (connection->m_placeBorderObjects) {
            placeRmgGroundConnectionBorder(*this, position, destination, guardValue);
            placeRmgGroundConnectionBorder(*this, otherPosition, source, guardValue);
        }

        if (guardValue > 0) {
            if (!(rand() & 1)) {
                placeGuard(guardValue, position);
            } else {
                placeGuard(guardValue, otherPosition);
            }
        }
    }
    return true;
}

// Mark admitted cardinal neighbours visited, but enqueue only water.
// Path-clearance water cells remain traversable.
VA(0x00541780, 0x18D)
MAC_ADDRESS(0x244034, 0x254) // anchor-callee 0x541f1f; thiscall, ret 0x0c
void type_random_map_generator::floodConnectionRegion(TRmgMapPosition position)
{
    std::vector<TRmgMapPosition> openPositions;
    openPositions.push_back(position);
    m_map.getMapItem(position)->setConnectionVisited();
    while (openPositions.size()) {
        position = openPositions.back();
        openPositions.pop_back();
        for (int direction = 0; direction < 8; direction += 2) {
            TRmgMapPosition nearby = position + g_rmgDirections[direction];
            if (!m_map.containsXY(nearby))
                continue;
            TRmgMapItem* item = m_map.getMapItem(nearby);
            b8 visited = item->isConnectionVisited();
            if (visited)
                continue;
            if (!item->hasPathClearance()) {
                int terrain = item->getLandType();
                if (terrain == eTerrainWater)
                    continue;
            }
            item->setConnectionVisited();
            int terrain = item->getLandType();
            if (terrain == eTerrainWater)
                openPositions.push_back(nearby);
        }
    }
}

// Footprint X validity is a caller precondition: retail checks only y+1 here,
// and only X for the side offsets. Preserve ordered water/entrance/passability/
// rock queries. Native 0x2443cc uses the same copied-origin offset operation as
// both shipyard water scans.
static TRmgMapPosition getRmgShipyardWaterPosition(TRmgMapPosition shipyardPosition,
    int waterOffset);

VA(0x00541960, 0x16C)
MAC_ADDRESS(0x244288, 0x2f4) // anchor-callee 0x541c94; thiscall, ret 0x0c
b8 type_random_map_generator::canPlaceShipyard(TRmgMapPosition position)
{
    if (position.m_y + 1 >= m_map.m_mapHeight)
        return false;
    TRmgMapPosition nearby = position;
    for (nearby.m_y = position.m_y; nearby.m_y <= position.m_y + 1; ++nearby.m_y) {
        for (nearby.m_x = position.m_x - 2; nearby.m_x <= position.m_x; ++nearby.m_x) {
            TRmgMapItem* item = m_map.getMapItem(nearby);
            if (item->getLandType() == eTerrainWater)
                return false;
            b8 entrance = item->m_tileData.m_roadEntrance;
            if (entrance || !item->isPassableLand())
                return false;
        }
    }
    int waterOffset;
    for (waterOffset = 0; waterOffset < RMG_SHIPYARD_WATER_OFFSET_COUNT; ++waterOffset) {
        nearby = getRmgShipyardWaterPosition(position, waterOffset);
        if (nearby.m_x < 0 || nearby.m_x >= m_map.m_mapWidth)
            continue;
        TRmgMapItem* item = m_map.getMapItem(nearby);
        int terrain = item->getLandType();
        if (terrain == eTerrainWater && item->hasPathClearance())
            break;
    }
    if (waterOffset == RMG_SHIPYARD_WATER_OFFSET_COUNT)
        return false;
    nearby = position;
    if (g_rmgShipyardWaterOffsets[waterOffset].m_x < 0)
        ++nearby.m_x;
    else
        nearby.m_x -= 3;
    if (nearby.m_x < 0 || nearby.m_x >= m_map.m_mapWidth)
        return false;
    int terrain = m_map.getMapItem(nearby)->getLandType();
    return terrain != eTerrainWater;
}

static TRmgMapPosition getRmgShipyardWaterPosition(TRmgMapPosition shipyardPosition,
    int waterOffset)
{
    return shipyardPosition + g_rmgShipyardWaterOffsets[waterOffset];
}

// Native 0x24457c owns the water scan and flood. Its offset translation at
// Mac 0x2445b0 is the same copied-origin operation as canPlaceShipyard's scan.
MAC_ADDRESS(0x24457c, 0x130)
void type_random_map_generator::floodShipyardWater(type_object* shipyard)
{
    TRmgMapPosition shipyardPosition = shipyard->getPosition();
    TRmgMapPosition waterPosition;
    int waterOffset = 0;
    for (; waterOffset < RMG_SHIPYARD_WATER_OFFSET_COUNT; ++waterOffset) {
        waterPosition = getRmgShipyardWaterPosition(shipyardPosition, waterOffset);
        if (waterPosition.m_x >= 0 && waterPosition.m_x < m_map.getWidth()
            && m_map.getMapItem(waterPosition)->getLandType() == eTerrainWater)
            break;
    }
    if (waterOffset != RMG_SHIPYARD_WATER_OFFSET_COUNT)
        floodConnectionRegion(waterPosition);
}

// Keep one nearby coordinate through scanning and placement; the scan initializes
// only Z. Read the prototype vector after the random index is selected. Snapshot
// the trigger offset before updating nearby and retain an owned selected position.
// Border success clears guardValue and reaches the common exit.
VA(0x00541AD0, 0x5B0)
MAC_ADDRESS(0x2446ac, 0x55c) // anchor-callee connectZones; thiscall, ret 8; retail-only
b8 type_random_map_generator::createShipyardConnection(
    TRmgZone* source, TRmgZoneConnection* connection)
{
    TRmgTemplateZone* sourceSlot = source->m_templateZone;
    int sourceZone = sourceSlot->m_zoneIndex;
    TRmgTemplateZone* destinationSlot = connection->m_destination;
    TRmgZone* destination = m_zones[destinationSlot->m_zoneIndex];
    int destinationZone = destination->m_templateZone->m_zoneIndex;
    if (source->getLevelPosition().m_z != destination->getLevelPosition().m_z)
        return false;

    std::vector<TRmgMapPosition> candidates;
    int prototypeIndex = rand() % m_objectPrototypes[SHIPYARD].size();
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[SHIPYARD][prototypeIndex];
    TObjectType* prototype = properties->m_prototype;
    TRmgMapPosition nearby;
    {
        TRmgZoneBounds bounds = source->m_bounds;
        TRmgMapPosition position;
        position.m_z = source->getLevelPosition().m_z;
        for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY;
             ++position.m_y) {
            for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX;
                 ++position.m_x) {
                TRmgMapItem* item = m_map.getMapItem(position);
                if (item->m_zoneState.m_zone == sourceZone
                    && item->m_zoneState.m_connectionEligibility == destinationZone) {
                    b8 visited = item->m_tileData.m_connectionVisited;
                    if (visited)
                        return true;
                    if (item->getLandType() != eTerrainWater) {
                        nearby = position;
                        if (nearby.m_y + 1 < m_map.m_mapHeight) {
                            for (nearby.m_x = position.m_x;
                                 nearby.m_x <= position.m_x + 2; ++nearby.m_x) {
                                if (m_map.canPlaceObject(properties, nearby, source)
                                    && canPlaceShipyard(nearby))
                                    candidates.push_back(nearby);
                            }
                        }
                    }
                }
            }
        }
    }
    if (candidates.size() == 0)
        return false;

    rmgOwnableObject* shipyard = new rmgOwnableObject(properties);
    TRmgMapPosition position = candidates[rand() % candidates.size()];
    addObject(shipyard, position);

    nearby = getRmgObjectTriggerPosition(position, prototype->m_triggerCell);
    int entranceX = nearby.m_x;
    m_roadTargets.push_back(nearby);

    nearby = position;
    ++nearby.m_y;
    for (nearby.m_x = position.m_x - prototype->getWidth() + 1;
         nearby.m_x <= position.m_x; ++nearby.m_x) {
        TRmgMapItem* item = m_map.getMapItem(nearby);
        item->openPath();
        source->m_entrances.push_back(TPoint(nearby.m_x, nearby.m_y));
    }

    floodShipyardWater(shipyard);

    int guardValue = getRmgConnectionGuardValue(connection, *this);

    if (connection->m_placeBorderObjects) {
        nearby.m_x = entranceX - 1;
        if (placeBorderObject(nearby, 3, destination) >= 0)
            guardValue = 0;
    }
    if (guardValue > 0) {
        nearby = position + TPoint(0, 1);
        nearby.m_x = entranceX;
        placeGuard(guardValue, nearby);
    }
    return true;
}

// Retail reloads the source zone's position after rand(); do not reuse the
// earlier level snapshot. Preserve intersection-bound and coordinate lifetimes.
VA(0x00542080, 0x8AA)
MAC_ADDRESS(0x244c08, 0xa1c)
b8 type_random_map_generator::createSubterraneanGate(
    TRmgZone* source, TRmgZoneConnection* connection)
{
    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];
    int sourceZone = source->m_templateZone->m_zoneIndex;
    int destinationZone = destination->m_templateZone->m_zoneIndex;
    if (source->getLevelPosition().m_z == destination->getLevelPosition().m_z)
        return false;
    if (source->m_terrain == eTerrainWater)
        return false;

    TRmgZoneBounds sourceBounds = source->m_bounds;
    {
        TRmgZoneBounds destinationBounds = destination->m_bounds;
        sourceBounds.m_minimumX = max(
            sourceBounds.m_minimumX, destinationBounds.m_minimumX);
        sourceBounds.m_minimumY = max(
            sourceBounds.m_minimumY, destinationBounds.m_minimumY);
        sourceBounds.m_maximumX = min(
            sourceBounds.m_maximumX, destinationBounds.m_maximumX);
        sourceBounds.m_maximumY = min(
            sourceBounds.m_maximumY, destinationBounds.m_maximumY);
    }
    if (sourceBounds.m_minimumX >= sourceBounds.m_maximumX || sourceBounds.m_minimumY >= sourceBounds.m_maximumY)
        return false;

    int gateIndex = rand() % m_objectPrototypes[UNDERGROUND_GATE].size();
    TRmgObjectPropertiesRef* gateProperties = m_objectPrototypes[UNDERGROUND_GATE][gateIndex];
    TObjectType* gatePrototype = gateProperties->m_prototype;

    std::vector<TRmgMapPosition> candidates;
    int bestScore = 0;
    TRmgMapPosition position;
    position = source->getLevelPosition();

    for (position.m_y = sourceBounds.m_minimumY; position.m_y < sourceBounds.m_maximumY; ++position.m_y) {
        for (position.m_x = sourceBounds.m_minimumX; position.m_x < sourceBounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* sourceItem = m_map.getMapItem(position);
            if (sourceItem->m_zoneState.m_zone != sourceZone)
                continue;
            int score = sourceItem->m_zoneState.m_score;

            TRmgMapPosition otherPosition;
            otherPosition.m_x = position.m_x;
            otherPosition.m_y = position.m_y;
            otherPosition.m_z = destination->getLevelPosition().m_z;
            TRmgMapItem* destinationItem = m_map.getMapItem(otherPosition);
            if (destinationItem->m_zoneState.m_zone != destinationZone)
                continue;

            score += destinationItem->m_zoneState.m_score;
            if (score < bestScore)
                continue;
            if (!m_map.canPlaceObject(gateProperties, position, source))
                continue;
            if (!m_map.canPlaceObject(
                    gateProperties, otherPosition, destination))
                continue;

            if (score > bestScore) {
                candidates.clear();
                bestScore = score;
            }
            candidates.push_back(position);
        }
    }

    if (candidates.size() == 0)
        return false;

    position = candidates[rand() % candidates.size()];
    addObject(new type_object(gateProperties), position);

    TRmgMapPosition otherPosition = position;
    otherPosition.m_z = destination->getLevelPosition().m_z;
    addObject(new type_object(gateProperties), otherPosition);

    position = getRmgObjectTriggerPosition(position, gatePrototype->m_triggerCell);
    otherPosition = destination->getLevelPosition();
    otherPosition.m_x = position.m_x;
    otherPosition.m_y = position.m_y;
    source->m_entrances.push_back(TPoint(position.m_x, position.m_y));
    destination->m_entrances.push_back(
        TPoint(otherPosition.m_x, otherPosition.m_y));

    int guardValue = getRmgConnectionGuardValue(connection, *this);

    ++position.m_y;
    ++otherPosition.m_y;
    TRmgMapItem* sourceEntrance = m_map.getMapItem(position);
    sourceEntrance->openPath();
    TRmgMapItem* destinationEntrance = m_map.getMapItem(otherPosition);
    destinationEntrance->openPath();

    if (connection->m_placeBorderObjects) {
        // Retail shares guardValue across both entrances. Success on either
        // side suppresses both guards; failed placement does not undo objects.
        int direction = placeBorderObject(position, 1, destination);
        if (direction >= 0) {
            guardValue = 0;
            markRmgGateSideBorderCells(m_map, position, direction);
        }

        direction = placeBorderObject(otherPosition, 1, source);
        if (direction >= 0) {
            markRmgGateSideBorderCells(m_map, otherPosition, direction);
            return true;
        }
    }

    if (guardValue > 0) {
        placeGuard(guardValue, position);
        placeGuard(guardValue, otherPosition);
    }

    return true;
}

// Object positions name the lower-right footprint cell. Inset the minimum
// anchor coordinates so the whole footprint fits, keeping height before width.
static inline void insetRmgObjectPlacementBounds(
    TRmgZoneBounds& bounds, const TObjectType* prototype)
{
    bounds.m_minimumY += prototype->getHeight() - 1;
    bounds.m_minimumX += prototype->getWidth() - 1;
}

// Mac 0x245670..0x24568c snapshots the bounds, and 0x2456c0..0x2456d4 copies
// the whole position before consuming its level. Keep the aggregate accessors
// and assign the chosen candidate back before the virtual addObject call.
VA(0x00542930, 0x1C6)
MAC_ADDRESS(0x245624, 0x24c) // anchor-callee 0x540e81; thiscall, ret 8; retail-only
b8 type_random_map_generator::placeObjectInZone(type_object* object, TRmgZone* zone)
{
    TRmgObjectPropertiesRef* properties = object->m_properties;
    TObjectType* prototype = properties->m_prototype;
    std::vector<TRmgMapPosition> candidates;
    TRmgZoneBounds bounds = zone->getBounds();
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    insetRmgObjectPlacementBounds(bounds, prototype);
    TRmgMapPosition position;
    position = zone->getLevelPosition();
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            if (m_map.getMapItem(position)->m_zoneState.m_zone == zoneIndex
                && m_map.canPlaceObject(properties, position, zone))
                candidates.push_back(position);
        }
    }
    if (!candidates.size())
        return false;
    unsigned int selected = rand() % candidates.size();
    position = candidates[selected];
    addObject(object, position);
    return true;
}

// A positive-cost entrance follows its predecessor; a zero-cost entrance
// searches same-zone open neighbours, falling back to the cell directly below.
VA(0x00542B00, 0x1D2)
MAC_ADDRESS(0x245870, 0x364) // anchor-callers 0x542ec5/0x54304f; Complete-only, ret 0x10
b8 type_random_map_generator::placeMonolithBorder(
    TRmgMapPosition position, TRmgZone* zone)
{
    TPoint offsets[5] = {
        TPoint(0, 1), TPoint(1, 0), TPoint(-1, 0), TPoint(1, 1), TPoint(-1, 1)
    };
    TRmgMapPosition borderPosition;
    const int directionCount = sizeof(offsets) / sizeof(offsets[0]);
    buildZoneConnectionPaths();
    TRmgMapItem* item = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
    int zoneIndex = item->m_zoneState.m_zone;
    unsigned movementCost = item->m_movement.m_cost;
    if (movementCost >= 30000)
        return false;
    if (movementCost > 0) {
        borderPosition = item->m_previousTile;
    } else {
        int direction;
        for (direction = 0; direction < directionCount; ++direction) {
            borderPosition = position + offsets[direction];
            TRmgMapItem* nearby = m_map.getMapItem(borderPosition);
            if (nearby->m_zoneState.m_zone == zoneIndex
                && nearby->hasPathClearance())
                break;
        }
        if (direction == directionCount) {
            borderPosition = position + TPoint(0, 1);
        }
    }
    int border = placeBorderObject(borderPosition, 1, zone);
    if (border >= 0) {
        for (int direction = 0; direction < directionCount; ++direction) {
            TPoint offset = offsets[direction];
            TRmgMapPosition nearby = position + offset;
            TRmgMapItem* neighbor = m_map.getMapItem(nearby);
            neighbor->markBorderConnection(border);
        }
    }
    return border >= 0;
}

// Each portal placement owns its allocation until placement succeeds. Only
// successful objects enter the registry and zone entrance list; a failure
// leaves the remaining endpoint attempts to the caller.
static inline type_object* placeRmgMonolith(type_random_map_generator& generator,
    TRmgObjectPropertiesRef* properties, TRmgZone* zone, bool oneWay)
{
    type_object* object = new type_object(properties);
    if (!generator.placeObjectInZone(object, zone)) {
        delete object;
        return 0;
    }
    std::vector<type_object*>& monoliths =
        oneWay ? generator.m_monolithsOneWay : generator.m_monolithsTwoWay;
    monoliths.push_back(object);
    TPoint entrance;
    entrance.m_x = object->m_position.m_x;
    entrance.m_y = object->m_position.m_y;
    zone->m_entrances.push_back(entrance);
    return object;
}

// One-way prototypes produce an entrance/exit pair in each zone. Failed
// placement deletes only that object; subsequent endpoint attempts continue.
VA(0x00542CE0, 0x554)
MAC_ADDRESS(0x245bd4, 0x6e8) // anchor-caller 0x543240; Complete-only, thiscall ret 0xc
void type_random_map_generator::createMonolithConnection(
    TRmgZone* source, TRmgZoneConnection* connection, int prototypeIndex)
{
    TRmgObjectPropertiesRef* exitProperties = 0;
    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];
    if (source->m_terrain == eTerrainWater || destination->m_terrain == eTerrainWater)
        return;
    TRmgObjectPropertiesRef* properties;
    if (prototypeIndex < m_objectPrototypes[LITH_TWOWAY].size()) {
        properties = m_objectPrototypes[LITH_TWOWAY][prototypeIndex];
    } else {
        prototypeIndex -= m_objectPrototypes[LITH_TWOWAY].size();
        properties = m_objectPrototypes[LITH_ONEWAY_ENTRANCE][prototypeIndex];
        exitProperties = m_objectPrototypes[LITH_ONEWAY_EXIT][prototypeIndex];
    }
    int guardValue = getRmgConnectionGuardValue(connection, *this);

    type_object* object = placeRmgMonolith(*this, properties, source, exitProperties != 0);
    if (object) {
        if (connection->m_placeBorderObjects
            && placeMonolithBorder(object->getPosition(), destination)) {
            // This value also controls the destination's guard. Preserve
            // that suppression even when its own later border placement fails.
            guardValue = 0;
        } else if (guardValue > 0) {
            placeGuard(guardValue, object->getPosition() + TPoint(0, 1));
        }
    }
    object = placeRmgMonolith(*this, properties, destination, exitProperties != 0);
    if (object) {
        if (!connection->m_placeBorderObjects
            || !placeMonolithBorder(object->getPosition(), source)) {
            if (guardValue > 0)
                placeGuard(guardValue, object->getPosition() + TPoint(0, 1));
        }
    }
    if (exitProperties) {
        object = placeRmgMonolith(*this, exitProperties, source, true);
        object = placeRmgMonolith(*this, exitProperties, destination, true);
    }
}

b8 TRmgZoneConnection::isConnected() const
{
    return m_connected;
}

void TRmgZoneConnection::setConnected()
{
    m_connected = true;
}

// Land crossings and paired portals complete both directed records. Preserve
// forward-before-reverse updates; shipyards complete only their own direction.
static inline void completeRmgBidirectionalConnection(
    TRmgZoneConnection* connection, TRmgZoneConnection* oppositeConnection)
{
    connection->setConnected();
    oppositeConnection->setConnected();
}

static inline void clearRmgConnectionVisits(type_random_map& map, int level)
{
    TRmgMapItem* item = map.getMapItem(TRmgMapPosition(0, 0, level));
    for (int remaining = map.getWidth() * map.getHeight(); remaining--; ++item)
        item->m_tileData.m_connectionVisited = false;
}

// Try template connections first, then repair remaining non-water connections
// with shipyard reachability and paired monoliths. Each connection pass preserves
// its directional completion policy.
VA(0x00543240, 0x797)
MAC_ADDRESS(0x2462bc, 0x6f4)
void type_random_map_generator::connectZones()
{
    std::vector<TRmgMapItem*> borderItems;
    std::vector<TRmgMapPosition> borderPositions;

    TRmgMapItem* mapItem = m_map.m_mapItems;
    TRmgMapPosition position;
    for (position.m_z = 0; position.m_z < m_map.getNumberLevels(); ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.getHeight(); ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.getWidth();
                 ++position.m_x, ++mapItem) {
                if (mapItem->m_zoneState.m_connectionEligibility < 0)
                    continue;

                if (mapItem->getLandType() == eTerrainWater
                    || !mapItem->isPassableLand())
                    continue;

                int direction = mapItem->m_tileData.m_connectionDirection;
                TRmgMapItem* otherMapItem = m_map.getMapItem(
                    position + g_rmgDirections[direction]);
                if (otherMapItem->getLandType() != eTerrainWater
                    && otherMapItem->m_zoneState.m_zone
                           != mapItem->m_zoneState.m_zone) {
                    borderItems.push_back(mapItem);
                    borderPositions.push_back(position);
                }
            }
        }
    }

    int prototypeIndex = 0;

    // Retail constructs and destroys this empty work vector.  Its element
    // type and abandoned role are not recoverable from the optimized body.
    std::vector<TRmgMapPosition> connectionPositionsScratch;

    TRmgZone* zone;
    TRmgTemplateZone* zoneTemplate;
    TRmgZone* destination;
    TRmgZoneConnection* oppositeConnection;
    int connectionIndex;
    int zoneIndex;
    for (zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        zone = m_zones[zoneIndex];
        zoneTemplate = zone->m_templateZone;
        if (zone->getTerrain() == eTerrainWater)
            continue;

        TRmgMapPosition levelPosition;
        levelPosition = zone->getLevelPosition();
        clearRmgConnectionVisits(m_map, levelPosition.m_z);

        for (connectionIndex = 0;
             connectionIndex < zoneTemplate->m_connections.size();
             ++connectionIndex) {
            TRmgZoneConnection* connection =
                &zoneTemplate->m_connections[connectionIndex];
            if (connection->isConnected())
                continue;

            destination =
                m_zones[connection->m_destination->m_zoneIndex];
            oppositeConnection =
                destination->m_templateZone->findConnection(zoneIndex);

            if (createGroundConnection(
                    zone,
                    connection,
                    &borderItems,
                    &borderPositions)) {
                completeRmgBidirectionalConnection(connection, oppositeConnection);
                continue;
            }

            if (createShipyardConnection(zone, connection)) {
                connection->setConnected();
                continue;
            }

            if (destination->getTerrain() == eTerrainWater)
                continue;

            if (createSubterraneanGate(zone, connection)) {
                completeRmgBidirectionalConnection(connection, oppositeConnection);
            }
        }
    }

    for (zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        zone = m_zones[zoneIndex];
        zoneTemplate = zone->m_templateZone;
        if (zone->getTerrain() == eTerrainWater)
            continue;

        connectionIndex = 0;
        while (connectionIndex < zoneTemplate->m_connections.size()
               && zoneTemplate->m_connections[connectionIndex].isConnected())
            ++connectionIndex;
        if (connectionIndex == zoneTemplate->m_connections.size())
            continue;

        TRmgMapPosition levelPosition;
        levelPosition = zone->getLevelPosition();
        clearRmgConnectionVisits(m_map, levelPosition.m_z);

        for (int objectIndex = 0; objectIndex < m_objects.size(); ++objectIndex) {
            type_object* object = m_objects[objectIndex];
            if (object->m_properties->m_prototype->getObjectType() == SHIPYARD) {
                position = object->getPosition();
                if (m_map.getMapItem(position)->m_zoneState.m_zone == zoneIndex) {
                    floodShipyardWater(object);
                }
            }
        }

        for (;
             connectionIndex < zoneTemplate->m_connections.size();
             ++connectionIndex) {
            TRmgZoneConnection* connection =
                &zoneTemplate->m_connections[connectionIndex];
            if (connection->isConnected())
                continue;

            destination =
                m_zones[connection->m_destination->m_zoneIndex];
            oppositeConnection =
                destination->m_templateZone->findConnection(zoneIndex);

            if (createShipyardConnection(zone, connection)) {
                connection->setConnected();
                continue;
            }

            if (destination->getTerrain() == eTerrainWater)
                continue;

            createMonolithConnection(
                zone, connection, prototypeIndex);
            completeRmgBidirectionalConnection(connection, oppositeConnection);
            prototypeIndex = (prototypeIndex + 1)
                % (m_objectPrototypes[LITH_TWOWAY].size()
                   + m_objectPrototypes[LITH_ONEWAY_ENTRANCE].size());
        }
    }

    if (m_progress)
        m_progress->advance(0x1900);
}

// Underground rock and treasure-group filler may close passable floor only
// outside path and entrance reservations. Keep this ordered policy separate
// from each caller's terrain and object checks; it does not exclude water.
// File-local review cleanup, with no recovered original helper name claimed.
static inline bool canBlockRmgFloorCell(const TRmgMapItem* item)
{
    return !item->hasPathClearance() && item->isPassableLand()
        && !item->isRoadEntrance();
}

// One borrowed level-one map and brush span both scans; both progress
// updates precede their cleanup.
VA(0x005439E0, 0x283)
MAC_ADDRESS(0x246a34, 0x31c)
void type_random_map_generator::decorateUnderground()
{
    TRmgMapPosition scan;
    scan.m_z = 1;
    TRmgMapItem* item = m_map.getMapItem(0, 0, scan.m_z);
    TPoint dimensions(m_map.m_mapWidth, m_map.m_mapHeight);
    type_random_map map(item, dimensions.m_x, dimensions.m_y);
    TRmgTerrainBrush brush(&map, eTerrainRock, 4);
    for (scan.m_y = 0; scan.m_y < m_map.m_mapHeight; ++scan.m_y) {
        for (scan.m_x = 0; scan.m_x < m_map.m_mapWidth; ++scan.m_x, ++item) {
            if (canBlockRmgFloorCell(item))
                brush.paintRectangle(scan.m_x, scan.m_y, 1, 1);
        }
    }
    if (m_progress)
        m_progress->advance(1200);
    int currentTerrain = eTerrainRock;
    for (unsigned int zone = 0; zone < m_zones.size(); ++zone) {
        scan = m_zones[zone]->getLevelPosition();
        if (scan.m_z != 1)
            continue;
        TRmgZoneBounds bounds = m_zones[zone]->m_bounds;
        int terrain = m_zones[zone]->m_terrain;
        if (currentTerrain == eTerrainRock) {
            brush.changeTerrain(terrain, 4);
            currentTerrain = terrain;
        }
        for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
            for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                TRmgMapItem* item = m_map.getMapItem(x, y, 1);
                if (item->getLandType() == eTerrainRock
                    && item->m_zoneState.m_zone == zone
                    && (item->hasPathClearance() || item->m_objects.size())) {
                    if (terrain != currentTerrain) {
                        brush.changeTerrain(terrain, 4);
                        currentTerrain = terrain;
                    }
                    brush.paintRectangle(x, y, 1, 1);
                }
            }
        }
    }
    if (m_progress)
        m_progress->advance(1200);
}

// This Bresenham-style ray continues beyond toward until the map edge or
// a path-clearance tile in its 3x3 neighbourhood. The first two steps ignore
// neighbours; the returned point precedes the obstruction.
VA(0x00543C70, 0x1A2)
MAC_ADDRESS(0x246d50, 0x30c) // anchor-callee 0x544226; Complete-only, hidden result, ret 0x18
TPoint type_random_map::traceBranchEnd(TPoint from, TPoint toward, int level)
{
    int dx = toward.m_x - from.m_x;
    int dy = toward.m_y - from.m_y;
    int major;
    int minor;
    TRmgVector axial;
    TRmgVector diagonal;
    if (abs(dx) > abs(dy)) {
        major = abs(dx);
        minor = abs(dy);
        axial.m_y = 0;
        axial.m_x = dx > 0 ? 1 : -1;
        diagonal.m_x = axial.m_x;
        diagonal.m_y = dy > 0 ? 1 : -1;
    } else {
        major = abs(dy);
        minor = abs(dx);
        axial = TRmgVector(0, dy > 0 ? 1 : -1);
        diagonal = axial;
        diagonal.m_x = dx > 0 ? 1 : -1;
    }
    int error = major / 2;
    int steps = 0;
    for (;;) {
        toward = from;
        error += minor;
        ++steps;
        if (error < major)
            from += axial;
        else {
            error -= major;
            from += diagonal;
        }
        if (from.m_x < 1 || from.m_x >= m_mapWidth - 1
            || from.m_y < 1 || from.m_y >= m_mapHeight - 1)
            return toward;
        if (steps > 2) {
            TRmgMapPosition nearby;
            nearby.m_z = level;
            for (nearby.m_x = from.m_x - 1; nearby.m_x <= from.m_x + 1; ++nearby.m_x) {
                for (nearby.m_y = from.m_y - 1; nearby.m_y <= from.m_y + 1; ++nearby.m_y) {
                    if (getMapItem(nearby)->hasPathClearance())
                        return toward;
                }
            }
        }
    }
}

// Inferred generator-owned query: retail tests X before loading the map
// dimensions and forms the map receiver only for openPathPatch.
bool type_random_map_generator::contains(const TPoint& point) const
{
    return m_map.containsXY(point);
}

// Midpoint subdivision uses a LIFO vector; deferred perpendicular branches
// use a FIFO queue. Retail gives both containers one level's lifetime.
// Midpoint and perpendicular arithmetic identify their values as TPoint pairs.
VA(0x00543E20, 0x574)
MAC_ADDRESS(0x24705c, 0x750) // anchor-callee 0x544920; Complete-only, thiscall, no arguments
void type_random_map_generator::carveBranchingPaths()
{
    TRmgMapItem* item = m_map.m_mapItems;
    int remaining = m_map.getHeight() * m_map.getWidth() * m_map.m_numberLevels;
    for (; remaining--; ++item) {
        if (!item->m_objects.size()) {
            item->markBorderObject();
        } else {
            item->openPath();
        }
    }
    for (int level = 0; level < m_map.m_numberLevels; ++level) {
        TPoint first;
        TPoint last;
        switch (rand() % 4) {
        case RMG_BRANCH_SEED_MAIN_DIAGONAL:
            first.m_x = 0;
            first.m_y = 0;
            last.m_x = m_map.getWidth() - 1;
            last.m_y = m_map.getHeight() - 1;
            break;
        case RMG_BRANCH_SEED_VERTICAL:
            first.m_x = m_map.getWidth() / 2;
            first.m_y = 0;
            last.m_x = first.m_x;
            last.m_y = m_map.getHeight() - 1;
            break;
        case RMG_BRANCH_SEED_ANTI_DIAGONAL:
            first.m_x = m_map.getWidth() - 1;
            first.m_y = 0;
            last.m_x = 0;
            last.m_y = m_map.getHeight() - 1;
            break;
        case RMG_BRANCH_SEED_HORIZONTAL:
            first.m_x = 0;
            first.m_y = m_map.getHeight() / 2;
            last.m_x = m_map.getWidth() - 1;
            last.m_y = first.m_y;
            break;
        }
        std::vector<TPoint> pending;
        std::queue<TPoint, std::list<TPoint> > branches;
        pending.push_back(first);
        pending.push_back(last);
        while (pending.size()) {
            while (pending.size()) {
                last = pending.back();
                pending.pop_back();
                first = pending.back();
                pending.pop_back();
                TPoint middle = getRmgSubdivisionMidpoint(first, last);
                if (middle != first && middle != last) {
                    TRmgVector delta = last - first;
                    TRmgVector perpendicular(-delta.m_y, delta.m_x);
                    int length = perpendicular.length();
                    if (length > 1) {
                        int displacement = getRmgCenteredRandomOffset(length);
                        middle += perpendicular * displacement / length;
                    }
                    pending.push_back(last);
                    pending.push_back(middle);
                    pending.push_back(middle);
                    pending.push_back(first);
                    if (length >= 8 && contains(middle)) {
                        first = middle + perpendicular;
                        branches.push(middle);
                        branches.push(first);
                        first = TPoint(middle.m_x - perpendicular.m_x, middle.m_y - perpendicular.m_y);
                        branches.push(middle);
                        branches.push(first);
                    }
                } else if (contains(first)) {
                    m_map.openPathPatch(first.m_x, first.m_y, level);
                }
            }
            while (branches.size() > 0 && pending.empty()) {
                first = branches.front();
                branches.pop();
                last = branches.front();
                branches.pop();
                last = m_map.traceBranchEnd(first, last, level);
                if (getRmgSquaredDistance(last, first) >= 25) {
                    pending.push_back(last);
                    pending.push_back(first);
                }
            }
        }
    }
    item = m_map.m_mapItems;
    TRmgMapPosition position;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (item->getLandType() == eTerrainWater || item->getLandType() == eTerrainRock) {
                    item->openPath();
                }
                if (item->hasBorderObject())
                    m_map.markBorderPatch(position);
            }
        }
    }
}

VA(0x005443A0, 0x2F5)
MAC_ADDRESS(0x247800, 0x59c)
void type_random_map_generator::connectJunctionEntrance(TPoint from, TPoint to,
    TRmgZone* zone)
{
    std::vector<TPoint> pending;
    TRmgMapPosition position = zone->getLevelPosition();
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    int roughness = zone->m_boundaryRoughness;
    pending.push_back(to);
    while (pending.size() > 0) {
        to = pending.back();
        pending.pop_back();
        TPoint midpoint = getRmgSubdivisionMidpoint(from, to);
        if (midpoint != from && midpoint != to) {
            displaceRmgBoundaryMidpoint(midpoint, from, to, roughness, 1);
            pending.push_back(to);
            pending.push_back(midpoint);
        } else {
            TPoint clamped = clampRmgBoundaryToMap(from, m_map);
            long x = clamped.m_x;
            long y = clamped.m_y;
            TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
            if (item->m_zoneState.m_zone == zoneIndex) {
                item->openPath();
                TRmgZoneBounds bounds;
                bounds.m_minimumX = cppMax<long>(x - 1, 0);
                bounds.m_minimumY = cppMax<long>(y - 1, 0);
                bounds.m_maximumX = cppMin<long>(x + 2, m_map.m_mapWidth);
                bounds.m_maximumY = cppMin<long>(y + 2, m_map.m_mapHeight);
                clearRmgZonePathBorders(m_map, bounds, position.m_z, zoneIndex);
            }
            from = to;
        }
    }
}

// Retail retains both floodConnectionCosts calls and the point-pair helper.
// The default position constructor is empty; reset sentinels require all
// three coordinates to be initialized explicitly.
VA(0x005446A0, 0x27E)
MAC_ADDRESS(0x247d9c, 0x3e0)
void type_random_map_generator::prepareJunctionZone(TRmgZone* zone)
{
    TRmgZoneBounds bounds = zone->m_bounds;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition position = zone->getLevelPosition();
    int level = position.m_z;
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position.m_x, position.m_y, level);
            if (item->m_zoneState.m_zone == zoneIndex
                && item->getLandType() != eTerrainWater) {
                TRmgMapPosition previous;
                previous.m_x = -1;
                previous.m_y = -1;
                previous.m_z = -1;
                item->resetMovement(previous);
                if (static_cast<int>(item->m_objects.size()) <= 0) {
                    item->markBorderObject();
                }
            }
        }
    }
    if (!zone->m_entrances.size())
        return;
    TRmgMapPosition first;
    first.m_x = zone->m_entrances[0].m_x;
    first.m_y = zone->m_entrances[0].m_y;
    first.m_z = level;
    TRmgMapItem* item = m_map.getMapItem(first.m_x, first.m_y, first.m_z);
    item->setMovementCost(0, TRmgMapPosition(-1, -1, -1));
    m_map.floodConnectionCosts(first, false);
    for (int entrance = 1; entrance < static_cast<int>(zone->m_entrances.size()); ++entrance) {
        TPoint from = zone->m_entrances[entrance];
        item = m_map.getMapItem(from.m_x, from.m_y, level);
        unsigned int cost = item->m_movement.m_cost;
        if (!cost || cost > 30000)
            continue;
        TRmgMapPosition previous;
        previous.m_x = from.m_x;
        previous.m_y = from.m_y;
        previous.m_z = level;
        while (item->m_movement.m_cost > 0) {
            previous = item->m_previousTile;
            item = m_map.getMapItem(previous.m_x, previous.m_y, previous.m_z);
        }
        connectJunctionEntrance(from, TPoint(previous.m_x, previous.m_y), zone);
        TRmgMapPosition next;
        next.m_x = from.m_x;
        next.m_y = from.m_y;
        next.m_z = level;
        m_map.floodConnectionCosts(next, false);
    }
}

VA(0x00544920, 0x124)
MAC_ADDRESS(0x24817c, 0x1ac)
void type_random_map_generator::prepareZoneConnections()
{
    carveBranchingPaths();
    expandObstacleClearance();
    TRmgMapItem* item = m_map.m_mapItems;
    TRmgMapPosition position;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (!item->hasBorderObject() && item->isPassableLand() && !item->isRoadEntrance()
                    && static_cast<int>(item->m_objects.size()) <= 0
                    && item->m_zoneState.m_zone < 0 && item->getLandType() != eTerrainWater)
                    m_map.markBorderPatch(position);
            }
        }
    }
    for (unsigned int zone = 0; zone < m_zones.size(); ++zone)
        prepareWaterZoneConnections(m_zones[zone]);
    buildZoneConnectionPaths();
    repairWaterZoneBorders();
    connectZones();
}

VA(0x00544A50, 0x90)
MAC_ADDRESS(0x248328, 0xf0)
void type_random_map_generator::placePrimaryTown(TRmgZone* zone)
{
    TRmgTemplateZone* slot = zone->m_templateZone;
    int alignment = zone->m_alignment;
    int player = m_playerIndexMap[slot->m_playerIndex + 1];
    if (slot->m_townPlacement[RMG_TOWN_PLAYER_OPTION_COUNT] > 0
        && tryPlacePrimaryTown(zone, alignment, player, 1))
        return;
    if (slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT] > 0
        && tryPlacePrimaryTown(zone, alignment, player, 0))
        return;
    if (slot->m_townPlacement[RMG_TOWN_NEUTRAL_OPTION_COUNT] > 0
        && tryPlacePrimaryTown(zone, alignment, -1, 1))
        return;
    if (slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT] > 0)
        tryPlacePrimaryTown(zone, alignment, -1, 0);
}

// Disabled categories leave both accumulators untouched.
static inline void initializeRmgDensityCategories(const int* densities,
    int categoryCount, b8* finished, int& totalDensity, int& densityProduct)
{
    for (int category = 0; category < categoryCount; ++category) {
        int density = densities[category];
        if (density <= 0) {
            finished[category] = true;
        } else {
            totalDensity += density;
            finished[category] = false;
            densityProduct *= density;
        }
    }
}

// Density is expressed per reference area. Preserve integer division before
// the square root and truncate only the resulting distance.
static inline int getRmgDensitySpacing(int referenceArea, int density)
{
    return static_cast<int>(sqrt(static_cast<double>(referenceArea / density)));
}

// Counts may alias weightedCounts; consume each input before assigning its
// output. Disabled entries stay untouched.
static inline void initializeRmgCategoryStrides(const int* densities,
    const int* counts, int categoryCount, int densityProduct,
    int* countSteps, int* weightedCounts)
{
    for (int category = 0; category < categoryCount; ++category) {
        if (densities[category] > 0) {
            countSteps[category] = densityProduct / densities[category];
            weightedCounts[category] = counts[category] * countSteps[category];
        }
    }
}

// Strict comparison preserves the first category on ties; callers advance
// by densityProduct / density with retail's integer product and rounding.
static inline int selectRmgWeightedCategory(const b8* finished,
    const int* weightedCounts, int categoryCount)
{
    int selected = -1;
    int lowest = 0;
    for (int category = 0; category < categoryCount; ++category) {
        if (!finished[category] && (selected == -1 || weightedCounts[category] < lowest)) {
            lowest = weightedCounts[category];
            selected = category;
        }
    }
    return selected;
}

// Only the first enabled category accounts for the already placed primary
// town. Keep count as a reference because placement can call virtual object
// insertion; each loop condition must still read the live template field.
static inline void placeRmgFixedTownCategory(type_random_map_generator* generator,
    TRmgZone* zone, const int& count, int alignment, int player,
    unsigned char townOption, b8& skipPrimary)
{
    if (count <= 0)
        return;
    int townIndex = skipPrimary ? 1 : 0;
    for (; townIndex < count; ++townIndex)
        generator->tryPlaceAdditionalTown(zone, alignment, player, townOption, 0);
    skipPrimary = false;
}

VA(0x00544AE0, 0x2B0)
MAC_ADDRESS(0x248418, 0x414)
void type_random_map_generator::placeAdditionalTowns(TRmgZone* zone)
{
    TRmgTemplateZone* slot = zone->m_templateZone;
    int alignment = zone->m_alignment;
    int player = m_playerIndexMap[slot->m_playerIndex + 1];
    b8 skipPrimary = true;
    placeRmgFixedTownCategory(this, zone,
        slot->m_townPlacement[RMG_TOWN_PLAYER_OPTION_COUNT], alignment, player, 1, skipPrimary);
    placeRmgFixedTownCategory(this, zone,
        slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT], alignment, player, 0, skipPrimary);
    placeRmgFixedTownCategory(this, zone,
        slot->m_townPlacement[RMG_TOWN_NEUTRAL_OPTION_COUNT], alignment, -1, 1, skipPrimary);
    placeRmgFixedTownCategory(this, zone,
        slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT], alignment, -1, 0, skipPrimary);
    int totalDensity = 0;
    int densityProduct = 1;
    int densities[4] = { slot->m_townPlacement[RMG_TOWN_PLAYER_OPTION_DENSITY], slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_DENSITY],
        slot->m_townPlacement[RMG_TOWN_NEUTRAL_OPTION_DENSITY], slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_DENSITY] };
    int weightedCounts[4] = { slot->m_townPlacement[RMG_TOWN_PLAYER_OPTION_COUNT], slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT],
        slot->m_townPlacement[RMG_TOWN_NEUTRAL_OPTION_COUNT], slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT] };
    int countSteps[4];
    b8 finished[4];
    initializeRmgDensityCategories(densities, 4, finished, totalDensity, densityProduct);
    if (!totalDensity)
        return;
    int spacing = getRmgDensitySpacing(82944, totalDensity);
    initializeRmgCategoryStrides(densities, weightedCounts, 4,
        densityProduct, countSteps, weightedCounts);
    for (;;) {
        int selected = selectRmgWeightedCategory(finished, weightedCounts, 4);
        if (selected == -1)
            break;
        weightedCounts[selected] += countSteps[selected];
        switch (selected) {
        case RMG_TOWN_PLAYER_OPTION:
            if (!tryPlaceAdditionalTown(zone, alignment, player, 1, spacing))
                finished[RMG_TOWN_PLAYER_OPTION] = true;
            break;
        case RMG_TOWN_PLAYER_BASIC:
            if (!tryPlaceAdditionalTown(zone, alignment, player, 0, spacing))
                finished[RMG_TOWN_PLAYER_BASIC] = true;
            break;
        case RMG_TOWN_NEUTRAL_OPTION:
            if (!tryPlaceAdditionalTown(zone, alignment, -1, 1, spacing))
                finished[RMG_TOWN_NEUTRAL_OPTION] = true;
            break;
        case RMG_TOWN_NEUTRAL_BASIC:
            if (!tryPlaceAdditionalTown(zone, alignment, -1, 0, spacing))
                finished[RMG_TOWN_NEUTRAL_BASIC] = true;
            break;
        }
    }
}

// Read a live cell score after placement validation; town and mine callers
// instead pass their existing score snapshots.
static inline void addRmgSpacingCandidate(
    std::vector<TRmgMapPosition>& candidates, const TRmgMapPosition& position,
    int score, int& spacing)
{
    if (score > spacing) {
        spacing = score;
        candidates.clear();
    }
    candidates.push_back(position);
}

// Keep the caller's position storage and append the trigger before stepping
// to its southern approach.
static inline void registerRmgTownRoadEntrance(
    type_random_map_generator* generator, TRmgMapPosition& entrance)
{
    generator->m_roadTargets.push_back(entrance);
    ++entrance.m_y;
    TRmgMapItem* item = generator->m_map.getMapItem(entrance);
    item->openPath();
}

// The trigger's entire clipped 3x3 neighbourhood must remain in the zone.
// Keep its saved trigger coordinates across placement callbacks.
VA(0x00544D90, 0x4B7)
MAC_ADDRESS(0x24882c, 0x5d4)
b8 type_random_map_generator::tryPlaceAdditionalTown(TRmgZone* zone,
    int alignment, int player, unsigned char townOption, int spacing)
{
    TRmgTemplateZone* slot = zone->m_templateZone;
    if ((player == -1 && !slot->m_neutralTownsMatchZone) || alignment == -1) {
        alignment = slot->selectAllowedTown();
        if (alignment == -1)
            alignment = rand() % (8 + (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE));
    }
    if (!zone->m_active)
        return tryPlacePrimaryTown(zone, alignment, player, townOption);

    std::vector<TRmgMapPosition> candidates;
    int zoneIndex = slot->m_zoneIndex;
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[TOWN][alignment];
    TObjectType* prototype = properties->m_prototype;
    TObjectType::TPoint trigger = prototype->m_triggerCell;
    TRmgMapPosition position = zone->getLevelPosition();
    TRmgZoneBounds bounds = zone->m_bounds;
    bounds.m_minimumY += prototype->getHeight();
    bounds.m_minimumX += prototype->getWidth();
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, trigger);
            TRmgMapItem* item = m_map.getMapItem(entrance.m_x, entrance.m_y, entrance.m_z);
            if (item->m_zoneState.m_zone != zoneIndex)
                continue;
            int score = item->m_zoneState.m_score;
            if (score < spacing || !m_map.canPlaceObject(properties, position, zone))
                continue;
            TRmgZoneBounds nearby;
            setRmgNeighborhoodBounds(nearby, entrance, m_map, 1);
            b8 valid = true;
            for (int y = nearby.m_minimumY; y < nearby.m_maximumY; ++y) {
                for (int x = nearby.m_minimumX; x < nearby.m_maximumX; ++x) {
                    int otherZone = m_map.getMapItem(x, y, entrance.m_z)->m_zoneState.m_zone;
                    if (otherZone < 0 || otherZone != zoneIndex)
                        valid = false;
                }
            }
            if (!valid)
                continue;
            addRmgSpacingCandidate(candidates, position, score, spacing);
        }
    }
    if (!candidates.size())
        return false;
    rmgTownObject* object = new rmgTownObject(properties, m_nextObjectId++, player, townOption);
    position = candidates[rand() % candidates.size()];
    addObject(object, position);
    TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, trigger);
    registerRmgTownRoadEntrance(this, entrance);
    return true;
}

// Caller 0x544a50 proves four stack arguments and byte success; retail
// constructs a 0x28-byte town object before updating the zone.
VA(0x00545250, 0x324)
MAC_ADDRESS(0x248e00, 0x3b4)
b8 type_random_map_generator::tryPlacePrimaryTown(
    TRmgZone* zone, int alignment, int player, unsigned char townOption)
{
    if (alignment == -1)
        return false;
    std::vector<TRmgMapPosition> candidates;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition position = zone->m_levelPosition;
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[TOWN][alignment];
    TObjectType* prototype = properties->m_prototype;
    int bestDistance = 32000;
    TRmgMapPosition nearby;
    nearby.m_z = position.m_z;
    TRmgZoneBounds bounds = zone->m_bounds;
    for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
        for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
            if (m_map.getMapItem(nearby)->m_zoneState.m_zone != zoneIndex)
                continue;
            int distance = getRmgSquaredDistance(nearby, position);
            if (distance <= bestDistance && m_map.canPlaceObject(properties, nearby, zone)) {
                if (distance < bestDistance) {
                    bestDistance = distance;
                    candidates.clear();
                }
                candidates.push_back(nearby);
            }
        }
    }
    if (!candidates.size())
        return false;
    rmgTownObject* town = new rmgTownObject(properties, m_nextObjectId++, player, townOption);
    unsigned int selected = rand() % candidates.size();
    position = candidates[selected];
    addObject(town, position);
    position = getRmgObjectTriggerPosition(position, prototype->m_triggerCell);
    zone->m_position = position;
    zone->m_active = true;
    registerRmgTownRoadEntrance(this, position);
    return true;
}

// Keep the distance, border-count and spacing filters in that order.
VA(0x00545580, 0x401)
MAC_ADDRESS(0x249214, 0x46c)
b8 type_random_map_generator::placeMineSite(type_object* object,
    TRmgZone* zone, b8 startingMine, int spacing)
{
    TRmgObjectPropertiesRef* properties = object->m_properties;
    TObjectType* prototype = properties->m_prototype;
    std::vector<TRmgMapPosition> candidates;
    TRmgZoneBounds bounds = zone->m_bounds;
    int bestBorderCount = 0;
    int bestDistance = 40000;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition townPosition;
    if (startingMine) {
        townPosition = zone->m_position;
        townPosition += TPoint(prototype->m_triggerCell.m_x, prototype->m_triggerCell.m_y);
    }
    insetRmgObjectPlacementBounds(bounds, prototype);
    TRmgMapPosition position = zone->m_levelPosition;
    properties->buildOutline();
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position);
            if (item->m_zoneState.m_zone != zoneIndex || !m_map.canPlaceObject(properties, position, zone))
                continue;
            if (startingMine) {
                int distance = getRmgSquaredDistance(position, townPosition);
                if (distance > bestDistance || distance < 16)
                    continue;
                if (distance < 144)
                    distance = 144;
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestBorderCount = 0;
                    spacing = 0;
                    candidates.clear();
                }
            }
            int score = item->m_zoneState.m_score;
            if (score < spacing)
                continue;
            int borderCount = 0;
            for (unsigned int i = 0; i < properties->m_outline.size(); ++i) {
                TPoint offset = properties->m_outline[i];
                int x = position.m_x + offset.m_x;
                int y = position.m_y + offset.m_y;
                if (x < 0 || x >= m_map.m_mapWidth || y < 0 || y >= m_map.m_mapHeight || y > position.m_y)
                    continue;
                TRmgMapItem* nearby = m_map.getMapItem(x, y, position.m_z);
                if (nearby->isPassableLand() && nearby->hasBorderObject())
                    ++borderCount;
            }
            if (borderCount > 5)
                borderCount = 5;
            if (borderCount < bestBorderCount)
                continue;
            if (borderCount > bestBorderCount) {
                candidates.clear();
                bestBorderCount = borderCount;
            }
            addRmgSpacingCandidate(candidates, position, score, spacing);
        }
    }
    if (!candidates.size())
        return false;
    unsigned int selected = rand() % candidates.size();
    position = candidates[selected];
    addObject(object, position);
    return true;
}

// Combine template and request difficulty for guarded mines and treasure.
// Keep the upper-bound-first clamp; zero template strength is a separate
// caller policy and must not be converted into an enabled guard here.
static inline int getRmgCombinedMonsterStrength(int zoneStrength, int mapStrength)
{
    int strength = zoneStrength + mapStrength - 3;
    if (strength > 5) strength = 5;
    else if (strength < 0) strength = 0;
    return strength;
}

// Inferred mine-valuation boundary; retail 0x545b76 retains the scalar guard
// curve after resource pricing, local enablement and difficulty combination.
int type_random_map_generator::getMineGuardValue(int resource, const TRmgZone* zone) const
{
    int value;
    switch (resource) {
    case WOOD: case ORE: value = 1500; break;
    case GOLD: value = 7000; break;
    default: value = 3500; break;
    }
    int localStrength = zone->m_templateZone->m_monsterStrength;
    if (!localStrength)
        return 0;
    int strength = getRmgCombinedMonsterStrength(localStrength, m_monsterStrength);
    return getRmgGuardValue(value, strength);
}

// Retail keeps the last scanned prototype at 0x5459f5/0x545a5d and reloads
// it at 0x545b7e/0x545ca9 after random selection. Its trigger/width quirk
// is intentional here. The non-const terrain query uses the checked bitset
// proxy; the later resource prototype has a separate lifetime.
VA(0x00545990, 0x466)
MAC_ADDRESS(0x249680, 0x5ac)
b8 type_random_map_generator::tryPlaceMine(TRmgZone* zone,
    int resource, b8 startingMine, int spacing)
{
    std::vector<TRmgObjectPropertiesRef*> candidates;
    int terrain = zone->m_terrain;
    TRmgObjectPropertiesRef* properties;
    TObjectType* lastScannedPrototype;
    for (unsigned i = 0; i < m_objectPrototypes[MINE].size(); ++i) {
        properties = m_objectPrototypes[MINE][i];
        lastScannedPrototype = properties->m_prototype;
        if (lastScannedPrototype->getSubtype() == resource
            && lastScannedPrototype->isRecommendedTerrain(terrain))
            candidates.push_back(properties);
    }
    if (!candidates.size()) {
        for (unsigned i = 0; i < m_objectPrototypes[MINE].size(); ++i) {
            properties = m_objectPrototypes[MINE][i];
            lastScannedPrototype = properties->m_prototype;
            if (lastScannedPrototype->getSubtype() == resource)
                candidates.push_back(properties);
        }
    }
    if (!candidates.size())
        return false;
    unsigned int selected = rand() % candidates.size();
    properties = candidates[selected];
    rmgOwnableObject* mine = new rmgOwnableObject(properties);
    if (!placeMineSite(mine, zone, startingMine, spacing)) {
        delete mine;
        return false;
    }
    int guardValue = getMineGuardValue(resource, zone);
    TRmgMapPosition entrance = getRmgObjectTriggerPosition(
        mine->getPosition(), lastScannedPrototype->m_triggerCell);
    ++entrance.m_y;
    TRmgMapItem* item = m_map.getMapItem(entrance);
    item->openPath();
    if (guardValue > 0)
        placeGuard(guardValue, entrance);
    int placed = 0;
    TRmgObjectPropertiesRef* resourceProperties = selectObjectPrototype(terrain, RESOURCE, resource);
    if (!resourceProperties)
        return true;
    TRmgMapPosition position = mine->getPosition();
    TRmgZoneBounds bounds;
    bounds.m_minimumY = max(position.m_y + 1, 0);
    bounds.m_maximumY = min(position.m_y + 2, m_map.getHeight());
    bounds.m_minimumX = max(position.m_x - lastScannedPrototype->getWidth(), 0);
    bounds.m_maximumX = min(position.m_x + 2, m_map.getWidth());
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX && placed <= 2; ++position.m_x) {
            if (rand() % 2 == 0 && m_map.canPlaceObject(resourceProperties, position, zone)) {
                ++placed;
                addObject(new rmgResourceObject(resourceProperties), position);
            }
        }
    }
    return true;
}

VA(0x00545E00, 0x5B)
MAC_ADDRESS(0x22ce7c, 0x74)
int getRmgGuardValue(int value, int strength)
{
    int guardValue = 0;
    if (value > g_rmgGuardThresholdLow[strength]) {
        guardValue = (value - g_rmgGuardThresholdLow[strength])
            * g_rmgGuardScaleLow[strength] / 4;
    }
    if (value > g_rmgGuardThresholdHigh[strength]) {
        guardValue += (value - g_rmgGuardThresholdHigh[strength])
            * g_rmgGuardScaleHigh[strength] / 4;
    }
    return guardValue < 2000 ? 0 : guardValue;
}

VA(0x00545E60, 0xFA)
MAC_ADDRESS(0x249c8c, 0x1c4)
void type_random_map_generator::placeExtraMines(TRmgZone* zone)
{
    TRmgTemplateZone* slot = zone->m_templateZone;
    b8 finished[7];
    int totalDensity = 0;
    int densityProduct = 1;
    initializeRmgDensityCategories(slot->m_mineDensities, 7,
        finished, totalDensity, densityProduct);
    if (!totalDensity)
        return;
    int spacing = getRmgDensitySpacing(82944, totalDensity);
    int countSteps[7];
    int weightedCounts[7];
    initializeRmgCategoryStrides(slot->m_mineDensities, slot->m_mineCounts, 7,
        densityProduct, countSteps, weightedCounts);
    for (;;) {
        int selected = selectRmgWeightedCategory(finished, weightedCounts, 7);
        if (selected == -1)
            break;
        weightedCounts[selected] += countSteps[selected];
        if (!tryPlaceMine(zone, selected, false, spacing))
            finished[selected] = true;
    }
}

VA(0x00545F60, 0xD6)
MAC_ADDRESS(0x249e50, 0x110)
void type_random_map_generator::placeMines()
{
    for (unsigned int index = 0; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        TRmgTemplateZone* slot = zone->m_templateZone;
        for (int resource = 0; resource <= GOLD; ++resource) {
            b8 startingMine = false;
            if ((resource == WOOD || resource == ORE)
                && (slot->m_kind == RMG_TEMPLATE_HUMAN || slot->m_kind == RMG_TEMPLATE_COMPUTER)
                && zone->m_active)
                startingMine = true;
            for (int mine = 0; mine < slot->m_mineCounts[resource]; ++mine) {
                if (!tryPlaceMine(zone, resource, startingMine, 0))
                    break;
                startingMine = false;
            }
        }
        placeExtraMines(zone);
    }
    if (m_progress)
        m_progress->advance(3900);
}

// Retail openConnectionPath calls this ordinary selector at 0x540954.
// Its const terrain query uses the checked bitset::test at Mac 0x24a004.
VA(0x00546040, 0x141)
MAC_ADDRESS(0x249f60, 0x194) // anchor-callee openConnectionPath; thiscall, ret 0x0c
TRmgObjectPropertiesRef* type_random_map_generator::selectObjectPrototype(
    int terrain, int objectType, int subtype)
{
    std::vector<TRmgObjectPropertiesRef*> candidates;
    for (unsigned int index = 0; index < m_objectPrototypes[objectType].size(); ++index) {
        TRmgObjectPropertiesRef* properties = m_objectPrototypes[objectType][index];
        const TObjectType* prototype = properties->m_prototype;
        if (prototype->getSubtype() != subtype)
            continue;
        if (prototype->m_slotCategory == TObjectType::SLOT_CATEGORY_4
            || prototype->m_slotCategory == TObjectType::SLOT_CATEGORY_5) {
            if (terrain == eTerrainWater)
                continue;
        } else if (!prototype->isRecommendedTerrain(terrain)) {
            continue;
        }
        candidates.push_back(properties);
    }
    if (!candidates.size())
        return 0;
    return candidates[rand() % candidates.size()];
}

// Keep candidate order and each prototype-selection random draw. The
// selected definition's virtual value query runs again before generation.
VA(0x00546190, 0x385)
MAC_ADDRESS(0x24a190, 0x3f4) // anchor-callee 0x546572/0x546663; thiscall, ret 0x28
type_object* type_random_map_generator::createTreasureObject(TRmgZone* zone,
    int minimum, int maximum, int* value, b8 primary,
    b8 allowTerrainDependent, b8 compact,
    TRmgMapPosition position)
{
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    int totalWeight = 0;
    std::vector<type_treasure_def*> candidates;
    std::vector<TRmgObjectPropertiesRef*> properties;
    int bestValuePerCell = 0;
    for (unsigned int index = 0; index < m_objectGenerators.size(); ++index) {
        type_treasure_def* definition = m_objectGenerators[index];
        int objectType = definition->m_objectType;
        if (!primary && g_adventureObjectTraits[objectType].m_blocksLanding
            && !g_adventureObjectTraits[objectType].m_trait2)
            continue;
        if (!allowTerrainDependent && definition->isTerrainDependent())
            continue;
        if (m_objectCountByType[objectType] >= g_rmgMapObjectLimits[objectType])
            continue;
        if (zone->m_objectCountByType[objectType] >= g_rmgZoneObjectLimits[objectType])
            continue;
        int objectValue = definition->getValue(zone, this);
        if (objectValue < 0 || objectValue < minimum || objectValue > maximum)
            continue;
        TRmgObjectPropertiesRef* candidate = selectObjectPrototype(
            zone->m_terrain, definition->m_objectType, definition->m_subtype);
        if (!candidate)
            continue;
        if (position.m_x >= 0 && m_map.isPlacementBlocked(candidate, position, zoneIndex, true))
            continue;
        if (compact) {
            TObjectType* prototype = candidate->m_prototype;
            int occupied = 0;
            for (unsigned int x = 0; x < prototype->getWidth(); ++x) {
                for (unsigned int y = 0; y < prototype->getHeight(); ++y) {
                    if (isRmgObjectFootprintCell(prototype, x, y))
                        ++occupied;
                }
            }
            objectValue /= occupied;
            if (objectValue < 3 * bestValuePerCell / 4)
                continue;
            if (bestValuePerCell < 3 * objectValue / 4) {
                totalWeight = 0;
                candidates.clear();
                properties.clear();
                bestValuePerCell = objectValue;
            }
        }
        totalWeight += definition->m_density;
        candidates.push_back(definition);
        properties.push_back(candidate);
    }
    if (!candidates.size())
        return 0;
    int selected = rand() % totalWeight;
    unsigned int selectedIndex;
    for (selectedIndex = 0; selectedIndex < candidates.size(); ++selectedIndex) {
        selected -= candidates[selectedIndex]->m_density;
        if (selected < 0)
            break;
    }
    type_treasure_def* definition = candidates[selectedIndex];
    *value = definition->getValue(zone, this);
    return definition->generate(properties[selectedIndex], this, zone);
}

// Preserve unsigned dimension addition and division, including even-size
// rounding, and set the temporary level independently of the object's level.
static inline TRmgMapPosition getRmgCenteredGroupObjectPosition(
    const TRmgTreasureGroup* group, const TObjectType* prototype)
{
    TRmgMapPosition position;
    position.m_x = (group->m_map.getWidth() + static_cast<unsigned>(prototype->getWidth())) / 2;
    position.m_y = (group->m_map.getHeight() + static_cast<unsigned>(prototype->getHeight())) / 2;
    position.m_z = 0;
    return position;
}

// Keep the output value in the caller's storage and the first/later primary
// flag explicit. Failed placement retries remain a separate caller policy.
static inline type_object* createRmgTreasureWithRetries(
    type_random_map_generator* generator, TRmgZone* zone, int minimum, int maximum,
    int* value, b8 primary, b8 alternate)
{
    for (int attempt = 0; attempt < RMG_TREASURE_ATTEMPTS; ++attempt) {
        TRmgMapPosition unspecified;
        unspecified.m_x = -1;
        unspecified.m_y = -1;
        unspecified.m_z = -1;
        type_object* object = generator->createTreasureObject(zone, minimum, maximum,
            value, primary, true, alternate, unspecified);
        if (object)
            return object;
    }
    return 0;
}

// Generation and fit have independent three-attempt limits. Keep distinct
// first/later object lifetimes and the native group->addObject boundary
// (Mac 0x24a670); failed fits release reservations before deletion.
VA(0x00546520, 0x1B6)
MAC_ADDRESS(0x24a584, 0x22c) // anchor-callee 0x54678a; thiscall, ret 0x10
int type_random_map_generator::fillTreasureGroup(TRmgZone* zone,
    TRmgTreasureGroup* group, b8 alternate, int value)
{
    int objectValue = 0;
    int attempts;
    int total;
    {
        type_object* selected = createRmgTreasureWithRetries(
            this, zone, value / 4, value, &objectValue, true, alternate);
        TRmgMapPosition position;
        if (!selected)
            return 0;
        TObjectType* prototype = selected->m_properties->m_prototype;
        type_object* object = selected;
        position = getRmgCenteredGroupObjectPosition(group, prototype);
        group->addObject(object, position);
        total = objectValue;
    }
    while (total < value) {
        int remainder = value - total;
        if (remainder < RMG_TREASURE_MINIMUM_REMAINDER && remainder < total / 2)
            break;
        type_object* nextObject;
        for (attempts = 0; ; ) {
            nextObject = createRmgTreasureWithRetries(this, zone,
                remainder / 4, 5 * remainder / 4, &objectValue, false, alternate);
            if (!nextObject)
                break;
            if (group->tryAddObject(nextObject))
                break;
            nextObject->releaseReservation();
            delete nextObject;
            if (++attempts >= RMG_TREASURE_ATTEMPTS) {
                nextObject = 0;
                break;
            }
        }
        if (!nextObject)
            break;
        total += objectValue;
    }
    group->updateBounds();
    return total;
}

// Failed placement still owns the group's objects. Release reservations before
// deleting each object, then clear the non-owning placement state. Successful
// groups transfer ownership to the generator and must use reset() alone.
static inline void discardRmgTreasureGroup(TRmgTreasureGroup* group)
{
    for (unsigned int index = 0; index < group->m_objects.size(); ++index) {
        group->m_objects[index]->releaseReservation();
        delete group->m_objects[index];
    }
    group->reset();
}

// A failed guard fit destroys the group's objects and the unaccepted guard.
VA(0x005466E0, 0x253)
MAC_ADDRESS(0x24a7b0, 0x110) // anchor-callee 0x547594/0x54768c; thiscall, ret 0x14; MAC_ABSTRACTION_FROM(tokens1:4deef3890efa,29.6117): discardRmgTreasureGroup shares ordered reservation release, deletion and reset across four failed-placement paths.
b8 type_random_map_generator::assembleTreasureGroup(TRmgZone* zone,
    TRmgTreasureGroup* group, b8 alternate, int minimum, int maximum)
{
    group->reset();
    int value = maximum <= minimum ? maximum : rand() % (maximum - minimum) + minimum;
    int totalValue = fillTreasureGroup(zone, group, alternate, value);
    if (!totalValue)
        return false;
    if (zone->m_templateZone->m_monsterStrength) {
        int strength = getRmgCombinedMonsterStrength(
            zone->m_templateZone->m_monsterStrength, m_monsterStrength);
        int guardValue = getRmgGuardValue(totalValue, strength);
        if (guardValue > 0) {
            type_object* guard = createGuard(guardValue, zone);
            if (guard && !group->addGuard(guard)) {
                discardRmgTreasureGroup(group);
                delete guard;
                return false;
            }
        }
    }
    group->traceOutline();
    group->markPlacementOutline();
    return true;
}

VA(0x00546940, 0x49) // anchor-callers + packed cell fields; thiscall ret 0x10
void TRmgMapItem::setTerrain(int terrain, int frame,
    b8 flipX, b8 flipY)
{
    m_tile.m_landType = terrain;
    m_tile.m_terrainFrame = frame;
    m_tileData.m_terrainFlipX = flipX;
    m_tileData.m_terrainFlipY = flipY;
}

VA(0x00546990, 0x1E) // anchor-callee reset expansions; Complete-only helper
TRmgMapItem* type_random_map::getMapItem(int x, int y)
{
    return &m_mapItems[y * m_mapWidth + x];
}

VA_COMPGEN(0x00404200, 0x209, VECTOR_INSERT, Int)

// The second overload returns an iterator (int*) rather than void.
VA_COMPGEN(0x00422F50, 0x1B1, VECTOR_INSERT_SINGLE, Int)

VA_COMPGEN(0x004347A0, 0x32E, VECTOR_INSERT, TRmgMapPosition)

VA_COMPGEN(0x0054C3F0, 0x21C, VECTOR_INSERT_SINGLE, TRmgMapPosition)

VA_COMPGEN(0x0054DD60, 0x15, STD_CONSTRUCT, TRmgMapPosition)

VA_COMPGEN(0x0054C730, 0x1DD, VECTOR_INSERT_SINGLE, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054C940, 0x23, VECTOR_DESTROY, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054D8B0, 0x38, VECTOR_UCOPY, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054D8F0, 0x29, VECTOR_UFILL, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054DD80, 0x104, STD_CONSTRUCT, TRmgObjectPlacementRule)

// The retained insertion calls these value-assignment loops. Both traverse
// 0x4c-byte rules with the two owned vectors at +0x2c and +0x3c.
VA_COMPGEN(0x0054DA20, 0x19F, STD_FILL, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054DBC0, 0x1A0, STD_COPY_BACKWARD, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054C970, 0x22F, VECTOR_INSERT_SINGLE, TRmgZoneConnection)

VA_COMPGEN(0x005157D0, 0x1B, CLASS_CTOR, vector)

VA_COMPGEN(0x00536BA0, 0x18, DEFAULT_CTOR_CLOSURE, vector)

VA_COMPGEN(0x00536B60, 0x3D, IMPLICIT_DTOR, TRmgObjectPlacementRule)
MAC_COMPGEN_ADDRESS(0x235010, 0x84, IMPLICIT_DTOR, TRmgObjectPlacementRule)

// The recovered generator-base constructor's exception cleanup naturally
// retains the value-vector destructor. Its 0x4c-stride loop and calls to
// TRmgObjectPlacementRule::~TRmgObjectPlacementRule distinguish it from
// the separate pointer-vector destructor emitted by the rule loader.
VA_COMPGEN(0x0054C170, 0x38, VECTOR_DTOR, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054C1B0, 0x23, VECTOR_SIZE, TRmgZoneConnection)

VA_COMPGEN(0x0054CD70, 0x3D, VECTOR_ERASE, TPoint)

// The neighboring 12-byte position worklist retains the same single-element
// erase specialization with TRmgMapPosition's three-dword copy loop.
VA_COMPGEN(0x0054C610, 0x53, VECTOR_ERASE, TRmgMapPosition)

// The reader's two resize shrink arms retain this int-vector erase.
VA_COMPGEN(0x0054CDB0, 0x33, VECTOR_ERASE, Int)

VA_COMPGEN(0x0054CDF0, 0x1D3, VECTOR_INSERT, unsigned_char)

VA_COMPGEN(0x0054CFD0, 0x2F, VECTOR_ERASE, unsigned_char)

// FilterZonePositions erases 12-byte positions through this forward copy;
// the retained body copies three dwords and returns the end pointer.
VA_COMPGEN(0x0054D9E0, 0x39, STD_COPY, TRmgMapPosition)

// Retail addObject (0x5402a0) calls the mutable form from its costs worklist;
// BlackBoxData assignment and the RMG helper cluster call the const form.
// Keep their distinct retained RVAs even though their machine code agrees.
VA_COMPGEN(0x005093c0, 0x25, STD_COPY, Int)

VA_COMPGEN(0x0054df40, 0x25, STD_COPY, const_int)

// Group objects store local XY positions on their temporary map. Translate
// those coordinates, but replace Z with the destination map level rather
// than adding it. Validation and commit must project each object identically.
static inline TRmgMapPosition getRmgPlacedGroupObjectPosition(
    type_object* object, const TRmgMapPosition& groupPosition)
{
    TRmgMapPosition position = object->getPosition();
    position.m_x += groupPosition.m_x;
    position.m_y += groupPosition.m_y;
    position.m_z = groupPosition.m_z;
    return position;
}

// Keep border-before-clearance snapshots and live object-vector bounds
// across callbacks. Mac 0x24ab68..0x24abc4 expands the land predicates;
// 0x24aa6c..0x24aacc copies the position and point before translation.
VA(0x005469B0, 0x2B4)
MAC_ADDRESS(0x24a8c0, 0x44c) // anchor-callee 0x547330; thiscall, ret 0x10
void type_random_map_generator::commitTreasureGroup(TRmgTreasureGroup* group,
    TRmgMapPosition position)
{
    group->m_position = position;
    for (unsigned int i = 0; i < group->m_objects.size(); ++i) {
        type_object* object = group->m_objects[i];
        TRmgMapPosition objectPosition = getRmgPlacedGroupObjectPosition(object, position);
        addObject(object, objectPosition);
    }
    TRmgZoneBounds bounds;
    bounds.m_minimumX = max(0, -position.m_x);
    bounds.m_minimumY = max(0, -position.m_y);
    bounds.m_maximumX = min(group->m_map.m_mapWidth, m_map.m_mapWidth - position.m_x);
    bounds.m_maximumY = min(group->m_map.m_mapHeight, m_map.m_mapHeight - position.m_y);
    TPoint point;
    for (point.m_y = bounds.m_minimumY; point.m_y < bounds.m_maximumY; ++point.m_y) {
        for (point.m_x = bounds.m_minimumX; point.m_x < bounds.m_maximumX; ++point.m_x) {
            TRmgMapItem* destination = m_map.getMapItem(position + point);
            b8 border = destination->hasBorderObject();
            b8 pathClearance = destination->hasPathClearance();
            TRmgMapItem* source = group->m_map.getMapItem(point.m_x, point.m_y);
            if (destination->getLandType() != eTerrainWater
                && canBlockRmgFloorCell(source)
                && destination->isPassableLand() && !destination->isRoadEntrance()) {
                destination->releasePathClearance();
                if (source->hasBorderObject()) {
                    destination->markBorderObject();
                }
            }
            if (!source->m_connection.m_present) {
                source->m_tileData.m_borderObject = border;
                if (border)
                    source->m_tileData.m_pathClearance = false;
            }
            if (!source->m_connection.m_present) {
                source->m_tileData.m_pathClearance = pathClearance;
                if (pathClearance)
                    source->m_tileData.m_borderObject = false;
            }
        }
    }
    for (unsigned int objectIndex = 0; objectIndex < group->m_objects.size(); ++objectIndex)
        group->m_objects[objectIndex]->isWritable();
}

// Keep cached bounds/zone identity but live object-vector bounds. Native
// objectsAllowEntrances precedes the guard check; Mac 0x24b0c8/0x24b1cc
// preserves the land predicates and 0x24ae00..0x24ae58 the guard translation.
VA(0x00546C70, 0x452)
MAC_ADDRESS(0x24ad0c, 0x6cc) // anchor-callee 0x54721c; thiscall, ret 0x14
b8 type_random_map_generator::canPlaceTreasureGroup(TRmgTreasureGroup* group,
    TRmgMapPosition position, TRmgZone* zone)
{
    TRmgMapPosition workingPosition;
    TRmgZoneBounds bounds = group->m_bounds;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    for (unsigned int i = 0; i < group->m_objects.size(); ++i) {
        type_object* object = group->m_objects[i];
        workingPosition = getRmgPlacedGroupObjectPosition(object, position);
        TRmgObjectPropertiesRef* properties = object->m_properties;
        if (m_map.isPlacementBlocked(properties, workingPosition, zoneIndex, true))
            return false;
    }
    if (group->m_hasGuard) {
        TPoint localGuard = group->m_guardPosition;
        workingPosition = position + localGuard;
        if (workingPosition.m_x < 1 || workingPosition.m_x + 1 >= m_map.m_mapWidth
            || workingPosition.m_y < 1 || workingPosition.m_y + 1 >= m_map.m_mapHeight)
            return false;
        TRmgMapPosition point;
        point.m_z = workingPosition.m_z;
        for (point.m_x = workingPosition.m_x - 1; point.m_x <= workingPosition.m_x + 1; ++point.m_x) {
            for (point.m_y = workingPosition.m_y - 1; point.m_y <= workingPosition.m_y + 1; ++point.m_y) {
                TRmgMapItem* item = m_map.getMapItem(point.m_x, point.m_y, point.m_z);
                if (item->isRoadEntrance()
                    && item->m_objects[0]->m_properties->m_prototype->getObjectType() == MONSTER)
                    return false;
            }
        }
    }
    int firstDirection = 0;
    int lastDirection = RMG_DIRECTION_COUNT;
    b8 waterZone = zone->m_terrain == eTerrainWater;
    type_object* lastObject = group->m_objects.back();
    TObjectType* prototype = lastObject->m_properties->m_prototype;
    TRmgMapPosition entrance = getRmgPlacedObjectEntrance(lastObject);
    if (!g_adventureObjectTraits[prototype->getObjectType()].m_trait1) {
        firstDirection = 1;
        lastDirection = 4;
    }
    int direction;
    for (direction = firstDirection; direction < lastDirection; ++direction) {
        TPoint point = g_rmgDirections[direction] + TRmgVector(entrance.m_x, entrance.m_y);
        TRmgMapItem* source = group->m_map.getMapItem(point.m_x, point.m_y);
        if (!source->isClearOutlineCell() || !source->isPlacementOutline())
            continue;
        point.m_x += position.m_x;
        point.m_y += position.m_y;
        if (!m_map.containsXY(point))
            continue;
        TRmgMapItem* destination = m_map.getMapItem(point.m_x, point.m_y, position.m_z);
        if ((destination->getLandType() == eTerrainWater) == waterZone
            && destination->isClearOutlineCell())
            break;
    }
    if (direction == lastDirection)
        return false;
    b32 allowEntrances = group->objectsAllowEntrances() && !group->m_hasGuard;
    if (!m_map.hasConnectedOutline(group->m_outline, position, allowEntrances, zone, true))
        return false;
    TPoint point;
    for (point.m_y = bounds.m_minimumY; point.m_y < bounds.m_maximumY; ++point.m_y) {
        for (point.m_x = bounds.m_minimumX; point.m_x < bounds.m_maximumX; ++point.m_x) {
            TRmgMapItem* source = group->m_map.getMapItem(point.m_x, point.m_y);
            if (!source->hasPathClearance()) {
                int x = point.m_x + position.m_x;
                int y = point.m_y + position.m_y;
                // Retail checks only the upper bounds here. Negative translated
                // coordinates are the caller's responsibility; preserve the unchecked query.
                if (x < m_map.m_mapWidth && y < m_map.m_mapHeight
                    && m_map.getMapItem(x, y, position.m_z)->isRoadEntrance())
                    return false;
            }
        }
    }
    return true;
}

// Read the live cell score after the fit callback; bounds and zone identity
// remain snapshots. Mac 0x24b514..0x24b590 copies the position and center
// point before the translated map query.
VA(0x005470D0, 0x286)
MAC_ADDRESS(0x24b3d8, 0x310) // anchor-callee 0x5475b2/0x5476aa; thiscall, ret 0xc
b8 type_random_map_generator::placeTreasureGroup(TRmgTreasureGroup* group,
    TRmgZone* zone, int spacing)
{
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    std::vector<TRmgMapPosition> candidates;
    TRmgZoneBounds bounds = zone->m_bounds;
    TRmgZoneBounds groupBounds = group->m_bounds;
    bounds.m_minimumY -= groupBounds.m_minimumY;
    TRmgMapPosition position = zone->getLevelPosition();
    position.m_y = bounds.m_minimumY;
    bounds.m_maximumX += 1 - groupBounds.m_maximumX;
    bounds.m_maximumY += 1 - groupBounds.m_maximumY;
    bounds.m_minimumX -= groupBounds.m_minimumX;
    TPoint center;
    center.m_x = (groupBounds.m_minimumX + groupBounds.m_maximumX) / 2;
    center.m_y = (groupBounds.m_minimumY + groupBounds.m_maximumY) / 2;
    for (; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position + center);
            if (item->m_zoneState.m_zone == zoneIndex && item->m_zoneState.m_score >= spacing
                && canPlaceTreasureGroup(group, position, zone)) {
                addRmgSpacingCandidate(candidates, position, item->m_zoneState.m_score, spacing);
            }
        }
    }
    if (candidates.size() == 0)
        return false;
    position = candidates[rand() % candidates.size()];
    commitTreasureGroup(group, position);
    return true;
}

// Keep the value cutoff before the density test.
static inline bool isRmgTreasureBandEnabled(const TRmgTreasureRange& range)
{
    return range.m_maximum >= 100 && range.m_density > 0;
}

// Retry one treasure-band shape, releasing each assembled group whose map
// placement fails. The caller retains the group across ordinary/compact
// passes and advances the scheduler only after this complete attempt batch.
static inline b8 tryPlaceRmgTreasureBand(
    type_random_map_generator* generator, TRmgZone* zone, TRmgTreasureGroup* group,
    b8 alternate, const TRmgTreasureRange& range, int spacing)
{
    for (int attempt = 0; attempt < RMG_TREASURE_ATTEMPTS; ++attempt) {
        if (generator->assembleTreasureGroup(zone, group, alternate,
                range.m_minimum, range.m_maximum)) {
            if (generator->placeTreasureGroup(group, zone, spacing))
                return true;
            discardRmgTreasureGroup(group);
        }
    }
    return false;
}

VA(0x00547360, 0x460)
MAC_ADDRESS(0x24b6e8, 0x358)
void type_random_map_generator::placeZoneTreasures(TRmgZone* zone)
{
    TRmgTemplateZone* slot = zone->m_templateZone;
    TRmgTreasureGroup group(16, 16);
    b8 finished[3];
    int totalDensity = 0;
    int densityProduct = 1;
    for (int band = 0; band < 3; ++band) {
        if (isRmgTreasureBandEnabled(slot->m_treasure[band])) {
            totalDensity += slot->m_treasure[band].m_density;
            densityProduct *= slot->m_treasure[band].m_density;
            finished[band] = false;
        } else {
            finished[band] = true;
        }
    }
    if (totalDensity == 0)
        return;
    int spacing;
    if (zone->m_terrain == eTerrainWater)
        spacing = getRmgDensitySpacing(1600, totalDensity);
    else
        spacing = getRmgDensitySpacing(800, totalDensity);
    int weightedCounts[3] = {0, 0, 0};
    int countSteps[3];
    for (band = 0; band < 3; ++band) {
        if (isRmgTreasureBandEnabled(slot->m_treasure[band]))
            countSteps[band] = densityProduct / slot->m_treasure[band].m_density;
    }
    for (;;) {
        int selected = selectRmgWeightedCategory(finished, weightedCounts, 3);
        if (selected == -1)
            break;
        weightedCounts[selected] += countSteps[selected];
        TRmgTreasureRange& range = slot->m_treasure[selected];
        if (tryPlaceRmgTreasureBand(this, zone, &group, false, range, spacing))
            continue;
        if (!tryPlaceRmgTreasureBand(this, zone, &group, true, range, spacing))
            finished[selected] = true;
    }
}

VA_COMPGEN(0x005477C0, 0xB3, IMPLICIT_DTOR, TRmgTreasureGroup)
MAC_COMPGEN_ADDRESS(0x24ba40, 0x90, IMPLICIT_DTOR, TRmgTreasureGroup)

// Record movement before queue insertion copies the next position. Callers
// retain their admission checks and random-cost draws.
static inline void queueRmgMovementStep(TRmgMapItem* destination,
    int cost, const TRmgMapPosition& previous, const TRmgMapPosition& next,
    std::vector<TRmgMapPosition>& positions, std::vector<int>& costs)
{
    destination->setMovementCost(cost, previous);
    insertRmgWorkItem(positions, costs, next, cost);
}

// Seed a zero-cost source in both worklists and the movement map. Keep both
// appends before the cell lookup and the by-value cost/predecessor update.
// Road and river searches pass a constructed invalid predecessor; the three
// coast-bound river sources share the caller's single instance.
static inline TRmgMapItem* seedRmgMovementSearch(type_random_map& map,
    const TRmgMapPosition& source, const TRmgMapPosition& invalidPredecessor,
    std::vector<TRmgMapPosition>& positions, std::vector<int>& costs)
{
    positions.push_back(source);
    costs.push_back(0);
    TRmgMapItem* item = map.getMapItem(source);
    item->setMovementCost(0, invalidPredecessor);
    return item;
}

// Road exits and entries share this restricted-approach policy. The caller
// retains its outgoing direction limit or incoming direction rejection.
static inline bool hasRmgRestrictedRoadApproach(const TAdvObjectTraits& traits)
{
    return !traits.m_trait1 && !traits.m_trait2;
}

// Dijkstra-style relaxation uses the back of a descending worklist, retaining
// duplicate entries. Monolith/gate transitions precede neighbour relaxation.
VA(0x00547880, 0x7B1)
MAC_ADDRESS(0x24bbe0, 0x7e4)  // roadTargets caller + monolith vectors; retail-only
void type_random_map_generator::buildRoadCostMap(TRmgMapPosition position)
{
    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;

    // Mac 0x24bcf8..0x24bd50 copies the constructed invalid predecessor into
    // a separate by-value setter parameter before storing cost and coordinates.
    TRmgMapItem* mapItem = seedRmgMovementSearch(m_map, position,
        TRmgMapPosition(-1, -1, -1), openPositions, openCosts);

    while (openPositions.size()) {
        popRmgMovementPosition(position, openPositions, openCosts);

        mapItem = m_map.getMapItem(position);
        int currentCost = mapItem->m_movement.m_cost;
        b8 currentHasRoad = mapItem->m_tile.m_roadType != 0;
        int direction = 8;
        b8 roadEntrance = mapItem->m_tileData.m_roadEntrance;

        if (roadEntrance) {
            type_object* object = mapItem->m_objects[0];
            TObjectType* prototype = object->m_properties->m_prototype;
            int objectType = prototype->getObjectType();
            if (hasRmgRestrictedRoadApproach(g_adventureObjectTraits[objectType]))
                direction = 5;

            switch (objectType) {
            case LITH_ONEWAY_ENTRANCE:
            case LITH_ONEWAY_EXIT:
            case LITH_TWOWAY: {
                // Both families relax matching subtypes in stored order.
                // Keep the one-way entrance/exit traversal used by retail.
                const std::vector<type_object*>& destinations = objectType == LITH_TWOWAY
                    ? m_monolithsTwoWay : m_monolithsOneWay;
                int subtype = prototype->getSubtype();
                for (int monolith = 0; monolith < destinations.size(); ++monolith) {
                    type_object* destination = destinations[monolith];
                    if (destination->m_properties->m_prototype->getSubtype() != subtype)
                        continue;

                    TRmgMapPosition nextPosition = destination->m_position;
                    TRmgMapItem* nextMapItem = m_map.getMapItem(nextPosition);
                    int nextCost = currentCost + 50;
                    if (nextMapItem->m_movement.m_cost <= nextCost)
                        continue;

                    queueRmgMovementStep(nextMapItem, nextCost, position, nextPosition,
                        openPositions, openCosts);
                }
                break;
            }

            case UNDERGROUND_GATE: {
                TRmgMapPosition nextPosition;
                nextPosition.m_x = position.m_x;
                nextPosition.m_y = position.m_y;
                nextPosition.m_z = 1 - position.m_z;
                TRmgMapItem* nextMapItem = m_map.getMapItem(nextPosition);
                int nextCost = currentCost + 1;
                if (nextMapItem->m_movement.m_cost > nextCost) {
                    queueRmgMovementStep(nextMapItem, nextCost, position, nextPosition,
                        openPositions, openCosts);
                }
                break;
            }
            }
        }

        while (direction--) {
            TRmgMapPosition nextPosition = position + g_rmgDirections[direction];

            if (!m_map.containsXY(nextPosition))
                continue;

            TRmgMapItem* nextMapItem = m_map.getMapItem(nextPosition);
            if (nextMapItem->getLandType() == eTerrainWater
                || !nextMapItem->isPassableLand())
                continue;

            b8 nextRoadEntrance =
                nextMapItem->m_tileData.m_roadEntrance;
            if (nextRoadEntrance) {
                int objectType =
                    nextMapItem->m_objects[0]->m_properties->m_prototype->getObjectType();
                const TAdvObjectTraits& traits = g_adventureObjectTraits[objectType];
                if (traits.m_blocksLanding && !traits.m_trait2)
                    continue;
                if (hasRmgRestrictedRoadApproach(traits)
                    && direction > 0 && direction < 4)
                    continue;
            }

            int nextCost = currentHasRoad
                               && nextMapItem->m_tile.m_roadType
                           ? 2 : 20;
            if (direction & 1)
                nextCost *= 3;
            nextCost += currentCost;

            if (nextMapItem->m_movement.m_cost <= nextCost)
                continue;

            queueRmgMovementStep(nextMapItem, nextCost, position, nextPosition,
                openPositions, openCosts);
        }
    }
}

// Paint same-level cardinal runs, restarting the map view at diagonal or
// level transitions. The adapter and painter live only for each painted run.
VA(0x00548040, 0x244)
MAC_ADDRESS(0x24c3c4, 0x2bc) // anchor-callee 0x548408 + adapter/painter vtables; retail-only
b8 type_random_map_generator::paintRoad(TRmgMapPosition position, int roadType)
{
    b8 painted = false;
    for (;;) {
        int level = position.m_z;
        type_random_map levelMap(m_map.getMapItem(0, 0, level),
            m_map.m_mapWidth, m_map.m_mapHeight);
        TRmgMapPosition previous = position;
        TRmgMapItem* existing = m_map.getMapItem(position);
        while (existing->m_tile.m_roadType == roadType) {
            if (existing->m_movement.m_cost == 0)
                return painted;
            previous = position;
            position = existing->m_previousTile;
            existing = m_map.getMapItem(position);
        }
        if (position.m_z == level) {
            position = previous;
            TRmgRoadMapAdapter adapter(&levelMap);
            TRmgRoadPainter painter(
                &adapter, roadType, TRmgGridPoint(position.m_x, position.m_y));
            painted = true;
            for (;;) {
                TRmgMapItem* item = m_map.getMapItem(position);
                if (item->m_movement.m_cost == 0)
                    return painted;
                position = item->m_previousTile;
                if (position.m_z != level
                    || (position.m_x != previous.m_x && position.m_y != previous.m_y))
                    break;
                painted = true;
                painter.drawTo(TRmgGridPoint(position.m_x, position.m_y));
                previous = position;
            }
        }
    }
}

// Retail createRiverToObject +0x2f/+0x3d retains the invalid-predecessor
// constructor and GetMapItem(0, 0). Preserve the signed levels*height row
// count before multiplying by width.
MAC_ADDRESS(0x24bb68, 0x78)
void type_random_map_generator::resetMovementCosts()
{
    TRmgMapPosition resetPosition(-1, -1, -1);
    TRmgMapItem* mapItem = m_map.getMapItem(0, 0);
    int rowCount = m_map.m_numberLevels * m_map.m_mapHeight;
    int mapItemCount = rowCount * m_map.m_mapWidth;
    while (mapItemCount--) {
        mapItem->resetMovement(resetPosition);
        ++mapItem;
    }
}

// Newly painted roads change traversal costs for subsequent targets.
static inline void rebuildRmgRoadCostMap(type_random_map_generator* generator,
    const TRmgMapPosition& source)
{
    generator->resetMovementCosts();
    generator->buildRoadCostMap(source);
}

VA(0x00548290, 0x26E)
MAC_ADDRESS(0x24c6d4, 0x1d8)
void type_random_map_generator::createRoads()
{
    int roadType = rand() % 3 + 1;
    // Retail subtracts before comparing: an empty target vector underflows.
    // Preserve this precondition/bug; do not replace it with a guarded loop.
    for (unsigned int first = 0; first < m_roadTargets.size() - 1; ++first) {
        TRmgMapPosition source = m_roadTargets[first];
        rebuildRmgRoadCostMap(this, source);
        for (unsigned int second = first + 1; second < m_roadTargets.size(); ++second) {
            TRmgMapPosition destination = m_roadTargets[second];
            if (m_map.getMapItem(destination)->m_movement.m_cost <= 30000
                && paintRoad(destination, roadType)
                && second < m_roadTargets.size() - 1) {
                rebuildRmgRoadCostMap(this, source);
                if (m_progress)
                    m_progress->advance(1000);
            }
        }
        if (m_progress)
            m_progress->advance(1000);
    }
}

// Both river searches draw a fresh step cost before testing improvement.
// Preserve the random draw before the destination's road surcharge, even
// when the caller subsequently rejects the relaxation or approach direction.
static inline int getRmgRiverStepCost(int currentCost, const TRmgMapItem* destination)
{
    int nextCost = currentCost + (rand() & 31) + 1;
    if (destination->m_tile.m_roadType)
        nextCost += 30;
    return nextCost;
}

// The source terrain determines both river graphics and the snow boundary
// restriction. Both searches initialize these together before traversal.
static inline void selectRmgRiverAppearance(const TRmgMapItem* source,
    b8& sourceIsSnow, int& riverType)
{
    if (source->getLandType() == eTerrainSnow) {
        sourceIsSnow = true;
        riverType = 2;
    } else {
        sourceIsSnow = false;
        riverType = 1;
    }
}

// Rivers stay on dry, non-rock terrain and cannot cross the snow boundary.
// Object and coast searches share this terrain rule; only the coast search
// also rejects impassable cells and restricted approach directions.
static inline bool isRmgRiverTerrain(const TRmgMapItem* item, b8 sourceIsSnow)
{
    return item->getLandType() != eTerrainWater
        && item->getLandType() != eTerrainRock
        && (item->getLandType() == eTerrainSnow) == sourceIsSnow;
}

// Random edge costs are drawn on each neighbour visit, even without an
// improvement. Unlike coast-bound rivers, this search ignores impassable
// and direction flags. Mac 0x24c9cc..0x24ca28 preserves the by-value
// invalid-predecessor setter at the seed.
VA(0x00548500, 0x533)
MAC_ADDRESS(0x24c8ac, 0x588)
void type_random_map_generator::createRiverToObject(TRmgMapPosition source)
{
    resetMovementCosts();
    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;
    TRmgMapItem* mapItem = seedRmgMovementSearch(m_map, source,
        TRmgMapPosition(-1, -1, -1), openPositions, openCosts);
    b8 sourceIsSnow;
    int riverType;
    selectRmgRiverAppearance(mapItem, sourceIsSnow, riverType);
    TRmgMapPosition position;
    TRmgMapPosition nextPosition;
    while (!openPositions.empty()) {
        popRmgMovementPosition(position, openPositions, openCosts);
        mapItem = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
        int positionCost = mapItem->m_movement.m_cost;
        for (int direction = 0; direction < 8; direction += 2) {
            nextPosition = position + g_rmgDirections[direction];
            if (!m_map.containsXY(nextPosition))
                continue;
            mapItem = m_map.getMapItem(nextPosition.m_x, nextPosition.m_y, nextPosition.m_z);
            if (!isRmgRiverTerrain(mapItem, sourceIsSnow))
                continue;
            int nextCost = getRmgRiverStepCost(positionCost, mapItem);
            if (nextCost >= mapItem->m_movement.m_cost)
                continue;
            queueRmgMovementStep(mapItem, nextCost, position, nextPosition,
                openPositions, openCosts);
            if (mapItem->hasRiver()) {
                openPositions.clear();
                break;
            }
        }
    }
    // As in createRiver, this is the last inspected tile, not an explicit
    // successful-search result. Preserve the retail acceptance condition.
    if (!mapItem->hasRiver())
        return;
    type_random_map levelMap(m_map.getMapItem(0, 0, nextPosition.m_z),
        m_map.getWidth(), m_map.getHeight());
    TRmgRiverMapAdapter mapAdapter(&levelMap);
    TRmgRiverPainter riverPainter(
        &mapAdapter, riverType, TRmgGridPoint(nextPosition.m_x, nextPosition.m_y));
    while (mapItem->m_movement.m_cost > 0) {
        position = mapItem->m_previousTile;
        mapItem = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
        riverPainter.drawTo(TRmgGridPoint(position.m_x, position.m_y));
    }
}

// This scan deliberately admits x == width. Preserve the retail river-coast
// policy separately from containsXY; correcting it would change generation.
static inline bool isOutsideRmgRiverCoastScan(
    const TRmgMapPosition& point, const type_random_map& map)
{
    return point.m_x < 0 || point.m_x > map.m_mapWidth
        || point.m_y < 0 || point.m_y >= map.m_mapHeight;
}

// The dry strip and inland approach share the same coast-scan bounds and
// water/entrance exclusions. Rock remains admissible, as in retail.
static inline TRmgMapItem* getRmgDryRiverCoastCell(
    type_random_map& map, const TRmgMapPosition& point)
{
    if (isOutsideRmgRiverCoastScan(point, map))
        return 0;
    TRmgMapItem* item = map.getMapItem(point.m_x, point.m_y, point.m_z);
    if (item->getLandType() == eTerrainWater || item->isRoadEntrance())
        return 0;
    return item;
}

// Test three water cells, three dry entrance-free cells, then four inland
// cells. Retail admits x == width; the last inland cell receives the target.
VA(0x00548A40, 0x222)
MAC_ADDRESS(0x24ce88, 0x404)
void type_random_map_generator::markRiverCoastTarget(TRmgMapPosition position, int direction)
{
    TRmgMapPosition point = position + g_rmgDirections[(direction + 2) & 7];
    TPoint step = g_rmgDirections[(direction - 2) & 7];
    for (int waterCount = 0; waterCount < 3; ++waterCount) {
        if (isOutsideRmgRiverCoastScan(point, m_map))
            return;
        if (m_map.getMapItem(point.m_x, point.m_y, point.m_z)->getLandType() != eTerrainWater)
            return;
        point += step;
    }
    point = position + g_rmgDirections[(direction + 1) & 7];
    for (int dryCount = 0; dryCount < 3; ++dryCount) {
        if (!getRmgDryRiverCoastCell(m_map, point))
            return;
        point += step;
    }
    point = position;
    point += g_rmgDirections[direction];
    TRmgMapItem* item;
    for (int inlandCount = 0; inlandCount < 4; ++inlandCount) {
        item = getRmgDryRiverCoastCell(m_map, point);
        if (!item)
            return;
        point += g_rmgDirections[direction];
    }
    item->m_tileData.m_blockedDirections |= 1 << (((direction - 4) >> 1) & 3);
    item->m_tileData.m_riverTarget = true;
}

VA(0x00548C70, 0x17D)
MAC_ADDRESS(0x24d28c, 0x264)
void type_random_map_generator::markRiverTargets()
{
    TRmgMapPosition position;
    TRmgMapItem* item = m_map.m_mapItems;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (item->getLandType() == eTerrainWater) {
                    for (int direction = 0; direction < 8; direction += 2)
                        markRiverCoastTarget(position, direction);
                }
            }
        }
    }
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            item = m_map.getMapItem(0, position.m_y, position.m_z);
            item->m_tileData.m_riverTarget = true;
            item = m_map.getMapItem(m_map.m_mapWidth - 1, position.m_y, position.m_z);
            item->m_tileData.m_riverTarget = true;
        }
        for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x) {
            item = m_map.getMapItem(position.m_x, 0, position.m_z);
            item->m_tileData.m_riverTarget = true;
            item = m_map.getMapItem(position.m_x, m_map.m_mapHeight - 1, position.m_z);
            item->m_tileData.m_riverTarget = true;
        }
    }
    if (m_progress)
        m_progress->advance(1000);
}

// Randomized best-first relaxation, with the same per-visit random edge costs
// as createRiverToObject, seeded at the three water-wheel approach cells.
VA(0x00548DF0, 0x99F)
MAC_ADDRESS(0x24d4f0, 0xb00)  // water-wheel caller + river-delta object; retail-only
void type_random_map_generator::createRiver(TRmgMapPosition source)
{
    resetMovementCosts();

    TRmgMapItem* mapItem;
    TRmgMapPosition invalidPredecessor;
    invalidPredecessor.m_x = -1;
    invalidPredecessor.m_y = -1;
    invalidPredecessor.m_z = -1;

    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;

    mapItem = seedRmgMovementSearch(m_map, source, invalidPredecessor,
        openPositions, openCosts);

    b8 sourceIsSnow;
    int riverType;
    selectRmgRiverAppearance(mapItem, sourceIsSnow, riverType);

    --source.m_y;
    mapItem = seedRmgMovementSearch(m_map, source, invalidPredecessor,
        openPositions, openCosts);

    ++source.m_x;
    mapItem = seedRmgMovementSearch(m_map, source, invalidPredecessor,
        openPositions, openCosts);

    TRmgMapPosition position;
    TRmgMapPosition nextPosition;
    int direction;

    while (!openPositions.empty()) {
        popRmgMovementPosition(position, openPositions, openCosts);

        mapItem = m_map.getMapItem(position);
        int positionCost = mapItem->m_movement.m_cost;
        for (direction = 0; direction < 8; direction += 2) {
            nextPosition = position + g_rmgDirections[direction];

            if (!m_map.containsXY(nextPosition))
                continue;

            mapItem = m_map.getMapItem(nextPosition);
            if (!isRmgRiverTerrain(mapItem, sourceIsSnow) || mapItem->isImpassable())
                continue;

            int nextCost = getRmgRiverStepCost(positionCost, mapItem);

            if (nextCost >= mapItem->m_movement.m_cost)
                continue;

            int oppositeDirection = ((direction - 4) >> 1) & 3;
            if (mapItem->m_tileData.m_blockedDirections
                & (1 << oppositeDirection))
                continue;

            queueRmgMovementStep(mapItem, nextCost, position, nextPosition,
                openPositions, openCosts);

            if (mapItem->isRiverTarget()) {
                openPositions.clear();
                break;
            }
        }
    }

    // Retail tests the last inspected tile, even if relaxation rejected it.
    // An unreached target can start an invalid predecessor walk; see
    // docs/reference/rmg-undefined-behavior.md. Preserve that behavior.
    if (!mapItem->isRiverTarget())
        return;

    mapItem->m_tileData.m_riverTarget = true;
    position = nextPosition;

    type_random_map levelMap(m_map.getMapItem(0, 0, nextPosition.m_z),
        m_map.m_mapWidth, m_map.m_mapHeight);
    TRmgRiverMapAdapter mapAdapter(&levelMap);
    TRmgRiverPainter riverPainter(
        &mapAdapter, riverType, TRmgGridPoint(nextPosition.m_x, nextPosition.m_y));

    if (mapItem->m_tileData.m_blockedDirections) {
        for (direction = 0; direction < 4; ++direction) {
            if (mapItem->m_tileData.m_blockedDirections & (1 << direction))
                break;
        }

        // Construction guard byte 0x69d59c; the four offsets at 0x69ce28 are
        // indexed by eight here. The guarded initializer registers
        // atexit(0x549790), the table's empty cleanup (a bare `ret`) right
        // after this function in rmg's contribution.
        DATA_COMPGEN_GUARD(0x0069d59c, riverDeltaOffsetsGuard, deltaOffsets)

        VA_COMPGEN(0x00549790, 0x1, STATIC_DTOR, deltaOffsets)
        DATA(0x0069ce28)
        static TRmgRiverDeltaOffset deltaOffsets[4] = {
            TRmgRiverDeltaOffset(4, 1),
            TRmgRiverDeltaOffset(1, 4),
            TRmgRiverDeltaOffset(-2, 1),
            TRmgRiverDeltaOffset(1, -2)
        };

        int deltaIndex = sourceIsSnow
            ? g_snowRiverDeltaIndex[direction]
            : g_landRiverDeltaIndex[direction];
        int landType = mapItem->getLandType();
        int prototypeIndex = 0;
        for (; prototypeIndex < m_objectPrototypes[TERRAIN_RIVER_DELTA].size();
             ++prototypeIndex) {
            TRmgObjectPropertiesRef* properties =
                m_objectPrototypes[TERRAIN_RIVER_DELTA][prototypeIndex];
            if (properties->m_prototype->m_recommendedTerrainMask[landType]
                && deltaIndex-- == 0)
                break;
        }

        if (prototypeIndex == m_objectPrototypes[TERRAIN_RIVER_DELTA].size())
            return;

        type_object* riverDelta = new type_object(
            m_objectPrototypes[TERRAIN_RIVER_DELTA][prototypeIndex]);
        addObject(
            riverDelta,
            TRmgMapPosition(
                nextPosition.m_x + deltaOffsets[direction].m_x,
                nextPosition.m_y + deltaOffsets[direction].m_y,
                nextPosition.m_z));

        nextPosition = nextPosition + g_rmgDirections[direction * 2];
        riverPainter.drawTo(TRmgGridPoint(nextPosition.m_x, nextPosition.m_y));
        mapItem = m_map.getMapItem(nextPosition);
        mapItem->m_tileData.m_riverTarget = true;

        riverPainter.drawTo(TRmgGridPoint(position.m_x, position.m_y));
        mapItem = m_map.getMapItem(position);
    }

    while (mapItem->m_movement.m_cost > 0) {
        position = mapItem->m_previousTile;
        mapItem = m_map.getMapItem(position);
        mapItem->m_tileData.m_riverTarget = true;
        riverPainter.drawTo(TRmgGridPoint(position.m_x, position.m_y));
    }
}

VA(0x005497A0, 0xCE)
MAC_ADDRESS(0x24dff0, 0x18c)
void type_random_map_generator::markRiverObjectTargets()
{
    for (unsigned int index = 0; index < m_objects.size(); ++index) {
        type_object* object = m_objects[index];
        TObjectType* prototype = object->m_properties->m_prototype;
        if (prototype->getObjectType() == TERRAIN_MOUNTAIN
            || prototype->getObjectType() == TERRAIN_LAKE
            || (prototype->getObjectType() == MINE && prototype->getSubtype() == GEMS)) {
            TRmgMapPosition position = object->m_position;
            int offsetX;
            int offsetY;
            if (prototype->m_hasTrigger) {
                offsetX = prototype->m_triggerCell.m_x;
                offsetY = prototype->m_triggerCell.m_y;
            } else {
                offsetX = static_cast<unsigned int>(prototype->getWidth()) / 2;
                offsetY = static_cast<unsigned int>(prototype->getHeight()) / 2;
            }
            position.m_x -= offsetX;
            position.m_y -= offsetY;
            if (m_map.containsXY(position))
            {
                TRmgMapItem* item = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
                item->m_tileData.m_hasRiver = true;
            }
        }
    }
    if (m_progress)
        m_progress->advance(1000);
}

VA(0x00549870, 0xB1)
MAC_ADDRESS(0x24e17c, 0x118)
void type_random_map_generator::createRivers()
{
    markRiverObjectTargets();
    markRiverTargets();
    for (unsigned int index = 0; index < m_objects.size(); ++index) {
        type_object* object = m_objects[index];
        TObjectType* prototype = object->m_properties->m_prototype;
        if (prototype->getObjectType() == WATER_WHEEL) {
            TRmgMapPosition position = getRmgPlacedObjectEntrance(object);
            createRiverToObject(position);
            position.m_x -= 2;
            createRiver(position);
            if (m_progress)
                m_progress->advance(1000);
        }
    }
}

// Preserve player order, separate town/connection passes, and the final
// coastal/decorative/road/river order. Mac 0x24e37c reloads the selected
// template through m_templates on every loop test; do not cache that pointer.
VA(0x00549930, 0x37B)
MAC_ADDRESS(0x24e294, 0x41c)
b8 type_random_map_generator::generate()
{
    if (!m_templates.size())
        return false;
    unsigned int selected = rand() % m_templates.size();
    m_templateName = m_templates[selected]->m_name;
    char humanSlots[8];
    int humanSlotByte;
    MEMSET(humanSlots, 0, sizeof(humanSlots), humanSlotByte);
    char allSlots[8];
    int allSlotByte;
    MEMSET(allSlots, 0, sizeof(allSlots), allSlotByte);
    for (unsigned int zone = 0; zone < m_templates[selected]->m_zones.size(); ++zone) {
        TRmgTemplateZone* slot = m_templates[selected]->m_zones[zone];
        if (slot->m_kind == RMG_TEMPLATE_HUMAN) {
            humanSlots[slot->m_playerIndex] = 1;
            allSlots[slot->m_playerIndex] = 1;
        } else if (slot->m_kind == RMG_TEMPLATE_COMPUTER) {
            allSlots[slot->m_playerIndex] = 1;
        }
    }
    int mapIndex;
    MEMSET(m_playerIndexMap, -1, sizeof(m_playerIndexMap), mapIndex);
    int players[8];
    int count = 0;
    for (int player = 0; player < 8; ++player)
        if (m_fixedHumanPlayers[player])
            players[count++] = player;
    for (player = 0; player < 8; ++player)
        if (!m_fixedHumanPlayers[player])
            players[count++] = player;
    int slot = 0;
    for (player = 0; player < m_humanPlayerCount; ++player) {
        while (slot < 8 && !humanSlots[slot])
            ++slot;
        allSlots[slot] = 0;
        m_playerIndexMap[++slot] = players[player];
    }
    slot = 0;
    for (; player < m_humanPlayerCount + m_computerPlayerCount; ++player) {
        while (slot < 8 && !allSlots[slot])
            ++slot;
        m_playerIndexMap[++slot] = players[player];
    }
    initializeZones(m_templates[selected]);
    for (int level = 0; level < m_map.m_numberLevels; ++level)
        buildZoneBoundaries(m_templates[selected], level);
    paintZoneTerrain();
    for (zone = 0; zone < m_zones.size(); ++zone)
        placePrimaryTown(m_zones[zone]);
    for (zone = 0; zone < m_zones.size(); ++zone)
        placeAdditionalTowns(m_zones[zone]);
    prepareZoneConnections();
    for (zone = 0; zone < m_zones.size(); ++zone)
        if (m_zones[zone]->m_templateZone->m_kind == RMG_TEMPLATE_JUNCTION
            && m_zones[zone]->m_terrain != eTerrainWater)
            prepareJunctionZone(m_zones[zone]);
    placeMines();
    memset(m_activeZoneCountsByAlignment, 0, sizeof(m_activeZoneCountsByAlignment));
    m_activeZoneCount = 0;
    for (zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->m_active) {
            ++m_activeZoneCountsByAlignment[m_zones[zone]->m_alignment];
            ++m_activeZoneCount;
        }
    }
    buildZoneConnectionPaths();
    for (zone = 0; zone < m_zones.size(); ++zone) {
        placeZoneTreasures(m_zones[zone]);
        if (m_progress)
            m_progress->advance(6900 / m_zones.size());
    }
    if (m_map.m_numberLevels > 1)
        decorateUnderground();
    m_map.markCoastalTiles();
    decorateMap();
    createRoads();
    createRivers();
    return true;
}

// These encoder/writer boundaries are inferred from repeated header expansions.
template <size_t N>
void encodePackedBits(const std::bitset<N>& bits, unsigned char* packed)
{
    memset(packed, 0, (N + 7) / 8);
    for (unsigned int index = 0; index < N; ++index) {
        if (bits.test(index))
            packed[index >> 3] |= 1 << (index & 7);
    }
}

template <size_t N>
int writePackedBits(TAbstractFile* outfile, const std::bitset<N>& bits)
{
    unsigned char packed[(N + 7) / 8];
    encodePackedBits(bits, packed);
    return outfile->write(packed, sizeof(packed));
}

// Length-prefixed text retains the caller-owned string/buffer lifetime and
// reads the payload length again after writing its native int prefix.
int writeString(TAbstractFile* outfile, const std::string& text)
{
    writeValue<int>(outfile, text.length());
    return outfile->write(text.c_str(), text.length());
}

int writeString(TAbstractFile* outfile, const char* text)
{
    writeValue<int>(outfile, strlen(text));
    return outfile->write(text, strlen(text));
}

// Each player clause in the map description appends a separator, the player
// color and the clause text. Retail uses unchecked strcat for all three.
static inline void appendRmgPlayerDescription(char* description, int player,
    const char* clause)
{
    strcat(description, DATA_COMPGEN(0x0066032C, rmgListSeparator, ", "));
    strcat(description, g_rmgPlayerNames[player]);
    strcat(description, clause);
}

VA(0x00549CB0, 0xE90)
MAC_ADDRESS(0x24e7d4, 0x11ac)  // GenerateRandomMap caller chain; retail-only RMG
void type_random_map_generator::writeMapHeader(TAbstractFile* outfile)
{
    writeValue<int>(outfile, getSerializedMapVersion());

    writeValue<char>(outfile, 1);

    writeValue<int>(outfile, m_map.getWidth());

    writeValue<char>(outfile, m_map.m_numberLevels > 1);

    std::string mapName(
        DATA_COMPGEN(0x00682900, rmgMapName, "Random Map"));
    writeString(outfile, mapName);

    // Retail places description at [ebp-0x324] and mainTowns at
    // [ebp-0x130]; their 0x1f4-byte separation proves the 500-byte extent.
    // Retail uses unchecked sprintf/strcat below; oversized template names
    // or appended player descriptions can exceed this fixed buffer.
    char description[500];
    sprintf(
        description,
        DATA_COMPGEN(
            0x0068286C,
            rmgDescriptionFormat,
            "Map created by the Random Map Generator.  Template was %s, "
            "Random seed was %i, size %i, levels %i, humans %i, "
            "computers %i, water %s, monsters %i"),
        m_templateName.c_str(),
        m_randomSeed,
        m_map.getWidth(),
        m_map.getNumberLevels(),
        m_humanPlayerCount,
        m_computerPlayerCount,
        g_rmgWaterNames[m_waterContent],
        m_monsterStrength);

    switch (m_mapVersion) {
    case RMG_MAP_RESTORATION_OF_ERATHIA:
        strcat(
            description,
            DATA_COMPGEN(0x0068282C, rmgOriginalMap, ", original map"));
        break;
    case RMG_MAP_ARMAGEDDONS_BLADE:
        strcat(
            description,
            DATA_COMPGEN(
                0x0068283C, rmgFirstExpansionMap, ", first expansion map"));
        break;
    case RMG_MAP_SHADOW_OF_DEATH:
        strcat(
            description,
            DATA_COMPGEN(
                0x00682854,
                rmgSecondExpansionMap,
                ", second expansion map"));
        break;
    }

    for (int descriptionPlayer = 0; descriptionPlayer < 8;
         ++descriptionPlayer) {
        if (m_fixedHumanPlayers[descriptionPlayer]) {
            appendRmgPlayerDescription(description, descriptionPlayer,
                DATA_COMPGEN(0x00682820, rmgIsHuman, " is human"));
        }

        if (m_townChoices[descriptionPlayer] != -1) {
            appendRmgPlayerDescription(description, descriptionPlayer,
                DATA_COMPGEN(
                    0x0068280C, rmgTownChoiceIs, " town choice is "));
            strcat(
                description,
                // Retail 0x549fba reuses the player-index byte offset for
                // this lookup, as it does for the preceding color name.
                // Preserve that description bug when another town was chosen.
                g_rmgTownNames[descriptionPlayer]);
        }
    }

    writeString(outfile, description);

    writeValue<char>(outfile, 1);
    if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<char>(outfile, 0);
    }

    b8 canBeHuman[8];
    int legalAlignments[8];
    TRmgMapPosition mainTowns[8];
    b8 canBeComputer[8];
    memset(canBeHuman, 0, sizeof(canBeHuman));
    memset(legalAlignments, 0, sizeof(legalAlignments));
    memset(mainTowns, 0, sizeof(mainTowns));
    memset(canBeComputer, 0, sizeof(canBeComputer));
    int generatedHumanTowns = 0;
    for (unsigned int townIndex = 0; townIndex < m_zones.size(); ++townIndex) {
        TRmgZone* town = m_zones[townIndex];
        TRmgTemplateZone* slot = town->m_templateZone;
        int player = slot->m_playerIndex;
        if (player < 0)
            continue;

        player = m_playerIndexMap[player + 1];
        if (player < 0 || !town->m_active)
            continue;

        if (slot->m_kind == RMG_TEMPLATE_HUMAN && !canBeHuman[player]) {
            ++generatedHumanTowns;
            canBeHuman[player] = true;
            mainTowns[player] = town->m_position;
        }

        if (slot->m_kind == RMG_TEMPLATE_COMPUTER && !canBeComputer[player]) {
            canBeComputer[player] = true;
            mainTowns[player] = town->m_position;
        }

        legalAlignments[player] |= 1 << town->m_alignment;
    }

    generatedHumanTowns -= m_humanPlayerCount;
    int reversePlayer = 7;
    do {
        if (canBeHuman[reversePlayer]
            && !m_fixedHumanPlayers[reversePlayer]
            && generatedHumanTowns > 0) {
            canBeComputer[reversePlayer] = true;
            canBeHuman[reversePlayer] = false;
            --generatedHumanTowns;
        }
    } while (reversePlayer-- != 0);

    m_computerPlayerCount = m_humanPlayerCount = 0;

    for (int serializedPlayer = 0; serializedPlayer < 8;
         ++serializedPlayer) {
        writeValue<char>(outfile, canBeHuman[serializedPlayer]);

        writeValue<char>(outfile, canBeHuman[serializedPlayer] || canBeComputer[serializedPlayer]);

        writeValue<char>(outfile, 0);

        if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
            writeValue<char>(outfile, 0);
        }

        if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
            writeValue<unsigned short>(outfile, legalAlignments[serializedPlayer]);
        } else {
            writeValue<char>(outfile, legalAlignments[serializedPlayer]);
        }

        writeValue<char>(outfile, 0);

        if (!canBeHuman[serializedPlayer]
            && !canBeComputer[serializedPlayer]) {
            writeValue<char>(outfile, 0);
        } else {
            if (canBeHuman[serializedPlayer])
                ++m_humanPlayerCount;
            else
                ++m_computerPlayerCount;

            writeValue<char>(outfile, 1);

            if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
                writeValue<char>(outfile, 1);
                writeValue<char>(outfile, -1);
            }

            writeRmgMapPosition(outfile, mainTowns[serializedPlayer]);
        }

        writeValue<char>(outfile, 0);
        writeValue<char>(outfile, -1);

        if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
            writeValue<char>(outfile, 0);
            writeValue<int>(outfile, 0);
        }
    }

    writeValue<char>(outfile, -1);
    writeValue<char>(outfile, -1);

    if (!m_computerTeamCount)
        m_computerTeamCount = m_computerPlayerCount;
    if (!m_humanTeamCount)
        m_humanTeamCount = m_humanPlayerCount;
    if (!m_computerPlayerCount)
        m_humanTeamCount = max(m_humanTeamCount, 2);

    if (m_humanTeamCount >= m_humanPlayerCount
        && m_computerTeamCount >= m_computerPlayerCount) {
        writeValue<char>(outfile, 0);
    } else {
        char teams[8];
        memset(teams, 0, sizeof(teams));

        m_humanTeamCount = max(m_humanTeamCount, 1);
        m_computerTeamCount = max(m_computerTeamCount, 1);
        m_humanTeamCount = min(m_humanPlayerCount, m_humanTeamCount);
        m_computerTeamCount = min(m_computerPlayerCount, m_computerTeamCount);

        assignRmgTeams(
            m_humanTeamCount,
            m_humanPlayerCount,
            0,
            canBeHuman,
            teams);
        assignRmgTeams(
            m_computerTeamCount,
            m_computerPlayerCount,
            m_humanTeamCount,
            canBeComputer,
            teams);

        writeValue<char>(outfile, m_humanTeamCount + m_computerTeamCount);
        outfile->write(teams, sizeof(teams));
    }

    if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
        std::bitset<156> availableHeroes;
        setAvailableRmgHeroes(
            &availableHeroes, m_disabledHeroes, m_disabledHeroes + 156);

        writePackedBits(outfile, availableHeroes);
    } else {
        std::bitset<128> availableHeroes;
        setAvailableRmgHeroes(
            &availableHeroes, m_disabledHeroes, m_disabledHeroes + 128);

        writePackedBits(outfile, availableHeroes);
    }

    if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<int>(outfile, 0);
    }
    if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
        writeValue<char>(outfile, 0);
    }

    writeRmgReservedBytes<31>(outfile);

    std::bitset<144> disabledArtifacts;
    for (int artifactIndex = 0; artifactIndex < 144; ++artifactIndex) {
        disabledArtifacts[artifactIndex] =
            g_artifactTraits[artifactIndex].m_comboType != -1;
    }
    // Retail 0x54a78d/0x54a790 sets bit 0 of word 4 and bit 31 of
    // word 3, relative to the bitset base at [ebp-0x64]: IDs 128 and 127.
    disabledArtifacts.set(128);
    disabledArtifacts.set(127);

    if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
        writePackedBits(outfile, disabledArtifacts);
    } else if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
        std::bitset<129> legacyDisabledArtifacts;
        std::copy(
            bitset_iterator<144>(disabledArtifacts, 0),
            bitset_iterator<144>(disabledArtifacts, 129),
            bitset_iterator<129>(legacyDisabledArtifacts, 0));

        writePackedBits(outfile, legacyDisabledArtifacts);
    }

    if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
        std::bitset<70> disabledSpells;
        writePackedBits(outfile, disabledSpells);

        std::bitset<28> disabledSkills;
        writePackedBits(outfile, disabledSkills);

        for (int hero = 0; hero < 156; ++hero) {
            writeValue<char>(outfile, 0);
        }
    }
}

VA(0x0054AB40, 0xAD)
MAC_ADDRESS(0x24e6b0, 0x124)
static void __fastcall assignRmgTeams(
    int teamCount,
    int playerCount,
    int firstTeam,
    const b8* players,
    char* teams)
{
    int playersPerTeam[8];
    int team;

    for (team = 0; team < teamCount; ++team) {
        playersPerTeam[team] =
            playerCount / teamCount + (playerCount % teamCount > team);
    }

    for (int player = 0; player < 8; ++player) {
        if (!players[player])
            continue;

        int nonemptyTeams = 0;
        for (team = 0; team < teamCount; ++team) {
            if (playersPerTeam[team] > 0)
                ++nonemptyTeams;
        }

        int selected = rand() % nonemptyTeams;
        for (team = 0; team < teamCount; ++team) {
            if (playersPerTeam[team] > 0 && --selected < 0)
                break;
        }

        teams[player] = firstTeam + static_cast<char>(team);
        --playersPerTeam[team];
    }
}

void __fastcall writeRmgObjectPrototype(TAbstractFile*, TObjectType*);

// Prototype slots 0/1 hold the first RANDOM_MONSTER/TERRAIN_HOLE entries;
// referenced prototypes start at 2. Keep prototype/object loop bounds live
// across serialization callbacks and retain the two trait-ordered object passes.
VA(0x0054ABF0, 0x235)
MAC_ADDRESS(0x24fd18, 0x350) // anchor-callee 0x54c05b + WriteMapHeader and map loops; retail-only
b8 type_random_map_generator::writeMap(TAbstractFile* outfile)
{
    TRmgMapPosition position;
    writeMapHeader(outfile);
    writeValue<int>(outfile, 0);
    TRmgMapItem* item = m_map.m_mapItems;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z)
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y)
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x) {
                item->write(outfile);
                ++item;
            }
    int prototypeCount = 2;
    for (int type = 0; type < 232; ++type)
        for (unsigned int index = 0; index < m_objectPrototypes[type].size(); ++index) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[type][index];
            if (static_cast<int>(properties->m_refCount) > 0)
                properties->m_prototypeIndex = prototypeCount++;
        }
    writeValue<int>(outfile, prototypeCount);
    writeRmgObjectPrototype(outfile, m_objectPrototypes[RANDOM_MONSTER][0]->m_prototype);
    writeRmgObjectPrototype(outfile, m_objectPrototypes[TERRAIN_HOLE][0]->m_prototype);
    for (int objectType = 0; objectType < 232; ++objectType)
        for (unsigned int prototype = 0; prototype < m_objectPrototypes[objectType].size(); ++prototype) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[objectType][prototype];
            if (static_cast<int>(properties->m_refCount) > 0)
                writeRmgObjectPrototype(outfile, properties->m_prototype);
        }
    writeValue<int>(outfile, m_objects.size());
    for (unsigned int first = 0; first < m_objects.size(); ++first) {
        type_object* object = m_objects[first];
        if (g_adventureObjectTraits[object->m_properties->m_prototype->getObjectType()].m_trait3)
            object->write(outfile, m_mapVersion);
    }
    for (unsigned int second = 0; second < m_objects.size(); ++second) {
        type_object* object = m_objects[second];
        if (!g_adventureObjectTraits[object->m_properties->m_prototype->getObjectType()].m_trait3)
            object->write(outfile, m_mapVersion);
    }
    if (m_progress)
        m_progress->advance(2000);
    b8 result = writeValue<int>(outfile, 0) == sizeof(int);
    return result;
}

enum TRmgPrototypeCellMask {
    RMG_PROTOTYPE_PASSABLE_CELLS,
    RMG_PROTOTYPE_TRIGGER_CELLS
};

// H3M stores each fixed 8x6 footprint in reverse row/column order, packed
// least-significant bit first. Both masks use the same traversal and packing;
// keep their buffers and stream writes at the caller's separate lifetimes.
static inline void encodeRmgPrototypeCellMask(TObjectType* prototype,
    TRmgPrototypeCellMask kind, unsigned char* mask)
{
    memset(mask, 0, 6);
    int bit = 0;
    for (int y = 6; y--;)
        for (int x = 7; x >= 0; --x) {
            if (kind == RMG_PROTOTYPE_PASSABLE_CELLS
                ? prototype->isPassableCell(x, y) : prototype->isTriggerCell(x, y))
                mask[bit / 8] |= 1 << (bit % 8);
            ++bit;
        }
}

VA(0x0054AE30, 0x2C5)
MAC_ADDRESS(0x24f980, 0x398)
void __fastcall writeRmgObjectPrototype(TAbstractFile* outfile, TObjectType* prototype)
{
    unsigned char terrainMask[2];
    int nameLength = prototype->getImageName().size();
    writeValue<int>(outfile, nameLength);
    outfile->write(prototype->getImageName().c_str(), nameLength);
    {
        unsigned char mask[6];
        encodeRmgPrototypeCellMask(prototype, RMG_PROTOTYPE_PASSABLE_CELLS, mask);
        outfile->write(mask, sizeof(mask));
    }
    {
        unsigned char mask[6];
        encodeRmgPrototypeCellMask(prototype, RMG_PROTOTYPE_TRIGGER_CELLS, mask);
        outfile->write(mask, sizeof(mask));
    }
    {
        encodePackedBits(prototype->m_terrainMask, terrainMask);
        outfile->write(terrainMask, sizeof(terrainMask));
    }
    {
        encodePackedBits(prototype->m_recommendedTerrainMask, terrainMask);
        outfile->write(terrainMask, sizeof(terrainMask));
    }
    writeValue<int>(outfile, prototype->getObjectType());
    writeValue<int>(outfile, prototype->getSubtype());
    writeValue<char>(outfile, prototype->m_slotCategory);
    writeValue<char>(outfile, prototype->isUnderlay());
    int reserved[4];
    memset(reserved, 0, sizeof(reserved));
    outfile->write(reserved, sizeof(reserved));
}

VA(0x0054B100, 0x71)
MAC_ADDRESS(0x250068, 0xd4)
int type_random_map_generator::selectPrisonHero()
{
    int available = 0;
    int hero;
    for (hero = (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE ? 145 : 128) - 1; hero >= 0; --hero) {
        if (!m_disabledHeroes[hero])
            ++available;
    }
    if (!available)
        return -1;

    int selected = rand() % available;
    for (hero = (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE ? 145 : 128) - 1; hero >= 0; --hero) {
        if (!m_disabledHeroes[hero]) {
            --selected;
            if (selected < 0)
                break;
        }
    }
    m_disabledHeroes[hero] = true;
    return hero;
}

// This one-vector worklist reads priorities from the zones themselves;
// the other zone overload carries a parallel cost vector. Retail 0x54b294
// expands the same descending binary-search boundary before count insert.
static void insertRmgWorkItem(std::vector<TRmgZone*>& zones, TRmgZone* zone)
{
    int first = 0;
    int last = zones.size();
    int middle;
    int priority = zone->m_questPlacementScore;
    while (1) {
        middle = (first + last) >> 1;
        if (first >= last)
            break;
        if (priority < zones[middle]->m_questPlacementScore)
            first = middle + 1;
        else
            last = middle;
    }
    zones.insert(zones.begin() + middle, 1, zone);
}

// Unit-edge shortest paths use a descending priority worklist. Keep its tie
// order even though FIFO BFS also computes distances. Template destinations
// index the generated zones through their 0x1c-byte connection records.
VA(0x0054B180, 0x174)
MAC_ADDRESS(0x25013c, 0x210) // anchor-callee + zone/template layouts; retail-only
void type_random_map_generator::calculateQuestZoneDistances(TRmgZone* origin)
{
    std::vector<TRmgZone*> pending;
    for (unsigned int index = 0; index < m_zones.size(); ++index)
        m_zones[index]->m_questPlacementScore = 20000;
    origin->m_questPlacementScore = 0;
    pending.push_back(origin);
    while (pending.size()) {
        TRmgZone* current = pending.back();
        pending.pop_back();
        TRmgTemplateZone* slot = current->m_templateZone;
        int distance = current->m_questPlacementScore + 1;
        for (unsigned int connection = 0; connection < slot->m_connections.size(); ++connection) {
            TRmgZone* next = m_zones[slot->m_connections[connection].m_destination->m_zoneIndex];
            if (next->m_questPlacementScore > distance) {
                next->m_questPlacementScore = distance;
                insertRmgWorkItem(pending, next);
            }
        }
    }
}

// Stable ascending insertion preserves zone order on ties. Random scores
// are drawn for excluded zones too.
VA(0x0054B300, 0x18B)
MAC_ADDRESS(0x25034c, 0x23c) // anchor-callee + placement call + zone fields; retail-only
b8 type_random_map_generator::placeQuestGroup(
    TRmgTreasureGroup* group, TRmgZone* origin)
{
    std::vector<TRmgZone*> candidates;
    TRmgZone* candidateZone;
    calculateQuestZoneDistances(origin);
    for (unsigned int index = 0; index < m_zones.size(); ++index) {
        candidateZone = m_zones[index];
        int distance = candidateZone->m_questPlacementScore;
        if (distance == 1)
            candidateZone->m_questPlacementScore = 1000 + rand() % 10;
        else {
            int priority = distance * 10 + rand() % 10;
            candidateZone->m_questPlacementScore = priority;
        }
    }
    for (index = 0; index < m_zones.size(); ++index) {
        candidateZone = m_zones[index];
        if (candidateZone == origin || candidateZone->m_templateZone->m_kind == RMG_TEMPLATE_JUNCTION
            || candidateZone->m_questPlacementScore > 2000 || candidateZone->m_terrain == eTerrainWater)
            continue;
        unsigned int insertion = 0;
        while (insertion < candidates.size()
            && candidateZone->m_questPlacementScore >= candidates[insertion]->m_questPlacementScore)
            ++insertion;
        candidates.insert(candidates.begin() + insertion, candidateZone);
    }
    for (index = 0; index < candidates.size(); ++index) {
        candidateZone = candidates[index];
        if (placeTreasureGroup(group, candidateZone, 1))
            return true;
    }
    return false;
}

// Artifact's table loader maps class 'T' to bit 2. The quest worker tests
// that same treasure-class bit at 0x54b4db/0x54b536. No separate data body.
static const int g_rmgQuestArtifactClass = 2;

// Counting and selecting must use the same quest-artifact eligibility rule.
// Preserve the disabled/used/class short-circuit order and the separate
// unchecked prototype lookup after selection.
static inline bool isRmgQuestArtifact(int artifact, const b8* usedArtifacts)
{
    return !g_artifactTraits[artifact].m_disabled && !usedArtifacts[artifact]
        && (g_artifactTraits[artifact].m_artifactClass & g_rmgQuestArtifactClass);
}

// Mac 0x2507c4/0x2507dc retains TRmgTreasureGroup::addObject and
// markPlacementOutline; preparePlacement preserves the latter call.
VA(0x0054B490, 0x42E)
MAC_ADDRESS(0x250588, 0x360) // anchor-caller + artifact/group/generator fields; retail-only
b8 type_random_map_generator::placeQuestArtifact(rmgQuestArtifactObject* object)
{
    rmgSeerHutObject* seerHut = object->m_seerHut;
    int available = 0;
    int artifact;
    for (artifact = 0; artifact < ARTIFACT_COUNT; ++artifact) {
        if (isRmgQuestArtifact(artifact, m_usedQuestArtifacts)) {
            ++available;
        }
    }
    if (available < 20)
        m_questArtifactPoolLow = true;
    if (!available)
        return false;
    int selected = rand() % available;
    for (artifact = 0; artifact < ARTIFACT_COUNT; ++artifact) {
        if (isRmgQuestArtifact(artifact, m_usedQuestArtifacts)) {
            if (selected-- <= 0)
                break;
        }
    }
    seerHut->m_artifact = artifact;
    unsigned int prototypeIndex = findRmgPrototypeSubtypeIndex<unsigned int>(
        m_objectPrototypes[ARTIFACT], artifact);
    // Retail assumes an eligible artifact always has a loaded prototype;
    // there is no end-of-vector check after the subtype search.
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[ARTIFACT][prototypeIndex];
    --object->m_properties->m_refCount;
    object->m_properties = properties;
    ++properties->m_refCount;
    TRmgZone* origin = m_zones[m_map.getMapItem(object->m_position)->m_zoneState.m_zone];
    TRmgTreasureGroup group(16, 16);
    TObjectType* prototype = seerHut->m_properties->m_prototype;
    TRmgMapPosition position;
    position = getRmgCenteredGroupObjectPosition(&group, prototype);
    type_object* questObject = seerHut;
    group.addObject(questObject, position);
    group.preparePlacement();
    if (!placeQuestGroup(&group, origin)) {
        int value = object->m_definition->getValue(origin, this);
        TRmgMapPosition originalPosition = object->m_position;
        replaceRmgObjectWithTreasure(this, object, value, originalPosition);
        return false;
    }
    m_usedQuestArtifacts[artifact] = true;
    m_nextSeerHutPrototypeIndex = (m_nextSeerHutPrototypeIndex + 1)
        % m_objectPrototypes[SEER].size();
    return true;
}

// Reserve the color before filling the group and release it on failure.
// Failed placement releases each object's reservation before deletion.
// The initial key-tent color remains unset, as in retail.
VA(0x0054B8C0, 0x385)
MAC_ADDRESS(0x2508e8, 0x334) // anchor-callee 0x5338e0; retail-only
b8 type_random_map_generator::placeKeyTentGuard(type_object* object, int maxValue)
{
    int color = object->m_properties->m_prototype->getSubtype();
    unsigned int index = findRmgPrototypeSubtypeIndex<unsigned int>(
        m_objectPrototypes[BORDER_GUARD], color);
    if (index == m_objectPrototypes[BORDER_GUARD].size())
        return false;
    TRmgZone* origin = m_zones[m_map.getMapItem(object->m_position)->m_zoneState.m_zone];
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[BORDER_GUARD][index];
    TRmgTreasureGroup group(16, 16);
    type_object* guard = new type_object(properties);
    setKeyTentColorDisabled(color, true);
    if (fillTreasureGroup(origin, &group, false, maxValue) && group.addGuard(guard)) {
        group.preparePlacement();
        if (placeQuestGroup(&group, origin))
            return true;
    } else {
        delete guard;
    }
    discardRmgTreasureGroup(&group);
    setKeyTentColorDisabled(color, false);
    return false;
}

// Callers retain ownership of the object. Both retail find loops test the
// iterator against null, including the end-iterator path (0x54bc95/0x54be6d).
// Preserve that bug and the footprint's position/prototype snapshots.
VA(0x0054BC50, 0x2AE)
MAC_ADDRESS(0x250c1c, 0x2ac) // anchor-callee 0x5338e0/0x54b490; retail-only
void type_random_map_generator::removeObject(type_object* object)
{
    TObjectType* prototype = object->m_properties->m_prototype;
    TRmgMapPosition position;
    position = object->m_position;
    std::vector<type_object*>::iterator found = std::find(m_objects.begin(), m_objects.end(), object);
    if (found) {
        m_objects.erase(found);
        --m_objectCountByType[prototype->getObjectType()];
        TAdventureObjectType objectType = prototype->getObjectType();
        TObjectType::TPoint trigger = prototype->m_triggerCell;
        TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, trigger);
        int zone = m_map.getMapItem(entrance.m_x,
            entrance.m_y, entrance.m_z)->m_zoneState.m_zone;
        if (zone >= 0) {
            m_zones[zone]->decrementObjectCount(objectType);
        }
    }
    if (prototype->getObjectType() == BORDER_GUARD) {
        setKeyTentColorDisabled(prototype->getSubtype(), false);
    }
    TRmgGridPoint cell;
    TRmgMapPosition mapPosition;
    mapPosition.m_z = position.m_z;
    for (cell.m_y = 0; cell.m_y < prototype->getHeight(); ++cell.m_y) {
        mapPosition.m_y = position.m_y - cell.m_y;
        if (mapPosition.m_y < 0 || mapPosition.m_y >= m_map.m_mapHeight)
            continue;
        for (cell.m_x = 0; cell.m_x < prototype->getWidth(); ++cell.m_x) {
            mapPosition.m_x = position.m_x - cell.m_x;
            if (mapPosition.m_x < 0 || mapPosition.m_x >= m_map.m_mapWidth)
                continue;
            if (isRmgObjectFootprintCell(prototype, cell.m_x, cell.m_y)) {
                TRmgMapItem* item = m_map.getMapItem(mapPosition);
                std::vector<type_object*>::iterator entry = std::find(item->m_objects.begin(), item->m_objects.end(), object);
                if (entry) {
                    item->m_objects.erase(entry);
                    if (item->m_objects.empty()) {
                        item->m_tileData.m_roadEntrance = false;
                        item->m_tileData.m_roadPassable = true;
                    }
                    item->m_zoneState.m_score = 32700;
                }
            }
        }
    }
}

VA(0x0054BF00, 0x57)
MAC_ADDRESS(0x250fe0, 0x90) // anchor-callee 0x5862e8; Complete-only, thiscall ret 0xc
TRandomMapRequest::TRandomMapRequest(int width, int height, int levels)
    : m_width(width), m_height(height), m_levels(levels),
      m_humanPlayerCount(2), m_humanTeamCount(2),
      m_computerPlayerCount(0), m_computerTeamCount(8),
      m_mapVersion(RMG_MAP_SHADOW_OF_DEATH)
{
    m_monsterStrength = 0;
    m_waterContent = RMG_WATER_RANDOM;
    memset(m_isHumanSeat, 0, sizeof(m_isHumanSeat));
    memset(m_townType, -1, sizeof(m_townType));
}

// Inferred ordinary seat setters expanded by retail worker 0x54bf60.
void type_random_map_generator::setHumanPlayer(int seat)
{
    m_fixedHumanPlayers[seat] = true;
}

void type_random_map_generator::setTownChoice(int seat, int town)
{
    m_townChoices[seat] = town;
}

VA(0x0054BF60, 0x130)
MAC_ADDRESS(0x251070, 0x140)
int TRandomMapRequest::generateToFile(TAbstractFile* outfile, TProgressSink* progress)
{
    int strength = m_monsterStrength + 3;
    if (strength < 1)
        strength = 1;
    if (strength > 5)
        strength = 5;
    if (m_humanPlayerCount + m_computerPlayerCount < 2) {
        m_humanPlayerCount = 1;
        m_computerPlayerCount = 1;
    }
    type_random_map_generator generator(m_width, m_height, m_levels,
        m_humanPlayerCount, m_humanTeamCount, m_computerPlayerCount,
        m_computerTeamCount, m_waterContent, strength,
        progress, m_mapVersion);
    for (int seat = 0; seat < 8; ++seat) {
        if (m_isHumanSeat[seat])
            generator.setHumanPlayer(seat);
        generator.setTownChoice(seat, m_townType[seat]);
    }
    if (!generator.generate())
        return RANDOM_MAP_GENERATION_FAILED;
    int result = RANDOM_MAP_OK;
    if (!generator.writeMap(outfile))
        result = RANDOM_MAP_WRITE_FAILED;
    return result;
}

VA(0x0054C090, 0x8C)
MAC_ADDRESS(0x2511b0, 0x98)
int TRandomMapRequest::generate(const char* fileName, TProgressSink* progress)
{
    try {
        TGzFile outfile(fileName, "wb6");
        return generateToFile(&outfile, progress);
    } catch (const TGzFile::TOpenFailure&) {
        return RANDOM_MAP_OPEN_FAILED;
    }
}

// Retail carveBranchingPaths unwind thunk 0x62fdb8 passes its queue at
// EBP-0x68 to 0x54c6a0, whose destructor expands the list teardown.
// Eight-byte coordinate values and 16-byte linked nodes identify list<TPoint>;
// erase's iterator result is returned through a hidden stack pointer.
VA_COMPGEN(0x0054C6A0, 0x4D, QUEUE_LIST_DTOR, TPoint)

VA_COMPGEN(0x0054D000, 0x5E, LIST_INSERT_SINGLE, TPoint)

VA_COMPGEN(0x0054D060, 0x36, LIST_ERASE_ITERATOR, TPoint)

VA_COMPGEN(0x0054D0A0, 0x45, LIST_ERASE_RANGE, TPoint)

VA_COMPGEN(0x0054D0F0, 0x2D, LIST_BUYNODE, TPoint)

// Mine's non-const terrain query retains the checked ten-bit test.
VA_COMPGEN(0x005166E0, 0x34, BITSET_TEST, Bitset10)

// Retail's 129-bit setter is claimed from its identical canonical COMDAT in
// customcampaign.cpp.

// The three-point orientation helper at 0x5fdae0 belongs with the retained
// Voronoi operations in rmg_support.cpp.
VA(0x005FDB10, 0x21)
MAC_ADDRESS(0x25c3b4, 0x38) // anchor-callee addSite; Complete-only, ret 0x10
int getRmgSquaredDistance(TPoint first, TPoint second)
{
    int dy = first.m_y - second.m_y;
    int dx = first.m_x - second.m_x;
    return getRmgSquaredNorm(dx, dy);
}

// Circumcenter of the triangle with these three sites: the perpendicular
// bisector of the origin-to-second side, scaled by the projected sides.
// Retail expands the first subtraction inside this ordinary helper, retaining
// two later subtractions and five arithmetic calls. Keep the caller's opposite-
// site snapshot, by-value parameters, and dot products' Y-before-X order.
static TPoint computeRmgCircumcenter(TPoint third, TPoint origin, TPoint second)
{
    TRmgVector axis = second - origin;
    TRmgVector perpendicular(-axis.m_y, axis.m_x);
    TRmgVector secondSide = third - second;
    TRmgVector thirdSide = origin - third;
    return origin + (axis + perpendicular * secondSide.dot(thirdSide)
        / perpendicular.dot(thirdSide)) / 2;
}

VA(0x005FDB40, 0x16E)
MAC_ADDRESS(0x25d144, 0x12c) // anchor-caller 0x53e050; Complete-only, thiscall ret 0
void TRmgVoronoi::buildVertices()
{
    for (unsigned int index = 0; index < m_edges.size(); ++index) {
        TRmgHalfEdge* edge = m_edges[index];
        if (edge->getZone() && !edge->isPositionComputed()) {
            TPoint second = edge->getOppositeSitePosition();
            TPoint position = computeRmgCircumcenter(edge->getNext()->getOppositeSitePosition(),
                edge->getSitePosition(), second);
            edge->setPosition(position);
            edge = edge->getLeftPrevious();
            edge->setPosition(position);
            edge = edge->getLeftPrevious();
            edge->setPosition(position);
        }
    }
}

VA(0x005FDCB0, 0x1E) // caller 0x5fdc49; thiscall, hidden result + eight-byte operand
TRmgVector TRmgVector::operator+(TRmgVector other) const
{
    return TRmgVector(m_x + other.m_x, m_y + other.m_y);
}

VA(0x005FDCD0, 0x1D) // caller 0x5fdc2f; thiscall, ret 8
TRmgVector TRmgVector::operator*(int scale) const
{
    TRmgVector result;
    result.m_x = m_x * scale;
    result.m_y = m_y * scale;
    return result;
}

VA(0x005FDCF0, 0x25) // callers 0x5fdc36/0x5fdc50; signed division, ret 8
TRmgVector TRmgVector::operator/(int divisor) const
{
    return TRmgVector(m_x / divisor, m_y / divisor);
}

VA(0x005FDD20, 0x20)
TPoint operator+(TPoint point, TRmgVector offset)
{
    return TPoint(point.m_x + offset.m_x, point.m_y + offset.m_y);
}

VA(0x005FDD40, 0x20)
TRmgVector operator-(TPoint left, TPoint right)
{
    return TRmgVector(left.m_x - right.m_x, left.m_y - right.m_y);
}
