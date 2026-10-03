// Random-map generator.

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

// Euclidean distance, truncated; the squares are 32-bit integers.
MAC_ADDRESS(0x22cef0, 0x84)
int getRmgDistance(TPoint first, TPoint second)
{
    return static_cast<int>(sqrt(static_cast<double>(
        getRmgSquaredDistance(first, second))));
}

DATA(0x006824E0)
s32 g_rmgCreatureValueByLevel[7] = {5000, 7000, 9000, 12000, 16000, 21000, 27000};

void type_object::releaseReservation() {}
b8 type_object::completePlacement() { return true; }

b8 type_treasure_def::isTerrainDependent() { return false; }
b8 type_quest_creature_def::isTerrainDependent() { return true; }
b8 type_quest_experience_def::isTerrainDependent() { return true; }
b8 type_quest_gold_def::isTerrainDependent() { return true; }
b8 type_key_tent_def::isTerrainDependent() { return true; }

// Per-type object limits for the whole map and for each zone.
DATA(0x0069CE4C)
s32 g_rmgMapObjectLimits[ADVENTURE_OBJECT_TRAIT_COUNT];
DATA(0x0069D1F4)
s32 g_rmgZoneObjectLimits[ADVENTURE_OBJECT_TRAIT_COUNT];
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

// Frame i of a river sprite draws shape g_rmgRiverPatterns[i]; repeated
// shapes are alternative frames picked at random.
DATA(0x00641140)
static const s32 g_rmgRiverPatterns[13] = {
    LINE_SE, LINE_SE, LINE_SE, LINE_SE, LINE_CROSS, LINE_ESW, LINE_ESW,
    LINE_NES, LINE_NES, LINE_NS, LINE_NS, LINE_EW, LINE_EW
};
DATA(0x0069E5D0)
TRmgLinePatternTable g_rmgRiverPatternTable(13, g_rmgRiverPatterns);

VA_COMPGEN(0x0055ED70, 0x1D, STATIC_CTOR, g_rmgRiverPatternTable)

VA_COMPGEN(0x0055ED90, 0x0A, STATIC_DTOR, g_rmgRiverPatternTable)

// The same for road sprites.
DATA(0x006411AC)
static const s32 g_rmgRoadPatterns[17] = {
    LINE_SE, LINE_SE, LINE_SE_VARIANT, LINE_SE_VARIANT, LINE_SE_VARIANT, LINE_SE_VARIANT,
    LINE_NES, LINE_NES, LINE_ESW, LINE_ESW, LINE_NS, LINE_NS, LINE_EW, LINE_EW,
    LINE_END_S, LINE_END_E, LINE_CROSS
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

VA(0x00530E80, 0x0D)
MAC_ADDRESS(0x22cfd4, 0x8)
void TProgressSink::setTotal(int totalSteps)
{
    m_steps = totalSteps;
}

namespace {

// Indices of g_rmgDirections, clockwise from east; y grows southward.
// North is up; . is the centre cell.
//   5 6 7
//   4 . 0
//   3 2 1
enum ERmgDirection {
    RMG_DIRECTION_EAST = 0,
    RMG_DIRECTION_SOUTH_EAST = 1,
    RMG_DIRECTION_SOUTH = 2,
    RMG_DIRECTION_SOUTH_WEST = 3,
    RMG_DIRECTION_WEST = 4,
    RMG_DIRECTION_NORTH_WEST = 5,
    RMG_DIRECTION_NORTH = 6,
    RMG_DIRECTION_NORTH_EAST = 7,
    // From here on (NW, N, NE) the directions step north.
    RMG_FIRST_NORTHERN_DIRECTION = RMG_DIRECTION_NORTH_WEST
};

// Eight neighbour directions, clockwise from east; even entries are the
// four cardinal directions.
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

// Rotates by 45-degree steps, clockwise when positive; wraps around the
// eight directions.
static inline int turnRmgDirection(int direction, int steps)
{
    return (direction + steps) & (RMG_DIRECTION_COUNT - 1);
}

static inline int getRmgOppositeDirection(int direction)
{
    return turnRmgDirection(direction, 4);
}

// SE, S or SW.
static inline bool isRmgSouthwardDirection(int direction)
{
    return direction >= RMG_DIRECTION_SOUTH_EAST
        && direction <= RMG_DIRECTION_SOUTH_WEST;
}

// Most object entrances are entered and left only through the cells beside
// and below them; these object types also allow the three cells above.
static inline bool isRmgEntranceOpenToNorth(int objectType)
{
    return g_adventureObjectTraits[objectType].m_trait1 != 0;
}

// Shipyards are three tiles wide; these offsets probe beside the left and
// right ends of the bottom footprint row and the row below it.
// North is up; digits are offset indices, P is the shipyard position,
// # the rest of its bottom row and . the row below.
//   0 # # P 1
//   2 . . . 3
DATA(0x0069CE00)
TPoint g_rmgShipyardWaterOffsets[RMG_SHIPYARD_WATER_OFFSET_COUNT] = {
    TPoint(-3, 0),
    TPoint(1, 0),
    TPoint(-3, 1),
    TPoint(1, 1)
};

DATA(0x006409A0)
static const s32 g_landRiverDeltaIndex[4] = {2, 0, 3, 1};

DATA(0x006409B0)
static const s32 g_snowRiverDeltaIndex[4] = {7, 5, 4, 6};

// Candidate town types for each zone terrain; -1 was meant to end a row.
DATA(0x00682450)
s32 g_rmgTerrainTownChoices[9][4] = {
    {0, 1, 4, -1}, {6, -1, 0, 0}, {0, 1, -1, 0},
    {2, -1, 0, 0}, {7, 4, -1, 0}, {6, 8, -1, 0},
    {5, 3, 4, -1}, {3, -1, 0, 0}, {-1, 0, 0, 0}
};

// Native terrain of each town alignment, used when choosing zone terrain.
DATA(0x006408C8)
static const s32 g_rmgTownNativeTerrains[TOWN_TYPE_COUNT] = {
    eTerrainGrass, eTerrainGrass, eTerrainSnow, eTerrainLava, eTerrainDirt,
    eTerrainDirt, eTerrainRough, eTerrainSwamp, eTerrainGrass
};

enum ERmgRadialDirectionLimits {
    RMG_RADIAL_DIRECTION_COUNT = 32
};

// Radial directions used by the placement and boundary passes.
DATA(0x00682500)
double g_rmgDirectionCosines[RMG_RADIAL_DIRECTION_COUNT] = {
    1.0, 0.9807, 0.9239, 0.8315, 0.7071, 0.5556, 0.3827, 0.1951,
    0.0, -0.1951, -0.3827, -0.5556, -0.7071, -0.8315, -0.9239, -0.9807,
    -1.0, -0.9807, -0.9239, -0.8315, -0.7071, -0.5556, -0.3827, -0.1951,
    0.0, 0.1951, 0.3827, 0.5556, 0.7071, 0.8315, 0.9239, 0.9807
};
DATA(0x00682600)
double g_rmgDirectionSines[RMG_RADIAL_DIRECTION_COUNT] = {
    0.0, 0.1951, 0.3827, 0.5556, 0.7071, 0.8315, 0.9239, 0.9807,
    1.0, 0.9807, 0.9239, 0.8315, 0.7071, 0.5556, 0.3827, 0.1951,
    0.0, -0.1951, -0.3827, -0.5556, -0.7071, -0.8315, -0.9239, -0.9807,
    -1.0, -0.9807, -0.9239, -0.8315, -0.7071, -0.5556, -0.3827, -0.1951
};

// Guard value thresholds and scales by monster strength (0-5).
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
static const char* g_rmgPlayerNames[RMG_PLAYER_COUNT] = {
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
static const char* g_rmgTownNames[TOWN_TYPE_COUNT] = {
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

static bool isRmgTemplateFieldSet(const char* value)
{
    return value && value[0] && value[0] != ' ';
}

// rmg.txt and rand_trn.txt rows start after three header rows.
enum ERmgSpreadsheetLayout {
    RMG_FIRST_DATA_ROW = 3
};

} // namespace

static void __fastcall assignRmgTeams(
    int teamCount,
    int playerCount,
    int firstTeam,
    const b8* players,
    char* teams);

template <u32 N>
static void setAvailableRmgHeroes(
    std::bitset<N>* availableHeroes,
    b8* heroFlag,
    b8* end)
{
    std::transform(heroFlag, end,
        bitset_iterator<N>(*availableHeroes, 0),
        std::logical_not<b8>());
}

VA_COMPGEN(0x00530F80, 0x21, SCALAR_DELETING_DTOR, type_random_map)

VA(0x00530E90, 0x4A)
MAC_ADDRESS(0x22cfdc, 0x50)
TRmgMapItem::TRmgMapItem()
{
    clear();
}

VA_COMPGEN(0x00530EE0, 0x26, IMPLICIT_DTOR, TRmgMapItem)
MAC_COMPGEN_ADDRESS(0x22d02c, 0x84, IMPLICIT_DTOR, TRmgMapItem)

// First plain (shape 0) water frame.
enum ERmgWaterFrames {
    RMG_WATER_BASE_FRAME = 21
};

// Resets the cell to empty water. The guard colour, connection-visited flag
// and previous-tile Y/Z are left unchanged.
VA(0x00530F10, 0x6F)
MAC_ADDRESS(0x22d110, 0x15c)
void TRmgMapItem::clear()
{
    m_objects.clear();
    TRmgConnectionDecoration connection = m_connection;
    TRmgGroundTileData tileData = m_tileData;

    connection.m_present = false;
    m_tile.m_landType = eTerrainWater;
    m_tile.m_terrainFrame = RMG_WATER_BASE_FRAME;
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
    tileData.m_nearRiver = false;
    m_connection = connection;
    m_movement.m_cost = RMG_CLEARED_CELL_COST;
    m_movement.m_zonePathCost = RMG_CLEARED_CELL_COST;
    m_zoneState.m_objectDistance = RMG_CLEARED_CELL_COST;
    m_zoneState.m_zone = -1;
    m_zoneState.m_connectionZone = -1;
    m_previousTile.m_x = -1;
    m_tileData = tileData;
}

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

// Shared by the footprint and surrounding-outline placement tests; border,
// water and path-clearance policies stay with each caller.
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
    // Counts blocked runs around the outline, revisiting the first point to
    // close the last run. The outline must be nonempty.
    b8 blocked = true;
    b8 foundBoundary = false;
    b8 waterZone = zone->m_terrain == eTerrainWater;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    for (u32 index = 0; index < outline.size() + 1; ++index) {
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

// Binary search in a cost-descending worklist. Equal costs go before older
// entries, so popping from the back processes ties oldest first.
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

static inline void popRmgWorkItem(TRmgMapPosition& position,
    std::vector<TRmgMapPosition>& positions, std::vector<int>& costs)
{
    position = positions.back();
    costs.pop_back();
    positions.pop_back();
}

// Seed a zero-cost source with no predecessor in both worklists and the
// movement map.
static inline TRmgMapItem* seedRmgMovementSearch(type_random_map& map,
    const TRmgMapPosition& source,
    std::vector<TRmgMapPosition>& positions, std::vector<int>& costs)
{
    positions.push_back(source);
    costs.push_back(0);
    TRmgMapItem* item = map.getMapItem(source);
    item->setMovementCost(0, TRmgMapPosition(-1, -1, -1));
    return item;
}

// Floods movement costs out from a seed cell. Steps onto water or outside the
// seed's zone cost 10, others 1. The flood spills into neighbouring zones as
// zone-path costs but never re-enters the seed zone or reaches a third zone.
VA(0x00531460, 0x441)
MAC_ADDRESS(0x22d9ac, 0x5cc)
void type_random_map::floodConnectionCosts(TRmgMapPosition position, b8 waterZone)
{
    // Clear cells beside a zero-cost cell also cost zero; this is applied
    // after the improvement test.
    std::vector<int> costs;
    std::vector<TRmgMapPosition> positions;
    TRmgMapItem* seed = seedRmgMovementSearch(*this, position,
        positions, costs);
    int zone = seed->m_zoneState.m_zone;
    while (positions.size()) {
        TRmgMapPosition currentPosition;
        popRmgWorkItem(currentPosition, positions, costs);
        TRmgMapItem* current = getMapItem(currentPosition);
        int currentZone = current->m_zoneState.m_zone;
        int currentCost = currentZone == zone
            ? current->m_movement.m_cost : current->m_movement.m_zonePathCost;
        int direction = RMG_DIRECTION_COUNT;
        if (current->isRoadEntrance()) {
            int objectType = current->getEntranceObjectType();
            if (!isRmgEntranceOpenToNorth(objectType))
                direction = RMG_FIRST_NORTHERN_DIRECTION;
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
                int objectType = next->getEntranceObjectType();
                const TAdvObjectTraits& traits = g_adventureObjectTraits[objectType];
                if (traits.m_blocksLanding && !traits.m_trait2)
                    continue;
                if (!isRmgEntranceOpenToNorth(objectType)
                    && isRmgSouthwardDirection(direction))
                    continue;
            }
            if (next->m_zoneState.m_zone != zone) {
                nextCost = currentCost + 10;
                if (currentZone != zone && currentZone != next->m_zoneState.m_zone)
                    continue;
                if (next->m_movement.m_zonePathCost <= nextCost)
                    continue;
                next->m_movement.m_zonePathCost = nextCost;
                next->m_tileData.m_connectionDirection =
                    getRmgOppositeDirection(direction);
                next->m_zoneState.m_connectionZone = zone;
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

static inline bool isRmgWaterOnlyPrototype(const TObjectType& prototype)
{
    return prototype.m_slotCategory == TObjectType::SLOT_CATEGORY_0
        && prototype.m_recommendedTerrainMask.test(eTerrainWater);
}

// Checks the footprint against map bounds, zones and entrances. Only trigger
// cells honour rejectBorder; only blocked cells apply the water rule
// (water-only objects must stand on water, others on land).
VA(0x005318B0, 0x212)
MAC_ADDRESS(0x22dfe0, 0x2a4)
b8 type_random_map::isPlacementBlocked(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position,
    int zoneIndex, b8 rejectBorder)
{
    TObjectType& prototype = *properties->m_prototype;
    if (position.m_x < prototype.getWidth() - 1 || position.m_x >= m_mapWidth
        || position.m_y < prototype.getHeight() - 1 || position.m_y >= m_mapHeight)
        return true;
    TRmgMapPosition cell = position;
    for (u32 y = 0; y < prototype.getHeight(); ++y, --cell.m_y) {
        cell.m_x = position.m_x;
        for (u32 x = 0; x < prototype.getWidth(); ++x, --cell.m_x) {
            TRmgGridPoint maskPoint(x, y);
            TRmgMapItem* item = getMapItem(cell);
            if (prototype.isTriggerCell(maskPoint.m_x, maskPoint.m_y)) {
                if (isRmgPlacementCellBlocked(item, zoneIndex))
                    return true;
                if (rejectBorder && item->hasBorderObject())
                    return true;
            }
            if (!prototype.isPassableCell(maskPoint.m_x, maskPoint.m_y)) {
                if (isRmgPlacementCellBlocked(item, zoneIndex))
                    return true;
                if ((item->getLandType() == eTerrainWater)
                    != isRmgWaterOnlyPrototype(prototype))
                    return true;
            }
        }
    }
    return false;
}

VA(0x00531AD0, 0x100)
MAC_ADDRESS(0x22e284, 0x1d0)
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
MAC_ADDRESS(0x22e454, 0x254)
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

// Map position of an object's trigger cell. Footprint mask cell (column, row)
// lies at position - (column, row): the mask grows west and north from the
// object's bottom-right cell P. North is up.
//   (2,1) (1,1) (0,1)
//   (2,0) (1,0) (0,0)=P
static inline TRmgMapPosition getRmgObjectTriggerPosition(
    TRmgMapPosition position, const TObjectType::TPoint& trigger)
{
    TPoint triggerOffset(trigger.m_x, trigger.m_y);
    position -= triggerOffset;
    return position;
}

// Entrance cell of a placed object: its position offset by the trigger cell.
static inline TRmgMapPosition getRmgPlacedObjectEntrance(const type_object* object)
{
    return getRmgObjectTriggerPosition(object->getPosition(),
        object->m_properties->m_prototype->m_triggerCell);
}

static inline bool allowsRmgSharedObjectEntrance(int objectType)
{
    return g_adventureObjectTraits[objectType].m_trait2
        && isRmgEntranceOpenToNorth(objectType);
}

// The object must fit and have a connected outline, and the cell below its
// trigger must be passable land of the same zone and water class. If that
// cell is another entrance, its object type needs trait 2.
VA(0x00531CF0, 0x1A5)
MAC_ADDRESS(0x22e6a8, 0x270)
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
    TRmgMapPosition approach = getRmgObjectTriggerPosition(position, prototype.m_triggerCell);
    ++approach.m_y;
    if (approach.m_y >= m_mapHeight)
        return false;
    TRmgMapItem* item = getMapItem(approach);
    if (!item->isPassableLand())
        return false;
    if (item->m_zoneState.m_zone < 0)
        return false;
    if (item->m_zoneState.m_zone != zoneIndex)
        return false;
    if (item->isRoadEntrance()) {
        int entranceType = item->getEntranceObjectType();
        if (!g_adventureObjectTraits[entranceType].m_trait2)
            return false;
    }
    b8 result = (item->getLandType() == eTerrainWater) == (zone->m_terrain == eTerrainWater);
    return result;
}

// Puts the object on the map: trigger cells become entrances with open
// paths, other non-passable cells become object-blocked, and both record it.
VA(0x00531EA0, 0x2E6)
MAC_ADDRESS(0x22e918, 0x1c4)
void type_random_map::addObject(type_object& object, TRmgMapPosition position)
{
    TObjectType& prototype = *object.m_properties->m_prototype;
    object.m_position = position;
    TRmgMapPosition cell = position;
    for (u32 y = 0; y < prototype.getHeight(); ++y, --cell.m_y) {
        if (cell.m_y < 0 || cell.m_y >= m_mapHeight)
            continue;
        cell.m_x = position.m_x;
        for (u32 x = 0; x < prototype.getWidth(); ++x, --cell.m_x) {
            if (cell.m_x < 0 || cell.m_x >= m_mapWidth)
                continue;
            TRmgGridPoint maskPoint(x, y);
            TRmgMapItem* item = getMapItem(cell);
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

VA(0x00532200, 0x3C)
MAC_ADDRESS(0x22eb4c, 0x38)
void type_random_map::setFrame(const TRmgGridPoint& point, int value)
{
    TRmgMapItem& item = *getMapItem(point.m_x, point.m_y);
    item.m_tile.m_terrainFrame = value;
}

VA(0x00532240, 0x15)
MAC_ADDRESS(0x22eb84, 0x14)
#if defined(HOMM3_TARGET_MAC)
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
    int roadType = tile.m_terrain;
    item.m_tile.m_roadType = roadType;
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

TRmgGridPoint TRmgRoadMapAdapter::getSize()
{
#if defined(HOMM3_TARGET_MAC)
    return m_map->getSize();
#else
    return m_map->getSize(TRmgGridPoint());
#endif
}

VA_COMPGEN(0x00532320, 0x21, SCALAR_DELETING_DTOR, TRmgRoadMapAdapter)

VA(0x00532510, 0x07)
MAC_ADDRESS(0x22eeec, 0x48)
TRmgRiverMapAdapterInterface::~TRmgRiverMapAdapterInterface()
{
}

VA_COMPGEN(0x00537910, 0x23, SCALAR_DELETING_DTOR, TRmgRiverMapAdapterInterface)

VA_COMPGEN(0x005324E0, 0x21, SCALAR_DELETING_DTOR, TRmgRiverMapAdapter)

VA(0x00532520, 0x205)
MAC_ADDRESS(0x22ef34, 0x35c)
void TRmgRiverMapAdapter::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    TRmgMapItem& item = *m_map->getMapItem(point.m_x, point.m_y);
    b8 flipX = tile.m_flipX;
    int riverType = tile.m_terrain;
    b8 flipY = tile.m_flipY;
    int frame = tile.m_frame;
    item.m_tile.m_riverType = riverType;
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
                    neighbour.m_tileData.m_nearRiver = true;
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
MAC_ADDRESS(0x22f290, 0x54)
void TRmgRiverMapAdapter::setLineType(const TRmgGridPoint& point, int value)
{
    TRmgMapItem& item = *m_map->getMapItem(point.m_x, point.m_y);
    item.m_tile.m_riverType = value;
    b8 present = value != 0;
    item.m_tileData.m_hasRiver = present;
}

VA(0x00532790, 0x27)
MAC_ADDRESS(0x22f2e4, 0x3c)
TRmgGridPoint TRmgRiverMapAdapter::getSize()
{
#if defined(HOMM3_TARGET_MAC)
    return m_map->getSize();
#else
    return m_map->getSize(TRmgGridPoint());
#endif
}

VA(0x005327C0, 0x63)
MAC_ADDRESS(0x22f320, 0xa4)
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

// Picks a uniformly random allowed town type, or -1 when none is allowed.
MAC_ADDRESS(0x22f6c8, 0xb4)
int TRmgTemplateZone::selectAllowedTown()
{
    int allowedTownCount = 0;
    for (int town = 0; town < TOWN_TYPE_COUNT; ++town) {
        if (m_allowedTowns[town])
            ++allowedTownCount;
    }
    if (!allowedTownCount)
        return -1;
    int selectedIndex = rand() % allowedTownCount;
    for (town = 0; town < TOWN_TYPE_COUNT; ++town) {
        if (m_allowedTowns[town] && --selectedIndex < 0)
            return town;
    }
    return -1;
}

VA(0x005329E0, 0xCF)
MAC_ADDRESS(0x22f7c4, 0xc0)
TRmgZone::TRmgZone(TRmgTemplateZone* newSlot)
{
    m_templateZone = newSlot;
    m_alignment = newSlot->selectAllowedTown();
    m_scaledSize = newSlot->m_size;
    m_bounds.resetEmpty();
    m_active = false;
    memset(m_objectCountByType, 0, sizeof(m_objectCountByType));
}

MAC_ADDRESS(0x22f9d8, 0xcc)
void TRmgZone::chooseTownType(b8 expanded)
{
    if (m_alignment != -1) {
        m_townType2 = m_alignment;
    } else {
        int count = 0;
        // Retail bug: this condition is always true, so all four row
        // entries are candidates, including -1 and the zero padding.
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

static inline bool isRmgZoneTerrainAllowed(const TRmgZone& zone, int terrain)
{
    return zone.m_templateZone->m_allowedTerrain[terrain]
        && (terrain != eTerrainSubterranean
            || zone.m_levelPosition.m_z == RMG_UNDERGROUND_LEVEL);
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
    if (m_levelPosition.m_z == RMG_UNDERGROUND_LEVEL && m_terrain != eTerrainLava)
        m_terrain = eTerrainSubterranean;
}

VA(0x00532B50, 0x76)
MAC_ADDRESS(0x22fbd4, 0xa0)
TRmgZone::~TRmgZone()
{
}

void TRmgZone::decrementObjectCount(TAdventureObjectType objectType)
{
    --m_objectCountByType[objectType];
}

TRmgMapPosition TRmgZone::getLevelPosition() const
{
    TRmgMapPosition result;
    result = m_levelPosition;
    return result;
}

void TRmgZone::setLevelPosition(TRmgMapPosition position)
{
    m_levelPosition = position;
}

// Whether two zones are close enough to connect: on one level within 110%
// of their combined sizes; across levels the overlap must exceed half the
// smaller size.
VA(0x00532BD0, 0xA8)
MAC_ADDRESS(0x22fdc0, 0xc8)
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

static inline bool isRmgObjectFootprintCell(
    const TObjectType* prototype, u32 x, u32 y)
{
    return !prototype->isPassableCell(x, y) || prototype->isTriggerCell(x, y);
}

static inline TPoint nextRmgOutlineProbe(const TPoint& position, int& direction)
{
    direction = turnRmgDirection(direction, -2);
    TPoint offset = g_rmgDirections[direction];
    return TPoint(position.m_x + offset.m_x, position.m_y + offset.m_y);
}

static inline void advanceRmgOutlineWalk(TPoint& position, int& direction)
{
    position = position + TRmgVector(g_rmgDirections[direction].m_x,
        g_rmgDirections[direction].m_y);
    direction = getRmgOppositeDirection(direction);
}

// A full 3x2 footprint gets this ring, walked clockwise from the start S.
// North is up; # is the footprint, P its bottom-right cell, o the outline.
//   o o o o o
//   o # # # o
//   o # # P o
//   o o o S o
VA(0x00532C80, 0x1BA)
MAC_ADDRESS(0x22fe88, 0x208)
void TRmgObjectPropertiesRef::buildOutline()
{
    // Wall-follows the footprint with cardinal steps. Offsets are relative
    // to the object's bottom-right cell, so footprint cells are nonpositive.
    if (m_outline.size() > 0)
        return;
    TPoint position;
    position.m_y = 0;
    position.m_x = 0;
    while (static_cast<u32>(-position.m_x) < m_prototype->getWidth()) {
        if (isRmgObjectFootprintCell(m_prototype, -position.m_x, 0))
            break;
        --position.m_x;
    }
    if (position.m_x == -m_prototype->getWidth())
        return;
    position.m_y = 1;
    TPoint start = position;
    int direction = RMG_DIRECTION_NORTH;
    do {
        m_outline.push_back(position);
        int attempts = 0;
        do {
            TPoint nearby = nextRmgOutlineProbe(position, direction);
            if (nearby.m_x > 0 || static_cast<u32>(-nearby.m_x) >= m_prototype->getWidth()
                || nearby.m_y > 0 || static_cast<u32>(-nearby.m_y) >= m_prototype->getHeight())
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
    for (u32 x = 0; x < m_prototype->getWidth(); ++x) {
        int priority = !m_prototype->isUnderlay();
        u32 y = 0;
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

VA(0x00532FE0, 0xB4)
MAC_ADDRESS(0x230254, 0xb0)
TRmgTemplate::~TRmgTemplate()
{
    for (int zone = 0; zone < m_zones.size(); ++zone)
        delete m_zones[zone];
}

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

rmgBlackBoxObject::rmgBlackBoxObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
    m_creatureType = -1;
    m_creatureCount = 0;
    m_experience = 0;
    memset(m_resources, 0, sizeof(m_resources));
}

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

static inline void writeRmgMapPosition(
    TAbstractFile* outputFile, const TRmgMapPosition& position)
{
    writeValue<char>(outputFile, position.m_x);
    writeValue<char>(outputFile, position.m_y);
    writeValue<char>(outputFile, position.m_z);
}

template <int N>
static inline void writeRmgReservedBytes(TAbstractFile* outputFile)
{
    char reserved[N];
    memset(reserved, 0, sizeof(reserved));
    outputFile->write(reserved, sizeof(reserved));
}

VA(0x00533170, 0x79)
MAC_ADDRESS(0x2304c8, 0xfc)
void type_object::write(TAbstractFile* outputFile, int version)
{
    writeRmgMapPosition(outputFile, m_position);
    writeValue<s32>(outputFile, m_properties->m_prototypeIndex);
    writeRmgReservedBytes<5>(outputFile);
}

VA(0x005331F0, 0xFD)
MAC_ADDRESS(0x23062c, 0x144)
void rmgMonsterObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<s32>(outputFile, m_objectId);
    }
    writeValue<s16>(outputFile, m_count);
    writeValue<char>(outputFile, m_disposition);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, 0);
    writeRmgReservedBytes<2>(outputFile);
}

VA(0x005332F0, 0x16A)
MAC_ADDRESS(0x2307d8, 0x20c)
void rmgTownObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<s32>(outputFile, m_objectId);
    }
    writeValue<char>(outputFile, m_player);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, 0);
    writeValue<char>(outputFile, m_hasFort);
    char spells[9];
    memset(spells, 0, sizeof(spells));
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE)
        outputFile->write(spells, sizeof(spells));
    outputFile->write(spells, sizeof(spells));
    writeValue<s32>(outputFile, 0);
    if (version >= RMG_MAP_SHADOW_OF_DEATH) {
        writeValue<char>(outputFile, -1);
    }
    writeRmgReservedBytes<3>(outputFile);
}

VA(0x00533460, 0xA0)
MAC_ADDRESS(0x230a1c, 0x78)
void rmgOwnableObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, -1); // player
    writeRmgReservedBytes<3>(outputFile);
}

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
MAC_ADDRESS(0x230bb4, 0x78)
void rmgResourceObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, 0); // has custom treasure
    writeValue<s32>(outputFile, 0); // amount
    writeValue<s32>(outputFile, 0);
}

VA_COMPGEN(0x00533680, 0x21, SCALAR_DELETING_DTOR, rmgBlackBoxObject)

VA_COMPGEN(0x005336B0, 0x36, IMPLICIT_DTOR, rmgBlackBoxObject)
MAC_COMPGEN_ADDRESS(0x251488, 0x7c, IMPLICIT_DTOR, rmgBlackBoxObject)

static inline void writeRmgCreatureReward(
    TAbstractFile* outputFile, int version, int creature, const int& count)
{
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE)
        writeValue<s16>(outputFile, creature);
    else
        writeValue<char>(outputFile, creature);
    writeValue<s16>(outputFile, count);
}

VA(0x005336F0, 0x1E0)
MAC_ADDRESS(0x230cac, 0x32c)
void rmgBlackBoxObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, 0); // has custom treasure
    writeValue<s32>(outputFile, m_experience);
    writeValue<s32>(outputFile, 0); // mana
    writeValue<char>(outputFile, 0); // morale
    writeValue<char>(outputFile, 0); // luck
    outputFile->write(m_resources, sizeof(m_resources));
    writeValue<s32>(outputFile, 0); // primary skills
    writeValue<char>(outputFile, 0); // secondary skill count
    writeValue<char>(outputFile, 0); // artifact count
    writeValue<char>(outputFile, m_spells.size()); // spell count
    for (u32 spellIndex = 0; spellIndex < m_spells.size(); ++spellIndex) {
        writeValue<char>(outputFile, m_spells[spellIndex]);
    }
    if (m_creatureType == -1) {
        writeValue<char>(outputFile, 0); // creature count
    } else {
        writeValue<char>(outputFile, 1); // creature count
        writeRmgCreatureReward(outputFile, version, m_creatureType, m_creatureCount);
    }
    writeValue<s32>(outputFile, 0);
    writeValue<s32>(outputFile, 0);
}

