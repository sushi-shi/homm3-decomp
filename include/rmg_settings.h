// Scalar generator settings shared by the C++ and Rust implementations.
#ifndef HOMM3_RMG_SETTINGS_H
#define HOMM3_RMG_SETTINGS_H

#include "terrain_type.h"

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

#endif
