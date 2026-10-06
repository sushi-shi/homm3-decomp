//! Object-fit queries over borrowed terrain and reusable placement state.

use crate::{
    boundaries::{BoundaryZone, TerrainCoverage},
    domain::{CellLayout, Terrain, WorldPosition},
    geometry::{Point, ZoneId},
    object::ObjectKind,
    prototype::{OutlineError, OutlineWorkspace, PreparedPrototype, PrototypeFault, PrototypeId},
    raw,
    terrain::PaintedTerrain,
};
use std::{collections::TryReserveError, error::Error, fmt};

mod border_guards;
mod guard_value;
mod guards;
pub use guard_value::GuardStrength;
mod gates;
mod junctions;
mod mines;
mod treasure_assembly;
mod treasure_commit;
mod treasure_completion;
mod treasure_generation;
mod treasure_group_guards;
mod treasure_group_sites;
mod treasure_quest_zones;
mod treasure_replacement;
mod treasure_zones;
pub use treasure_assembly::TreasurePacking;
use treasure_zones::TreasuresPlaced;
mod treasure_group_lifetime;
mod treasure_groups;
pub use treasure_group_lifetime::RejectedTreasure;
pub use treasure_groups::TreasureGroupWorkspace;
mod treasure_paths;
mod treasure_payloads;
mod treasures;
pub use junctions::JunctionsPrepared;
pub use mines::{MineError, MinesPlaced};
pub use treasure_generation::{
    PendingTreasure, SelectedTreasure, TreasureGeneration, TreasureGenerationError,
};
pub use treasure_paths::{TownZoneCounts, TreasurePaths};
pub use treasure_payloads::{
    PandoraReward, PrisonPayload, QuestArtifactPayload, SeerPayload, SeerReward,
};
pub use treasures::{TreasureValueError, TreasuresReady};
mod portals;
pub use portals::{ConnectionsPlaced, PortalDirection};
mod ground_connections;
mod shipyards;
pub use border_guards::{BorderGuardCount, BorderGuardPlacement};
mod zone_connections;
pub use zone_connections::{ConnectingZones, ConnectionId, DirectConnections};
mod zone_objects;
use zone_objects::ZonePlacementScratch;
mod objects;
pub use guards::GuardPlacementError;
pub use objects::{
    MapObjectId, MonsterPayload, ObjectArena, ObjectGeometry, ObjectId, ObjectPayload, ObjectRef,
    PositionedObject,
};
mod mutation;
pub use mutation::BorderColor;
mod neighborhood;
pub use neighborhood::Neighborhood;
mod connections;
mod water_islands;
pub use water_islands::WaterIslands;
mod connection_paths;
mod movement;
pub use connection_paths::{ConnectionPaths, PathWidth, RepairedWaterBorders};
pub use movement::{Direction, Movement, ZoneDistance};
mod decoration;
use decoration::CoastsMarked;
mod lines;
pub use lines::{LineTile, RiverType, RoadType};
mod obstacles;
mod rivers;
mod roads;
pub use obstacles::ObstacleWorkspace;
use obstacles::ObstaclesPlaced;
pub(crate) use rivers::RiversCreated;
use roads::RoadsCreated;
mod density;
mod island_noise;
use connections::ConnectionScratch;
pub use connections::{ConnectionBorders, ConnectionError};
mod towns;
use objects::{Chain, Memberships};
use towns::TownState;
pub use towns::{Fort, TownError, TownPayload, TownsPlaced};
mod registration;
use registration::Registration;
pub use registration::{KeyTentColor, KeyTentCursor};

#[expect(
    clippy::cast_possible_truncation,
    reason = "canonical cost is checked at compile time"
)]
const CLEARED_DISTANCE: u16 = {
    assert!(raw::RMG_CLEARED_CELL_COST <= u16::MAX as u32);
    raw::RMG_CLEARED_CELL_COST as u16
};

/// Mutually exclusive path and obstacle reservations.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum PathReservation {
    /// Reserved for open traversal; obstacles must leave this cell clear.
    #[default]
    Open,
    /// Obstacles may be placed, but none is requested yet.
    Unreserved,
    /// This cell is marked for obstacle filling.
    Obstacle,
}

/// Nonempty ring of offsets, preventing native modulo-by-zero on an empty outline.
#[derive(Clone, Copy, Debug)]
pub struct NonEmptyOutline<'a>(&'a [Point]);
impl<'a> NonEmptyOutline<'a> {
    /// Admit any nonempty sequence in its original traversal order.
    #[must_use]
    pub fn parse(points: &'a [Point]) -> Option<Self> {
        (!points.is_empty()).then_some(Self(points))
    }
    /// Original offsets; the connected-outline check revisits the first one.
    #[must_use]
    pub const fn points(self) -> &'a [Point] {
        self.0
    }
}