VA(0x005338D0, 0x0D)
MAC_ADDRESS(0x230448, 0x58)
type_object::~type_object()
{
    --m_properties->m_refCount;
}

// Removes the object and, if one fits, puts a treasure worth 1-1.5x its value
// in its place.
// removeObject keeps the object alive and its position unchanged.
static inline void replaceRmgObjectWithTreasure(type_random_map_generator* generator,
    type_object* object, int value)
{
    TRmgMapPosition position = object->m_position;
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
b8 rmgKeyTentObject::completePlacement()
{
    if (m_generator->placeKeyTentGuard(this, m_value * 3 / 2))
        return true;
    replaceRmgObjectWithTreasure(m_generator, this, m_value);
    return false;
}

VA_COMPGEN(0x005339C0, 0x21, SCALAR_DELETING_DTOR, rmgQuestArtifactObject)

// Deletes the pending seer hut if it was never placed.
VA(0x005339F0, 0x58)
MAC_ADDRESS(0x231100, 0x9c)
rmgQuestArtifactObject::~rmgQuestArtifactObject()
{
    delete m_seerHut;
}

// On success the generator places the seer hut and takes ownership; on
// failure the hut is destroyed.
VA(0x00533A50, 0x33)
MAC_ADDRESS(0x2311fc, 0x78)
b8 rmgQuestArtifactObject::completePlacement()
{
    if (m_generator->placeQuestArtifact(this)) {
        m_seerHut = 0;
        return true;
    }
    delete m_seerHut;
    m_seerHut = 0;
    return false;
}

// Artifact quest followed by exactly one reward: experience, creatures or
// resources, in that precedence. The AB format adds the quest kind,
// artifact count, deadline and three empty texts.
VA(0x00533A90, 0x1E0)
MAC_ADDRESS(0x2312d0, 0x31c)
void rmgSeerHutObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<char>(outputFile, 5); // quest kind
        writeValue<char>(outputFile, 1); // artifact count
        writeValue<s16>(outputFile, m_artifact);
        writeValue<s32>(outputFile, -1); // deadline
        writeValue<s32>(outputFile, 0); // first visit length
        writeValue<s32>(outputFile, 0); // next visit length
        writeValue<s32>(outputFile, 0); // completion length
    } else {
        writeValue<char>(outputFile, m_artifact);
    }
    if (m_experience > 0) {
        writeValue<char>(outputFile, 1); // reward kind
        writeValue<s32>(outputFile, m_experience);
    } else if (m_creatureType != -1) {
        writeValue<char>(outputFile, 10); // reward kind
        writeRmgCreatureReward(outputFile, version, m_creatureType, m_creatureCount);
    } else {
        writeValue<char>(outputFile, 5); // reward kind
        writeValue<char>(outputFile, m_resourceType);
        writeValue<s32>(outputFile, m_resourceCount);
    }
    writeRmgReservedBytes<2>(outputFile);
}

// The factory reserves the hero in m_disabledHeroes; releaseReservation
// frees it again.
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
MAC_ADDRESS(0x231644, 0x18)
void rmgHeroObject::releaseReservation()
{
    m_generator->m_disabledHeroes[m_heroIndex] = false;
}

VA(0x00533C80, 0x1E4)
MAC_ADDRESS(0x23165c, 0x318)
void rmgHeroObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<s32>(outputFile, m_objectId);
    }
    writeValue<char>(outputFile, -1); // owner
    writeValue<char>(outputFile, m_heroIndex);
    writeValue<char>(outputFile, 0); // custom name
    if (version >= RMG_MAP_SHADOW_OF_DEATH) {
        writeValue<char>(outputFile, m_experience != 0); // custom experience
        if (m_experience != 0) {
            writeValue<s32>(outputFile, m_experience);
        }
    } else {
        writeValue<s32>(outputFile, m_experience);
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
MAC_ADDRESS(0x2319ac, 0xbc)
void rmgScholarObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, -1); // reward kind
    writeValue<char>(outputFile, 0); // reward value
    writeValue<s32>(outputFile, 0);
    writeRmgReservedBytes<2>(outputFile);
}

VA(0x00533F40, 0xAF)
MAC_ADDRESS(0x231aa0, 0x9c)
void rmgShrineObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, -1); // spell
    writeRmgReservedBytes<2>(outputFile);
    writeValue<char>(outputFile, 0); // reserved byte
}

VA(0x00533FF0, 0xC2)
MAC_ADDRESS(0x231b84, 0xc8)
void rmgSpellScrollObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    writeValue<char>(outputFile, 0); // message
    writeValue<char>(outputFile, m_spell);
    writeRmgReservedBytes<2>(outputFile);
    writeValue<char>(outputFile, 0);
}

// Witch hut skill mask: secondary skills 0-15 except 5 and 12 (Navigation
// and Necromancy).
enum ERmgWitchHutSkills {
    RMG_WITCH_HUT_ALLOWED_SKILLS = 0xefdf
};

VA(0x005340C0, 0x93)
MAC_ADDRESS(0x231c84, 0x68)
void rmgWitchHutObject::write(TAbstractFile* outputFile, int version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<u32>(outputFile, RMG_WITCH_HUT_ALLOWED_SKILLS);
    }
}

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

// Rounds a positive count to the nearest multiple of step, ties upward.
static inline int roundRmgCreatureCount(int count, int step)
{
    return ((count + step / 2) / step) * step;
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
        m_creatureCount = roundRmgCreatureCount(m_creatureCount, 10);
    else if (m_creatureCount > 12)
        m_creatureCount = roundRmgCreatureCount(m_creatureCount, 5);
    else if (m_creatureCount > 5)
        m_creatureCount = roundRmgCreatureCount(m_creatureCount, 2);
}

// Raises a value by the share of active zones with the same alignment.
static inline int adjustRmgValueForAlignment(int value, int alignmentCount, int zoneCount)
{
    if (zoneCount > 0)
        value += alignmentCount * value / zoneCount;
    return value;
}

// Active zones of a town alignment; none for neutral (-1).
static inline int getRmgAlignedZoneCount(
    const type_random_map_generator* generator, int alignment)
{
    int alignmentCount = 0;
    if (alignment != -1)
        alignmentCount = generator->m_activeZoneCountsByAlignment[alignment];
    return alignmentCount;
}

// Creature rewards are offered only in zones of the creature's town; the
// value rises with that town's share of active zones.
VA(0x00534310, 0x64)
MAC_ADDRESS(0x231f04, 0x6c)
int type_black_box_creature_def::getValue(
    TRmgZone* zone, type_random_map_generator* generator)
{
    int alignment = g_creatureTypeTraits[m_creatureType].m_townType;
    if (alignment != zone->m_townType2)
        return -1;
    int value = g_creatureTypeTraits[m_creatureType].m_aiValue * m_creatureCount;
    int alignmentCount = getRmgAlignedZoneCount(generator, alignment);
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

// Spell-trait flag of spells the map loader disables on every map (see
// game.cpp); never a generated reward.
enum ERmgSpellTraitFlags {
    RMG_SPELL_DISABLED_BY_DEFAULT = 0x2000
};

static inline bool isRmgSpellDisabledByDefault(int spell)
{
    return (g_spellTraits[spell].m_flags & RMG_SPELL_DISABLED_BY_DEFAULT) != 0;
}

VA(0x00534520, 0x267)
MAC_ADDRESS(0x2321ec, 0xc0)
type_object* type_black_box_spells_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    rmgBlackBoxObject* object = new rmgBlackBoxObject(properties);
    for (int level = m_maximumLevel; level >= m_minimumLevel; --level) {
        for (long spell = 0; spell < hero::NUM_SPELLS; ++spell) {
            if (!isRmgSpellDisabledByDefault(spell)
                && g_spellTraits[spell].m_level == level
                && (g_spellTraits[spell].m_school & m_schoolMask))
                object->m_spells.push_back(spell);
        }
    }
    return object;
}

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
    int alignmentZoneCount = getRmgAlignedZoneCount(generator, creature.m_townType);
    value = adjustRmgValueForAlignment(value, alignmentZoneCount, generator->m_activeZoneCount);
    return value + creature.m_aiValue * alignmentZoneCount / 2;
}

VA(0x00534870, 0x53)
MAC_ADDRESS(0x23243c, 0x50)
type_object* type_resource_lump_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgResourceObject(properties);
}

// Reserves a random available hero; no prison when none is left.
VA(0x005348D0, 0x93)
MAC_ADDRESS(0x2324e4, 0x84)
type_object* type_prison_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    int heroIndex = generator->selectPrisonHero();
    if (heroIndex == -1)
        return 0;
    return new rmgHeroObject(properties, generator,
        generator->m_nextObjectId++, heroIndex, m_experience);
}

VA(0x00534970, 0x53)
MAC_ADDRESS(0x2325b0, 0x50)
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

VA(0x00534A00, 0x53)
MAC_ADDRESS(0x232644, 0x50)
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

VA(0x00534A90, 0x53)
MAC_ADDRESS(0x2326dc, 0x50)
type_object* type_witch_hut_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgWitchHutObject(properties);
}

static inline bool canUseRmgSeerHutPrototype(
    const type_random_map_generator* generator, int prototypeIndex)
{
    return generator->m_nextSeerHutPrototypeIndex == prototypeIndex
        && !generator->m_questArtifactPoolLow;
}

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
MAC_ADDRESS(0x2327ec, 0xa4)
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

// Seer huts are offered only for the current seer-hut prototype and while
// enough quest artifacts remain.
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

static inline bool isRmgScrollSpell(int spell, int level)
{
    return !isRmgSpellDisabledByDefault(spell)
        && g_spellTraits[spell].m_schoolBits
        && g_spellTraits[spell].m_level == level;
}

VA(0x00534ED0, 0xC3)
MAC_ADDRESS(0x232b80, 0x108)
type_object* type_spell_scroll_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    // One random draw picks among eligible spells in spell order; at least
    // one eligible spell must exist.
    int eligibleSpellCount = 0;
    int spell;
    for (spell = 0; spell < hero::NUM_SPELLS; ++spell) {
        if (isRmgScrollSpell(spell, m_spellLevel))
            ++eligibleSpellCount;
    }
    int selectedIndex = rand() % eligibleSpellCount;
    for (spell = 0; spell < hero::NUM_SPELLS; ++spell) {
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

// The tent stores its treasure value; its colour is the prototype subtype.
VA(0x00534FD0, 0x64)
MAC_ADDRESS(0x232cfc, 0x70)
type_object* type_key_tent_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    return new rmgKeyTentObject(properties, generator, m_value);
}

// Empties the group; its objects are owned elsewhere.
VA(0x00535040, 0xC6)
MAC_ADDRESS(0x232e70, 0xa8)
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

// Marks the group ready and flags its outline cells.
MAC_ADDRESS(0x232fc4, 0x64)
void TRmgTreasureGroup::markPlacementOutline()
{
    m_ready = true;
    for (u32 index = 0; index < m_outline.size(); ++index)
        m_map.getMapItem(m_outline[index].m_x,
            m_outline[index].m_y)->m_tileData.m_placementOutline = true;
}

MAC_ADDRESS(0x233d18, 0x60)
b8 TRmgTreasureGroup::objectsAllowEntrances() const
{
    for (u32 index = 0; index < m_objects.size(); ++index) {
        int objectType = m_objects[index]->m_properties->m_prototype->getObjectType();
        if (!g_adventureObjectTraits[objectType].m_trait2)
            return false;
    }
    return true;
}

MAC_ADDRESS(0x233d78, 0xd4)
void TRmgTreasureGroup::addObject(type_object* object, TPoint point)
{
    m_objects.push_back(object);
    TRmgMapPosition position(point.m_x, point.m_y, 0);
    m_map.addObject(*object, position);
}

// Rings the objects' entrances with border cells, places the guard on a
// random fitting border cell of the outline (failing if none), opens paths
// around it and retraces the outline.
VA(0x00535110, 0x4AB)
MAC_ADDRESS(0x233028, 0x6a8)
b8 TRmgTreasureGroup::addGuard(type_object* guard)
{
    traceOutline();
    // Retail bug: the guard position is adjusted by the trigger of the last
    // object's prototype, not the guard's. The group must hold an object.
    TObjectType* prototype;
    for (u32 objectIndex = 0; objectIndex < m_objects.size(); ++objectIndex) {
        type_object* object = m_objects[objectIndex];
        prototype = object->m_properties->m_prototype;
        TRmgMapPosition entrance = getRmgPlacedObjectEntrance(object);
        u32 direction = isRmgEntranceOpenToNorth(prototype->getObjectType())
            ? RMG_DIRECTION_COUNT : RMG_FIRST_NORTHERN_DIRECTION;
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
    for (u32 index = m_outline.size(); index--;) {
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
    u32 outlineCount = m_outline.size();
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
        // Open the next cell outward too; diagonals fan out to three cells.
        // North is up; G is the guard, and each cell shows the
        // g_rmgDirections index of the neighbour that opens it.
        //   5 5 6 7 7
        //   5 5 6 7 7
        //   4 4 G 0 0
        //   3 3 2 1 1
        //   3 3 2 1 1
        int fanDirection;
        u32 count;
        if (isRmgDiagonalDirection(direction)) {
            fanDirection = turnRmgDirection(direction, -1);
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
            fanDirection = turnRmgDirection(fanDirection, 1);
        }
    }
    m_guardPosition = guardPosition;
    m_hasGuard = true;
    m_outline.clear();
    updateBounds();
    traceOutline();
    return true;
}

VA(0x005355C0, 0x1A)
TRmgMapPosition::TRmgMapPosition(int newX, int newY, int newZ)
    : TPoint(newX, newY), m_z(newZ)
{
}

// Whether an object fits on the group map without blocking neighbouring
// entrances. Other objects' entrances avoid border cells; guards may stand
// on them but need a free neighbouring cell.
VA(0x005355E0, 0x1F9)
MAC_ADDRESS(0x2336d0, 0x374)
b8 TRmgTreasureGroup::canFitObject(TRmgObjectPropertiesRef* properties,
    TRmgMapPosition position)
{
    TObjectType* prototype = properties->m_prototype;
    int objectType = prototype->getObjectType();
    TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, prototype->m_triggerCell);
    TRmgVector origin(entrance.m_x, entrance.m_y);
    if (!isRmgEntranceOpenToNorth(objectType)) {
        for (int direction = RMG_FIRST_NORTHERN_DIRECTION; direction < RMG_DIRECTION_COUNT; ++direction) {
            TPoint nearby = g_rmgDirections[direction] + origin;
            if (m_map.getMapItem(nearby.m_x, nearby.m_y)->isRoadEntrance())
                goto placementFailure;
        }
    }
    {
        for (int direction = 0; direction < RMG_FIRST_NORTHERN_DIRECTION; ++direction) {
            TPoint nearby = g_rmgDirections[direction] + origin;
            TRmgMapItem* item = m_map.getMapItem(nearby.m_x, nearby.m_y);
            if (item->isRoadEntrance()) {
                int neighborType = item->getEntranceObjectType();
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

// Puts the object's entrance on a random fitting cell next to an existing
// entrance: one of the three below it, or any of the eight when that entrance
// is open to the north.
VA(0x00535970, 0x240)
MAC_ADDRESS(0x233a44, 0x2d4)
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
    for (u32 index = 0; index < m_objects.size(); ++index) {
        type_object* existing = m_objects[index];
        TObjectType* existingPrototype = existing->m_properties->m_prototype;
        TRmgMapPosition entrance = getRmgPlacedObjectEntrance(existing);
        b8 openToNorth = isRmgEntranceOpenToNorth(existingPrototype->getObjectType());
        for (int direction = RMG_DIRECTION_COUNT; direction--; ) {
            if (!openToNorth && !isRmgSouthwardDirection(direction))
                continue;
            TRmgMapPosition candidate = entrance + g_rmgDirections[direction] + trigger;
            if (bounds.contains(candidate) && canFitObject(properties, candidate))
                candidates.push_back(candidate);
        }
    }
    u32 count = candidates.size();
    if (!count)
        return false;
    u32 selected = rand() % count;
    TRmgMapPosition position = candidates[selected];
    addObject(object, position);
    return true;
}

VA(0x00535DF0, 0xEA)
MAC_ADDRESS(0x233e4c, 0x16c)
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

// Traces the closed outline around the occupied cells, starting above the
// first occupied cell in row-major order.
VA(0x00535EE0, 0x18F)
MAC_ADDRESS(0x233fb8, 0x274)
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
    int direction = RMG_DIRECTION_SOUTH;
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

// Progress steps reported by loadObjectPrototypes, which the base
// constructor runs and budgets.
enum ERmgPrototypeLoadProgress {
    RMG_PROTOTYPE_LOAD_PROGRESS = 15300
};

VA(0x00536070, 0xFB)
MAC_ADDRESS(0x23422c, 0xec)
TRmgGeneratorBase::TRmgGeneratorBase(int width, int height, int levels,
    TProgressSink* progress, int additionalSteps, int version)
    : m_map(width, height, levels)
{
    m_progress = progress;
    m_mapVersion = version;
    if (progress)
        progress->setTotal(progress->m_steps + additionalSteps
            + RMG_PROTOTYPE_LOAD_PROGRESS);
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

// Overlap priorities stay uninitialized until first built.
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
    for (u32 index = 0; index < m_objectsTxt.m_objectTypes.size(); ++index) {
        int type = m_objectsTxt.m_objectTypes[index].getObjectType();
        if (!isRmgObjectAvailableInVersion(type, m_mapVersion))
            continue;
        if (m_mapVersion < RMG_MAP_SHADOW_OF_DEATH && (type == LITH_TWOWAY || type == LITH_ONEWAY_ENTRANCE || type == LITH_ONEWAY_EXIT)
            && m_objectsTxt.m_objectTypes[index].getSubtype() >= 3)
            continue;
        if (type < 0 || type >= ADVENTURE_OBJECT_TRAIT_COUNT)
            continue;
        TRmgObjectPropertiesRef* properties =
            new TRmgObjectPropertiesRef(&m_objectsTxt.m_objectTypes[index]);
        // Aliased object types are listed under their objnames.txt row.
        int mappedType;
        memcpy(&mappedType, &g_adventureObjectTraits[type].m_nameRow, sizeof(mappedType));
        m_objectPrototypes[mappedType].push_back(properties);
    }
    // Exchange sort of the monsters by subtype, swapping prototypes. Retail
    // bug: an empty monster list underflows size() - 1.
    for (u32 first = 0; first < m_objectPrototypes[MONSTER].size() - 1; ++first) {
        for (u32 second = first + 1; second < m_objectPrototypes[MONSTER].size(); ++second) {
            if (m_objectPrototypes[MONSTER][first]->m_prototype->getSubtype() > m_objectPrototypes[MONSTER][second]->m_prototype->getSubtype()) {
                std::swap(m_objectPrototypes[MONSTER][first]->m_prototype, m_objectPrototypes[MONSTER][second]->m_prototype);
            }
        }
    }
    readObjectPlacementRules();
    if (m_progress)
        m_progress->advance(RMG_PROTOTYPE_LOAD_PROGRESS);
}

VA(0x005363B0, 0x1A9)
MAC_ADDRESS(0x2346a0, 0x18c)
TRmgGeneratorBase::~TRmgGeneratorBase()
{
    for (u32 object = 0; object < m_objects.size(); ++object)
        delete m_objects[object];
    for (int type = 0; type < ADVENTURE_OBJECT_TRAIT_COUNT; ++type)
        for (u32 prototype = 0; prototype < m_objectPrototypes[type].size(); ++prototype)
            delete m_objectPrototypes[type][prototype];
}

// Neighbour scores start at column 16, after the type and terrain scores.
static inline void readRmgPlacementScores(std::vector<int>& scores,
    const TSpreadsheetResource::TStringVector& fields, int precedingScores, int count)
{
    scores.resize(count, 0);
    for (int index = 0; index < count; ++index)
        scores[index] = atoi(fields[index + precedingScores + 16]);
}

// Reads rand_trn.txt and binds each prototype to the last rule matching its
// object type, subtype and first recommended terrain.
VA(0x00536560, 0x5F2)
MAC_ADDRESS(0x23486c, 0x6a0)
void TRmgGeneratorBase::readObjectPlacementRules()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x006827F4, rmgPlacementRulesFilename, "rand_trn.txt"));
    int row = RMG_FIRST_DATA_ROW;
    std::vector<TAdventureObjectType> objectTypes;
    std::vector<TTerrainType> terrains;
    std::vector<int> subtypes;
    for (; row < sheet->getNumberOfRows();) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        if (values[0][0] == ' ' || values[0][0] == 0)
            break;
        TRmgObjectPlacementRule rule;
        rule.m_index = row - RMG_FIRST_DATA_ROW;
        TAdventureObjectType ruleObjectType =
            H3_ENUM_DECODE(TAdventureObjectType, atoi(values[3]));
        int ruleSubtype = atoi(values[4]);
        TTerrainType ruleTerrain = H3_ENUM_DECODE(TTerrainType, atoi(values[6]));
        objectTypes.push_back(ruleObjectType);
        terrains.push_back(ruleTerrain);
        subtypes.push_back(ruleSubtype);
        TTerrainType terrain;
        for (terrain = eTerrainDirt; terrain <= eTerrainWater;
             terrain = H3_ENUM_DECODE(TTerrainType, terrain + 1))
            rule.m_terrainScores[terrain] = atoi(values[terrain + 7]);
        for (; terrain < 10; terrain = H3_ENUM_DECODE(TTerrainType, terrain + 1))
            rule.m_terrainScores[terrain] = RMG_PLACEMENT_INVALID;
        m_placementRules.push_back(rule);
        ++row;
    }
    int ruleCount = m_placementRules.size();
    for (row = RMG_FIRST_DATA_ROW; row < ruleCount + RMG_FIRST_DATA_ROW; ++row) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        TRmgObjectPlacementRule& rule = m_placementRules[row - RMG_FIRST_DATA_ROW];
        readRmgPlacementScores(rule.m_adjacentScores, values, 0, ruleCount);
        readRmgPlacementScores(rule.m_blockedScores, values, ruleCount, ruleCount);
    }
    sheet->dispose();

