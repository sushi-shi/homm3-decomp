//! Literals owned by the translated algorithms, not native data declarations.
//!
//! Keep named native enums and extracted constructor defaults in `raw`; these
//! constants name otherwise literal policies in the corresponding C++ bodies.

// TRmgGenerator::buildZoneBoundaries water-zone treasure assignments.
pub(crate) const RMG_WATER_TREASURE_0_DENSITY: u32 = 5;
pub(crate) const RMG_WATER_TREASURE_0_MINIMUM: u32 = 100;
pub(crate) const RMG_WATER_TREASURE_0_MAXIMUM: u32 = 1000;
pub(crate) const RMG_WATER_TREASURE_1_DENSITY: u32 = 1;
pub(crate) const RMG_WATER_TREASURE_1_MINIMUM: u32 = 2000;
pub(crate) const RMG_WATER_TREASURE_1_MAXIMUM: u32 = 6000;

// Native object/water distance floods and TRmgMap::floodConnectionCosts.
pub(crate) const RMG_CHAMFER_CARDINAL_COST: u32 = 2;
pub(crate) const RMG_CHAMFER_DIAGONAL_COST: u32 = 3;
pub(crate) const RMG_CONNECTION_LAND_STEP_COST: u32 = 1;
pub(crate) const RMG_CONNECTION_WATER_OR_BORDER_STEP_COST: u32 = 10;

// TRmgGenerator::carveBranchingPaths and traceBranchEnd split/trace policy.
pub(crate) const RMG_BRANCH_MINIMUM_SPLIT_LENGTH: u32 = 8;
pub(crate) const RMG_BRANCH_MINIMUM_SQUARED_DISTANCE: u32 = 25;
pub(crate) const RMG_BRANCH_UNCHECKED_STEPS: u32 = 2;

// TRmgGenerator constructor starts object IDs at one; tryPlacePrimaryTown uses this distance ceiling.
pub(crate) const RMG_FIRST_OBJECT_ID: u32 = 1;
pub(crate) const RMG_PRIMARY_TOWN_MAXIMUM_SQUARED_DISTANCE: u32 = 32000;

// TRmgGenerator::placeMineSite / tryPlaceMine / placeMines placement and guard policy.
pub(crate) const RMG_MINE_MAXIMUM_OBSTACLE_SCORE: u32 = 5;
pub(crate) const RMG_MINE_RESOURCE_PILE_LIMIT: u32 = 3;
pub(crate) const RMG_BASIC_MINE_GUARD_VALUE: u32 = 1500;
pub(crate) const RMG_RARE_MINE_GUARD_VALUE: u32 = 3500;
pub(crate) const RMG_GOLD_MINE_GUARD_VALUE: u32 = 7000;

// River search costs.
pub(crate) const RMG_RIVER_STEP_MASK: u32 = 31;
pub(crate) const RMG_RIVER_MINIMUM_STEP_COST: u32 = 1;
pub(crate) const RMG_RIVER_ROAD_PENALTY: u32 = 30;

// Artifact resource reader: R/J/N/T and the default class in src/artifact.cpp.
pub(crate) const ARTIFACT_CLASS_SPECIAL: u32 = 1;
pub(crate) const ARTIFACT_CLASS_TREASURE: u32 = 2;
pub(crate) const ARTIFACT_CLASS_MINOR: u32 = 4;
pub(crate) const ARTIFACT_CLASS_MAJOR: u32 = 8;
pub(crate) const ARTIFACT_CLASS_RELIC: u32 = 16;
