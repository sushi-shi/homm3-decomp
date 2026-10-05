//! Branch carving and reservations preceding water islands and connection searches.

use super::{
    neighborhood::Neighborhood, BorderColor, GuardStrength, PathReservation, PlacementError,
    PlacementMap, PlacementView, TownsPlaced,
};
use crate::{
    boundaries::{BoundaryZone, ZoneConnection},
    domain::{Level, Terrain, WorldPosition},
    geometry::{GeometryError, Point, ZoneId},
    object::ObjectKind,
    prototype::{PreparedPrototype, PrototypeCatalog, PrototypeId},
    raw,
    rng::{RetailRng, RngCheckpoint},
    terrain::TerrainError,
    worklist::Worklist,
};
use std::{
    collections::{TryReserveError, VecDeque},
    error::Error,
    fmt,
    num::NonZeroU32,
};

/// Connection preparation or creation reached a native fault or failed allocation.
#[derive(Debug)]
pub enum ConnectionError {
    /// Cell access, coordinate arithmetic or placement state failed.
    Placement(PlacementError),
    /// A native vector length or subdivision operation failed.
    Geometry(GeometryError),
    /// Junction boundary subdivision failed.
    Raster(crate::raster::RasterError),
    /// A terrain brush failed while repainting an island.
    Terrain(TerrainError),
    /// Creature selection or placement failed while guarding a crossing.
    Guard(super::GuardPlacementError),
    /// A directed-record handle came from another connection stage.
    UnknownConnection(super::ConnectionId),
    /// A whole-family draw attempted the native remainder with no prototypes.
    EmptyFamily(crate::object::ObjectKind),
    /// Native portal indexing reached an absent entry before any placement.
    MissingPortalPrototype {
        /// Indexed two-way, one-way entrance or one-way exit family.
        family: ObjectKind,
        /// Family-relative native index.
        index: usize,
    },
    /// Native completion marks this edge before dereferencing a missing reverse.
    MissingReverse {
        /// Already-completed edge's starting zone.
        source: ZoneId,
        /// Zone whose reverse adjacency is absent.
        destination: ZoneId,
    },
    /// No dirt border-guard prototype has the stored color.
    MissingBorderGuard(BorderColor),
    /// A reached positive-cost path has no predecessor or contains a cycle.
    InvalidPredecessor(WorldPosition),
    /// Retail reads uninitialized coordinates before any zone provides a seed.
    SeedReplayRequired(ZoneId),
}
impl fmt::Display for ConnectionError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Placement(error) => error.fmt(f),
            Self::Geometry(error) => error.fmt(f),
            Self::Raster(error) => error.fmt(f),
            Self::Terrain(error) => error.fmt(f),
            Self::Guard(error) => error.fmt(f),
            Self::EmptyFamily(kind) => {
                write!(f, "connection prototype family {} is empty", kind.index())
            }
            Self::MissingPortalPrototype { family, index } => {
                write!(f, "portal family {} has no entry {index}", family.index())
            }
            Self::UnknownConnection(id) => {
                write!(f, "connection {} belongs to another stage", id.index())
            }
            Self::MissingReverse {
                source,
                destination,
            } => write!(
                f,
                "connection {} to {} has no reverse record",
                source.index(),
                destination.index()
            ),
            Self::MissingBorderGuard(color) => write!(
                f,
                "missing border-guard prototype for color {}",
                color.value()
            ),
            Self::SeedReplayRequired(zone) => write!(
                f,
                "retail connection seed for zone {} requires replay input",
                zone.index()
            ),
            Self::InvalidPredecessor(position) => {
                write!(f, "invalid connection predecessor at {position:?}")
            }
        }
    }
}
impl Error for ConnectionError {}
impl From<PlacementError> for ConnectionError {
    fn from(error: PlacementError) -> Self {
        Self::Placement(error)
    }
}
impl From<super::GuardPlacementError> for ConnectionError {
    fn from(error: super::GuardPlacementError) -> Self {
        Self::Guard(error)
    }
}
impl From<GeometryError> for ConnectionError {
    fn from(error: GeometryError) -> Self {
        Self::Geometry(error)
    }
}
impl From<crate::raster::RasterError> for ConnectionError {
    fn from(error: crate::raster::RasterError) -> Self {
        Self::Raster(error)
    }
}
impl From<TerrainError> for ConnectionError {
    fn from(error: TerrainError) -> Self {
        Self::Terrain(error)
    }
}
impl From<TryReserveError> for ConnectionError {
    fn from(error: TryReserveError) -> Self {
        Self::Placement(error.into())
    }
}