#if defined(HOMM3_TARGET_MAC)
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
    for (TAdventureObjectType objectType = NOTHING; objectType < ADVENTURE_OBJECT_TRAIT_COUNT;
         objectType = H3_ENUM_DECODE(TAdventureObjectType, objectType + 1)) {
        for (int index = 0; index < m_objectPrototypes[objectType].size();
             ++index) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[objectType][index];
            TObjectType* prototype = properties->m_prototype;
            properties->m_placementRule = 0;
            TTerrainType terrain;
            for (terrain = eTerrainDirt; terrain < eTerrainRock;
                 terrain = H3_ENUM_DECODE(TTerrainType, terrain + 1)) {
                if (prototype->m_recommendedTerrainMask[terrain])
                    break;
            }
            properties->m_preferredTerrain = terrain;
            if (terrain != eTerrainRock) {
                int subtype = prototype->getSubtype();
                int mappedType;
                // Aliased object types use their objnames.txt row.
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

// Scores a candidate position from the terrain under the object's blocked
// cells and the objects it touches; callers accept only positive scores.
VA(0x00536BC0, 0x5F4)
MAC_ADDRESS(0x23515c, 0x7a0)
int TRmgGeneratorBase::scoreObjectPlacement(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position)
{
    TObjectType* prototype = properties->m_prototype;
    std::vector<type_object*> affected;
    b8 terrainSeen[10];
    memset(terrainSeen, 0, sizeof(terrainSeen));
    // The 8x6 footprint plus a one-cell border; footprint cell (column, row)
    // is marks[column + 1][row + 1].
    u32 marks[8 + 2][6 + 2];
    memset(marks, 0, sizeof(marks));
    for (u32 row = 0; row < prototype->getHeight(); ++row) {
        int y = position.m_y - row;
        if (y < 0 || y >= m_map.m_mapHeight)
            continue;
        for (u32 column = 0; column < prototype->getWidth(); ++column) {
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

                // Retail bug: this replaces the OVERLAP and BLOCKED marks just
                // set, and BLOCKED is set nowhere else. Blocked cells therefore
                // never take the overlap-order or blocked-score paths below:
                // objects under them count only as adjacent, and rand_trn.txt's
                // blocked scores are never used.
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
        for (u32 column = 0; column < prototype->getWidth() + 2;
             ++column) {
            int x = position.m_x + 1 - column;
            if (x < 0 || x >= m_map.m_mapWidth)
                continue;
            u32 mark = marks[column][row];
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

    for (u32 index = 0; index < affected.size(); ++index) {
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

// Decoration (obstacle) object types, excluding holes, rivers and roads.
// The last six are unnamed expansion types, unavailable on RoE maps.
DATA(0x006408EC)
static const s32 g_rmgDecorationTypes[45] = {
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

// Fills obstacles outward from a cell with weighted random decorations.
VA(0x005373A0, 0x53D)
MAC_ADDRESS(0x2359b4, 0x76c)
void TRmgGeneratorBase::decorateMapCell(TRmgMapPosition start, int progressSteps)
{
    std::vector<TRmgMapPosition> pending;
    pending.push_back(start);
    while (pending.size()) {
        TRmgMapPosition position = pending.back();
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
        std::vector<type_object*> unusedObjects;
        int totalWeight = 0;
        for (const int* type = g_rmgDecorationTypes;
            type < g_rmgDecorationTypes + sizeof(g_rmgDecorationTypes) / sizeof(g_rmgDecorationTypes[0]); ++type) {
            std::vector<TRmgObjectPropertiesRef*>& prototypes = m_objectPrototypes[*type];
            for (u32 index = 0; index < prototypes.size(); ++index) {
                TRmgObjectPropertiesRef* properties = prototypes[index];
                TObjectType* prototype = properties->m_prototype;
                if (!properties->m_placementRule
                    || properties->m_placementRule->m_terrainScores[terrain] <= RMG_PLACEMENT_INVALID)
                    continue;
                // Never skips: loadObjectPrototypes listed only prototypes
                // available in m_mapVersion. Retail repeats the test.
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
            // Weighted random pick; candidate order affects the result.
            int selected = rand() % totalWeight;
            u32 index;
            for (index = 0; index < candidates.size(); ++index) {
                selected -= weights[index];
                if (selected < 0)
                    break;
            }
            TRmgObjectPropertiesRef* properties = candidates[index];
            TRmgMapPosition placement = positions[index];
            TObjectType* prototype = properties->m_prototype;
            addObject(new type_object(properties), placement);
            TRmgZoneBounds bounds;
            bounds.m_minimumX = max(placement.m_x - prototype->getWidth(), 0);
            bounds.m_minimumY = max(placement.m_y - prototype->getHeight(), 0);
            bounds.m_maximumX = min(placement.m_x + 2, m_map.m_mapWidth);
            bounds.m_maximumY = min(placement.m_y + 2, m_map.m_mapHeight);
            TRmgMapPosition nearbyPosition;
            nearbyPosition.m_z = position.m_z;
            for (nearbyPosition.m_y = bounds.m_minimumY;
                nearbyPosition.m_y < bounds.m_maximumY; ++nearbyPosition.m_y) {
                for (nearbyPosition.m_x = bounds.m_minimumX;
                    nearbyPosition.m_x < bounds.m_maximumX; ++nearbyPosition.m_x) {
                    TRmgMapItem* nearby = m_map.getMapItem(nearbyPosition);
                    if (nearby->hasBorderObject() && nearby->isPassableLand()) {
                        nearby->clearBorderObject();
                        pending.push_back(nearbyPosition);
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

// Decorates the whole map; runs after coastal tiles are marked.
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

// Unlisted types get 32000 (unlimited); an earlier override wins on repeats.
static inline void initializeRmgObjectLimits(int* limits,
    const TRmgObjectLimit* overrides, int count)
{
    for (int objectType = 0; objectType < ADVENTURE_OBJECT_TRAIT_COUNT; ++objectType)
        limits[objectType] = 32000;
    while (count--)
        limits[overrides[count].m_objectType] = overrides[count].m_limit;
}

// Special heroes and heroes missing from the map version are never offered.
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
        for (int hero = 0; hero < sizeof(m_disabledHeroes); ++hero) {
            if (g_heroTraits[hero].m_availability.m_special)
                m_disabledHeroes[hero] = true;
            else if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
                if (!g_heroTraits[hero].m_availability.m_availableInExpansion)
                    m_disabledHeroes[hero] = true;
            } else if (!g_heroTraits[hero].m_availability.m_availableInOriginal)
                m_disabledHeroes[hero] = true;
        }
        initializeRmgObjectLimits(g_rmgMapObjectLimits, g_rmgMapObjectLimitOverrides,
            sizeof(g_rmgMapObjectLimitOverrides) / sizeof(g_rmgMapObjectLimitOverrides[0]));
        initializeRmgObjectLimits(g_rmgZoneObjectLimits, g_rmgZoneObjectLimitOverrides,
            sizeof(g_rmgZoneObjectLimitOverrides) / sizeof(g_rmgZoneObjectLimitOverrides[0]));
        memset(m_fixedHumanPlayers, 0, sizeof(m_fixedHumanPlayers));
    }
}

VA_COMPGEN(0x00537DC0, 0x21, SCALAR_DELETING_DTOR, type_random_map_generator)

VA(0x00537DF0, 0x200)
MAC_ADDRESS(0x23685c, 0x1cc)
type_random_map_generator::~type_random_map_generator()
{
    for (u32 zone = 0; zone < m_zones.size(); ++zone)
        delete m_zones[zone];
    for (u32 mapTemplate = 0; mapTemplate < m_templates.size(); ++mapTemplate)
        delete m_templates[mapTemplate];
    for (u32 definition = 0; definition < m_objectGenerators.size(); ++definition)
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

template<class Record>
static inline bool allowsRmgTemplatePlayerCounts(const Record& record,
    int humanPlayers, int computerPlayers)
{
    return record.m_minimumHumanPlayers <= humanPlayers
        && record.m_maximumHumanPlayers >= humanPlayers
        && record.m_minimumPlayers <= humanPlayers + computerPlayers
        && record.m_maximumPlayers >= humanPlayers + computerPlayers;
}

// Store one connection on both zones, each copy pointing at the other zone.
static inline void appendRmgTwoWayConnection(TRmgTemplateZone* first,
    TRmgTemplateZone* second, TRmgZoneConnection connection)
{
    connection.m_destination = second;
    first->m_connections.push_back(connection);
    connection.m_destination = first;
    second->m_connections.push_back(connection);
}

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
                connection.m_value = atoi(fields[78]);
                connection.m_unguarded = isRmgTemplateFieldSet(fields[79]);
                connection.m_placeBorderObjects = isRmgTemplateFieldSet(fields[80]);
                readRmgTemplatePlayerLimits(connection, fields, 81);
                connection.m_connected = false;
                if (allowsRmgTemplatePlayerCounts(connection, humanPlayers, computerPlayers))
                    appendRmgTwoWayConnection(first, second, connection);
            }
        }
    }
}

static bool hasRmgTemplatePlayerSlots(TRmgTemplate* mapTemplate,
    int humanPlayers, int computerPlayers)
{
    int playerSlots = 0;
    for (u32 slot = 0; slot < mapTemplate->m_zones.size(); ++slot)
        if (mapTemplate->m_zones[slot]->m_kind == RMG_TEMPLATE_HUMAN)
            ++playerSlots;
    if (playerSlots < humanPlayers)
        return false;
    for (slot = 0; slot < mapTemplate->m_zones.size(); ++slot)
        if (mapTemplate->m_zones[slot]->m_kind == RMG_TEMPLATE_COMPUTER)
            ++playerSlots;
    return playerSlots >= humanPlayers + computerPlayers;
}

// Template sizes are in units of 36x36 cells.
VA(0x00537FF0, 0x482)
MAC_ADDRESS(0x2372ec, 0x304)
void type_random_map_generator::loadTemplates()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00682804, rmgTemplatesFilename, "rmg.txt"));
    int mapSize = m_map.getWidth() * m_map.getHeight() * m_map.m_numberLevels / (36 * 36);
    int row = RMG_FIRST_DATA_ROW;
    if (m_waterContent == RMG_WATER_ISLANDS)
        mapSize = max(mapSize / 2, 1);
    for (; row < sheet->getNumberOfRows();) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        // Retail bug: a two-field row still reads field [2].
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
            for (u32 zone = 0; zone < mapTemplate->m_zones.size(); ++zone)
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
        // Retail bug: a three-field row reads values[3] before the full
        // row-length check.
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
                slot->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_COUNT] = atoi(values[15]);
                slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_DENSITY] = atoi(values[16]);
                slot->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_DENSITY] = atoi(values[17]);
                slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT] = atoi(values[18]);
                slot->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_COUNT] = atoi(values[19]);
                slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_DENSITY] = atoi(values[20]);
                slot->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_DENSITY] = atoi(values[21]);
                slot->m_neutralTownsMatchZone = isRmgTemplateFieldSet(values[22]);
                // RoE maps have no Conflux.
                int townCount;
                if (mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE)
                    townCount = TOWN_TYPE_COUNT;
                else {
                    townCount = TOWN_CONFLUX;
                    slot->m_allowedTowns[TOWN_CONFLUX] = false;
                }
                while (townCount--)
                    slot->m_allowedTowns[townCount] =
                        isRmgTemplateFieldSet(values[23 + townCount]);
                for (int resource = 0; resource < NUM_RESOURCES; ++resource) {
                    slot->m_mineCounts[resource] = atoi(values[32 + resource]);
                    slot->m_mineDensities[resource] = atoi(values[39 + resource]);
                }
                slot->m_useNativeTerrain = isRmgTemplateFieldSet(values[46]);
                b8 anyTerrain = false;
                for (int terrain = 0; terrain < eTerrainWater; ++terrain) {
                    slot->m_allowedTerrain[terrain] =
                        isRmgTemplateFieldSet(values[47 + terrain]);
                    if (slot->m_allowedTerrain[terrain])
                        anyTerrain = true;
                }
                if (!anyTerrain)
                    slot->m_allowedTerrain[eTerrainDirt] = true;
                switch (tolower(values[55][0])) {
                case 'n': slot->m_monsterStrength = RMG_ZONE_MONSTERS_NONE; break;
                case 'w': slot->m_monsterStrength = RMG_ZONE_MONSTERS_WEAK; break;
                case 's': slot->m_monsterStrength = RMG_ZONE_MONSTERS_STRONG; break;
                case 'a': slot->m_monsterStrength = RMG_ZONE_MONSTERS_AVERAGE; break;
                default: slot->m_monsterStrength = RMG_ZONE_MONSTERS_AVERAGE; break;
                }
                slot->m_guardsMatchZone = isRmgTemplateFieldSet(values[56]);
                // Neutral, then one slot per town type.
                for (int monster = 0; monster < TOWN_TYPE_COUNT + 1; ++monster)
                    slot->m_allowedMonsters[monster] =
                        isRmgTemplateFieldSet(values[57 + monster]);
                // Retail bug: TOWN_CONFLUX is the Conflux slot of m_allowedTowns,
                // but monster slots are offset by one, so RoE maps disallow
                // Fortress guards instead.
                if (mapVersion < RMG_MAP_ARMAGEDDONS_BLADE)
                    slot->m_allowedMonsters[TOWN_CONFLUX] = false;
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

static inline int getRmgCreatureTypeCount(int mapVersion)
{
    return mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE
        ? RMG_CREATURE_TYPE_COUNT : RMG_ROE_CREATURE_TYPE_COUNT;
}

// Creature dwelling subtypes offered as treasures (g_creatureGenerator1Types).
enum ERmgDwellingSubtypeCounts {
    RMG_DWELLING_SUBTYPE_COUNT = 80,
    RMG_ROE_DWELLING_SUBTYPE_COUNT = 58
};

VA(0x00538B10, 0x2241)
MAC_ADDRESS(0x2375f0, 0x4880)
void type_random_map_generator::initializeObjectGenerators()
{
    m_objectGenerators.push_back(new type_treasure_def(ALTAR_OF_SACRIFICE, 0, 100, 20));
    m_objectGenerators.push_back(new type_treasure_def(ARENA, 0, 3000, 50));

    for (int creature = getRmgCreatureTypeCount(m_mapVersion); creature--;) {
        if (g_creatureTypeTraits[creature].m_level >= 0)
            m_objectGenerators.push_back(
                new type_black_box_creature_def(creature));
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

    m_objectGenerators.push_back(new type_black_box_spells_def(5000, 1, 1, eSchoolAll));
    m_objectGenerators.push_back(new type_black_box_spells_def(7500, 2, 2, eSchoolAll));
    m_objectGenerators.push_back(new type_black_box_spells_def(10000, 3, 3, eSchoolAll));
    m_objectGenerators.push_back(new type_black_box_spells_def(12500, 4, 4, eSchoolAll));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 5, 5, eSchoolAll));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, eSchoolAir));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, eSchoolFire));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, eSchoolWater));
    m_objectGenerators.push_back(new type_black_box_spells_def(15000, 1, 5, eSchoolEarth));
    m_objectGenerators.push_back(new type_black_box_spells_def(30000, 1, 5, eSchoolAll));

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

    int dwelling = RMG_DWELLING_SUBTYPE_COUNT;
    if (m_mapVersion < RMG_MAP_ARMAGEDDONS_BLADE)
        dwelling = RMG_ROE_DWELLING_SUBTYPE_COUNT;
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

    for (int prototypeIndex = 0; prototypeIndex < m_objectPrototypes[SEER].size();
         ++prototypeIndex) {
        for (int creature = getRmgCreatureTypeCount(m_mapVersion); creature--;) {
            if (g_creatureTypeTraits[creature].m_level >= 0)
                m_objectGenerators.push_back(
                    new type_quest_creature_def(creature, prototypeIndex));
        }

        m_objectGenerators.push_back(
            new type_quest_experience_def(prototypeIndex, 2000, 5000));
        m_objectGenerators.push_back(
            new type_quest_experience_def(prototypeIndex, 5333, 10000));
        m_objectGenerators.push_back(
            new type_quest_experience_def(prototypeIndex, 8666, 15000));
        m_objectGenerators.push_back(
            new type_quest_experience_def(prototypeIndex, 12000, 20000));
        m_objectGenerators.push_back(new type_quest_gold_def(prototypeIndex, 2000, 5000));
        m_objectGenerators.push_back(new type_quest_gold_def(prototypeIndex, 5333, 10000));
        m_objectGenerators.push_back(new type_quest_gold_def(prototypeIndex, 8666, 15000));
        m_objectGenerators.push_back(new type_quest_gold_def(prototypeIndex, 12000, 20000));
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

// Player zones go underground only with Inferno, Necropolis or Dungeon
// towns; zones on one level stay at least 80% of their combined size apart.
VA(0x0053AD60, 0x113)
MAC_ADDRESS(0x23be70, 0x14c)
b8 type_random_map_generator::canPlaceZone(TRmgZone* zone)
{
    TRmgTemplateZone* slot = zone->m_templateZone;
    TRmgMapPosition position = zone->getLevelPosition();
    int size = slot->m_size;
    if ((slot->m_kind == RMG_TEMPLATE_HUMAN ||
         slot->m_kind == RMG_TEMPLATE_COMPUTER) &&
        position.m_z == RMG_UNDERGROUND_LEVEL && zone->m_alignment != TOWN_INFERNO &&
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

// The other level of a two-level map.
static inline int getRmgOtherLevel(int level)
{
    return RMG_UNDERGROUND_LEVEL - level;
}

// One of the 32 radial candidate positions around a zone.
static inline TRmgMapPosition getRmgRadialZonePosition(
    const TRmgMapPosition& center, int radius, int direction, int level)
{
    const int& y = static_cast<int>(center.m_y + radius * g_rmgDirectionSines[direction]);
    return TRmgMapPosition(
        static_cast<int>(center.m_x + radius * g_rmgDirectionCosines[direction]),
        y, level);
}

// Trial placement leaves the zone at the candidate even when rejected.
static inline void appendRmgZoneCandidate(type_random_map_generator* generator,
    TRmgZone* zone, const TRmgMapPosition& candidate,
    std::vector<TRmgMapPosition>& candidates)
{
    zone->setLevelPosition(candidate);
    if (generator->canPlaceZone(zone))
        candidates.push_back(zone->getLevelPosition());
}

VA(0x0053AE80, 0x36A)
MAC_ADDRESS(0x23bfbc, 0x28c)
void type_random_map_generator::appendZonePositions(TRmgZone* center,
    TRmgZone* zone, std::vector<TRmgMapPosition>& candidates)
{
    int radius = center->m_templateZone->m_size + zone->m_templateZone->m_size;
    TRmgMapPosition position = center->getLevelPosition();
    TRmgMapPosition candidate;
    for (int direction = 0; direction < RMG_RADIAL_DIRECTION_COUNT; ++direction) {
        candidate = getRmgRadialZonePosition(position, radius, direction, position.m_z);
        appendRmgZoneCandidate(this, zone, candidate, candidates);
    }
    if (m_map.m_numberLevels == 1)
        return;
    int level = getRmgOtherLevel(position.m_z);
    candidate = TRmgMapPosition(position.m_x, position.m_y, level);
    appendRmgZoneCandidate(this, zone, candidate, candidates);
    radius = center->m_templateZone->m_size;
    if (radius < zone->m_templateZone->m_size)
        radius = zone->m_templateZone->m_size;
    for (direction = 0; direction < RMG_RADIAL_DIRECTION_COUNT; ++direction) {
        candidate = getRmgRadialZonePosition(position, radius, direction, level);
        appendRmgZoneCandidate(this, zone, candidate, candidates);
    }
}

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

// Square-map extent needed to enclose a candidate zone and all other zones,
// at least the requested map size.
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

// Prefer unused levels, then the most connections, then the smallest
// enclosing square.
VA(0x0053B2F0, 0x678)
MAC_ADDRESS(0x23c454, 0x62c)
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
        if (!occupiedLevels[RMG_SURFACE_LEVEL]
            || !occupiedLevels[RMG_UNDERGROUND_LEVEL]) {
            int candidate = candidates.size();
            while (candidate--) {
                if (!occupiedLevels[candidates[candidate].m_z])
                    break;
            }
            // Retail bug: a lone unused-level candidate at index zero does
            // not trigger filtering.
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
    TRmgZoneBounds bounds = {0, 0, 0, 0};
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

// The first zone starts at the origin, on the surface or, when eligible,
// underground. Later zones sample template neighbours, falling back to all
// placed zones before filtering.
VA(0x0053B970, 0x232)
MAC_ADDRESS(0x23ca80, 0x20c)
void type_random_map_generator::positionZone(TRmgZone* zone, int mapSize)
{
    std::vector<TRmgMapPosition> candidates;
    if (m_zones.size() == 0) {
        zone->m_levelPosition.m_y = 0;
        zone->m_levelPosition.m_z = RMG_SURFACE_LEVEL;
        zone->m_levelPosition.m_x = 0;
        candidates.push_back(zone->getLevelPosition());
        if (m_map.m_numberLevels > 1)
            appendRmgZoneCandidate(this, zone,
                TRmgMapPosition(0, 0, RMG_UNDERGROUND_LEVEL), candidates);
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
    // Assumes a legal candidate exists; an empty set reaches rand() % 0.
    u32 count = candidates.size();
    u32 selected = rand() % count;
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

// Places each zone, repositions all of them twice, scales the layout into a
// centred square, then picks each zone's terrain and town type.
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
        if (slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT] + slot->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_COUNT] > 0
            && slot->m_playerIndex >= 0) {
            int player = m_playerIndexMap[slot->m_playerIndex + 1];
            if (player >= 0 && m_townChoices[player] != -1)
                zone->m_alignment = m_townChoices[player];
        }
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
    int originY = (minimumY - span + maximumY) / 2;
    int originX = (minimumX - span + maximumX) / 2;
    for (int zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgMapPosition position = zone->getLevelPosition();
        position.m_x = (position.m_x - originX) * size / span;
        position.m_y = (position.m_y - originY) * size / span;
        zone->setLevelPosition(position);
        zone->m_scaledSize = zone->m_templateZone->m_size * size / span;
        zone->chooseTerrain();
        // True for every version.
        zone->chooseTownType(m_mapVersion >= RMG_MAP_RESTORATION_OF_ERATHIA);
    }
}

static inline TPoint getRmgSubdivisionMidpoint(const TPoint& from, const TPoint& to)
{
    return TPoint((from.m_x + to.m_x + 1) / 2, (from.m_y + to.m_y + 1) / 2);
}

// One random draw centred on zero (modulo bias; even ranges are asymmetric).
static inline int getRmgCenteredRandomOffset(int range)
{
    return rand() % range - range / 2;
}

// Split a boundary segment at a midpoint displaced at random across it,
// pushing the far half first so the near half is walked next. The
// displacement range is the segment length (half of it for island coasts),
// capped by roughness. Returns false when the segment has no interior midpoint.
static inline bool splitRmgBoundarySegment(std::vector<TPoint>& pending,
    const TPoint& from, const TPoint& to, int roughness, int lengthDivisor)
{
    TPoint midpoint = getRmgSubdivisionMidpoint(from, to);
    if (midpoint == from || midpoint == to)
        return false;
    TRmgVector perpendicular;
    {
        TRmgVector delta = to - from;
        perpendicular = TRmgVector(-delta.m_y, delta.m_x);
    }
    int length = perpendicular.length();
    if (length > 1) {
        // Roughness must be positive; zero reaches rand() % 0.
        int limit = cppMin<long>(length / lengthDivisor, roughness);
        int displacement = getRmgCenteredRandomOffset(limit);
        perpendicular = perpendicular * displacement / length;
        midpoint += perpendicular;
    }
    pending.push_back(to);
    pending.push_back(midpoint);
    return true;
}

static inline TPoint clampRmgBoundaryToMap(
    const TPoint& point, const type_random_map& map)
{
    long x = cppMax<long>(point.m_x, 0);
    x = cppMin<long>(x, map.m_mapWidth - 1);
    long y = cppMax<long>(point.m_y, 0);
    y = cppMin<long>(y, map.m_mapHeight - 1);
    return TPoint(x, y);
}

// Island maps paint surface zone terrain only on the islands.
static inline bool paintsRmgZoneTerrainOnLevel(int waterContent, int level)
{
    return level == RMG_UNDERGROUND_LEVEL || waterContent != RMG_WATER_ISLANDS;
}

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
    b8 markForTerrain = paintsRmgZoneTerrainOnLevel(m_waterContent, level);
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

// Bresenham line whose error starts at half the major distance.
// The final cell receives its zone but not the terrain mark.
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
    b8 markForTerrain = paintsRmgZoneTerrainOnLevel(m_waterContent, level);
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
// A shared edge is drawn only by the lower-indexed zone.
VA(0x0053C390, 0x730)
MAC_ADDRESS(0x23dd34, 0x614)
void type_random_map_generator::traceZoneBoundary(
    TRmgHalfEdge* first, b8 irregular)
{
    TRmgHalfEdge* vertex = first;
    TRmgZone* zone = vertex->m_zone;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition zonePosition = zone->m_levelPosition;
    TRmgZoneBounds bounds = {0, 0, m_map.m_mapWidth, m_map.m_mapHeight};
    TPoint upperLeft(bounds.m_minimumX, bounds.m_minimumY);
    TPoint upperRight(bounds.m_maximumX - 1, bounds.m_minimumY);
    TPoint lowerLeft(bounds.m_minimumX, bounds.m_maximumY - 1);
    TPoint lowerRight(bounds.m_maximumX - 1, bounds.m_maximumY - 1);
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
            int roughness = zone->m_scaledSize;
            if (neighbour) {
                int ownRoughness = roughness;
                int neighbourRoughness = neighbour->m_scaledSize;
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
            // Follow the map border clockwise, one corner at a time.
            while (from.m_x != to.m_x && from.m_y != to.m_y) {
                TPoint corner;
                if (from.m_x == upperLeft.m_x && from != upperLeft)
                    corner = upperLeft;
                else if (from.m_y == upperRight.m_y && from != upperRight)
                    corner = upperRight;
                else if (from.m_x == lowerRight.m_x && from != lowerRight)
                    corner = lowerRight;
                else
                    corner = lowerLeft;
                drawStraightZoneBoundary(from, corner, zoneIndex, zonePosition.m_z);
                zone->m_boundary.push_back(TPoint(from));
                from = corner;
            }
            drawStraightZoneBoundary(from, to, zoneIndex, zonePosition.m_z);
            zone->m_boundary.push_back(TPoint(from));
        }
    } while (vertex != first);
}

// Reject clipping across an axis limit from its permitted side.
static inline bool crossesRmgBoundaryAxis(
    int original, int clipped, int minimum, int maximum)
{
    return (original >= minimum && clipped < minimum)
        || (original < maximum && clipped >= maximum);
}

// Integer clipping toward `toward` against the left, top, right, then bottom
// edge; a rejected clip returns the point unchanged.
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
static void insertRmgWorkItem(
    std::vector<TRmgZone*>& zones, std::vector<int>& costs,
    TRmgZone* zone, int cost)
{
    int insertionIndex = findRmgWorkItemInsertionIndex(costs, zones.size(), cost);
    costs.insert(costs.begin() + insertionIndex, cost);
    zones.insert(zones.begin() + insertionIndex, 1, zone);
}

// Island-coast variant of drawIrregularZoneBoundary: half the displacement
// range, and it only marks cells already in the zone for terrain painting.
VA(0x0053CD30, 0x212)
MAC_ADDRESS(0x23e348, 0x3f0)
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
        for (int direction = 0; direction < RMG_DIRECTION_COUNT; direction += 2) {
            TRmgMapPosition next = position + g_rmgDirections[direction];
            if (!m_map.containsXY(next))
                continue;
            TRmgMapItem* item = m_map.getMapItem(next);
            if (item->shouldPaintZoneTerrain() || item->m_zoneState.m_zone != zoneIndex)
                continue;
            item->m_tileData.m_paintZoneTerrain = true;
            pending.push_back(next);
        }
    }
}