/// Policy for entrances encountered around an object's outline.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum OutlineEntrances {
    /// Reject any entrance before evaluating the blocked run.
    Reject,
    /// Treat entrances as blocked outline cells without immediately rejecting.
    Allow,
}
/// Whether an outline also needs generated-path clearance.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum OutlineClearance {
    /// Only placement blocking and water class matter.
    Ignore,
    /// Each open cell must also have path clearance.
    Require,
}
/// Whether trigger cells can occupy obstacle-fill marks.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ObstacleEntrances {
    /// Allow a trigger on an obstacle-fill mark.
    Allow,
    /// Reject a trigger on an obstacle-fill mark.
    Reject,
}

/// A safe placement query cannot reproduce an undefined native access.
#[derive(Debug)]
pub enum PlacementError {
    /// Retail underflows the bound of its empty road-target loop.
    EmptyRoadTargets,
    /// A native route follows a predecessor that has never been written.
    MissingPredecessor(WorldPosition),
    /// A prototype's placement rule belongs to a different rule table.
    RuleContext,
    /// Native overlap scoring reads a cell with no initialized priority.
    UnwrittenOverlap(PrototypeId),
    /// A typed payload factory received a prototype of another object kind.
    PayloadKind {
        /// Kind required by the payload factory.
        expected: ObjectKind,
        /// Kind of the supplied prototype.
        actual: ObjectKind,
    },
    /// Supplied arena is not the one already bound to cell membership.
    ArenaContext,
    /// Objects are being registered with a different catalog, mode or format.
    CatalogContext,
    /// This payload requires its generation-owned reservation/child cleanup.
    TreasureCleanupRequired(ObjectId),
    /// Retail erases a missing global-list entry after that list allocated storage.
    NotRegistered(ObjectId),
    /// A signed per-type counter or path cost cannot be represented.
    Arithmetic,
    /// Border guard subtype does not index the native tent availability vector.
    KeyTentSubtype(i32),
    /// Retail reads its unwritten initial key-tent cursor without replay input.
    KeyTentReplayRequired,
    /// Process-local object ownership tags have been exhausted.
    IdentityExhausted,
    /// Prototype ID does not belong to the supplied catalog.
    UnknownPrototype(PrototypeId),
    /// Object ID does not belong to the supplied arena or its current generation.
    UnknownObject(ObjectId),
    /// A live object has not yet been assigned a position.
    UnpositionedObject(ObjectId),
    /// Zone index is outside this map.
    UnknownZone(ZoneId),
    /// A temporary group exclusively owns this record.
    GroupOwned(ObjectId),
    /// Discarding an object that may still be shared by world or temporary maps.
    PreviouslyPlaced(ObjectId),
    /// Retail would erase an end iterator from a cell with allocated vector storage.
    MissingMembership(ObjectId),
    /// Invalid prototype dimensions.
    Prototype(PrototypeFault),
    /// Outline traversal cannot complete safely.
    Outline(OutlineError),
    /// Native connected-outline testing would divide by zero.
    EmptyOutline,
    /// Native code would dereference a missing map cell or plane.
    OutsideMap(WorldPosition),
    /// Signed coordinate arithmetic overflowed.
    CoordinateOverflow,
    /// Placement state could not grow.
    Allocation(TryReserveError),
}
impl fmt::Display for PlacementError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::EmptyRoadTargets => {
                f.write_str("retail road generation indexes an empty target list")
            }
            Self::MissingPredecessor(position) => {
                write!(f, "route has no predecessor at {position:?}")
            }
            Self::RuleContext => f.write_str("prototype placement rule belongs to another table"),
            Self::UnwrittenOverlap(id) => write!(
                f,
                "prototype {} has no overlap priority at the accessed cell",
                id.index()
            ),
            Self::PayloadKind { expected, actual } => write!(
                f,
                "payload kind {} cannot use object kind {}",
                expected.index(),
                actual.index()
            ),
            Self::ArenaContext => f.write_str("object arena does not own map membership"),
            Self::TreasureCleanupRequired(id) => {
                write!(f, "object {} requires treasure cleanup", id.index())
            }
            Self::CatalogContext => {
                f.write_str("object catalog does not match the registered map context")
            }
            Self::NotRegistered(id) => {
                write!(f, "retail removes absent registered object {}", id.index())
            }
            Self::Arithmetic => f.write_str("placement arithmetic overflow"),
            Self::KeyTentSubtype(value) => write!(
                f,
                "key-tent subtype {value} does not index availability storage"
            ),
            Self::KeyTentReplayRequired => {
                f.write_str("initial retail key-tent cursor requires replay input")
            }
            Self::IdentityExhausted => f.write_str("object ownership identities exhausted"),
            Self::UnknownPrototype(id) => {
                write!(f, "prototype {} belongs to another catalog", id.index())
            }
            Self::UnknownObject(id) => write!(
                f,
                "object {} belongs to another arena generation",
                id.index()
            ),
            Self::UnpositionedObject(id) => {
                write!(f, "object {} has no assigned position", id.index())
            }
            Self::UnknownZone(id) => write!(f, "zone {} does not belong to this map", id.index()),
            Self::GroupOwned(id) => write!(f, "object {} is owned by a treasure group", id.index()),
            Self::PreviouslyPlaced(id) => {
                write!(f, "object {} has already been placed", id.index())
            }
            Self::MissingMembership(id) => write!(
                f,
                "retail removes absent object {} from allocated cell storage",
                id.index()
            ),
            Self::Prototype(error) => error.fmt(f),
            Self::Outline(error) => error.fmt(f),
            Self::EmptyOutline => f.write_str("connected placement outline is empty"),
            Self::OutsideMap(position) => write!(f, "placement accesses missing cell {position:?}"),
            Self::CoordinateOverflow => f.write_str("placement coordinate overflow"),
            Self::Allocation(error) => error.fmt(f),
        }
    }
}
impl Error for PlacementError {}
impl From<PrototypeFault> for PlacementError {
    fn from(error: PrototypeFault) -> Self {
        Self::Prototype(error)
    }
}
impl From<OutlineError> for PlacementError {
    fn from(error: OutlineError) -> Self {
        Self::Outline(error)
    }
}
impl From<TryReserveError> for PlacementError {
    fn from(error: TryReserveError) -> Self {
        Self::Allocation(error)
    }
}