// Native stores adjacent point pairs. A record cannot contain a dangling half.
#[derive(Clone, Copy, Debug)]
struct Segment {
    from: Point,
    to: Point,
}
#[derive(Default, Debug)]
pub(super) struct ConnectionScratch {
    pending: Vec<Segment>,
    branches: VecDeque<Segment>,
    pub(super) junction_targets: Vec<Point>,
    pub(super) candidates: Vec<WorldPosition>,
    pub(super) outline: crate::prototype::OutlineWorkspace,
    pub(super) water_stack: Vec<WorldPosition>,
    pub(super) portals: super::portals::PortalState,
    pub(super) crossing: super::zone_connections::CrossingState,
    pub(super) flood: Worklist<WorldPosition>,
    pub(super) noise: super::island_noise::NoiseWorkspace,
    pub(super) repairs: Vec<super::connection_paths::PaintRequest>,
}
impl ConnectionScratch {
    pub(super) fn reset(&mut self) {
        self.pending.clear();
        self.junction_targets.clear();
        self.branches.clear();
        self.candidates.clear();
        self.water_stack.clear();
        self.portals.reset();
        self.crossing.reset();
        self.flood.clear();
        self.noise.clear();
        self.repairs.clear();
    }
}

/// Branching paths and border reservations are ready for water-zone islands.
pub struct ConnectionBorders<'state, 'zones, 'tiles> {
    pub(super) towns: TownsPlaced<'state, 'zones, 'tiles>,
    rng: RngCheckpoint,
}
impl ConnectionBorders<'_, '_, '_> {
    /// Placement state after the in-place branch and border passes.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.towns.map()
    }
    /// Town payloads and their earlier stage checkpoint remain available.
    #[must_use]
    pub const fn towns(&self) -> &TownsPlaced<'_, '_, '_> {
        &self.towns
    }
    /// RNG after branching; border marking itself draws nothing.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}
impl<'state, 'zones, 'tiles> TownsPlaced<'state, 'zones, 'tiles> {
    /// Carve branching paths, mark zone borders, then patch unassigned dry cells.
    /// Consuming the town stage prevents repeating its branching RNG work.
    ///
    /// # Errors
    /// Reports native arithmetic/access faults or scratch-allocation failure.
    pub fn reserve_connection_borders(
        mut self,
        rng: &mut RetailRng,
    ) -> Result<ConnectionBorders<'state, 'zones, 'tiles>, ConnectionError> {
        self.map.carve_branching_paths(rng)?;
        self.map.mark_zone_borders()?;
        self.map.mark_unassigned_obstacles()?;
        Ok(ConnectionBorders {
            towns: self,
            rng: rng.checkpoint(),
        })
    }
}

