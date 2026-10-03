// RMG constants shared by the generator's sources; kept out of rmg.h so
// game TUs that include rmg.h are unaffected.

#ifndef HOMM3_RMG_CONSTANTS_H
#define HOMM3_RMG_CONSTANTS_H

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

// Candidate town types one zone terrain can list.
enum ERmgTerrainTownChoiceLimits {
    RMG_TERRAIN_TOWN_CHOICE_COUNT = 4
};

enum ERmgRadialDirectionLimits {
    RMG_RADIAL_DIRECTION_COUNT = 32
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

// First plain (shape 0) water frame.
enum ERmgWaterFrames {
    RMG_WATER_BASE_FRAME = 21
};

// hasConnectedOutline's requirePathClearance argument.
enum ERmgOutlinePathClearance {
    RMG_IGNORE_PATH_CLEARANCE = false,
    RMG_REQUIRE_PATH_CLEARANCE = true
};

// Radius of a square neighbourhood around a cell, clipped to the map.
enum ERmgNeighborhoodRadius {
    RMG_NEIGHBORHOOD_3X3 = 1,
    RMG_NEIGHBORHOOD_5X5 = 2
};

// isPlacementBlocked's rejectObstacleFill argument.
enum ERmgObstacleEntrancePolicy {
    RMG_ALLOW_OBSTACLE_ENTRANCES = false,
    RMG_REJECT_OBSTACLE_ENTRANCES = true
};

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

// Witch hut skill mask: the first 16 secondary skills except Navigation and
// Necromancy.
enum ERmgWitchHutSkills {
    RMG_WITCH_HUT_ALLOWED_SKILLS = 0xffff
        & ~((1 << eSecSkillNavigation) | (1 << eSecSkillNecromancy))
};

// getValue result for a treasure this zone or moment cannot offer.
enum ERmgTreasureOffer {
    RMG_TREASURE_NOT_OFFERED = -1
};

// Spell-trait flag of spells the map loader disables on every map (see
// game.cpp); never a generated reward.
enum ERmgSpellTraitFlags {
    RMG_SPELL_DISABLED_BY_DEFAULT = 0x2000
};

// Progress steps reported by loadObjectPrototypes, which the base
// constructor runs and budgets.
enum ERmgPrototypeLoadProgress {
    RMG_PROTOTYPE_LOAD_PROGRESS = 15300
};

// Monolith subtypes, per kind, that maps before Shadow of Death may use.
enum ERmgMonolithSubtypeLimits {
    RMG_PRE_SOD_MONOLITH_SUBTYPE_COUNT = 3
};

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

// Creature dwelling subtypes offered as treasures (g_creatureGenerator1Types).
enum ERmgDwellingSubtypeCounts {
    RMG_DWELLING_SUBTYPE_COUNT = 80,
    RMG_ROE_DWELLING_SUBTYPE_COUNT = 58
};

// lengthDivisor values of splitRmgBoundarySegment.
enum ERmgBoundaryDisplacement {
    RMG_FULL_LENGTH_DISPLACEMENT = 1,
    RMG_HALF_LENGTH_DISPLACEMENT = 2
};

// Indices of TRmgNoiseRegion::m_corners, named by their bounds corner.
enum ERmgNoiseCorner {
    RMG_NOISE_MIN_X_MIN_Y = 0,
    RMG_NOISE_MIN_X_MAX_Y = 1,
    RMG_NOISE_MAX_X_MIN_Y = 2,
    RMG_NOISE_MAX_X_MAX_Y = 3
};

// Island radius in tiles, and the clearance (in m_zonePathCost units, 2 per
// cardinal step) an island centre keeps from the zone edge and earlier
// island centres.
enum ERmgWaterZoneIslandLimits {
    RMG_ISLAND_MINIMUM_RADIUS = 3,
    RMG_ISLAND_MAXIMUM_RADIUS = 6,
    RMG_ISLAND_CLEARANCE = 20
};

// openConnectionPath's narrow argument; wide paths also clear nearby
// same-zone obstacle marks.
enum ERmgConnectionPathWidth {
    RMG_WIDE_CONNECTION_PATH = false,
    RMG_NARROW_CONNECTION_PATH = true
};

enum ERmgGuardPrototype {
    RMG_NO_GUARD_PROTOTYPE = -1
};

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

enum ERmgGroundCrossingLimits {
    RMG_BORDER_CELLS_PER_CROSSING = 40,
    RMG_MAXIMUM_CROSSING_COST = 100
};

// Squared entrance-distance areas per unit of density.
enum ERmgDensityArea {
    // 144x144 tiles, the extra-large map size: towns and extra mines.
    RMG_TOWN_AND_MINE_DENSITY_AREA = 4 * 144 * 144,
    // 200 and 400 tiles: treasure groups on land and in water zones.
    RMG_LAND_TREASURE_DENSITY_AREA = 4 * 200,
    RMG_WATER_TREASURE_DENSITY_AREA = 4 * 400
};

enum ERmgWeightedCategory {
    RMG_NO_CATEGORY = -1
};

// Spacing arguments: the minimum m_objectDistance at a placement. Fixed
// towns and mines ignore spacing; quest groups only avoid entrance cells,
// whose distance is zero.
enum ERmgObjectSpacing {
    RMG_NO_SPACING = 0,
    RMG_QUEST_GROUP_SPACING = 1
};

// Squared tile distances of a starting mine from the town: never within 4
// tiles or beyond 200, and any site within 12 tiles ranks as 12.
enum ERmgStartingMineDistance {
    RMG_STARTING_MINE_MINIMUM_SQUARED_DISTANCE = 4 * 4,
    RMG_STARTING_MINE_NEAR_SQUARED_DISTANCE = 12 * 12,
    RMG_STARTING_MINE_MAXIMUM_SQUARED_DISTANCE = 200 * 200
};

// Weaker guards are not placed.
enum ERmgGuardValueLimits {
    RMG_MINIMUM_GUARD_VALUE = 2000
};

// Treasure groups are assembled on a square scratch map before placement.
enum ERmgTreasureGroupMapSize {
    RMG_TREASURE_GROUP_MAP_SIZE = 16
};

// Road search step costs; a diagonal step costs three times a cardinal one.
enum ERmgRoadStepCost {
    RMG_ROAD_MONOLITH_COST = 50,
    RMG_ROAD_GATE_COST = 1,
    RMG_ROAD_ALONG_ROAD_COST = 2,
    RMG_ROAD_OFF_ROAD_COST = 20,
    RMG_ROAD_DIAGONAL_FACTOR = 3
};

// Road-layer types, dirt through cobblestone; zero is no road.
enum ERmgRoadType {
    RMG_ROAD_DIRT = 1,
    RMG_ROAD_GRAVEL = 2,
    RMG_ROAD_COBBLESTONE = 3,
    RMG_ROAD_TYPE_COUNT = RMG_ROAD_COBBLESTONE
};

// River-layer line types the generator paints; zero is no river.
enum ERmgRiverType {
    RMG_RIVER_CLEAR = 1,
    RMG_RIVER_ICY = 2
};

// Artifact ids an AB map knows: it ends before the SoD combination artifacts.
enum ERmgArtifactCount {
    RMG_AB_ARTIFACT_COUNT = ARTIFACT_ANGELIC_ALLIANCE
};

enum TRmgPrototypeCellMask {
    RMG_PROTOTYPE_PASSABLE_CELLS,
    RMG_PROTOTYPE_TRIGGER_CELLS
};

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

// Below this many unused quest artifacts, no more seer huts are offered.
enum ERmgQuestArtifactPool {
    RMG_LOW_QUEST_ARTIFACT_COUNT = 20
};

#endif  // HOMM3_RMG_CONSTANTS_H
