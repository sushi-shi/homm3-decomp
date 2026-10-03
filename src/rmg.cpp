// Random-map generator. HOMM3_RMG_HOTFIX (described in rmg.h, off by
// default) selects defined, non-crashing alternatives to retail bugs.

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
#include "advmgr.h"
#include "advmgr_objects.h"
#include "armygrp.h"
#include "artifact.h"
#include "creature_bank.h"
#include "hero.h"
#include "mapcell.h"
#include "objnames.h"
#include "quest.h"
#include "resourcemanager.h"
#include "rmg_request.h"
#include "rmg_terrain.h"
#include "savegame.h"
#include "seerhut.h"
#include "textresource.h"
#include "town.h"

enum ERmgMapLevel {
    RMG_SURFACE_LEVEL = 0,
    RMG_UNDERGROUND_LEVEL = 1,
    RMG_MAP_LEVEL_COUNT = 2
};

// Zone index of a cell outside every zone; cleared cells start here.
enum ERmgZoneSentinel {
    RMG_NO_ZONE = -1
};

// Each coordinate of an unset map position: no path predecessor, an unplaced
// object or no requested placement.
enum ERmgPositionSentinel {
    RMG_NO_POSITION = -1
};

// Euclidean distance, truncated; the squares are 32-bit integers.
MAC_ADDRESS(0x22cef0, 0x84)
s32 getRmgDistance(TPoint first, TPoint second)
{
    return static_cast<s32>(sqrt(static_cast<double>(
        getRmgSquaredDistance(first, second))));
}

DATA(0x006824e0)
s32 g_rmgCreatureValueByLevel[TOWN_DWELLING_COUNT] = {
    5000, 7000, 9000, 12000, 16000, 21000, 27000
};

void type_object::releaseReservation() {}
b8 type_object::completePlacement() { return true; }

b8 type_treasure_def::isTerrainDependent() { return false; }
b8 type_quest_creature_def::isTerrainDependent() { return true; }
b8 type_quest_experience_def::isTerrainDependent() { return true; }
b8 type_quest_gold_def::isTerrainDependent() { return true; }
b8 type_key_tent_def::isTerrainDependent() { return true; }

// Per-type object limits for the whole map and for each zone.
DATA(0x0069ce4c)
s32 g_rmgMapObjectLimits[ADVENTURE_OBJECT_TRAIT_COUNT];
DATA(0x0069d1f4)
s32 g_rmgZoneObjectLimits[ADVENTURE_OBJECT_TRAIT_COUNT];
DATA(0x00640718)
static const TRmgObjectLimit g_rmgMapObjectLimitOverrides[30] = {
    {EVENT,                200},
    {BLACK_BOX,            200},
    {OBELISK,              48},
    {BOAT,                 64},
    {TRAINING_GROUNDS,     32},
    {DEFENSE_TOWER,        32},
    {GARDEN_OF_REVELATION, 32},
    {MERC_CAMP,            32},
    {POWER_SCHOOL,         32},
    {TREE_OF_KNOWLEDGE,    32},
    {LIBRARY,              32},
    {ARENA,                32},
    {MAGIC_SCHOOL,         32},
    {WAR_SCHOOL,           32},
    {UNIVERSITY,           32},
    {WITCH_HUT,            32},
    {SHRINE1,              32},
    {SHRINE2,              32},
    {SHRINE3,              32},
    {SIREN,                32},
    {MYSTICAL_GARDEN,      32},
    {WATER_WHEEL,          32},
    {WINDMILL,             32},
    {MAGIC_SPRING,         32},
    {DEAD_GUY,             32},
    {LEAN_TO,              32},
    {WARRIOR_TOMB,         32},
    {WAGON,                32},
    {SEER,                 48},
    {BLACK_MARKET,         32},
};
DATA(0x00640808)
static const TRmgObjectLimit g_rmgZoneObjectLimitOverrides[24] = {
    {ALTAR_OF_SACRIFICE,  1},
    {CARTOGRAPHER,        1},
    {CLOVER_FIELD,        1},
    {COVER_OF_DARKNESS,   1},
    {EYE_OF_MAGI,         1},
    {FAERIE_RING,         1},
    {FOUNTAIN_OF_FORTUNE, 1},
    {FOUNTAIN_OF_YOUTH,   1},
    {HILL_FORT,           1},
    {IDOL_OF_FORTUNE,     1},
    {LIGHTHOUSE,          1},
    {MAGIC_SPRING,        1},
    {MAGIC_WELL,          1},
    {OASIS,               1},
    {OBSERVATORY,         1},
    {PILLAR_OF_FIRE,      1},
    {RALLY_FLAG,          1},
    {SANCTUARY,           1},
    {STABLES,             1},
    {TEMPLE,              1},
    {TRADING_POST,        1},
    {WAR_MACHINE_FACTORY, 1},
    {WATERING_HOLE,       1},
    {WITCH_HUT,           3},
};

// Frame i of a river sprite draws shape g_rmgRiverPatterns[i]; repeated
// shapes are alternative frames picked at random.
DATA(0x00641140)
static const s32 g_rmgRiverPatterns[13] = {
    LINE_SE, LINE_SE, LINE_SE, LINE_SE, LINE_CROSS, LINE_ESW, LINE_ESW,
    LINE_NES, LINE_NES, LINE_NS, LINE_NS, LINE_EW, LINE_EW
};
DATA(0x0069e5d0)
TRmgLinePatternTable g_rmgRiverPatternTable(13, g_rmgRiverPatterns);

VA_COMPGEN(0x0055ed70, 0x1d, STATIC_CTOR, g_rmgRiverPatternTable)

VA_COMPGEN(0x0055ed90, 0x0a, STATIC_DTOR, g_rmgRiverPatternTable)

// The same for road sprites.
DATA(0x006411ac)
static const s32 g_rmgRoadPatterns[17] = {
    LINE_SE, LINE_SE, LINE_SE_VARIANT, LINE_SE_VARIANT, LINE_SE_VARIANT, LINE_SE_VARIANT,
    LINE_NES, LINE_NES, LINE_ESW, LINE_ESW, LINE_NS, LINE_NS, LINE_EW, LINE_EW,
    LINE_END_S, LINE_END_E, LINE_CROSS
};
DATA(0x0069e650)
TRmgLinePatternTable g_rmgRoadPatternTable(17, g_rmgRoadPatterns);

VA_COMPGEN(0x0055f2f0, 0x1d, STATIC_CTOR, g_rmgRoadPatternTable)

VA_COMPGEN(0x0055f310, 0x0a, STATIC_DTOR, g_rmgRoadPatternTable)

VA(0x00530e20, 0x1c)
MAC_ADDRESS(0x22cf74, 0x18)
TProgressSink::TProgressSink(s32 totalSteps)
{
    m_steps = totalSteps;
    m_done = 0;
}

VA_COMPGEN(0x00530e40, 0x23, SCALAR_DELETING_DTOR, TProgressSink)

VA(0x00530e70, 0x07)
MAC_ADDRESS(0x22cf8c, 0x48)
TProgressSink::~TProgressSink()
{
}

VA(0x00530e80, 0x0d)
MAC_ADDRESS(0x22cfd4, 0x8)
void TProgressSink::setTotal(s32 totalSteps)
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
    RMG_FIRST_NORTHERN_DIRECTION = RMG_DIRECTION_NORTH_WEST,
    // Cardinal c is direction c * 2; stepping by this visits E, S, W, N.
    RMG_CARDINAL_DIRECTION_STEP = 2,
    RMG_CARDINAL_DIRECTION_COUNT = RMG_DIRECTION_COUNT / RMG_CARDINAL_DIRECTION_STEP
};

// Eight neighbour directions, clockwise from east; even entries are the
// four cardinal directions.
DATA(0x0069cdc0)
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
static inline s32 turnRmgDirection(s32 direction, s32 steps)
{
    return (direction + steps) & (RMG_DIRECTION_COUNT - 1);
}

static inline s32 getRmgOppositeDirection(s32 direction)
{
    return turnRmgDirection(direction, RMG_DIRECTION_COUNT / 2);
}

// SE, S or SW.
static inline bool isRmgSouthwardDirection(s32 direction)
{
    return direction >= RMG_DIRECTION_SOUTH_EAST
        && direction <= RMG_DIRECTION_SOUTH_WEST;
}

// Most object entrances are entered and left only through the cells beside
// and below them; these object types also allow the three cells above.
static inline bool isRmgEntranceOpenToNorth(TAdventureObjectType objectType)
{
    return g_adventureObjectTraits[objectType].m_enterableFromNorth != 0;
}

// Shipyards are three tiles wide; these offsets probe beside the left and
// right ends of the bottom footprint row and the row below it.
// North is up; digits are offset indices, P is the shipyard position,
// # the rest of its bottom row and . the row below.
//   0 # # P 1
//   2 . . . 3
DATA(0x0069ce00)
TPoint g_rmgShipyardWaterOffsets[RMG_SHIPYARD_WATER_OFFSET_COUNT] = {
    TPoint(-3, 0),
    TPoint(1, 0),
    TPoint(-3, 1),
    TPoint(1, 1)
};

// River-delta choice per coast side (east, south, west, north), for land and
// then snow rivers: the nth (from 0) delta recommended for the end's terrain.
DATA(0x006409a0)
static const s32 g_landRiverDeltaIndex[RMG_CARDINAL_DIRECTION_COUNT] = {2, 0, 3, 1};

DATA(0x006409b0)
static const s32 g_snowRiverDeltaIndex[RMG_CARDINAL_DIRECTION_COUNT] = {7, 5, 4, 6};

// Candidate town types one zone terrain can list.
enum ERmgTerrainTownChoiceLimits {
    RMG_TERRAIN_TOWN_CHOICE_COUNT = 4
};

// Candidate town types for each zone terrain, dirt to water. The pick only
// selects which creatures' dwellings and rewards the zone favours; eTownNeutral
// (no faction) favours neutral creatures. It was meant to end a row, but
// chooseTownType draws from all four entries, so the zero padding after it is
// a Castle candidate.
DATA(0x00682450)
s32 g_rmgTerrainTownChoices[eTerrainWater + 1][RMG_TERRAIN_TOWN_CHOICE_COUNT] = {
    {TOWN_CASTLE,     TOWN_RAMPART,    TOWN_NECROPOLIS, eTownNeutral}, // dirt
    {TOWN_STRONGHOLD, eTownNeutral,    0,               0},            // sand
    {TOWN_CASTLE,     TOWN_RAMPART,    eTownNeutral,    0},            // grass
    {TOWN_TOWER,      eTownNeutral,    0,               0},            // snow
    {TOWN_FORTRESS,   TOWN_NECROPOLIS, eTownNeutral,    0},            // swamp
    {TOWN_STRONGHOLD, TOWN_CONFLUX,    eTownNeutral,    0},            // rough
    {TOWN_DUNGEON,    TOWN_INFERNO,    TOWN_NECROPOLIS, eTownNeutral}, // subterranean
    {TOWN_INFERNO,    eTownNeutral,    0,               0},            // lava
    {eTownNeutral,    0,               0,               0}             // water
};

// Native terrain of each town alignment, used when choosing zone terrain.
DATA(0x006408c8)
static const TTerrainType g_rmgTownNativeTerrains[TOWN_TYPE_COUNT] = {
    eTerrainGrass, // castle
    eTerrainGrass, // rampart
    eTerrainSnow,  // tower
    eTerrainLava,  // inferno
    eTerrainDirt,  // necropolis
    eTerrainDirt,  // dungeon
    eTerrainRough, // stronghold
    eTerrainSwamp, // fortress
    eTerrainGrass  // conflux
};

enum ERmgRadialDirectionLimits {
    RMG_RADIAL_DIRECTION_COUNT = 32
};

// Radial directions used by the placement and boundary passes. Direction k
// lies k * 11.25 degrees from east; y grows southward, so k turns clockwise.
// North is up:
//         24
//   16     .     0
//          8
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

// Keymaster's tent and border guard colours (their object subtypes).
enum ERmgKeyColor {
    RMG_KEY_LIGHT_BLUE = 0,
    RMG_KEY_GREEN = 1,
    RMG_KEY_RED = 2,
    RMG_KEY_DARK_BLUE = 3,
    RMG_KEY_BROWN = 4,
    RMG_KEY_PURPLE = 5,
    RMG_KEY_WHITE = 6,
    RMG_KEY_BLACK = 7
};

// Guard strengths: the zone scale (ERmgZoneMonsterStrength) shifted by the
// map strength, from 0 up to the strongest.
enum ERmgGuardStrengthLimits {
    RMG_STRONGEST_GUARD_STRENGTH = 5,
    RMG_GUARD_STRENGTH_COUNT = RMG_STRONGEST_GUARD_STRENGTH + 1
};

// Guard value thresholds and scales by guard strength; a scale counts
// quarters of the value above its threshold.
DATA(0x006823f0)
s32 g_rmgGuardThresholdLow[RMG_GUARD_STRENGTH_COUNT] = {50000, 2500, 1500, 1000, 500, 0};
DATA(0x00682408)
s32 g_rmgGuardThresholdHigh[RMG_GUARD_STRENGTH_COUNT] = {50000, 7500, 7500, 7500, 5000, 5000};
DATA(0x00682420)
s32 g_rmgGuardScaleLow[RMG_GUARD_STRENGTH_COUNT] = {0, 2, 3, 4, 6, 6};
DATA(0x00682438)
s32 g_rmgGuardScaleHigh[RMG_GUARD_STRENGTH_COUNT] = {0, 2, 3, 4, 4, 6};

// Map-description names per resolved water content.
DATA(0x00682700)
static const char* g_rmgWaterNames[RMG_WATER_RANDOM] = {
    DATA_COMPGEN(0x006827ec, rmgWaterNone, "None"),
    DATA_COMPGEN(0x006827e4, rmgWaterNormal, "normal"),
    DATA_COMPGEN(0x006827dc, rmgWaterIslands, "islands")
};

DATA(0x0068270c)
static const char* g_rmgPlayerNames[RMG_PLAYER_COUNT] = {
    DATA_COMPGEN(0x006827d8, rmgPlayerRed, "red"),
    DATA_COMPGEN(0x006827d0, rmgPlayerBlue, "blue"),
    DATA_COMPGEN(0x006827cc, rmgPlayerTan, "tan"),
    DATA_COMPGEN(0x006827c4, rmgPlayerGreen, "green"),
    DATA_COMPGEN(0x006827bc, rmgPlayerOrange, "orange"),
    DATA_COMPGEN(0x006827b4, rmgPlayerPurple, "purple"),
    DATA_COMPGEN(0x006827ac, rmgPlayerTeal, "teal"),
    DATA_COMPGEN(0x006827a4, rmgPlayerPink, "pink")
};

DATA(0x0068272c)
static const char* g_rmgTownNames[TOWN_TYPE_COUNT] = {
    DATA_COMPGEN(0x0068279c, rmgTownCastle, "castle"),
    DATA_COMPGEN(0x00682794, rmgTownRampart, "rampart"),
    DATA_COMPGEN(0x0068278c, rmgTownTower, "tower"),
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

// rmg.txt columns. A template's first row holds its name and size range;
// any row may also hold one zone and one connection.
enum ERmgTemplateColumn {
    RMG_TEMPLATE_COLUMN_NAME = 0,
    RMG_TEMPLATE_COLUMN_MINIMUM_SIZE = 1,
    RMG_TEMPLATE_COLUMN_MAXIMUM_SIZE = 2,
    RMG_TEMPLATE_COLUMN_ZONE_INDEX = 3,
    RMG_TEMPLATE_COLUMN_KIND_HUMAN = 4,
    RMG_TEMPLATE_COLUMN_KIND_COMPUTER = 5,
    RMG_TEMPLATE_COLUMN_KIND_TREASURE = 6,
    RMG_TEMPLATE_COLUMN_KIND_JUNCTION = 7,
    RMG_TEMPLATE_COLUMN_SIZE = 8,
    RMG_TEMPLATE_COLUMN_MINIMUM_HUMAN_PLAYERS = 9,
    RMG_TEMPLATE_COLUMN_MAXIMUM_HUMAN_PLAYERS = 10,
    RMG_TEMPLATE_COLUMN_MINIMUM_PLAYERS = 11,
    RMG_TEMPLATE_COLUMN_MAXIMUM_PLAYERS = 12,
    RMG_TEMPLATE_COLUMN_PLAYER_INDEX = 13,
    RMG_TEMPLATE_COLUMN_PLAYER_BASIC_COUNT = 14,
    RMG_TEMPLATE_COLUMN_PLAYER_CASTLE_COUNT = 15,
    RMG_TEMPLATE_COLUMN_PLAYER_BASIC_DENSITY = 16,
    RMG_TEMPLATE_COLUMN_PLAYER_CASTLE_DENSITY = 17,
    RMG_TEMPLATE_COLUMN_NEUTRAL_BASIC_COUNT = 18,
    RMG_TEMPLATE_COLUMN_NEUTRAL_CASTLE_COUNT = 19,
    RMG_TEMPLATE_COLUMN_NEUTRAL_BASIC_DENSITY = 20,
    RMG_TEMPLATE_COLUMN_NEUTRAL_CASTLE_DENSITY = 21,
    RMG_TEMPLATE_COLUMN_NEUTRAL_TOWNS_MATCH_ZONE = 22,
    RMG_TEMPLATE_COLUMN_ALLOWED_TOWNS = 23,      // one per town type
    RMG_TEMPLATE_COLUMN_MINE_COUNTS = 32,        // one per resource
    RMG_TEMPLATE_COLUMN_MINE_DENSITIES = 39,     // one per resource
    RMG_TEMPLATE_COLUMN_USE_NATIVE_TERRAIN = 46,
    RMG_TEMPLATE_COLUMN_ALLOWED_TERRAIN = 47,    // one per land terrain
    RMG_TEMPLATE_COLUMN_MONSTER_STRENGTH = 55,
    RMG_TEMPLATE_COLUMN_GUARDS_MATCH_ZONE = 56,
    RMG_TEMPLATE_COLUMN_ALLOWED_MONSTERS = 57,   // neutral, then one per town type
    // Three columns per treasure tier.
    RMG_TEMPLATE_COLUMN_TREASURE_MINIMUM = 67,
    RMG_TEMPLATE_COLUMN_TREASURE_MAXIMUM = 68,
    RMG_TEMPLATE_COLUMN_TREASURE_DENSITY = 69,
    RMG_TEMPLATE_TREASURE_COLUMN_COUNT = 3,
    RMG_TEMPLATE_COLUMN_LAST_TREASURE_DENSITY = 75,
    RMG_TEMPLATE_COLUMN_CONNECTION_FIRST_ZONE = 76,
    RMG_TEMPLATE_COLUMN_CONNECTION_SECOND_ZONE = 77,
    RMG_TEMPLATE_COLUMN_CONNECTION_VALUE = 78,
    RMG_TEMPLATE_COLUMN_CONNECTION_UNGUARDED = 79,            // "Wide"
    RMG_TEMPLATE_COLUMN_CONNECTION_BORDER_GUARD = 80,         // "Border Guard"
    RMG_TEMPLATE_COLUMN_CONNECTION_MINIMUM_HUMAN_PLAYERS = 81,
    RMG_TEMPLATE_COLUMN_CONNECTION_MAXIMUM_HUMAN_PLAYERS = 82,
    RMG_TEMPLATE_COLUMN_CONNECTION_MINIMUM_PLAYERS = 83,
    RMG_TEMPLATE_COLUMN_CONNECTION_MAXIMUM_PLAYERS = 84
};

// Offsets from a zone's or connection's minimum human players column.
enum ERmgPlayerLimitColumn {
    RMG_PLAYER_LIMIT_MAXIMUM_HUMAN_PLAYERS = 1,
    RMG_PLAYER_LIMIT_MINIMUM_PLAYERS = 2,
    RMG_PLAYER_LIMIT_MAXIMUM_PLAYERS = 3
};

// rand_trn.txt columns. Neighbour scores take one column per rule: all
// adjacent scores, then all blocked scores.
enum ERmgPlacementRuleColumn {
    RMG_PLACEMENT_COLUMN_OBJECT_TYPE = 3,
    RMG_PLACEMENT_COLUMN_SUBTYPE = 4,
    RMG_PLACEMENT_COLUMN_TERRAIN = 6,
    RMG_PLACEMENT_COLUMN_TERRAIN_SCORES = 7,     // dirt through water
    RMG_PLACEMENT_COLUMN_NEIGHBOUR_SCORES = 16
};

} // namespace

static void __fastcall assignRmgTeams(
    s32 teamCount,
    s32 playerCount,
    s32 firstTeam,
    const b8* players,
    char* teams);

template <u32 N>
static void setAvailableRmgHeroes(
    std::bitset<N>* availableHeroes,
    b8* disabledBegin,
    b8* disabledEnd)
{
    std::transform(disabledBegin, disabledEnd,
        bitset_iterator<N>(*availableHeroes, 0),
        std::logical_not<b8>());
}

VA_COMPGEN(0x00530f80, 0x21, SCALAR_DELETING_DTOR, type_random_map)

VA(0x00530e90, 0x4a)
MAC_ADDRESS(0x22cfdc, 0x50)
TRmgMapItem::TRmgMapItem()
{
    clear();
}

VA_COMPGEN(0x00530ee0, 0x26, IMPLICIT_DTOR, TRmgMapItem)
MAC_COMPGEN_ADDRESS(0x22d02c, 0x84, IMPLICIT_DTOR, TRmgMapItem)

// First plain (shape 0) water frame.
enum ERmgWaterFrames {
    RMG_WATER_BASE_FRAME = 21
};

// Resets the cell to empty water. The guard colour, connection-visited flag
// and previous-tile Y/Z are left unchanged.
VA(0x00530f10, 0x6f)
MAC_ADDRESS(0x22d110, 0x15c)
void TRmgMapItem::clear()
{
    m_objects.clear();
    TRmgConnectionDecoration borderConnection = m_borderConnection;
    TRmgGroundTileData tileData = m_tileData;

    borderConnection.m_present = false;
    m_tile.m_landType = eTerrainWater;
    m_tile.m_terrainFrame = RMG_WATER_BASE_FRAME;
    m_tile.m_riverType = 0;
    m_tile.m_riverFrame = 0;
    m_tile.m_roadType = 0;
    tileData.m_roadFrame = 0;
    tileData.m_blockedDirections = 0;
    // Read only once a flood sets the connection zone.
    tileData.m_connectionDirection = RMG_DIRECTION_EAST;
    tileData.m_terrainFlipX = false;
    tileData.m_terrainFlipY = false;
    tileData.m_riverFlipX = false;
    tileData.m_riverFlipY = false;
    tileData.m_roadFlipX = false;
    tileData.m_roadFlipY = false;
    tileData.m_coastal = false;
    tileData.m_objectEntrance = false;
    tileData.m_placementOutline = false;
    tileData.m_passable = true;
    tileData.m_obstacleFill = false;
    tileData.m_pathClearance = true;
    tileData.m_paintZoneTerrain = false;
    tileData.m_hasRiver = false;
    tileData.m_riverTarget = false;
    tileData.m_nearRiver = false;
    m_borderConnection = borderConnection;
    m_movement.m_cost = RMG_CLEARED_CELL_COST;
    m_movement.m_zonePathCost = RMG_CLEARED_CELL_COST;
    m_zoneState.m_objectDistance = RMG_CLEARED_CELL_COST;
    m_zoneState.m_zone = RMG_NO_ZONE;
    m_zoneState.m_connectionZone = RMG_NO_ZONE;
    m_previousTile.m_x = RMG_NO_POSITION;
    m_tileData = tileData;
}

VA(0x00530fb0, 0xa0)
MAC_ADDRESS(0x22d394, 0xbc)
type_random_map::type_random_map(s32 width, s32 height, s32 levels)
{
    m_mapWidth = width;
    m_mapHeight = height;
    m_numberLevels = levels;
    m_ownsMapItems = true;
    m_mapItems = new TRmgMapItem[m_mapWidth * m_mapHeight * m_numberLevels];
}

VA_COMPGEN(0x00531050, 0x58, VECTOR_DELETING_DTOR, TRmgMapItem)

VA(0x005310b0, 0x83)
MAC_ADDRESS(0x22d450, 0x84)
type_random_map::~type_random_map()
{
    if (m_ownsMapItems)
        delete[] m_mapItems;
}

VA(0x00531140, 0x2a)
MAC_ADDRESS(0x22d4d4, 0x60)
void type_random_map::clear()
{
    TRmgMapItem* mapItem = m_mapItems;
    s32 mapItemCount = m_mapWidth * m_mapHeight * m_numberLevels;
    while (mapItemCount--) {
        mapItem->clear();
        ++mapItem;
    }
}

// Shared by the footprint and surrounding-outline placement tests;
// obstacle-mark, water and path-clearance policies stay with each caller.
inline bool TRmgMapItem::isPlacementBlocked(s32 zoneIndex) const
{
    return !isPassableLand() || isObjectEntrance()
        || m_zoneState.m_zone != zoneIndex;
}

// hasConnectedOutline's requirePathClearance argument.
enum ERmgOutlinePathClearance {
    RMG_IGNORE_PATH_CLEARANCE = false,
    RMG_REQUIRE_PATH_CLEARANCE = true
};

VA(0x00531170, 0x19c)
MAC_ADDRESS(0x22d534, 0x244)
b8 type_random_map::hasConnectedOutline(
    const std::vector<TPoint>& outline, TRmgMapPosition position,
    b8 allowEntrances, TRmgZone* zone, b8 requirePathClearance)
{
    // Accepts at most one blocked run and at least one open cell, and rejects
    // entrances unless allowed. The first point is revisited to close the last
    // run; the outline must be nonempty.
    b8 blocked = true;
    b8 foundBoundary = false;
    b8 waterZone = zone->m_terrain == eTerrainWater;
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    for (u32 index = 0; index < outline.size() + 1; ++index) {
        b8 previouslyBlocked = blocked;
        TPoint offset(outline[index % outline.size()]);
        s32 x = position.m_x + offset.m_x;
        s32 y = position.m_y + offset.m_y;
        if (!containsXY(TPoint(x, y))) {
            blocked = true;
        } else {
            TRmgMapItem* item = getMapItem(x, y, position.m_z);
            if (!allowEntrances && item->isObjectEntrance())
                return false;
            blocked = item->isPlacementBlocked(zoneIndex);
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
    return !blocked || foundBoundary;
}

// Radius of a square neighbourhood around a cell, clipped to the map.
enum ERmgNeighborhoodRadius {
    RMG_NEIGHBORHOOD_3X3 = 1,
    RMG_NEIGHBORHOOD_5X5 = 2
};

inline void type_random_map::getNeighborhoodBounds(TRmgZoneBounds& bounds,
    const TPoint& position, s32 radius) const
{
    bounds.m_minimumY = max(position.m_y - radius, 0);
    bounds.m_minimumX = max(position.m_x - radius, 0);
    bounds.m_maximumY = min(position.m_y + radius + 1, m_mapHeight);
    bounds.m_maximumX = min(position.m_x + radius + 1, m_mapWidth);
}

VA(0x00531310, 0x14b)
MAC_ADDRESS(0x22d778, 0x234)
void type_random_map::markCoastalTiles()
{
    TRmgMapPosition position;
    TRmgMapItem* item = m_mapItems;
    for (position.m_z = RMG_SURFACE_LEVEL; position.m_z < m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_mapWidth; ++position.m_x, ++item) {
                if (item->getLandType() == eTerrainWater) {
                    TRmgZoneBounds bounds;
                    getNeighborhoodBounds(bounds, position, RMG_NEIGHBORHOOD_3X3);
                    for (s32 nearY = bounds.m_minimumY; nearY < bounds.m_maximumY; ++nearY) {
                        for (s32 nearX = bounds.m_minimumX; nearX < bounds.m_maximumX; ++nearX) {
                            TRmgMapItem* neighbour = getMapItem(nearX, nearY, position.m_z);
                            if (neighbour->getLandType() != eTerrainWater
                                && neighbour->getLandType() != eTerrainRock)
                                neighbour->m_tileData.m_coastal = true;
                        }
                    }
                }
            }
        }
    }
}

// Binary search in a cost-descending worklist. Equal costs go before older
// entries, so popping from the back processes ties oldest first.
static s32 findRmgWorkItemInsertionIndex(
    const std::vector<s32>& costs, s32 count, s32 cost)
{
    s32 first = 0;
    s32 last = count;
    while (first < last) {
        s32 middle = (first + last) >> 1;
        if (cost < costs[middle])
            first = middle + 1;
        else
            last = middle;
    }
    return first;
}

static void insertRmgWorkItem(
    std::vector<TRmgMapPosition>& positions,
    std::vector<s32>& costs,
    TRmgMapPosition position,
    s32 cost)
{
    s32 insertionIndex = findRmgWorkItemInsertionIndex(costs, positions.size(), cost);
    positions.insert(positions.begin() + insertionIndex, position);
    costs.insert(costs.begin() + insertionIndex, cost);
}

static inline void popRmgWorkItem(TRmgMapPosition& position,
    std::vector<TRmgMapPosition>& positions, std::vector<s32>& costs)
{
    position = positions.back();
    costs.pop_back();
    positions.pop_back();
}