/// Read-only placement flags and membership metadata for one map cell.
#[derive(Clone, Copy, Debug)]
#[expect(
    clippy::struct_excessive_bools,
    reason = "independent tile facts may coexist; exclusive reservation and line states already use enums"
)]
pub struct CellState {
    objects: Chain,
    retail_membership_storage: bool,
    border: Option<BorderColor>,
    object_distance: u16,
    passable: bool,
    entrance: Option<ObjectKind>,
    reservation: PathReservation,
    zone_distance: ZoneDistance,
    movement: Movement,
    connection_visited: bool,
    coastal: bool,
    road: LineTile<RoadType>,
    river: LineTile<RiverType>,
    river_join_target: bool,
    near_river: bool,
    river_outlet_target: bool,
    blocked_river_directions: u8,
}
impl Default for CellState {
    fn default() -> Self {
        Self {
            objects: Chain::default(),
            retail_membership_storage: false,
            border: None,
            object_distance: CLEARED_DISTANCE,
            passable: true,
            entrance: None,
            reservation: PathReservation::Open,
            zone_distance: ZoneDistance::default(),
            movement: Movement::default(),
            connection_visited: false,
            coastal: false,
            road: LineTile::default(),
            river: LineTile::default(),
            river_join_target: false,
            near_river: false,
            river_outlet_target: false,
            blocked_river_directions: 0,
        }
    }
}

/// Reusable flat placement state; no per-cell collections or terrain copies.
#[derive(Default, Debug)]
pub struct PlacementWorkspace {
    cells: Vec<CellState>,
    memberships: Memberships,
    registration: Registration,
    towns: TownState,
    road_targets: Vec<WorldPosition>,
    connections: ConnectionScratch,
    zone_placement: ZonePlacementScratch,
}
impl PlacementWorkspace {
    /// Consume painted terrain into the placement stage and reset cell state.
    ///
    /// # Errors
    /// Returns a failed state-buffer reservation.
    pub fn begin<'map>(
        &'map mut self,
        terrain: PaintedTerrain<'map>,
    ) -> Result<PlacementMap<'map>, PlacementError> {
        let count = terrain.tiles().len();
        self.cells
            .try_reserve(count.saturating_sub(self.cells.len()))?;
        self.cells.resize(count, CellState::default());
        self.cells.fill(CellState::default());
        self.memberships.reset();
        self.towns.reset();
        self.road_targets.clear();
        self.connections.reset();
        self.zone_placement.reset();
        self.registration
            .reset(terrain.coverage().map().zones().len())?;
        Ok(PlacementMap {
            terrain,
            cells: &mut self.cells,
            memberships: &mut self.memberships,
            registration: &mut self.registration,
            towns: &mut self.towns,
            road_targets: &mut self.road_targets,
            connections: &mut self.connections,
            zone_placement: &mut self.zone_placement,
        })
    }
}