VA(0x0053D0D0, 0xE3)
MAC_ADDRESS(0x23e928, 0x184)
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

// Moves a coast vertex toward the zone centre by a quarter of its distance,
// at least 4 cells but never more than half.
static inline void insetRmgIslandBoundaryPoint(
    TPoint& point, const TRmgMapPosition& center)
{
    TRmgVector delta(center.m_x - point.m_x, center.m_y - point.m_y);
    int length = delta.length();
    if (length > 0) {
        long displacement = std::max<long>(4, length / 4);
        displacement = std::min<long>(displacement, length / 2);
        delta = delta * displacement / length;
        point += delta;
    }
}

VA(0x0053D1C0, 0x1B9)
MAC_ADDRESS(0x23eaac, 0x2b0)
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
        drawIslandBoundary(point, previous, zoneIndex, center.m_z, zone->m_scaledSize / 2);
    }
    fillIslandInterior(zone);
}

// Scanline bookkeeping for the row above or below (rowStep -1 or 1): the
// first unassigned cell of each span there becomes a seed, queued once the
// span ends.
static inline void scanRmgFillSpan(const TRmgMapItem* neighbour,
    const TRmgMapPosition& position, int rowStep, b8& inSpan,
    TRmgMapPosition& seed, std::vector<TRmgMapPosition>& pending)
{
    if (neighbour->m_zoneState.m_zone == -1) {
        if (!inSpan) {
            seed = position;
            seed.m_y += rowStep;
            inSpan = true;
        }
    } else if (inSpan) {
        inSpan = false;
        pending.push_back(seed);
    }
}

// Four-connected scanline fill of unassigned cells. An out-of-bounds centre
// is clipped toward the interior ring site with the greatest edge clearance;
// with no interior site nothing is filled.
VA(0x0053D380, 0x551)
MAC_ADDRESS(0x23ed74, 0x4a4)
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
                paintsRmgZoneTerrainOnLevel(m_waterContent, position.m_z));
            if (position.m_y > 0)
                scanRmgFillSpan(item - m_map.getWidth(), position, -1,
                    hasUpperSpan, upperSeed, pending);
            if (position.m_y < m_map.getHeight() - 1)
                scanRmgFillSpan(item + m_map.getWidth(), position, 1,
                    hasLowerSpan, lowerSeed, pending);
            ++item;
            ++position.m_x;
        }
        if (hasUpperSpan)
            pending.push_back(upperSeed);
        if (hasLowerSpan)
            pending.push_back(lowerSeed);
    }
}

// Unit-weight distance relaxation with a cost-ordered worklist. The next
// distance comes from the zone's table, not the queued cost.
VA(0x0053D8E0, 0x1EC)
MAC_ADDRESS(0x23f218, 0x2f0)
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
            for (u32 connection = 0; connection < slot->m_connections.size(); ++connection) {
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
            zone->m_zoneDistances[column] = RMG_UNREACHED_COST;
        if (zone->m_templateZone->m_zoneIndex < originalZones)
            zone->m_zoneDistances[zone->m_templateZone->m_zoneIndex] = 0;
    }
}

// Search a zone's closed boundary ring for an edge adjoining another zone,
// testing the starting edge last.
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

// Add both directed records for one unguarded extra-zone connection. The
// player-count limits stay uninitialized; these records skip template
// filtering.
static inline void appendRmgExtraZoneConnection(
    TRmgZone* source, TRmgZone* destination, b8 connected)
{
    TRmgZoneConnection connection;
    connection.m_value = 0;
    connection.m_unguarded = true;
    connection.m_placeBorderObjects = false;
    connection.m_connected = connected;
    appendRmgTwoWayConnection(source->m_templateZone,
        destination->m_templateZone, connection);
}

// Extra-to-extra edges are completed unguarded connections when their boundary
// intersects the map. An extra-to-original edge must not shorten that original
// zone's distance to any other original zone; each one added triggers another
// graph relaxation.
VA(0x0053DAD0, 0x57F)
MAC_ADDRESS(0x23f518, 0x40c)
void type_random_map_generator::joinExtraZones(int originalZones, TRmgVoronoi* diagram)
{
    TRmgZoneBounds bounds = {0, 0, m_map.m_mapWidth, m_map.m_mapHeight};
    for (int index = originalZones; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        TRmgMapPosition position = zone->getLevelPosition();
        TRmgHalfEdge* first = diagram->locate(position);
        for (int other = index + 1; other < m_zones.size(); ++other) {
            TRmgZone* destination = m_zones[other];
            if (destination->getLevelPosition().m_z != position.m_z)
                continue;
            TRmgHalfEdge* edge = findRmgBoundaryWithZone(first, destination);
            if (!edge)
                continue;
            TPoint clipped = clipRmgBoundaryPoint(bounds, edge->m_position, edge->m_previous->m_position);
            if (bounds.contains(clipped))
                appendRmgExtraZoneConnection(zone, destination, true);
        }
    }
    initializeRmgZoneDistances(this, originalZones);
    for (index = 0; index < originalZones; ++index)
        propagateZoneDistances(m_zones[index]);

    for (index = originalZones; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        TRmgMapPosition position = zone->getLevelPosition();
        TRmgHalfEdge* first = diagram->locate(position);
        for (int other = 0; other < originalZones; ++other) {
            TRmgZone* destination = m_zones[other];
            if (destination->getLevelPosition().m_z != position.m_z)
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
            appendRmgExtraZoneConnection(zone, destination, false);
            propagateZoneDistances(destination);
        }
    }
}

VA_COMPGEN(0x0054DE90, 0x14, STD_CONSTRUCT, TRmgZoneConnection)

// Per axis, a radial site lies twice halfOffset from its zone; it may overhang
// only the map edge it points toward, by about halfOffset.
static inline bool isRmgRadialSiteTooFarOffMap(int coordinate, int extent, double halfOffset)
{
    return (coordinate < 0 && coordinate < halfOffset)
        || (coordinate >= extent && coordinate >= extent + halfOffset);
}

// Existing zones seed the subdivision; radial sites add surface water zones
// and underground boundaries.
VA(0x0053E050, 0x64D)
MAC_ADDRESS(0x23f924, 0x798)
void type_random_map_generator::buildZoneBoundaries(
    TRmgTemplate* mapTemplate, int level)
{
    TRmgVoronoi diagram;
    for (int zone = 0; zone < m_zones.size(); ++zone) {
        TRmgMapPosition position = m_zones[zone]->getLevelPosition();
        if (position.m_z == level)
            diagram.addSite(position, m_zones[zone]);
    }
    int originalZones = m_zones.size();
    if (level == RMG_UNDERGROUND_LEVEL || m_waterContent != RMG_WATER_NONE) {
        // Retail bug: testSlot's m_allowedTowns is uninitialized too, so
        // stack contents decide whether testZone's constructor draws a town.
        TRmgTemplateZone testSlot;
        testSlot.m_zoneIndex = -1;
        testSlot.m_kind = RMG_TEMPLATE_JUNCTION;
        testSlot.m_size = 0;
        TRmgZone testZone(&testSlot);
        TRmgZone* addedZone = 0;
        for (zone = 0; zone < originalZones; ++zone) {
            TRmgZone* current = m_zones[zone];
            TRmgMapPosition center = current->getLevelPosition();
            if (center.m_z != level)
                continue;
            int radius = current->m_scaledSize;
            testSlot.m_size = radius;
            TRmgMapPosition position = center;
            for (int direction = 0; direction < RMG_RADIAL_DIRECTION_COUNT; direction += 4) {
                double dx = radius * g_rmgDirectionCosines[direction];
                position.m_x = static_cast<int>(center.m_x + dx * 2);
                double dy = radius * g_rmgDirectionSines[direction];
                position.m_y = static_cast<int>(center.m_y + dy * 2);
                if (isRmgRadialSiteTooFarOffMap(position.m_x, m_map.m_mapWidth, dx)
                    || isRmgRadialSiteTooFarOffMap(position.m_y, m_map.m_mapHeight, dy))
                    continue;
                testZone.setLevelPosition(position);
                if (!canPlaceZone(&testZone))
                    continue;
                if (position.m_z == RMG_SURFACE_LEVEL) {
                    // Retail bug: m_allowedTowns is left uninitialized before the
                    // zone constructor reads it, so heap contents affect RNG use.
                    TRmgTemplateZone* slot = new TRmgTemplateZone;
                    slot->m_zoneIndex = mapTemplate->m_zones.size();
                    slot->m_size = radius;
                    memset(slot->m_allowedMonsters, 0, sizeof(slot->m_allowedMonsters));
                    memset(slot->m_allowedTerrain, 0, sizeof(slot->m_allowedTerrain));
                    memset(slot->m_mineCounts, 0, sizeof(slot->m_mineCounts));
                    memset(slot->m_mineDensities, 0, sizeof(slot->m_mineDensities));
                    memset(slot->m_townPlacement, 0, sizeof(slot->m_townPlacement));
                    slot->m_monsterStrength = RMG_ZONE_MONSTERS_NONE;
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
                diagram.addSite(position, addedZone);
            }
        }
    }
    diagram.buildVertices();
    for (zone = 0; zone < m_zones.size(); ++zone) {
        TRmgMapPosition position = m_zones[zone]->getLevelPosition();
        if (position.m_z == level) {
            TRmgHalfEdge* first = diagram.locate(position);
            traceZoneBoundary(first,
                zone < originalZones && paintsRmgZoneTerrainOnLevel(m_waterContent, level));
        }
    }
    for (zone = 0; zone < m_zones.size(); ++zone) {
        TRmgZone* current = m_zones[zone];
        TRmgMapPosition position = current->getLevelPosition();
        if (position.m_z == level)
            fillZoneArea(current, diagram.locate(position));
    }
    joinExtraZones(originalZones, &diagram);
}

VA(0x0053E6A0, 0x337)
MAC_ADDRESS(0x2400bc, 0x354)
void type_random_map_generator::paintZoneTerrain()
{
    calculateZoneBounds();
    for (int zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        recenterZone(zone);
        if (m_waterContent == RMG_WATER_ISLANDS
            && zone->getLevelPosition().m_z == RMG_SURFACE_LEVEL)
            insetIslandZone(zone);
    }
    if (m_map.getNumberLevels() > 1) {
        type_random_map levelMap(m_map.getMapItem(0, 0, RMG_UNDERGROUND_LEVEL),
            m_map.getWidth(), m_map.getHeight());
        TRmgTerrainBrush brush(&levelMap, eTerrainRock, RMG_BRUSH_STRENGTH);
        brush.paintRectangle(0, 0, m_map.getWidth(), m_map.getHeight());
        if (m_progress)
            m_progress->advance(12500);
    }
    {
        TRmgTerrainBrush brush(&m_map, eTerrainWater, RMG_BRUSH_STRENGTH);
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
            TRmgTerrainBrush brush(&levelMap, zone->getTerrain(), RMG_BRUSH_STRENGTH);
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

// Indices of TRmgNoiseRegion::m_corners, named by their bounds corner.
enum ERmgNoiseCorner {
    RMG_NOISE_MIN_X_MIN_Y = 0,
    RMG_NOISE_MIN_X_MAX_Y = 1,
    RMG_NOISE_MAX_X_MIN_Y = 2,
    RMG_NOISE_MAX_X_MAX_Y = 3
};

// Omit collapsed dimensions, but preserve reversed bounds.
static inline void appendRmgNoiseQuadrant(
    std::vector<TRmgNoiseRegion>& pending, const TRmgNoiseRegion& quadrant)
{
    if (quadrant.m_bounds.m_minimumX != quadrant.m_bounds.m_maximumX
        && quadrant.m_bounds.m_minimumY != quadrant.m_bounds.m_maximumY)
        pending.push_back(quadrant);
}

// Each nondegenerate quadrant inherits the region's variation.
VA(0x0053E9E0, 0x31E)
MAC_ADDRESS(0x240410, 0x2ac)
void subdivideRmgNoiseRegion(std::vector<TRmgNoiseRegion>& pending,
    TRmgNoiseRegion region,
    TRmgNoiseMidpoints midpoints,
    int centerValue)
{
    int middleY = (region.m_bounds.m_minimumY + region.m_bounds.m_maximumY) / 2;
    int middleX = (region.m_bounds.m_minimumX + region.m_bounds.m_maximumX) / 2;
    TRmgNoiseRegion quadrant = region;
    quadrant.m_bounds.m_minimumX = middleX;
    quadrant.m_bounds.m_minimumY = middleY;
    quadrant.m_corners[RMG_NOISE_MIN_X_MIN_Y] = centerValue;
    quadrant.m_corners[RMG_NOISE_MIN_X_MAX_Y] = midpoints.m_maxYValue;
    quadrant.m_corners[RMG_NOISE_MAX_X_MIN_Y] = midpoints.m_maxXValue;
    appendRmgNoiseQuadrant(pending, quadrant);

    quadrant = region;
    quadrant.m_bounds.m_minimumX = middleX;
    quadrant.m_bounds.m_maximumY = middleY;
    quadrant.m_corners[RMG_NOISE_MIN_X_MIN_Y] = midpoints.m_minYValue;
    quadrant.m_corners[RMG_NOISE_MIN_X_MAX_Y] = centerValue;
    quadrant.m_corners[RMG_NOISE_MAX_X_MAX_Y] = midpoints.m_maxXValue;
    appendRmgNoiseQuadrant(pending, quadrant);

    quadrant = region;
    quadrant.m_bounds.m_maximumX = middleX;
    quadrant.m_bounds.m_minimumY = middleY;
    quadrant.m_corners[RMG_NOISE_MIN_X_MIN_Y] = midpoints.m_minXValue;
    quadrant.m_corners[RMG_NOISE_MAX_X_MIN_Y] = centerValue;
    quadrant.m_corners[RMG_NOISE_MAX_X_MAX_Y] = midpoints.m_maxYValue;
    appendRmgNoiseQuadrant(pending, quadrant);

    quadrant = region;
    quadrant.m_bounds.m_maximumX = middleX;
    quadrant.m_bounds.m_maximumY = middleY;
    quadrant.m_corners[RMG_NOISE_MIN_X_MAX_Y] = midpoints.m_minXValue;
    quadrant.m_corners[RMG_NOISE_MAX_X_MIN_Y] = midpoints.m_minYValue;
    quadrant.m_corners[RMG_NOISE_MAX_X_MAX_Y] = centerValue;
    appendRmgNoiseQuadrant(pending, quadrant);
}

VA_COMPGEN(0x0054C670, 0x21, VECTOR_SIZE, TRmgNoiseRegion)

VA_COMPGEN(0x0054D5C0, 0x2E4, VECTOR_INSERT_COUNT, TRmgNoiseRegion)

VA_COMPGEN(0x0054D960, 0x3B, VECTOR_UCOPY, TRmgNoiseRegion)

VA_COMPGEN(0x0054D9A0, 0x31, VECTOR_UFILL, TRmgNoiseRegion)

TRmgZoneConnection* TRmgTemplateZone::findConnection(int destinationZone)
{
    for (u32 connectionIndex = 0; connectionIndex < m_connections.size(); ++connectionIndex) {
        if (m_connections[connectionIndex].m_destination->m_zoneIndex == destinationZone)
            return &m_connections[connectionIndex];
    }
    return 0;
}

void __fastcall generateRmgIslandMask(u8* mask, int width, int height);

VA(0x0053ED00, 0x29B)
MAC_ADDRESS(0x2406bc, 0x484)
void __fastcall generateRmgIslandMask(u8* mask, int width, int height)
{
    std::vector<TRmgNoiseRegion> patches;
    // This noise grid uses bounds X for rows and Y for columns:
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
            int value = min(max(patch.m_corners[RMG_NOISE_MIN_X_MIN_Y], 0), 255);
            mask[patch.m_bounds.m_minimumX * width + patch.m_bounds.m_minimumY] = value;
            continue;
        }
        if (patch.m_bounds.m_maximumX < 0 || patch.m_bounds.m_maximumY < 0
            || patch.m_bounds.m_minimumX >= height || patch.m_bounds.m_minimumY >= width)
            continue;
        const int* corners = patch.m_corners;
        edges.m_minXValue = (corners[RMG_NOISE_MIN_X_MAX_Y] + corners[RMG_NOISE_MIN_X_MIN_Y]) / 2;
        edges.m_minYValue = (corners[RMG_NOISE_MAX_X_MIN_Y] + corners[RMG_NOISE_MIN_X_MIN_Y]) / 2;
        edges.m_maxXValue = (corners[RMG_NOISE_MAX_X_MAX_Y] + corners[RMG_NOISE_MAX_X_MIN_Y]) / 2;
        edges.m_maxYValue = (corners[RMG_NOISE_MAX_X_MAX_Y] + corners[RMG_NOISE_MIN_X_MAX_Y]) / 2;
        int center = (corners[RMG_NOISE_MAX_X_MAX_Y] + corners[RMG_NOISE_MAX_X_MIN_Y]
            + corners[RMG_NOISE_MIN_X_MAX_Y] + corners[RMG_NOISE_MIN_X_MIN_Y]) / 4;
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
    u8* mask = new u8[width * height];
    TRmgMapPosition point;
    int terrain = rand() % 6;
    {
        type_random_map map(m_map.getMapItem(0, 0, level),
            m_map.m_mapWidth, m_map.m_mapHeight);
        TRmgTerrainBrush brush(&map, terrain, RMG_BRUSH_STRENGTH);
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

// Chamfer distance of one step: 2 to a cardinal neighbour, 3 to a diagonal.
static inline int getRmgChamferStepCost(int direction)
{
    return isRmgDiagonalDirection(direction) ? 3 : 2;
}

// Eight-neighbour chamfer distances within one zone; a shorter distance
// resets connection metadata without checking terrain or objects.
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
        popRmgWorkItem(position, positions, costs);
        u32 currentCost = m_map.getMapItem(position)->m_movement.m_zonePathCost;
        for (int direction = 0; direction < RMG_DIRECTION_COUNT; ++direction) {
            TRmgMapPosition next = position + g_rmgDirections[direction];
            if (!m_map.containsXY(next))
                continue;
            TRmgMapItem* item = m_map.getMapItem(next);
            if (item->m_zoneState.m_zone != zoneIndex)
                continue;
            u32 nextCost = currentCost + getRmgChamferStepCost(direction);
            if (nextCost >= item->m_movement.m_zonePathCost)
                continue;
            item->setWaterZoneDistance(nextCost, direction);
            insertRmgWorkItem(positions, costs, next, nextCost);
        }
    }
}

// Seed islands at least 20 distance units from the zone edge and from earlier
// island centres, rebuilding the candidate list after every island.
VA(0x0053F470, 0x409)
MAC_ADDRESS(0x24123c, 0x638)
void type_random_map_generator::placeWaterZoneIslands(TRmgZone* zone)
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
            item->setWaterZoneDistance(RMG_UNREACHED_COST, 0);
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

// markZoneBorders finishes each marked cell, and repairWaterZoneBorders each
// repaired water cell, by releasing path clearance on unoccupied cells of a
// square around it.
static inline void releaseRmgNeighborhoodPathClearance(type_random_map& map,
    const TRmgMapPosition& center, int radius)
{
    TRmgZoneBounds bounds;
    setRmgNeighborhoodBounds(bounds, center, map, radius);
    TRmgMapPosition nearby;
    nearby.m_z = center.m_z;
    for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
        for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
            TRmgMapItem* item = map.getMapItem(nearby);
            if (!item->hasObjects())
                item->releasePathClearance();
        }
    }
}