// Seed a zero-cost source with no predecessor in both worklists and the
// movement map.
inline TRmgMapItem* type_random_map::seedMovementSearch(const TRmgMapPosition& source,
    std::vector<TRmgMapPosition>& positions, std::vector<s32>& costs)
{
    positions.push_back(source);
    costs.push_back(0);
    TRmgMapItem* item = getMapItem(source);
    item->setMovementCost(0,
        TRmgMapPosition(RMG_NO_POSITION, RMG_NO_POSITION, RMG_NO_POSITION));
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
    std::vector<s32> costs;
    std::vector<TRmgMapPosition> positions;
    TRmgMapItem* seed = seedMovementSearch(position,
        positions, costs);
    s32 zone = seed->m_zoneState.m_zone;
    while (positions.size()) {
        TRmgMapPosition currentPosition;
        popRmgWorkItem(currentPosition, positions, costs);
        TRmgMapItem* current = getMapItem(currentPosition);
        s32 currentZone = current->m_zoneState.m_zone;
        s32 currentCost = currentZone == zone
            ? current->m_movement.m_cost : current->m_movement.m_zonePathCost;
        s32 direction = RMG_DIRECTION_COUNT;
        if (current->isObjectEntrance()) {
            TAdventureObjectType objectType = current->getEntranceObjectType();
            if (!isRmgEntranceOpenToNorth(objectType))
                direction = RMG_FIRST_NORTHERN_DIRECTION;
        }
        while (direction--) {
            s32 nextCost = currentCost + 1;
            TRmgMapPosition nextPosition = currentPosition;
            nextPosition += g_rmgDirections[direction];
            if (!containsXY(nextPosition))
                continue;
            TRmgMapItem* next = getMapItem(nextPosition);
            if (next->m_zoneState.m_zone < 0 || !next->isPassableLand())
                continue;
            if (next->isObjectEntrance()) {
                TAdventureObjectType objectType = next->getEntranceObjectType();
                const TAdvObjectTraits& traits = g_adventureObjectTraits[objectType];
                if (traits.m_blocksLanding && !traits.m_clearedOnVisit)
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
            s32 insertionIndex = findRmgWorkItemInsertionIndex(
                costs, positions.size(), nextCost);
            costs.insert(costs.begin() + insertionIndex, nextCost);
            positions.insert(positions.begin() + insertionIndex, nextPosition);
        }
    }
}

static inline bool isRmgWaterOnlyPrototype(const TObjectType& prototype)
{
    return prototype.m_slotCategory == TObjectType::SLOT_CATEGORY_0
        && prototype.m_recommendedTerrainMask.test(eTerrainWater);
}

// isPlacementBlocked's rejectObstacleFill argument.
enum ERmgObstacleEntrancePolicy {
    RMG_ALLOW_OBSTACLE_ENTRANCES = false,
    RMG_REJECT_OBSTACLE_ENTRANCES = true
};

// Checks the footprint against map bounds, zones and entrances. Only trigger
// cells honour rejectObstacleFill; only blocked cells apply the water rule
// (water-only objects must stand on water, others on land).
VA(0x005318b0, 0x212)
MAC_ADDRESS(0x22dfe0, 0x2a4)
b8 type_random_map::isPlacementBlocked(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position,
    s32 zoneIndex, b8 rejectObstacleFill)
{
    TObjectType& prototype = *properties->m_prototype;
    if (position.m_x < prototype.getWidth() - 1 || position.m_x >= m_mapWidth
        || position.m_y < prototype.getHeight() - 1 || position.m_y >= m_mapHeight)
        return true;
    TRmgMapPosition cell = position;
    for (u32 y = 0; y < prototype.getHeight(); ++y, --cell.m_y) {
        cell.m_x = position.m_x;
        for (u32 x = 0; x < prototype.getWidth(); ++x, --cell.m_x) {
            TRmgMapItem* item = getMapItem(cell);
            if (prototype.isTriggerCell(x, y)) {
                if (item->isPlacementBlocked(zoneIndex))
                    return true;
                if (rejectObstacleFill && item->hasObstacleFill())
                    return true;
            }
            if (!prototype.isPassableCell(x, y)) {
                if (item->isPlacementBlocked(zoneIndex))
                    return true;
                if ((item->getLandType() == eTerrainWater)
                    != isRmgWaterOnlyPrototype(prototype))
                    return true;
            }
        }
    }
    return false;
}

VA(0x00531ad0, 0x100)
MAC_ADDRESS(0x22e284, 0x1d0)
void type_random_map::openPathPatch(s32 x, s32 y, s32 level)
{
    TRmgMapItem* item = getMapItem(x, y, level);
    item->openPath();
    TRmgZoneBounds bounds;
    getNeighborhoodBounds(bounds, TPoint(x, y), RMG_NEIGHBORHOOD_3X3);
    for (s32 row = bounds.m_minimumY; row < bounds.m_maximumY; ++row) {
        for (s32 column = bounds.m_minimumX; column < bounds.m_maximumX; ++column) {
            TRmgMapItem* nearby = getMapItem(column, row, level);
            nearby->openPath();
        }
    }
}

VA(0x00531bd0, 0x11f)
MAC_ADDRESS(0x22e454, 0x254)
void type_random_map::markBorderPatch(TRmgMapPosition position)
{
    TRmgMapItem* item = getMapItem(position);
    item->markObstacleFill();
    TRmgZoneBounds bounds;
    getNeighborhoodBounds(bounds, position, RMG_NEIGHBORHOOD_3X3);
    for (s32 row = bounds.m_minimumY; row < bounds.m_maximumY; ++row) {
        for (s32 column = bounds.m_minimumX; column < bounds.m_maximumX; ++column) {
            TRmgMapItem* nearby = getMapItem(column, row, position.m_z);
            if (!nearby->isObjectEntrance() && nearby->isPassableLand()
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
inline TRmgMapPosition type_object::getEntrance() const
{
    return getRmgObjectTriggerPosition(getPosition(),
        m_properties->m_prototype->m_triggerCell);
}

static inline bool allowsRmgSharedObjectEntrance(TAdventureObjectType objectType)
{
    return g_adventureObjectTraits[objectType].m_clearedOnVisit
        && isRmgEntranceOpenToNorth(objectType);
}

// The object must fit and have a connected outline. With a trigger, the cell
// below it must also be passable land of the same zone and water class; if
// that cell is another entrance, its object must be cleared on visit.
VA(0x00531cf0, 0x1a5)
MAC_ADDRESS(0x22e6a8, 0x270)
b8 type_random_map::canPlaceObject(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position, TRmgZone* zone)
{
    TObjectType& prototype = *properties->m_prototype;
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    if (isPlacementBlocked(properties, position, zoneIndex, RMG_ALLOW_OBSTACLE_ENTRANCES))
        return false;
    TAdventureObjectType objectType = prototype.getObjectType();
    properties->buildOutline();
    if (!hasConnectedOutline(properties->m_outline, position,
            allowsRmgSharedObjectEntrance(objectType),
            zone, RMG_IGNORE_PATH_CLEARANCE))
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
    if (item->isObjectEntrance()) {
        TAdventureObjectType entranceType = item->getEntranceObjectType();
        if (!g_adventureObjectTraits[entranceType].m_clearedOnVisit)
            return false;
    }
    return (item->getLandType() == eTerrainWater) == (zone->m_terrain == eTerrainWater);
}

// Puts the object on the map: trigger cells become entrances with open
// paths, other non-passable cells become object-blocked, and both record it.
VA(0x00531ea0, 0x2e6)
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
            TRmgMapItem* item = getMapItem(cell);
            if (prototype.isTriggerCell(x, y)) {
                item->m_tileData.m_objectEntrance = true;
                item->openPath();
                item->m_objects.push_back(&object);
            } else if (!prototype.isPassableCell(x, y)) {
                item->m_tileData.m_passable = false;
                item->m_objects.push_back(&object);
            }
        }
    }
}

VA(0x00532190, 0x6d)
MAC_ADDRESS(0x22eadc, 0x70)
void type_random_map::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    TRmgMapItem& item = *getMapItem(point.m_x, point.m_y);
    item.setTerrain(tile.m_terrain, tile.m_frame, tile.m_flipX, tile.m_flipY);
}

VA(0x00532200, 0x3c)
MAC_ADDRESS(0x22eb4c, 0x38)
void type_random_map::setFrame(const TRmgGridPoint& point, s32 value)
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

VA(0x005322c0, 0x2a)
MAC_ADDRESS(0x22ec18, 0x34)
s32 type_random_map::getTerrain(const TRmgGridPoint& point)
{
    return getMapItem(point.m_x, point.m_y)->getLandType();
}

VA(0x005322f0, 0x2a)
MAC_ADDRESS(0x22ec4c, 0x34)
s32 type_random_map::getFrame(const TRmgGridPoint& point)
{
    return getMapItem(point.m_x, point.m_y)->m_tile.m_terrainFrame;
}

VA(0x00532350, 0x07)
MAC_ADDRESS(0x22ec98, 0x48)
TRmgRoadMapAdapterInterface::~TRmgRoadMapAdapterInterface()
{
}

VA_COMPGEN(0x00537940, 0x23, SCALAR_DELETING_DTOR, TRmgRoadMapAdapterInterface)

VA(0x00532360, 0x6e)
MAC_ADDRESS(0x22ece0, 0x6c)
void TRmgRoadMapAdapter::setTile(
    const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    TRmgMapItem& item = *m_map->getMapItem(point.m_x, point.m_y);
    item.m_tile.m_roadType = tile.m_terrain;
    item.m_tileData.m_roadFrame = tile.m_frame;
    item.m_tileData.m_roadFlipX = tile.m_flipX;
    item.m_tileData.m_roadFlipY = tile.m_flipY;
}

VA(0x005323d0, 0x3c)
MAC_ADDRESS(0x22ed4c, 0x3c)
void TRmgRoadMapAdapter::setLineType(const TRmgGridPoint& point, s32 value)
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

VA(0x00532480, 0x2d)
MAC_ADDRESS(0x22ee64, 0x38)
s32 TRmgRoadMapAdapter::getLineType(const TRmgGridPoint& point)
{
    return m_map->getMapItem(point.m_x, point.m_y)->m_tile.m_roadType;
}

VA(0x005324b0, 0x2d)
MAC_ADDRESS(0x22ee9c, 0x38)
s32 TRmgRoadMapAdapter::getTerrain(const TRmgGridPoint& point)
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

VA_COMPGEN(0x005324e0, 0x21, SCALAR_DELETING_DTOR, TRmgRiverMapAdapter)

VA(0x00532520, 0x205)
MAC_ADDRESS(0x22ef34, 0x35c)
void TRmgRiverMapAdapter::setTile(const TRmgGridPoint& point, const rmgTerrainTile& tile)
{
    TRmgMapItem& item = *m_map->getMapItem(point.m_x, point.m_y);
    item.m_tile.m_riverType = tile.m_terrain;
    item.m_tile.m_riverFrame = tile.m_frame;
    item.m_tileData.m_riverFlipX = tile.m_flipX;
    item.m_tileData.m_riverFlipY = tile.m_flipY;
    item.m_tileData.m_hasRiver = tile.m_terrain != 0;
    if (item.hasRiver()) {
        TRmgZoneBounds bounds;
        m_map->getNeighborhoodBounds(bounds, point, RMG_NEIGHBORHOOD_3X3);
        for (s32 y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
            for (s32 x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                TRmgMapItem& neighbour = *m_map->getMapItem(x, y);
                neighbour.m_tileData.m_nearRiver = true;
            }
        }
        m_map->getNeighborhoodBounds(bounds, point, RMG_NEIGHBORHOOD_5X5);
        for (y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
            for (s32 x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                TRmgMapItem& neighbour = *m_map->getMapItem(x, y);
                if (neighbour.m_tile.m_riverType == 0)
                    neighbour.m_tileData.m_riverTarget = false;
            }
        }
    }
}

VA(0x00532730, 0x57)
MAC_ADDRESS(0x22f290, 0x54)
void TRmgRiverMapAdapter::setLineType(const TRmgGridPoint& point, s32 value)
{
    TRmgMapItem& item = *m_map->getMapItem(point.m_x, point.m_y);
    item.m_tile.m_riverType = value;
    item.m_tileData.m_hasRiver = value != 0;
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

VA(0x005327c0, 0x63)
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

VA(0x00532830, 0x2d)
MAC_ADDRESS(0x22f3c4, 0x38)
s32 TRmgRiverMapAdapter::getLineType(const TRmgGridPoint& point)
{
    return m_map->getMapItem(point.m_x, point.m_y)->m_tile.m_riverType;
}

VA(0x00532860, 0x2d)
MAC_ADDRESS(0x22f3fc, 0x38)
s32 TRmgRiverMapAdapter::getTerrain(const TRmgGridPoint& point)
{
    return m_map->getMapItem(point.m_x, point.m_y)->getLandType();
}

// Bits of the H3M tile flag byte.
enum ERmgTileFlags {
    RMG_TILE_TERRAIN_FLIP_X = 0x01,
    RMG_TILE_TERRAIN_FLIP_Y = 0x02,
    RMG_TILE_RIVER_FLIP_X = 0x04,
    RMG_TILE_RIVER_FLIP_Y = 0x08,
    RMG_TILE_ROAD_FLIP_X = 0x10,
    RMG_TILE_ROAD_FLIP_Y = 0x20,
    RMG_TILE_COASTAL = 0x40
};

VA(0x00532890, 0x104)
MAC_ADDRESS(0x22f434, 0x1d4)
void TRmgMapItem::write(TAbstractFile* outputFile)
{
    writeValue<u8>(outputFile, m_tile.m_landType);
    writeValue<u8>(outputFile, m_tile.m_terrainFrame);
    writeValue<u8>(outputFile, m_tile.m_riverType);
    writeValue<u8>(outputFile, m_tile.m_riverFrame);
    writeValue<u8>(outputFile, m_tile.m_roadType);
    writeValue<u8>(outputFile, m_tileData.m_roadFrame);
    u8 flags = 0;
    if (m_tileData.m_terrainFlipX) flags |= RMG_TILE_TERRAIN_FLIP_X;
    if (m_tileData.m_terrainFlipY) flags |= RMG_TILE_TERRAIN_FLIP_Y;
    if (m_tileData.m_riverFlipX) flags |= RMG_TILE_RIVER_FLIP_X;
    if (m_tileData.m_riverFlipY) flags |= RMG_TILE_RIVER_FLIP_Y;
    if (m_tileData.m_roadFlipX) flags |= RMG_TILE_ROAD_FLIP_X;
    if (m_tileData.m_roadFlipY) flags |= RMG_TILE_ROAD_FLIP_Y;
    if (m_tileData.m_coastal) flags |= RMG_TILE_COASTAL;
    writeValue<u8>(outputFile, flags);
}

VA_COMPGEN(0x005329a0, 0x32, IMPLICIT_DTOR, TRmgTemplateZone)
MAC_COMPGEN_ADDRESS(0x22f660, 0x68, IMPLICIT_DTOR, TRmgTemplateZone)

// Picks a uniformly random allowed town type, or eTownNeutral if none.
MAC_ADDRESS(0x22f6c8, 0xb4)
s32 TRmgTemplateZone::selectAllowedTown()
{
    s32 allowedTownCount = 0;
    for (s32 town = TOWN_CASTLE; town < TOWN_TYPE_COUNT; ++town) {
        if (m_allowedTowns[town])
            ++allowedTownCount;
    }
    if (!allowedTownCount)
        return eTownNeutral;
    s32 selectedIndex = rand() % allowedTownCount;
    for (town = TOWN_CASTLE; town < TOWN_TYPE_COUNT; ++town) {
        if (m_allowedTowns[town] && --selectedIndex < 0)
            return town;
    }
    return eTownNeutral;
}

VA(0x005329e0, 0xcf)
MAC_ADDRESS(0x22f7c4, 0xc0)
TRmgZone::TRmgZone(TRmgTemplateZone* templateZone)
{
    m_templateZone = templateZone;
    m_alignment = static_cast<TTownType>(templateZone->selectAllowedTown());
    m_scaledSize = templateZone->m_size;
    m_bounds.resetEmpty();
    m_hasPrimaryTown = false;
    memset(m_objectCountByType, 0, sizeof(m_objectCountByType));
}

MAC_ADDRESS(0x22f9d8, 0xcc)
void TRmgZone::chooseTownType(b8 expanded)
{
    if (m_alignment != eTownNeutral) {
        m_creatureTownType = m_alignment;
    } else {
        s32 count = 0;
        // Retail bug: this condition is always true, so all four row
        // entries are candidates, including eTownNeutral and the padding.
        while (count < RMG_TERRAIN_TOWN_CHOICE_COUNT &&
            (g_rmgTerrainTownChoices[m_terrain][count] != eTownNeutral || expanded ||
             g_rmgTerrainTownChoices[m_terrain][count] != TOWN_CONFLUX))
            ++count;
        if (count == 0)
            m_creatureTownType = eTownNeutral;
        else
            m_creatureTownType = static_cast<TTownType>(
                g_rmgTerrainTownChoices[m_terrain][rand() % count]);
    }
}

inline bool TRmgZone::isTerrainAllowed(s32 terrain) const
{
    return m_templateZone->m_allowedTerrain[terrain]
        && (terrain != eTerrainSubterranean
            || m_levelPosition.m_z == RMG_UNDERGROUND_LEVEL);
}

VA(0x00532ab0, 0x96)
MAC_ADDRESS(0x22faa4, 0x130)
void TRmgZone::chooseTerrain()
{
    if (m_templateZone->m_useNativeTerrain && m_alignment != eTownNeutral) {
        m_terrain = g_rmgTownNativeTerrains[m_alignment];
    } else {
        s32 count = 0;
        for (s32 terrain = eTerrainDirt; terrain < eTerrainWater; ++terrain) {
            if (isTerrainAllowed(terrain))
                ++count;
        }
        if (!count) {
            m_terrain = eTerrainDirt;
        } else {
            s32 selected = rand() % count;
            s32 terrain;
            for (terrain = eTerrainDirt; terrain < eTerrainWater; ++terrain) {
                if (isTerrainAllowed(terrain) && selected-- <= 0)
                    break;
            }
            m_terrain = static_cast<TTerrainType>(terrain);
        }
    }
    if (m_levelPosition.m_z == RMG_UNDERGROUND_LEVEL && m_terrain != eTerrainLava)
        m_terrain = eTerrainSubterranean;
}

VA(0x00532b50, 0x76)
MAC_ADDRESS(0x22fbd4, 0xa0)
TRmgZone::~TRmgZone()
{
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
VA(0x00532bd0, 0xa8)
MAC_ADDRESS(0x22fdc0, 0xc8)
b8 TRmgZone::canConnect(const TRmgZone* other) const
{
    s32 distance = getRmgDistance(m_levelPosition, other->m_levelPosition);
    s32 thisSize = m_templateZone->m_size;
    s32 otherSize = other->m_templateZone->m_size;
    s32 combinedSize = thisSize + otherSize;
    if (other->m_levelPosition.m_z != m_levelPosition.m_z) {
        if (combinedSize < distance)
            return false;
        s32 minimumSize = thisSize;
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

static inline TPoint nextRmgOutlineProbe(const TPoint& position, s32& direction)
{
    direction = turnRmgDirection(direction, -2);
    TPoint offset = g_rmgDirections[direction];
    return TPoint(position.m_x + offset.m_x, position.m_y + offset.m_y);
}

static inline void advanceRmgOutlineWalk(TPoint& position, s32& direction)
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
VA(0x00532c80, 0x1ba)
MAC_ADDRESS(0x22fe88, 0x208)
void TRmgObjectPropertiesRef::buildOutline()
{
    // Wall-follows the footprint with cardinal steps. Offsets are relative
    // to the object's bottom-right cell, so footprint cells are nonpositive.
    if (m_outline.size() > 0)
        return;
    TPoint position(0, 0);
    while (static_cast<u32>(-position.m_x) < m_prototype->getWidth()) {
        if (isRmgObjectFootprintCell(m_prototype, -position.m_x, 0))
            break;
        --position.m_x;
    }
    if (position.m_x == -m_prototype->getWidth())
        return;
    position.m_y = 1;
    TPoint start = position;
    s32 direction = RMG_DIRECTION_NORTH;
    do {
        m_outline.push_back(position);
        s32 attempts = 0;
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

VA(0x00532e40, 0x19e)
MAC_ADDRESS(0x230090, 0x16c)
void TRmgObjectPropertiesRef::buildOverlapPriorities()
{
    if (m_prioritiesInitialized)
        return;
    m_prioritiesInitialized = true;
    for (u32 x = 0; x < m_prototype->getWidth(); ++x) {
        s32 priority = !m_prototype->isUnderlay();
        for (u32 y = 0; y < m_prototype->getHeight(); ++y) {
            if (y > 0 && !m_prototype->isUnderlay()) {
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
            if (m_prototype->isDrawCell(x, y))
                m_overlapPriorities[x][y] = priority;
        }
    }
}

VA(0x00532fe0, 0xb4)
MAC_ADDRESS(0x230254, 0xb0)
TRmgTemplate::~TRmgTemplate()
{
    for (s32 zone = 0; zone < m_zones.size(); ++zone)
        delete m_zones[zone];
}

VA(0x005330a0, 0x3e)
MAC_ADDRESS(0x230304, 0x48)
TRmgTemplateZone* TRmgTemplate::findZone(s32 zoneIndex)
{
    for (s32 zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->m_zoneIndex == zoneIndex)
            return m_zones[zone];
    }
    return 0;
}

TRmgMapPosition type_object::getPosition() const
{
    return m_position;
}

VA(0x005330e0, 0x39)
MAC_ADDRESS(0x2303ec, 0x5c)
type_object::type_object(TRmgObjectPropertiesRef* newProperties)
{
    m_properties = newProperties;
    ++m_properties->m_refCount;
    m_position.m_x = RMG_NO_POSITION;
    m_position.m_y = RMG_NO_POSITION;
    m_position.m_z = RMG_NO_POSITION;
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

rmgSpellScrollObject::rmgSpellScrollObject(TRmgObjectPropertiesRef* properties, ESpellId spell)
    : type_object(properties), m_spell(spell)
{
}

rmgWitchHutObject::rmgWitchHutObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

rmgBlackBoxObject::rmgBlackBoxObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties), m_experience(0), m_creatureType(CREATURE_NONE),
      m_creatureCount(0)
{
    memset(m_resources, 0, sizeof(m_resources));
}

VA_COMPGEN(0x00533120, 0x2d, SCALAR_DELETING_DTOR, type_object)

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
    writeValue<u8>(outputFile, position.m_x);
    writeValue<u8>(outputFile, position.m_y);
    writeValue<u8>(outputFile, position.m_z);
}

// Writes count zero bytes (at most 32). count is an argument, not a template
// parameter: VC6 merges function templates that differ only in such a value.
static inline void writeRmgReservedBytes(TAbstractFile* outputFile, s32 count)
{
    u8 reserved[32];
    memset(reserved, 0, count);
    outputFile->write(reserved, count);
}

VA(0x00533170, 0x79)
MAC_ADDRESS(0x2304c8, 0xfc)
void type_object::write(TAbstractFile* outputFile, s32 version)
{
    writeRmgMapPosition(outputFile, m_position);
    writeValue<s32>(outputFile, m_properties->m_prototypeIndex);
    writeRmgReservedBytes(outputFile, 5);
}

VA(0x005331f0, 0xfd)
MAC_ADDRESS(0x23062c, 0x144)
void rmgMonsterObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<s32>(outputFile, m_objectId);
    }
    writeValue<s16>(outputFile, m_creatureCount);
    writeValue<u8>(outputFile, m_disposition);
    writeValue<u8>(outputFile, 0);
    writeValue<u8>(outputFile, 0);
    writeValue<u8>(outputFile, 0);
    writeRmgReservedBytes(outputFile, 2);
}

VA(0x005332f0, 0x16a)
MAC_ADDRESS(0x2307d8, 0x20c)
void rmgTownObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<s32>(outputFile, m_objectId);
    }
    writeValue<s8>(outputFile, m_player);
    writeValue<u8>(outputFile, 0);
    writeValue<u8>(outputFile, 0);
    writeValue<u8>(outputFile, 0);
    writeValue<u8>(outputFile, 0);
    writeValue<b8>(outputFile, m_hasFort);
    u8 spells[(hero::NUM_SPELLS + 7) / 8];
    memset(spells, 0, sizeof(spells));
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE)
        outputFile->write(spells, sizeof(spells));
    outputFile->write(spells, sizeof(spells));
    writeValue<s32>(outputFile, 0);
    if (version >= RMG_MAP_SHADOW_OF_DEATH) {
        writeValue<s8>(outputFile, -1);
    }
    writeRmgReservedBytes(outputFile, 3);
}

VA(0x00533460, 0xa0)
MAC_ADDRESS(0x230a1c, 0x78)
void rmgOwnableObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    writeValue<s8>(outputFile, -1); // player
    writeRmgReservedBytes(outputFile, 3);
}

rmgArtifactObject::rmgArtifactObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties)
{
}

rmgSeerHutObject::rmgSeerHutObject(TRmgObjectPropertiesRef* properties)
    : type_object(properties), m_artifact(ARTIFACT_NONE), m_experience(0),
      m_resourceType(GOLD), m_resourceCount(0), m_creatureType(CREATURE_NONE),
      m_creatureCount(0)
{
}

rmgQuestArtifactObject::rmgQuestArtifactObject(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, rmgSeerHutObject* seerHut,
    type_treasure_def* definition)
    : rmgArtifactObject(properties), m_generator(generator),
      m_seerHut(seerHut), m_definition(definition)
{
}

rmgKeyTentObject::rmgKeyTentObject(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, s32 treasureValue)
    : type_object(properties), m_generator(generator), m_treasureValue(treasureValue)
{
}

VA(0x00533500, 0x8a)
MAC_ADDRESS(0x230acc, 0x50)
void rmgArtifactObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    writeValue<b8>(outputFile, 0); // has custom treasure
}

VA_COMPGEN(0x00533590, 0x21, SCALAR_DELETING_DTOR, rmgOwnableObject)

VA(0x005335c0, 0xb2)
MAC_ADDRESS(0x230bb4, 0x78)
void rmgResourceObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    writeValue<b8>(outputFile, 0); // has custom treasure
    writeValue<s32>(outputFile, 0); // amount
    writeValue<s32>(outputFile, 0);
}

VA_COMPGEN(0x00533680, 0x21, SCALAR_DELETING_DTOR, rmgBlackBoxObject)

VA_COMPGEN(0x005336b0, 0x36, IMPLICIT_DTOR, rmgBlackBoxObject)
MAC_COMPGEN_ADDRESS(0x251488, 0x7c, IMPLICIT_DTOR, rmgBlackBoxObject)

static inline void writeRmgCreatureReward(
    TAbstractFile* outputFile, s32 version, TCreatureType creature, s32 count)
{
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE)
        writeValue<s16>(outputFile, creature);
    else
        writeValue<u8>(outputFile, creature);
    writeValue<s16>(outputFile, count);
}

VA(0x005336f0, 0x1e0)
MAC_ADDRESS(0x230cac, 0x32c)
void rmgBlackBoxObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    writeValue<b8>(outputFile, 0); // has custom treasure
    writeValue<s32>(outputFile, m_experience);
    writeValue<s32>(outputFile, 0); // mana
    writeValue<s8>(outputFile, 0); // morale
    writeValue<s8>(outputFile, 0); // luck
    outputFile->write(m_resources, sizeof(m_resources));
    writeValue<s32>(outputFile, 0); // primary skills
    writeValue<u8>(outputFile, 0); // secondary skill count
    writeValue<u8>(outputFile, 0); // artifact count
    writeValue<u8>(outputFile, m_spells.size()); // spell count
    for (u32 spellIndex = 0; spellIndex < m_spells.size(); ++spellIndex) {
        writeValue<u8>(outputFile, m_spells[spellIndex]);
    }
    if (m_creatureType == CREATURE_NONE) {
        writeValue<u8>(outputFile, 0); // creature count
    } else {
        writeValue<u8>(outputFile, 1); // creature count
        writeRmgCreatureReward(outputFile, version, m_creatureType, m_creatureCount);
    }
    writeValue<s32>(outputFile, 0);
    writeValue<s32>(outputFile, 0);
}

VA(0x005338d0, 0x0d)
MAC_ADDRESS(0x230448, 0x58)
type_object::~type_object()
{
    --m_properties->m_refCount;
}

// Removes the object and, if one fits, puts a treasure worth 1-1.5x its value
// in its place.
// removeObject keeps the object alive and its position unchanged.
inline void type_random_map_generator::replaceObjectWithTreasure(type_object* object,
    s32 value)
{
    TRmgMapPosition position = object->m_position;
    removeObject(object);
    TRmgZone* zone = m_zones[
        m_map.getMapItem(position)->m_zoneState.m_zone];
    s32 actualValue;
    // Not a group's first object, no terrain-dependent treasures, no compact
    // selection.
    type_object* replacement = createTreasureObject(
        zone, value, value * 3 / 2, &actualValue, false, false, false, position);
    if (replacement)
        addObject(replacement, position);
}

VA(0x005338e0, 0xd4)
MAC_ADDRESS(0x231030, 0x68)
b8 rmgKeyTentObject::completePlacement()
{
    if (m_generator->placeKeyTentGuard(this, m_treasureValue * 3 / 2))
        return true;
    m_generator->replaceObjectWithTreasure(this, m_treasureValue);
    return false;
}

VA_COMPGEN(0x005339c0, 0x21, SCALAR_DELETING_DTOR, rmgQuestArtifactObject)

// Deletes the pending seer hut if it was never placed.
VA(0x005339f0, 0x58)
MAC_ADDRESS(0x231100, 0x9c)
rmgQuestArtifactObject::~rmgQuestArtifactObject()
{
    delete m_seerHut;
}

// On success the generator places the seer hut and takes ownership; on
// failure the hut is destroyed.
VA(0x00533a50, 0x33)
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
VA(0x00533a90, 0x1e0)
MAC_ADDRESS(0x2312d0, 0x31c)
void rmgSeerHutObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<u8>(outputFile, QUEST_ARTIFACTS);
        writeValue<u8>(outputFile, 1); // artifact count
        writeValue<s16>(outputFile, m_artifact);
        writeValue<s32>(outputFile, -1); // deadline
        writeValue<s32>(outputFile, 0); // first visit length
        writeValue<s32>(outputFile, 0); // next visit length
        writeValue<s32>(outputFile, 0); // completion length
    } else {
        writeValue<u8>(outputFile, m_artifact);
    }
    if (m_experience > 0) {
        writeValue<u8>(outputFile, eRewardExperience);
        writeValue<s32>(outputFile, m_experience);
    } else if (m_creatureType != CREATURE_NONE) {
        writeValue<u8>(outputFile, eRewardCreature);
        writeRmgCreatureReward(outputFile, version, m_creatureType, m_creatureCount);
    } else {
        writeValue<u8>(outputFile, eRewardResource);
        writeValue<u8>(outputFile, m_resourceType);
        writeValue<s32>(outputFile, m_resourceCount);
    }
    writeRmgReservedBytes(outputFile, 2);
}

// The factory reserves the hero in m_disabledHeroes; releaseReservation
// frees it again.
rmgHeroObject::rmgHeroObject(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, s32 objectId, s32 heroIndex,
    s32 experience)
    : type_object(properties), m_generator(generator), m_objectId(objectId),
      m_heroIndex(heroIndex), m_experience(experience)
{
}

VA(0x00533c70, 0x0f)
MAC_ADDRESS(0x231644, 0x18)
void rmgHeroObject::releaseReservation()
{
    m_generator->m_disabledHeroes[m_heroIndex] = false;
}

VA(0x00533c80, 0x1e4)
MAC_ADDRESS(0x23165c, 0x318)
void rmgHeroObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<s32>(outputFile, m_objectId);
    }
    writeValue<s8>(outputFile, -1); // owner
    writeValue<u8>(outputFile, m_heroIndex);
    writeValue<b8>(outputFile, 0); // custom name
    if (version >= RMG_MAP_SHADOW_OF_DEATH) {
        writeValue<b8>(outputFile, m_experience != 0); // custom experience
        if (m_experience != 0) {
            writeValue<s32>(outputFile, m_experience);
        }
    } else {
        writeValue<s32>(outputFile, m_experience);
    }
    writeValue<b8>(outputFile, 0); // custom portrait
    writeValue<b8>(outputFile, 0); // custom secondary skills
    writeValue<b8>(outputFile, 0); // custom armies
    writeValue<u8>(outputFile, 0); // group formation
    writeValue<b8>(outputFile, 0); // custom artifacts
    writeValue<s8>(outputFile, -1); // patrol radius
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<b8>(outputFile, 0); // custom biography
        writeValue<s8>(outputFile, -1); // sex
        if (version >= RMG_MAP_SHADOW_OF_DEATH) {
            writeValue<b8>(outputFile, 0); // custom spells
            writeValue<b8>(outputFile, 0); // custom primary skills
        } else {
            writeValue<s8>(outputFile, -2); // spell
        }
    }
    writeRmgReservedBytes(outputFile, 16);
}

VA(0x00533e70, 0xc3)
MAC_ADDRESS(0x2319ac, 0xbc)
void rmgScholarObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    writeValue<s8>(outputFile, -1); // ScholarAwards; -1 picks one at random
    writeValue<u8>(outputFile, 0); // award value
    writeValue<s32>(outputFile, 0);
    writeRmgReservedBytes(outputFile, 2);
}

VA(0x00533f40, 0xaf)
MAC_ADDRESS(0x231aa0, 0x9c)
void rmgShrineObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    writeValue<s8>(outputFile, SPELL_NONE);
    writeRmgReservedBytes(outputFile, 2);
    writeValue<u8>(outputFile, 0); // reserved byte
}

VA(0x00533ff0, 0xc2)
MAC_ADDRESS(0x231b84, 0xc8)
void rmgSpellScrollObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    writeValue<b8>(outputFile, 0); // message
    writeValue<u8>(outputFile, m_spell);
    writeRmgReservedBytes(outputFile, 2);
    writeValue<u8>(outputFile, 0);
}

// Witch hut skill mask: the first 16 secondary skills except Navigation and
// Necromancy.
enum ERmgWitchHutSkills {
    RMG_WITCH_HUT_ALLOWED_SKILLS = 0xffff
        & ~((1 << eSecSkillNavigation) | (1 << eSecSkillNecromancy))
};

VA(0x005340c0, 0x93)
MAC_ADDRESS(0x231c84, 0x68)
void rmgWitchHutObject::write(TAbstractFile* outputFile, s32 version)
{
    type_object::write(outputFile, version);
    if (version >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<u32>(outputFile, RMG_WITCH_HUT_ALLOWED_SKILLS);
    }
}

VA(0x00534160, 0x27)
MAC_ADDRESS(0x231cec, 0x1c)
type_treasure_def::type_treasure_def(
    s32 objectType, s32 subtype, s32 value, s32 density)
{
    m_objectType = objectType;
    m_subtype = subtype;
    m_value = value;
    m_density = density;
}

VA(0x00534190, 0x06)
MAC_ADDRESS(0x231d10, 0x8)
s32 type_treasure_def::getValue(TRmgZone*, type_random_map_generator*)
{
    return m_value;
}

VA(0x005341a0, 0x4d)
MAC_ADDRESS(0x231d18, 0x50)
type_object* type_treasure_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new type_object(properties);
}

VA(0x005341f0, 0x53)
MAC_ADDRESS(0x231dac, 0x50)
type_object* type_artifact_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgArtifactObject(properties);
}

// Rounds a positive count to the nearest multiple of step, ties upward.
static inline s32 roundRmgCreatureCount(s32 count, s32 step)
{
    return ((count + step / 2) / step) * step;
}

VA(0x00534250, 0xb5)
MAC_ADDRESS(0x231dfc, 0x108)
type_black_box_creature_def::type_black_box_creature_def(s32 creatureType)
    : type_treasure_def(BLACK_BOX, 0, -1, 3),
      m_creatureType(static_cast<TCreatureType>(creatureType))
{
    m_creatureCount =
        g_rmgCreatureValueByLevel[g_creatureTypeTraits[creatureType].m_level]
        / g_creatureTypeTraits[creatureType].m_aiValue;

    if (m_creatureCount > 50)
        m_creatureCount = roundRmgCreatureCount(m_creatureCount, 10);
    else if (m_creatureCount > 12)
        m_creatureCount = roundRmgCreatureCount(m_creatureCount, 5);
    else if (m_creatureCount > 5)
        m_creatureCount = roundRmgCreatureCount(m_creatureCount, 2);
}

// Raises a value by the share of town zones with the same alignment.
static inline s32 adjustRmgValueForAlignment(s32 value,
    s32 alignedTownZoneCount, s32 townZoneCount)
{
    if (townZoneCount > 0)
        value += alignedTownZoneCount * value / townZoneCount;
    return value;
}

// Town zones of an alignment; none for eTownNeutral.
inline s32 type_random_map_generator::getTownZoneCount(s32 alignment) const
{
    if (alignment == eTownNeutral)
        return 0;
    return m_townZoneCountsByAlignment[alignment];
}

// getValue result for a treasure this zone or moment cannot offer.
enum ERmgTreasureOffer {
    RMG_TREASURE_NOT_OFFERED = -1
};

// Creature rewards are offered only in zones of the creature's town; the
// value rises with that town's share of town zones.
VA(0x00534310, 0x64)
MAC_ADDRESS(0x231f04, 0x6c)
s32 type_black_box_creature_def::getValue(
    TRmgZone* zone, type_random_map_generator* generator)
{
    s32 alignment = g_creatureTypeTraits[m_creatureType].m_townType;
    if (alignment != zone->m_creatureTownType)
        return RMG_TREASURE_NOT_OFFERED;
    s32 value = g_creatureTypeTraits[m_creatureType].m_aiValue * m_creatureCount;
    return adjustRmgValueForAlignment(value,
        generator->getTownZoneCount(alignment), generator->m_townZoneCount);
}

VA(0x00534380, 0x85)
MAC_ADDRESS(0x231f70, 0x6c)
type_object* type_black_box_creature_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    rmgBlackBoxObject* object = new rmgBlackBoxObject(properties);
    object->m_creatureType = m_creatureType;
    object->m_creatureCount = m_creatureCount;
    return object;
}

VA(0x00534410, 0x7f)
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

static inline bool isRmgSpellDisabledByDefault(s32 spell)
{
    return (g_spellTraits[spell].m_flags & RMG_SPELL_DISABLED_BY_DEFAULT) != 0;
}

VA(0x00534520, 0x267)
MAC_ADDRESS(0x2321ec, 0xc0)
type_object* type_black_box_spells_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    rmgBlackBoxObject* object = new rmgBlackBoxObject(properties);
    for (s32 level = m_maximumLevel; level >= m_minimumLevel; --level) {
        for (s32 spell = SPELL_SUMMON_BOAT; spell < hero::NUM_SPELLS; ++spell) {
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

VA(0x005347f0, 0x79)
MAC_ADDRESS(0x23237c, 0x88)
s32 type_map_dwelling_def::getValue(TRmgZone* zone, type_random_map_generator* generator)
{
    const TCreatureTypeTraits& creature =
        g_creatureTypeTraits[g_creatureGenerator1Types[m_subtype]];
    if (creature.m_townType != zone->m_creatureTownType)
        return RMG_TREASURE_NOT_OFFERED;

    s32 value = creature.m_growthRate * creature.m_aiValue;
    s32 alignedTownZoneCount = generator->getTownZoneCount(creature.m_townType);
    value = adjustRmgValueForAlignment(value, alignedTownZoneCount,
        generator->m_townZoneCount);
    return value + creature.m_aiValue * alignedTownZoneCount / 2;
}

VA(0x00534870, 0x53)
MAC_ADDRESS(0x23243c, 0x50)
type_object* type_resource_lump_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgResourceObject(properties);
}

// Reserves a random available hero; no prison when none is left.
VA(0x005348d0, 0x93)
MAC_ADDRESS(0x2324e4, 0x84)
type_object* type_prison_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    s32 heroIndex = generator->selectPrisonHero();
    if (heroIndex == heroIdNone)
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

VA(0x005349d0, 0x29)
MAC_ADDRESS(0x232600, 0x44)
type_shrine_def::type_shrine_def(s32 objectType, s32 value)
    : type_treasure_def(objectType, 0, value, 100)
{
}

VA(0x00534a00, 0x53)
MAC_ADDRESS(0x232644, 0x50)
type_object* type_shrine_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgShrineObject(properties);
}

VA(0x00534a60, 0x25)
MAC_ADDRESS(0x232694, 0x48)
type_witch_hut_def::type_witch_hut_def()
    : type_treasure_def(WITCH_HUT, 0, 1500, 80)
{
}

VA(0x00534a90, 0x53)
MAC_ADDRESS(0x2326dc, 0x50)
type_object* type_witch_hut_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    return new rmgWitchHutObject(properties);
}

inline bool type_random_map_generator::canUseSeerHutPrototype(s32 prototypeIndex) const
{
    return m_nextSeerHutPrototypeIndex == prototypeIndex
        && !m_questArtifactPoolLow;
}

inline rmgQuestArtifactObject* type_random_map_generator::createQuestArtifactForHut(
    rmgSeerHutObject* seerHut, type_treasure_def* definition)
{
    TRmgObjectPropertiesRef* artifact = selectObjectPrototype(
        eTerrainDirt, RANDOM_ARTIFACT, 0);
    return new rmgQuestArtifactObject(artifact, this, seerHut, definition);
}

VA(0x00534af0, 0x9e)
MAC_ADDRESS(0x23277c, 0x68)
s32 type_quest_creature_def::getValue(TRmgZone* zone, type_random_map_generator* generator)
{
    if (!generator->canUseSeerHutPrototype(m_subtype))
        return RMG_TREASURE_NOT_OFFERED;
    s32 value = type_black_box_creature_def::getValue(zone, generator);
    return (2 * value - 4000) / 3;
}

VA(0x00534b90, 0xe7)
MAC_ADDRESS(0x2327ec, 0xa4)
type_object* type_quest_creature_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    rmgSeerHutObject* seerHut = new rmgSeerHutObject(properties);
    rmgQuestArtifactObject* object = generator->createQuestArtifactForHut(seerHut, this);
    seerHut->m_creatureType = m_creatureType;
    seerHut->m_creatureCount = m_creatureCount;
    return object;
}

// Seer huts are offered only for the current seer-hut prototype and while
// enough quest artifacts remain.
VA(0x00534c80, 0x34)
MAC_ADDRESS(0x232900, 0x34)
s32 type_quest_experience_def::getValue(TRmgZone*, type_random_map_generator* generator)
{
    if (!generator->canUseSeerHutPrototype(m_subtype))
        return RMG_TREASURE_NOT_OFFERED;
    return m_value;
}

s32 type_quest_gold_def::getValue(TRmgZone*, type_random_map_generator* generator)
{
    if (!generator->canUseSeerHutPrototype(m_subtype))
        return RMG_TREASURE_NOT_OFFERED;
    return m_value;
}

VA(0x00534cc0, 0xe1)
MAC_ADDRESS(0x23293c, 0x9c)
type_object* type_quest_experience_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    rmgSeerHutObject* seerHut = new rmgSeerHutObject(properties);
    rmgQuestArtifactObject* object = generator->createQuestArtifactForHut(seerHut, this);
    seerHut->m_experience = m_experience;
    return object;
}

VA(0x00534db0, 0xe8)
MAC_ADDRESS(0x232a84, 0xa4)
type_object* type_quest_gold_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    rmgSeerHutObject* seerHut = new rmgSeerHutObject(properties);
    rmgQuestArtifactObject* object = generator->createQuestArtifactForHut(seerHut, this);
    seerHut->m_resourceType = GOLD;
    seerHut->m_resourceCount = m_gold;
    return object;
}

VA(0x00534ea0, 0x30)
MAC_ADDRESS(0x232b28, 0x58)
type_spell_scroll_def::type_spell_scroll_def(s32 spellLevel, s32 value)
    : type_treasure_def(SPELL_SCROLL, 0, value, 30)
{
    m_spellLevel = spellLevel;
}

static inline bool isRmgScrollSpell(s32 spell, s32 level)
{
    return !isRmgSpellDisabledByDefault(spell)
        && g_spellTraits[spell].m_schoolBits
        && g_spellTraits[spell].m_level == level;
}

VA(0x00534ed0, 0xc3)
MAC_ADDRESS(0x232b80, 0x108)
type_object* type_spell_scroll_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator*, TRmgZone*)
{
    // One random draw picks among eligible spells in spell order; at least
    // one eligible spell must exist.
    s32 eligibleSpellCount = 0;
    s32 spell;
    for (spell = SPELL_SUMMON_BOAT; spell < hero::NUM_SPELLS; ++spell) {
        if (isRmgScrollSpell(spell, m_spellLevel))
            ++eligibleSpellCount;
    }
    s32 selectedIndex = rand() % eligibleSpellCount;
    for (spell = SPELL_SUMMON_BOAT; spell < hero::NUM_SPELLS; ++spell) {
        if (isRmgScrollSpell(spell, m_spellLevel) && selectedIndex-- <= 0)
            break;
    }
    return new rmgSpellScrollObject(properties, static_cast<ESpellId>(spell));
}

VA(0x00534fa0, 0x21)
MAC_ADDRESS(0x232cd4, 0x20)
s32 type_key_tent_def::getValue(TRmgZone*, type_random_map_generator* generator)
{
    if (generator->m_nextKeyTentColor != m_subtype)
        return RMG_TREASURE_NOT_OFFERED;
    return m_value;
}