impl PlacementMap<'_, '_, '_> {
    fn carve_branching_paths(&mut self, rng: &mut RetailRng) -> Result<(), ConnectionError> {
        for cell in &mut *self.cells {
            if self.memberships.first(cell.objects).is_none() {
                cell.mark_obstacle();
            } else {
                cell.open_path();
            }
        }
        let side =
            i32::try_from(self.view().side).map_err(|_| PlacementError::CoordinateOverflow)?;
        for level in [Level::Surface, Level::Underground]
            .into_iter()
            .take(self.coverage().map().request().levels().count() as usize)
        {
            self.connections.reset();
            let count = NonZeroU32::new(raw::RMG_BRANCH_SEED_PATTERN_COUNT).unwrap();
            // Initial segment direction. North is up and # is the segment.
            //   MAIN_DIAGONAL  VERTICAL  ANTI_DIAGONAL  HORIZONTAL
            //   # . .          . # .     . . #          . . .
            //   . # .          . # .     . # .          # # #
            //   . . #          . # .     # . .          . . .
            let segment = match rng.below(count) {
                raw::RMG_BRANCH_SEED_MAIN_DIAGONAL => Segment {
                    from: Point::new(0, 0),
                    to: Point::new(side - 1, side - 1),
                },
                raw::RMG_BRANCH_SEED_VERTICAL => Segment {
                    from: Point::new(side / 2, 0),
                    to: Point::new(side / 2, side - 1),
                },
                raw::RMG_BRANCH_SEED_ANTI_DIAGONAL => Segment {
                    from: Point::new(side - 1, 0),
                    to: Point::new(0, side - 1),
                },
                raw::RMG_BRANCH_SEED_HORIZONTAL => Segment {
                    from: Point::new(0, side / 2),
                    to: Point::new(side - 1, side / 2),
                },
                _ => unreachable!("source seed patterns cover the drawn domain"),
            };
            self.connections.pending.try_reserve(1)?;
            self.connections.pending.push(segment);
            self.carve_plane(level, rng)?;
        }
        for index in 0..self.cells.len() {
            if matches!(
                self.terrain.tiles()[index].terrain(),
                Terrain::Water | Terrain::Rock
            ) {
                self.cells[index].open_path();
            }
            if self.cells[index].reservation == PathReservation::Obstacle {
                self.mark_obstacle_patch(self.position_at(index))?;
            }
        }
        Ok(())
    }

    fn carve_plane(&mut self, level: Level, rng: &mut RetailRng) -> Result<(), ConnectionError> {
        while !self.connections.pending.is_empty() {
            while let Some(Segment { from, to }) = self.connections.pending.pop() {
                let mut middle = from
                    .subdivision_midpoint(to)
                    .ok_or(PlacementError::CoordinateOverflow)?;
                if middle == from || middle == to {
                    if self.view().contains(from) {
                        self.open_path_patch(WorldPosition { point: from, level })?;
                    }
                    continue;
                }
                let delta = to
                    .checked_sub(from)
                    .ok_or(PlacementError::CoordinateOverflow)?;
                let perpendicular = Point::new(
                    delta
                        .y
                        .checked_neg()
                        .ok_or(PlacementError::CoordinateOverflow)?,
                    delta.x,
                );
                let length = perpendicular.distance(Point::new(0, 0))?;
                if length > 1 {
                    let range = NonZeroU32::new(u32::try_from(length).unwrap()).unwrap();
                    let displacement = rng.centered_offset(range);
                    let offset = perpendicular
                        .checked_scale_ratio(displacement, length)
                        .ok_or(PlacementError::CoordinateOverflow)?;
                    middle = middle
                        .checked_add(offset)
                        .ok_or(PlacementError::CoordinateOverflow)?;
                }
                self.connections.pending.try_reserve(2)?;
                // Native pushes [last,middle,middle,first], then pops two points:
                // next is (middle,first), followed by (last,middle).
                self.connections.pending.push(Segment {
                    from: to,
                    to: middle,
                });
                self.connections.pending.push(Segment {
                    from: middle,
                    to: from,
                });
                if i64::from(length) >= i64::from(crate::constants::RMG_BRANCH_MINIMUM_SPLIT_LENGTH)
                    && self.view().contains(middle)
                {
                    let positive = middle
                        .checked_add(perpendicular)
                        .ok_or(PlacementError::CoordinateOverflow)?;
                    let negative = middle
                        .checked_sub(perpendicular)
                        .ok_or(PlacementError::CoordinateOverflow)?;
                    self.connections.branches.try_reserve(2)?;
                    self.connections.branches.push_back(Segment {
                        from: middle,
                        to: positive,
                    });
                    self.connections.branches.push_back(Segment {
                        from: middle,
                        to: negative,
                    });
                }
            }
            while let Some(Segment { from, to }) = self.connections.branches.pop_front() {
                let end = self.trace_branch_end(from, to, level)?;
                if i64::from(end.squared_distance(from))
                    >= i64::from(crate::constants::RMG_BRANCH_MINIMUM_SQUARED_DISTANCE)
                {
                    self.connections.pending.try_reserve(1)?;
                    self.connections.pending.push(Segment {
                        from: end,
                        to: from,
                    });
                    break;
                }
            }
        }
        Ok(())
    }

    fn trace_branch_end(
        &self,
        mut from: Point,
        toward: Point,
        level: Level,
    ) -> Result<Point, ConnectionError> {
        let delta = toward
            .checked_sub(from)
            .ok_or(PlacementError::CoordinateOverflow)?;
        let dx = delta
            .x
            .checked_abs()
            .ok_or(PlacementError::CoordinateOverflow)?;
        let dy = delta
            .y
            .checked_abs()
            .ok_or(PlacementError::CoordinateOverflow)?;
        let diagonal = Point::new(
            if delta.x > 0 { 1 } else { -1 },
            if delta.y > 0 { 1 } else { -1 },
        );
        let (major, minor, axial) = if dx > dy {
            (dx, dy, Point::new(diagonal.x, 0))
        } else {
            (dy, dx, Point::new(0, diagonal.y))
        };
        let side =
            i32::try_from(self.view().side).map_err(|_| PlacementError::CoordinateOverflow)?;
        let mut error = major / 2;
        let mut steps = 0_u32;
        loop {
            let previous = from;
            error = error
                .checked_add(minor)
                .ok_or(PlacementError::CoordinateOverflow)?;
            steps = steps.checked_add(1).ok_or(PlacementError::Arithmetic)?;
            let step = if error < major {
                axial
            } else {
                error -= major;
                diagonal
            };
            from = from
                .checked_add(step)
                .ok_or(PlacementError::CoordinateOverflow)?;
            if from.x < 1 || from.x >= side - 1 || from.y < 1 || from.y >= side - 1 {
                return Ok(previous);
            }
            if steps > crate::constants::RMG_BRANCH_UNCHECKED_STEPS {
                // This helper scans X before Y, unlike neighborhood patches.
                for x in from.x - 1..=from.x + 1 {
                    for y in from.y - 1..=from.y + 1 {
                        let index = self.view().index(WorldPosition {
                            point: Point::new(x, y),
                            level,
                        })?;
                        if self.cells[index].reservation == PathReservation::Open {
                            return Ok(previous);
                        }
                    }
                }
            }
        }
    }

    pub(super) fn position_at(&self, index: usize) -> WorldPosition {
        let side = self.view().side;
        WorldPosition {
            point: Point::new(
                i32::try_from(index % side).unwrap(),
                i32::try_from(index / side % side).unwrap(),
            ),
            level: if index / (side * side) == 0 {
                Level::Surface
            } else {
                Level::Underground
            },
        }
    }

    fn mark_zone_borders(&mut self) -> Result<(), ConnectionError> {
        for index in 0..self.cells.len() {
            let Some(zone) = self.coverage().map().raster().cells()[index].zone else {
                continue;
            };
            if self.terrain.tiles()[index].terrain() == Terrain::Water {
                continue;
            }
            let position = self.position_at(index);
            let mut needs_border = false;
            for nearby in Neighborhood::ThreeByThree.cells(position, self.view().side)? {
                let index = self.view().index(nearby)?;
                match self.coverage().map().raster().cells()[index].zone {
                    None => needs_border |= self.terrain.tiles()[index].terrain() == Terrain::Water,
                    Some(other) if other != zone => {
                        let connection =
                            self.coverage()
                                .map()
                                .connections()
                                .iter()
                                .find(|connection| {
                                    connection.source == zone && connection.destination == other
                                });
                        if position.level == Level::Underground
                            || connection.is_none_or(|connection| !connection.unguarded)
                        {
                            needs_border = true;
                        }
                    }
                    _ => (),
                }
            }
            if needs_border {
                self.cells[index].mark_obstacle();
                self.release_neighborhood_path(position, Neighborhood::ThreeByThree)?;
            }
        }
        Ok(())
    }

    fn mark_unassigned_obstacles(&mut self) -> Result<(), ConnectionError> {
        for index in 0..self.cells.len() {
            let cell = &self.cells[index];
            if cell.reservation != PathReservation::Obstacle
                && self.view().passable(index)
                && cell.entrance.is_none()
                && self.memberships.first(cell.objects).is_none()
                && self.coverage().map().raster().cells()[index].zone.is_none()
                && self.terrain.tiles()[index].terrain() != Terrain::Water
            {
                self.mark_obstacle_patch(self.position_at(index))?;
            }
        }
        Ok(())
    }
}