// Marks dry zone cells for border obstacles beside unassigned water or
// another zone, unless that zone is joined by an unguarded surface connection.
VA(0x0053F880, 0x429)
MAC_ADDRESS(0x241874, 0x628)
void type_random_map_generator::markZoneBorders()
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
                b8 needsBorder = false;
                nearby.m_z = position.m_z;
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        int otherZone = item->m_zoneState.m_zone;
                        if (otherZone < 0) {
                            if (item->getLandType() == eTerrainWater)
                                needsBorder = true;
                        } else if (otherZone != zoneIndex) {
                            TRmgZoneConnection* connection = zone->m_templateZone->findConnection(otherZone);
                            if (!connection || position.m_z == RMG_UNDERGROUND_LEVEL
                                || !connection->m_unguarded)
                                needsBorder = true;
                        }
                    }
                }
                if (!needsBorder)
                    continue;
                current->markBorderObject();
                releaseRmgNeighborhoodPathClearance(m_map, position, 1);
            }
        }
    }
    if (m_progress)
        m_progress->advance(1600);
}

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
                int destinationZone = current->m_zoneState.m_connectionZone;
                if (destinationZone < 0)
                    continue;

                b8 foundLandTerrain = false;
                TRmgZoneBounds bounds;
                setRmgNeighborhoodBounds(bounds, position, m_map, 1);
                nearby.m_z = position.m_z;
                TRmgZone* zone = m_zones[zoneIndex];
                for (nearby.m_y = bounds.m_minimumY;
                     nearby.m_y < bounds.m_maximumY && !foundLandTerrain; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX;
                         ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        if (item->getLandType() != eTerrainWater
                            && item->isPassableLand()
                            && !item->hasBorderObject()) {
                            terrain = H3_ENUM_DECODE(TTerrainType, item->getLandType());
                            foundLandTerrain = true;
                            break;
                        }
                    }
                }
                if (!foundLandTerrain || zone->m_templateZone->findConnection(destinationZone))
                    continue;

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

                releaseRmgNeighborhoodPathClearance(m_map, position, 2);
            }
            if (m_progress)
                m_progress->advance(20);
        }
        if (positions.size()) {
            TTerrainType lastTerrain = terrains[0];
            type_random_map levelMap(m_map.getMapItem(0, 0, position.m_z),
                m_map.m_mapWidth, m_map.m_mapHeight);
            TRmgTerrainBrush brush(&levelMap, lastTerrain, RMG_BRUSH_STRENGTH);
            for (u32 paintIndex = 0; paintIndex < positions.size(); ++paintIndex) {
                terrain = terrains[paintIndex];
                if (terrain != lastTerrain) {
                    brush.changeTerrain(terrain, RMG_BRUSH_STRENGTH);
                    lastTerrain = terrain;
                }
                TRmgMapPosition painted = positions[paintIndex];
                brush.paintRectangle(painted.m_x, painted.m_y, 1, 1);
            }
            positions.clear();
            terrains.clear();
        }
    }
}

// Registers the object and counts its type. An object with an entrance is
// also counted in the entrance's zone and floods entrance distances from it.
VA(0x005402A0, 0x32A)
MAC_ADDRESS(0x24260c, 0x434)
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
        seed->m_zoneState.m_objectDistance = 0;
        positions.push_back(currentPosition);
        costs.push_back(0);
        while (positions.size()) {
            popRmgWorkItem(currentPosition, positions, costs);
            int distance = m_map.getMapItem(currentPosition)->m_zoneState.m_objectDistance;
            for (int direction = 0; direction < RMG_DIRECTION_COUNT; ++direction) {
                int nextCost = distance + getRmgChamferStepCost(direction);
                TRmgMapPosition nextPosition = currentPosition + g_rmgDirections[direction];
                if (!m_map.containsXY(nextPosition))
                    continue;
                TRmgMapItem* next = m_map.getMapItem(nextPosition);
                if (nextCost >= next->m_zoneState.m_objectDistance)
                    continue;
                next->m_zoneState.m_objectDistance = nextCost;
                insertRmgWorkItem(positions, costs, nextPosition, nextCost);
            }
        }
    }
}

// Floods path costs from a seed cell in each zone. Every clear, dry zone cell
// still at nonzero cost then opens a path back along the costs (if reached)
// and reseeds the flood from itself.
VA(0x005405D0, 0x304)
MAC_ADDRESS(0x242a40, 0x448)
void type_random_map_generator::buildZoneConnectionPaths()
{
    int count = m_map.m_numberLevels * m_map.m_mapHeight * m_map.m_mapWidth;
    TRmgMapItem* item = m_map.m_mapItems;
    while (count--) {
        item->m_movement.m_zonePathCost = RMG_UNREACHED_COST;
        item->m_tileData.m_connectionDirection = 0;
        item->m_zoneState.m_connectionZone = -1;
        item->resetMovement();
        ++item;
    }
    for (u32 zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgZoneBounds bounds = zone->m_bounds;
        int level = zone->m_levelPosition.m_z;
        // Retail bug: with no eligible empty cell the scan never assigns
        // seed, yet the fallback below still uses it.
        TRmgMapPosition seed;
        b8 foundClearPath = false;
        for (int y = bounds.m_minimumY; y < bounds.m_maximumY && !foundClearPath; ++y) {
            for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                TRmgMapItem* current = m_map.getMapItem(x, y, level);
                if (current->m_zoneState.m_zone == zoneIndex) {
                    u32 terrain = current->getLandType();
                    if ((terrain != eTerrainWater || zone->m_terrain == terrain)
                        && !current->hasObjects()) {
                        seed = TRmgMapPosition(x, y, level);
                        if (current->hasPathClearance() && current->isPassableLand()) {
                            foundClearPath = true;
                            break;
                        }
                    }
                }
            }
        }
        if (!foundClearPath) {
            TRmgMapItem* current = m_map.getMapItem(seed);
            current->openPath();
        }
        m_map.floodConnectionCosts(seed, zone->m_terrain == eTerrainWater);
        TRmgMapPosition pathPosition = zone->m_levelPosition;
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

// Widen an opened route by removing same-zone border obstacles around the
// cell, keeping cells reserved for a connection.
static inline void clearRmgZonePathBorders(type_random_map& map,
    const TPoint& center, int level, int zoneIndex)
{
    TRmgZoneBounds bounds;
    setRmgNeighborhoodBounds(bounds, center, map, 1);
    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
        for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
            TRmgMapItem* nearby = map.getMapItem(x, y, level);
            if (nearby->m_zoneState.m_zone == zoneIndex)
                nearby->clearBorderObject();
        }
    }
}

// From a reached cell, follow predecessors to the zero-cost seed without
// changing costs; marked connection cells on the way get a border guard of
// their colour. Widened routes clear only the border mark of nearby
// same-zone cells.
VA(0x005408E0, 0x23F)
MAC_ADDRESS(0x242e88, 0x380)
void type_random_map_generator::openConnectionPath(
    TRmgMapPosition position, b8 narrow)
{
    TRmgMapItem* item = m_map.getMapItem(position);
    int zone = item->m_zoneState.m_zone;
    if (item->m_movement.m_cost >= RMG_REACHED_COST_LIMIT)
        return;
    while (item->m_movement.m_cost > 0) {
        if (item->m_connection.m_present) {
            TRmgObjectPropertiesRef* properties = selectObjectPrototype(
                eTerrainDirt, BORDER_GUARD, item->m_connection.m_guardColor);
            type_object* object = new type_object(properties);
            item->clearBorderConnection();
            addObject(object, position);
        }
        item->openPath();
        TRmgMapPosition previous = item->m_previousTile;
        if (!narrow)
            clearRmgZonePathBorders(m_map, position, position.m_z, zone);
        position = previous;
        item = m_map.getMapItem(position);
    }
}

// Picks a random allowed creature for a guard of the given value. Retail bug
// on RoE maps: creature 117 is neither evaluated nor excluded, so it can be
// selected without being counted. Stacks of four or more get two draws of
// size variation.
VA(0x00540B20, 0x240)
MAC_ADDRESS(0x243208, 0x290)
type_object* type_random_map_generator::createGuard(int value, TRmgZone* zone)
{
    // Indexed by town type + 1; entry zero is neutral creatures.
    b8 allowedFactions[TOWN_TYPE_COUNT + 1];
    if (zone->m_templateZone->m_guardsMatchZone && zone->m_alignment != -1) {
        memset(allowedFactions, 0, sizeof(allowedFactions));
        allowedFactions[zone->m_alignment + 1] = true;
    } else {
        memcpy(allowedFactions, zone->m_templateZone->m_allowedMonsters, sizeof(allowedFactions));
    }
    int prototypeIndices[RMG_CREATURE_TYPE_COUNT];
    memset(prototypeIndices, -1, sizeof(prototypeIndices));
    for (u32 index = 0; index < m_objectPrototypes[MONSTER].size(); ++index) {
        TRmgObjectPropertiesRef* properties = m_objectPrototypes[MONSTER][index];
        prototypeIndices[properties->m_prototype->getSubtype()] = index;
    }
    int eligibleCreatureCount = 0;
    int creature = RMG_CREATURE_TYPE_COUNT;
    if (m_mapVersion < RMG_MAP_ARMAGEDDONS_BLADE) {
        while (--creature >= RMG_ROE_CREATURE_TYPE_COUNT)
            prototypeIndices[creature] = -1;
    }
    for (--creature; creature >= 0; --creature) {
        const TCreatureTypeTraits& traits = g_creatureTypeTraits[creature];
        if ((traits.m_wanderingHigh + traits.m_wanderingLow) / 2 * traits.m_aiValue <= value
            && value <= traits.m_aiValue * RMG_GUARD_MAXIMUM_COUNT
            && traits.m_level >= 0 && allowedFactions[traits.m_townType + 1]) {
            // Retail bug: counted even without a loaded prototype, while
            // selection only picks loaded prototypes.
            ++eligibleCreatureCount;
        } else {
            prototypeIndices[creature] = -1;
        }
    }
    if (!eligibleCreatureCount)
        return 0;
    int selectionRank = rand() % eligibleCreatureCount;
    for (creature = RMG_CREATURE_TYPE_COUNT - 1; creature >= 0; --creature) {
        if (prototypeIndices[creature] >= 0 && --selectionRank < 0)
            break;
    }
    // Retail bug: if the counts disagree, creature can reach -1.
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[MONSTER][prototypeIndices[creature]];
    int aiValue = g_creatureTypeTraits[creature].m_aiValue;
    int creatureCount = (value + aiValue / 2) / aiValue;
    int countVariation = creatureCount / 4 + 1;
    if (countVariation > 1) {
        creatureCount = creatureCount + (rand() % countVariation - rand() % countVariation);
    }
    return new rmgMonsterObject(properties, m_nextObjectId++, creatureCount);
}

// First prototype of the requested subtype, or size() when missing.
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
MAC_ADDRESS(0x24356c, 0x2b8)
int type_random_map_generator::placeBorderObject(
    TRmgMapPosition position, int guardCount, TRmgZone* keyTentZone)
{
    int color = m_nextKeyTentColor;
    int tentPrototypeIndex = findRmgPrototypeSubtypeIndex<int>(m_objectPrototypes[BORDER_TENT], color);
    if (tentPrototypeIndex == m_objectPrototypes[BORDER_TENT].size())
        return -1;
    TRmgObjectPropertiesRef* tentProperties = m_objectPrototypes[BORDER_TENT][tentPrototypeIndex];

    int guardPrototypeIndex = findRmgPrototypeSubtypeIndex<int>(m_objectPrototypes[BORDER_GUARD], color);
    // Retail bug: missing guard art returns colour zero, which callers treat
    // as success, unlike the -1 for missing tent art.
    if (guardPrototypeIndex == m_objectPrototypes[BORDER_GUARD].size())
        return 0;
    TRmgObjectPropertiesRef* guardProperties = m_objectPrototypes[BORDER_GUARD][guardPrototypeIndex];
    type_object* tent = new type_object(tentProperties);
    if (!placeObjectInZone(tent, keyTentZone)) {
        delete tent;
        return -1;
    }

    for (int guardIndex = 0; guardIndex < guardCount; ++guardIndex) {
        type_object* guard = new type_object(guardProperties);
        TRmgMapItem* item = m_map.getMapItem(position);
        item->clearBorderConnection();
        addObject(guard, position);
        ++position.m_x;
    }

    setKeyTentColorDisabled(color, true);
    return color;
}

// Marks an empty cell as a border connection; an existing connection keeps
// its tile flags but takes the new guard colour.
static inline void markRmgEmptyBorderConnection(TRmgMapItem* item, int color)
{
    if (!item->hasObjects()) {
        item->markBorderConnection(color);
    }
}

VA(0x00540FC0, 0x172)
MAC_ADDRESS(0x243824, 0x2d4)
void type_random_map_generator::markBorderObjectArea(
    TRmgMapPosition position, int color)
{
    TRmgZoneBounds bounds;
    setRmgNeighborhoodBounds(bounds, position, m_map, 1);
    for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
        for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
            TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
            markRmgEmptyBorderConnection(item, color);
        }
    }
    TRmgMapPosition previous = m_map.getMapItem(position)->m_previousTile;
    if (m_map.containsXY(previous)) {
        TRmgMapItem* item = m_map.getMapItem(previous);
        item->clearBorderConnection();
    }
}

TRmgMapPosition operator+(TRmgMapPosition position, TPoint offset)
{
    return TRmgMapPosition(position.m_x + offset.m_x, position.m_y + offset.m_y, position.m_z);
}

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

// Places a guard of the given value unless the cell is occupied or no
// creature qualifies.
MAC_ADDRESS(0x243498, 0xd4)
void type_random_map_generator::placeGuard(int value, TRmgMapPosition position)
{
    TRmgMapItem* item = m_map.getMapItem(position);
    TRmgZone* zone = m_zones[item->m_zoneState.m_zone];
    if (item->hasObjects())
        return;
    type_object* guard = createGuard(value, zone);
    if (guard)
        addObject(guard, position);
}

// A successful ground border placement marks its surrounding area and
// suppresses guards for this and all remaining crossings.
static inline void placeRmgGroundConnectionBorder(type_random_map_generator& generator,
    TRmgMapPosition position, TRmgZone* keyTentZone, int& guardValue)
{
    int color = generator.placeBorderObject(position, 1, keyTentZone);
    if (color >= 0) {
        generator.markBorderObjectArea(position, color);
        guardValue = 0;
    }
}

// Place a gate border on the approach cell below the gate's entrance. On
// success both gate guards are dropped and the border extends to the empty
// cells either side of it.
static inline void placeRmgGateConnectionBorder(type_random_map_generator& generator,
    TRmgMapPosition approach, TRmgZone* keyTentZone, int& guardValue)
{
    int color = generator.placeBorderObject(approach, 1, keyTentZone);
    if (color >= 0) {
        guardValue = 0;
        TRmgMapPosition side = approach;
        --side.m_x;
        markRmgEmptyBorderConnection(generator.m_map.getMapItem(side), color);
        side.m_x += 2;
        markRmgEmptyBorderConnection(generator.m_map.getMapItem(side), color);
    }
}

// Unguarded template connections need no guard value.
static inline int getRmgConnectionGuardValue(const TRmgZoneConnection* connection,
    const type_random_map_generator& generator)
{
    if (connection->m_unguarded)
        return 0;
    int strength = generator.m_monsterStrength;
    int value = connection->m_value;
    return getRmgGuardValue(value, strength);
}

// Keeps every position tied for the best score; a strictly better score
// restarts the list and becomes the new bound.
static inline void addRmgHighestScoreCandidate(
    std::vector<TRmgMapPosition>& candidates, const TRmgMapPosition& position,
    int score, int& highestScore)
{
    if (score > highestScore) {
        highestScore = score;
        candidates.clear();
    }
    candidates.push_back(position);
}

static inline void addRmgLowestScoreCandidate(
    std::vector<TRmgMapPosition>& candidates, const TRmgMapPosition& position,
    int score, int& lowestScore)
{
    if (score < lowestScore) {
        lowestScore = score;
        candidates.clear();
    }
    candidates.push_back(position);
}

// Adds the object at a uniformly drawn candidate and returns that position.
// The candidate list must be nonempty.
static inline TRmgMapPosition addRmgObjectAtRandomCandidate(
    type_random_map_generator* generator, type_object* object,
    const std::vector<TRmgMapPosition>& candidates)
{
    TRmgMapPosition position = candidates[rand() % candidates.size()];
    generator->addObject(object, position);
    return position;
}

// Connects land zones on one level. Opens one crossing per 40 eligible
// border cells (rounded up), drawn without repeats from the empty ones tied
// for the lowest zone-path cost, which must be at most 100; each gets open
// paths and entrances on both sides before its border or guard.
VA(0x00541140, 0x63A)
MAC_ADDRESS(0x243af8, 0x53c)
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
            && item->m_zoneState.m_connectionZone == destinationZone
            && !item->hasObjects()) {
            TRmgMapPosition other = (*borderPositions)[index]
                + g_rmgDirections[item->m_tileData.m_connectionDirection];
            if (!m_map.getMapItem(other)->hasObjects()) {
                ++eligibleCount;
                int cost = item->m_movement.m_zonePathCost;
                if (cost <= bestCost)
                    addRmgLowestScoreCandidate(candidates, (*borderPositions)[index], cost, bestCost);
            }
        }
    }

    if (candidates.size() == 0)
        return false;

    int guardValue = getRmgConnectionGuardValue(connection, *this);

    if (bestCost == 1 && guardValue == 0 && !connection->m_placeBorderObjects)
        return true;

    int count = min(candidates.size(), (eligibleCount + 39) / 40);
    for (int crossing = 0; crossing < count; ++crossing) {
        int selected = rand() % candidates.size();
        TRmgMapPosition position = candidates[selected];
        TPoint direction = g_rmgDirections[
            m_map.getMapItem(position)->m_tileData.m_connectionDirection];
        TRmgMapPosition otherPosition = position + direction;

        openConnectionPath(position, connection->m_placeBorderObjects);
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

// Cardinal flood across path-clearance water; adjoining land is marked
// visited but not expanded.
VA(0x00541780, 0x18D)
MAC_ADDRESS(0x244034, 0x254)
void type_random_map_generator::floodConnectionRegion(TRmgMapPosition position)
{
    std::vector<TRmgMapPosition> openPositions;
    openPositions.push_back(position);
    m_map.getMapItem(position)->setConnectionVisited();
    while (openPositions.size()) {
        position = openPositions.back();
        openPositions.pop_back();
        for (int direction = 0; direction < RMG_DIRECTION_COUNT; direction += 2) {
            TRmgMapPosition nearby = position + g_rmgDirections[direction];
            if (!m_map.containsXY(nearby))
                continue;
            TRmgMapItem* item = m_map.getMapItem(nearby);
            if (item->isConnectionVisited())
                continue;
            if (!item->hasPathClearance() && item->getLandType() == eTerrainWater)
                continue;
            item->setConnectionVisited();
            if (item->getLandType() == eTerrainWater)
                openPositions.push_back(nearby);
        }
    }
}

// Water cell probed beside a shipyard; callers bounds-check only its column.
static TRmgMapPosition getRmgShipyardWaterPosition(TRmgMapPosition shipyardPosition,
    int waterOffset);

VA(0x00541960, 0x16C)
MAC_ADDRESS(0x244288, 0x2f4)
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
            if (item->isRoadEntrance() || !item->isPassableLand())
                return false;
        }
    }
    int waterOffset;
    for (waterOffset = 0; waterOffset < RMG_SHIPYARD_WATER_OFFSET_COUNT; ++waterOffset) {
        TRmgMapPosition water = getRmgShipyardWaterPosition(position, waterOffset);
        if (water.m_x < 0 || water.m_x >= m_map.m_mapWidth)
            continue;
        TRmgMapItem* item = m_map.getMapItem(water);
        int terrain = item->getLandType();
        if (terrain == eTerrainWater && item->hasPathClearance())
            break;
    }
    if (waterOffset == RMG_SHIPYARD_WATER_OFFSET_COUNT)
        return false;
    TRmgMapPosition farSide = position;
    if (g_rmgShipyardWaterOffsets[waterOffset].m_x < 0)
        ++farSide.m_x;
    else
        farSide.m_x -= 3;
    if (farSide.m_x < 0 || farSide.m_x >= m_map.m_mapWidth)
        return false;
    int terrain = m_map.getMapItem(farSide)->getLandType();
    return terrain != eTerrainWater;
}

static TRmgMapPosition getRmgShipyardWaterPosition(TRmgMapPosition shipyardPosition,
    int waterOffset)
{
    return shipyardPosition + g_rmgShipyardWaterOffsets[waterOffset];
}

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

// Connects zones by a shipyard, or succeeds without one when water flooded
// from this zone's shipyards already reaches a cell facing the destination.
// A successful border placement clears the guard value.
VA(0x00541AD0, 0x5B0)
MAC_ADDRESS(0x2446ac, 0x55c)
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
                    && item->m_zoneState.m_connectionZone == destinationZone) {
                    if (item->isConnectionVisited())
                        return true;
                    if (item->getLandType() != eTerrainWater) {
                        TRmgMapPosition site = position;
                        if (site.m_y + 1 < m_map.m_mapHeight) {
                            for (site.m_x = position.m_x;
                                 site.m_x <= position.m_x + 2; ++site.m_x) {
                                if (m_map.canPlaceObject(properties, site, source)
                                    && canPlaceShipyard(site))
                                    candidates.push_back(site);
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
    TRmgMapPosition position = addRmgObjectAtRandomCandidate(this, shipyard, candidates);

    TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, prototype->m_triggerCell);
    m_roadTargets.push_back(entrance);

    // Open the row below the footprint as source-zone entrances.
    TRmgMapPosition approach = position;
    ++approach.m_y;
    for (approach.m_x = position.m_x - prototype->getWidth() + 1;
         approach.m_x <= position.m_x; ++approach.m_x) {
        TRmgMapItem* item = m_map.getMapItem(approach);
        item->openPath();
        source->m_entrances.push_back(TPoint(approach.m_x, approach.m_y));
    }

    floodShipyardWater(shipyard);

    int guardValue = getRmgConnectionGuardValue(connection, *this);

    // Border guards and the monster guard stand on that row, centred on the
    // entrance column.
    if (connection->m_placeBorderObjects) {
        approach.m_x = entrance.m_x - 1;
        if (placeBorderObject(approach, 3, destination) >= 0)
            guardValue = 0;
    }
    if (guardValue > 0) {
        approach.m_x = entrance.m_x;
        placeGuard(guardValue, approach);
    }
    return true;
}

// Opens the approach cell directly below an object entrance and returns it.
static inline TRmgMapPosition openRmgEntranceApproach(type_random_map& map,
    TRmgMapPosition entrance)
{
    TRmgMapPosition approach = entrance;
    ++approach.m_y;
    map.getMapItem(approach)->openPath();
    return approach;
}

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

    // Gate sites lie where the two zones' bounds overlap.
    TRmgZoneBounds overlap = source->m_bounds;
    {
        TRmgZoneBounds destinationBounds = destination->m_bounds;
        overlap.m_minimumX = max(
            overlap.m_minimumX, destinationBounds.m_minimumX);
        overlap.m_minimumY = max(
            overlap.m_minimumY, destinationBounds.m_minimumY);
        overlap.m_maximumX = min(
            overlap.m_maximumX, destinationBounds.m_maximumX);
        overlap.m_maximumY = min(
            overlap.m_maximumY, destinationBounds.m_maximumY);
    }
    if (overlap.m_minimumX >= overlap.m_maximumX || overlap.m_minimumY >= overlap.m_maximumY)
        return false;

    int gateIndex = rand() % m_objectPrototypes[UNDERGROUND_GATE].size();
    TRmgObjectPropertiesRef* gateProperties = m_objectPrototypes[UNDERGROUND_GATE][gateIndex];
    TObjectType* gatePrototype = gateProperties->m_prototype;

    std::vector<TRmgMapPosition> candidates;
    int bestScore = 0;
    TRmgMapPosition position;
    position = source->getLevelPosition();

    for (position.m_y = overlap.m_minimumY; position.m_y < overlap.m_maximumY; ++position.m_y) {
        for (position.m_x = overlap.m_minimumX; position.m_x < overlap.m_maximumX; ++position.m_x) {
            TRmgMapItem* sourceItem = m_map.getMapItem(position);
            if (sourceItem->m_zoneState.m_zone != sourceZone)
                continue;
            int score = sourceItem->m_zoneState.m_objectDistance;

            TRmgMapPosition otherPosition(position.m_x, position.m_y,
                destination->getLevelPosition().m_z);
            TRmgMapItem* destinationItem = m_map.getMapItem(otherPosition);
            if (destinationItem->m_zoneState.m_zone != destinationZone)
                continue;

            score += destinationItem->m_zoneState.m_objectDistance;
            if (score < bestScore)
                continue;
            if (!m_map.canPlaceObject(gateProperties, position, source))
                continue;
            if (!m_map.canPlaceObject(
                    gateProperties, otherPosition, destination))
                continue;

            addRmgHighestScoreCandidate(candidates, position, score, bestScore);
        }
    }

    if (candidates.size() == 0)
        return false;

    TRmgMapPosition gatePosition = addRmgObjectAtRandomCandidate(
        this, new type_object(gateProperties), candidates);

    TRmgMapPosition otherGatePosition(gatePosition.m_x, gatePosition.m_y,
        destination->getLevelPosition().m_z);
    addObject(new type_object(gateProperties), otherGatePosition);

    TRmgMapPosition entrance =
        getRmgObjectTriggerPosition(gatePosition, gatePrototype->m_triggerCell);
    TRmgMapPosition otherEntrance(entrance.m_x, entrance.m_y,
        destination->getLevelPosition().m_z);
    source->m_entrances.push_back(TPoint(entrance.m_x, entrance.m_y));
    destination->m_entrances.push_back(
        TPoint(otherEntrance.m_x, otherEntrance.m_y));

    int guardValue = getRmgConnectionGuardValue(connection, *this);

    TRmgMapPosition approach = openRmgEntranceApproach(m_map, entrance);
    TRmgMapPosition otherApproach = openRmgEntranceApproach(m_map, otherEntrance);

    if (connection->m_placeBorderObjects) {
        // Success on either side suppresses both guards; a failed placement
        // does not undo the other side's objects.
        placeRmgGateConnectionBorder(*this, approach, destination, guardValue);
        placeRmgGateConnectionBorder(*this, otherApproach, source, guardValue);
    }

    if (guardValue > 0) {
        placeGuard(guardValue, approach);
        placeGuard(guardValue, otherApproach);
    }

    return true;
}

// Object positions name the lower-right footprint cell. Inset the minimum
// anchor coordinates so the whole footprint fits.
static inline void insetRmgObjectPlacementBounds(
    TRmgZoneBounds& bounds, const TObjectType* prototype)
{
    bounds.m_minimumY += prototype->getHeight() - 1;
    bounds.m_minimumX += prototype->getWidth() - 1;
}

// Places an object at a random fitting cell of the zone.
VA(0x00542930, 0x1C6)
MAC_ADDRESS(0x245624, 0x24c)
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
    addRmgObjectAtRandomCandidate(this, object, candidates);
    return true;
}