/// Placement stage owns its terrain-stage token while borrowing all map buffers.
pub struct PlacementMap<'map> {
    terrain: PaintedTerrain<'map>,
    cells: &'map mut [CellState],
    memberships: &'map mut Memberships,
    registration: &'map mut Registration,
    towns: &'map mut TownState,
    road_targets: &'map mut Vec<WorldPosition>,
    connections: &'map mut ConnectionScratch,
    zone_placement: &'map mut ZonePlacementScratch,
}
impl PlacementMap<'_> {
    /// Town and shipyard road targets in native insertion order, including repeats.
    #[must_use]
    pub fn road_targets(&self) -> &[WorldPosition] {
        self.road_targets
    }
    pub(super) fn append_road_target(
        &mut self,
        position: WorldPosition,
    ) -> Result<(), PlacementError> {
        self.road_targets.try_reserve(1)?;
        self.road_targets.push(position);
        Ok(())
    }

    /// Flat cell state in the same plane/row/column order as terrain tiles.
    #[must_use]
    pub fn cells(&self) -> &[CellState] {
        self.cells
    }

    /// Existing painted tiles and zone coverage, without copying either buffer.
    #[must_use]
    pub const fn terrain(&self) -> &PaintedTerrain<'_> {
        &self.terrain
    }

    /// Admit an in-bounds position and borrow its cell, terrain and zone together.
    /// The borrow prevents map mutation while the resolved cell is in use.
    ///
    /// # Errors
    /// Rejects coordinates outside the map or on an absent level.
    pub fn cell(&self, position: WorldPosition) -> Result<MapCell<'_>, PlacementError> {
        self.view().cell(position)
    }

    fn view(&self) -> PlacementView<'_> {
        PlacementView {
            layout: self.terrain.coverage().map().raster().layout(),
            surface: PlacementSurface::World {
                terrain: self.terrain.tiles(),
                zones: self.terrain.coverage().map().raster().cells(),
            },
            cells: self.cells,
        }
    }

    /// Check whole-footprint bounds, blocking, zone ownership and water policy.
    /// This is independent of the later outline and entrance-approach tests.
    ///
    /// # Errors
    /// Returns invalid dimensions or an accessed plane absent from the map.
    pub fn footprint_blocked(
        &self,
        entry: &PreparedPrototype<'_>,
        anchor: WorldPosition,
        zone: Option<ZoneId>,
        obstacles: ObstacleEntrances,
    ) -> Result<bool, PlacementError> {
        self.view()
            .footprint_blocked(entry, anchor, zone, obstacles)
    }

    /// Accept at most one blocked run and at least one open outline cell.
    ///
    /// # Errors
    /// Returns an absent plane or overflow while adding a signed outline offset.
    pub fn has_connected_outline(
        &self,
        outline: NonEmptyOutline<'_>,
        anchor: WorldPosition,
        entrances: OutlineEntrances,
        zone: ZoneId,
        clearance: OutlineClearance,
    ) -> Result<bool, PlacementError> {
        let zone = self.zone(zone)?;
        self.view().has_connected_outline(
            outline,
            anchor,
            entrances,
            zone.id(),
            zone.terrain(),
            clearance,
        )
    }

    /// Run the native footprint, outline and southern entrance-approach checks.
    ///
    /// # Errors
    /// Reports malformed geometry or an unsafe native coordinate/outline access.
    pub fn can_place(
        &self,
        entry: &PreparedPrototype<'_>,
        anchor: WorldPosition,
        zone: ZoneId,
        outline: &mut OutlineWorkspace,
    ) -> Result<bool, PlacementError> {
        let zone = self.zone(zone)?;
        self.view()
            .can_place(entry, anchor, zone.id(), zone.terrain(), outline)
    }

    fn zone(&self, id: ZoneId) -> Result<BoundaryZone, PlacementError> {
        self.coverage()
            .map()
            .zones()
            .get(id.index())
            .copied()
            .ok_or(PlacementError::UnknownZone(id))
    }

    /// Zone ownership remains borrowed from the boundary workspace.
    #[must_use]
    pub const fn coverage(&self) -> &TerrainCoverage<'_> {
        self.terrain.coverage()
    }
}

/// A resolved map cell. Construction admits its coordinates once; queries then
/// use borrowed cell state without another lookup. A cell is tied to its map
/// borrow, so it cannot survive mutation or be used as an index into another map.
///
/// ```compile_fail
/// use homm3_rmg::{domain::WorldPosition, placement::{MapCell, PlacementMap}};
/// fn cannot_escape(map: &PlacementMap<'_>, at: WorldPosition) -> MapCell<'static> {
///     map.cell(at).unwrap()
/// }
/// ```
#[derive(Clone, Copy)]
pub struct MapCell<'a> {
    position: WorldPosition,
    state: &'a CellState,
    terrain: Terrain,
    zone: Option<ZoneId>,
}
impl<'a> MapCell<'a> {
    /// Canonical in-bounds position of this cell.
    #[must_use]
    pub const fn position(self) -> WorldPosition {
        self.position
    }
    /// State borrowed from the owning map.
    #[must_use]
    pub const fn state(self) -> &'a CellState {
        self.state
    }
    /// Painted terrain at this cell.
    #[must_use]
    pub const fn terrain(self) -> Terrain {
        self.terrain
    }
    /// Zone assigned to this cell, if any.
    #[must_use]
    pub const fn zone(self) -> Option<ZoneId> {
        self.zone
    }
    /// Whether both cell state and terrain allow passage.
    #[must_use]
    pub fn passable(self) -> bool {
        self.state.passable && self.terrain != Terrain::Rock
    }
    fn blocked(self, zone: Option<ZoneId>) -> bool {
        !self.passable() || self.state.entrance.is_some() || self.zone != zone
    }
    fn clear_outline(self) -> bool {
        self.state.entrance.is_none()
            && self.passable()
            && self.state.reservation == PathReservation::Open
    }
}