// Whole-family selection performs its draw before the native zero-divisor fault.
pub(super) fn draw_connection_prototype(
    catalog: &PrototypeCatalog<'_>,
    family: ObjectKind,
    rng: &mut RetailRng,
) -> Result<PrototypeId, ConnectionError> {
    let draw = rng.draw();
    let count =
        u32::try_from(catalog.family(family).len()).map_err(|_| PlacementError::Arithmetic)?;
    let index = draw
        .checked_rem(count)
        .ok_or(ConnectionError::EmptyFamily(family))?;
    Ok(catalog
        .at(family, index as usize)
        .expect("remainder indexes admitted family"))
}

impl PlacementMap<'_, '_, '_> {
    pub(super) fn connection_guard_value(
        &self,
        connection: ZoneConnection,
    ) -> Result<i32, PlacementError> {
        if connection.unguarded {
            Ok(0)
        } else {
            GuardStrength::from(self.coverage().map().request().strength()).scale(connection.value)
        }
    }

    pub(super) fn object_fits(
        &mut self,
        entry: &PreparedPrototype<'_>,
        position: WorldPosition,
        zone: BoundaryZone,
    ) -> Result<bool, PlacementError> {
        let map = self.terrain.coverage().map();
        let view = PlacementView {
            side: map.raster().dimension(),
            surface: super::PlacementSurface::World {
                terrain: self.terrain.tiles(),
                zones: map.raster().cells(),
            },
            cells: self.cells,
        };
        view.can_place(
            entry,
            position,
            zone.id(),
            zone.terrain(),
            &mut self.connections.outline,
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{
        behavior::Behavior, placement_rules::PlacementRules, prototype::PrototypeSource,
        request::MapVersion,
    };

    #[test]
    fn empty_connection_family_faults_after_its_native_draw() {
        let source =
            PrototypeSource::parse(b"0\r\n", |_| Ok::<_, std::convert::Infallible>(None)).unwrap();
        let rules =
            PlacementRules::parse(b"header\r\nheader\r\nheader\r\n", Behavior::Hotfix).unwrap();
        let catalog = source
            .prepare(&rules, MapVersion::ShadowOfDeath, Behavior::Hotfix)
            .unwrap();
        let shipyard = ObjectKind::SHIPYARD;
        let mut rng = RetailRng::new(1);
        let mut expected = RetailRng::new(1);
        expected.draw();
        assert!(
            matches!(draw_connection_prototype(&catalog, shipyard, &mut rng), Err(ConnectionError::EmptyFamily(kind)) if kind == shipyard)
        );
        assert_eq!(rng.checkpoint(), expected.checkpoint());
    }
}