// The tent stores its treasure value; its colour is the prototype subtype.
VA(0x00534fd0, 0x64)
MAC_ADDRESS(0x232cfc, 0x70)
type_object* type_key_tent_def::generate(TRmgObjectPropertiesRef* properties,
    type_random_map_generator* generator, TRmgZone*)
{
    return new rmgKeyTentObject(properties, generator, m_value);
}

// Empties the group; its objects are owned elsewhere.
VA(0x00535040, 0xc6)
MAC_ADDRESS(0x232e70, 0xa8)
void TRmgTreasureGroup::reset()
{
    m_objects.clear();
    m_outline.clear();
    m_map.clear();
    m_hasGuard = false;
    m_outlineMarked = false;
    TRmgMapItem* item = m_map.getMapItem(0, 0);
    s32 count = m_map.m_mapWidth * m_map.m_mapHeight;
    while (count--) {
        item->setTerrain(eTerrainDirt, 0, false, false);
        ++item;
    }
}

// Flags the group's outline cells on its map.
MAC_ADDRESS(0x232fc4, 0x64)
void TRmgTreasureGroup::markPlacementOutline()
{
    m_outlineMarked = true;
    for (u32 index = 0; index < m_outline.size(); ++index)
        m_map.getMapItem(m_outline[index].m_x,
            m_outline[index].m_y)->m_tileData.m_placementOutline = true;
}

MAC_ADDRESS(0x233d18, 0x60)
b8 TRmgTreasureGroup::objectsAllowEntrances() const
{
    for (u32 index = 0; index < m_objects.size(); ++index) {
        TAdventureObjectType objectType =
            m_objects[index]->m_properties->m_prototype->getObjectType();
        if (!g_adventureObjectTraits[objectType].m_clearedOnVisit)
            return false;
    }
    return true;
}

MAC_ADDRESS(0x233d78, 0xd4)
void TRmgTreasureGroup::addObject(type_object* object, TPoint point)
{
    m_objects.push_back(object);
    m_map.addObject(*object, TRmgMapPosition(point.m_x, point.m_y, RMG_SURFACE_LEVEL));
}

// Rings the objects' entrances with cells marked for obstacles, places the
// guard on a random fitting marked cell of the outline (failing if none),
// opens paths around it and retraces the outline.
VA(0x00535110, 0x4ab)
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
        TRmgMapPosition entrance = object->getEntrance();
        u32 direction = isRmgEntranceOpenToNorth(prototype->getObjectType())
            ? RMG_DIRECTION_COUNT : RMG_FIRST_NORTHERN_DIRECTION;
        while (direction--) {
            TRmgMapPosition position = entrance + g_rmgDirections[direction];
            TRmgMapItem* item = m_map.getMapItem(position);
            if (item->isObjectEntrance() || !item->isPassableLand())
                continue;
            item->markObstacleFill();
            for (s32 x = position.m_x - 1; x <= position.m_x + 1; ++x) {
                for (s32 y = position.m_y - 1; y <= position.m_y + 1; ++y) {
                    TRmgMapItem* nearby = m_map.getMapItem(x, y);
                    if (nearby->isPassableLand()
                        && !nearby->isObjectEntrance())
                        nearby->releasePathClearance();
                }
            }
        }
    }
    TRmgObjectPropertiesRef* guardProperties = guard->m_properties;
    for (u32 index = m_outline.size(); index--;) {
        TPoint point = m_outline[index];
        TRmgMapPosition position(point.m_x, point.m_y, RMG_SURFACE_LEVEL);
        if (!m_map.getMapItem(point.m_x, point.m_y)->hasObstacleFill()
            || !canFitObject(guardProperties, position))
            m_outline.erase(m_outline.begin() + index);
    }
    if (!m_outline.size())
        return false;
    TPoint guardPosition = m_outline[rand() % m_outline.size()];
    addObject(guard, guardPosition);
    TPoint guardEntrance(guardPosition.m_x - prototype->m_triggerCell.m_x,
        guardPosition.m_y - prototype->m_triggerCell.m_y);
    TAdventureObjectType guardType = guardProperties->m_prototype->getObjectType();
    for (s32 direction = RMG_DIRECTION_EAST; direction < RMG_DIRECTION_COUNT; ++direction) {
        TPoint point = g_rmgDirections[direction]
            + TRmgVector(guardEntrance.m_x, guardEntrance.m_y);
        TRmgMapItem* item = m_map.getMapItem(point.m_x, point.m_y);
        if (!item->isPassableLand())
            continue;
        if (guardType == BORDER_GUARD && item->hasObstacleFill())
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
        s32 fanDirection;
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
                if (!next->hasObstacleFill() && !next->hasPathClearance()
                    && next->isPassableLand()) {
                    next->openPath();
                }
            }
            fanDirection = turnRmgDirection(fanDirection, 1);
        }
    }
    m_guardPosition = guardEntrance;
    m_hasGuard = true;
    m_outline.clear();
    updateBounds();
    traceOutline();
    return true;
}

VA(0x005355c0, 0x1a)
TRmgMapPosition::TRmgMapPosition(s32 newX, s32 newY, s32 newZ)
    : TPoint(newX, newY), m_z(newZ)
{
}

// Whether an object fits on the group map without blocking neighbouring
// entrances. Other objects' entrances avoid cells marked for obstacles;
// guards may stand on them but need a free neighbouring cell.
VA(0x005355e0, 0x1f9)
MAC_ADDRESS(0x2336d0, 0x374)
b8 TRmgTreasureGroup::canFitObject(TRmgObjectPropertiesRef* properties,
    TRmgMapPosition position)
{
    TObjectType* prototype = properties->m_prototype;
    TAdventureObjectType objectType = prototype->getObjectType();
    TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, prototype->m_triggerCell);
    TRmgVector origin(entrance.m_x, entrance.m_y);
    if (!isRmgEntranceOpenToNorth(objectType)) {
        for (s32 direction = RMG_FIRST_NORTHERN_DIRECTION; direction < RMG_DIRECTION_COUNT; ++direction) {
            TPoint nearby = g_rmgDirections[direction] + origin;
            if (m_map.getMapItem(nearby.m_x, nearby.m_y)->isObjectEntrance())
                return false;
        }
    }
    for (s32 direction = RMG_DIRECTION_EAST; direction < RMG_FIRST_NORTHERN_DIRECTION; ++direction) {
        TPoint nearby = g_rmgDirections[direction] + origin;
        TRmgMapItem* item = m_map.getMapItem(nearby.m_x, nearby.m_y);
        if (item->isObjectEntrance()
            && !allowsRmgSharedObjectEntrance(item->getEntranceObjectType()))
            return false;
    }
    // Group-map cells belong to no zone.
    if (objectType != MONSTER && objectType != BORDER_GUARD)
        return !m_map.isPlacementBlocked(properties, position, RMG_NO_ZONE,
            RMG_REJECT_OBSTACLE_ENTRANCES);
    if (m_map.isPlacementBlocked(properties, position, RMG_NO_ZONE,
            RMG_ALLOW_OBSTACLE_ENTRANCES))
        return false;
    for (s32 neighbour = RMG_DIRECTION_EAST; neighbour < RMG_DIRECTION_COUNT; ++neighbour) {
        TPoint nearby = g_rmgDirections[neighbour] + origin;
        TRmgMapItem* item = m_map.getMapItem(nearby.m_x, nearby.m_y);
        if (!item->isObjectEntrance() && item->isPassableLand()
            && !item->hasObstacleFill())
            return true;
    }
    return false;
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
    TPoint trigger(prototype->m_triggerCell.m_x, prototype->m_triggerCell.m_y);
    std::vector<TRmgMapPosition> candidates;
    for (u32 index = 0; index < m_objects.size(); ++index) {
        type_object* existing = m_objects[index];
        TRmgMapPosition entrance = existing->getEntrance();
        b8 openToNorth = isRmgEntranceOpenToNorth(
            existing->m_properties->m_prototype->getObjectType());
        for (s32 direction = RMG_DIRECTION_COUNT; direction--; ) {
            if (!openToNorth && !isRmgSouthwardDirection(direction))
                continue;
            TRmgMapPosition candidate = entrance + g_rmgDirections[direction] + trigger;
            if (bounds.contains(candidate) && canFitObject(properties, candidate))
                candidates.push_back(candidate);
        }
    }
    if (!candidates.size())
        return false;
    addObject(object, candidates[rand() % candidates.size()]);
    return true;
}

VA(0x00535df0, 0xea)
MAC_ADDRESS(0x233e4c, 0x16c)
void TRmgTreasureGroup::updateBounds()
{
    m_bounds.resetEmpty();
    TRmgMapItem* item = m_map.m_mapItems;
    for (s32 y = 0; y < m_map.m_mapHeight; ++y) {
        for (s32 x = 0; x < m_map.m_mapWidth; ++x, ++item) {
            if (!item->isClearOutlineCell()) {
                m_bounds.includeCell(x, y);
            }
        }
    }
}

// Traces the closed outline around the occupied cells, starting above the
// first occupied cell in row-major order.
VA(0x00535ee0, 0x18f)
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
    s32 direction = RMG_DIRECTION_SOUTH;
    do {
        m_outline.push_back(position);
        s32 attempts = 0;
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

VA(0x00536070, 0xfb)
MAC_ADDRESS(0x23422c, 0xec)
TRmgGeneratorBase::TRmgGeneratorBase(s32 width, s32 height, s32 levels,
    TProgressSink* progress, s32 additionalSteps, s32 version)
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

VA(0x005361a0, 0x07)
MAC_ADDRESS(0x22d34c, 0x48)
TRmgMapInterface::~TRmgMapInterface()
{
}

VA_COMPGEN(0x005361b0, 0x23, SCALAR_DELETING_DTOR, TRmgMapInterface)

VA_COMPGEN(0x005361e0, 0x18, DEFAULT_CTOR_CLOSURE, TRmgObjectPropertiesRef)

// Overlap priorities stay uninitialized until first built.
TRmgObjectPropertiesRef::TRmgObjectPropertiesRef(TObjectType* prototype)
{
    m_prototype = prototype;
    m_placementRule = 0;
    m_refCount = 0;
    m_prototypeIndex = 0;
    m_preferredTerrain = TERRAIN_NONE;
    m_prioritiesInitialized = false;
}

static inline bool isRmgObjectAvailableInVersion(
    TAdventureObjectType objectType, s32 version)
{
    if (version < RMG_MAP_SHADOW_OF_DEATH && objectType >= CLOVER_FIELD_2)
        return false;
    if (version < RMG_MAP_ARMAGEDDONS_BLADE && objectType >= MAX_EVENT_TYPE)
        return false;
    return true;
}

// Monolith subtypes, per kind, that maps before Shadow of Death may use.
enum ERmgMonolithSubtypeLimits {
    RMG_PRE_SOD_MONOLITH_SUBTYPE_COUNT = 3
};

VA(0x00536200, 0x1ac)
MAC_ADDRESS(0x234440, 0x24c)
void TRmgGeneratorBase::loadObjectPrototypes()
{
    m_objectsTxt.load("objects.txt");
    for (u32 index = 0; index < m_objectsTxt.m_objectTypes.size(); ++index) {
        TAdventureObjectType type = m_objectsTxt.m_objectTypes[index].getObjectType();
        if (!isRmgObjectAvailableInVersion(type, m_mapVersion))
            continue;
        if (m_mapVersion < RMG_MAP_SHADOW_OF_DEATH && (type == LITH_TWOWAY || type == LITH_ONEWAY_ENTRANCE || type == LITH_ONEWAY_EXIT)
            && m_objectsTxt.m_objectTypes[index].getSubtype() >= RMG_PRE_SOD_MONOLITH_SUBTYPE_COUNT)
            continue;
        if (type < 0 || type >= ADVENTURE_OBJECT_TRAIT_COUNT)
            continue;
        TRmgObjectPropertiesRef* properties =
            new TRmgObjectPropertiesRef(&m_objectsTxt.m_objectTypes[index]);
        // Aliased object types are listed under their objnames.txt row.
        s32 mappedType;
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

VA(0x005363b0, 0x1a9)
MAC_ADDRESS(0x2346a0, 0x18c)
TRmgGeneratorBase::~TRmgGeneratorBase()
{
    for (u32 object = 0; object < m_objects.size(); ++object)
        delete m_objects[object];
    for (s32 type = NOTHING; type < ADVENTURE_OBJECT_TRAIT_COUNT; ++type)
        for (u32 prototype = 0; prototype < m_objectPrototypes[type].size(); ++prototype)
            delete m_objectPrototypes[type][prototype];
}

static inline void readRmgPlacementScores(std::vector<s32>& scores,
    const TSpreadsheetResource::TStringVector& fields, s32 precedingScores, s32 count)
{
    scores.resize(count, 0);
    for (s32 index = 0; index < count; ++index)
        scores[index] = atoi(
            fields[index + precedingScores + RMG_PLACEMENT_COLUMN_NEIGHBOUR_SCORES]);
}

// Reads rand_trn.txt and binds each prototype to the last rule matching its
// object type, subtype and first recommended terrain.
VA(0x00536560, 0x5f2)
MAC_ADDRESS(0x23486c, 0x6a0)
void TRmgGeneratorBase::readObjectPlacementRules()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x006827f4, rmgPlacementRulesFilename, "rand_trn.txt"));
    s32 row = RMG_FIRST_DATA_ROW;
    std::vector<TAdventureObjectType> objectTypes;
    std::vector<TTerrainType> terrains;
    std::vector<s32> subtypes;
    while (row < sheet->getNumberOfRows()) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        if (values[0][0] == ' ' || values[0][0] == 0)
            break;
        TRmgObjectPlacementRule rule;
        rule.m_index = row - RMG_FIRST_DATA_ROW;
        TAdventureObjectType ruleObjectType =
            H3_ENUM_DECODE(TAdventureObjectType,
                atoi(values[RMG_PLACEMENT_COLUMN_OBJECT_TYPE]));
        s32 ruleSubtype = atoi(values[RMG_PLACEMENT_COLUMN_SUBTYPE]);
        TTerrainType ruleTerrain = H3_ENUM_DECODE(TTerrainType,
            atoi(values[RMG_PLACEMENT_COLUMN_TERRAIN]));
        objectTypes.push_back(ruleObjectType);
        terrains.push_back(ruleTerrain);
        subtypes.push_back(ruleSubtype);
        TTerrainType terrain;
        for (terrain = eTerrainDirt; terrain <= eTerrainWater;
             terrain = H3_ENUM_DECODE(TTerrainType, terrain + 1))
            rule.m_terrainScores[terrain] =
                atoi(values[terrain + RMG_PLACEMENT_COLUMN_TERRAIN_SCORES]);
        for (; terrain < RMG_TERRAIN_COUNT;
             terrain = H3_ENUM_DECODE(TTerrainType, terrain + 1))
            rule.m_terrainScores[terrain] = RMG_PLACEMENT_INVALID;
        m_placementRules.push_back(rule);
        ++row;
    }
    s32 ruleCount = m_placementRules.size();
    for (row = RMG_FIRST_DATA_ROW; row < ruleCount + RMG_FIRST_DATA_ROW; ++row) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
        TRmgObjectPlacementRule& rule = m_placementRules[row - RMG_FIRST_DATA_ROW];
        readRmgPlacementScores(rule.m_adjacentScores, values, 0, ruleCount);
        readRmgPlacementScores(rule.m_blockedScores, values, ruleCount, ruleCount);
    }
    sheet->dispose();

#if defined(HOMM3_TARGET_MAC)
    struct TPlacementTables {
        std::vector<TRmgObjectPlacementRule*>
            rulesByType[ADVENTURE_OBJECT_TRAIT_COUNT][RMG_TERRAIN_COUNT];
        std::vector<s32> subtypesByType[ADVENTURE_OBJECT_TRAIT_COUNT][RMG_TERRAIN_COUNT];
    };
    TPlacementTables* tables = new TPlacementTables;
    std::vector<TRmgObjectPlacementRule*>
        (&rulesByType)[ADVENTURE_OBJECT_TRAIT_COUNT][RMG_TERRAIN_COUNT] = tables->rulesByType;
    std::vector<s32> (&subtypesByType)[ADVENTURE_OBJECT_TRAIT_COUNT][RMG_TERRAIN_COUNT] =
        tables->subtypesByType;
#else
    std::vector<TRmgObjectPlacementRule*>
        rulesByType[ADVENTURE_OBJECT_TRAIT_COUNT][RMG_TERRAIN_COUNT];
    std::vector<s32> subtypesByType[ADVENTURE_OBJECT_TRAIT_COUNT][RMG_TERRAIN_COUNT];
#endif
    for (s32 index = 0; index < ruleCount; ++index) {
        TRmgObjectPlacementRule* rule = &m_placementRules[index];
        rulesByType[objectTypes[index]][terrains[index]].push_back(rule);
        subtypesByType[objectTypes[index]][terrains[index]].push_back(subtypes[index]);
    }
    for (TAdventureObjectType objectType = NOTHING; objectType < ADVENTURE_OBJECT_TRAIT_COUNT;
         objectType = H3_ENUM_DECODE(TAdventureObjectType, objectType + 1)) {
        for (s32 index = 0; index < m_objectPrototypes[objectType].size();
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
                s32 subtype = prototype->getSubtype();
                s32 mappedType;
                // Aliased object types use their objnames.txt row.
                memcpy(&mappedType, &g_adventureObjectTraits[objectType].m_nameRow,
                       sizeof(mappedType));
                s32 match = rulesByType[mappedType][terrain].size();
                while (match--) {
                    if (subtypesByType[mappedType][terrain][match] == subtype)
                        break;
                }
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
VA(0x00536bc0, 0x5f4)
MAC_ADDRESS(0x23515c, 0x7a0)
s32 TRmgGeneratorBase::scoreObjectPlacement(
    TRmgObjectPropertiesRef* properties, TRmgMapPosition position)
{
    TObjectType* prototype = properties->m_prototype;
    std::vector<type_object*> affected;
    b8 terrainSeen[RMG_TERRAIN_COUNT];
    memset(terrainSeen, 0, sizeof(terrainSeen));
    // The 8x6 footprint plus a one-cell border; footprint cell (column, row)
    // is marks[column + 1][row + 1].
    // marks[c][r] lies at P + (1 - c, 1 - r): c grows west, r north. North is
    // up; a 3x2 footprint #, its position P (marks[1][1]) and the border o:
    //   c: 4 3 2 1 0
    //      o o o o o  r = 3
    //      o # # # o  r = 2
    //      o # # P o  r = 1
    //      o o o o o  r = 0
    u32 marks[8 + 2][6 + 2];
    memset(marks, 0, sizeof(marks));
    for (u32 row = 0; row < prototype->getHeight(); ++row) {
        s32 y = position.m_y - row;
        if (y < 0 || y >= m_map.m_mapHeight)
            continue;
        for (u32 column = 0; column < prototype->getWidth(); ++column) {
            s32 x = position.m_x - column;
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

                // Retail bug: overwrites the OVERLAP/BLOCKED marks just set, so
                // rand_trn.txt's blocked scores are never used.
                marks[column + 1][row + 1] = RMG_PLACEMENT_ADJACENT;
                terrainSeen[item->getLandType()] = true;
                s32 firstRow = position.m_y - min(y + 1, m_map.m_mapHeight) + 1;
                s32 lastRow = position.m_y - max(y - 2, 0) + 1;
                s32 firstColumn = position.m_x - min(x + 1, m_map.m_mapWidth) + 1;
                s32 lastColumn = position.m_x - max(x - 2, 0) + 1;
                for (s32 nearColumn = firstColumn; nearColumn < lastColumn;
                     ++nearColumn) {
                    for (s32 nearRow = firstRow; nearRow < lastRow; ++nearRow)
                        marks[nearColumn][nearRow] |= RMG_PLACEMENT_ADJACENT;
                }
            }
        }
    }

    TRmgObjectPlacementRule* rule = properties->m_placementRule;
    s32 score = 0;
    b8 hasPositiveTerrain = false;
    for (s32 terrain = eTerrainDirt; terrain < RMG_TERRAIN_COUNT; ++terrain) {
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
        s32 y = position.m_y + 1 - row;
        if (y < 0 || y >= m_map.m_mapHeight)
            continue;
        for (u32 column = 0; column < prototype->getWidth() + 2;
             ++column) {
            s32 x = position.m_x + 1 - column;
            if (x < 0 || x >= m_map.m_mapWidth)
                continue;
            u32 mark = marks[column][row];
            if (!mark)
                continue;
            TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
            if (item->isPassableLand())
                continue;
            s32 priority;
            // Only interior cells receive OVERLAP above (column+1, row+1),
            // so these subtractions cannot underflow for an overlap cell.
            if (mark & RMG_PLACEMENT_OVERLAP)
                priority = properties->m_overlapPriorities[column - 1][row - 1];
            for (s32 index = 0; index < static_cast<s32>(item->m_objects.size());
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

VA(0x005371c0, 0x1da)
MAC_ADDRESS(0x2358fc, 0xb8)
void TRmgGeneratorBase::addObject(type_object* object, TRmgMapPosition position)
{
    m_map.addObject(*object, position);
    m_objects.push_back(object);
}

// Expansion decoration object types; the shared object type enum has no
// enumerators for them.
enum ERmgExpansionDecorationType {
    RMG_OBJECT_DESERT_HILLS        = 206,
    RMG_OBJECT_DIRT_HILLS          = 207,
    RMG_OBJECT_GRASS_HILLS         = 208,
    RMG_OBJECT_ROUGH_HILLS         = 209,
    RMG_OBJECT_SUBTERRANEAN_ROCKS  = 210,
    RMG_OBJECT_SWAMP_FOLIAGE       = 211
};

// Decoration (obstacle) object types, excluding holes, rivers and roads.
// The last six are expansion types, unavailable on RoE maps.
DATA(0x006408ec)
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
    TERRAIN_YUCCA_TREE, TERRAIN_REEF, RMG_OBJECT_DESERT_HILLS,
    RMG_OBJECT_DIRT_HILLS, RMG_OBJECT_GRASS_HILLS, RMG_OBJECT_ROUGH_HILLS,
    RMG_OBJECT_SUBTERRANEAN_ROCKS, RMG_OBJECT_SWAMP_FOLIAGE
};

// Fills obstacles outward from a cell with weighted random decorations.
VA(0x005373a0, 0x53d)
MAC_ADDRESS(0x2359b4, 0x76c)
void TRmgGeneratorBase::decorateMapCell(TRmgMapPosition start, s32 progressSteps)
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
        s32 terrain = item->getLandType();
        std::vector<TRmgObjectPropertiesRef*> candidates;
        std::vector<TRmgMapPosition> positions;
        std::vector<s32> weights;
        std::vector<type_object*> unusedObjects;
        s32 totalWeight = 0;
        for (const s32* type = g_rmgDecorationTypes;
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
                // Candidate anchors put the start cell S under each blocked
                // footprint cell in turn. For a fully blocked 3x2 object they
                // are S and each o; North is up:
                //   S o o
                //   o o o
                for (s32 row = 0; row < prototype->getHeight(); ++row) {
                    for (s32 column = 0; column < prototype->getWidth(); ++column) {
                        if (prototype->isPassableCell(column, row))
                            continue;
                        TRmgMapPosition candidatePosition(position.m_x + column,
                            position.m_y + row, position.m_z);
                        s32 score = scoreObjectPlacement(properties, candidatePosition);
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
            s32 selected = rand() % totalWeight;
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
                    if (nearby->hasObstacleFill() && nearby->isPassableLand()) {
                        nearby->clearObstacleFill();
                        pending.push_back(nearbyPosition);
                    }
                }
            }
        }
    }
}

VA(0x005378e0, 0x27)
TRmgMapItem* type_random_map::getMapItem(TRmgMapPosition point)
{
    return &m_mapItems[(point.m_z * m_mapHeight + point.m_y) * m_mapWidth + point.m_x];
}

// Decorates the whole map; runs after coastal tiles are marked.
VA(0x00537970, 0x199)
MAC_ADDRESS(0x236120, 0x2c4)
void TRmgGeneratorBase::decorateMap()
{
    TRmgMapItem* end = m_map.m_mapItems
        + m_map.m_numberLevels * m_map.m_mapHeight * m_map.m_mapWidth;
    s32 count = 0;
    TRmgMapItem* item;
    for (item = m_map.m_mapItems; item != end; ++item) {
        if (item->hasObstacleFill())
            ++count;
    }
    if (!count)
        return;
    s32 progressSteps = 276300 / count;
    item = m_map.m_mapItems;
    TRmgMapPosition position;
    for (position.m_z = RMG_SURFACE_LEVEL; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (item->hasObstacleFill()) {
                    if (item->isPassableLand())
                        decorateMapCell(position, progressSteps);
                    else if (m_progress)
                        m_progress->advance(progressSteps);
                }
            }
        }
    }
    for (item = m_map.m_mapItems; item != end; ++item) {
        if (!item->hasPathClearance() && item->isPassableLand())
            item->openPath();
    }
}

// Unlisted types get 32000 (unlimited); an earlier override wins on repeats.
static inline void initializeRmgObjectLimits(s32* limits,
    const TRmgObjectLimit* overrides, s32 count)
{
    for (s32 objectType = NOTHING; objectType < ADVENTURE_OBJECT_TRAIT_COUNT; ++objectType)
        limits[objectType] = 32000;
    while (count--)
        limits[overrides[count].m_objectType] = overrides[count].m_limit;
}

// Special heroes and heroes missing from the map version are never offered.
VA(0x00537b10, 0x2a8)
MAC_ADDRESS(0x2363e4, 0x2ec)
type_random_map_generator::type_random_map_generator(
    s32 width, s32 height, s32 levels, s32 humanPlayers, s32 humanTeams,
    s32 computerPlayers, s32 computerTeams, s32 waterContent,
    s32 monsterStrength, TProgressSink* progress, s32 mapVersion)
    : TRmgGeneratorBase(width, height, levels, progress,
        width * height + 326900, mapVersion)
{
    m_nextObjectId = 1;
#if defined(HOMM3_RMG_HOTFIX)
    // Key tents start at the first colour, not stack residue.
    m_nextKeyTentColor = RMG_KEY_LIGHT_BLUE;
#endif
    m_questArtifactPoolLow = false;
    m_waterContent = static_cast<ERmgWaterContent>(waterContent);
    m_monsterStrength = monsterStrength;
    m_humanPlayerCount = humanPlayers;
    m_humanTeamCount = humanTeams;
    m_computerPlayerCount = computerPlayers;
    m_computerTeamCount = computerTeams;
    if (m_waterContent == RMG_WATER_RANDOM)
        m_waterContent = static_cast<ERmgWaterContent>(rand() % RMG_WATER_RANDOM);
    loadTemplates();
    if (m_templates.size()) {
        m_nextSeerHutPrototypeIndex = 0;
        memset(m_usedQuestArtifacts, 0, sizeof(m_usedQuestArtifacts));
        memset(m_disabledHeroes, 0, sizeof(m_disabledHeroes));
        memset(m_objectCountByType, 0, sizeof(m_objectCountByType));
        initializeObjectGenerators();
        for (s32 hero = 0; hero < sizeof(m_disabledHeroes); ++hero) {
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

VA_COMPGEN(0x00537dc0, 0x21, SCALAR_DELETING_DTOR, type_random_map_generator)

VA(0x00537df0, 0x200)
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
    const TSpreadsheetResource::TStringVector& fields, s32 firstField)
{
    record.m_minimumHumanPlayers = atoi(fields[firstField]);
    record.m_maximumHumanPlayers =
        atoi(fields[firstField + RMG_PLAYER_LIMIT_MAXIMUM_HUMAN_PLAYERS]);
    record.m_minimumPlayers = atoi(fields[firstField + RMG_PLAYER_LIMIT_MINIMUM_PLAYERS]);
    record.m_maximumPlayers = atoi(fields[firstField + RMG_PLAYER_LIMIT_MAXIMUM_PLAYERS]);
}

template<class Record>
static inline bool allowsRmgTemplatePlayerCounts(const Record& record,
    s32 humanPlayers, s32 computerPlayers)
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
    TRmgTemplate* mapTemplate, s32 firstRow, s32 endRow,
    s32 humanPlayers, s32 computerPlayers)
{
    for (s32 connectionRow = firstRow; connectionRow < endRow; ++connectionRow) {
        const TSpreadsheetResource::TStringVector& fields = sheet->getRow(connectionRow);
        if (fields.size() > RMG_TEMPLATE_COLUMN_CONNECTION_MAXIMUM_PLAYERS
            && isRmgTemplateFieldSet(fields[RMG_TEMPLATE_COLUMN_CONNECTION_FIRST_ZONE])
            && fields[RMG_TEMPLATE_COLUMN_CONNECTION_SECOND_ZONE][0]) {
            s32 firstZone = atoi(fields[RMG_TEMPLATE_COLUMN_CONNECTION_FIRST_ZONE]);
            s32 secondZone = atoi(fields[RMG_TEMPLATE_COLUMN_CONNECTION_SECOND_ZONE]);
            TRmgTemplateZone* first = mapTemplate->findZone(firstZone);
            TRmgTemplateZone* second = mapTemplate->findZone(secondZone);
            if (first && second) {
                TRmgZoneConnection connection;
                connection.m_value = atoi(fields[RMG_TEMPLATE_COLUMN_CONNECTION_VALUE]);
                connection.m_unguarded = isRmgTemplateFieldSet(
                    fields[RMG_TEMPLATE_COLUMN_CONNECTION_UNGUARDED]);
                connection.m_borderGuard = isRmgTemplateFieldSet(
                    fields[RMG_TEMPLATE_COLUMN_CONNECTION_BORDER_GUARD]);
                readRmgTemplatePlayerLimits(connection, fields,
                    RMG_TEMPLATE_COLUMN_CONNECTION_MINIMUM_HUMAN_PLAYERS);
                connection.m_connected = false;
                if (allowsRmgTemplatePlayerCounts(connection, humanPlayers, computerPlayers))
                    appendRmgTwoWayConnection(first, second, connection);
            }
        }
    }
}

bool TRmgTemplate::hasPlayerSlots(s32 humanPlayers, s32 computerPlayers) const
{
    s32 playerSlots = 0;
    for (u32 slot = 0; slot < m_zones.size(); ++slot)
        if (m_zones[slot]->m_kind == RMG_TEMPLATE_HUMAN)
            ++playerSlots;
    if (playerSlots < humanPlayers)
        return false;
    for (slot = 0; slot < m_zones.size(); ++slot)
        if (m_zones[slot]->m_kind == RMG_TEMPLATE_COMPUTER)
            ++playerSlots;
    return playerSlots >= humanPlayers + computerPlayers;
}

// Template sizes are in units of 36x36 cells.
VA(0x00537ff0, 0x482)
MAC_ADDRESS(0x2372ec, 0x304)
void type_random_map_generator::loadTemplates()
{
    TSpreadsheetResource* sheet = ResourceManager::getSpreadsheet(
        DATA_COMPGEN(0x00682804, rmgTemplatesFilename, "rmg.txt"));
    s32 mapSize = m_map.getWidth() * m_map.getHeight() * m_map.m_numberLevels / (36 * 36);
    s32 row = RMG_FIRST_DATA_ROW;
    if (m_waterContent == RMG_WATER_ISLANDS)
        mapSize = max(mapSize / 2, 1);
    while (row < sheet->getNumberOfRows()) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
#if defined(HOMM3_RMG_HOTFIX)
        // Rows without a maximum size are skipped.
        if (values.size() <= RMG_TEMPLATE_COLUMN_MAXIMUM_SIZE) {
#else
        // Retail bug: a row ending at the minimum size still reads the maximum.
        if (values.size() < RMG_TEMPLATE_COLUMN_MAXIMUM_SIZE) {
#endif
            ++row;
            continue;
        }
        TRmgTemplate* mapTemplate = new TRmgTemplate;
        mapTemplate->m_minimumSize = atoi(values[RMG_TEMPLATE_COLUMN_MINIMUM_SIZE]);
        mapTemplate->m_maximumSize = atoi(values[RMG_TEMPLATE_COLUMN_MAXIMUM_SIZE]);
        mapTemplate->m_name = values[RMG_TEMPLATE_COLUMN_NAME];
        s32 endRow = row + 1;
        while (endRow < sheet->getNumberOfRows()
            && !isRmgTemplateFieldSet(sheet->getRow(endRow)[RMG_TEMPLATE_COLUMN_NAME]))
            ++endRow;
        bool accepted = mapSize >= mapTemplate->m_minimumSize
            && mapSize <= mapTemplate->m_maximumSize;
        if (accepted) {
            readRmgTemplateZones(sheet, mapTemplate, row, endRow,
                m_humanPlayerCount, m_computerPlayerCount, m_mapVersion);
            readRmgTemplateConnections(sheet, mapTemplate, row, endRow,
                m_humanPlayerCount, m_computerPlayerCount);
            accepted = mapTemplate->hasPlayerSlots(
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
    s32 firstRow, s32 endRow, s32 humanPlayers, s32 computerPlayers,
    s32 mapVersion)
{
    for (s32 row = firstRow; row < endRow; ++row) {
        const TSpreadsheetResource::TStringVector& values = sheet->getRow(row);
#if defined(HOMM3_RMG_HOTFIX)
        // Rows without a zone index are skipped.
        if (values.size() > RMG_TEMPLATE_COLUMN_ZONE_INDEX
#else
        // Retail bug: a row ending before the zone index still reads it.
        if (values.size() >= RMG_TEMPLATE_COLUMN_ZONE_INDEX
#endif
            && isRmgTemplateFieldSet(values[RMG_TEMPLATE_COLUMN_ZONE_INDEX])
            && values.size() > RMG_TEMPLATE_COLUMN_LAST_TREASURE_DENSITY) {

            TRmgTemplateZone* templateZone = new TRmgTemplateZone;
            templateZone->m_zoneIndex = atoi(values[RMG_TEMPLATE_COLUMN_ZONE_INDEX]);
            templateZone->m_kind = RMG_TEMPLATE_TREASURE;
            if (isRmgTemplateFieldSet(values[RMG_TEMPLATE_COLUMN_KIND_HUMAN]))
                templateZone->m_kind = RMG_TEMPLATE_HUMAN;
            if (isRmgTemplateFieldSet(values[RMG_TEMPLATE_COLUMN_KIND_COMPUTER]))
                templateZone->m_kind = RMG_TEMPLATE_COMPUTER;
            if (isRmgTemplateFieldSet(values[RMG_TEMPLATE_COLUMN_KIND_TREASURE]))
                templateZone->m_kind = RMG_TEMPLATE_TREASURE;
            if (isRmgTemplateFieldSet(values[RMG_TEMPLATE_COLUMN_KIND_JUNCTION]))
                templateZone->m_kind = RMG_TEMPLATE_JUNCTION;
            templateZone->m_size = atoi(values[RMG_TEMPLATE_COLUMN_SIZE]);
            readRmgTemplatePlayerLimits(*templateZone, values,
                RMG_TEMPLATE_COLUMN_MINIMUM_HUMAN_PLAYERS);
            if (!allowsRmgTemplatePlayerCounts(*templateZone, humanPlayers, computerPlayers)) {
                delete templateZone;
            } else {
                templateZone->m_playerIndex = atoi(values[RMG_TEMPLATE_COLUMN_PLAYER_INDEX]) - 1;
                templateZone->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT] =
                    atoi(values[RMG_TEMPLATE_COLUMN_PLAYER_BASIC_COUNT]);
                templateZone->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_COUNT] =
                    atoi(values[RMG_TEMPLATE_COLUMN_PLAYER_CASTLE_COUNT]);
                templateZone->m_townPlacement[RMG_TOWN_PLAYER_BASIC_DENSITY] =
                    atoi(values[RMG_TEMPLATE_COLUMN_PLAYER_BASIC_DENSITY]);
                templateZone->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_DENSITY] =
                    atoi(values[RMG_TEMPLATE_COLUMN_PLAYER_CASTLE_DENSITY]);
                templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT] =
                    atoi(values[RMG_TEMPLATE_COLUMN_NEUTRAL_BASIC_COUNT]);
                templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_COUNT] =
                    atoi(values[RMG_TEMPLATE_COLUMN_NEUTRAL_CASTLE_COUNT]);
                templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_DENSITY] =
                    atoi(values[RMG_TEMPLATE_COLUMN_NEUTRAL_BASIC_DENSITY]);
                templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_DENSITY] =
                    atoi(values[RMG_TEMPLATE_COLUMN_NEUTRAL_CASTLE_DENSITY]);
                templateZone->m_neutralTownsMatchZone =
                    isRmgTemplateFieldSet(values[RMG_TEMPLATE_COLUMN_NEUTRAL_TOWNS_MATCH_ZONE]);
                // RoE maps have no Conflux.
                s32 townCount;
                if (mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE)
                    townCount = TOWN_TYPE_COUNT;
                else {
                    townCount = TOWN_CONFLUX;
                    templateZone->m_allowedTowns[TOWN_CONFLUX] = false;
                }
                while (townCount--)
                    templateZone->m_allowedTowns[townCount] =
                        isRmgTemplateFieldSet(
                            values[RMG_TEMPLATE_COLUMN_ALLOWED_TOWNS + townCount]);
                for (s32 resource = WOOD; resource < NUM_RESOURCES; ++resource) {
                    templateZone->m_mineCounts[resource] =
                        atoi(values[RMG_TEMPLATE_COLUMN_MINE_COUNTS + resource]);
                    templateZone->m_mineDensities[resource] =
                        atoi(values[RMG_TEMPLATE_COLUMN_MINE_DENSITIES + resource]);
                }
                templateZone->m_useNativeTerrain =
                    isRmgTemplateFieldSet(values[RMG_TEMPLATE_COLUMN_USE_NATIVE_TERRAIN]);
                b8 anyTerrain = false;
                for (s32 terrain = eTerrainDirt; terrain < eTerrainWater; ++terrain) {
                    templateZone->m_allowedTerrain[terrain] =
                        isRmgTemplateFieldSet(
                            values[RMG_TEMPLATE_COLUMN_ALLOWED_TERRAIN + terrain]);
                    if (templateZone->m_allowedTerrain[terrain])
                        anyTerrain = true;
                }
                if (!anyTerrain)
                    templateZone->m_allowedTerrain[eTerrainDirt] = true;
                switch (tolower(values[RMG_TEMPLATE_COLUMN_MONSTER_STRENGTH][0])) {
                case 'n': templateZone->m_monsterStrength = RMG_ZONE_MONSTERS_NONE; break;
                case 'w': templateZone->m_monsterStrength = RMG_ZONE_MONSTERS_WEAK; break;
                case 's': templateZone->m_monsterStrength = RMG_ZONE_MONSTERS_STRONG; break;
                case 'a': templateZone->m_monsterStrength = RMG_ZONE_MONSTERS_AVERAGE; break;
                default: templateZone->m_monsterStrength = RMG_ZONE_MONSTERS_AVERAGE; break;
                }
                templateZone->m_guardsMatchZone =
                    isRmgTemplateFieldSet(values[RMG_TEMPLATE_COLUMN_GUARDS_MATCH_ZONE]);
                // Neutral, then one slot per town type.
                for (s32 monster = 0; monster < TOWN_TYPE_COUNT + 1; ++monster)
                    templateZone->m_allowedMonsters[monster] =
                        isRmgTemplateFieldSet(
                            values[RMG_TEMPLATE_COLUMN_ALLOWED_MONSTERS + monster]);
                // Retail bug: monster slots are offset by one, so this disallows
                // Fortress guards, not Conflux.
                if (mapVersion < RMG_MAP_ARMAGEDDONS_BLADE)
                    templateZone->m_allowedMonsters[TOWN_CONFLUX] = false;
                for (s32 treasure = 0; treasure < 3; ++treasure) {
                    templateZone->m_treasure[treasure].m_minimum =
                        atoi(values[RMG_TEMPLATE_COLUMN_TREASURE_MINIMUM
                            + RMG_TEMPLATE_TREASURE_COLUMN_COUNT * treasure]);
                    templateZone->m_treasure[treasure].m_maximum =
                        atoi(values[RMG_TEMPLATE_COLUMN_TREASURE_MAXIMUM
                            + RMG_TEMPLATE_TREASURE_COLUMN_COUNT * treasure]);
                    templateZone->m_treasure[treasure].m_density =
                        atoi(values[RMG_TEMPLATE_COLUMN_TREASURE_DENSITY
                            + RMG_TEMPLATE_TREASURE_COLUMN_COUNT * treasure]);
                }
                mapTemplate->m_zones.push_back(templateZone);
            }
        }
    }
}

