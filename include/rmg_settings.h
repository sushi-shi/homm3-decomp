// Scalar generator settings shared by the C++ and Rust implementations.
#ifndef HOMM3_RMG_SETTINGS_H
#define HOMM3_RMG_SETTINGS_H

#include "terrain_type.h"

// The eight neighbour directions, clockwise.
enum ERmgDirectionLimits {
    RMG_DIRECTION_COUNT = 8
};

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

// Width of the native border-connection colour bitfield.
enum ERmgBorderConnectionBits {
    RMG_BORDER_COLOR_BITS = 4
};

// Eight-neighbour chamfer metric used by generator distance floods.
enum ERmgChamferStepCost {
    RMG_CHAMFER_CARDINAL_COST = 2,
    RMG_CHAMFER_DIAGONAL_COST = 3
};

// Movement flood costs inside a zone and onto water or across its border.
enum ERmgConnectionStepCost {
    RMG_CONNECTION_LAND_STEP_COST = 1,
    RMG_CONNECTION_WATER_OR_BORDER_STEP_COST = 10
};

// Radius of a square neighbourhood around a cell, clipped to the map.
enum ERmgNeighborhoodRadius {
    RMG_NEIGHBORHOOD_3X3 = 1,
    RMG_NEIGHBORHOOD_5X5 = 2
};

// Town count/density groups by ownership and starting fort.
enum ERmgTownPlacementCategory {
    RMG_TOWN_PLAYER_CASTLE,
    RMG_TOWN_PLAYER_BASIC,
    RMG_TOWN_NEUTRAL_CASTLE,
    RMG_TOWN_NEUTRAL_BASIC,
    RMG_TOWN_CATEGORY_COUNT
};

// Initial segment direction of a carved branching path. Each spans the map;
// North is up and # is the segment.
//   MAIN_DIAGONAL  VERTICAL  ANTI_DIAGONAL  HORIZONTAL
//   # . .          . # .     . . #          . . .
//   . # .          . # .     . # .          # # #
//   . . #          . # .     # . .          . . .
enum ERmgBranchSeedPattern {
    RMG_BRANCH_SEED_MAIN_DIAGONAL = 0,
    RMG_BRANCH_SEED_VERTICAL = 1,
    RMG_BRANCH_SEED_ANTI_DIAGONAL = 2,
    RMG_BRANCH_SEED_HORIZONTAL = 3,
    RMG_BRANCH_SEED_PATTERN_COUNT = 4
};

enum ERmgBranchLimits {
    RMG_BRANCH_MINIMUM_SPLIT_LENGTH = 8,
    RMG_BRANCH_MINIMUM_SQUARED_DISTANCE = 25,
    RMG_BRANCH_UNCHECKED_STEPS = 2
};

// Island radius in tiles, and the clearance (in m_zonePathCost units, 2 per
// cardinal step) an island centre keeps from the zone edge and earlier
// island centres.
enum ERmgWaterZoneIslandLimits {
    RMG_ISLAND_MINIMUM_RADIUS = 3,
    RMG_ISLAND_MAXIMUM_RADIUS = 6,
    RMG_ISLAND_CLEARANCE = 20
};

enum ERmgObjectCounter {
    RMG_FIRST_OBJECT_ID = 1
};

enum ERmgTownPlacementLimit {
    RMG_PRIMARY_TOWN_MAXIMUM_SQUARED_DISTANCE = 32000
};

// Squared entrance-distance areas per unit of density.
enum ERmgDensityArea {
    // 144x144 tiles, the extra-large map size: towns and extra mines.
    RMG_TOWN_AND_MINE_DENSITY_AREA = 4 * 144 * 144,
    // 200 and 400 tiles: treasure groups on land and in water zones.
    RMG_LAND_TREASURE_DENSITY_AREA = 4 * 200,
    RMG_WATER_TREASURE_DENSITY_AREA = 4 * 400
};

// Terrain types, dirt through rock.
enum ERmgTerrainLimits {
    RMG_TERRAIN_COUNT = eTerrainRock + 1
};