// Rebuilds the zone connection paths, then puts the border on the portal
// cell's path predecessor; a zero-cost cell uses the first same-zone open
// neighbour, else the cell below, and an unreached cell fails. Success also
// marks the cell's five side and lower neighbours as border connections.
VA(0x00542B00, 0x1D2)
MAC_ADDRESS(0x245870, 0x364)
b8 type_random_map_generator::placeMonolithBorder(
    TRmgMapPosition position, TRmgZone* keyTentZone)
{
    TPoint offsets[5] = {
        TPoint(0, 1), TPoint(1, 0), TPoint(-1, 0), TPoint(1, 1), TPoint(-1, 1)
    };
    TRmgMapPosition borderPosition;
    const int directionCount = sizeof(offsets) / sizeof(offsets[0]);
    buildZoneConnectionPaths();
    TRmgMapItem* item = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
    int zoneIndex = item->m_zoneState.m_zone;
    u32 movementCost = item->m_movement.m_cost;
    if (movementCost >= RMG_REACHED_COST_LIMIT)
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
    int color = placeBorderObject(borderPosition, 1, keyTentZone);
    if (color >= 0) {
        for (int direction = 0; direction < directionCount; ++direction) {
            TPoint offset = offsets[direction];
            TRmgMapPosition nearby = position + offset;
            TRmgMapItem* neighbor = m_map.getMapItem(nearby);
            neighbor->markBorderConnection(color);
        }
    }
    return color >= 0;
}

// Places one portal; a failed placement deletes it.
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

// Protects a portal with a border guard keyed to a tent in the other zone,
// or else with a monster guard on the cell below it.
static inline void protectRmgMonolith(type_random_map_generator& generator,
    type_object* portal, const TRmgZoneConnection* connection,
    TRmgZone* keyTentZone, int& guardValue)
{
    if (connection->m_placeBorderObjects
        && generator.placeMonolithBorder(portal->getPosition(), keyTentZone)) {
        guardValue = 0;
    } else if (guardValue > 0) {
        generator.placeGuard(guardValue, portal->getPosition() + TPoint(0, 1));
    }
}

// One-way prototypes produce an entrance/exit pair in each zone. Failed
// placement deletes only that object; subsequent endpoint attempts continue.
VA(0x00542CE0, 0x554)
MAC_ADDRESS(0x245bd4, 0x6e8)
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
        int oneWayIndex = prototypeIndex - m_objectPrototypes[LITH_TWOWAY].size();
        properties = m_objectPrototypes[LITH_ONEWAY_ENTRANCE][oneWayIndex];
        exitProperties = m_objectPrototypes[LITH_ONEWAY_EXIT][oneWayIndex];
    }
    int guardValue = getRmgConnectionGuardValue(connection, *this);

    // A source-side border also drops the destination's guard, even when the
    // destination's own border placement later fails.
    type_object* object = placeRmgMonolith(*this, properties, source, exitProperties != 0);
    if (object)
        protectRmgMonolith(*this, object, connection, destination, guardValue);
    object = placeRmgMonolith(*this, properties, destination, exitProperties != 0);
    if (object)
        protectRmgMonolith(*this, object, connection, source, guardValue);
    if (exitProperties) {
        placeRmgMonolith(*this, exitProperties, source, true);
        placeRmgMonolith(*this, exitProperties, destination, true);
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

// Land crossings, gates and portals complete both directed records;
// shipyards complete only their own direction.
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

// First pass: land crossings, then shipyards, then subterranean gates.
// Second pass: remaining connections get a shipyard, or monoliths between
// land zones.
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
                if (mapItem->m_zoneState.m_connectionZone < 0)
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

    // Unused.
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

        clearRmgConnectionVisits(m_map, zone->getLevelPosition().m_z);

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

        clearRmgConnectionVisits(m_map, zone->getLevelPosition().m_z);

        for (int objectIndex = 0; objectIndex < m_objects.size(); ++objectIndex) {
            type_object* object = m_objects[objectIndex];
            if (object->m_properties->m_prototype->getObjectType() == SHIPYARD) {
                TRmgMapPosition shipyardPosition = object->getPosition();
                if (m_map.getMapItem(shipyardPosition)->m_zoneState.m_zone == zoneIndex) {
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
        m_progress->advance(6400);
}

// Underground rock and treasure-group filler may close passable floor only
// outside path and entrance reservations; water is not excluded here.
static inline bool canBlockRmgFloorCell(const TRmgMapItem* item)
{
    return !item->hasPathClearance() && item->isPassableLand()
        && !item->isRoadEntrance();
}

VA(0x005439E0, 0x283)
MAC_ADDRESS(0x246a34, 0x31c)
void type_random_map_generator::decorateUnderground()
{
    TRmgMapPosition scan;
    scan.m_z = RMG_UNDERGROUND_LEVEL;
    TRmgMapItem* item = m_map.getMapItem(0, 0, scan.m_z);
    type_random_map map(item, m_map.m_mapWidth, m_map.m_mapHeight);
    TRmgTerrainBrush brush(&map, eTerrainRock, RMG_BRUSH_STRENGTH);
    for (scan.m_y = 0; scan.m_y < m_map.m_mapHeight; ++scan.m_y) {
        for (scan.m_x = 0; scan.m_x < m_map.m_mapWidth; ++scan.m_x, ++item) {
            if (canBlockRmgFloorCell(item))
                brush.paintRectangle(scan.m_x, scan.m_y, 1, 1);
        }
    }
    if (m_progress)
        m_progress->advance(1200);
    int currentTerrain = eTerrainRock;
    for (u32 zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->getLevelPosition().m_z != RMG_UNDERGROUND_LEVEL)
            continue;
        TRmgZoneBounds bounds = m_zones[zone]->m_bounds;
        int terrain = m_zones[zone]->m_terrain;
        if (currentTerrain == eTerrainRock) {
            brush.changeTerrain(terrain, RMG_BRUSH_STRENGTH);
            currentTerrain = terrain;
        }
        for (int y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
            for (int x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                TRmgMapItem* cell = m_map.getMapItem(x, y, RMG_UNDERGROUND_LEVEL);
                if (cell->getLandType() == eTerrainRock
                    && cell->m_zoneState.m_zone == zone
                    && (cell->hasPathClearance() || cell->hasObjects())) {
                    if (terrain != currentTerrain) {
                        brush.changeTerrain(terrain, RMG_BRUSH_STRENGTH);
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

// This Bresenham-style ray continues beyond toward until it reaches the map's
// outermost ring or a path-clearance tile in its 3x3 neighbourhood. The first
// two steps ignore neighbours; the returned point precedes the obstruction.
VA(0x00543C70, 0x1A2)
MAC_ADDRESS(0x246d50, 0x30c)
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
    TPoint previous;
    for (;;) {
        previous = from;
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
            return previous;
        if (steps > 2) {
            TRmgMapPosition nearby;
            nearby.m_z = level;
            for (nearby.m_x = from.m_x - 1; nearby.m_x <= from.m_x + 1; ++nearby.m_x) {
                for (nearby.m_y = from.m_y - 1; nearby.m_y <= from.m_y + 1; ++nearby.m_y) {
                    if (getMapItem(nearby)->hasPathClearance())
                        return previous;
                }
            }
        }
    }
}

bool type_random_map_generator::contains(const TPoint& point) const
{
    return m_map.containsXY(point);
}

// Midpoint subdivision uses a LIFO stack; deferred perpendicular branches
// use a FIFO queue.
VA(0x00543E20, 0x574)
MAC_ADDRESS(0x24705c, 0x750)
void type_random_map_generator::carveBranchingPaths()
{
    TRmgMapItem* item = m_map.m_mapItems;
    int remaining = m_map.getHeight() * m_map.getWidth() * m_map.m_numberLevels;
    for (; remaining--; ++item) {
        if (!item->hasObjects()) {
            item->markBorderObject();
        } else {
            item->openPath();
        }
    }
    for (int level = 0; level < m_map.m_numberLevels; ++level) {
        TPoint first;
        TPoint last;
        switch (rand() % RMG_BRANCH_SEED_PATTERN_COUNT) {
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
                        TPoint toward = middle + perpendicular;
                        branches.push(middle);
                        branches.push(toward);
                        toward = TPoint(middle.m_x - perpendicular.m_x, middle.m_y - perpendicular.m_y);
                        branches.push(middle);
                        branches.push(toward);
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
    int roughness = zone->m_scaledSize;
    pending.push_back(to);
    while (pending.size() > 0) {
        to = pending.back();
        pending.pop_back();
        if (!splitRmgBoundarySegment(pending, from, to, roughness, 1)) {
            TPoint clamped = clampRmgBoundaryToMap(from, m_map);
            TRmgMapItem* item = m_map.getMapItem(clamped.m_x, clamped.m_y, position.m_z);
            if (item->m_zoneState.m_zone == zoneIndex) {
                item->openPath();
                clearRmgZonePathBorders(m_map, clamped, position.m_z, zoneIndex);
            }
            from = to;
        }
    }
}

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
                item->resetMovement();
                if (!item->hasObjects()) {
                    item->markBorderObject();
                }
            }
        }
    }
    if (!zone->m_entrances.size())
        return;
    TRmgMapPosition first(zone->m_entrances[0].m_x, zone->m_entrances[0].m_y, level);
    TRmgMapItem* item = m_map.getMapItem(first.m_x, first.m_y, first.m_z);
    item->setMovementCost(0, TRmgMapPosition(-1, -1, -1));
    m_map.floodConnectionCosts(first, false);
    for (int entrance = 1; entrance < static_cast<int>(zone->m_entrances.size()); ++entrance) {
        TPoint from = zone->m_entrances[entrance];
        item = m_map.getMapItem(from.m_x, from.m_y, level);
        u32 cost = item->m_movement.m_cost;
        if (!cost || cost > RMG_REACHED_COST_LIMIT)
            continue;
        // Follow predecessors from the positive-cost entrance to the seed.
        TRmgMapPosition previous;
        do {
            previous = item->m_previousTile;
            item = m_map.getMapItem(previous.m_x, previous.m_y, previous.m_z);
        } while (item->m_movement.m_cost > 0);
        connectJunctionEntrance(from, TPoint(previous.m_x, previous.m_y), zone);
        m_map.floodConnectionCosts(TRmgMapPosition(from.m_x, from.m_y, level), false);
    }
}

VA(0x00544920, 0x124)
MAC_ADDRESS(0x24817c, 0x1ac)
void type_random_map_generator::prepareZoneConnections()
{
    carveBranchingPaths();
    markZoneBorders();
    TRmgMapItem* item = m_map.m_mapItems;
    TRmgMapPosition position;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (!item->hasBorderObject() && item->isPassableLand() && !item->isRoadEntrance()
                    && !item->hasObjects()
                    && item->m_zoneState.m_zone < 0 && item->getLandType() != eTerrainWater)
                    m_map.markBorderPatch(position);
            }
        }
    }
    for (u32 zone = 0; zone < m_zones.size(); ++zone)
        placeWaterZoneIslands(m_zones[zone]);
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
    if (slot->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_COUNT] > 0
        && tryPlacePrimaryTown(zone, alignment, player, true))
        return;
    if (slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT] > 0
        && tryPlacePrimaryTown(zone, alignment, player, false))
        return;
    if (slot->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_COUNT] > 0
        && tryPlacePrimaryTown(zone, alignment, -1, true))
        return;
    if (slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT] > 0)
        tryPlacePrimaryTown(zone, alignment, -1, false);
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

// Object spacing for a density given per reference area (integer area
// division, truncated square root).
static inline int getRmgDensitySpacing(int referenceArea, int density)
{
    return static_cast<int>(sqrt(static_cast<double>(referenceArea / density)));
}

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

// Picks the unfinished category with the lowest weighted count; ties go to
// the first.
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
// town.
static inline void placeRmgFixedTownCategory(type_random_map_generator* generator,
    TRmgZone* zone, int count, int alignment, int player,
    b8 hasFort, b8& skipPrimary)
{
    if (count <= 0)
        return;
    int townIndex = skipPrimary ? 1 : 0;
    for (; townIndex < count; ++townIndex)
        generator->tryPlaceAdditionalTown(zone, alignment, player, hasFort, 0);
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
        slot->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_COUNT], alignment, player, true, skipPrimary);
    placeRmgFixedTownCategory(this, zone,
        slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT], alignment, player, false, skipPrimary);
    placeRmgFixedTownCategory(this, zone,
        slot->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_COUNT], alignment, -1, true, skipPrimary);
    placeRmgFixedTownCategory(this, zone,
        slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT], alignment, -1, false, skipPrimary);
    int totalDensity = 0;
    int densityProduct = 1;
    // Indexed by ERmgTownPlacementCategory.
    const int categoryCount = RMG_TOWN_NEUTRAL_BASIC + 1;
    int densities[categoryCount] = {
        slot->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_DENSITY],
        slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_DENSITY],
        slot->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_DENSITY],
        slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_DENSITY]
    };
    int weightedCounts[categoryCount] = {
        slot->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_COUNT],
        slot->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT],
        slot->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_COUNT],
        slot->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT]
    };
    int countSteps[categoryCount];
    b8 finished[categoryCount];
    initializeRmgDensityCategories(densities, categoryCount, finished, totalDensity, densityProduct);
    if (!totalDensity)
        return;
    int spacing = getRmgDensitySpacing(82944, totalDensity);
    initializeRmgCategoryStrides(densities, weightedCounts, categoryCount,
        densityProduct, countSteps, weightedCounts);
    for (;;) {
        int selected = selectRmgWeightedCategory(finished, weightedCounts, categoryCount);
        if (selected == -1)
            break;
        weightedCounts[selected] += countSteps[selected];
        b8 playerTown = selected == RMG_TOWN_PLAYER_CASTLE || selected == RMG_TOWN_PLAYER_BASIC;
        b8 hasFort = selected == RMG_TOWN_PLAYER_CASTLE || selected == RMG_TOWN_NEUTRAL_CASTLE;
        if (!tryPlaceAdditionalTown(zone, alignment, playerTown ? player : -1, hasFort, spacing))
            finished[selected] = true;
    }
}

// Places a town at a random candidate and returns its entrance, now a road
// target with the cell below it opened.
static inline TRmgMapPosition placeRmgTownAtRandomCandidate(
    type_random_map_generator* generator, TRmgObjectPropertiesRef* properties,
    int player, b8 hasFort,
    const std::vector<TRmgMapPosition>& candidates, const TObjectType::TPoint& trigger)
{
    rmgTownObject* town = new rmgTownObject(properties,
        generator->m_nextObjectId++, player, hasFort);
    TRmgMapPosition position = addRmgObjectAtRandomCandidate(generator, town, candidates);
    TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, trigger);
    generator->m_roadTargets.push_back(entrance);
    openRmgEntranceApproach(generator->m_map, entrance);
    return entrance;
}

// A zone without a primary town gets this one as its primary town.
// Otherwise the entrance must be at least spacing from other objects, with
// its clipped 3x3 neighbourhood in the zone; the farthest such sites win.
VA(0x00544D90, 0x4B7)
MAC_ADDRESS(0x24882c, 0x5d4)
b8 type_random_map_generator::tryPlaceAdditionalTown(TRmgZone* zone,
    int alignment, int player, b8 hasFort, int spacing)
{
    TRmgTemplateZone* slot = zone->m_templateZone;
    if ((player == -1 && !slot->m_neutralTownsMatchZone) || alignment == -1) {
        alignment = slot->selectAllowedTown();
        // RoE maps have no Conflux.
        if (alignment == -1)
            alignment = rand() % (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE
                ? TOWN_TYPE_COUNT : TOWN_CONFLUX);
    }
    if (!zone->m_active)
        return tryPlacePrimaryTown(zone, alignment, player, hasFort);

    std::vector<TRmgMapPosition> candidates;
    int zoneIndex = slot->m_zoneIndex;
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[TOWN][alignment];
    TObjectType* prototype = properties->m_prototype;
    TObjectType::TPoint trigger = prototype->m_triggerCell;
    TRmgMapPosition position = zone->getLevelPosition();
    TRmgZoneBounds bounds = zone->m_bounds;
    bounds.m_minimumY += prototype->getHeight();
    bounds.m_minimumX += prototype->getWidth();
    int bestScore = spacing;
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, trigger);
            TRmgMapItem* item = m_map.getMapItem(entrance.m_x, entrance.m_y, entrance.m_z);
            if (item->m_zoneState.m_zone != zoneIndex)
                continue;
            int score = item->m_zoneState.m_objectDistance;
            if (score < bestScore || !m_map.canPlaceObject(properties, position, zone))
                continue;
            TRmgZoneBounds nearby;
            setRmgNeighborhoodBounds(nearby, entrance, m_map, 1);
            b8 valid = true;
            for (int y = nearby.m_minimumY; y < nearby.m_maximumY; ++y) {
                for (int x = nearby.m_minimumX; x < nearby.m_maximumX; ++x) {
                    if (m_map.getMapItem(x, y, entrance.m_z)->m_zoneState.m_zone != zoneIndex)
                        valid = false;
                }
            }
            if (!valid)
                continue;
            addRmgHighestScoreCandidate(candidates, position, score, bestScore);
        }
    }
    if (!candidates.size())
        return false;
    placeRmgTownAtRandomCandidate(this, properties,
        player, hasFort, candidates, trigger);
    return true;
}

VA(0x00545250, 0x324)
MAC_ADDRESS(0x248e00, 0x3b4)
b8 type_random_map_generator::tryPlacePrimaryTown(
    TRmgZone* zone, int alignment, int player, b8 hasFort)
{
    if (alignment == -1)
        return false;
    std::vector<TRmgMapPosition> candidates;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition center = zone->m_levelPosition;
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[TOWN][alignment];
    TObjectType* prototype = properties->m_prototype;
    int bestDistance = 32000;
    TRmgMapPosition site;
    site.m_z = center.m_z;
    TRmgZoneBounds bounds = zone->m_bounds;
    for (site.m_y = bounds.m_minimumY; site.m_y < bounds.m_maximumY; ++site.m_y) {
        for (site.m_x = bounds.m_minimumX; site.m_x < bounds.m_maximumX; ++site.m_x) {
            if (m_map.getMapItem(site)->m_zoneState.m_zone != zoneIndex)
                continue;
            int distance = getRmgSquaredDistance(site, center);
            if (distance <= bestDistance && m_map.canPlaceObject(properties, site, zone))
                addRmgLowestScoreCandidate(candidates, site, distance, bestDistance);
        }
    }
    if (!candidates.size())
        return false;
    zone->m_position = placeRmgTownAtRandomCandidate(this, properties,
        player, hasFort, candidates, prototype->m_triggerCell);
    zone->m_active = true;
    return true;
}

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
    int bestScore = spacing;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    // The mine position whose entrance would be the town entrance, so anchor
    // distances below equal entrance-to-entrance distances.
    TRmgMapPosition townAlignedAnchor;
    if (startingMine) {
        townAlignedAnchor = zone->m_position;
        townAlignedAnchor += TPoint(prototype->m_triggerCell.m_x, prototype->m_triggerCell.m_y);
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
                int distance = getRmgSquaredDistance(position, townAlignedAnchor);
                if (distance > bestDistance || distance < 16)
                    continue;
                if (distance < 144)
                    distance = 144;
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestBorderCount = 0;
                    bestScore = 0;
                    candidates.clear();
                }
            }
            int score = item->m_zoneState.m_objectDistance;
            if (score < bestScore)
                continue;
            int borderCount = 0;
            for (u32 i = 0; i < properties->m_outline.size(); ++i) {
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
            addRmgHighestScoreCandidate(candidates, position, score, bestScore);
        }
    }
    if (!candidates.size())
        return false;
    addRmgObjectAtRandomCandidate(this, object, candidates);
    return true;
}

// Guard value for a zone object; none in a zone without monsters. Map
// strength shares the zone scale (2 weak, 3 normal, 4 strong), so an average
// zone keeps the map strength that connection guards use.
static inline int getRmgZoneGuardValue(int value, const TRmgZone* zone,
    const type_random_map_generator& generator)
{
    int zoneStrength = zone->m_templateZone->m_monsterStrength;
    if (zoneStrength == RMG_ZONE_MONSTERS_NONE)
        return 0;
    int strength = zoneStrength + generator.m_monsterStrength - RMG_ZONE_MONSTERS_AVERAGE;
    if (strength > 5) strength = 5;
    else if (strength < 0) strength = 0;
    return getRmgGuardValue(value, strength);
}

// Mine guard value by resource; zero when the zone has no monsters.
int type_random_map_generator::getMineGuardValue(int resource, const TRmgZone* zone) const
{
    int value;
    switch (resource) {
    case WOOD: case ORE: value = 1500; break;
    case GOLD: value = 7000; break;
    default: value = 3500; break;
    }
    return getRmgZoneGuardValue(value, zone, *this);
}