static inline s32 getRmgCreatureTypeCount(s32 mapVersion)
{
    return mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE
        ? RMG_CREATURE_TYPE_COUNT : RMG_ROE_CREATURE_TYPE_COUNT;
}

// Creature dwelling subtypes offered as treasures (g_creatureGenerator1Types).
enum ERmgDwellingSubtypeCounts {
    RMG_DWELLING_SUBTYPE_COUNT = 80,
    RMG_ROE_DWELLING_SUBTYPE_COUNT = 58
};

VA(0x00538b10, 0x2241)
MAC_ADDRESS(0x2375f0, 0x4880)
void type_random_map_generator::initializeObjectGenerators()
{
    m_objectGenerators.push_back(new type_treasure_def(ALTAR_OF_SACRIFICE, 0, 100, 20));
    m_objectGenerators.push_back(new type_treasure_def(ARENA, 0, 3000, 50));

    for (s32 creature = getRmgCreatureTypeCount(m_mapVersion); creature--;) {
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

    s32 tentIndex = m_objectPrototypes[BORDER_TENT].size();
    m_disabledKeyTentColors.resize(tentIndex);
    while (tentIndex--) {
        m_disabledKeyTentColors[tentIndex] = false;
        m_objectGenerators.push_back(new type_key_tent_def(tentIndex, 5000));
        m_objectGenerators.push_back(new type_key_tent_def(tentIndex, 7500));
        m_objectGenerators.push_back(new type_key_tent_def(tentIndex, 10000));
        m_objectGenerators.push_back(new type_key_tent_def(tentIndex, 15000));
        m_objectGenerators.push_back(new type_key_tent_def(tentIndex, 20000));
    }

    m_objectGenerators.push_back(new type_treasure_def(BLACK_MARKET, 0, 8000, 20));
    m_objectGenerators.push_back(new type_treasure_def(BUOY, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(CAMPFIRE, 0, 2000, 500));
    m_objectGenerators.push_back(new type_treasure_def(CARTOGRAPHER, CARTOGRAPHER_WATER, 5000, 20));
    m_objectGenerators.push_back(new type_treasure_def(CARTOGRAPHER, CARTOGRAPHER_LAND, 10000, 20));
    m_objectGenerators.push_back(new type_treasure_def(CARTOGRAPHER, CARTOGRAPHER_UNDERGROUND, 7500, 20));
    m_objectGenerators.push_back(new type_treasure_def(CLOVER_FIELD, 0, 100, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, CREATURE_BANK_CYCLOPS, 3000, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, CREATURE_BANK_DWARF, 2000, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, CREATURE_BANK_GRIFFIN, 2000, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, CREATURE_BANK_IMP, 5000, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, CREATURE_BANK_MEDUSA, 1500, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, CREATURE_BANK_NAGA, 3000, 100));
    m_objectGenerators.push_back(new type_treasure_def(CREATURE_BANK, CREATURE_BANK_DRAGONFLY, 9000, 100));

    s32 dwelling = RMG_DWELLING_SUBTYPE_COUNT;
    if (m_mapVersion < RMG_MAP_ARMAGEDDONS_BLADE)
        dwelling = RMG_ROE_DWELLING_SUBTYPE_COUNT;
    while (dwelling--)
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
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, WOOD, 1400, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, ORE, 1400, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, MERCURY, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, SULFUR, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, CRYSTAL, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, GEMS, 2000, 300));
    m_objectGenerators.push_back(new type_resource_lump_def(RESOURCE, GOLD, 750, 300));
    m_objectGenerators.push_back(new type_treasure_def(SANCTUARY, 0, 100, 50));
    m_objectGenerators.push_back(new type_scholar_def());
    m_objectGenerators.push_back(new type_treasure_def(SEA_CHEST, 0, 1500, 500));

    for (s32 prototypeIndex = 0; prototypeIndex < m_objectPrototypes[SEER].size();
         ++prototypeIndex) {
        for (s32 creature = getRmgCreatureTypeCount(m_mapVersion); creature--;) {
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
VA(0x0053ad60, 0x113)
MAC_ADDRESS(0x23be70, 0x14c)
b8 type_random_map_generator::canPlaceZone(TRmgZone* zone)
{
    TRmgTemplateZone* templateZone = zone->m_templateZone;
    TRmgMapPosition position = zone->getLevelPosition();
    s32 size = templateZone->m_size;
    if ((templateZone->m_kind == RMG_TEMPLATE_HUMAN ||
         templateZone->m_kind == RMG_TEMPLATE_COMPUTER) &&
        position.m_z == RMG_UNDERGROUND_LEVEL && zone->m_alignment != TOWN_INFERNO &&
        zone->m_alignment != TOWN_NECROPOLIS && zone->m_alignment != TOWN_DUNGEON)
        return false;
    s32 zoneIndex = templateZone->m_zoneIndex;
    for (s32 other = 0; other < m_zones.size(); ++other) {
        TRmgZone* otherZone = m_zones[other];
        if (otherZone->getLevelPosition().m_z != position.m_z ||
            otherZone->m_templateZone->m_zoneIndex == zoneIndex)
            continue;
        TRmgMapPosition otherPosition = otherZone->getLevelPosition();
        s32 distance = getRmgDistance(otherPosition, position);
        if (10 * distance < 8 * (otherZone->m_templateZone->m_size + size))
            return false;
    }
    return true;
}

// The other level of a two-level map.
static inline s32 getRmgOtherLevel(s32 level)
{
    return RMG_UNDERGROUND_LEVEL - level;
}

// One of the 32 radial candidate positions around a zone.
static inline TRmgMapPosition getRmgRadialZonePosition(
    const TRmgMapPosition& center, s32 radius, s32 direction, s32 level)
{
    return TRmgMapPosition(
        static_cast<s32>(center.m_x + radius * g_rmgDirectionCosines[direction]),
        static_cast<s32>(center.m_y + radius * g_rmgDirectionSines[direction]),
        level);
}

// Trial placement leaves the zone at the candidate even when rejected.
inline void type_random_map_generator::appendZoneCandidate(TRmgZone* zone,
    const TRmgMapPosition& candidate, std::vector<TRmgMapPosition>& candidates)
{
    zone->setLevelPosition(candidate);
    if (canPlaceZone(zone))
        candidates.push_back(zone->getLevelPosition());
}

VA(0x0053ae80, 0x36a)
MAC_ADDRESS(0x23bfbc, 0x28c)
void type_random_map_generator::appendZonePositions(TRmgZone* center,
    TRmgZone* zone, std::vector<TRmgMapPosition>& candidates)
{
    s32 radius = center->m_templateZone->m_size + zone->m_templateZone->m_size;
    TRmgMapPosition position = center->getLevelPosition();
    TRmgMapPosition candidate;
    for (s32 direction = 0; direction < RMG_RADIAL_DIRECTION_COUNT; ++direction) {
        candidate = getRmgRadialZonePosition(position, radius, direction, position.m_z);
        appendZoneCandidate(zone, candidate, candidates);
    }
    if (m_map.m_numberLevels == 1)
        return;
    s32 level = getRmgOtherLevel(position.m_z);
    candidate = TRmgMapPosition(position.m_x, position.m_y, level);
    appendZoneCandidate(zone, candidate, candidates);
    radius = center->m_templateZone->m_size;
    if (radius < zone->m_templateZone->m_size)
        radius = zone->m_templateZone->m_size;
    for (direction = 0; direction < RMG_RADIAL_DIRECTION_COUNT; ++direction) {
        candidate = getRmgRadialZonePosition(position, radius, direction, level);
        appendZoneCandidate(zone, candidate, candidates);
    }
}

MAC_ADDRESS(0x23c248, 0x9c)
s32 type_random_map_generator::countPlacedZoneConnections(TRmgZone* zone) const
{
    s32 connectionCount = 0;
    TRmgTemplateZone* templateZone = zone->m_templateZone;
    for (s32 connection = 0; connection < templateZone->m_connections.size(); ++connection) {
        s32 destination = templateZone->m_connections[connection].m_destination->m_zoneIndex;
        if (destination < m_zones.size() && m_zones[destination]->canConnect(zone))
            ++connectionCount;
    }
    return connectionCount;
}

// Grow a half-open bounding rectangle around a zone's nominal radius.
static inline void includeRmgZoneFootprint(s32& minimumY, s32& minimumX,
    s32& maximumY, s32& maximumX, const TRmgMapPosition& position, s32 size)
{
    minimumY = min(minimumY, position.m_y - size);
    minimumX = min(minimumX, position.m_x - size);
    maximumY = max(maximumY, position.m_y + size + 1);
    maximumX = max(maximumX, position.m_x + size + 1);
}

VA(0x0053b1f0, 0xfe)
MAC_ADDRESS(0x23c2e4, 0x170)
void type_random_map_generator::getInitialZoneBounds(s32& minimumY, s32& minimumX,
    s32& maximumY, s32& maximumX) const
{
    minimumY = 0;
    minimumX = 0;
    maximumY = 0;
    maximumX = 0;
    for (s32 zone = 0; zone < m_zones.size(); ++zone) {
        TRmgMapPosition position;
        position = m_zones[zone]->getLevelPosition();
        s32 size = m_zones[zone]->m_templateZone->m_size;
        includeRmgZoneFootprint(minimumY, minimumX, maximumY, maximumX, position, size);
    }
}

// Square-map extent needed to enclose a candidate zone and all other zones,
// at least the requested map size.
static inline s32 getRmgCandidateMapSize(const TRmgZoneBounds& bounds,
    const TRmgMapPosition& position, s32 zoneSize, s32 mapSize)
{
    TRmgZoneBounds candidate = bounds;
    includeRmgZoneFootprint(candidate.m_minimumY, candidate.m_minimumX,
        candidate.m_maximumY, candidate.m_maximumX, position, zoneSize);
    s32 candidateSize = max(mapSize, candidate.m_maximumY - candidate.m_minimumY);
    candidateSize = max(candidateSize, candidate.m_maximumX - candidate.m_minimumX);
    return candidateSize;
}

// Prefer unused levels, then the most connections, then the smallest
// enclosing square.
VA(0x0053b2f0, 0x678)
MAC_ADDRESS(0x23c454, 0x62c)
void type_random_map_generator::filterZonePositions(
    TRmgZone* zone, std::vector<TRmgMapPosition>& candidates, s32 mapSize)
{
    s32 bestConnections = 0;
    if (m_map.m_numberLevels > 1) {
        b8 occupiedLevels[RMG_MAP_LEVEL_COUNT] = {false, false};
        for (s32 other = 0; other < m_zones.size(); ++other) {
            if (m_zones[other] != zone)
                occupiedLevels[m_zones[other]->getLevelPosition().m_z] = true;
        }
        if (!occupiedLevels[RMG_SURFACE_LEVEL]
            || !occupiedLevels[RMG_UNDERGROUND_LEVEL]) {
            s32 candidate = candidates.size();
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

    for (s32 candidate = 0; candidate < candidates.size(); ++candidate) {
        zone->setLevelPosition(candidates[candidate]);
        s32 connections = countPlacedZoneConnections(zone);
        if (connections > bestConnections)
            bestConnections = connections;
    }
    for (candidate = candidates.size() - 1; candidate >= 0; --candidate) {
        zone->setLevelPosition(candidates[candidate]);
        if (countPlacedZoneConnections(zone) < bestConnections)
            candidates.erase(candidates.begin() + candidate);
    }

    s32 bestSize = 32000;
    TRmgZoneBounds bounds = {0, 0, 0, 0};
    for (s32 other = 0; other < m_zones.size(); ++other) {
        if (m_zones[other] != zone) {
            TRmgMapPosition position = m_zones[other]->getLevelPosition();
            s32 size = m_zones[other]->getTemplateSize();
            includeRmgZoneFootprint(bounds.m_minimumY, bounds.m_minimumX,
                bounds.m_maximumY, bounds.m_maximumX, position, size);
        }
    }
    s32 size = zone->getTemplateSize();
    for (candidate = 0; candidate < candidates.size(); ++candidate) {
        s32 candidateSize = getRmgCandidateMapSize(
            bounds, candidates[candidate], size, mapSize);
        bestSize = min(bestSize, candidateSize);
    }
    for (candidate = candidates.size() - 1; candidate >= 0; --candidate) {
        s32 candidateSize = getRmgCandidateMapSize(
            bounds, candidates[candidate], size, mapSize);
        if (bestSize < candidateSize)
            candidates.erase(candidates.begin() + candidate);
    }
}

// The first zone starts at the origin, on the surface or, when eligible,
// underground. Later zones sample template neighbours, falling back to all
// placed zones before filtering.
VA(0x0053b970, 0x232)
MAC_ADDRESS(0x23ca80, 0x20c)
void type_random_map_generator::positionZone(TRmgZone* zone, s32 mapSize)
{
    std::vector<TRmgMapPosition> candidates;
    if (m_zones.size() == 0) {
        zone->m_levelPosition.m_y = 0;
        zone->m_levelPosition.m_z = RMG_SURFACE_LEVEL;
        zone->m_levelPosition.m_x = 0;
        candidates.push_back(zone->getLevelPosition());
        if (m_map.m_numberLevels > 1)
            appendZoneCandidate(zone,
                TRmgMapPosition(0, 0, RMG_UNDERGROUND_LEVEL), candidates);
    } else {
        TRmgTemplateZone* templateZone = zone->m_templateZone;
        for (s32 connection = 0; connection < templateZone->m_connections.size(); ++connection) {
            TRmgTemplateZone* destination = templateZone->m_connections[connection].m_destination;
            if (destination->m_zoneIndex < m_zones.size())
                appendZonePositions(m_zones[destination->m_zoneIndex], zone, candidates);
        }
        if (candidates.size() == 0) {
            for (s32 other = 0; other < m_zones.size(); ++other)
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

VA(0x0053bbb0, 0xfd)
MAC_ADDRESS(0x23cc8c, 0xf8)
void type_random_map_generator::calculateZoneBounds()
{
    TRmgMapItem* item = m_map.m_mapItems;
    TRmgMapPosition position;
    for (position.m_z = RMG_SURFACE_LEVEL; position.m_z < m_map.m_numberLevels; ++position.m_z) {
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
VA(0x0053bcb0, 0x33b)
MAC_ADDRESS(0x23cd84, 0x458)
void type_random_map_generator::initializeZones(TRmgTemplate* mapTemplate)
{
    m_zones.clear();
    s32 minimumSize = 32000;
    for (s32 index = 0; index < mapTemplate->m_zones.size(); ++index)
        minimumSize = min(minimumSize, mapTemplate->m_zones[index]->m_size);
    s32 mapSize = min(minimumSize * m_map.m_mapWidth,
        minimumSize * m_map.m_mapHeight);
    switch (m_waterContent) {
    case RMG_WATER_NONE: mapSize /= 5; break;
    case RMG_WATER_NORMAL: mapSize /= 6; break;
    default: mapSize /= 7; break;
    }
    for (index = 0; index < mapTemplate->m_zones.size(); ++index) {
        TRmgTemplateZone* templateZone = mapTemplate->m_zones[index];
        TRmgZone* zone = new TRmgZone(templateZone);
        if (templateZone->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT]
                + templateZone->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_COUNT] > 0
            && templateZone->m_playerIndex >= 0) {
            s32 player = m_playerIndexMap[templateZone->m_playerIndex + 1];
            if (player >= 0 && m_townChoices[player] != eTownNeutral)
                zone->m_alignment = m_townChoices[player];
        }
        positionZone(zone, mapSize);
        m_zones.push_back(zone);
    }
    for (s32 pass = 0; pass < 2; ++pass) {
        for (index = 0; index < mapTemplate->m_zones.size(); ++index)
            positionZone(m_zones[index], mapSize);
    }
    s32 minimumY, minimumX, maximumY, maximumX;
    getInitialZoneBounds(minimumY, minimumX, maximumY, maximumX);
    s32 span = max(maximumY - minimumY, maximumX - minimumX);
    s32 size = max(m_map.m_mapWidth, m_map.m_mapHeight);
    s32 originY = (minimumY - span + maximumY) / 2;
    s32 originX = (minimumX - span + maximumX) / 2;
    for (s32 zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
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
static inline s32 getRmgCenteredRandomOffset(s32 range)
{
    return rand() % range - range / 2;
}

// lengthDivisor values of splitRmgBoundarySegment.
enum ERmgBoundaryDisplacement {
    RMG_FULL_LENGTH_DISPLACEMENT = 1,
    RMG_HALF_LENGTH_DISPLACEMENT = 2
};

// Split a boundary segment at a midpoint displaced at random across it,
// pushing the far half first so the near half is walked next. The
// displacement range is the segment length (half of it for island coasts),
// capped by roughness. Returns false when the segment has no interior midpoint.
static inline bool splitRmgBoundarySegment(std::vector<TPoint>& pending,
    const TPoint& from, const TPoint& to, s32 roughness, s32 lengthDivisor)
{
    TPoint midpoint = getRmgSubdivisionMidpoint(from, to);
    if (midpoint == from || midpoint == to)
        return false;
    TRmgVector delta = to - from;
    TRmgVector perpendicular(-delta.m_y, delta.m_x);
    s32 length = perpendicular.length();
    if (length > 1) {
        // Roughness must be positive; zero reaches rand() % 0.
        s32 limit = cppMin<long>(length / lengthDivisor, roughness);
        s32 displacement = getRmgCenteredRandomOffset(limit);
        perpendicular = perpendicular * displacement / length;
        midpoint += perpendicular;
    }
    pending.push_back(to);
    pending.push_back(midpoint);
    return true;
}

// As members of type_random_map and TRmgMapItem, these helpers change
// unrelated code in game units that include rmg.h.
static inline TPoint clampRmgBoundaryToMap(
    const TPoint& point, const type_random_map& map)
{
    return TPoint(min(max(point.m_x, 0), map.m_mapWidth - 1),
        min(max(point.m_y, 0), map.m_mapHeight - 1));
}

// Island maps paint surface zone terrain only on the islands.
static inline bool paintsRmgZoneTerrainOnLevel(
    ERmgWaterContent waterContent, s32 level)
{
    return level == RMG_UNDERGROUND_LEVEL || waterContent != RMG_WATER_ISLANDS;
}

static inline void assignRmgZoneCell(
    TRmgMapItem* item, s32 zoneIndex, b8 markForTerrain)
{
    item->m_zoneState.m_zone = zoneIndex;
    if (markForTerrain)
        item->m_tileData.m_paintZoneTerrain = true;
}

// Depth-first midpoint displacement: rounding, displacement bounds and RNG
// order are observable in generated maps.
VA(0x0053bff0, 0x22b)
MAC_ADDRESS(0x23d684, 0x408)
void type_random_map_generator::drawIrregularZoneBoundary(
    TPoint from, TPoint to, s32 zoneIndex, s32 level, s32 roughness)
{
    std::vector<TPoint> pending;
    b8 markForTerrain = paintsRmgZoneTerrainOnLevel(m_waterContent, level);
    pending.push_back(to);
    while (pending.size() > 0) {
        to = pending.back();
        pending.pop_back();
        if (!splitRmgBoundarySegment(pending, from, to, roughness,
                RMG_FULL_LENGTH_DISPLACEMENT)) {
            TPoint clamped = clampRmgBoundaryToMap(from, m_map);
            TRmgMapItem* item = m_map.getMapItem(clamped.m_x, clamped.m_y, level);
            assignRmgZoneCell(item, zoneIndex, markForTerrain);
            from = to;
        }
    }
}

// Bresenham line whose error starts at half the major distance.
// The final cell receives its zone but not the terrain mark.
VA(0x0053c220, 0x16a)
MAC_ADDRESS(0x23da90, 0x2a4)
void type_random_map_generator::drawStraightZoneBoundary(
    TPoint from, TPoint to, s32 zoneIndex, s32 level)
{
    if (from.m_x > to.m_x)
        std::swap(from, to);
    s32 deltaX = to.m_x - from.m_x;
    s32 deltaY = to.m_y - from.m_y;
    s32 verticalDistance = abs(deltaY);
    s32 majorDistance;
    s32 minorDistance;
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
    s32 error = majorDistance / 2;
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
VA(0x0053c390, 0x730)
MAC_ADDRESS(0x23dd34, 0x614)
void type_random_map_generator::traceZoneBoundary(
    TRmgHalfEdge* first, b8 irregular)
{
    TRmgHalfEdge* edge = first;
    TRmgZone* zone = edge->m_zone;
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition zonePosition = zone->m_levelPosition;
    TRmgZoneBounds bounds = {0, 0, m_map.m_mapWidth, m_map.m_mapHeight};
    TPoint upperLeft(bounds.m_minimumX, bounds.m_minimumY);
    TPoint upperRight(bounds.m_maximumX - 1, bounds.m_minimumY);
    TPoint lowerLeft(bounds.m_minimumX, bounds.m_maximumY - 1);
    TPoint lowerRight(bounds.m_maximumX - 1, bounds.m_maximumY - 1);

    bool found = false;
    do {
        TRmgHalfEdge* next = edge->m_next;
        TPoint from;
        TPoint to;
        clipRmgBoundarySegment(bounds, edge->m_vertex, next->m_vertex, from, to);
        if (bounds.contains(from) && from != to) {
            found = true;
            break;
        }
        edge = next;
    } while (edge != first);
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

    first = edge;
    do {
        TRmgHalfEdge* next = edge->m_next;
        TRmgZone* neighbour = next->getOppositeZone();
        TPoint originalTo = next->m_vertex;
        TPoint from;
        TPoint to;
        clipRmgBoundarySegment(bounds, edge->m_vertex, originalTo, from, to);
        zone->m_boundary.push_back(TPoint(from));

        if (!neighbour || neighbour->m_templateZone->m_zoneIndex > zoneIndex) {
            s32 roughness = zone->m_scaledSize;
            if (neighbour)
                roughness = min(roughness, neighbour->m_scaledSize);
            if (irregular)
                drawIrregularZoneBoundary(from, to, zoneIndex, zonePosition.m_z, roughness);
            else
                drawStraightZoneBoundary(from, to, zoneIndex, zonePosition.m_z);
        }

        edge = next;
        if (to != originalTo) {
            from = to;
            for (;;) {
                next = next->m_next;
                to = clipRmgBoundaryPoint(bounds, edge->m_vertex, next->m_vertex);
                if (bounds.contains(to))
                    break;
                edge = next;
            }
            // Follow the map border clockwise, one corner at a time.
            // North is up:  upperLeft -> upperRight
            //                   ^             v
            //               lowerLeft <- lowerRight
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
    } while (edge != first);
}

// Reject clipping across an axis limit from its permitted side.
static inline bool crossesRmgBoundaryAxis(
    s32 original, s32 clipped, s32 minimum, s32 maximum)
{
    return (original >= minimum && clipped < minimum)
        || (original < maximum && clipped >= maximum);
}

// Integer clipping toward `toward` against the left, top, right, then bottom
// edge; a rejected clip returns the point unchanged.
VA(0x0053cac0, 0x266)
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
    std::vector<TRmgZone*>& zones, std::vector<s32>& costs,
    TRmgZone* zone, s32 cost)
{
    s32 insertionIndex = findRmgWorkItemInsertionIndex(costs, zones.size(), cost);
    costs.insert(costs.begin() + insertionIndex, cost);
    zones.insert(zones.begin() + insertionIndex, 1, zone);
}

// Island-coast variant of drawIrregularZoneBoundary: half the displacement
// range, and it only marks cells already in the zone for terrain painting.
VA(0x0053cd30, 0x212)
MAC_ADDRESS(0x23e348, 0x3f0)
void type_random_map_generator::drawIslandBoundary(TPoint from, TPoint to,
    s32 zoneIndex, s32 level, s32 roughness)
{
    std::vector<TPoint> pending;
    pending.push_back(to);
    while (pending.size() > 0) {
        to = pending.back();
        pending.pop_back();
        if (!splitRmgBoundarySegment(pending, from, to, roughness,
                RMG_HALF_LENGTH_DISPLACEMENT)) {
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
VA(0x0053cf50, 0x177)
MAC_ADDRESS(0x23e738, 0x1f0)
void type_random_map_generator::fillIslandInterior(TRmgZone* zone)
{
    std::vector<TRmgMapPosition> pending;
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition position = zone->getLevelPosition();
    pending.push_back(position);
    while (pending.size()) {
        position = pending.back();
        pending.pop_back();
        for (s32 direction = RMG_DIRECTION_EAST; direction < RMG_DIRECTION_COUNT;
             direction += RMG_CARDINAL_DIRECTION_STEP) {
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

VA(0x0053d0d0, 0xe3)
MAC_ADDRESS(0x23e928, 0x184)
void type_random_map_generator::recenterZone(TRmgZone* zone)
{
    TRmgZoneBounds bounds = zone->m_bounds;
    TRmgMapPosition position = zone->getLevelPosition();
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    s32 cellCount = 0;
    TRmgMapPosition coordinateTotal(0, 0, position.m_z);
    for (s32 y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
        for (s32 x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
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
    s32 length = delta.length();
    if (length > 0) {
        s32 displacement = std::max<s32>(4, length / 4);
        displacement = std::min<s32>(displacement, length / 2);
        delta = delta * displacement / length;
        point += delta;
    }
}

VA(0x0053d1c0, 0x1b9)
MAC_ADDRESS(0x23eaac, 0x2b0)
void type_random_map_generator::insetIslandZone(TRmgZone* zone)
{
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition center = zone->getLevelPosition();
    s32 count = zone->m_boundary.size();
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
    const TRmgMapPosition& position, s32 rowStep, b8& inSpan,
    TRmgMapPosition& seed, std::vector<TRmgMapPosition>& pending)
{
    if (neighbour->m_zoneState.m_zone == RMG_NO_ZONE) {
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
VA(0x0053d380, 0x551)
MAC_ADDRESS(0x23ed74, 0x4a4)
void type_random_map_generator::fillZoneArea(TRmgZone* zone, TRmgHalfEdge* first)
{
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    std::vector<TRmgMapPosition> pending;
    TRmgMapPosition position = zone->getLevelPosition();
    if (!m_map.containsXY(position)) {
        s32 bestClearance = 0;
        TPoint best;
        best.m_x = RMG_NO_POSITION;
        TRmgHalfEdge* edge = first;
        do {
            edge = edge->m_next;
            TPoint point = edge->getOppositeSitePosition();
            if (point.m_x >= 1 && point.m_x < m_map.getWidth() - 1
                && point.m_y >= 1 && point.m_y < m_map.getHeight() - 1) {
                s32 clearance = min(min(min(point.m_x,
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
        TRmgMapPosition upperSeed;
        TRmgMapPosition lowerSeed;
        while (position.m_x > 0 && (item - 1)->m_zoneState.m_zone == RMG_NO_ZONE) {
            --item;
            --position.m_x;
        }
        while (position.m_x < m_map.getWidth() && item->m_zoneState.m_zone == RMG_NO_ZONE) {
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
VA(0x0053d8e0, 0x1ec)
MAC_ADDRESS(0x23f218, 0x2f0)
void type_random_map_generator::propagateZoneDistances(TRmgZone* zone)
{
    std::vector<TRmgZone*> pending;
    std::vector<s32> costs;
    s32 distanceCount = zone->m_zoneDistances.size();
    for (s32 column = 0; column < distanceCount; ++column) {
        pending.push_back(zone);
        costs.push_back(0);
        while (pending.size()) {
            TRmgZone* current = pending.back();
            pending.pop_back();
            TRmgTemplateZone* templateZone = current->m_templateZone;
            costs.pop_back();
            s32 distance = current->m_zoneDistances[column] + 1;
            for (u32 connection = 0; connection < templateZone->m_connections.size(); ++connection) {
                TRmgZone* next = m_zones[templateZone->m_connections[connection].m_destination->m_zoneIndex];
                if (next->m_zoneDistances[column] > distance) {
                    next->m_zoneDistances[column] = distance;
                    insertRmgWorkItem(pending, costs, next, distance);
                }
            }
        }
    }
}

VA_COMPGEN(0x0054c1e0, 0x1e9, VECTOR_RESIZE, Short)

VA_COMPGEN(0x0054c3d0, 0x12, VECTOR_SIZE, Short)

void type_random_map_generator::initializeZoneDistances(s32 originalZones)
{
    for (s32 index = 0; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        zone->m_zoneDistances.resize(originalZones);
        for (s32 column = originalZones; column--;)
            zone->m_zoneDistances[column] = RMG_UNREACHED_COST;
        if (zone->m_templateZone->m_zoneIndex < originalZones)
            zone->m_zoneDistances[zone->m_templateZone->m_zoneIndex] = 0;
    }
}

// Search a zone's closed boundary ring for an edge adjoining another zone,
// testing the starting edge last.
inline TRmgHalfEdge* TRmgHalfEdge::findBoundaryWithZone(const TRmgZone* destination)
{
    TRmgHalfEdge* edge = this;
    do {
        edge = edge->m_next;
        if (edge->getOppositeZone() == destination)
            return edge;
    } while (edge != this);
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
    connection.m_borderGuard = false;
    connection.m_connected = connected;
    appendRmgTwoWayConnection(source->m_templateZone,
        destination->m_templateZone, connection);
}

// Extra-to-extra edges are completed unguarded connections when their boundary
// intersects the map. An extra-to-original edge must not shorten that original
// zone's distance to any other original zone; each one added triggers another
// graph relaxation.
VA(0x0053dad0, 0x57f)
MAC_ADDRESS(0x23f518, 0x40c)
void type_random_map_generator::joinExtraZones(s32 originalZones, TRmgVoronoi* diagram)
{
    TRmgZoneBounds bounds = {0, 0, m_map.m_mapWidth, m_map.m_mapHeight};
    for (s32 index = originalZones; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        TRmgMapPosition position = zone->getLevelPosition();
        TRmgHalfEdge* first = diagram->locate(position);
        for (s32 other = index + 1; other < m_zones.size(); ++other) {
            TRmgZone* destination = m_zones[other];
            if (destination->getLevelPosition().m_z != position.m_z)
                continue;
            TRmgHalfEdge* edge = first->findBoundaryWithZone(destination);
            if (!edge)
                continue;
            TPoint clipped = clipRmgBoundaryPoint(bounds, edge->m_vertex, edge->m_previous->m_vertex);
            if (bounds.contains(clipped))
                appendRmgExtraZoneConnection(zone, destination, true);
        }
    }
    initializeZoneDistances(originalZones);
    for (index = 0; index < originalZones; ++index)
        propagateZoneDistances(m_zones[index]);

    for (index = originalZones; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        TRmgMapPosition position = zone->getLevelPosition();
        TRmgHalfEdge* first = diagram->locate(position);
        for (s32 other = 0; other < originalZones; ++other) {
            TRmgZone* destination = m_zones[other];
            if (destination->getLevelPosition().m_z != position.m_z)
                continue;
            TRmgHalfEdge* edge = first->findBoundaryWithZone(destination);
            if (!edge)
                continue;
            s32 column;
            for (column = 0; column < originalZones; ++column) {
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

VA_COMPGEN(0x0054de90, 0x14, STD_CONSTRUCT, TRmgZoneConnection)

// Per axis, a radial site lies twice halfOffset from its zone; it may overhang
// only the map edge it points toward, by about halfOffset.
static inline bool isRmgRadialSiteTooFarOffMap(s32 coordinate, s32 extent, double halfOffset)
{
    return (coordinate < 0 && coordinate < halfOffset)
        || (coordinate >= extent && coordinate >= extent + halfOffset);
}

// Existing zones seed the subdivision; radial sites add surface water zones
// and underground boundaries.
VA(0x0053e050, 0x64d)
MAC_ADDRESS(0x23f924, 0x798)
void type_random_map_generator::buildZoneBoundaries(
    TRmgTemplate* mapTemplate, s32 level)
{
    TRmgVoronoi diagram;
    for (s32 zone = 0; zone < m_zones.size(); ++zone) {
        TRmgMapPosition position = m_zones[zone]->getLevelPosition();
        if (position.m_z == level)
            diagram.addSite(position, m_zones[zone]);
    }
    s32 originalZones = m_zones.size();
    if (level == RMG_UNDERGROUND_LEVEL || m_waterContent != RMG_WATER_NONE) {
        // Retail bug: testSlot's town flags are uninitialized stack, nonzero in
        // practice, so one unused town is drawn. All true keeps that rand().
        TRmgTemplateZone testSlot;
        testSlot.m_zoneIndex = RMG_NO_ZONE;
        testSlot.m_kind = RMG_TEMPLATE_JUNCTION;
        testSlot.m_size = 0;
#if defined(HOMM3_RMG_HOTFIX)
        // The placement probe needs no town: allowing none skips the draw.
        memset(testSlot.m_allowedTowns, false, sizeof(testSlot.m_allowedTowns));
#else
        memset(testSlot.m_allowedTowns, true, sizeof(testSlot.m_allowedTowns));
#endif
        TRmgZone testZone(&testSlot);
        TRmgZone* addedZone = 0;
        for (zone = 0; zone < originalZones; ++zone) {
            TRmgZone* current = m_zones[zone];
            TRmgMapPosition center = current->getLevelPosition();
            if (center.m_z != level)
                continue;
            s32 radius = current->m_scaledSize;
            testSlot.m_size = radius;
            TRmgMapPosition position = center;
            // Eight radial sites, 45 degrees apart.
            for (s32 direction = 0; direction < RMG_RADIAL_DIRECTION_COUNT; direction += 4) {
                double dx = radius * g_rmgDirectionCosines[direction];
                position.m_x = static_cast<s32>(center.m_x + dx * 2);
                double dy = radius * g_rmgDirectionSines[direction];
                position.m_y = static_cast<s32>(center.m_y + dy * 2);
                if (isRmgRadialSiteTooFarOffMap(position.m_x, m_map.m_mapWidth, dx)
                    || isRmgRadialSiteTooFarOffMap(position.m_y, m_map.m_mapHeight, dy))
                    continue;
                testZone.setLevelPosition(position);
                if (!canPlaceZone(&testZone))
                    continue;
                if (position.m_z == RMG_SURFACE_LEVEL) {
                    // Retail bug: m_allowedTowns is uninitialized heap, nonzero in
                    // practice, so one unused town is drawn. All true keeps that rand().
                    TRmgTemplateZone* templateZone = new TRmgTemplateZone;
#if defined(HOMM3_RMG_HOTFIX)
                    // A water zone has no town; its other unwritten flags are cleared too.
                    memset(templateZone->m_allowedTowns, false, sizeof(templateZone->m_allowedTowns));
                    templateZone->m_neutralTownsMatchZone = false;
                    templateZone->m_useNativeTerrain = false;
                    templateZone->m_guardsMatchZone = false;
#else
                    memset(templateZone->m_allowedTowns, true, sizeof(templateZone->m_allowedTowns));
#endif
                    templateZone->m_zoneIndex = mapTemplate->m_zones.size();
                    templateZone->m_size = radius;
                    memset(templateZone->m_allowedMonsters, 0, sizeof(templateZone->m_allowedMonsters));
                    memset(templateZone->m_allowedTerrain, 0, sizeof(templateZone->m_allowedTerrain));
                    memset(templateZone->m_mineCounts, 0, sizeof(templateZone->m_mineCounts));
                    memset(templateZone->m_mineDensities, 0, sizeof(templateZone->m_mineDensities));
                    memset(templateZone->m_townPlacement, 0, sizeof(templateZone->m_townPlacement));
                    templateZone->m_monsterStrength = RMG_ZONE_MONSTERS_NONE;
                    templateZone->m_playerIndex = -1;
                    memset(templateZone->m_treasure, 0, sizeof(templateZone->m_treasure));
                    templateZone->m_treasure[0].m_density = 5;
                    templateZone->m_treasure[0].m_maximum = 1000;
                    templateZone->m_treasure[0].m_minimum = 100;
                    templateZone->m_treasure[1].m_density = 1;
                    templateZone->m_treasure[1].m_maximum = 6000;
                    templateZone->m_treasure[1].m_minimum = 2000;
                    templateZone->m_kind = RMG_TEMPLATE_JUNCTION;
                    addedZone = new TRmgZone(templateZone);
#if defined(HOMM3_RMG_HOTFIX)
                    // chooseTownType never runs for added zones.
                    addedZone->m_creatureTownType = eTownNeutral;
#endif
                    addedZone->m_terrain = eTerrainWater;
                    addedZone->setLevelPosition(position);
                    mapTemplate->m_zones.push_back(templateZone);
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

VA(0x0053e6a0, 0x337)
MAC_ADDRESS(0x2400bc, 0x354)
void type_random_map_generator::paintZoneTerrain()
{
    calculateZoneBounds();
    for (s32 zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
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
    s32 progressSteps = 15800 / m_zones.size();
    for (zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgZoneBounds bounds = zone->getBounds();
        TRmgMapPosition position = zone->getLevelPosition();
        if (zone->getTerrain() != eTerrainWater) {
            type_random_map levelMap(m_map.getMapItem(0, 0, position.m_z),
                m_map.getWidth(), m_map.getHeight());
            TRmgTerrainBrush brush(&levelMap, zone->getTerrain(), RMG_BRUSH_STRENGTH);
            for (s32 y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
                for (s32 x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
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

// Each nondegenerate quadrant inherits the region's variation. Quadrants are
// pushed in the order shown, with x growing right and y down. Each keeps its
// outer region corner; the centre value and the two edge midpoints beside it
// become its other corners.
//          minY
//   minX   4 | 2   maxX
//          --+--
//          3 | 1
//          maxY
VA(0x0053e9e0, 0x31e)
MAC_ADDRESS(0x240410, 0x2ac)
void subdivideRmgNoiseRegion(std::vector<TRmgNoiseRegion>& pending,
    TRmgNoiseRegion region,
    TRmgNoiseMidpoints midpoints,
    s32 centerValue)
{
    s32 middleY = (region.m_bounds.m_minimumY + region.m_bounds.m_maximumY) / 2;
    s32 middleX = (region.m_bounds.m_minimumX + region.m_bounds.m_maximumX) / 2;
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

VA_COMPGEN(0x0054c670, 0x21, VECTOR_SIZE, TRmgNoiseRegion)

VA_COMPGEN(0x0054d5c0, 0x2e4, VECTOR_INSERT_COUNT, TRmgNoiseRegion)

VA_COMPGEN(0x0054d960, 0x3b, VECTOR_UCOPY, TRmgNoiseRegion)

VA_COMPGEN(0x0054d9a0, 0x31, VECTOR_UFILL, TRmgNoiseRegion)

TRmgZoneConnection* TRmgTemplateZone::findConnection(s32 destinationZone)
{
    for (u32 connectionIndex = 0; connectionIndex < m_connections.size(); ++connectionIndex) {
        if (m_connections[connectionIndex].m_destination->m_zoneIndex == destinationZone)
            return &m_connections[connectionIndex];
    }
    return 0;
}

void __fastcall generateRmgIslandMask(u8* mask, s32 width, s32 height);

VA(0x0053ed00, 0x29b)
MAC_ADDRESS(0x2406bc, 0x484)
void __fastcall generateRmgIslandMask(u8* mask, s32 width, s32 height)
{
    std::vector<TRmgNoiseRegion> pending;
    // This noise grid uses bounds X for rows and Y for columns:
    // subdivision receives (height, width), and output uses X*width + Y.
    TRmgNoiseRegion region = {
        { 0, 0, height, width }, { 0, 0, 0, 0 }, (height + width) / 4 + 1
    };
    TRmgNoiseMidpoints midpoints;
    midpoints.m_minYValue = 0;
    midpoints.m_maxYValue = 0;
    midpoints.m_minXValue = 0;
    midpoints.m_maxXValue = 0;
    subdivideRmgNoiseRegion(pending, region, midpoints, region.m_variation / 2);
    while (pending.size()) {
        region = pending.back();
        pending.pop_back();
        if (region.m_bounds.m_maximumY == region.m_bounds.m_minimumY + 1
            && region.m_bounds.m_maximumX == region.m_bounds.m_minimumX + 1) {
            if (region.m_bounds.m_minimumX < 0 || region.m_bounds.m_minimumX >= height
                || region.m_bounds.m_minimumY < 0 || region.m_bounds.m_minimumY >= width)
                continue;
            s32 value = min(max(region.m_corners[RMG_NOISE_MIN_X_MIN_Y], 0), 255);
            mask[region.m_bounds.m_minimumX * width + region.m_bounds.m_minimumY] = value;
            continue;
        }
        if (region.m_bounds.m_maximumX < 0 || region.m_bounds.m_maximumY < 0
            || region.m_bounds.m_minimumX >= height || region.m_bounds.m_minimumY >= width)
            continue;
        const s32* corners = region.m_corners;
        midpoints.m_minXValue = (corners[RMG_NOISE_MIN_X_MAX_Y] + corners[RMG_NOISE_MIN_X_MIN_Y]) / 2;
        midpoints.m_minYValue = (corners[RMG_NOISE_MAX_X_MIN_Y] + corners[RMG_NOISE_MIN_X_MIN_Y]) / 2;
        midpoints.m_maxXValue = (corners[RMG_NOISE_MAX_X_MAX_Y] + corners[RMG_NOISE_MAX_X_MIN_Y]) / 2;
        midpoints.m_maxYValue = (corners[RMG_NOISE_MAX_X_MAX_Y] + corners[RMG_NOISE_MIN_X_MAX_Y]) / 2;
        s32 center = (corners[RMG_NOISE_MAX_X_MAX_Y] + corners[RMG_NOISE_MAX_X_MIN_Y]
            + corners[RMG_NOISE_MIN_X_MAX_Y] + corners[RMG_NOISE_MIN_X_MIN_Y]) / 4;
        s32 variation = region.m_variation;
        if (variation > 1) {
            midpoints.m_minXValue += getRmgCenteredRandomOffset(variation);
            midpoints.m_minYValue += getRmgCenteredRandomOffset(variation);
            midpoints.m_maxXValue += getRmgCenteredRandomOffset(variation);
            midpoints.m_maxYValue += getRmgCenteredRandomOffset(variation);
            center += getRmgCenteredRandomOffset(variation);
        }
        region.m_variation = (variation - 1) / 2 + 1;
        subdivideRmgNoiseRegion(pending, region, midpoints, center);
    }
}

VA(0x0053efa0, 0x1f2)
MAC_ADDRESS(0x240ba8, 0x270)
void type_random_map_generator::createWaterZoneIsland(const TRmgZoneBounds& bounds, s32 level)
{
    s32 width = bounds.m_maximumX - bounds.m_minimumX;
    s32 height = bounds.m_maximumY - bounds.m_minimumY;
    u8* mask = new u8[width * height];
    TRmgMapPosition point;
    s32 terrain = rand() % eTerrainSubterranean; // dirt to rough
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
                item->markObstacleFill();
            }
        }
    }
    delete[] mask;
    if (m_progress)
        m_progress->advance(1000);
}

// Chamfer distance of one step: 2 to a cardinal neighbour, 3 to a diagonal.
static inline s32 getRmgChamferStepCost(s32 direction)
{
    return isRmgDiagonalDirection(direction) ? 3 : 2;
}

// Eight-neighbour chamfer distances within one zone; a shorter distance
// resets connection metadata without checking terrain or objects.
VA(0x0053f1a0, 0x2c6)
MAC_ADDRESS(0x240ee4, 0x358)
void type_random_map_generator::floodWaterZoneDistances(TRmgMapPosition position, s32 zoneIndex)
{
    std::vector<TRmgMapPosition> positions;
    std::vector<s32> costs;
    positions.push_back(position);
    costs.push_back(0);
    TRmgMapItem* seed = m_map.getMapItem(position);
    seed->setWaterZoneDistance(0, RMG_DIRECTION_EAST);
    while (positions.size()) {
        popRmgWorkItem(position, positions, costs);
        u32 currentCost = m_map.getMapItem(position)->m_movement.m_zonePathCost;
        for (s32 direction = RMG_DIRECTION_EAST; direction < RMG_DIRECTION_COUNT; ++direction) {
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

// Island radius in tiles, and the clearance (in m_zonePathCost units, 2 per
// cardinal step) an island centre keeps from the zone edge and earlier
// island centres.
enum ERmgWaterZoneIslandLimits {
    RMG_ISLAND_MINIMUM_RADIUS = 3,
    RMG_ISLAND_MAXIMUM_RADIUS = 6,
    RMG_ISLAND_CLEARANCE = 20
};

// Seed islands clear of the zone edge and earlier island centres, rebuilding
// the candidate list after every island.
VA(0x0053f470, 0x409)
MAC_ADDRESS(0x24123c, 0x638)
void type_random_map_generator::placeWaterZoneIslands(TRmgZone* zone)
{
    if (zone->m_terrain != eTerrainWater)
        return;
    TRmgZoneBounds bounds = zone->m_bounds;
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition position = zone->m_levelPosition;
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position);
            item->setWaterZoneDistance(RMG_UNREACHED_COST, RMG_DIRECTION_EAST);
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
    for (;;) {
        std::vector<TRmgMapPosition> candidates;
        for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
            for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
                if (m_map.getMapItem(position)->m_movement.m_zonePathCost
                    >= RMG_ISLAND_CLEARANCE)
                    candidates.push_back(position);
            }
        }
        if (!candidates.size())
            break;
        position = candidates[rand() % candidates.size()];
        TRmgMapItem* selectedItem = m_map.getMapItem(position);
        s32 range = selectedItem->m_movement.m_zonePathCost / 3 - 5;
        s32 radius = rand() % range + RMG_ISLAND_MINIMUM_RADIUS;
        if (radius > RMG_ISLAND_MAXIMUM_RADIUS)
            radius = RMG_ISLAND_MAXIMUM_RADIUS;
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
inline void type_random_map::releaseNeighborhoodPathClearance(
    const TRmgMapPosition& center, s32 radius)
{
    TRmgZoneBounds bounds;
    getNeighborhoodBounds(bounds, center, radius);
    TRmgMapPosition nearby;
    nearby.m_z = center.m_z;
    for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
        for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
            TRmgMapItem* item = getMapItem(nearby);
            if (!item->hasObjects())
                item->releasePathClearance();
        }
    }
}

// Marks dry zone cells for border obstacles beside unassigned water or
// another zone, unless that zone is joined by an unguarded surface connection.
VA(0x0053f880, 0x429)
MAC_ADDRESS(0x241874, 0x628)
void type_random_map_generator::markZoneBorders()
{
    TRmgMapItem* current = m_map.m_mapItems;
    TRmgMapPosition position;
    TRmgMapPosition nearby;
    for (position.m_z = RMG_SURFACE_LEVEL; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++current) {
                s32 zoneIndex = current->m_zoneState.m_zone;
                if (zoneIndex < 0 || current->getLandType() == eTerrainWater)
                    continue;
                TRmgZoneBounds bounds;
                m_map.getNeighborhoodBounds(bounds, position, RMG_NEIGHBORHOOD_3X3);
                TRmgZone* zone = m_zones[zoneIndex];
                b8 needsBorder = false;
                nearby.m_z = position.m_z;
                for (nearby.m_y = bounds.m_minimumY; nearby.m_y < bounds.m_maximumY; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX; ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        s32 otherZone = item->m_zoneState.m_zone;
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
                current->markObstacleFill();
                m_map.releaseNeighborhoodPathClearance(position, RMG_NEIGHBORHOOD_3X3);
            }
        }
    }
    if (m_progress)
        m_progress->advance(1600);
}

VA(0x0053fcb0, 0x5ec)
MAC_ADDRESS(0x241e9c, 0x770)
void type_random_map_generator::repairWaterZoneBorders()
{
    TRmgMapItem* current = m_map.m_mapItems;
    TTerrainType terrain;
    TRmgMapPosition position;
    TRmgMapPosition nearby;
    std::vector<TRmgMapPosition> positions;
    std::vector<TTerrainType> terrains;
    for (position.m_z = RMG_SURFACE_LEVEL; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++current) {
                s32 zoneIndex = current->m_zoneState.m_zone;
                if (zoneIndex < 0 || current->getLandType() != eTerrainWater)
                    continue;
                s32 destinationZone = current->m_zoneState.m_connectionZone;
                if (destinationZone < 0)
                    continue;

                b8 foundLandTerrain = false;
                TRmgZoneBounds bounds;
                m_map.getNeighborhoodBounds(bounds, position, RMG_NEIGHBORHOOD_3X3);
                nearby.m_z = position.m_z;
                TRmgZone* zone = m_zones[zoneIndex];
                for (nearby.m_y = bounds.m_minimumY;
                     nearby.m_y < bounds.m_maximumY && !foundLandTerrain; ++nearby.m_y) {
                    for (nearby.m_x = bounds.m_minimumX; nearby.m_x < bounds.m_maximumX;
                         ++nearby.m_x) {
                        TRmgMapItem* item = m_map.getMapItem(nearby);
                        if (item->getLandType() != eTerrainWater
                            && item->isPassableLand()
                            && !item->hasObstacleFill()) {
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
                        item->markObstacleFill();
                        if (item->getLandType() == eTerrainWater) {
                            positions.push_back(nearby);
                            terrains.push_back(terrain);
                        }
                    }
                }

                m_map.releaseNeighborhoodPathClearance(position, RMG_NEIGHBORHOOD_5X5);
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
VA(0x005402a0, 0x32a)
MAC_ADDRESS(0x24260c, 0x434)
void type_random_map_generator::addObject(type_object* object, TRmgMapPosition position)
{
    TRmgGeneratorBase::addObject(object, position);
    TObjectType* prototype = object->m_properties->m_prototype;
    TAdventureObjectType objectType = prototype->getObjectType();
    ++m_objectCountByType[objectType];
    if (prototype->m_hasTrigger) {
        TObjectType::TPoint trigger = prototype->m_triggerCell;
        std::vector<TRmgMapPosition> positions;
        std::vector<s32> costs;
        TRmgMapPosition currentPosition = getRmgObjectTriggerPosition(position, trigger);
        TRmgMapItem* seed = m_map.getMapItem(currentPosition);
        s32 zoneIndex = seed->m_zoneState.m_zone;
        if (zoneIndex >= 0)
            ++m_zones[zoneIndex]->m_objectCountByType[objectType];
        seed->m_zoneState.m_objectDistance = 0;
        positions.push_back(currentPosition);
        costs.push_back(0);
        while (positions.size()) {
            popRmgWorkItem(currentPosition, positions, costs);
            s32 distance = m_map.getMapItem(currentPosition)->m_zoneState.m_objectDistance;
            for (s32 direction = RMG_DIRECTION_EAST; direction < RMG_DIRECTION_COUNT; ++direction) {
                s32 nextCost = distance + getRmgChamferStepCost(direction);
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

// openConnectionPath's narrow argument; wide paths also clear nearby
// same-zone obstacle marks.
enum ERmgConnectionPathWidth {
    RMG_WIDE_CONNECTION_PATH = false,
    RMG_NARROW_CONNECTION_PATH = true
};

// Floods path costs from a seed cell in each zone. Every clear, dry zone cell
// still at nonzero cost then opens a path back along the costs (if reached)
// and reseeds the flood from itself.
VA(0x005405d0, 0x304)
MAC_ADDRESS(0x242a40, 0x448)
void type_random_map_generator::buildZoneConnectionPaths()
{
    s32 count = m_map.m_numberLevels * m_map.m_mapHeight * m_map.m_mapWidth;
    TRmgMapItem* item = m_map.m_mapItems;
    while (count--) {
        item->m_movement.m_zonePathCost = RMG_UNREACHED_COST;
        item->m_tileData.m_connectionDirection = RMG_DIRECTION_EAST;
        item->m_zoneState.m_connectionZone = RMG_NO_ZONE;
        item->resetMovement();
        ++item;
    }
    // Retail bug: a zone whose scan finds no eligible empty cell (in practice
    // a zone with no cells) floods from the seed of the last zone that had one.
    TRmgMapPosition seed(RMG_NO_POSITION, RMG_NO_POSITION, RMG_NO_POSITION);
    for (u32 zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgZoneBounds bounds = zone->m_bounds;
        s32 level = zone->m_levelPosition.m_z;
        b8 foundClearPath = false;
#if defined(HOMM3_RMG_HOTFIX)
        b8 foundSeed = false;
#endif
        for (s32 y = bounds.m_minimumY; y < bounds.m_maximumY && !foundClearPath; ++y) {
            for (s32 x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
                TRmgMapItem* current = m_map.getMapItem(x, y, level);
                if (current->m_zoneState.m_zone == zoneIndex) {
                    u32 terrain = current->getLandType();
                    if ((terrain != eTerrainWater || zone->m_terrain == terrain)
                        && !current->hasObjects()) {
                        seed = TRmgMapPosition(x, y, level);
#if defined(HOMM3_RMG_HOTFIX)
                        foundSeed = true;
#endif
                        if (current->hasPathClearance() && current->isPassableLand()) {
                            foundClearPath = true;
                            break;
                        }
                    }
                }
            }
        }
#if defined(HOMM3_RMG_HOTFIX)
        // A zone without an eligible cell gets no connection paths.
        if (!foundSeed)
            continue;
#else
        // Retail bug: before any seed exists, retail floods from stack garbage.
        if (seed.m_x == RMG_NO_POSITION)
            continue;
#endif
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
                    openConnectionPath(pathPosition, RMG_WIDE_CONNECTION_PATH);
                    m_map.floodConnectionCosts(pathPosition, zone->m_terrain == eTerrainWater);
                }
            }
        }
    }
}

// Widen an opened route by clearing same-zone obstacle marks around the
// cell, keeping border-connection cells.
inline void type_random_map::clearNearbyObstacleFill(const TPoint& center, s32 level,
    s32 zoneIndex)
{
    TRmgZoneBounds bounds;
    getNeighborhoodBounds(bounds, center, RMG_NEIGHBORHOOD_3X3);
    for (s32 y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
        for (s32 x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
            TRmgMapItem* nearby = getMapItem(x, y, level);
            if (nearby->m_zoneState.m_zone == zoneIndex)
                nearby->clearObstacleFill();
        }
    }
}

// From a reached cell, follow predecessors to the zero-cost seed without
// changing costs; border-connection cells on the way get a border guard of
// their colour. Widened routes clear only the obstacle mark of nearby
// same-zone cells.
VA(0x005408e0, 0x23f)
MAC_ADDRESS(0x242e88, 0x380)
void type_random_map_generator::openConnectionPath(
    TRmgMapPosition position, b8 narrow)
{
    TRmgMapItem* item = m_map.getMapItem(position);
    s32 zone = item->m_zoneState.m_zone;
    if (item->m_movement.m_cost >= RMG_REACHED_COST_LIMIT)
        return;
    while (item->m_movement.m_cost > 0) {
        if (item->m_borderConnection.m_present) {
            TRmgObjectPropertiesRef* properties = selectObjectPrototype(
                eTerrainDirt, BORDER_GUARD, item->m_borderConnection.m_guardColor);
            type_object* object = new type_object(properties);
            item->clearBorderConnection();
            addObject(object, position);
        }
        item->openPath();
        TRmgMapPosition previous = item->m_previousTile;
        if (!narrow)
            m_map.clearNearbyObstacleFill(position, position.m_z, zone);
        position = previous;
        item = m_map.getMapItem(position);
    }
}

enum ERmgGuardPrototype {
    RMG_NO_GUARD_PROTOTYPE = -1
};

// Picks a random allowed creature for a guard of the given value. Retail bug
// on RoE maps: creature 117 is neither evaluated nor excluded, so it can be
// selected without being counted. Stacks of four or more get two draws of
// size variation.
VA(0x00540b20, 0x240)
MAC_ADDRESS(0x243208, 0x290)
type_object* type_random_map_generator::createGuard(s32 value, TRmgZone* zone)
{
    // Indexed by town type + 1; entry zero is neutral creatures.
    b8 allowedFactions[TOWN_TYPE_COUNT + 1];
    if (zone->m_templateZone->m_guardsMatchZone && zone->m_alignment != eTownNeutral) {
        memset(allowedFactions, 0, sizeof(allowedFactions));
        allowedFactions[zone->m_alignment + 1] = true;
    } else {
        memcpy(allowedFactions, zone->m_templateZone->m_allowedMonsters, sizeof(allowedFactions));
    }
    // Monster prototype per creature type; filling every byte with 0xff
    // starts each entry at RMG_NO_GUARD_PROTOTYPE.
    s32 prototypeIndices[RMG_CREATURE_TYPE_COUNT];
    memset(prototypeIndices, -1, sizeof(prototypeIndices));
    for (u32 index = 0; index < m_objectPrototypes[MONSTER].size(); ++index) {
        TRmgObjectPropertiesRef* properties = m_objectPrototypes[MONSTER][index];
        prototypeIndices[properties->m_prototype->getSubtype()] = index;
    }
    s32 eligibleCreatureCount = 0;
    s32 lastEvaluated = RMG_CREATURE_TYPE_COUNT - 1;
    if (m_mapVersion < RMG_MAP_ARMAGEDDONS_BLADE) {
        for (s32 expansion = RMG_CREATURE_TYPE_COUNT - 1;
             expansion >= RMG_ROE_CREATURE_TYPE_COUNT; --expansion)
            prototypeIndices[expansion] = RMG_NO_GUARD_PROTOTYPE;
        lastEvaluated = RMG_ROE_CREATURE_TYPE_COUNT - 2; // skips 117 (above)
    }
    s32 creature;
    for (creature = lastEvaluated; creature >= 0; --creature) {
        const TCreatureTypeTraits& traits = g_creatureTypeTraits[creature];
        if ((traits.m_wanderingHigh + traits.m_wanderingLow) / 2 * traits.m_aiValue <= value
            && value <= traits.m_aiValue * RMG_GUARD_MAXIMUM_COUNT
            && traits.m_level >= 0 && allowedFactions[traits.m_townType + 1]) {
            // Retail bug: counted even without a loaded prototype, while
            // selection only picks loaded prototypes.
            ++eligibleCreatureCount;
        } else {
            prototypeIndices[creature] = RMG_NO_GUARD_PROTOTYPE;
        }
    }
    if (!eligibleCreatureCount)
        return 0;
    s32 selectionRank = rand() % eligibleCreatureCount;
    for (creature = RMG_CREATURE_TYPE_COUNT - 1; creature >= 0; --creature) {
        if (prototypeIndices[creature] >= 0 && --selectionRank < 0)
            break;
    }
#if defined(HOMM3_RMG_HOTFIX)
    // A rank beyond the loaded prototypes selects no guard.
    if (creature < 0)
        return 0;
#else
    // Retail bug: if the counts disagree, creature can reach -1.
#endif
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[MONSTER][prototypeIndices[creature]];
    s32 aiValue = g_creatureTypeTraits[creature].m_aiValue;
    s32 creatureCount = (value + aiValue / 2) / aiValue;
    s32 countVariation = creatureCount / 4 + 1;
    if (countVariation > 1) {
        creatureCount = creatureCount + (rand() % countVariation - rand() % countVariation);
    }
    return new rmgMonsterObject(properties, m_nextObjectId++, creatureCount);
}

// First prototype of the requested subtype, or size() when missing.
static inline u32 findRmgPrototypeSubtypeIndex(
    std::vector<TRmgObjectPropertiesRef*>& prototypes, s32 subtype)
{
    u32 index = 0;
    while (index < prototypes.size()
        && prototypes[index]->m_prototype->getSubtype() != subtype)
        ++index;
    return index;
}

// placeBorderObject's result when no keymaster's tent could be placed;
// otherwise it returns the guard colour.
enum ERmgBorderPlacement {
    RMG_BORDER_NOT_PLACED = -1
};

// Border guards placed side by side, eastward from the given cell.
enum ERmgBorderGuardCount {
    RMG_SINGLE_BORDER_GUARD = 1,
    // One per cell of the row below a three-tile shipyard.
    RMG_SHIPYARD_BORDER_GUARDS = 3
};

VA(0x00540d60, 0x256)
MAC_ADDRESS(0x24356c, 0x2b8)
s32 type_random_map_generator::placeBorderObject(
    TRmgMapPosition position, s32 guardCount, TRmgZone* keyTentZone)
{
    s32 color = m_nextKeyTentColor;
    u32 tentPrototypeIndex = findRmgPrototypeSubtypeIndex(m_objectPrototypes[BORDER_TENT], color);
    if (tentPrototypeIndex == m_objectPrototypes[BORDER_TENT].size())
        return RMG_BORDER_NOT_PLACED;
    TRmgObjectPropertiesRef* tentProperties = m_objectPrototypes[BORDER_TENT][tentPrototypeIndex];

    u32 guardPrototypeIndex = findRmgPrototypeSubtypeIndex(m_objectPrototypes[BORDER_GUARD], color);
    // Retail bug: missing guard art returns colour zero, which callers treat
    // as success, unlike RMG_BORDER_NOT_PLACED for missing tent art.
    if (guardPrototypeIndex == m_objectPrototypes[BORDER_GUARD].size())
        return 0;
    TRmgObjectPropertiesRef* guardProperties = m_objectPrototypes[BORDER_GUARD][guardPrototypeIndex];
    type_object* tent = new type_object(tentProperties);
    if (!placeObjectInZone(tent, keyTentZone)) {
        delete tent;
        return RMG_BORDER_NOT_PLACED;
    }

    for (s32 guardIndex = 0; guardIndex < guardCount; ++guardIndex) {
        type_object* guard = new type_object(guardProperties);
        TRmgMapItem* item = m_map.getMapItem(position);
        item->clearBorderConnection();
        addObject(guard, position);
        ++position.m_x;
    }

    setKeyTentColorDisabled(color, true);
    return color;
}

// Marks the cell as a border connection unless an object covers it.
inline void TRmgMapItem::markEmptyBorderConnection(s32 color)
{
    if (!hasObjects()) {
        markBorderConnection(color);
    }
}

VA(0x00540fc0, 0x172)
MAC_ADDRESS(0x243824, 0x2d4)
void type_random_map_generator::markBorderObjectArea(
    TRmgMapPosition position, s32 color)
{
    TRmgZoneBounds bounds;
    m_map.getNeighborhoodBounds(bounds, position, RMG_NEIGHBORHOOD_3X3);
    for (s32 y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
        for (s32 x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
            TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
            item->markEmptyBorderConnection(color);
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
void type_random_map_generator::placeGuard(s32 value, TRmgMapPosition position)
{
#if defined(HOMM3_RMG_HOTFIX)
    // No guard off the map or outside every zone.
    if (!m_map.containsXY(position))
        return;
    TRmgMapItem* item = m_map.getMapItem(position);
    if (item->m_zoneState.m_zone == RMG_NO_ZONE)
        return;
#else
    TRmgMapItem* item = m_map.getMapItem(position);
    // Retail bug: an unassigned cell indexes m_zones[-1].
#endif
    TRmgZone* zone = m_zones[item->m_zoneState.m_zone];
    if (item->hasObjects())
        return;
    type_object* guard = createGuard(value, zone);
    if (guard)
        addObject(guard, position);
}

// A successful ground border-guard placement marks its surrounding area and
// suppresses guards for this and all remaining crossings.
inline void type_random_map_generator::placeGroundConnectionBorderGuard(
    TRmgMapPosition position, TRmgZone* keyTentZone, s32& guardValue)
{
    s32 color = placeBorderObject(position, RMG_SINGLE_BORDER_GUARD, keyTentZone);
    if (color >= 0) {
        markBorderObjectArea(position, color);
        guardValue = 0;
    }
}

// Places a border guard on the approach cell below the gate's entrance. On
// success both gate guards are dropped and the border connection extends to
// the empty cells either side of it.
inline void type_random_map_generator::placeGateConnectionBorderGuard(
    TRmgMapPosition approach, TRmgZone* keyTentZone, s32& guardValue)
{
    s32 color = placeBorderObject(approach, RMG_SINGLE_BORDER_GUARD, keyTentZone);
    if (color >= 0) {
        guardValue = 0;
        TRmgMapPosition side = approach;
        --side.m_x;
        m_map.getMapItem(side)->markEmptyBorderConnection(color);
        side.m_x += 2;
        m_map.getMapItem(side)->markEmptyBorderConnection(color);
    }
}

// Unguarded template connections need no guard value.
inline s32 type_random_map_generator::getConnectionGuardValue(
    const TRmgZoneConnection* connection) const
{
    if (connection->m_unguarded)
        return 0;
    s32 strength = m_monsterStrength;
    s32 value = connection->m_value;
    return getRmgGuardValue(value, strength);
}

// Best-score candidate lists (highest, then lowest). Callers pass only scores
// at least as good as the bound; a strictly better score restarts the list
// and becomes the new bound, and a tie is appended.
static inline void addRmgHighestScoreCandidate(
    std::vector<TRmgMapPosition>& candidates, const TRmgMapPosition& position,
    s32 score, s32& highestScore)
{
    if (score > highestScore) {
        highestScore = score;
        candidates.clear();
    }
    candidates.push_back(position);
}

static inline void addRmgLowestScoreCandidate(
    std::vector<TRmgMapPosition>& candidates, const TRmgMapPosition& position,
    s32 score, s32& lowestScore)
{
    if (score < lowestScore) {
        lowestScore = score;
        candidates.clear();
    }
    candidates.push_back(position);
}

// Adds the object at a uniformly drawn candidate and returns that position.
// The candidate list must be nonempty.
inline TRmgMapPosition type_random_map_generator::addObjectAtRandomCandidate(
    type_object* object, const std::vector<TRmgMapPosition>& candidates)
{
    TRmgMapPosition position = candidates[rand() % candidates.size()];
    addObject(object, position);
    return position;
}

enum ERmgGroundCrossingLimits {
    RMG_BORDER_CELLS_PER_CROSSING = 40,
    RMG_MAXIMUM_CROSSING_COST = 100
};

// Connects land zones on one level. Opens one crossing per 40 eligible
// border cells (rounded up), drawn without repeats from the empty ones tied
// for the lowest zone-path cost, which must be at most 100; each gets open
// paths and entrances on both sides before its border guard or monster guard.
VA(0x00541140, 0x63a)
MAC_ADDRESS(0x243af8, 0x53c)
b8 type_random_map_generator::createGroundConnection(
    TRmgZone* source,
    TRmgZoneConnection* connection,
    std::vector<TRmgMapItem*>* borderItems,
    std::vector<TRmgMapPosition>* borderPositions)
{
    TRmgTemplateZone* sourceTemplateZone = source->m_templateZone;
    TRmgTemplateZone* destinationTemplateZone = connection->m_destination;
    s32 sourceZone = sourceTemplateZone->m_zoneIndex;
    TRmgZone* destination = m_zones[destinationTemplateZone->m_zoneIndex];
    s32 destinationZone = destination->m_templateZone->m_zoneIndex;
    if (source->getLevelPosition().m_z != destination->getLevelPosition().m_z)
        return false;
    if (source->m_terrain == eTerrainWater)
        return false;
    if (destination->m_terrain == eTerrainWater)
        return false;

    std::vector<TRmgMapPosition> candidates;
    s32 eligibleCount = 0;
    s32 bestCost = RMG_MAXIMUM_CROSSING_COST;
    for (s32 index = 0; index < borderItems->size(); ++index) {
        TRmgMapItem* item = (*borderItems)[index];
        if (item->m_zoneState.m_zone == sourceZone
            && item->m_zoneState.m_connectionZone == destinationZone
            && !item->hasObjects()) {
            TRmgMapPosition other = (*borderPositions)[index]
                + g_rmgDirections[item->m_tileData.m_connectionDirection];
            if (!m_map.getMapItem(other)->hasObjects()) {
                ++eligibleCount;
                s32 cost = item->m_movement.m_zonePathCost;
                if (cost <= bestCost)
                    addRmgLowestScoreCandidate(candidates, (*borderPositions)[index], cost, bestCost);
            }
        }
    }

    if (candidates.size() == 0)
        return false;

    s32 guardValue = getConnectionGuardValue(connection);

    // Retail dead path: floodConnectionCosts sets crossing costs to 10 or more.
    if (bestCost == 1 && guardValue == 0 && !connection->m_borderGuard)
        return true;

    s32 count = min(candidates.size(),
        (eligibleCount + RMG_BORDER_CELLS_PER_CROSSING - 1) / RMG_BORDER_CELLS_PER_CROSSING);
    for (s32 crossing = 0; crossing < count; ++crossing) {
        s32 selected = rand() % candidates.size();
        TRmgMapPosition position = candidates[selected];
        TPoint direction = g_rmgDirections[
            m_map.getMapItem(position)->m_tileData.m_connectionDirection];
        TRmgMapPosition otherPosition = position + direction;

        openConnectionPath(position, connection->m_borderGuard);
        source->m_entrances.push_back(TPoint(position.m_x, position.m_y));
        openConnectionPath(otherPosition, connection->m_borderGuard);
        destination->m_entrances.push_back(TPoint(otherPosition.m_x, otherPosition.m_y));
        candidates.erase(candidates.begin() + selected);

        if (connection->m_borderGuard) {
            placeGroundConnectionBorderGuard(position, destination, guardValue);
            placeGroundConnectionBorderGuard(otherPosition, source, guardValue);
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
VA(0x00541780, 0x18d)
MAC_ADDRESS(0x244034, 0x254)
void type_random_map_generator::floodConnectionRegion(TRmgMapPosition position)
{
    std::vector<TRmgMapPosition> openPositions;
    openPositions.push_back(position);
    m_map.getMapItem(position)->setConnectionVisited();
    while (openPositions.size()) {
        position = openPositions.back();
        openPositions.pop_back();
        for (s32 direction = RMG_DIRECTION_EAST; direction < RMG_DIRECTION_COUNT;
             direction += RMG_CARDINAL_DIRECTION_STEP) {
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
    s32 waterOffset);

VA(0x00541960, 0x16c)
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
            if (item->isObjectEntrance() || !item->isPassableLand())
                return false;
        }
    }
    s32 waterOffset;
    for (waterOffset = 0; waterOffset < RMG_SHIPYARD_WATER_OFFSET_COUNT; ++waterOffset) {
        TRmgMapPosition water = getRmgShipyardWaterPosition(position, waterOffset);
        if (water.m_x < 0 || water.m_x >= m_map.m_mapWidth)
            continue;
        TRmgMapItem* item = m_map.getMapItem(water);
        if (item->getLandType() == eTerrainWater && item->hasPathClearance())
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
    return m_map.getMapItem(farSide)->getLandType() != eTerrainWater;
}

static TRmgMapPosition getRmgShipyardWaterPosition(TRmgMapPosition shipyardPosition,
    s32 waterOffset)
{
    return shipyardPosition + g_rmgShipyardWaterOffsets[waterOffset];
}

MAC_ADDRESS(0x24457c, 0x130)
void type_random_map_generator::floodShipyardWater(type_object* shipyard)
{
    TRmgMapPosition shipyardPosition = shipyard->getPosition();
    TRmgMapPosition waterPosition;
    s32 waterOffset;
    for (waterOffset = 0; waterOffset < RMG_SHIPYARD_WATER_OFFSET_COUNT; ++waterOffset) {
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
// A successful border-guard placement clears the guard value.
VA(0x00541ad0, 0x5b0)
MAC_ADDRESS(0x2446ac, 0x55c)
b8 type_random_map_generator::createShipyardConnection(
    TRmgZone* source, TRmgZoneConnection* connection)
{
    s32 sourceZone = source->m_templateZone->m_zoneIndex;
    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];
    s32 destinationZone = destination->m_templateZone->m_zoneIndex;
    if (source->getLevelPosition().m_z != destination->getLevelPosition().m_z)
        return false;

    std::vector<TRmgMapPosition> candidates;
    s32 prototypeIndex = rand() % m_objectPrototypes[SHIPYARD].size();
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[SHIPYARD][prototypeIndex];
    TObjectType* prototype = properties->m_prototype;
    TRmgZoneBounds bounds = source->m_bounds;
    TRmgMapPosition cell;
    cell.m_z = source->getLevelPosition().m_z;
    for (cell.m_y = bounds.m_minimumY; cell.m_y < bounds.m_maximumY; ++cell.m_y) {
        for (cell.m_x = bounds.m_minimumX; cell.m_x < bounds.m_maximumX; ++cell.m_x) {
            TRmgMapItem* item = m_map.getMapItem(cell);
            if (item->m_zoneState.m_zone == sourceZone
                && item->m_zoneState.m_connectionZone == destinationZone) {
                if (item->isConnectionVisited())
                    return true;
                if (item->getLandType() != eTerrainWater) {
                    TRmgMapPosition site = cell;
                    if (site.m_y + 1 < m_map.m_mapHeight) {
                        for (site.m_x = cell.m_x; site.m_x <= cell.m_x + 2; ++site.m_x) {
                            if (m_map.canPlaceObject(properties, site, source)
                                && canPlaceShipyard(site))
                                candidates.push_back(site);
                        }
                    }
                }
            }
        }
    }
    if (candidates.size() == 0)
        return false;

    rmgOwnableObject* shipyard = new rmgOwnableObject(properties);
    TRmgMapPosition position = addObjectAtRandomCandidate(shipyard, candidates);

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

    s32 guardValue = getConnectionGuardValue(connection);

    // Border guards and the monster guard stand on that row, centred on the
    // entrance column.
    if (connection->m_borderGuard) {
        approach.m_x = entrance.m_x - 1;
        if (placeBorderObject(approach, RMG_SHIPYARD_BORDER_GUARDS, destination) >= 0)
            guardValue = 0;
    }
    if (guardValue > 0) {
        approach.m_x = entrance.m_x;
        placeGuard(guardValue, approach);
    }
    return true;
}

// Opens the approach cell directly below an object entrance and returns it.
inline TRmgMapPosition type_random_map::openEntranceApproach(TRmgMapPosition entrance)
{
    TRmgMapPosition approach = entrance;
    ++approach.m_y;
    getMapItem(approach)->openPath();
    return approach;
}

VA(0x00542080, 0x8aa)
MAC_ADDRESS(0x244c08, 0xa1c)
b8 type_random_map_generator::createSubterraneanGate(
    TRmgZone* source, TRmgZoneConnection* connection)
{
    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];
    s32 sourceZone = source->m_templateZone->m_zoneIndex;
    s32 destinationZone = destination->m_templateZone->m_zoneIndex;
    if (source->getLevelPosition().m_z == destination->getLevelPosition().m_z)
        return false;
    if (source->m_terrain == eTerrainWater)
        return false;

    // Gate sites lie where the two zones' bounds overlap.
    TRmgZoneBounds overlap = source->m_bounds;
    TRmgZoneBounds destinationBounds = destination->m_bounds;
    overlap.m_minimumX = max(overlap.m_minimumX, destinationBounds.m_minimumX);
    overlap.m_minimumY = max(overlap.m_minimumY, destinationBounds.m_minimumY);
    overlap.m_maximumX = min(overlap.m_maximumX, destinationBounds.m_maximumX);
    overlap.m_maximumY = min(overlap.m_maximumY, destinationBounds.m_maximumY);
    if (overlap.m_minimumX >= overlap.m_maximumX || overlap.m_minimumY >= overlap.m_maximumY)
        return false;

    s32 gateIndex = rand() % m_objectPrototypes[UNDERGROUND_GATE].size();
    TRmgObjectPropertiesRef* gateProperties = m_objectPrototypes[UNDERGROUND_GATE][gateIndex];
    TObjectType* gatePrototype = gateProperties->m_prototype;

    std::vector<TRmgMapPosition> candidates;
    s32 bestScore = 0;
    TRmgMapPosition position = source->getLevelPosition();

    for (position.m_y = overlap.m_minimumY; position.m_y < overlap.m_maximumY; ++position.m_y) {
        for (position.m_x = overlap.m_minimumX; position.m_x < overlap.m_maximumX; ++position.m_x) {
            TRmgMapItem* sourceItem = m_map.getMapItem(position);
            if (sourceItem->m_zoneState.m_zone != sourceZone)
                continue;
            s32 score = sourceItem->m_zoneState.m_objectDistance;

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

    TRmgMapPosition gatePosition = addObjectAtRandomCandidate(
        new type_object(gateProperties), candidates);

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

    s32 guardValue = getConnectionGuardValue(connection);

    TRmgMapPosition approach = m_map.openEntranceApproach(entrance);
    TRmgMapPosition otherApproach = m_map.openEntranceApproach(otherEntrance);

    if (connection->m_borderGuard) {
        // Success on either side suppresses both guards; a failed placement
        // does not undo the other side's objects.
        placeGateConnectionBorderGuard(approach, destination, guardValue);
        placeGateConnectionBorderGuard(otherApproach, source, guardValue);
    }

    if (guardValue > 0) {
        placeGuard(guardValue, approach);
        placeGuard(guardValue, otherApproach);
    }

    return true;
}

// Object positions name the lower-right footprint cell. Inset the minimum
// anchor coordinates so the whole footprint fits.
inline void TRmgZoneBounds::insetForObjectFootprint(const TObjectType* prototype)
{
    m_minimumY += prototype->getHeight() - 1;
    m_minimumX += prototype->getWidth() - 1;
}

// Places an object at a random fitting cell of the zone.
VA(0x00542930, 0x1c6)
MAC_ADDRESS(0x245624, 0x24c)
b8 type_random_map_generator::placeObjectInZone(type_object* object, TRmgZone* zone)
{
    TRmgObjectPropertiesRef* properties = object->m_properties;
    TObjectType* prototype = properties->m_prototype;
    std::vector<TRmgMapPosition> candidates;
    TRmgZoneBounds bounds = zone->getBounds();
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    bounds.insetForObjectFootprint(prototype);
    TRmgMapPosition position = zone->getLevelPosition();
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            if (m_map.getMapItem(position)->m_zoneState.m_zone == zoneIndex
                && m_map.canPlaceObject(properties, position, zone))
                candidates.push_back(position);
        }
    }
    if (!candidates.size())
        return false;
    addObjectAtRandomCandidate(object, candidates);
    return true;
}

// Rebuilds the zone connection paths, then puts a border guard on the portal
// cell's path predecessor; a zero-cost cell uses the first same-zone open
// neighbour, else the cell below, and an unreached cell fails. Success also
// marks the cell's five side and lower neighbours as border connections.
VA(0x00542b00, 0x1d2)
MAC_ADDRESS(0x245870, 0x364)
b8 type_random_map_generator::placeMonolithBorder(
    TRmgMapPosition position, TRmgZone* keyTentZone)
{
    // North is up; digits are offset indices (probe order) around the
    // portal position P.
    //   2 P 1
    //   4 0 3
    TPoint offsets[5] = {
        TPoint(0, 1), TPoint(1, 0), TPoint(-1, 0), TPoint(1, 1), TPoint(-1, 1)
    };
    TRmgMapPosition guardPosition;
    const s32 probeCount = sizeof(offsets) / sizeof(offsets[0]);
    buildZoneConnectionPaths();
    TRmgMapItem* item = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
    s32 zoneIndex = item->m_zoneState.m_zone;
    u32 movementCost = item->m_movement.m_cost;
    if (movementCost >= RMG_REACHED_COST_LIMIT)
        return false;
    if (movementCost > 0) {
        guardPosition = item->m_previousTile;
    } else {
        s32 probe;
        for (probe = 0; probe < probeCount; ++probe) {
            guardPosition = position + offsets[probe];
            TRmgMapItem* nearby = m_map.getMapItem(guardPosition);
            if (nearby->m_zoneState.m_zone == zoneIndex
                && nearby->hasPathClearance())
                break;
        }
        if (probe == probeCount) {
            guardPosition = position + TPoint(0, 1);
        }
    }
    s32 color = placeBorderObject(guardPosition, RMG_SINGLE_BORDER_GUARD, keyTentZone);
    if (color >= 0) {
        for (s32 probe = 0; probe < probeCount; ++probe)
            m_map.getMapItem(position + offsets[probe])->markBorderConnection(color);
    }
    return color >= 0;
}

// Places one portal; a failed placement deletes it.
inline type_object* type_random_map_generator::placeMonolith(
    TRmgObjectPropertiesRef* properties, TRmgZone* zone, bool oneWay)
{
    type_object* object = new type_object(properties);
    if (!placeObjectInZone(object, zone)) {
        delete object;
        return 0;
    }
    std::vector<type_object*>& monoliths =
        oneWay ? m_monolithsOneWay : m_monolithsTwoWay;
    monoliths.push_back(object);
    zone->m_entrances.push_back(TPoint(object->m_position.m_x, object->m_position.m_y));
    return object;
}

// Protects a portal with a border guard keyed to a tent in the other zone,
// or else with a monster guard on the cell below it.
inline void type_random_map_generator::protectMonolith(type_object* portal,
    const TRmgZoneConnection* connection, TRmgZone* keyTentZone, s32& guardValue)
{
    if (connection->m_borderGuard
        && placeMonolithBorder(portal->getPosition(), keyTentZone)) {
        guardValue = 0;
    } else if (guardValue > 0) {
        placeGuard(guardValue, portal->getPosition() + TPoint(0, 1));
    }
}

// One-way prototypes produce an entrance/exit pair in each zone. Failed
// placement deletes only that object; subsequent endpoint attempts continue.
VA(0x00542ce0, 0x554)
MAC_ADDRESS(0x245bd4, 0x6e8)
void type_random_map_generator::createMonolithConnection(
    TRmgZone* source, TRmgZoneConnection* connection, s32 prototypeIndex)
{
    TRmgObjectPropertiesRef* exitProperties = 0;
    TRmgZone* destination = m_zones[connection->m_destination->m_zoneIndex];
    if (source->m_terrain == eTerrainWater || destination->m_terrain == eTerrainWater)
        return;
    TRmgObjectPropertiesRef* properties;
    if (prototypeIndex < m_objectPrototypes[LITH_TWOWAY].size()) {
        properties = m_objectPrototypes[LITH_TWOWAY][prototypeIndex];
    } else {
        s32 oneWayIndex = prototypeIndex - m_objectPrototypes[LITH_TWOWAY].size();
        properties = m_objectPrototypes[LITH_ONEWAY_ENTRANCE][oneWayIndex];
        exitProperties = m_objectPrototypes[LITH_ONEWAY_EXIT][oneWayIndex];
    }
    s32 guardValue = getConnectionGuardValue(connection);

    // A source-side border guard also drops the destination's guard, even when
    // the destination's own border-guard placement later fails.
    type_object* object = placeMonolith(properties, source, exitProperties != 0);
    if (object)
        protectMonolith(object, connection, destination, guardValue);
    object = placeMonolith(properties, destination, exitProperties != 0);
    if (object)
        protectMonolith(object, connection, source, guardValue);
    if (exitProperties) {
        placeMonolith(exitProperties, source, true);
        placeMonolith(exitProperties, destination, true);
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

inline void type_random_map::clearConnectionVisits(s32 level)
{
    TRmgMapItem* item = getMapItem(TRmgMapPosition(0, 0, level));
    for (s32 remaining = getWidth() * getHeight(); remaining--; ++item)
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
    for (position.m_z = RMG_SURFACE_LEVEL; position.m_z < m_map.getNumberLevels(); ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.getHeight(); ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.getWidth();
                 ++position.m_x, ++mapItem) {
                if (mapItem->m_zoneState.m_connectionZone < 0)
                    continue;

                if (mapItem->getLandType() == eTerrainWater
                    || !mapItem->isPassableLand())
                    continue;

                s32 direction = mapItem->m_tileData.m_connectionDirection;
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

    s32 prototypeIndex = 0;

    // Unused.
    std::vector<TRmgMapPosition> connectionPositionsScratch;

    s32 zoneIndex;
    for (zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgTemplateZone* templateZone = zone->m_templateZone;
        if (zone->getTerrain() == eTerrainWater)
            continue;

        m_map.clearConnectionVisits(zone->getLevelPosition().m_z);

        for (s32 connectionIndex = 0;
             connectionIndex < templateZone->m_connections.size();
             ++connectionIndex) {
            TRmgZoneConnection* connection =
                &templateZone->m_connections[connectionIndex];
            if (connection->isConnected())
                continue;

            TRmgZone* destination =
                m_zones[connection->m_destination->m_zoneIndex];
            TRmgZoneConnection* oppositeConnection =
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
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgTemplateZone* templateZone = zone->m_templateZone;
        if (zone->getTerrain() == eTerrainWater)
            continue;

        s32 connectionIndex = 0;
        while (connectionIndex < templateZone->m_connections.size()
               && templateZone->m_connections[connectionIndex].isConnected())
            ++connectionIndex;
        if (connectionIndex == templateZone->m_connections.size())
            continue;

        m_map.clearConnectionVisits(zone->getLevelPosition().m_z);

        for (s32 objectIndex = 0; objectIndex < m_objects.size(); ++objectIndex) {
            type_object* object = m_objects[objectIndex];
            if (object->m_properties->m_prototype->getObjectType() == SHIPYARD) {
                TRmgMapPosition shipyardPosition = object->getPosition();
                if (m_map.getMapItem(shipyardPosition)->m_zoneState.m_zone == zoneIndex) {
                    floodShipyardWater(object);
                }
            }
        }

        for (;
             connectionIndex < templateZone->m_connections.size();
             ++connectionIndex) {
            TRmgZoneConnection* connection =
                &templateZone->m_connections[connectionIndex];
            if (connection->isConnected())
                continue;

            TRmgZone* destination =
                m_zones[connection->m_destination->m_zoneIndex];
            TRmgZoneConnection* oppositeConnection =
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
inline bool TRmgMapItem::canBlockFloor() const
{
    return !hasPathClearance() && isPassableLand()
        && !isObjectEntrance();
}

VA(0x005439e0, 0x283)
MAC_ADDRESS(0x246a34, 0x31c)
void type_random_map_generator::decorateUnderground()
{
    TRmgMapItem* item = m_map.getMapItem(0, 0, RMG_UNDERGROUND_LEVEL);
    type_random_map map(item, m_map.m_mapWidth, m_map.m_mapHeight);
    TRmgTerrainBrush brush(&map, eTerrainRock, RMG_BRUSH_STRENGTH);
    for (s32 y = 0; y < m_map.m_mapHeight; ++y) {
        for (s32 x = 0; x < m_map.m_mapWidth; ++x, ++item) {
            if (item->canBlockFloor())
                brush.paintRectangle(x, y, 1, 1);
        }
    }
    if (m_progress)
        m_progress->advance(1200);
    TTerrainType currentTerrain = eTerrainRock;
    for (u32 zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->getLevelPosition().m_z != RMG_UNDERGROUND_LEVEL)
            continue;
        TRmgZoneBounds bounds = m_zones[zone]->m_bounds;
        TTerrainType terrain = m_zones[zone]->m_terrain;
        if (currentTerrain == eTerrainRock) {
            brush.changeTerrain(terrain, RMG_BRUSH_STRENGTH);
            currentTerrain = terrain;
        }
        for (s32 y = bounds.m_minimumY; y < bounds.m_maximumY; ++y) {
            for (s32 x = bounds.m_minimumX; x < bounds.m_maximumX; ++x) {
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
VA(0x00543c70, 0x1a2)
MAC_ADDRESS(0x246d50, 0x30c)
TPoint type_random_map::traceBranchEnd(TPoint from, TPoint toward, s32 level)
{
    s32 dx = toward.m_x - from.m_x;
    s32 dy = toward.m_y - from.m_y;
    s32 major;
    s32 minor;
    TRmgVector axial;
    if (abs(dx) > abs(dy)) {
        major = abs(dx);
        minor = abs(dy);
        axial = TRmgVector(dx > 0 ? 1 : -1, 0);
    } else {
        major = abs(dy);
        minor = abs(dx);
        axial = TRmgVector(0, dy > 0 ? 1 : -1);
    }
    TRmgVector diagonal(dx > 0 ? 1 : -1, dy > 0 ? 1 : -1);
    s32 error = major / 2;
    s32 steps = 0;
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
VA(0x00543e20, 0x574)
MAC_ADDRESS(0x24705c, 0x750)
void type_random_map_generator::carveBranchingPaths()
{
    TRmgMapItem* item = m_map.m_mapItems;
    s32 remaining = m_map.getHeight() * m_map.getWidth() * m_map.m_numberLevels;
    for (; remaining--; ++item) {
        if (!item->hasObjects()) {
            item->markObstacleFill();
        } else {
            item->openPath();
        }
    }
    for (s32 level = RMG_SURFACE_LEVEL; level < m_map.m_numberLevels; ++level) {
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
                    s32 length = perpendicular.length();
                    if (length > 1) {
                        s32 displacement = getRmgCenteredRandomOffset(length);
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
    for (position.m_z = RMG_SURFACE_LEVEL; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (item->getLandType() == eTerrainWater || item->getLandType() == eTerrainRock) {
                    item->openPath();
                }
                if (item->hasObstacleFill())
                    m_map.markBorderPatch(position);
            }
        }
    }
}

VA(0x005443a0, 0x2f5)
MAC_ADDRESS(0x247800, 0x59c)
void type_random_map_generator::connectJunctionEntrance(TPoint from, TPoint to,
    TRmgZone* zone)
{
    std::vector<TPoint> pending;
    TRmgMapPosition position = zone->getLevelPosition();
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    s32 roughness = zone->m_scaledSize;
    pending.push_back(to);
    while (pending.size() > 0) {
        to = pending.back();
        pending.pop_back();
        if (!splitRmgBoundarySegment(pending, from, to, roughness,
                RMG_FULL_LENGTH_DISPLACEMENT)) {
            TPoint clamped = clampRmgBoundaryToMap(from, m_map);
            TRmgMapItem* item = m_map.getMapItem(clamped.m_x, clamped.m_y, position.m_z);
            if (item->m_zoneState.m_zone == zoneIndex) {
                item->openPath();
                m_map.clearNearbyObstacleFill(clamped, position.m_z, zoneIndex);
            }
            from = to;
        }
    }
}

VA(0x005446a0, 0x27e)
MAC_ADDRESS(0x247d9c, 0x3e0)
void type_random_map_generator::prepareJunctionZone(TRmgZone* zone)
{
    TRmgZoneBounds bounds = zone->m_bounds;
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition position = zone->getLevelPosition();
    s32 level = position.m_z;
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position.m_x, position.m_y, level);
            if (item->m_zoneState.m_zone == zoneIndex
                && item->getLandType() != eTerrainWater) {
                item->resetMovement();
                if (!item->hasObjects()) {
                    item->markObstacleFill();
                }
            }
        }
    }
    if (!zone->m_entrances.size())
        return;
    TRmgMapPosition first(zone->m_entrances[0].m_x, zone->m_entrances[0].m_y, level);
    TRmgMapItem* item = m_map.getMapItem(first.m_x, first.m_y, first.m_z);
    item->setMovementCost(0,
        TRmgMapPosition(RMG_NO_POSITION, RMG_NO_POSITION, RMG_NO_POSITION));
    // generate prepares only land junction zones.
    m_map.floodConnectionCosts(first, false);
    for (s32 entrance = 1; entrance < static_cast<s32>(zone->m_entrances.size()); ++entrance) {
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
    for (position.m_z = RMG_SURFACE_LEVEL; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (!item->hasObstacleFill() && item->isPassableLand() && !item->isObjectEntrance()
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

VA(0x00544a50, 0x90)
MAC_ADDRESS(0x248328, 0xf0)
void type_random_map_generator::placePrimaryTown(TRmgZone* zone)
{
    TRmgTemplateZone* templateZone = zone->m_templateZone;
    TTownType alignment = zone->m_alignment;
    s32 player = m_playerIndexMap[templateZone->m_playerIndex + 1];
    if (templateZone->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_COUNT] > 0
        && tryPlacePrimaryTown(zone, alignment, player, true))
        return;
    if (templateZone->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT] > 0
        && tryPlacePrimaryTown(zone, alignment, player, false))
        return;
    if (templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_COUNT] > 0
        && tryPlacePrimaryTown(zone, alignment, -1, true))
        return;
    if (templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT] > 0)
        tryPlacePrimaryTown(zone, alignment, -1, false);
}

// Disabled categories leave both accumulators untouched.
static inline void initializeRmgDensityCategories(const s32* densities,
    s32 categoryCount, b8* finished, s32& totalDensity, s32& densityProduct)
{
    for (s32 category = 0; category < categoryCount; ++category) {
        s32 density = densities[category];
        if (density <= 0) {
            finished[category] = true;
        } else {
            totalDensity += density;
            finished[category] = false;
            densityProduct *= density;
        }
    }
}

// Minimum entrance distance (2 per cardinal step, see m_objectDistance) for
// a total density: sqrt(densityArea / density), with integer division and a
// truncated root. Squared distances are 4x tile areas, so each object gets
// about densityArea / 4 / density tiles.
static inline s32 getRmgDensitySpacing(s32 densityArea, s32 density)
{
    return static_cast<s32>(sqrt(static_cast<double>(densityArea / density)));
}

// Squared entrance-distance areas per unit of density.
enum ERmgDensityArea {
    // 144x144 tiles, the extra-large map size: towns and extra mines.
    RMG_TOWN_AND_MINE_DENSITY_AREA = 4 * 144 * 144,
    // 200 and 400 tiles: treasure groups on land and in water zones.
    RMG_LAND_TREASURE_DENSITY_AREA = 4 * 200,
    RMG_WATER_TREASURE_DENSITY_AREA = 4 * 400
};

static inline void initializeRmgCategoryStrides(const s32* densities,
    const s32* counts, s32 categoryCount, s32 densityProduct,
    s32* countSteps, s32* weightedCounts)
{
    for (s32 category = 0; category < categoryCount; ++category) {
        if (densities[category] > 0) {
            countSteps[category] = densityProduct / densities[category];
            weightedCounts[category] = counts[category] * countSteps[category];
        }
    }
}

enum ERmgWeightedCategory {
    RMG_NO_CATEGORY = -1
};

// Picks the unfinished category with the lowest weighted count; ties go to
// the first. RMG_NO_CATEGORY once every category is finished.
static inline s32 selectRmgWeightedCategory(const b8* finished,
    const s32* weightedCounts, s32 categoryCount)
{
    s32 selected = RMG_NO_CATEGORY;
    s32 lowest = 0;
    for (s32 category = 0; category < categoryCount; ++category) {
        if (!finished[category]
            && (selected == RMG_NO_CATEGORY || weightedCounts[category] < lowest)) {
            lowest = weightedCounts[category];
            selected = category;
        }
    }
    return selected;
}

// Spacing arguments: the minimum m_objectDistance at a placement. Fixed
// towns and mines ignore spacing; quest groups only avoid entrance cells,
// whose distance is zero.
enum ERmgObjectSpacing {
    RMG_NO_SPACING = 0,
    RMG_QUEST_GROUP_SPACING = 1
};

// The first category with a positive count places one town fewer for the
// primary town, even if placePrimaryTown used a later category or failed.
inline void type_random_map_generator::placeFixedTownCategory(TRmgZone* zone, s32 count,
    TTownType alignment, s32 player, b8 hasFort, b8& skipPrimary)
{
    if (count <= 0)
        return;
    for (s32 townIndex = skipPrimary ? 1 : 0; townIndex < count; ++townIndex)
        tryPlaceAdditionalTown(zone, alignment, player, hasFort, RMG_NO_SPACING);
    skipPrimary = false;
}

VA(0x00544ae0, 0x2b0)
MAC_ADDRESS(0x248418, 0x414)
void type_random_map_generator::placeAdditionalTowns(TRmgZone* zone)
{
    TRmgTemplateZone* templateZone = zone->m_templateZone;
    TTownType alignment = zone->m_alignment;
    s32 player = m_playerIndexMap[templateZone->m_playerIndex + 1];
    b8 skipPrimary = true;
    placeFixedTownCategory(zone,
        templateZone->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_COUNT], alignment, player, true, skipPrimary);
    placeFixedTownCategory(zone,
        templateZone->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT], alignment, player, false, skipPrimary);
    placeFixedTownCategory(zone,
        templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_COUNT], alignment, -1, true, skipPrimary);
    placeFixedTownCategory(zone,
        templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT], alignment, -1, false, skipPrimary);
    s32 totalDensity = 0;
    s32 densityProduct = 1;
    // Indexed by ERmgTownPlacementCategory.
    const s32 categoryCount = RMG_TOWN_NEUTRAL_BASIC + 1;
    s32 densities[categoryCount] = {
        templateZone->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_DENSITY],
        templateZone->m_townPlacement[RMG_TOWN_PLAYER_BASIC_DENSITY],
        templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_DENSITY],
        templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_DENSITY]
    };
    s32 weightedCounts[categoryCount] = {
        templateZone->m_townPlacement[RMG_TOWN_PLAYER_CASTLE_COUNT],
        templateZone->m_townPlacement[RMG_TOWN_PLAYER_BASIC_COUNT],
        templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_CASTLE_COUNT],
        templateZone->m_townPlacement[RMG_TOWN_NEUTRAL_BASIC_COUNT]
    };
    s32 countSteps[categoryCount];
    b8 finished[categoryCount];
    initializeRmgDensityCategories(densities, categoryCount, finished, totalDensity, densityProduct);
    if (!totalDensity)
        return;
    s32 spacing = getRmgDensitySpacing(RMG_TOWN_AND_MINE_DENSITY_AREA, totalDensity);
    initializeRmgCategoryStrides(densities, weightedCounts, categoryCount,
        densityProduct, countSteps, weightedCounts);
    for (;;) {
        s32 selected = selectRmgWeightedCategory(finished, weightedCounts, categoryCount);
        if (selected == RMG_NO_CATEGORY)
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
inline TRmgMapPosition type_random_map_generator::placeTownAtRandomCandidate(
    TRmgObjectPropertiesRef* properties, s32 player, b8 hasFort,
    const std::vector<TRmgMapPosition>& candidates, const TObjectType::TPoint& trigger)
{
    rmgTownObject* town = new rmgTownObject(properties,
        m_nextObjectId++, player, hasFort);
    TRmgMapPosition position = addObjectAtRandomCandidate(town, candidates);
    TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, trigger);
    m_roadTargets.push_back(entrance);
    m_map.openEntranceApproach(entrance);
    return entrance;
}

// A zone without a primary town gets this one as its primary town.
// Otherwise the entrance must be at least spacing from other objects, with
// its clipped 3x3 neighbourhood in the zone; the farthest such sites win.
VA(0x00544d90, 0x4b7)
MAC_ADDRESS(0x24882c, 0x5d4)
b8 type_random_map_generator::tryPlaceAdditionalTown(TRmgZone* zone,
    s32 alignment, s32 player, b8 hasFort, s32 spacing)
{
    TRmgTemplateZone* templateZone = zone->m_templateZone;
    if ((player == -1 && !templateZone->m_neutralTownsMatchZone) || alignment == eTownNeutral) {
        alignment = templateZone->selectAllowedTown();
        // RoE maps have no Conflux.
        if (alignment == eTownNeutral)
            alignment = rand() % (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE
                ? TOWN_TYPE_COUNT : TOWN_CONFLUX);
    }
    if (!zone->m_hasPrimaryTown)
        return tryPlacePrimaryTown(zone, alignment, player, hasFort);

    std::vector<TRmgMapPosition> candidates;
    s32 zoneIndex = templateZone->m_zoneIndex;
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[TOWN][alignment];
    TObjectType* prototype = properties->m_prototype;
    TObjectType::TPoint trigger = prototype->m_triggerCell;
    TRmgMapPosition position = zone->getLevelPosition();
    TRmgZoneBounds bounds = zone->m_bounds;
    bounds.m_minimumY += prototype->getHeight();
    bounds.m_minimumX += prototype->getWidth();
    s32 bestScore = spacing;
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapPosition entrance = getRmgObjectTriggerPosition(position, trigger);
            TRmgMapItem* item = m_map.getMapItem(entrance.m_x, entrance.m_y, entrance.m_z);
            if (item->m_zoneState.m_zone != zoneIndex)
                continue;
            s32 score = item->m_zoneState.m_objectDistance;
            if (score < bestScore || !m_map.canPlaceObject(properties, position, zone))
                continue;
            TRmgZoneBounds nearby;
            m_map.getNeighborhoodBounds(nearby, entrance, RMG_NEIGHBORHOOD_3X3);
            b8 valid = true;
            for (s32 y = nearby.m_minimumY; y < nearby.m_maximumY; ++y) {
                for (s32 x = nearby.m_minimumX; x < nearby.m_maximumX; ++x) {
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
    placeTownAtRandomCandidate(properties,
        player, hasFort, candidates, trigger);
    return true;
}

VA(0x00545250, 0x324)
MAC_ADDRESS(0x248e00, 0x3b4)
b8 type_random_map_generator::tryPlacePrimaryTown(
    TRmgZone* zone, s32 alignment, s32 player, b8 hasFort)
{
    if (alignment == eTownNeutral)
        return false;
    std::vector<TRmgMapPosition> candidates;
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    TRmgMapPosition center = zone->m_levelPosition;
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[TOWN][alignment];
    TObjectType* prototype = properties->m_prototype;
    s32 bestSquaredDistance = 32000;
    TRmgMapPosition site;
    site.m_z = center.m_z;
    TRmgZoneBounds bounds = zone->m_bounds;
    for (site.m_y = bounds.m_minimumY; site.m_y < bounds.m_maximumY; ++site.m_y) {
        for (site.m_x = bounds.m_minimumX; site.m_x < bounds.m_maximumX; ++site.m_x) {
            if (m_map.getMapItem(site)->m_zoneState.m_zone != zoneIndex)
                continue;
            s32 squaredDistance = getRmgSquaredDistance(site, center);
            if (squaredDistance <= bestSquaredDistance
                && m_map.canPlaceObject(properties, site, zone))
                addRmgLowestScoreCandidate(candidates, site, squaredDistance,
                    bestSquaredDistance);
        }
    }
    if (!candidates.size())
        return false;
    zone->m_primaryTownEntrance = placeTownAtRandomCandidate(properties,
        player, hasFort, candidates, prototype->m_triggerCell);
    zone->m_hasPrimaryTown = true;
    return true;
}

// Squared tile distances of a starting mine from the town: never within 4
// tiles or beyond 200, and any site within 12 tiles ranks as 12.
enum ERmgStartingMineDistance {
    RMG_STARTING_MINE_MINIMUM_SQUARED_DISTANCE = 4 * 4,
    RMG_STARTING_MINE_NEAR_SQUARED_DISTANCE = 12 * 12,
    RMG_STARTING_MINE_MAXIMUM_SQUARED_DISTANCE = 200 * 200
};

VA(0x00545580, 0x401)
MAC_ADDRESS(0x249214, 0x46c)
b8 type_random_map_generator::placeMineSite(type_object* object,
    TRmgZone* zone, b8 startingMine, s32 spacing)
{
    TRmgObjectPropertiesRef* properties = object->m_properties;
    TObjectType* prototype = properties->m_prototype;
    std::vector<TRmgMapPosition> candidates;
    TRmgZoneBounds bounds = zone->m_bounds;
    s32 bestObstacleCount = 0;
    s32 bestSquaredDistance = RMG_STARTING_MINE_MAXIMUM_SQUARED_DISTANCE;
    s32 bestScore = spacing;
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    // The mine position whose entrance would be the town entrance, so anchor
    // distances below equal entrance-to-entrance distances.
    TRmgMapPosition townAlignedAnchor;
    if (startingMine) {
        townAlignedAnchor = zone->m_primaryTownEntrance;
        townAlignedAnchor += TPoint(prototype->m_triggerCell.m_x, prototype->m_triggerCell.m_y);
    }
    bounds.insetForObjectFootprint(prototype);
    TRmgMapPosition position = zone->m_levelPosition;
    properties->buildOutline();
    for (position.m_y = bounds.m_minimumY; position.m_y < bounds.m_maximumY; ++position.m_y) {
        for (position.m_x = bounds.m_minimumX; position.m_x < bounds.m_maximumX; ++position.m_x) {
            TRmgMapItem* item = m_map.getMapItem(position);
            if (item->m_zoneState.m_zone != zoneIndex || !m_map.canPlaceObject(properties, position, zone))
                continue;
            if (startingMine) {
                s32 squaredDistance = getRmgSquaredDistance(position, townAlignedAnchor);
                if (squaredDistance > bestSquaredDistance
                    || squaredDistance < RMG_STARTING_MINE_MINIMUM_SQUARED_DISTANCE)
                    continue;
                if (squaredDistance < RMG_STARTING_MINE_NEAR_SQUARED_DISTANCE)
                    squaredDistance = RMG_STARTING_MINE_NEAR_SQUARED_DISTANCE;
                if (squaredDistance < bestSquaredDistance) {
                    bestSquaredDistance = squaredDistance;
                    bestObstacleCount = 0;
                    bestScore = 0;
                    candidates.clear();
                }
            }
            s32 score = item->m_zoneState.m_objectDistance;
            if (score < bestScore)
                continue;
            s32 obstacleCount = 0;
            for (u32 i = 0; i < properties->m_outline.size(); ++i) {
                TPoint offset = properties->m_outline[i];
                s32 x = position.m_x + offset.m_x;
                s32 y = position.m_y + offset.m_y;
                if (x < 0 || x >= m_map.m_mapWidth || y < 0 || y >= m_map.m_mapHeight || y > position.m_y)
                    continue;
                TRmgMapItem* nearby = m_map.getMapItem(x, y, position.m_z);
                if (nearby->isPassableLand() && nearby->hasObstacleFill())
                    ++obstacleCount;
            }
            if (obstacleCount > 5)
                obstacleCount = 5;
            if (obstacleCount < bestObstacleCount)
                continue;
            if (obstacleCount > bestObstacleCount) {
                candidates.clear();
                bestObstacleCount = obstacleCount;
            }
            addRmgHighestScoreCandidate(candidates, position, score, bestScore);
        }
    }
    if (!candidates.size())
        return false;
    addObjectAtRandomCandidate(object, candidates);
    return true;
}

// Guard value for a zone object; none in a zone without monsters. Map
// strength shares the zone scale (2 weak, 3 normal, 4 strong), so an average
// zone keeps the map strength that connection guards use.
inline s32 type_random_map_generator::getZoneGuardValue(s32 value,
    const TRmgZone* zone) const
{
    ERmgZoneMonsterStrength zoneStrength =
        zone->m_templateZone->m_monsterStrength;
    if (zoneStrength == RMG_ZONE_MONSTERS_NONE)
        return 0;
    s32 strength = zoneStrength + m_monsterStrength - RMG_ZONE_MONSTERS_AVERAGE;
    if (strength > RMG_STRONGEST_GUARD_STRENGTH) strength = RMG_STRONGEST_GUARD_STRENGTH;
    else if (strength < 0) strength = 0;
    return getRmgGuardValue(value, strength);
}

// Mine guard value by resource; zero when the zone has no monsters.
s32 type_random_map_generator::getMineGuardValue(s32 resource, const TRmgZone* zone) const
{
    s32 value;
    switch (resource) {
    case WOOD: case ORE: value = 1500; break;
    case GOLD: value = 7000; break;
    default: value = 3500; break;
    }
    return getZoneGuardValue(value, zone);
}

// Retail bug: the entrance and resource area use the trigger and width of
// the last scanned mine prototype, not the selected one.
VA(0x00545990, 0x466)
MAC_ADDRESS(0x249680, 0x5ac)
b8 type_random_map_generator::tryPlaceMine(TRmgZone* zone,
    s32 resource, b8 startingMine, s32 spacing)
{
    std::vector<TRmgObjectPropertiesRef*> candidates;
    TTerrainType terrain = zone->m_terrain;
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
    s32 guardValue = getMineGuardValue(resource, zone);
    TRmgMapPosition approach = m_map.openEntranceApproach(getRmgObjectTriggerPosition(
        mine->getPosition(), lastScannedPrototype->m_triggerCell));
    if (guardValue > 0)
        placeGuard(guardValue, approach);
    s32 placed = 0;
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

// Weaker guards are not placed.
enum ERmgGuardValueLimits {
    RMG_MINIMUM_GUARD_VALUE = 2000
};

VA(0x00545e00, 0x5b)
MAC_ADDRESS(0x22ce7c, 0x74)
s32 getRmgGuardValue(s32 value, s32 strength)
{
    s32 guardValue = 0;
    if (value > g_rmgGuardThresholdLow[strength]) {
        guardValue = (value - g_rmgGuardThresholdLow[strength])
            * g_rmgGuardScaleLow[strength] / 4;
    }
    if (value > g_rmgGuardThresholdHigh[strength]) {
        guardValue += (value - g_rmgGuardThresholdHigh[strength])
            * g_rmgGuardScaleHigh[strength] / 4;
    }
    return guardValue < RMG_MINIMUM_GUARD_VALUE ? 0 : guardValue;
}

VA(0x00545e60, 0xfa)
MAC_ADDRESS(0x249c8c, 0x1c4)
void type_random_map_generator::placeExtraMines(TRmgZone* zone)
{
    TRmgTemplateZone* templateZone = zone->m_templateZone;
    b8 finished[NUM_RESOURCES];
    s32 totalDensity = 0;
    s32 densityProduct = 1;
    initializeRmgDensityCategories(templateZone->m_mineDensities, NUM_RESOURCES,
        finished, totalDensity, densityProduct);
    if (!totalDensity)
        return;
    s32 spacing = getRmgDensitySpacing(RMG_TOWN_AND_MINE_DENSITY_AREA, totalDensity);
    s32 countSteps[NUM_RESOURCES];
    s32 weightedCounts[NUM_RESOURCES];
    initializeRmgCategoryStrides(templateZone->m_mineDensities, templateZone->m_mineCounts,
        NUM_RESOURCES, densityProduct, countSteps, weightedCounts);
    for (;;) {
        s32 selected = selectRmgWeightedCategory(finished, weightedCounts, NUM_RESOURCES);
        if (selected == RMG_NO_CATEGORY)
            break;
        weightedCounts[selected] += countSteps[selected];
        if (!tryPlaceMine(zone, selected, false, spacing))
            finished[selected] = true;
    }
}

VA(0x00545f60, 0xd6)
MAC_ADDRESS(0x249e50, 0x110)
void type_random_map_generator::placeMines()
{
    for (u32 index = 0; index < m_zones.size(); ++index) {
        TRmgZone* zone = m_zones[index];
        TRmgTemplateZone* templateZone = zone->m_templateZone;
        for (s32 resource = WOOD; resource <= GOLD; ++resource) {
            b8 startingMine = (resource == WOOD || resource == ORE)
                && (templateZone->m_kind == RMG_TEMPLATE_HUMAN
                    || templateZone->m_kind == RMG_TEMPLATE_COMPUTER)
                && zone->m_hasPrimaryTown;
            for (s32 mine = 0; mine < templateZone->m_mineCounts[resource]; ++mine) {
                if (!tryPlaceMine(zone, resource, startingMine, RMG_NO_SPACING))
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
    s32 terrain, s32 objectType, s32 subtype)
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
    s32 minimum, s32 maximum, s32* value, b8 firstInGroup,
    b8 allowTerrainDependent, b8 compact,
    TRmgMapPosition position)
{
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    s32 totalWeight = 0;
    std::vector<type_treasure_def*> candidates;
    std::vector<TRmgObjectPropertiesRef*> candidateProperties;
    s32 bestValuePerCell = 0;
    for (u32 index = 0; index < m_objectGenerators.size(); ++index) {
        type_treasure_def* definition = m_objectGenerators[index];
        s32 objectType = definition->m_objectType;
        if (!firstInGroup && g_adventureObjectTraits[objectType].m_blocksLanding
            && !g_adventureObjectTraits[objectType].m_clearedOnVisit)
            continue;
        if (!allowTerrainDependent && definition->isTerrainDependent())
            continue;
        if (m_objectCountByType[objectType] >= g_rmgMapObjectLimits[objectType])
            continue;
        if (zone->m_objectCountByType[objectType] >= g_rmgZoneObjectLimits[objectType])
            continue;
        s32 objectValue = definition->getValue(zone, this);
        if (objectValue < 0 || objectValue < minimum || objectValue > maximum)
            continue;
        TRmgObjectPropertiesRef* properties = selectObjectPrototype(
            zone->m_terrain, definition->m_objectType, definition->m_subtype);
        if (!properties)
            continue;
        if (position.m_x >= 0 && m_map.isPlacementBlocked(properties, position, zoneIndex,
                RMG_REJECT_OBSTACLE_ENTRANCES))
            continue;
        if (compact) {
            TObjectType* prototype = properties->m_prototype;
            s32 occupied = 0;
            for (u32 x = 0; x < prototype->getWidth(); ++x) {
                for (u32 y = 0; y < prototype->getHeight(); ++y) {
                    if (isRmgObjectFootprintCell(prototype, x, y))
                        ++occupied;
                }
            }
            s32 valuePerCell = objectValue / occupied;
            if (valuePerCell < 3 * bestValuePerCell / 4)
                continue;
            if (bestValuePerCell < 3 * valuePerCell / 4) {
                totalWeight = 0;
                candidates.clear();
                candidateProperties.clear();
                bestValuePerCell = valuePerCell;
            }
        }
        totalWeight += definition->m_density;
        candidates.push_back(definition);
        candidateProperties.push_back(properties);
    }
    if (!candidates.size())
        return 0;
    s32 remainingWeight = rand() % totalWeight;
    u32 selectedIndex;
    for (selectedIndex = 0; selectedIndex < candidates.size(); ++selectedIndex) {
        remainingWeight -= candidates[selectedIndex]->m_density;
        if (remainingWeight < 0)
            break;
    }
    type_treasure_def* definition = candidates[selectedIndex];
    *value = definition->getValue(zone, this);
    return definition->generate(candidateProperties[selectedIndex], this, zone);
}

// Treasure groups are assembled on a square scratch map before placement.
enum ERmgTreasureGroupMapSize {
    RMG_TREASURE_GROUP_MAP_SIZE = 16
};

// Adds an object centred on the group map (unsigned division).
inline void TRmgTreasureGroup::addCenteredObject(type_object* object)
{
    const TObjectType* prototype = object->m_properties->m_prototype;
    addObject(object,
        TPoint((m_map.getWidth() + static_cast<u32>(prototype->getWidth())) / 2,
            (m_map.getHeight() + static_cast<u32>(prototype->getHeight())) / 2));
}

inline type_object* type_random_map_generator::createTreasureWithRetries(TRmgZone* zone,
    s32 minimum, s32 maximum, s32* value, b8 firstInGroup, b8 compact)
{
    for (s32 attempt = 0; attempt < RMG_TREASURE_ATTEMPTS; ++attempt) {
        TRmgMapPosition unspecified(RMG_NO_POSITION, RMG_NO_POSITION, RMG_NO_POSITION);
        type_object* object = createTreasureObject(zone, minimum, maximum,
            value, firstInGroup, true, compact, unspecified);
        if (object)
            return object;
    }
    return 0;
}

// Generation and fit have independent three-attempt limits; failed fits
// release reservations before deletion.
VA(0x00546520, 0x1b6)
MAC_ADDRESS(0x24a584, 0x22c)
s32 type_random_map_generator::fillTreasureGroup(TRmgZone* zone,
    TRmgTreasureGroup* group, b8 compact, s32 targetValue)
{
    s32 objectValue = 0;
    type_object* firstObject = createTreasureWithRetries(
        zone, targetValue / 4, targetValue, &objectValue, true, compact);
    if (!firstObject)
        return 0;
    group->addCenteredObject(firstObject);
    s32 total = objectValue;
    while (total < targetValue) {
        s32 remainder = targetValue - total;
        if (remainder < RMG_TREASURE_MINIMUM_REMAINDER && remainder < total / 2)
            break;
        b8 added = false;
        for (s32 attempt = 0; attempt < RMG_TREASURE_ATTEMPTS; ++attempt) {
            type_object* nextObject = createTreasureWithRetries(zone,
                remainder / 4, 5 * remainder / 4, &objectValue, false, compact);
            if (!nextObject)
                break;
            if (group->tryAddObject(nextObject)) {
                added = true;
                break;
            }
            nextObject->releaseReservation();
            delete nextObject;
        }
        if (!added)
            break;
        total += objectValue;
    }
    group->updateBounds();
    return total;
}

// Discards a failed group: releases reservations, deletes its objects and
// resets it.
inline void TRmgTreasureGroup::discard()
{
    for (u32 index = 0; index < m_objects.size(); ++index) {
        m_objects[index]->releaseReservation();
        delete m_objects[index];
    }
    reset();
}

// Without a suitable guard creature the group stays unguarded; a failed
// guard fit destroys the group's objects and the unaccepted guard.
VA(0x005466e0, 0x253)
MAC_ADDRESS(0x24a7b0, 0x110) // MAC_ABSTRACTION_FROM(tokens1:4deef3890efa,29.6117): TRmgTreasureGroup::discard shares ordered reservation release, deletion and reset across four failed-placement paths.
b8 type_random_map_generator::assembleTreasureGroup(TRmgZone* zone,
    TRmgTreasureGroup* group, b8 compact, s32 minimum, s32 maximum)
{
    group->reset();
    s32 targetValue = maximum <= minimum ? maximum : rand() % (maximum - minimum) + minimum;
    s32 totalValue = fillTreasureGroup(zone, group, compact, targetValue);
    if (!totalValue)
        return false;
    s32 guardValue = getZoneGuardValue(totalValue, zone);
    if (guardValue > 0) {
        type_object* guard = createGuard(guardValue, zone);
        if (guard && !group->addGuard(guard)) {
            group->discard();
            delete guard;
            return false;
        }
    }
    group->traceOutline();
    group->markPlacementOutline();
    return true;
}

VA(0x00546940, 0x49)
void TRmgMapItem::setTerrain(s32 terrain, s32 frame,
    b8 flipX, b8 flipY)
{
    m_tile.m_landType = terrain;
    m_tile.m_terrainFrame = frame;
    m_tileData.m_terrainFlipX = flipX;
    m_tileData.m_terrainFlipY = flipY;
}

VA(0x00546990, 0x1e)
TRmgMapItem* type_random_map::getMapItem(s32 x, s32 y)
{
    return &m_mapItems[y * m_mapWidth + x];
}

VA_COMPGEN(0x00404200, 0x209, VECTOR_INSERT, Int)

VA_COMPGEN(0x00422f50, 0x1b1, VECTOR_INSERT_SINGLE, Int)

VA_COMPGEN(0x004347a0, 0x32e, VECTOR_INSERT, TRmgMapPosition)

VA_COMPGEN(0x0054c3f0, 0x21c, VECTOR_INSERT_SINGLE, TRmgMapPosition)

VA_COMPGEN(0x0054dd60, 0x15, STD_CONSTRUCT, TRmgMapPosition)

VA_COMPGEN(0x0054c730, 0x1dd, VECTOR_INSERT_SINGLE, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054c940, 0x23, VECTOR_DESTROY, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054d8b0, 0x38, VECTOR_UCOPY, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054d8f0, 0x29, VECTOR_UFILL, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054dd80, 0x104, STD_CONSTRUCT, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054da20, 0x19f, STD_FILL, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054dbc0, 0x1a0, STD_COPY_BACKWARD, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054c970, 0x22f, VECTOR_INSERT_SINGLE, TRmgZoneConnection)

VA_COMPGEN(0x005157d0, 0x1b, CLASS_CTOR, vector)

VA_COMPGEN(0x00536ba0, 0x18, DEFAULT_CTOR_CLOSURE, vector)

VA_COMPGEN(0x00536b60, 0x3d, IMPLICIT_DTOR, TRmgObjectPlacementRule)
MAC_COMPGEN_ADDRESS(0x235010, 0x84, IMPLICIT_DTOR, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054c170, 0x38, VECTOR_DTOR, TRmgObjectPlacementRule)

VA_COMPGEN(0x0054c1b0, 0x23, VECTOR_SIZE, TRmgZoneConnection)

VA_COMPGEN(0x0054cd70, 0x3d, VECTOR_ERASE, TPoint)

VA_COMPGEN(0x0054c610, 0x53, VECTOR_ERASE, TRmgMapPosition)

VA_COMPGEN(0x0054cdb0, 0x33, VECTOR_ERASE, Int)

VA_COMPGEN(0x0054cdf0, 0x1d3, VECTOR_INSERT, unsigned_char)

VA_COMPGEN(0x0054cfd0, 0x2f, VECTOR_ERASE, unsigned_char)

VA_COMPGEN(0x0054d9e0, 0x39, STD_COPY, TRmgMapPosition)

VA_COMPGEN(0x005093c0, 0x25, STD_COPY, Int)

VA_COMPGEN(0x0054df40, 0x25, STD_COPY, const_int)

// Group objects store local XY positions; translate them and replace Z with
// the destination level.
inline TRmgMapPosition type_object::getPlacedGroupPosition(
    const TRmgMapPosition& groupPosition) const
{
    TRmgMapPosition position = getPosition();
    position.m_x += groupPosition.m_x;
    position.m_y += groupPosition.m_y;
    position.m_z = groupPosition.m_z;
    return position;
}

VA(0x005469b0, 0x2b4)
MAC_ADDRESS(0x24a8c0, 0x44c)
void type_random_map_generator::commitTreasureGroup(TRmgTreasureGroup* group,
    TRmgMapPosition position)
{
    group->m_position = position;
    for (u32 i = 0; i < group->m_objects.size(); ++i) {
        type_object* object = group->m_objects[i];
        addObject(object, object->getPlacedGroupPosition(position));
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
            b8 obstacleFill = destination->hasObstacleFill();
            b8 pathClearance = destination->hasPathClearance();
            TRmgMapItem* source = group->m_map.getMapItem(point.m_x, point.m_y);
            if (destination->getLandType() != eTerrainWater
                && source->canBlockFloor()
                && destination->isPassableLand() && !destination->isObjectEntrance()) {
                destination->releasePathClearance();
                if (source->hasObstacleFill()) {
                    destination->markObstacleFill();
                }
            }
            // Copy the destination's earlier marks back to the group map.
            if (obstacleFill)
                source->markObstacleFill();
            else
                source->clearObstacleFill();
            if (pathClearance)
                source->openPath();
            else
                source->releasePathClearance();
        }
    }
    for (u32 objectIndex = 0; objectIndex < group->m_objects.size(); ++objectIndex)
        group->m_objects[objectIndex]->completePlacement();
}

VA(0x00546c70, 0x452)
MAC_ADDRESS(0x24ad0c, 0x6cc)
b8 type_random_map_generator::canPlaceTreasureGroup(TRmgTreasureGroup* group,
    TRmgMapPosition position, TRmgZone* zone)
{
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    for (u32 i = 0; i < group->m_objects.size(); ++i) {
        type_object* object = group->m_objects[i];
        if (m_map.isPlacementBlocked(object->m_properties,
                object->getPlacedGroupPosition(position), zoneIndex,
                RMG_REJECT_OBSTACLE_ENTRANCES))
            return false;
    }
    if (group->m_hasGuard) {
        TRmgMapPosition guardPosition = position + group->m_guardPosition;
        if (guardPosition.m_x < 1 || guardPosition.m_x + 1 >= m_map.m_mapWidth
            || guardPosition.m_y < 1 || guardPosition.m_y + 1 >= m_map.m_mapHeight)
            return false;
        for (s32 x = guardPosition.m_x - 1; x <= guardPosition.m_x + 1; ++x) {
            for (s32 y = guardPosition.m_y - 1; y <= guardPosition.m_y + 1; ++y) {
                TRmgMapItem* item = m_map.getMapItem(x, y, guardPosition.m_z);
                if (item->isObjectEntrance() && item->getEntranceObjectType() == MONSTER)
                    return false;
            }
        }
    }
    s32 firstDirection = RMG_DIRECTION_EAST;
    s32 lastDirection = RMG_DIRECTION_COUNT;
    b8 waterZone = zone->m_terrain == eTerrainWater;
    type_object* lastObject = group->m_objects.back();
    TObjectType* prototype = lastObject->m_properties->m_prototype;
    TRmgMapPosition entrance = lastObject->getEntrance();
    if (!isRmgEntranceOpenToNorth(prototype->getObjectType())) {
        firstDirection = RMG_DIRECTION_SOUTH_EAST;
        lastDirection = RMG_DIRECTION_SOUTH_WEST + 1;
    }
    s32 direction;
    for (direction = firstDirection; direction < lastDirection; ++direction) {
        TPoint point = g_rmgDirections[direction] + TRmgVector(entrance.m_x, entrance.m_y);
        TRmgMapItem* source = group->m_map.getMapItem(point.m_x, point.m_y);
        if (!source->isClearOutlineCell() || !source->isPlacementOutline())
            continue;
        TRmgMapPosition target = position + point;
        if (!m_map.containsXY(target))
            continue;
        TRmgMapItem* destination = m_map.getMapItem(target);
        if ((destination->getLandType() == eTerrainWater) == waterZone
            && destination->isClearOutlineCell())
            break;
    }
    if (direction == lastDirection)
        return false;
    b32 allowEntrances = group->objectsAllowEntrances() && !group->m_hasGuard;
    if (!m_map.hasConnectedOutline(group->m_outline, position, allowEntrances, zone,
            RMG_REQUIRE_PATH_CLEARANCE))
        return false;
    const TRmgZoneBounds& bounds = group->m_bounds;
    for (s32 groupY = bounds.m_minimumY; groupY < bounds.m_maximumY; ++groupY) {
        for (s32 groupX = bounds.m_minimumX; groupX < bounds.m_maximumX; ++groupX) {
            TRmgMapItem* source = group->m_map.getMapItem(groupX, groupY);
            if (!source->hasPathClearance()) {
                s32 x = groupX + position.m_x;
                s32 y = groupY + position.m_y;
                // Only the upper bounds are checked; callers keep translated
                // coordinates nonnegative.
                if (x < m_map.m_mapWidth && y < m_map.m_mapHeight
                    && m_map.getMapItem(x, y, position.m_z)->isObjectEntrance())
                    return false;
            }
        }
    }
    return true;
}

// A candidate position is where the group map's origin O lands, so group cell
// g goes to position + g. Its occupied bounds (#) must fit the zone bounds,
// and the object distance under their centre c must reach the spacing.
// North is up:
//   O . . . . .
//   . . # # # .
//   . . # c # .
//   . . # # # .
VA(0x005470d0, 0x286)
MAC_ADDRESS(0x24b3d8, 0x310)
b8 type_random_map_generator::placeTreasureGroup(TRmgTreasureGroup* group,
    TRmgZone* zone, s32 spacing)
{
    s32 zoneIndex = zone->m_templateZone->m_zoneIndex;
    std::vector<TRmgMapPosition> candidates;
    TRmgZoneBounds bounds = zone->m_bounds;
    TRmgZoneBounds groupBounds = group->m_bounds;
    bounds.m_minimumX -= groupBounds.m_minimumX;
    bounds.m_minimumY -= groupBounds.m_minimumY;
    bounds.m_maximumX += 1 - groupBounds.m_maximumX;
    bounds.m_maximumY += 1 - groupBounds.m_maximumY;
    TRmgMapPosition position = zone->getLevelPosition();
    TPoint center((groupBounds.m_minimumX + groupBounds.m_maximumX) / 2,
        (groupBounds.m_minimumY + groupBounds.m_maximumY) / 2);
    s32 bestScore = spacing;
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
    commitTreasureGroup(group, candidates[rand() % candidates.size()]);
    return true;
}

// Up to three attempts to assemble and place a group for one treasure band;
// groups that fail placement are discarded.
inline b8 type_random_map_generator::tryPlaceTreasureBand(TRmgZone* zone,
    TRmgTreasureGroup* group, b8 compact, const TRmgTreasureRange& range, s32 spacing)
{
    for (s32 attempt = 0; attempt < RMG_TREASURE_ATTEMPTS; ++attempt) {
        if (assembleTreasureGroup(zone, group, compact,
                range.m_minimum, range.m_maximum)) {
            if (placeTreasureGroup(group, zone, spacing))
                return true;
            group->discard();
        }
    }
    return false;
}

VA(0x00547360, 0x460)
MAC_ADDRESS(0x24b6e8, 0x358)
void type_random_map_generator::placeZoneTreasures(TRmgZone* zone)
{
    TRmgTemplateZone* templateZone = zone->m_templateZone;
    TRmgTreasureGroup group(RMG_TREASURE_GROUP_MAP_SIZE, RMG_TREASURE_GROUP_MAP_SIZE);
    const s32 bandCount = sizeof(templateZone->m_treasure) / sizeof(templateZone->m_treasure[0]);
    // Bands with a maximum value below 100 or no density are skipped.
    s32 densities[bandCount];
    for (s32 band = 0; band < bandCount; ++band) {
        const TRmgTreasureRange& range = templateZone->m_treasure[band];
        densities[band] = range.m_maximum >= 100 && range.m_density > 0
            ? range.m_density : 0;
    }
    b8 finished[bandCount];
    s32 totalDensity = 0;
    s32 densityProduct = 1;
    initializeRmgDensityCategories(densities, bandCount, finished, totalDensity, densityProduct);
    if (totalDensity == 0)
        return;
    s32 densityArea = zone->m_terrain == eTerrainWater
        ? RMG_WATER_TREASURE_DENSITY_AREA : RMG_LAND_TREASURE_DENSITY_AREA;
    s32 spacing = getRmgDensitySpacing(densityArea, totalDensity);
    // No band has placed a treasure yet.
    s32 weightedCounts[bandCount] = {0, 0, 0};
    s32 countSteps[bandCount];
    initializeRmgCategoryStrides(densities, weightedCounts, bandCount,
        densityProduct, countSteps, weightedCounts);
    for (;;) {
        s32 selected = selectRmgWeightedCategory(finished, weightedCounts, bandCount);
        if (selected == RMG_NO_CATEGORY)
            break;
        weightedCounts[selected] += countSteps[selected];
        TRmgTreasureRange& range = templateZone->m_treasure[selected];
        // On failure, retry the band with compact treasure selection.
        if (!tryPlaceTreasureBand(zone, &group, false, range, spacing)
            && !tryPlaceTreasureBand(zone, &group, true, range, spacing))
            finished[selected] = true;
    }
}

VA_COMPGEN(0x005477c0, 0xb3, IMPLICIT_DTOR, TRmgTreasureGroup)
MAC_COMPGEN_ADDRESS(0x24ba40, 0x90, IMPLICIT_DTOR, TRmgTreasureGroup)

static inline void queueRmgMovementStep(TRmgMapItem* destination,
    s32 cost, const TRmgMapPosition& previous, const TRmgMapPosition& next,
    std::vector<TRmgMapPosition>& positions, std::vector<s32>& costs)
{
    destination->setMovementCost(cost, previous);
    insertRmgWorkItem(positions, costs, next, cost);
}

// Road exits and entries share this restricted-approach policy.
static inline bool hasRmgRestrictedRoadApproach(TAdventureObjectType objectType)
{
    return !isRmgEntranceOpenToNorth(objectType)
        && !g_adventureObjectTraits[objectType].m_clearedOnVisit;
}

// Road search step costs; a diagonal step costs three times a cardinal one.
enum ERmgRoadStepCost {
    RMG_ROAD_MONOLITH_COST = 50,
    RMG_ROAD_GATE_COST = 1,
    RMG_ROAD_ALONG_ROAD_COST = 2,
    RMG_ROAD_OFF_ROAD_COST = 20,
    RMG_ROAD_DIAGONAL_FACTOR = 3
};

// Dijkstra-style relaxation uses the back of a descending worklist, retaining
// duplicate entries. Monolith/gate transitions precede neighbour relaxation.
VA(0x00547880, 0x7b1)
MAC_ADDRESS(0x24bbe0, 0x7e4)
void type_random_map_generator::buildRoadCostMap(TRmgMapPosition source)
{
    std::vector<TRmgMapPosition> openPositions;
    std::vector<s32> openCosts;

    m_map.seedMovementSearch(source, openPositions, openCosts);

    while (openPositions.size()) {
        TRmgMapPosition position;
        popRmgWorkItem(position, openPositions, openCosts);

        TRmgMapItem* mapItem = m_map.getMapItem(position);
        s32 currentCost = mapItem->m_movement.m_cost;
        b8 currentHasRoad = mapItem->m_tile.m_roadType != 0;
        s32 direction = RMG_DIRECTION_COUNT;
        if (mapItem->isObjectEntrance()) {
            type_object* object = mapItem->m_objects[0];
            TObjectType* prototype = object->m_properties->m_prototype;
            TAdventureObjectType objectType = prototype->getObjectType();
            if (hasRmgRestrictedRoadApproach(objectType))
                direction = RMG_FIRST_NORTHERN_DIRECTION;

            switch (objectType) {
            case LITH_ONEWAY_ENTRANCE:
            case LITH_ONEWAY_EXIT:
            case LITH_TWOWAY: {
                // Monoliths of the same subtype connect, in stored order.
                const std::vector<type_object*>& destinations = objectType == LITH_TWOWAY
                    ? m_monolithsTwoWay : m_monolithsOneWay;
                s32 subtype = prototype->getSubtype();
                for (s32 monolith = 0; monolith < destinations.size(); ++monolith) {
                    type_object* destination = destinations[monolith];
                    if (destination->m_properties->m_prototype->getSubtype() != subtype)
                        continue;

                    TRmgMapPosition nextPosition = destination->m_position;
                    TRmgMapItem* nextMapItem = m_map.getMapItem(nextPosition);
                    s32 nextCost = currentCost + RMG_ROAD_MONOLITH_COST;
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
                s32 nextCost = currentCost + RMG_ROAD_GATE_COST;
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

            if (nextMapItem->isObjectEntrance()) {
                TAdventureObjectType objectType = nextMapItem->getEntranceObjectType();
                const TAdvObjectTraits& traits = g_adventureObjectTraits[objectType];
                if (traits.m_blocksLanding && !traits.m_clearedOnVisit)
                    continue;
                if (hasRmgRestrictedRoadApproach(objectType)
                    && isRmgSouthwardDirection(direction))
                    continue;
            }

            s32 nextCost = currentHasRoad && nextMapItem->m_tile.m_roadType
                ? RMG_ROAD_ALONG_ROAD_COST : RMG_ROAD_OFF_ROAD_COST;
            if (isRmgDiagonalDirection(direction))
                nextCost *= RMG_ROAD_DIAGONAL_FACTOR;
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
b8 type_random_map_generator::paintRoad(TRmgMapPosition position, s32 roadType)
{
    b8 painted = false;
    for (;;) {
        s32 level = position.m_z;
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
    s32 mapItemCount = m_map.m_numberLevels * m_map.m_mapHeight * m_map.m_mapWidth;
    while (mapItemCount--) {
        mapItem->resetMovement();
        ++mapItem;
    }
}

// Newly painted roads change traversal costs for subsequent targets.
inline void type_random_map_generator::rebuildRoadCostMap(const TRmgMapPosition& source)
{
    resetMovementCosts();
    buildRoadCostMap(source);
}

// Road-layer types, dirt through cobblestone; zero is no road.
enum ERmgRoadType {
    RMG_ROAD_DIRT = 1,
    RMG_ROAD_GRAVEL = 2,
    RMG_ROAD_COBBLESTONE = 3,
    RMG_ROAD_TYPE_COUNT = RMG_ROAD_COBBLESTONE
};

// Every road on the map uses one random road type.
VA(0x00548290, 0x26e)
MAC_ADDRESS(0x24c6d4, 0x1d8)
void type_random_map_generator::createRoads()
{
    ERmgRoadType roadType =
        static_cast<ERmgRoadType>(rand() % RMG_ROAD_TYPE_COUNT + RMG_ROAD_DIRT);
#if defined(HOMM3_RMG_HOTFIX)
    // No road targets means no roads.
    for (u32 first = 0; first + 1 < m_roadTargets.size(); ++first) {
#else
    // Retail bug: an empty target list underflows size() - 1.
    for (u32 first = 0; first < m_roadTargets.size() - 1; ++first) {
#endif
        TRmgMapPosition source = m_roadTargets[first];
        rebuildRoadCostMap(source);
        for (u32 second = first + 1; second < m_roadTargets.size(); ++second) {
            TRmgMapPosition destination = m_roadTargets[second];
            if (m_map.getMapItem(destination)->m_movement.m_cost <= RMG_REACHED_COST_LIMIT
                && paintRoad(destination, roadType)
                && second < m_roadTargets.size() - 1) {
                rebuildRoadCostMap(source);
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
static inline s32 getRmgRiverStepCost(s32 currentCost, const TRmgMapItem* destination)
{
    s32 nextCost = currentCost + (rand() & 31) + 1;
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
    b8& sourceIsSnow, ERmgRiverType& riverType)
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
inline bool TRmgMapItem::isRiverTerrain(b8 sourceIsSnow) const
{
    return getLandType() != eTerrainWater
        && getLandType() != eTerrainRock
        && (getLandType() == eTerrainSnow) == sourceIsSnow;
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
    std::vector<s32> openCosts;
    TRmgMapItem* mapItem = m_map.seedMovementSearch(source,
        openPositions, openCosts);
    b8 sourceIsSnow;
    ERmgRiverType riverType;
    selectRmgRiverAppearance(mapItem, sourceIsSnow, riverType);
    TRmgMapPosition nextPosition;
    while (!openPositions.empty()) {
        TRmgMapPosition position;
        popRmgWorkItem(position, openPositions, openCosts);
        mapItem = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
        s32 positionCost = mapItem->m_movement.m_cost;
        for (s32 direction = RMG_DIRECTION_EAST; direction < RMG_DIRECTION_COUNT;
             direction += RMG_CARDINAL_DIRECTION_STEP) {
            nextPosition = position + g_rmgDirections[direction];
            if (!m_map.containsXY(nextPosition))
                continue;
            mapItem = m_map.getMapItem(nextPosition.m_x, nextPosition.m_y, nextPosition.m_z);
            if (!mapItem->isRiverTerrain(sourceIsSnow))
                continue;
            s32 nextCost = getRmgRiverStepCost(positionCost, mapItem);
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
#if defined(HOMM3_RMG_HOTFIX)
    // Draw only to a river cell the search reached.
    if (!mapItem->hasRiver() || mapItem->m_movement.m_cost >= RMG_UNREACHED_COST)
        return;
#else
    // Retail bug: tests the last inspected tile, as in createRiver.
    if (!mapItem->hasRiver())
        return;
#endif
    type_random_map levelMap(m_map.getMapItem(0, 0, nextPosition.m_z),
        m_map.getWidth(), m_map.getHeight());
    TRmgRiverMapAdapter mapAdapter(&levelMap);
    TRmgRiverPainter riverPainter(
        &mapAdapter, riverType, TRmgGridPoint(nextPosition.m_x, nextPosition.m_y));
    while (mapItem->m_movement.m_cost > 0) {
        TRmgMapPosition position = mapItem->m_previousTile;
        mapItem = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
        riverPainter.drawTo(TRmgGridPoint(position.m_x, position.m_y));
    }
}

// Blocked-direction bits are indexed by cardinal (direction / 2); this is
// the bit of the cardinal opposite an eight-way direction.
static inline s32 getRmgOppositeCardinalBit(s32 direction)
{
    return 1 << (getRmgOppositeDirection(direction) / RMG_CARDINAL_DIRECTION_STEP);
}

// Retail bug: this scan admits x == width, reading the next row's first cell
// or, at the end of the map, past the cell array.
inline bool type_random_map::isOutsideRiverCoastScan(const TRmgMapPosition& point) const
{
#if defined(HOMM3_RMG_HOTFIX)
    // The coast scan stops at the right edge like the others.
    return point.m_x < 0 || point.m_x >= m_mapWidth
        || point.m_y < 0 || point.m_y >= m_mapHeight;
#else
    return point.m_x < 0 || point.m_x > m_mapWidth
        || point.m_y < 0 || point.m_y >= m_mapHeight;
#endif
}

// The dry strip and inland approach exclude water and entrances; rock is
// allowed.
inline TRmgMapItem* type_random_map::getDryRiverCoastCell(const TRmgMapPosition& point)
{
    if (isOutsideRiverCoastScan(point))
        return 0;
    TRmgMapItem* item = getMapItem(point.m_x, point.m_y, point.m_z);
    if (item->getLandType() == eTerrainWater || item->isObjectEntrance())
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
VA(0x00548a40, 0x222)
MAC_ADDRESS(0x24ce88, 0x404)
void type_random_map_generator::markRiverCoastTarget(TRmgMapPosition position, s32 direction)
{
    TRmgMapPosition point = position + g_rmgDirections[turnRmgDirection(direction, 2)];
    TPoint step = g_rmgDirections[turnRmgDirection(direction, -2)];
    for (s32 waterCount = 0; waterCount < 3; ++waterCount) {
        if (m_map.isOutsideRiverCoastScan(point))
            return;
        if (m_map.getMapItem(point.m_x, point.m_y, point.m_z)->getLandType() != eTerrainWater)
            return;
        point += step;
    }
    point = position + g_rmgDirections[turnRmgDirection(direction, 1)];
    for (s32 dryCount = 0; dryCount < 3; ++dryCount) {
        if (!m_map.getDryRiverCoastCell(point))
            return;
        point += step;
    }
    point = position + g_rmgDirections[direction];
    TRmgMapItem* item;
    for (s32 inlandCount = 0; inlandCount < 4; ++inlandCount) {
        item = m_map.getDryRiverCoastCell(point);
        if (!item)
            return;
        point += g_rmgDirections[direction];
    }
    item->m_tileData.m_blockedDirections |= getRmgOppositeCardinalBit(direction);
    item->m_tileData.m_riverTarget = true;
}

VA(0x00548c70, 0x17d)
MAC_ADDRESS(0x24d28c, 0x264)
void type_random_map_generator::markRiverTargets()
{
    TRmgMapPosition position;
    TRmgMapItem* item = m_map.m_mapItems;
    for (position.m_z = RMG_SURFACE_LEVEL; position.m_z < m_map.m_numberLevels; ++position.m_z) {
        for (position.m_y = 0; position.m_y < m_map.m_mapHeight; ++position.m_y) {
            for (position.m_x = 0; position.m_x < m_map.m_mapWidth; ++position.m_x, ++item) {
                if (item->getLandType() == eTerrainWater) {
                    for (s32 direction = RMG_DIRECTION_EAST; direction < RMG_DIRECTION_COUNT;
                         direction += RMG_CARDINAL_DIRECTION_STEP)
                        markRiverCoastTarget(position, direction);
                }
            }
        }
    }
    for (s32 level = RMG_SURFACE_LEVEL; level < m_map.m_numberLevels; ++level) {
        for (s32 y = 0; y < m_map.m_mapHeight; ++y) {
            m_map.getMapItem(0, y, level)->m_tileData.m_riverTarget = true;
            m_map.getMapItem(m_map.m_mapWidth - 1, y, level)->m_tileData.m_riverTarget = true;
        }
        for (s32 x = 0; x < m_map.m_mapWidth; ++x) {
            m_map.getMapItem(x, 0, level)->m_tileData.m_riverTarget = true;
            m_map.getMapItem(x, m_map.m_mapHeight - 1, level)->m_tileData.m_riverTarget = true;
        }
    }
    if (m_progress)
        m_progress->advance(1000);
}

// Randomized best-first relaxation, with the same per-visit random edge costs
// as createRiverToObject, seeded at the water wheel's south-west cell and the
// cells north and north-east of it.
VA(0x00548df0, 0x99f)
MAC_ADDRESS(0x24d4f0, 0xb00)
void type_random_map_generator::createRiver(TRmgMapPosition source)
{
    resetMovementCosts();

    std::vector<TRmgMapPosition> openPositions;
    std::vector<s32> openCosts;

    TRmgMapItem* mapItem = m_map.seedMovementSearch(source, openPositions, openCosts);

    b8 sourceIsSnow;
    ERmgRiverType riverType;
    selectRmgRiverAppearance(mapItem, sourceIsSnow, riverType);

    m_map.seedMovementSearch(source + TPoint(0, -1), openPositions, openCosts);
    m_map.seedMovementSearch(source + TPoint(1, -1), openPositions, openCosts);

    TRmgMapPosition nextPosition;

    while (!openPositions.empty()) {
        TRmgMapPosition position;
        popRmgWorkItem(position, openPositions, openCosts);

        mapItem = m_map.getMapItem(position);
        s32 positionCost = mapItem->m_movement.m_cost;
        for (s32 direction = RMG_DIRECTION_EAST; direction < RMG_DIRECTION_COUNT;
             direction += RMG_CARDINAL_DIRECTION_STEP) {
            nextPosition = position + g_rmgDirections[direction];

            if (!m_map.containsXY(nextPosition))
                continue;

            mapItem = m_map.getMapItem(nextPosition);
            if (!mapItem->isRiverTerrain(sourceIsSnow) || mapItem->isNearRiver())
                continue;

            s32 nextCost = getRmgRiverStepCost(positionCost, mapItem);

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

#if defined(HOMM3_RMG_HOTFIX)
    // Draw only to a target the search reached.
    if (!mapItem->isRiverTarget() || mapItem->m_movement.m_cost >= RMG_UNREACHED_COST)
        return;
#else
    // Retail bug: tests the last inspected tile, even an unreached one.
    if (!mapItem->isRiverTarget())
        return;
#endif

    type_random_map levelMap(m_map.getMapItem(0, 0, nextPosition.m_z),
        m_map.m_mapWidth, m_map.m_mapHeight);
    TRmgRiverMapAdapter mapAdapter(&levelMap);
    TRmgRiverPainter riverPainter(
        &mapAdapter, riverType, TRmgGridPoint(nextPosition.m_x, nextPosition.m_y));

    if (mapItem->m_tileData.m_blockedDirections) {
        s32 cardinal;
        for (cardinal = 0; cardinal < RMG_CARDINAL_DIRECTION_COUNT; ++cardinal) {
            if (mapItem->m_tileData.m_blockedDirections & (1 << cardinal))
                break;
        }

        DATA_COMPGEN_GUARD(0x0069d59c, riverDeltaOffsetsGuard, riverDeltaOffsets)

        VA_COMPGEN(0x00549790, 0x1, STATIC_DTOR, riverDeltaOffsets)
        DATA(0x0069ce28)
        static TRmgRiverDeltaOffset riverDeltaOffsets[RMG_CARDINAL_DIRECTION_COUNT] = {
            TRmgRiverDeltaOffset(4, 1),  // coast to the east
            TRmgRiverDeltaOffset(1, 4),  // south
            TRmgRiverDeltaOffset(-2, 1), // west
            TRmgRiverDeltaOffset(1, -2)  // north
        };

        s32 deltaIndex = sourceIsSnow
            ? g_snowRiverDeltaIndex[cardinal]
            : g_landRiverDeltaIndex[cardinal];
        s32 landType = mapItem->getLandType();
        s32 prototypeIndex;
        for (prototypeIndex = 0; prototypeIndex < m_objectPrototypes[TERRAIN_RIVER_DELTA].size();
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
                nextPosition.m_x + riverDeltaOffsets[cardinal].m_x,
                nextPosition.m_y + riverDeltaOffsets[cardinal].m_y,
                nextPosition.m_z));

        TRmgMapPosition mouth =
            nextPosition + g_rmgDirections[cardinal * RMG_CARDINAL_DIRECTION_STEP];
        riverPainter.drawTo(TRmgGridPoint(mouth.m_x, mouth.m_y));
        mapItem = m_map.getMapItem(mouth);
        mapItem->m_tileData.m_riverTarget = true;

        riverPainter.drawTo(TRmgGridPoint(nextPosition.m_x, nextPosition.m_y));
        mapItem = m_map.getMapItem(nextPosition);
    }

    while (mapItem->m_movement.m_cost > 0) {
        TRmgMapPosition position = mapItem->m_previousTile;
        mapItem = m_map.getMapItem(position);
        mapItem->m_tileData.m_riverTarget = true;
        riverPainter.drawTo(TRmgGridPoint(position.m_x, position.m_y));
    }
}

VA(0x005497a0, 0xce)
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
                position = object->getEntrance();
            } else {
                position = object->m_position;
                position -= TPoint(static_cast<u32>(prototype->getWidth()) / 2,
                    static_cast<u32>(prototype->getHeight()) / 2);
            }
            if (m_map.containsXY(position)) {
                TRmgMapItem* item = m_map.getMapItem(position.m_x, position.m_y, position.m_z);
                item->m_tileData.m_hasRiver = true;
            }
        }
    }
    if (m_progress)
        m_progress->advance(1000);
}

VA(0x00549870, 0xb1)
MAC_ADDRESS(0x24e17c, 0x118)
void type_random_map_generator::createRivers()
{
    markRiverObjectTargets();
    markRiverTargets();
    for (u32 index = 0; index < m_objects.size(); ++index) {
        type_object* object = m_objects[index];
        TObjectType* prototype = object->m_properties->m_prototype;
        if (prototype->getObjectType() == WATER_WHEEL) {
            TRmgMapPosition entrance = object->getEntrance();
            createRiverToObject(entrance);
            createRiver(entrance + TPoint(-2, 0));
            if (m_progress)
                m_progress->advance(1000);
        }
    }
}

// Assigns players to template slots, lays out zones and terrain, places
// towns, connections, mines and treasures, then decorates and adds roads
// and rivers.
VA(0x00549930, 0x37b)
MAC_ADDRESS(0x24e294, 0x41c)
b8 type_random_map_generator::generate()
{
    if (!m_templates.size())
        return false;
    u32 selected = rand() % m_templates.size();
    m_templateName = m_templates[selected]->m_name;
    b8 humanSlots[RMG_PLAYER_COUNT];
    s32 humanSlotByte;
    MEMSET(humanSlots, 0, sizeof(humanSlots), humanSlotByte);
    b8 allSlots[RMG_PLAYER_COUNT];
    s32 allSlotByte;
    MEMSET(allSlots, 0, sizeof(allSlots), allSlotByte);
    for (u32 zone = 0; zone < m_templates[selected]->m_zones.size(); ++zone) {
        TRmgTemplateZone* templateZone = m_templates[selected]->m_zones[zone];
        if (templateZone->m_kind == RMG_TEMPLATE_HUMAN) {
            humanSlots[templateZone->m_playerIndex] = true;
            allSlots[templateZone->m_playerIndex] = true;
        } else if (templateZone->m_kind == RMG_TEMPLATE_COMPUTER) {
            allSlots[templateZone->m_playerIndex] = true;
        }
    }
    s32 mapIndex;
    MEMSET(m_playerIndexMap, -1, sizeof(m_playerIndexMap), mapIndex);
    s32 playerOrder[RMG_PLAYER_COUNT];
    s32 orderedCount = 0;
    for (s32 player = 0; player < RMG_PLAYER_COUNT; ++player)
        if (m_fixedHumanPlayers[player])
            playerOrder[orderedCount++] = player;
    for (player = 0; player < RMG_PLAYER_COUNT; ++player)
        if (!m_fixedHumanPlayers[player])
            playerOrder[orderedCount++] = player;
    s32 slot = 0;
    s32 orderIndex;
    for (orderIndex = 0; orderIndex < m_humanPlayerCount; ++orderIndex) {
        while (slot < RMG_PLAYER_COUNT && !humanSlots[slot])
            ++slot;
        allSlots[slot] = false;
        m_playerIndexMap[++slot] = playerOrder[orderIndex];
    }
    slot = 0;
    for (; orderIndex < m_humanPlayerCount + m_computerPlayerCount; ++orderIndex) {
        while (slot < RMG_PLAYER_COUNT && !allSlots[slot])
            ++slot;
        m_playerIndexMap[++slot] = playerOrder[orderIndex];
    }
    initializeZones(m_templates[selected]);
    for (s32 level = RMG_SURFACE_LEVEL; level < m_map.m_numberLevels; ++level)
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
    memset(m_townZoneCountsByAlignment, 0, sizeof(m_townZoneCountsByAlignment));
    m_townZoneCount = 0;
    for (zone = 0; zone < m_zones.size(); ++zone) {
        if (m_zones[zone]->m_hasPrimaryTown) {
            ++m_townZoneCountsByAlignment[m_zones[zone]->m_alignment];
            ++m_townZoneCount;
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

template <u32 N>
void encodePackedBits(const std::bitset<N>& bits, u8* packed)
{
    memset(packed, 0, (N + 7) / 8);
    for (u32 index = 0; index < N; ++index) {
        if (bits.test(index))
            packed[index / 8] |= 1 << (index % 8);
    }
}

template <u32 N>
s32 writePackedBits(TAbstractFile* outputFile, const std::bitset<N>& bits)
{
    u8 packed[(N + 7) / 8];
    encodePackedBits(bits, packed);
    return outputFile->write(packed, sizeof(packed));
}

// Length-prefixed text: a 32-bit length followed by the characters.
s32 writeString(TAbstractFile* outputFile, const std::string& text)
{
    writeValue<s32>(outputFile, text.length());
    return outputFile->write(text.c_str(), text.length());
}

s32 writeString(TAbstractFile* outputFile, const char* text)
{
    writeValue<s32>(outputFile, strlen(text));
    return outputFile->write(text, strlen(text));
}

// Artifact ids an AB map knows: it ends before the SoD combination artifacts.
enum ERmgArtifactCount {
    RMG_AB_ARTIFACT_COUNT = ARTIFACT_ANGELIC_ALLIANCE
};

// The shared artifact enum has no enumerator for the Vial of Dragon Blood.
static const s32 g_rmgArtifactVialOfDragonBlood = 127;

// Each player clause in the map description appends a separator, the player
// colour and the clause text, using unchecked strcat.
static inline void appendRmgPlayerDescription(char* description, s32 player,
    const char* clause)
{
    strcat(description, DATA_COMPGEN(0x0066032c, rmgListSeparator, ", "));
    strcat(description, g_rmgPlayerNames[player]);
    strcat(description, clause);
}

VA(0x00549cb0, 0xe90)
MAC_ADDRESS(0x24e7d4, 0x11ac)
void type_random_map_generator::writeMapHeader(TAbstractFile* outputFile)
{
    writeValue<s32>(outputFile, getSerializedMapVersion());

    writeValue<b8>(outputFile, 1); // playable

    writeValue<s32>(outputFile, m_map.getWidth());

    writeValue<b8>(outputFile, m_map.m_numberLevels > 1);

    std::string mapName(
        DATA_COMPGEN(0x00682900, rmgMapName, "Random Map"));
    writeString(outputFile, mapName);

#if defined(HOMM3_RMG_HOTFIX)
    // With the name capped at 255 characters, a description fits in 1024.
    char description[1024];
    if (m_templateName.size() > 255)
        m_templateName.resize(255);
#else
    // Retail bug: unchecked sprintf/strcat can overflow this buffer.
    char description[500];
#endif
    sprintf(
        description,
        DATA_COMPGEN(
            0x0068286c,
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
            DATA_COMPGEN(0x0068282c, rmgOriginalMap, ", original map"));
        break;
    case RMG_MAP_ARMAGEDDONS_BLADE:
        strcat(
            description,
            DATA_COMPGEN(
                0x0068283c, rmgFirstExpansionMap, ", first expansion map"));
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

    for (s32 descriptionPlayer = 0; descriptionPlayer < RMG_PLAYER_COUNT;
         ++descriptionPlayer) {
        if (m_fixedHumanPlayers[descriptionPlayer]) {
            appendRmgPlayerDescription(description, descriptionPlayer,
                DATA_COMPGEN(0x00682820, rmgIsHuman, " is human"));
        }

        if (m_townChoices[descriptionPlayer] != eTownNeutral) {
            appendRmgPlayerDescription(description, descriptionPlayer,
                DATA_COMPGEN(
                    0x0068280c, rmgTownChoiceIs, " town choice is "));
            strcat(
                description,
                // Retail bug: indexed by the player rather than the chosen
                // town, so the named town can be wrong.
                g_rmgTownNames[descriptionPlayer]);
        }
    }

    writeString(outputFile, description);

    writeValue<u8>(outputFile, 1); // difficulty
    if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<u8>(outputFile, 0); // hero level limit
    }

    b8 canBeHuman[RMG_PLAYER_COUNT];
    // One bit per town type of the player's zones.
    s32 legalAlignments[RMG_PLAYER_COUNT];
    TRmgMapPosition mainTowns[RMG_PLAYER_COUNT];
    b8 canBeComputer[RMG_PLAYER_COUNT];
    memset(canBeHuman, 0, sizeof(canBeHuman));
    memset(legalAlignments, 0, sizeof(legalAlignments));
    memset(mainTowns, 0, sizeof(mainTowns));
    memset(canBeComputer, 0, sizeof(canBeComputer));
    s32 generatedHumanTowns = 0;
    for (u32 zoneIndex = 0; zoneIndex < m_zones.size(); ++zoneIndex) {
        TRmgZone* zone = m_zones[zoneIndex];
        TRmgTemplateZone* templateZone = zone->m_templateZone;
        s32 templatePlayer = templateZone->m_playerIndex;
        if (templatePlayer < 0)
            continue;

        s32 player = m_playerIndexMap[templatePlayer + 1];
        if (player < 0 || !zone->m_hasPrimaryTown)
            continue;

        if (templateZone->m_kind == RMG_TEMPLATE_HUMAN && !canBeHuman[player]) {
            ++generatedHumanTowns;
            canBeHuman[player] = true;
            mainTowns[player] = zone->m_primaryTownEntrance;
        }

        if (templateZone->m_kind == RMG_TEMPLATE_COMPUTER && !canBeComputer[player]) {
            canBeComputer[player] = true;
            mainTowns[player] = zone->m_primaryTownEntrance;
        }

        legalAlignments[player] |= 1 << zone->m_alignment;
    }

    s32 surplusHumanTowns = generatedHumanTowns - m_humanPlayerCount;
    for (s32 player = RMG_PLAYER_COUNT - 1; player >= 0; --player) {
        if (canBeHuman[player] && !m_fixedHumanPlayers[player]
            && surplusHumanTowns > 0) {
            canBeComputer[player] = true;
            canBeHuman[player] = false;
            --surplusHumanTowns;
        }
    }

    m_computerPlayerCount = m_humanPlayerCount = 0;

    for (s32 serializedPlayer = 0; serializedPlayer < RMG_PLAYER_COUNT;
         ++serializedPlayer) {
        writeValue<char>(outputFile, canBeHuman[serializedPlayer]);

        writeValue<b8>(outputFile, canBeHuman[serializedPlayer] || canBeComputer[serializedPlayer]);

        writeValue<u8>(outputFile, 0); // AI strategy

        if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
            writeValue<u8>(outputFile, 0); // byte the reader skips
        }

        if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
            writeValue<u16>(outputFile, legalAlignments[serializedPlayer]);
        } else {
            writeValue<u8>(outputFile, legalAlignments[serializedPlayer]);
        }

        writeValue<b8>(outputFile, 0); // random alignment

        if (!canBeHuman[serializedPlayer]
            && !canBeComputer[serializedPlayer]) {
            writeValue<b8>(outputFile, 0); // no main town
        } else {
            if (canBeHuman[serializedPlayer])
                ++m_humanPlayerCount;
            else
                ++m_computerPlayerCount;

            writeValue<b8>(outputFile, 1); // has main town

            if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
                writeValue<b8>(outputFile, 1); // generate hero there
                writeValue<s8>(outputFile, eTownNeutral); // main town type
            }

            writeRmgMapPosition(outputFile, mainTowns[serializedPlayer]);
        }

        writeValue<b8>(outputFile, 0); // random hero
        writeValue<s8>(outputFile, heroIdNone); // main custom hero

        if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
            writeValue<u8>(outputFile, 0); // placeholder heroes
            writeValue<s32>(outputFile, 0); // hero count
        }
    }

    writeValue<s8>(outputFile, -1); // no special victory condition
    writeValue<s8>(outputFile, -1); // no special loss condition

    if (!m_computerTeamCount)
        m_computerTeamCount = m_computerPlayerCount;
    if (!m_humanTeamCount)
        m_humanTeamCount = m_humanPlayerCount;
    if (!m_computerPlayerCount)
        m_humanTeamCount = max(m_humanTeamCount, 2);

    if (m_humanTeamCount >= m_humanPlayerCount
        && m_computerTeamCount >= m_computerPlayerCount) {
        writeValue<u8>(outputFile, 0); // no teams
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

        writeValue<u8>(outputFile, m_humanTeamCount + m_computerTeamCount);
        outputFile->write(teams, sizeof(teams));
    }

    if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
        std::bitset<RMG_HERO_COUNT> availableHeroes;
        setAvailableRmgHeroes(
            &availableHeroes, m_disabledHeroes, m_disabledHeroes + RMG_HERO_COUNT);

        writePackedBits(outputFile, availableHeroes);
    } else {
        std::bitset<RMG_ROE_HERO_COUNT> availableHeroes;
        setAvailableRmgHeroes(
            &availableHeroes, m_disabledHeroes, m_disabledHeroes + RMG_ROE_HERO_COUNT);

        writePackedBits(outputFile, availableHeroes);
    }

    if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
        writeValue<s32>(outputFile, 0);
    }
    if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
        writeValue<u8>(outputFile, 0);
    }

    writeRmgReservedBytes(outputFile, 31);

    // Combination artifacts, the Vial of Dragon Blood and Armageddon's Blade
    // are disabled.
    std::bitset<ARTIFACT_COUNT> disabledArtifacts;
    for (s32 artifactIndex = ARTIFACT_SPELLBOOK; artifactIndex < ARTIFACT_COUNT; ++artifactIndex) {
        disabledArtifacts[artifactIndex] =
            g_artifactTraits[artifactIndex].m_comboType != -1;
    }
    disabledArtifacts.set(ARTIFACT_ARMAGEDDONS_BLADE);
    disabledArtifacts.set(g_rmgArtifactVialOfDragonBlood);

    if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
        writePackedBits(outputFile, disabledArtifacts);
    } else if (m_mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE) {
        std::bitset<RMG_AB_ARTIFACT_COUNT> legacyDisabledArtifacts;
        std::copy(
            bitset_iterator<ARTIFACT_COUNT>(disabledArtifacts, 0),
            bitset_iterator<ARTIFACT_COUNT>(disabledArtifacts, RMG_AB_ARTIFACT_COUNT),
            bitset_iterator<RMG_AB_ARTIFACT_COUNT>(legacyDisabledArtifacts, 0));

        writePackedBits(outputFile, legacyDisabledArtifacts);
    }

    if (m_mapVersion >= RMG_MAP_SHADOW_OF_DEATH) {
        std::bitset<hero::NUM_SPELLS> disabledSpells;
        writePackedBits(outputFile, disabledSpells);

        std::bitset<kNumSecSkills> disabledSkills;
        writePackedBits(outputFile, disabledSkills);

        for (s32 hero = 0; hero < RMG_HERO_COUNT; ++hero) {
            writeValue<u8>(outputFile, 0);
        }
    }
}

VA(0x0054ab40, 0xad)
MAC_ADDRESS(0x24e6b0, 0x124)
static void __fastcall assignRmgTeams(
    s32 teamCount,
    s32 playerCount,
    s32 firstTeam,
    const b8* players,
    char* teams)
{
    s32 playersPerTeam[RMG_PLAYER_COUNT];
    s32 team;

    for (team = 0; team < teamCount; ++team) {
        playersPerTeam[team] =
            playerCount / teamCount + (playerCount % teamCount > team);
    }

    for (s32 player = 0; player < RMG_PLAYER_COUNT; ++player) {
        if (!players[player])
            continue;

        s32 nonemptyTeams = 0;
        for (team = 0; team < teamCount; ++team) {
            if (playersPerTeam[team] > 0)
                ++nonemptyTeams;
        }

        s32 selected = rand() % nonemptyTeams;
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
VA(0x0054abf0, 0x235)
MAC_ADDRESS(0x24fd18, 0x350)
b8 type_random_map_generator::writeMap(TAbstractFile* outputFile)
{
    writeMapHeader(outputFile);
    writeValue<s32>(outputFile, 0);
    TRmgMapItem* item = m_map.m_mapItems;
    s32 itemCount = m_map.m_numberLevels * m_map.m_mapHeight * m_map.m_mapWidth;
    while (itemCount--) {
        item->write(outputFile);
        ++item;
    }
    s32 prototypeCount = 2;
    for (s32 type = NOTHING; type < ADVENTURE_OBJECT_TRAIT_COUNT; ++type)
        for (u32 index = 0; index < m_objectPrototypes[type].size(); ++index) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[type][index];
            if (static_cast<s32>(properties->m_refCount) > 0)
                properties->m_prototypeIndex = prototypeCount++;
        }
    writeValue<s32>(outputFile, prototypeCount);
    writeRmgObjectPrototype(outputFile, m_objectPrototypes[RANDOM_MONSTER][0]->m_prototype);
    writeRmgObjectPrototype(outputFile, m_objectPrototypes[TERRAIN_HOLE][0]->m_prototype);
    for (s32 objectType = NOTHING; objectType < ADVENTURE_OBJECT_TRAIT_COUNT; ++objectType)
        for (u32 index = 0; index < m_objectPrototypes[objectType].size(); ++index) {
            TRmgObjectPropertiesRef* properties = m_objectPrototypes[objectType][index];
            if (static_cast<s32>(properties->m_refCount) > 0)
                writeRmgObjectPrototype(outputFile, properties->m_prototype);
        }
    writeValue<s32>(outputFile, m_objects.size());
    for (u32 first = 0; first < m_objects.size(); ++first) {
        type_object* object = m_objects[first];
        if (g_adventureObjectTraits[object->m_properties->m_prototype->getObjectType()].m_isDecoration)
            object->write(outputFile, m_mapVersion);
    }
    for (u32 second = 0; second < m_objects.size(); ++second) {
        type_object* object = m_objects[second];
        if (!g_adventureObjectTraits[object->m_properties->m_prototype->getObjectType()].m_isDecoration)
            object->write(outputFile, m_mapVersion);
    }
    if (m_progress)
        m_progress->advance(2000);
    return writeValue<s32>(outputFile, 0) == sizeof(s32);
}

enum TRmgPrototypeCellMask {
    RMG_PROTOTYPE_PASSABLE_CELLS,
    RMG_PROTOTYPE_TRIGGER_CELLS
};

// H3M stores each fixed 8x6 footprint in reverse row/column order, packed
// least-significant bit first. On the map that is reading order: North is up,
// numbers are bit indices, byte k holds row k and P is the object's position.
//    0  1 ..  6  7
//    8  9 .. 14 15
//   ..
//   40 41 .. 46  P   (bit 47)
static inline void writeRmgPrototypeCellMask(TAbstractFile* outputFile,
    TObjectType* prototype, TRmgPrototypeCellMask kind)
{
    u8 mask[6];
    memset(mask, 0, sizeof(mask));
    s32 bit = 0;
    for (s32 y = 5; y >= 0; --y) {
        for (s32 x = 7; x >= 0; --x) {
            if (kind == RMG_PROTOTYPE_PASSABLE_CELLS
                ? prototype->isPassableCell(x, y) : prototype->isTriggerCell(x, y))
                mask[bit / 8] |= 1 << (bit % 8);
            ++bit;
        }
    }
    outputFile->write(mask, sizeof(mask));
}

VA(0x0054ae30, 0x2c5)
MAC_ADDRESS(0x24f980, 0x398)
void __fastcall writeRmgObjectPrototype(TAbstractFile* outputFile, TObjectType* prototype)
{
    writeString(outputFile, prototype->getImageName());
    writeRmgPrototypeCellMask(outputFile, prototype, RMG_PROTOTYPE_PASSABLE_CELLS);
    writeRmgPrototypeCellMask(outputFile, prototype, RMG_PROTOTYPE_TRIGGER_CELLS);
    writePackedBits(outputFile, prototype->m_terrainMask);
    writePackedBits(outputFile, prototype->m_recommendedTerrainMask);
    writeValue<s32>(outputFile, prototype->getObjectType());
    writeValue<s32>(outputFile, prototype->getSubtype());
    writeValue<u8>(outputFile, prototype->m_slotCategory);
    writeValue<b8>(outputFile, prototype->isUnderlay());
    s32 reserved[4];
    memset(reserved, 0, sizeof(reserved));
    outputFile->write(reserved, sizeof(reserved));
}

// Prisons draw from the RoE heroes, or in later formats from the first
// RMG_PRISON_HERO_COUNT.
static inline s32 getRmgPrisonHeroCount(s32 mapVersion)
{
    return mapVersion >= RMG_MAP_ARMAGEDDONS_BLADE
        ? RMG_PRISON_HERO_COUNT : RMG_ROE_HERO_COUNT;
}

VA(0x0054b100, 0x71)
MAC_ADDRESS(0x250068, 0xd4)
s32 type_random_map_generator::selectPrisonHero()
{
    s32 available = 0;
    s32 hero;
    for (hero = getRmgPrisonHeroCount(m_mapVersion) - 1; hero >= 0; --hero) {
        if (!m_disabledHeroes[hero])
            ++available;
    }
    if (!available)
        return heroIdNone;

    s32 selected = rand() % available;
    for (hero = getRmgPrisonHeroCount(m_mapVersion) - 1; hero >= 0; --hero) {
        if (!m_disabledHeroes[hero] && --selected < 0)
            break;
    }
    m_disabledHeroes[hero] = true;
    return hero;
}

// Priority worklist ordered by each zone's quest placement score.
static void insertRmgWorkItem(std::vector<TRmgZone*>& zones, TRmgZone* zone)
{
    s32 first = 0;
    s32 last = zones.size();
    s32 priority = zone->m_questPlacementScore;
    while (first < last) {
        s32 middle = (first + last) >> 1;
        if (priority < zones[middle]->m_questPlacementScore)
            first = middle + 1;
        else
            last = middle;
    }
    zones.insert(zones.begin() + first, 1, zone);
}

// Quest zone ranking. A zone's score is its connection distance from the
// origin times the scale plus a random tie-break below the scale; adjacent
// zones instead score from RMG_QUEST_ADJACENT_ZONE_SCORE, and zones above
// the maximum (including unreachable ones) are skipped.
enum ERmgQuestZoneScore {
    RMG_QUEST_UNREACHED_DISTANCE = 20000,
    RMG_QUEST_DISTANCE_SCALE = 10,
    RMG_QUEST_ADJACENT_ZONE_SCORE = 1000,
    RMG_QUEST_MAXIMUM_SCORE = 2000
};

// Stores each zone's connection-graph distance from origin in its quest
// placement score (RMG_QUEST_UNREACHED_DISTANCE when unreachable).
VA(0x0054b180, 0x174)
MAC_ADDRESS(0x25013c, 0x210)
void type_random_map_generator::calculateQuestZoneDistances(TRmgZone* origin)
{
    std::vector<TRmgZone*> pending;
    for (u32 index = 0; index < m_zones.size(); ++index)
        m_zones[index]->m_questPlacementScore = RMG_QUEST_UNREACHED_DISTANCE;
    origin->m_questPlacementScore = 0;
    pending.push_back(origin);
    while (pending.size()) {
        TRmgZone* current = pending.back();
        pending.pop_back();
        TRmgTemplateZone* templateZone = current->m_templateZone;
        s32 distance = current->m_questPlacementScore + 1;
        for (u32 connection = 0; connection < templateZone->m_connections.size(); ++connection) {
            TRmgZone* next = m_zones[templateZone->m_connections[connection].m_destination->m_zoneIndex];
            if (next->m_questPlacementScore > distance) {
                next->m_questPlacementScore = distance;
                insertRmgWorkItem(pending, next);
            }
        }
    }
}

// Tries the other reachable non-junction land zones by ascending score:
// nearest first, random order within a distance, and adjacent zones after
// those up to 99 connections away. Excluded zones still draw a random score.
VA(0x0054b300, 0x18b)
MAC_ADDRESS(0x25034c, 0x23c)
b8 type_random_map_generator::placeQuestGroup(
    TRmgTreasureGroup* group, TRmgZone* origin)
{
    std::vector<TRmgZone*> candidates;
    calculateQuestZoneDistances(origin);
    for (u32 index = 0; index < m_zones.size(); ++index) {
        TRmgZone* candidateZone = m_zones[index];
        s32 distance = candidateZone->m_questPlacementScore;
        if (distance == 1)
            candidateZone->m_questPlacementScore =
                RMG_QUEST_ADJACENT_ZONE_SCORE + rand() % RMG_QUEST_DISTANCE_SCALE;
        else
            candidateZone->m_questPlacementScore = distance * RMG_QUEST_DISTANCE_SCALE
                + rand() % RMG_QUEST_DISTANCE_SCALE;
    }
    for (index = 0; index < m_zones.size(); ++index) {
        TRmgZone* candidateZone = m_zones[index];
        if (candidateZone == origin || candidateZone->m_templateZone->m_kind == RMG_TEMPLATE_JUNCTION
            || candidateZone->m_questPlacementScore > RMG_QUEST_MAXIMUM_SCORE
            || candidateZone->m_terrain == eTerrainWater)
            continue;
        u32 insertion = 0;
        while (insertion < candidates.size()
            && candidateZone->m_questPlacementScore >= candidates[insertion]->m_questPlacementScore)
            ++insertion;
        candidates.insert(candidates.begin() + insertion, candidateZone);
    }
    for (index = 0; index < candidates.size(); ++index) {
        if (placeTreasureGroup(group, candidates[index], RMG_QUEST_GROUP_SPACING))
            return true;
    }
    return false;
}

// Below this many unused quest artifacts, no more seer huts are offered.
enum ERmgQuestArtifactPool {
    RMG_LOW_QUEST_ARTIFACT_COUNT = 20
};

// Treasure artifact class ('T') bit in the artifact table.
static const s32 g_rmgQuestArtifactClass = 2;

// Enabled treasure-class artifacts not yet used by a seer hut.
static inline bool isRmgQuestArtifact(s32 artifact, const b8* usedArtifacts)
{
    return !g_artifactTraits[artifact].m_disabled && !usedArtifacts[artifact]
        && (g_artifactTraits[artifact].m_artifactClass & g_rmgQuestArtifactClass);
}

VA(0x0054b490, 0x42e)
MAC_ADDRESS(0x250588, 0x360)
b8 type_random_map_generator::placeQuestArtifact(rmgQuestArtifactObject* object)
{
    rmgSeerHutObject* seerHut = object->m_seerHut;
    s32 available = 0;
    s32 artifact;
    for (artifact = ARTIFACT_SPELLBOOK; artifact < ARTIFACT_COUNT; ++artifact) {
        if (isRmgQuestArtifact(artifact, m_usedQuestArtifacts)) {
            ++available;
        }
    }
    if (available < RMG_LOW_QUEST_ARTIFACT_COUNT)
        m_questArtifactPoolLow = true;
    if (!available)
        return false;
    s32 selected = rand() % available;
    for (artifact = ARTIFACT_SPELLBOOK; artifact < ARTIFACT_COUNT; ++artifact) {
        if (isRmgQuestArtifact(artifact, m_usedQuestArtifacts) && selected-- <= 0)
            break;
    }
    seerHut->m_artifact = static_cast<TArtifact>(artifact);
    u32 prototypeIndex = findRmgPrototypeSubtypeIndex(
        m_objectPrototypes[ARTIFACT], artifact);
    // Assumes an eligible artifact always has a loaded prototype.
    TRmgObjectPropertiesRef* properties = m_objectPrototypes[ARTIFACT][prototypeIndex];
    --object->m_properties->m_refCount;
    object->m_properties = properties;
    ++properties->m_refCount;
    TRmgZone* origin = m_zones[m_map.getMapItem(object->m_position)->m_zoneState.m_zone];
    TRmgTreasureGroup group(RMG_TREASURE_GROUP_MAP_SIZE, RMG_TREASURE_GROUP_MAP_SIZE);
    group.addCenteredObject(seerHut);
    group.preparePlacement();
    if (!placeQuestGroup(&group, origin)) {
        replaceObjectWithTreasure(object, object->m_definition->getValue(origin, this));
        return false;
    }
    m_usedQuestArtifacts[artifact] = true;
    m_nextSeerHutPrototypeIndex = (m_nextSeerHutPrototypeIndex + 1)
        % m_objectPrototypes[SEER].size();
    return true;
}

// Reserve the colour before filling the group and release it on failure.
VA(0x0054b8c0, 0x385)
MAC_ADDRESS(0x2508e8, 0x334)
b8 type_random_map_generator::placeKeyTentGuard(type_object* object, s32 targetValue)
{
    s32 color = object->m_properties->m_prototype->getSubtype();
    u32 index = findRmgPrototypeSubtypeIndex(
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
    group.discard();
    setKeyTentColorDisabled(color, false);
    return false;
}

// Callers keep ownership of the object. Retail bug: both find loops compare
// the iterator with null rather than end().
VA(0x0054bc50, 0x2ae)
MAC_ADDRESS(0x250c1c, 0x2ac)
void type_random_map_generator::removeObject(type_object* object)
{
    TObjectType* prototype = object->m_properties->m_prototype;
    TRmgMapPosition position = object->m_position;
    std::vector<type_object*>::iterator found = std::find(m_objects.begin(), m_objects.end(), object);
#if defined(HOMM3_RMG_HOTFIX)
    // An object missing from a list is not erased from it.
    if (found != m_objects.end()) {
#else
    if (found) {
#endif
        m_objects.erase(found);
        TAdventureObjectType objectType = prototype->getObjectType();
        --m_objectCountByType[objectType];
        TRmgMapPosition entrance = object->getEntrance();
        s32 zone = m_map.getMapItem(entrance.m_x,
            entrance.m_y, entrance.m_z)->m_zoneState.m_zone;
        if (zone >= 0)
            --m_zones[zone]->m_objectCountByType[objectType];
    }
    if (prototype->getObjectType() == BORDER_GUARD) {
        setKeyTentColorDisabled(prototype->getSubtype(), false);
    }
    for (u32 row = 0; row < prototype->getHeight(); ++row) {
        s32 y = position.m_y - row;
        if (y < 0 || y >= m_map.m_mapHeight)
            continue;
        for (u32 column = 0; column < prototype->getWidth(); ++column) {
            s32 x = position.m_x - column;
            if (x < 0 || x >= m_map.m_mapWidth)
                continue;
            if (isRmgObjectFootprintCell(prototype, column, row)) {
                TRmgMapItem* item = m_map.getMapItem(x, y, position.m_z);
                std::vector<type_object*>::iterator entry = std::find(item->m_objects.begin(), item->m_objects.end(), object);
#if defined(HOMM3_RMG_HOTFIX)
                if (entry != item->m_objects.end()) {
#else
                if (entry) {
#endif
                    item->m_objects.erase(entry);
                    if (!item->hasObjects()) {
                        item->m_tileData.m_objectEntrance = false;
                        item->m_tileData.m_passable = true;
                    }
                    item->m_zoneState.m_objectDistance = RMG_CLEARED_CELL_COST;
                }
            }
        }
    }
}

VA(0x0054bf00, 0x57)
MAC_ADDRESS(0x250fe0, 0x90)
TRandomMapRequest::TRandomMapRequest(s32 width, s32 height, s32 levels)
    : m_width(width), m_height(height), m_levels(levels),
      m_humanPlayerCount(2), m_humanTeamCount(2),
      m_computerPlayerCount(0), m_computerTeamCount(8),
      m_waterContent(RMG_WATER_RANDOM), m_monsterStrength(0),
      m_mapVersion(RMG_MAP_SHADOW_OF_DEATH)
{
    memset(m_isHumanSeat, 0, sizeof(m_isHumanSeat));
    memset(m_townType, -1, sizeof(m_townType));
}

void type_random_map_generator::setHumanPlayer(s32 seat)
{
    m_fixedHumanPlayers[seat] = true;
}

void type_random_map_generator::setTownChoice(s32 seat, TTownType town)
{
    m_townChoices[seat] = town;
}

VA(0x0054bf60, 0x130)
MAC_ADDRESS(0x251070, 0x140)
s32 TRandomMapRequest::generateToFile(TAbstractFile* outputFile, TProgressSink* progress)
{
    s32 strength = m_monsterStrength + RMG_ZONE_MONSTERS_AVERAGE;
    if (strength < 1)
        strength = 1;
    if (strength > RMG_STRONGEST_GUARD_STRENGTH)
        strength = RMG_STRONGEST_GUARD_STRENGTH;
    if (m_humanPlayerCount + m_computerPlayerCount < 2) {
        m_humanPlayerCount = 1;
        m_computerPlayerCount = 1;
    }
    type_random_map_generator generator(m_width, m_height, m_levels,
        m_humanPlayerCount, m_humanTeamCount, m_computerPlayerCount,
        m_computerTeamCount, m_waterContent, strength,
        progress, m_mapVersion);
    for (s32 seat = 0; seat < RMG_PLAYER_COUNT; ++seat) {
        if (m_isHumanSeat[seat])
            generator.setHumanPlayer(seat);
        generator.setTownChoice(seat, static_cast<TTownType>(m_townType[seat]));
    }
    if (!generator.generate())
        return RANDOM_MAP_GENERATION_FAILED;
    if (!generator.writeMap(outputFile))
        return RANDOM_MAP_WRITE_FAILED;
    return RANDOM_MAP_OK;
}

VA(0x0054c090, 0x8c)
MAC_ADDRESS(0x2511b0, 0x98)
s32 TRandomMapRequest::generate(const char* fileName, TProgressSink* progress)
{
    try {
        TGzFile outputFile(fileName, "wb6");
        return generateToFile(&outputFile, progress);
    } catch (const TGzFile::TOpenFailure&) {
        return RANDOM_MAP_OPEN_FAILED;
    }
}

VA_COMPGEN(0x0054c6a0, 0x4d, QUEUE_LIST_DTOR, TPoint)

VA_COMPGEN(0x0054d000, 0x5e, LIST_INSERT_SINGLE, TPoint)

VA_COMPGEN(0x0054d060, 0x36, LIST_ERASE_ITERATOR, TPoint)

VA_COMPGEN(0x0054d0a0, 0x45, LIST_ERASE_RANGE, TPoint)

VA_COMPGEN(0x0054d0f0, 0x2d, LIST_BUYNODE, TPoint)

VA_COMPGEN(0x005166e0, 0x34, BITSET_TEST, Bitset10)

VA(0x005fdb10, 0x21)
MAC_ADDRESS(0x25c3b4, 0x38)
s32 getRmgSquaredDistance(TPoint first, TPoint second)
{
    return getRmgSquaredNorm(first.m_x - second.m_x, first.m_y - second.m_y);
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

VA(0x005fdb40, 0x16e)
MAC_ADDRESS(0x25d144, 0x12c)
void TRmgVoronoi::buildVertices()
{
    for (u32 index = 0; index < m_edges.size(); ++index) {
        TRmgHalfEdge* edge = m_edges[index];
        if (edge->getZone() && !edge->isVertexComputed()) {
            TPoint second = edge->getOppositeSitePosition();
            TPoint vertex = computeRmgCircumcenter(edge->getNext()->getOppositeSitePosition(),
                edge->getSitePosition(), second);
            edge->setVertex(vertex);
            edge = edge->getLeftPrevious();
            edge->setVertex(vertex);
            edge = edge->getLeftPrevious();
            edge->setVertex(vertex);
        }
    }
}

VA(0x005fdcb0, 0x1e)
TRmgVector TRmgVector::operator+(TRmgVector other) const
{
    return TRmgVector(m_x + other.m_x, m_y + other.m_y);
}

VA(0x005fdcd0, 0x1d)
TRmgVector TRmgVector::operator*(s32 scale) const
{
    TRmgVector result;
    result.m_x = m_x * scale;
    result.m_y = m_y * scale;
    return result;
}

VA(0x005fdcf0, 0x25)
TRmgVector TRmgVector::operator/(s32 divisor) const
{
    return TRmgVector(m_x / divisor, m_y / divisor);
}

VA(0x005fdd20, 0x20)
TPoint operator+(TPoint point, TRmgVector offset)
{
    return TPoint(point.m_x + offset.m_x, point.m_y + offset.m_y);
}

VA(0x005fdd40, 0x20)
TRmgVector operator-(TPoint left, TPoint right)
{
    return TRmgVector(left.m_x - right.m_x, left.m_y - right.m_y);
}