// A private slice view also permits focused synthetic maps in tests without
// manufacturing completed generation-stage tokens or copying production grids.
enum PlacementSurface<'a> {
    World {
        terrain: &'a [crate::terrain_rules::TerrainTile],
        zones: &'a [crate::raster::ZoneCell],
    },
    Group,
}
struct PlacementView<'a> {
    layout: CellLayout,
    surface: PlacementSurface<'a>,
    cells: &'a [CellState],
}
impl<'a> PlacementView<'a> {
    const fn side(&self) -> usize {
        self.layout.side()
    }
    fn signed_side(&self) -> i32 {
        self.layout.signed_side()
    }
    fn cell(&self, position: WorldPosition) -> Result<MapCell<'a>, PlacementError> {
        let index = self.index(position)?;
        Ok(self.resolved_cell(index, position))
    }
    // Resolve flat aliases explicitly; coordinate calculations keep their original
    // signed positions, while this view names the allocated cell they address.
    fn aliased_cell(&self, position: WorldPosition) -> Result<MapCell<'a>, PlacementError> {
        let index = self.native_index(position)?;
        Ok(self.resolved_cell(index, self.layout.position(index)))
    }
    fn resolved_cell(&self, index: usize, position: WorldPosition) -> MapCell<'a> {
        MapCell {
            position,
            state: &self.cells[index],
            terrain: self.terrain(index),
            zone: self.zone(index),
        }
    }
    fn terrain(&self, index: usize) -> Terrain {
        match &self.surface {
            PlacementSurface::World { terrain, .. } => terrain[index].terrain(),
            PlacementSurface::Group => Terrain::Dirt,
        }
    }
    fn zone(&self, index: usize) -> Option<ZoneId> {
        match &self.surface {
            PlacementSurface::World { zones, .. } => zones[index].zone,
            PlacementSurface::Group => None,
        }
    }
    // Source getMapItem performs flat signed indexing without an XY check.
    // Negative X may alias the preceding row while still addressing this allocation.
    // Keep this separate from queries whose source explicitly checks containsXY.
    // With a four-cell row, out-of-row coordinates can name the same cell:
    //
    //                 x=0  1  2  3
    //           y=0    .   .  .  A  <- also (-1, 1)
    //           y=1    B   .  .  .
    //                  ^
    //                  also (4, 0)
    //
    // The index is (level * side + y) * side + x. A row beyond the surface
    // can likewise reach underground; only leaving the allocation is a fault.
    fn native_index(&self, position: WorldPosition) -> Result<usize, PlacementError> {
        let side = self.signed_side();
        let level = i32::try_from(position.level.index())
            .map_err(|_| PlacementError::CoordinateOverflow)?;
        let index = level
            .checked_mul(side)
            .and_then(|value| value.checked_add(position.point.y))
            .and_then(|value| value.checked_mul(side))
            .and_then(|value| value.checked_add(position.point.x))
            .ok_or(PlacementError::CoordinateOverflow)?;
        let index = usize::try_from(index).map_err(|_| PlacementError::OutsideMap(position))?;
        if index >= self.cells.len() {
            return Err(PlacementError::OutsideMap(position));
        }
        Ok(index)
    }
    fn contains(&self, point: Point) -> bool {
        self.layout.plane_index(point).is_some()
    }
    fn index(&self, position: WorldPosition) -> Result<usize, PlacementError> {
        self.layout
            .index(position)
            .filter(|&index| index < self.cells.len())
            .ok_or(PlacementError::OutsideMap(position))
    }
    fn passable(&self, index: usize) -> bool {
        self.cells[index].passable && self.terrain(index) != Terrain::Rock
    }
    fn clear_outline_cell(&self, index: usize) -> bool {
        self.cells[index].entrance.is_none()
            && self.passable(index)
            && self.cells[index].reservation == PathReservation::Open
    }
    fn footprint_blocked(
        &self,
        entry: &PreparedPrototype<'_>,
        anchor: WorldPosition,
        zone: Option<ZoneId>,
        obstacles: ObstacleEntrances,
    ) -> Result<bool, PlacementError> {
        let size = entry.image_mask().size()?;
        if anchor.point.x < i32::from(size.width()) - 1
            || anchor.point.y < i32::from(size.height()) - 1
            || !self.contains(anchor.point)
        {
            return Ok(true);
        }
        let prototype = entry.prototype();
        let water_only = i64::from(prototype.category()) == i64::from(raw::OBJECT_SLOT_CATEGORY_0)
            && prototype.recommends(Terrain::Water);
        for mask_cell in size.cells() {
            let (x, y) = (mask_cell.x(), mask_cell.y());
            // Bounds above prove every footprint coordinate is in range.
            let cell = self.cell(WorldPosition {
                point: Point::new(anchor.point.x - i32::from(x), anchor.point.y - i32::from(y)),
                level: anchor.level,
            })?;
            if prototype.is_trigger(mask_cell)
                && (cell.blocked(zone)
                    || (obstacles == ObstacleEntrances::Reject
                        && cell.state.reservation == PathReservation::Obstacle))
            {
                return Ok(true);
            }
            if !prototype.is_passable(mask_cell)
                && (cell.blocked(zone) || (cell.terrain() == Terrain::Water) != water_only)
            {
                return Ok(true);
            }
        }
        Ok(false)
    }
    fn has_connected_outline(
        &self,
        outline: NonEmptyOutline<'_>,
        anchor: WorldPosition,
        entrances: OutlineEntrances,
        zone: ZoneId,
        zone_terrain: Terrain,
        clearance: OutlineClearance,
    ) -> Result<bool, PlacementError> {
        let mut blocked = true;
        let mut found_boundary = false;
        for offset in outline.0.iter().chain(outline.0.first()) {
            let previously_blocked = blocked;
            let point = Point::new(
                anchor
                    .point
                    .x
                    .checked_add(offset.x)
                    .ok_or(PlacementError::CoordinateOverflow)?,
                anchor
                    .point
                    .y
                    .checked_add(offset.y)
                    .ok_or(PlacementError::CoordinateOverflow)?,
            );
            blocked = if self.contains(point) {
                let cell = self.cell(WorldPosition {
                    point,
                    level: anchor.level,
                })?;
                if entrances == OutlineEntrances::Reject && cell.state.entrance.is_some() {
                    return Ok(false);
                }
                cell.blocked(Some(zone))
                    || (clearance == OutlineClearance::Require
                        && cell.state.reservation != PathReservation::Open)
                    || (cell.terrain() == Terrain::Water) != (zone_terrain == Terrain::Water)
            } else {
                true
            };
            if blocked && !previously_blocked {
                if found_boundary {
                    return Ok(false);
                }
                found_boundary = true;
            }
        }
        Ok(!blocked || found_boundary)
    }
    fn can_place(
        &self,
        entry: &PreparedPrototype<'_>,
        anchor: WorldPosition,
        zone: ZoneId,
        zone_terrain: Terrain,
        workspace: &mut OutlineWorkspace,
    ) -> Result<bool, PlacementError> {
        if self.footprint_blocked(entry, anchor, Some(zone), ObstacleEntrances::Allow)? {
            return Ok(false);
        }
        let traits = entry.prototype().kind().traits();
        let entrances = if traits.cleared_on_visit() && traits.enterable_from_north() {
            OutlineEntrances::Allow
        } else {
            OutlineEntrances::Reject
        };
        let points = workspace.trace(entry)?;
        let outline = NonEmptyOutline::parse(points).ok_or(PlacementError::EmptyOutline)?;
        if !self.has_connected_outline(
            outline,
            anchor,
            entrances,
            zone,
            zone_terrain,
            OutlineClearance::Ignore,
        )? {
            return Ok(false);
        }
        let Some(trigger) = entry.prototype().entrance() else {
            return Ok(true);
        };
        // Approach cell immediately south of the object's trigger. Mask cell (column, row)
        // lies at position - (column, row): the mask grows west and north from the
        // object's bottom-right cell P. North is up.
        //   (2,1) (1,1) (0,1)
        //   (2,0) (1,0) (0,0)=P
        let point = Point::new(
            anchor.point.x - i32::from(trigger.x()),
            anchor.point.y - i32::from(trigger.y()) + 1,
        );
        if usize::try_from(point.y).is_ok_and(|y| y >= self.side()) {
            return Ok(false);
        }
        let cell = self.cell(WorldPosition {
            point,
            level: anchor.level,
        })?;
        Ok(cell.passable()
            && cell.zone() == Some(zone)
            && cell
                .state
                .entrance
                .is_none_or(|kind| kind.traits().cleared_on_visit())
            && (cell.terrain() == Terrain::Water) == (zone_terrain == Terrain::Water))
    }
}