// Retail bug: the entrance and resource area use the trigger and width of
// the last scanned mine prototype, not the selected one.
VA(0x00545990, 0x466)
MAC_ADDRESS(0x249680, 0x5ac)
b8 type_random_map_generator::tryPlaceMine(TRmgZone* zone,
    int resource, b8 startingMine, int spacing)
{
    std::vector<TRmgObjectPropertiesRef*> candidates;
    int terrain = zone->m_terrain;
    TObjectType* lastScannedPrototype;
    for (u32 i = 0; i < m_objectPrototypes[MINE].size(); ++i) {
        TRmgObjectPropertiesRef* properties = m_objectPrototypes[MINE][i];
        lastScannedPrototype = properties->m_prototype;
        if (lastScannedPrototype->getSubtype() == resource
            && lastScannedPrototype->isRecommendedTerrain(terrain))
            candidates.push_back(properties);
    }
    if (!candidates.size()) {
        for (u32 i = 0; i < m_objectPrototypes[MINE].size(); ++i) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[MINE][i];
            lastScannedPrototype = properties->m_prototype;
            if (lastScannedPrototype->getSubtype() == resource)
                candidates.push_back(properties);
        }
    }
    if (!candidates.size())
        return false;
    u32 selected = rand() % candidates.size();
    TRmgObjectPropertiesRef* selectedProperties = candidates[selected];
    rmgOwnableObject* mine = new rmgOwnableObject(selectedProperties);
    if (!placeMineSite(mine, zone, startingMine, spacing)) {
        delete mine;
        return false;
    }
    int guardValue = getMineGuardValue(resource, zone);
    TRmgMapPosition approach = openRmgEntranceApproach(m_map, getRmgObjectTriggerPosition(
        mine->getPosition(), lastScannedPrototype->m_triggerCell));
    if (guardValue > 0)
        placeGuard(guardValue, approach);
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
    b8 finished[NUM_RESOURCES];
    int totalDensity = 0;
    int densityProduct = 1;
    initializeRmgDensityCategories(slot->m_mineDensities, NUM_RESOURCES,
        finished, totalDensity, densityProduct);
    if (!totalDensity)
        return;
    int spacing = getRmgDensitySpacing(82944, totalDensity);
    int countSteps[NUM_RESOURCES];
    int weightedCounts[NUM_RESOURCES];
    initializeRmgCategoryStrides(slot->m_mineDensities, slot->m_mineCounts, NUM_RESOURCES,
        densityProduct, countSteps, weightedCounts);
    for (;;) {
        int selected = selectRmgWeightedCategory(finished, weightedCounts, NUM_RESOURCES);
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
    for (u32 index = 0; index < m_zones.size(); ++index) {
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

VA(0x00546040, 0x141)
MAC_ADDRESS(0x249f60, 0x194)
TRmgObjectPropertiesRef* type_random_map_generator::selectObjectPrototype(
    int terrain, int objectType, int subtype)
{
    std::vector<TRmgObjectPropertiesRef*> candidates;
    for (u32 index = 0; index < m_objectPrototypes[objectType].size(); ++index) {
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

// Picks a random treasure in the value range. The chosen definition's
// value is queried again before generation.
VA(0x00546190, 0x385)
MAC_ADDRESS(0x24a190, 0x3f4)
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
    for (u32 index = 0; index < m_objectGenerators.size(); ++index) {
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
            for (u32 x = 0; x < prototype->getWidth(); ++x) {
                for (u32 y = 0; y < prototype->getHeight(); ++y) {
                    if (isRmgObjectFootprintCell(prototype, x, y))
                        ++occupied;
                }
            }
            int valuePerCell = objectValue / occupied;
            if (valuePerCell < 3 * bestValuePerCell / 4)
                continue;
            if (bestValuePerCell < 3 * valuePerCell / 4) {
                totalWeight = 0;
                candidates.clear();
                properties.clear();
                bestValuePerCell = valuePerCell;
            }
        }
        totalWeight += definition->m_density;
        candidates.push_back(definition);
        properties.push_back(candidate);
    }
    if (!candidates.size())
        return 0;
    int remainingWeight = rand() % totalWeight;
    u32 selectedIndex;
    for (selectedIndex = 0; selectedIndex < candidates.size(); ++selectedIndex) {
        remainingWeight -= candidates[selectedIndex]->m_density;
        if (remainingWeight < 0)
            break;
    }
    type_treasure_def* definition = candidates[selectedIndex];
    *value = definition->getValue(zone, this);
    return definition->generate(properties[selectedIndex], this, zone);
}

// Treasure groups are assembled on a square scratch map before placement.
enum ERmgTreasureGroupMapSize {
    RMG_TREASURE_GROUP_MAP_SIZE = 16
};

// Adds an object centred on the group map (unsigned division).
static inline void addRmgCenteredGroupObject(TRmgTreasureGroup* group, type_object* object)
{
    const TObjectType* prototype = object->m_properties->m_prototype;
    TRmgMapPosition position;
    position.m_x = (group->m_map.getWidth() + static_cast<u32>(prototype->getWidth())) / 2;
    position.m_y = (group->m_map.getHeight() + static_cast<u32>(prototype->getHeight())) / 2;
    position.m_z = 0;
    group->addObject(object, position);
}

static inline type_object* createRmgTreasureWithRetries(
    type_random_map_generator* generator, TRmgZone* zone, int minimum, int maximum,
    int* value, b8 primary, b8 compact)
{
    for (int attempt = 0; attempt < RMG_TREASURE_ATTEMPTS; ++attempt) {
        TRmgMapPosition unspecified(-1, -1, -1);
        type_object* object = generator->createTreasureObject(zone, minimum, maximum,
            value, primary, true, compact, unspecified);
        if (object)
            return object;
    }
    return 0;
}

// Generation and fit have independent three-attempt limits; failed fits
// release reservations before deletion.
VA(0x00546520, 0x1B6)
MAC_ADDRESS(0x24a584, 0x22c)
int type_random_map_generator::fillTreasureGroup(TRmgZone* zone,
    TRmgTreasureGroup* group, b8 compact, int targetValue)
{
    int objectValue = 0;
    int attempts;
    type_object* firstObject = createRmgTreasureWithRetries(
        this, zone, targetValue / 4, targetValue, &objectValue, true, compact);
    if (!firstObject)
        return 0;
    addRmgCenteredGroupObject(group, firstObject);
    int total = objectValue;
    while (total < targetValue) {
        int remainder = targetValue - total;
        if (remainder < RMG_TREASURE_MINIMUM_REMAINDER && remainder < total / 2)
            break;
        type_object* nextObject;
        for (attempts = 0; ; ) {
            nextObject = createRmgTreasureWithRetries(this, zone,
                remainder / 4, 5 * remainder / 4, &objectValue, false, compact);
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

// Discards a failed group: releases reservations, deletes its objects and
// resets it.
static inline void discardRmgTreasureGroup(TRmgTreasureGroup* group)
{
    for (u32 index = 0; index < group->m_objects.size(); ++index) {
        group->m_objects[index]->releaseReservation();
        delete group->m_objects[index];
    }
    group->reset();
}

// Without a suitable guard creature the group stays unguarded; a failed
// guard fit destroys the group's objects and the unaccepted guard.
VA(0x005466E0, 0x253)
MAC_ADDRESS(0x24a7b0, 0x110) // MAC_ABSTRACTION_FROM(tokens1:4deef3890efa,29.6117): discardRmgTreasureGroup shares ordered reservation release, deletion and reset across four failed-placement paths.
b8 type_random_map_generator::assembleTreasureGroup(TRmgZone* zone,
    TRmgTreasureGroup* group, b8 compact, int minimum, int maximum)
{
    group->reset();
    int targetValue = maximum <= minimum ? maximum : rand() % (maximum - minimum) + minimum;
    int totalValue = fillTreasureGroup(zone, group, compact, targetValue);
    if (!totalValue)
        return false;
    int guardValue = getRmgZoneGuardValue(totalValue, zone, *this);
    if (guardValue > 0) {
        type_object* guard = createGuard(guardValue, zone);
        if (guard && !group->addGuard(guard)) {
            discardRmgTreasureGroup(group);
            delete guard;
            return false;
        }
    }
    group->traceOutline();
    group->markPlacementOutline();
    return true;
}

VA(0x00546940, 0x49)
void TRmgMapItem::setTerrain(int terrain, int frame,
    b8 flipX, b8 flipY)
{
    m_tile.m_landType = terrain;
    m_tile.m_terrainFrame = frame;
    m_tileData.m_terrainFlipX = flipX;
    m_tileData.m_terrainFlipY = flipY;
}

VA(0x00546990, 0x1E)
TRmgMapItem* type_random_map::getMapItem(int x, int y)
{
    return &m_mapItems[y * m_mapWidth + x];
}

VA_COMPGEN(0x00404200, 0x209, VECTOR_INSERT, Int)

VA_COMPGEN(0x00422F50, 0x1B1, VECTOR_INSERT_SINGLE, Int)

VA_COMPGEN(0x004347A0, 0x32E, VECTOR_INSERT, TRmgMapPosition)

VA_COMPGEN(0x0054C3F0, 0x21C, VECTOR_INSERT_SINGLE, TRmgMapPosition)

VA_COMPGEN(0x0054DD60, 0x15, STD_CONSTRUCT, TRmgMapPosition)

VA_COMPGEN(0x0054C730, 0x1DD, VECTOR_INSERT_SINGLE, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054C940, 0x23, VECTOR_DESTROY, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054D8B0, 0x38, VECTOR_UCOPY, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054D8F0, 0x29, VECTOR_UFILL, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054DD80, 0x104, STD_CONSTRUCT, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054DA20, 0x19F, STD_FILL, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054DBC0, 0x1A0, STD_COPY_BACKWARD, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054C970, 0x22F, VECTOR_INSERT_SINGLE, TRmgZoneConnection)

VA_COMPGEN(0x005157D0, 0x1B, CLASS_CTOR, vector)

VA_COMPGEN(0x00536BA0, 0x18, DEFAULT_CTOR_CLOSURE, vector)

VA_COMPGEN(0x00536B60, 0x3D, IMPLICIT_DTOR, TRmgObjectPlacementRule)
MAC_COMPGEN_ADDRESS(0x235010, 0x84, IMPLICIT_DTOR, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054C170, 0x38, VECTOR_DTOR, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054C1B0, 0x23, VECTOR_SIZE, TRmgZoneConnection)

VA_COMPGEN(0x0054CD70, 0x3D, VECTOR_ERASE, TPoint)

VA_COMPGEN(0x0054C610, 0x53, VECTOR_ERASE, TRmgMapPosition)

VA_COMPGEN(0x0054CDB0, 0x33, VECTOR_ERASE, Int)

VA_COMPGEN(0x0054CDF0, 0x1D3, VECTOR_INSERT, unsigned_char)

VA_COMPGEN(0x0054CFD0, 0x2F, VECTOR_ERASE, unsigned_char)

VA_COMPGEN(0x0054D9E0, 0x39, STD_COPY, TRmgMapPosition)

VA_COMPGEN(0x005093c0, 0x25, STD_COPY, Int)

VA_COMPGEN(0x0054df40, 0x25, STD_COPY, const_int)

// Group objects store local XY positions; translate them and replace Z with
// the destination level.
static inline TRmgMapPosition getRmgPlacedGroupObjectPosition(
    type_object* object, const TRmgMapPosition& groupPosition)
{
    TRmgMapPosition position = object->getPosition();
    position.m_x += groupPosition.m_x;
    position.m_y += groupPosition.m_y;
    position.m_z = groupPosition.m_z;
    return position;
}

VA(0x005469B0, 0x2B4)
MAC_ADDRESS(0x24a8c0, 0x44c)
void type_random_map_generator::commitTreasureGroup(TRmgTreasureGroup* group,
    TRmgMapPosition position)
{
    group->m_position = position;
    for (u32 i = 0; i < group->m_objects.size(); ++i) {
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
            // Copy the destination's earlier marks back to the group map.
            if (border)
                source->markBorderObject();
            else
                source->clearBorderObject();
            if (pathClearance)
                source->openPath();
            else
                source->releasePathClearance();
        }
    }
    for (u32 objectIndex = 0; objectIndex < group->m_objects.size(); ++objectIndex)
        group->m_objects[objectIndex]->completePlacement();
}

VA(0x00546C70, 0x452)
MAC_ADDRESS(0x24ad0c, 0x6cc)
b8 type_random_map_generator::canPlaceTreasureGroup(TRmgTreasureGroup* group,
    TRmgMapPosition position, TRmgZone* zone)
{
    TRmgMapPosition workingPosition;
    TRmgZoneBounds bounds = group->m_bounds;
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    for (u32 i = 0; i < group->m_objects.size(); ++i) {
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
                if (item->isRoadEntrance() && item->getEntranceObjectType() == MONSTER)
                    return false;
            }
        }
    }
    int firstDirection = RMG_DIRECTION_EAST;
    int lastDirection = RMG_DIRECTION_COUNT;
    b8 waterZone = zone->m_terrain == eTerrainWater;
    type_object* lastObject = group->m_objects.back();
    TObjectType* prototype = lastObject->m_properties->m_prototype;
    TRmgMapPosition entrance = getRmgPlacedObjectEntrance(lastObject);
    if (!isRmgEntranceOpenToNorth(prototype->getObjectType())) {
        firstDirection = RMG_DIRECTION_SOUTH_EAST;
        lastDirection = RMG_DIRECTION_SOUTH_WEST + 1;
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
                // Only the upper bounds are checked; callers keep translated
                // coordinates nonnegative.
                if (x < m_map.m_mapWidth && y < m_map.m_mapHeight
                    && m_map.getMapItem(x, y, position.m_z)->isRoadEntrance())
                    return false;
            }
        }
    }
    return true;
}

VA(0x005470D0, 0x286)
MAC_ADDRESS(0x24b3d8, 0x310)
b8 type_random_map_generator::placeTreasureGroup(TRmgTreasureGroup* group,
    TRmgZone* zone, int spacing)
{
    int zoneIndex = zone->m_templateZone->m_zoneIndex;
    std::vector<TRmgMapPosition> candidates;
    TRmgZoneBounds bounds = zone->m_bounds;
    TRmgZoneBounds groupBounds = group->m_bounds;
    bounds.m_minimumY -= groupBounds.m_minimumY;
    TRmgMapPosition position = zone->getLevelPosition();
    bounds.m_maximumX += 1 - groupBounds.m_maximumX;
    bounds.m_maximumY += 1 - groupBounds.m_maximumY;
    bounds.m_minimumX -= groupBounds.m_minimumX;
    TPoint center;
    center.m_x = (groupBounds.m_minimumX + groupBounds.m_maximumX) / 2;
    center.m_y = (groupBounds.m_minimumY + groupBounds.m_maximumY) / 2;
    int bestScore = spacing;
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position + center);
            if (item->m_zoneState.m_zone == zoneIndex
                && item->m_zoneState.m_objectDistance >= bestScore
                && canPlaceTreasureGroup(group, position, zone)) {
                addRmgHighestScoreCandidate(candidates, position,
                    item->m_zoneState.m_objectDistance, bestScore);
            }
        }
    }
    if (candidates.size() == 0)
        return false;
    position = candidates[rand() % candidates.size()];
    commitTreasureGroup(group, position);
    return true;
}