enum ERmgMapLevel {
    RMG_SURFACE_LEVEL = 0,
    RMG_UNDERGROUND_LEVEL = 1,
    RMG_MAP_LEVEL_COUNT = 2
};

// Zone monster strength, from the template letter n, w, a or s.
enum ERmgZoneMonsterStrength {
    RMG_ZONE_MONSTERS_NONE = 0,
    RMG_ZONE_MONSTERS_WEAK = 2,
    RMG_ZONE_MONSTERS_AVERAGE = 3,
    RMG_ZONE_MONSTERS_STRONG = 4
};

// Guard strengths: the zone scale (ERmgZoneMonsterStrength) shifted by the
// map strength, from 0 up to the strongest.
enum ERmgGuardStrengthLimits {
    RMG_STRONGEST_GUARD_STRENGTH = 5,
    RMG_GUARD_STRENGTH_COUNT = RMG_STRONGEST_GUARD_STRENGTH + 1
};

enum ERmgTreasurePlacementLimits {
    RMG_TREASURE_ATTEMPTS = 3,
    RMG_TREASURE_MINIMUM_REMAINDER = 1500,
    RMG_TREASURE_MINIMUM_VALUE = 100
};

// Treasure bands per template zone in rmg.txt (low, medium and high value).
enum ERmgTreasureBandLimits {
    RMG_TREASURE_BAND_COUNT = 3
};

enum ERmgTerrainTownChoiceLimits {
    RMG_TERRAIN_TOWN_CHOICE_COUNT = 4
};

enum ERmgRadialDirectionLimits {
    RMG_RADIAL_DIRECTION_COUNT = 32
};

// Path costs and zone graph distances start at RMG_UNREACHED_COST. Searches
// treat costs above RMG_REACHED_COST_LIMIT as unreached; some also reject the
// limit itself. Clearing a cell sets its costs and object distance to
// RMG_CLEARED_CELL_COST; removing an object resets the object distance under
// its footprint to it.
enum ERmgPathCostLimits {
    RMG_REACHED_COST_LIMIT = 30000,
    RMG_UNREACHED_COST = 32000,
    RMG_CLEARED_CELL_COST = 32700
};

enum ERmgObjectPlacementScore {
    RMG_PLACEMENT_INVALID = -5000,
    RMG_PLACEMENT_MINIMUM_TERRAIN_SCORE = -1000,
    RMG_PLACEMENT_NO_TERRAIN_PREFERENCE = -1
};

// Monolith subtypes, per kind, that maps before Shadow of Death may use.
enum ERmgMonolithSubtypeLimits {
    RMG_PRE_SOD_MONOLITH_SUBTYPE_COUNT = 3
};

// Creature-type counts and guard limits. RoE maps lack the expansion creature
// types; their guards exclude creatures from 118 but evaluate only those below
// 117, so 117 slips through (retail bug).
enum ERmgGuardConstants {
    RMG_CREATURE_TYPE_COUNT = 145,
    RMG_ROE_CREATURE_TYPE_COUNT = 118,
    RMG_GUARD_MAXIMUM_COUNT = 100,
    RMG_GUARD_DISPOSITION = 3
};

// placeBorderGuard's result when no keymaster's tent could be placed;
// otherwise it returns the guard colour.
enum ERmgBorderGuardPlacement {
    RMG_BORDER_GUARD_NOT_PLACED = -1
};

// Border guards placed side by side, eastward from the given cell.
enum ERmgBorderGuardCount {
    RMG_SINGLE_BORDER_GUARD = 1,
    // One per cell of the row below a three-tile shipyard.
    RMG_SHIPYARD_BORDER_GUARDS = 3
};

// Spell-trait flag of spells the map loader disables on every map (see
// game.cpp); never a generated reward.
enum ERmgSpellTraitFlags {
    RMG_SPELL_DISABLED_BY_DEFAULT = 0x2000
};

// Hero ids a map format knows: RoE maps stop before the expansion heroes.
// Prisons in later formats hold only the first RMG_PRISON_HERO_COUNT.
enum ERmgHeroCount {
    RMG_ROE_HERO_COUNT = 128,
    RMG_PRISON_HERO_COUNT = 145,
    RMG_HERO_COUNT = 156
};

#endif