// Native world offsets retain the current plane; callers choose flat access or clipping.
fn offset_position(position: WorldPosition, delta: Point) -> Result<WorldPosition, PlacementError> {
    Ok(WorldPosition {
        point: position
            .point
            .checked_add(delta)
            .ok_or(PlacementError::CoordinateOverflow)?,
        level: position.level,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{domain::Level, line::Reflection, raster::ZoneCell, terrain_rules::TerrainTile};

    #[test]
    fn admitted_cells_borrow_state_and_flat_aliases_have_canonical_positions() {
        let cells = vec![CellState::default(); 32];
        let view = PlacementView {
            layout: CellLayout::new(4),
            surface: PlacementSurface::Group,
            cells: &cells,
        };
        let at = |x, y, level| WorldPosition {
            point: Point::new(x, y),
            level,
        };
        let position = at(3, 0, Level::Surface);
        let cell = view.cell(position).unwrap();
        assert_eq!(cell.position(), position);
        assert!(std::ptr::eq(cell.state(), std::ptr::from_ref(&cells[3])));
        assert_eq!(cell.terrain(), Terrain::Dirt);
        assert_eq!(cell.zone(), None);
        assert!(view.cell(at(-1, 1, Level::Surface)).is_err());
        assert_eq!(
            view.aliased_cell(at(-1, 1, Level::Surface))
                .unwrap()
                .position(),
            position
        );
        assert_eq!(
            view.aliased_cell(at(0, 4, Level::Surface))
                .unwrap()
                .position(),
            at(0, 0, Level::Underground)
        );
        assert!(view.cell(at(0, 4, Level::Surface)).is_err());
        assert!(view.aliased_cell(at(0, 4, Level::Underground)).is_err());
        let surface = PlacementView {
            layout: CellLayout::new(4),
            surface: PlacementSurface::Group,
            cells: &cells[..16],
        };
        assert!(surface.cell(at(0, 0, Level::Underground)).is_err());
    }

    #[test]
    fn native_flat_access_preserves_in_allocation_aliases_and_rejects_overflow() {
        let cells = vec![CellState::default(); 36 * 36];
        let view = PlacementView {
            layout: CellLayout::new(36),
            surface: PlacementSurface::World {
                terrain: &[],
                zones: &[],
            },
            cells: &cells,
        };
        let at = |x, y| WorldPosition {
            point: Point::new(x, y),
            level: Level::Surface,
        };
        assert_eq!(view.native_index(at(-3, 4)).unwrap(), 141);
        assert!(matches!(
            view.index(at(-3, 4)),
            Err(PlacementError::OutsideMap(_))
        ));
        assert_eq!(view.native_index(at(i32::MAX, -59_652_323)).unwrap(), 19);
        assert!(matches!(
            view.native_index(at(-1, 0)),
            Err(PlacementError::OutsideMap(_))
        ));
        assert!(matches!(
            view.native_index(at(0, 36)),
            Err(PlacementError::OutsideMap(_))
        ));
        assert!(matches!(
            view.native_index(at(0, i32::MAX)),
            Err(PlacementError::CoordinateOverflow)
        ));
    }

    #[test]
    fn only_triggers_reject_obstacle_fill_and_blocked_cells_apply_water_policy() {
        use crate::{
            behavior::Behavior,
            placement_rules::PlacementRules,
            prototype::{ImageMask, PrototypeSource},
            request::MapVersion,
        };
        use std::convert::Infallible;
        let rules =
            PlacementRules::parse(b"header\r\nheader\r\nheader\r\n", Behavior::Hotfix).unwrap();
        let mut bytes = [0; 14];
        bytes[0] = 1;
        bytes[1] = 1;
        bytes[7] = 128; // Anchor is draw bit 47.
        let mask = ImageMask::parse(&bytes).unwrap();
        let zone = ZoneId::new(0);
        let zones = [ZoneCell {
            zone: Some(zone),
            paint_terrain: true,
        }];
        let cells = [CellState {
            reservation: PathReservation::Obstacle,
            ..CellState::default()
        }];
        let anchor = WorldPosition {
            point: Point::new(0, 0),
            level: Level::Surface,
        };
        for trigger in [false, true] {
            for water_only in [false, true] {
                let row = format!(
                    "1\r\ntest.def 0 {:048b} 1 {:09b} {} 0 0 0\r\n",
                    u64::from(trigger) << 47,
                    if water_only {
                        1 << Terrain::Water.index()
                    } else {
                        1
                    },
                    raw::RESOURCE
                );
                let source =
                    PrototypeSource::parse(row.as_bytes(), |_| Ok::<_, Infallible>(Some(mask)))
                        .unwrap();
                let catalog = source
                    .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
                    .unwrap();
                let entry = &catalog.entries()[0];
                for terrain in [Terrain::Grass, Terrain::Water] {
                    let tiles = [TerrainTile::parse(terrain, 0, Reflection::default()).unwrap()];
                    let view = PlacementView {
                        layout: CellLayout::new(1),
                        surface: PlacementSurface::World {
                            terrain: &tiles,
                            zones: &zones,
                        },
                        cells: &cells,
                    };
                    let wrong_water = (terrain == Terrain::Water) != water_only;
                    assert_eq!(
                        view.footprint_blocked(entry, anchor, Some(zone), ObstacleEntrances::Allow)
                            .unwrap(),
                        wrong_water
                    );
                    assert_eq!(
                        view.footprint_blocked(
                            entry,
                            anchor,
                            Some(zone),
                            ObstacleEntrances::Reject
                        )
                        .unwrap(),
                        wrong_water || trigger
                    );
                    assert!(view
                        .footprint_blocked(entry, anchor, None, ObstacleEntrances::Allow)
                        .unwrap());
                }
            }
        }
    }

    #[test]
    fn all_outline_blocking_patterns_preserve_circular_connectivity() {
        let zone = ZoneId::new(0);
        let terrain = [TerrainTile::parse(Terrain::Grass, 0, Reflection::default()).unwrap(); 9];
        let zones = [ZoneCell {
            zone: Some(zone),
            paint_terrain: true,
        }; 9];
        let ring = [
            Point::new(-1, -1),
            Point::new(0, -1),
            Point::new(1, -1),
            Point::new(1, 0),
            Point::new(1, 1),
            Point::new(0, 1),
            Point::new(-1, 1),
            Point::new(-1, 0),
        ];
        let outline = NonEmptyOutline::parse(&ring).unwrap();
        let anchor = WorldPosition {
            point: Point::new(1, 1),
            level: Level::Surface,
        };
        let indices = [0, 1, 2, 5, 8, 7, 6, 3];
        for pattern in 0_u8..=255 {
            let mut cells = [CellState::default(); 9];
            for (bit, &index) in indices.iter().enumerate() {
                cells[index].passable = pattern & (1 << bit) == 0;
            }
            let view = PlacementView {
                layout: CellLayout::new(3),
                surface: PlacementSurface::World {
                    terrain: &terrain,
                    zones: &zones,
                },
                cells: &cells,
            };
            let open_runs = (pattern & !pattern.rotate_left(1)).count_ones();
            let expected = pattern == 0 || (pattern != 255 && open_runs == 1);
            assert_eq!(
                view.has_connected_outline(
                    outline,
                    anchor,
                    OutlineEntrances::Allow,
                    zone,
                    Terrain::Grass,
                    OutlineClearance::Ignore
                )
                .unwrap(),
                expected,
                "{pattern:08b}"
            );
        }
    }

    #[test]
    fn entrance_path_and_water_policies_are_independent() {
        let zone = ZoneId::new(0);
        let mut cells = [CellState::default(); 4];
        cells[0].entrance = Some(ObjectKind::RESOURCE);
        cells[1].reservation = PathReservation::Unreserved;
        cells[2].reservation = PathReservation::Obstacle;
        let mut terrain =
            [TerrainTile::parse(Terrain::Grass, 0, Reflection::default()).unwrap(); 4];
        terrain[3] = TerrainTile::parse(Terrain::Water, 0, Reflection::default()).unwrap();
        let zones = [ZoneCell {
            zone: Some(zone),
            paint_terrain: true,
        }; 4];
        let view = PlacementView {
            layout: CellLayout::new(2),
            surface: PlacementSurface::World {
                terrain: &terrain,
                zones: &zones,
            },
            cells: &cells,
        };
        let points = [
            Point::new(0, 0),
            Point::new(1, 0),
            Point::new(0, 1),
            Point::new(1, 1),
        ];
        let outline = NonEmptyOutline::parse(&points).unwrap();
        let anchor = WorldPosition {
            point: Point::new(0, 0),
            level: Level::Surface,
        };
        assert!(!view
            .has_connected_outline(
                outline,
                anchor,
                OutlineEntrances::Reject,
                zone,
                Terrain::Grass,
                OutlineClearance::Ignore
            )
            .unwrap());
        assert!(view
            .has_connected_outline(
                outline,
                anchor,
                OutlineEntrances::Allow,
                zone,
                Terrain::Grass,
                OutlineClearance::Ignore
            )
            .unwrap());
        assert!(!view
            .has_connected_outline(
                outline,
                anchor,
                OutlineEntrances::Allow,
                zone,
                Terrain::Grass,
                OutlineClearance::Require
            )
            .unwrap());
        assert!(view
            .has_connected_outline(
                outline,
                anchor,
                OutlineEntrances::Allow,
                zone,
                Terrain::Water,
                OutlineClearance::Require
            )
            .unwrap());
        assert!(NonEmptyOutline::parse(&[]).is_none());
        assert!(matches!(
            view.has_connected_outline(
                outline,
                WorldPosition {
                    level: Level::Underground,
                    ..anchor
                },
                OutlineEntrances::Allow,
                zone,
                Terrain::Grass,
                OutlineClearance::Ignore
            ),
            Err(PlacementError::OutsideMap(_))
        ));
    }
}