// Up to three attempts to assemble and place a group for one treasure band;
// groups that fail placement are discarded.
static inline b8 tryPlaceRmgTreasureBand(
    type_random_map_generator* generator, TRmgZone* zone, TRmgTreasureGroup* group,
    b8 compact, const TRmgTreasureRange& range, int spacing)
{
    for (int attempt = 0; attempt < RMG_TREASURE_ATTEMPTS; ++attempt) {
        if (generator->assembleTreasureGroup(zone, group, compact,
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
    TRmgTreasureGroup group(RMG_TREASURE_GROUP_MAP_SIZE, RMG_TREASURE_GROUP_MAP_SIZE);
    const int bandCount = sizeof(slot->m_treasure) / sizeof(slot->m_treasure[0]);
    // Bands with a maximum value below 100 or no density are skipped.
    int densities[bandCount];
    for (int band = 0; band < bandCount; ++band) {
        const TRmgTreasureRange& range = slot->m_treasure[band];
        densities[band] = range.m_maximum >= 100 && range.m_density > 0
            ? range.m_density : 0;
    }
    b8 finished[bandCount];
    int totalDensity = 0;
    int densityProduct = 1;
    initializeRmgDensityCategories(densities, bandCount, finished, totalDensity, densityProduct);
    if (totalDensity == 0)
        return;
    int spacing;
    if (zone->m_terrain == eTerrainWater)
        spacing = getRmgDensitySpacing(1600, totalDensity);
    else
        spacing = getRmgDensitySpacing(800, totalDensity);
    // No band has placed a treasure yet.
    int weightedCounts[bandCount] = {0, 0, 0};
    int countSteps[bandCount];
    initializeRmgCategoryStrides(densities, weightedCounts, bandCount,
        densityProduct, countSteps, weightedCounts);
    for (;;) {
        int selected = selectRmgWeightedCategory(finished, weightedCounts, bandCount);
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

static inline void queueRmgMovementStep(TRmgMapItem* destination,
    int cost, const TRmgMapPosition& previous, const TRmgMapPosition& next,
    std::vector<TRmgMapPosition>& positions, std::vector<int>& costs)
{
    destination->setMovementCost(cost, previous);
    insertRmgWorkItem(positions, costs, next, cost);
}

// Road exits and entries share this restricted-approach policy.
static inline bool hasRmgRestrictedRoadApproach(int objectType)
{
    return !isRmgEntranceOpenToNorth(objectType)
        && !g_adventureObjectTraits[objectType].m_trait2;
}

// Dijkstra-style relaxation uses the back of a descending worklist, retaining
// duplicate entries. Monolith/gate transitions precede neighbour relaxation.
VA(0x00547880, 0x7B1)
MAC_ADDRESS(0x24bbe0, 0x7e4)
void type_random_map_generator::buildRoadCostMap(TRmgMapPosition position)
{
    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;

    seedRmgMovementSearch(m_map, position, openPositions, openCosts);

    while (openPositions.size()) {
        popRmgWorkItem(position, openPositions, openCosts);

        TRmgMapItem* mapItem = m_map.getMapItem(position);
        int currentCost = mapItem->m_movement.m_cost;
        b8 currentHasRoad = mapItem->m_tile.m_roadType != 0;
        int direction = RMG_DIRECTION_COUNT;
        if (mapItem->isRoadEntrance()) {
            type_object* object = mapItem->m_objects[0];
            TObjectType* prototype = object->m_properties->m_prototype;
            int objectType = prototype->getObjectType();
            if (hasRmgRestrictedRoadApproach(objectType))
                direction = RMG_FIRST_NORTHERN_DIRECTION;

            switch (objectType) {
            case LITH_ONEWAY_ENTRANCE:
            case LITH_ONEWAY_EXIT:
            case LITH_TWOWAY: {
                // Monoliths of the same subtype connect, in stored order.
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
                // The gate's twin stands at the same cell on the other level.
                TRmgMapPosition nextPosition(
                    position.m_x, position.m_y, getRmgOtherLevel(position.m_z));
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

            if (nextMapItem->isRoadEntrance()) {
                int objectType = nextMapItem->getEntranceObjectType();
                const TAdvObjectTraits& traits = g_adventureObjectTraits[objectType];
                if (traits.m_blocksLanding && !traits.m_trait2)
                    continue;
                if (hasRmgRestrictedRoadApproach(objectType)
                    && isRmgSouthwardDirection(direction))
                    continue;
            }

            int nextCost = currentHasRoad && nextMapItem->m_tile.m_roadType ? 2 : 20;
            if (isRmgDiagonalDirection(direction))
                nextCost *= 3;
            nextCost += currentCost;

            if (nextMapItem->m_movement.m_cost <= nextCost)
                continue;

            queueRmgMovementStep(nextMapItem, nextCost, position, nextPosition,
                openPositions, openCosts);
        }
    }
}

// Paint same-level cardinal runs, restarting at diagonal or level transitions.
VA(0x00548040, 0x244)
MAC_ADDRESS(0x24c3c4, 0x2bc)
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
                painter.drawTo(TRmgGridPoint(position.m_x, position.m_y));
                previous = position;
            }
        }
    }
}

MAC_ADDRESS(0x24bb68, 0x78)
void type_random_map_generator::resetMovementCosts()
{
    TRmgMapItem* mapItem = m_map.getMapItem(0, 0);
    int rowCount = m_map.m_numberLevels * m_map.m_mapHeight;
    int mapItemCount = rowCount * m_map.m_mapWidth;
    while (mapItemCount--) {
        mapItem->resetMovement();
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
    // Retail bug: an empty target list underflows size() - 1.
    for (u32 first = 0; first < m_roadTargets.size() - 1; ++first) {
        TRmgMapPosition source = m_roadTargets[first];
        rebuildRmgRoadCostMap(this, source);
        for (u32 second = first + 1; second < m_roadTargets.size(); ++second) {
            TRmgMapPosition destination = m_roadTargets[second];
            if (m_map.getMapItem(destination)->m_movement.m_cost <= RMG_REACHED_COST_LIMIT
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

// River step cost: a random 1-32, plus 30 on roads. The draw happens even
// when the step is later rejected.
static inline int getRmgRiverStepCost(int currentCost, const TRmgMapItem* destination)
{
    int nextCost = currentCost + (rand() & 31) + 1;
    if (destination->m_tile.m_roadType)
        nextCost += 30;
    return nextCost;
}

// River-layer line types the generator paints; zero is no river.
enum ERmgRiverType {
    RMG_RIVER_CLEAR = 1,
    RMG_RIVER_ICY = 2
};

// The source terrain determines both river graphics and the snow boundary
// restriction.
static inline void selectRmgRiverAppearance(const TRmgMapItem* source,
    b8& sourceIsSnow, int& riverType)
{
    if (source->getLandType() == eTerrainSnow) {
        sourceIsSnow = true;
        riverType = RMG_RIVER_ICY;
    } else {
        sourceIsSnow = false;
        riverType = RMG_RIVER_CLEAR;
    }
}

// Rivers stay on dry, non-rock terrain and cannot cross the snow boundary.
// Both river searches share this terrain rule; only createRiver also rejects
// cells beside rivers and blocked approach directions.
static inline bool isRmgRiverTerrain(const TRmgMapItem* item, b8 sourceIsSnow)
{
    return item->getLandType() != eTerrainWater
        && item->getLandType() != eTerrainRock
        && (item->getLandType() == eTerrainSnow) == sourceIsSnow;
}

// Paints a river from source to the first m_hasRiver cell the search reaches.
// Random edge costs are drawn for every river-terrain neighbour, even without
// an improvement. Unlike createRiver, this search ignores near-river and
// blocked-direction flags.
VA(0x00548500, 0x533)
MAC_ADDRESS(0x24c8ac, 0x588)
void type_random_map_generator::createRiverToObject(TRmgMapPosition source)
{
    resetMovementCosts();
    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;
    TRmgMapItem* mapItem = seedRmgMovementSearch(m_map, source,
        openPositions, openCosts);
    b8 sourceIsSnow;
    int riverType;
    selectRmgRiverAppearance(mapItem, sourceIsSnow, riverType);
    TRmgMapPosition position;
    TRmgMapPosition nextPosition;
    while (!openPositions.empty()) {
        popRmgWorkItem(position, openPositions, openCosts);
        mapItem = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
        int positionCost = mapItem->m_movement.m_cost;
        for (int direction = 0; direction < RMG_DIRECTION_COUNT; direction += 2) {
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
    // As in createRiver, this tests the last inspected tile rather than an
    // explicit search result.
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

// Blocked-direction bits are indexed by cardinal (direction / 2); this is
// the bit of the cardinal opposite an eight-way direction.
static inline int getRmgOppositeCardinalBit(int direction)
{
    return 1 << (getRmgOppositeDirection(direction) / 2);
}

// Retail quirk: this scan admits x == width.
static inline bool isOutsideRmgRiverCoastScan(
    const TRmgMapPosition& point, const type_random_map& map)
{
    return point.m_x < 0 || point.m_x > map.m_mapWidth
        || point.m_y < 0 || point.m_y >= map.m_mapHeight;
}

// The dry strip and inland approach exclude water and entrances; rock is
// allowed.
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
// cells starting with the middle dry one. The last becomes a river target
// whose blocked bit faces the coast: rivers cannot arrive from there, and
// createRiver puts its delta on that side. For direction East, North up:
// P and ~ water, d the dry strip, i inland, T the target.
//   ~ d
//   P d i i T
//   ~ d
VA(0x00548A40, 0x222)
MAC_ADDRESS(0x24ce88, 0x404)
void type_random_map_generator::markRiverCoastTarget(TRmgMapPosition position, int direction)
{
    TRmgMapPosition point = position + g_rmgDirections[turnRmgDirection(direction, 2)];
    TPoint step = g_rmgDirections[turnRmgDirection(direction, -2)];
    for (int waterCount = 0; waterCount < 3; ++waterCount) {
        if (isOutsideRmgRiverCoastScan(point, m_map))
            return;
        if (m_map.getMapItem(point.m_x, point.m_y, point.m_z)->getLandType() != eTerrainWater)
            return;
        point += step;
    }
    point = position + g_rmgDirections[turnRmgDirection(direction, 1)];
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
    item->m_tileData.m_blockedDirections |= getRmgOppositeCardinalBit(direction);
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
                    for (int direction = 0; direction < RMG_DIRECTION_COUNT; direction += 2)
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
// as createRiverToObject, seeded at three cells beside the water wheel.
VA(0x00548DF0, 0x99F)
MAC_ADDRESS(0x24d4f0, 0xb00)
void type_random_map_generator::createRiver(TRmgMapPosition source)
{
    resetMovementCosts();

    TRmgMapItem* mapItem;

    std::vector<TRmgMapPosition> openPositions;
    std::vector<int> openCosts;

    mapItem = seedRmgMovementSearch(m_map, source, openPositions, openCosts);

    b8 sourceIsSnow;
    int riverType;
    selectRmgRiverAppearance(mapItem, sourceIsSnow, riverType);

    --source.m_y;
    seedRmgMovementSearch(m_map, source, openPositions, openCosts);

    ++source.m_x;
    seedRmgMovementSearch(m_map, source, openPositions, openCosts);

    TRmgMapPosition position;
    TRmgMapPosition nextPosition;
    int direction;

    while (!openPositions.empty()) {
        popRmgWorkItem(position, openPositions, openCosts);

        mapItem = m_map.getMapItem(position);
        int positionCost = mapItem->m_movement.m_cost;
        for (direction = 0; direction < RMG_DIRECTION_COUNT; direction += 2) {
            nextPosition = position + g_rmgDirections[direction];

            if (!m_map.containsXY(nextPosition))
                continue;

            mapItem = m_map.getMapItem(nextPosition);
            if (!isRmgRiverTerrain(mapItem, sourceIsSnow) || mapItem->isNearRiver())
                continue;

            int nextCost = getRmgRiverStepCost(positionCost, mapItem);

            if (nextCost >= mapItem->m_movement.m_cost)
                continue;

            if (mapItem->m_tileData.m_blockedDirections
                & getRmgOppositeCardinalBit(direction))
                continue;

            queueRmgMovementStep(mapItem, nextCost, position, nextPosition,
                openPositions, openCosts);

            if (mapItem->isRiverTarget()) {
                openPositions.clear();
                break;
            }
        }
    }

    // Retail bug: this tests the last inspected tile, even if relaxation
    // rejected it; an unreached target can start an invalid predecessor walk
    // (see docs/reference/rmg-undefined-behavior.md).
    if (!mapItem->isRiverTarget())
        return;

    position = nextPosition;

    type_random_map levelMap(m_map.getMapItem(0, 0, nextPosition.m_z),
        m_map.m_mapWidth, m_map.m_mapHeight);
    TRmgRiverMapAdapter mapAdapter(&levelMap);
    TRmgRiverPainter riverPainter(
        &mapAdapter, riverType, TRmgGridPoint(nextPosition.m_x, nextPosition.m_y));

    if (mapItem->m_tileData.m_blockedDirections) {
        int cardinal;
        for (cardinal = 0; cardinal < 4; ++cardinal) {
            if (mapItem->m_tileData.m_blockedDirections & (1 << cardinal))
                break;
        }

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
            ? g_snowRiverDeltaIndex[cardinal]
            : g_landRiverDeltaIndex[cardinal];
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
                nextPosition.m_x + deltaOffsets[cardinal].m_x,
                nextPosition.m_y + deltaOffsets[cardinal].m_y,
                nextPosition.m_z));

        TRmgMapPosition mouth = nextPosition + g_rmgDirections[cardinal * 2];
        riverPainter.drawTo(TRmgGridPoint(mouth.m_x, mouth.m_y));
        mapItem = m_map.getMapItem(mouth);
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
    for (u32 index = 0; index < m_objects.size(); ++index) {
        type_object* object = m_objects[index];
        TObjectType* prototype = object->m_properties->m_prototype;
        if (prototype->getObjectType() == TERRAIN_MOUNTAIN
            || prototype->getObjectType() == TERRAIN_LAKE
            || (prototype->getObjectType() == MINE && prototype->getSubtype() == GEMS)) {
            // Objects without an entrance use their footprint centre.
            TRmgMapPosition position;
            if (prototype->m_hasTrigger) {
                position = getRmgPlacedObjectEntrance(object);
            } else {
                position = object->m_position;
                position -= TPoint(static_cast<u32>(prototype->getWidth()) / 2,
                    static_cast<u32>(prototype->getHeight()) / 2);
            }
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
    for (u32 index = 0; index < m_objects.size(); ++index) {
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

// Assigns players to template slots, lays out zones and terrain, places
// towns, connections, mines and treasures, then decorates and adds roads
// and rivers.
VA(0x00549930, 0x37B)
MAC_ADDRESS(0x24e294, 0x41c)
b8 type_random_map_generator::generate()
{
    if (!m_templates.size())
        return false;
    u32 selected = rand() % m_templates.size();
    m_templateName = m_templates[selected]->m_name;
    char humanSlots[RMG_PLAYER_COUNT];
    int humanSlotByte;
    MEMSET(humanSlots, 0, sizeof(humanSlots), humanSlotByte);
    char allSlots[RMG_PLAYER_COUNT];
    int allSlotByte;
    MEMSET(allSlots, 0, sizeof(allSlots), allSlotByte);
    for (u32 zone = 0; zone < m_templates[selected]->m_zones.size(); ++zone) {
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
    int playerOrder[RMG_PLAYER_COUNT];
    int orderedCount = 0;
    for (int player = 0; player < RMG_PLAYER_COUNT; ++player)
        if (m_fixedHumanPlayers[player])
            playerOrder[orderedCount++] = player;
    for (player = 0; player < RMG_PLAYER_COUNT; ++player)
        if (!m_fixedHumanPlayers[player])
            playerOrder[orderedCount++] = player;
    int slot = 0;
    int orderIndex;
    for (orderIndex = 0; orderIndex < m_humanPlayerCount; ++orderIndex) {
        while (slot < RMG_PLAYER_COUNT && !humanSlots[slot])
            ++slot;
        allSlots[slot] = 0;
        m_playerIndexMap[++slot] = playerOrder[orderIndex];
    }
    slot = 0;
    for (; orderIndex < m_humanPlayerCount + m_computerPlayerCount; ++orderIndex) {
        while (slot < RMG_PLAYER_COUNT && !allSlots[slot])
            ++slot;
        m_playerIndexMap[++slot] = playerOrder[orderIndex];
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

template <size_t N>
void encodePackedBits(const std::bitset<N>& bits, u8* packed)
{
    memset(packed, 0, (N + 7) / 8);
    for (u32 index = 0; index < N; ++index) {
        if (bits.test(index))
            packed[index >> 3] |= 1 << (index & 7);
    }
}

template <size_t N>
int writePackedBits(TAbstractFile* outfile, const std::bitset<N>& bits)
{
    u8 packed[(N + 7) / 8];
    encodePackedBits(bits, packed);
    return outfile->write(packed, sizeof(packed));
}

// Length-prefixed text: an int length followed by the characters.
int writeString(TAbstractFile* outfile, const std::string& text)
{
    writeValue<s32>(outfile, text.length());
    return outfile->write(text.c_str(), text.length());
}

int writeString(TAbstractFile* outfile, const char* text)
{
    writeValue<s32>(outfile, strlen(text));
    return outfile->write(text, strlen(text));
}

// Hero ids a map format knows: RoE maps stop before the expansion heroes.
enum ERmgHeroCount {
    RMG_ROE_HERO_COUNT = 128,
    RMG_HERO_COUNT = 156
};

// Artifact ids an AB map knows: it ends before the SoD combination artifacts.
enum ERmgArtifactCount {
    RMG_AB_ARTIFACT_COUNT = ARTIFACT_ANGELIC_ALLIANCE
};

// Each player clause in the map description appends a separator, the player
// colour and the clause text, using unchecked strcat.
static inline void appendRmgPlayerDescription(char* description, int player,
    const char* clause)
{
    strcat(description, DATA_COMPGEN(0x0066032C, rmgListSeparator, ", "));
    strcat(description, g_rmgPlayerNames[player]);
    strcat(description, clause);
}

VA(0x00549CB0, 0xE90)
MAC_ADDRESS(0x24e7d4, 0x11ac)
void type_random_map_generator::writeMapHeader(TAbstractFile* outfile)
{
    writeValue<s32>(outfile, getSerializedMapVersion());

    writeValue<char>(outfile, 1);

    writeValue<s32>(outfile, m_map.getWidth());

    writeValue<char>(outfile, m_map.m_numberLevels > 1);

    std::string mapName(
        DATA_COMPGEN(0x00682900, rmgMapName, "Random Map"));
    writeString(outfile, mapName);

    // Retail uses unchecked sprintf/strcat below; long template names or
    // player descriptions can overflow this fixed buffer.
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

    for (int descriptionPlayer = 0; descriptionPlayer < RMG_PLAYER_COUNT;
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
                // Retail bug: indexed by the player rather than the chosen
                // town, so the named town can be wrong.
                g_rmgTownNames[descriptionPlayer]);
        }
    }

    writeString(outfile, description);

    writeValue<char>(outfile, 1);
    if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<char>(outfile, 0);
    }

    b8 canBeHuman[RMG_PLAYER_COUNT];
    int legalAlignments[RMG_PLAYER_COUNT];
    TRmgMapPosition mainTowns[RMG_PLAYER_COUNT];
    b8 canBeComputer[RMG_PLAYER_COUNT];
    memset(canBeHuman, 0, sizeof(canBeHuman));
    memset(legalAlignments, 0, sizeof(legalAlignments));
    memset(mainTowns, 0, sizeof(mainTowns));
    memset(canBeComputer, 0, sizeof(canBeComputer));
    int generatedHumanTowns = 0;
    for (u32 zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgTemplateZone* slot = zone->m_templateZone;
        int slotPlayer = slot->m_playerIndex;
        if (slotPlayer < 0)
            continue;

        int player = m_playerIndexMap[slotPlayer + 1];
        if (player < 0 || !zone->m_active)
            continue;

        if (slot->m_kind == RMG_TEMPLATE_HUMAN && !canBeHuman[player]) {
            ++generatedHumanTowns;
            canBeHuman[player] = true;
            mainTowns[player] = zone->m_position;
        }

        if (slot->m_kind == RMG_TEMPLATE_COMPUTER && !canBeComputer[player]) {
            canBeComputer[player] = true;
            mainTowns[player] = zone->m_position;
        }

        legalAlignments[player] |= 1 << zone->m_alignment;
    }

    int surplusHumanTowns = generatedHumanTowns - m_humanPlayerCount;
    int reversePlayer = RMG_PLAYER_COUNT - 1;
    do {
        if (canBeHuman[reversePlayer]
            && !m_fixedHumanPlayers[reversePlayer]
            && surplusHumanTowns > 0) {
            canBeComputer[reversePlayer] = true;
            canBeHuman[reversePlayer] = false;
            --surplusHumanTowns;
        }
    } while (reversePlayer-- != 0);

    m_computerPlayerCount = m_humanPlayerCount = 0;

    for (int serializedPlayer = 0; serializedPlayer < RMG_PLAYER_COUNT;
         ++serializedPlayer) {
        writeValue<char>(outfile, canBeHuman[serializedPlayer]);

        writeValue<char>(outfile, canBeHuman[serializedPlayer] || canBeComputer[serializedPlayer]);

        writeValue<char>(outfile, 0);

        if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
            writeValue<char>(outfile, 0);
        }

        if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
            writeValue<u16>(outfile, legalAlignments[serializedPlayer]);
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
            writeValue<s32>(outfile, 0);
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
        char teams[RMG_PLAYER_COUNT];
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
        std::bitset<RMG_HERO_COUNT> availableHeroes;
        setAvailableRmgHeroes(
            &availableHeroes, m_disabledHeroes, m_disabledHeroes + RMG_HERO_COUNT);

        writePackedBits(outfile, availableHeroes);
    } else {
        std::bitset<RMG_ROE_HERO_COUNT> availableHeroes;
        setAvailableRmgHeroes(
            &availableHeroes, m_disabledHeroes, m_disabledHeroes + RMG_ROE_HERO_COUNT);

        writePackedBits(outfile, availableHeroes);
    }

    if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<s32>(outfile, 0);
    }
    if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
        writeValue<char>(outfile, 0);
    }

    writeRmgReservedBytes<31>(outfile);

    // Combination artifacts, artifact 127 and Armageddon's Blade are disabled.
    std::bitset<ARTIFACT_COUNT> disabledArtifacts;
    for (int artifactIndex = 0; artifactIndex < ARTIFACT_COUNT; ++artifactIndex) {
        disabledArtifacts[artifactIndex] =
            g_artifactTraits[artifactIndex].m_comboType != -1;
    }
    disabledArtifacts.set(ARTIFACT_ARMAGEDDONS_BLADE);
    disabledArtifacts.set(127);

    if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
        writePackedBits(outfile, disabledArtifacts);
    } else if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
        std::bitset<RMG_AB_ARTIFACT_COUNT> legacyDisabledArtifacts;
        std::copy(
            bitset_iterator<ARTIFACT_COUNT>(disabledArtifacts, 0),
            bitset_iterator<ARTIFACT_COUNT>(disabledArtifacts, RMG_AB_ARTIFACT_COUNT),
            bitset_iterator<RMG_AB_ARTIFACT_COUNT>(legacyDisabledArtifacts, 0));

        writePackedBits(outfile, legacyDisabledArtifacts);
    }

    if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
        std::bitset<hero::NUM_SPELLS> disabledSpells;
        writePackedBits(outfile, disabledSpells);

        std::bitset<kNumSecSkills> disabledSkills;
        writePackedBits(outfile, disabledSkills);

        for (int hero = 0; hero < RMG_HERO_COUNT; ++hero) {
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
    int playersPerTeam[RMG_PLAYER_COUNT];
    int team;

    for (team = 0; team < teamCount; ++team) {
        playersPerTeam[team] =
            playerCount / teamCount + (playerCount % teamCount > team);
    }

    for (int player = 0; player < RMG_PLAYER_COUNT; ++player) {
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
// referenced prototypes start at 2. Objects are written in two trait-ordered
// passes.
VA(0x0054ABF0, 0x235)
MAC_ADDRESS(0x24fd18, 0x350)
b8 type_random_map_generator::writeMap(TAbstractFile* outfile)
{
    TRmgMapPosition position;
    writeMapHeader(outfile);
    writeValue<s32>(outfile, 0);
    TRmgMapItem* item = m_map.m_mapItems;
    for (position.m_z = 0; position.m_z < m_map.m_numberLevels; ++position.m_z)
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y)
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x) {
                item->write(outfile);
                ++item;
            }
    int prototypeCount = 2;
    for (int type = 0; type < ADVENTURE_OBJECT_TRAIT_COUNT; ++type)
        for (u32 index = 0; index < m_objectPrototypes[type].size(); ++index) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[type][index];
            if (static_cast<int>(properties->m_refCount) > 0)
                properties->m_prototypeIndex = prototypeCount++;
        }
    writeValue<s32>(outfile, prototypeCount);
    writeRmgObjectPrototype(outfile, m_objectPrototypes[RANDOM_MONSTER][0]->m_prototype);
    writeRmgObjectPrototype(outfile, m_objectPrototypes[TERRAIN_HOLE][0]->m_prototype);
    for (int objectType = 0; objectType < ADVENTURE_OBJECT_TRAIT_COUNT; ++objectType)
        for (u32 index = 0; index < m_objectPrototypes[objectType].size(); ++index) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[objectType][index];
            if (static_cast<int>(properties->m_refCount) > 0)
                writeRmgObjectPrototype(outfile, properties->m_prototype);
        }
    writeValue<s32>(outfile, m_objects.size());
    for (u32 first = 0; first < m_objects.size(); ++first) {
        type_object* object = m_objects[first];
        if (g_adventureObjectTraits[object->m_properties->m_prototype->getObjectType()].m_trait3)
            object->write(outfile, m_mapVersion);
    }
    for (u32 second = 0; second < m_objects.size(); ++second) {
        type_object* object = m_objects[second];
        if (!g_adventureObjectTraits[object->m_properties->m_prototype->getObjectType()].m_trait3)
            object->write(outfile, m_mapVersion);
    }
    if (m_progress)
        m_progress->advance(2000);
    b8 result = writeValue<s32>(outfile, 0) == sizeof(s32);
    return result;
}

enum TRmgPrototypeCellMask {
    RMG_PROTOTYPE_PASSABLE_CELLS,
    RMG_PROTOTYPE_TRIGGER_CELLS
};

// H3M stores each fixed 8x6 footprint in reverse row/column order, packed
// least-significant bit first.
static inline void writeRmgPrototypeCellMask(TAbstractFile* outfile,
    TObjectType* prototype, TRmgPrototypeCellMask kind)
{
    u8 mask[6];
    memset(mask, 0, sizeof(mask));
    int bit = 0;
    for (int y = 6; y--;)
        for (int x = 7; x >= 0; --x) {
            if (kind == RMG_PROTOTYPE_PASSABLE_CELLS
                ? prototype->isPassableCell(x, y) : prototype->isTriggerCell(x, y))
                mask[bit / 8] |= 1 << (bit % 8);
            ++bit;
        }
    outfile->write(mask, sizeof(mask));
}

VA(0x0054AE30, 0x2C5)
MAC_ADDRESS(0x24f980, 0x398)
void __fastcall writeRmgObjectPrototype(TAbstractFile* outfile, TObjectType* prototype)
{
    writeString(outfile, prototype->getImageName());
    writeRmgPrototypeCellMask(outfile, prototype, RMG_PROTOTYPE_PASSABLE_CELLS);
    writeRmgPrototypeCellMask(outfile, prototype, RMG_PROTOTYPE_TRIGGER_CELLS);
    writePackedBits(outfile, prototype->m_terrainMask);
    writePackedBits(outfile, prototype->m_recommendedTerrainMask);
    writeValue<s32>(outfile, prototype->getObjectType());
    writeValue<s32>(outfile, prototype->getSubtype());
    writeValue<char>(outfile, prototype->m_slotCategory);
    writeValue<char>(outfile, prototype->isUnderlay());
    int reserved[4];
    memset(reserved, 0, sizeof(reserved));
    outfile->write(reserved, sizeof(reserved));
}

// Prisons draw from the RoE heroes, or in later formats from the first 145.
static inline int getRmgPrisonHeroCount(int mapVersion)
{
    return mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE ? 145 : RMG_ROE_HERO_COUNT;
}

VA(0x0054B100, 0x71)
MAC_ADDRESS(0x250068, 0xd4)
int type_random_map_generator::selectPrisonHero()
{
    int available = 0;
    int hero;
    for (hero = getRmgPrisonHeroCount(m_mapVersion) - 1; hero >= 0; --hero) {
        if (!m_disabledHeroes[hero])
            ++available;
    }
    if (!available)
        return -1;

    int selected = rand() % available;
    for (hero = getRmgPrisonHeroCount(m_mapVersion) - 1; hero >= 0; --hero) {
        if (!m_disabledHeroes[hero]) {
            --selected;
            if (selected < 0)
                break;
        }
    }
    m_disabledHeroes[hero] = true;
    return hero;
}

// Priority worklist ordered by each zone's quest placement score.
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

// Stores each zone's connection-graph distance from origin in its quest
// placement score (20000 when unreachable).
VA(0x0054B180, 0x174)
MAC_ADDRESS(0x25013c, 0x210)
void type_random_map_generator::calculateQuestZoneDistances(TRmgZone* origin)
{
    std::vector<TRmgZone*> pending;
    for (u32 index = 0; index < m_zones.size(); ++index)
        m_zones[index]->m_questPlacementScore = 20000;
    origin->m_questPlacementScore = 0;
    pending.push_back(origin);
    while (pending.size()) {
        TRmgZone* current = pending.back();
        pending.pop_back();
        TRmgTemplateZone* slot = current->m_templateZone;
        int distance = current->m_questPlacementScore + 1;
        for (u32 connection = 0; connection < slot->m_connections.size(); ++connection) {
            TRmgZone* next = m_zones[slot->m_connections[connection].m_destination->m_zoneIndex];
            if (next->m_questPlacementScore > distance) {
                next->m_questPlacementScore = distance;
                insertRmgWorkItem(pending, next);
            }
        }
    }
}

// Tries reachable non-junction land zones nearest first, but adjacent zones
// last; ties keep zone order. Excluded zones still draw a random score.
VA(0x0054B300, 0x18B)
MAC_ADDRESS(0x25034c, 0x23c)
b8 type_random_map_generator::placeQuestGroup(
    TRmgTreasureGroup* group, TRmgZone* origin)
{
    std::vector<TRmgZone*> candidates;
    TRmgZone* candidateZone;
    calculateQuestZoneDistances(origin);
    for (u32 index = 0; index < m_zones.size(); ++index) {
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
        u32 insertion = 0;
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

// Treasure artifact class ('T') bit in the artifact table.
static const int g_rmgQuestArtifactClass = 2;

// Enabled treasure-class artifacts not yet used by a seer hut.
static inline bool isRmgQuestArtifact(int artifact, const b8* usedArtifacts)
{
    return !g_artifactTraits[artifact].m_disabled && !usedArtifacts[artifact]
        && (g_artifactTraits[artifact].m_artifactClass & g_rmgQuestArtifactClass);
}

VA(0x0054B490, 0x42E)
MAC_ADDRESS(0x250588, 0x360)
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
    u32 prototypeIndex = findRmgPrototypeSubtypeIndex<u32>(
        m_objectPrototypes[ARTIFACT], artifact);
    // Assumes an eligible artifact always has a loaded prototype.
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[ARTIFACT][prototypeIndex];
    --object->m_properties->m_refCount;
    object->m_properties = properties;
    ++properties->m_refCount;
    TRmgZone* origin = m_zones[m_map.getMapItem(object->m_position)->m_zoneState.m_zone];
    TRmgTreasureGroup group(RMG_TREASURE_GROUP_MAP_SIZE, RMG_TREASURE_GROUP_MAP_SIZE);
    addRmgCenteredGroupObject(&group, seerHut);
    group.preparePlacement();
    if (!placeQuestGroup(&group, origin)) {
        int value = object->m_definition->getValue(origin, this);
        replaceRmgObjectWithTreasure(this, object, value);
        return false;
    }
    m_usedQuestArtifacts[artifact] = true;
    m_nextSeerHutPrototypeIndex = (m_nextSeerHutPrototypeIndex + 1)
        % m_objectPrototypes[SEER].size();
    return true;
}

// Reserve the colour before filling the group and release it on failure.
// Failed placement releases each object's reservation before deletion.
VA(0x0054B8C0, 0x385)
MAC_ADDRESS(0x2508e8, 0x334)
b8 type_random_map_generator::placeKeyTentGuard(type_object* object, int targetValue)
{
    int color = object->m_properties->m_prototype->getSubtype();
    u32 index = findRmgPrototypeSubtypeIndex<u32>(
        m_objectPrototypes[BORDER_GUARD], color);
    if (index == m_objectPrototypes[BORDER_GUARD].size())
        return false;
    TRmgZone* origin = m_zones[m_map.getMapItem(object->m_position)->m_zoneState.m_zone];
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[BORDER_GUARD][index];
    TRmgTreasureGroup group(RMG_TREASURE_GROUP_MAP_SIZE, RMG_TREASURE_GROUP_MAP_SIZE);
    type_object* guard = new type_object(properties);
    setKeyTentColorDisabled(color, true);
    if (fillTreasureGroup(origin, &group, false, targetValue) && group.addGuard(guard)) {
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

// Callers keep ownership of the object. Retail bug: both find loops compare
// the iterator with null rather than end().
VA(0x0054BC50, 0x2AE)
MAC_ADDRESS(0x250c1c, 0x2ac)
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
        TRmgMapPosition entrance = getRmgPlacedObjectEntrance(object);
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
                    if (!item->hasObjects()) {
                        item->m_tileData.m_roadEntrance = false;
                        item->m_tileData.m_roadPassable = true;
                    }
                    item->m_zoneState.m_objectDistance = RMG_CLEARED_CELL_COST;
                }
            }
        }
    }
}

VA(0x0054BF00, 0x57)
MAC_ADDRESS(0x250fe0, 0x90)
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
    int strength = m_monsterStrength + RMG_ZONE_MONSTERS_AVERAGE;
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
    for (int seat = 0; seat < RMG_PLAYER_COUNT; ++seat) {
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

VA_COMPGEN(0x0054C6A0, 0x4D, QUEUE_LIST_DTOR, TPoint)

VA_COMPGEN(0x0054D000, 0x5E, LIST_INSERT_SINGLE, TPoint)

VA_COMPGEN(0x0054D060, 0x36, LIST_ERASE_ITERATOR, TPoint)

VA_COMPGEN(0x0054D0A0, 0x45, LIST_ERASE_RANGE, TPoint)

VA_COMPGEN(0x0054D0F0, 0x2D, LIST_BUYNODE, TPoint)

VA_COMPGEN(0x005166E0, 0x34, BITSET_TEST, Bitset10)

VA(0x005FDB10, 0x21)
MAC_ADDRESS(0x25c3b4, 0x38)
int getRmgSquaredDistance(TPoint first, TPoint second)
{
    int dy = first.m_y - second.m_y;
    int dx = first.m_x - second.m_x;
    return getRmgSquaredNorm(dx, dy);
}

// Circumcenter of the triangle with these three sites: the perpendicular
// bisector of the origin-to-second side, scaled by the projected sides.
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
MAC_ADDRESS(0x25d144, 0x12c)
void TRmgVoronoi::buildVertices()
{
    for (u32 index = 0; index < m_edges.size(); ++index) {
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

VA(0x005FDCB0, 0x1E)
TRmgVector TRmgVector::operator+(TRmgVector other) const
{
    return TRmgVector(m_x + other.m_x, m_y + other.m_y);
}

VA(0x005FDCD0, 0x1D)
TRmgVector TRmgVector::operator*(int scale) const
{
    TRmgVector result;
    result.m_x = m_x * scale;
    result.m_y = m_y * scale;
    return result;
}

VA(0x005FDCF0, 0x25)
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
