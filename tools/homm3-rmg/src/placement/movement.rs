//! Clockwise movement directions and zone-path flood state.
use super::{CellState, PathReservation, PlacementError, PlacementMap, CLEARED_DISTANCE};
use crate::{
    domain::{Terrain, WorldPosition},
    geometry::{Point, ZoneId},
    raw,
};

#[expect(
    clippy::cast_possible_truncation,
    reason = "canonical cost is checked at compile time"
)]
const UNREACHED_DISTANCE: u16 = {
    assert!(raw::RMG_UNREACHED_COST <= u16::MAX as u32);
    raw::RMG_UNREACHED_COST as u16
};

/// Native movement order, distinct from terrain pattern directions.
/// North is up; . is the centre cell.
/// ```text
///   5 6 7
///   4 . 0
///   3 2 1
/// ```
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum Direction {
    /// East.
    East = raw::RMG_DIRECTION_EAST,
    /// South-east.
    SouthEast = raw::RMG_DIRECTION_SOUTH_EAST,
    /// South.
    South = raw::RMG_DIRECTION_SOUTH,
    /// South-west.
    SouthWest = raw::RMG_DIRECTION_SOUTH_WEST,
    /// West.
    West = raw::RMG_DIRECTION_WEST,
    /// North-west.
    NorthWest = raw::RMG_DIRECTION_NORTH_WEST,
    /// North.
    North = raw::RMG_DIRECTION_NORTH,
    /// North-east.
    NorthEast = raw::RMG_DIRECTION_NORTH_EAST,
}
impl Direction {
    pub(super) const ALL: [Self; raw::RMG_DIRECTION_COUNT as usize] = [
        Self::East,
        Self::SouthEast,
        Self::South,
        Self::SouthWest,
        Self::West,
        Self::NorthWest,
        Self::North,
        Self::NorthEast,
    ];
    pub(super) fn opposite(self) -> Self {
        Self::ALL[(self.index() + Self::ALL.len() / 2) % Self::ALL.len()]
    }
    pub(super) const fn southward(self) -> bool {
        matches!(self, Self::SouthEast | Self::South | Self::SouthWest)
    }
    /// Index into the source movement direction table.
    #[must_use]
    pub const fn index(self) -> usize {
        self as usize
    }
    pub(super) fn offset(self) -> Point {
        let (x, y) = raw::DIRECTIONS[self.index()];
        Point::new(x, y)
    }
    pub(super) const fn chamfer_cost(self) -> u32 {
        if self.index() % 2 == 0 {
            crate::constants::RMG_CHAMFER_CARDINAL_COST
        } else {
            crate::constants::RMG_CHAMFER_DIAGONAL_COST
        }
    }
}

/// Distance plus optional native connection metadata.
/// Absence of a connection never exposes an uninitialized direction.
#[derive(Clone, Copy, Debug)]
pub struct ZoneDistance {
    cost: u16,
    zone: Option<ZoneId>,
    direction: Direction,
}
impl Default for ZoneDistance {
    fn default() -> Self {
        Self {
            cost: CLEARED_DISTANCE,
            zone: None,
            direction: Direction::East,
        }
    }
}
impl ZoneDistance {
    pub(super) fn water(cost: u16, direction: Direction) -> Self {
        // Source setWaterZoneDistance writes zone zero, not the absent sentinel.
        Self {
            cost,
            zone: Some(ZoneId::new(0)),
            direction,
        }
    }
    pub(super) fn reset() -> Self {
        Self {
            cost: UNREACHED_DISTANCE,
            ..Self::default()
        }
    }
    fn from_flood(cost: u16, zone: Option<ZoneId>, direction: Direction) -> Self {
        Self {
            cost,
            zone,
            direction,
        }
    }
    /// Stored direction even when a flood originated in an unassigned cell.
    #[must_use]
    pub const fn direction(self) -> Direction {
        self.direction
    }
    /// Stored native distance; water spacing uses cardinal 2 / diagonal 3.
    #[must_use]
    pub const fn cost(self) -> u16 {
        self.cost
    }
    /// Native connection zone and direction, when assigned.
    /// Water spacing stores zone zero and a forward direction; these values
    /// do not identify the flood origin or a backtracking route.
    #[must_use]
    pub const fn connection(self) -> Option<(ZoneId, Direction)> {
        match self.zone {
            Some(zone) => Some((zone, self.direction)),
            None => None,
        }
    }
}
impl CellState {
    /// Current water-spacing or subsequent connection-flood metadata.
    #[must_use]
    pub const fn zone_distance(&self) -> ZoneDistance {
        self.zone_distance
    }
}

/// Movement cost and predecessor from a map search.
/// Zero-cost arrivals retain their predecessor but stop path opening like seeds.
#[derive(Clone, Copy, Debug, Default)]
pub enum Movement {
    /// Cell has not participated in a movement search.
    #[default]
    Initial,
    /// Search cleared the cell but has not reached it.
    Unreached,
    /// Search origin, with zero cost and no predecessor.
    Seed,
    /// Search reached the cell from a predecessor, including at zero cost.
    Arrived {
        /// Stored path cost.
        cost: u16,
        /// Cell from which the search arrived.
        previous: WorldPosition,
    },
}
impl Movement {
    pub(super) fn arrived(cost: u32, previous: WorldPosition) -> Result<Self, PlacementError> {
        Ok(Self::Arrived {
            cost: u16::try_from(cost).map_err(|_| PlacementError::Arithmetic)?,
            previous,
        })
    }
    /// Stored movement cost, including the two unreached sentinels.
    #[must_use]
    pub fn cost(self) -> u16 {
        match self {
            Self::Initial => CLEARED_DISTANCE,
            Self::Unreached => UNREACHED_DISTANCE,
            Self::Seed => 0,
            Self::Arrived { cost, .. } => cost,
        }
    }
    /// Predecessor for diagnostic snapshots, including zero-cost arrivals.
    #[must_use]
    pub const fn previous(self) -> Option<WorldPosition> {
        match self {
            Self::Arrived { previous, .. } => Some(previous),
            _ => None,
        }
    }
}
impl CellState {
    /// Current search cost and predecessor, independent of object distance.
    #[must_use]
    pub const fn movement(&self) -> Movement {
        self.movement
    }
}
impl PlacementMap<'_, '_, '_> {
    /// Flood within the seed's zone and spill into each adjacent zone.
    /// Existing costs remain; zero-cost clearance propagates after the strict
    /// improvement test. Equal-cost work items retain insertion order.
    ///
    /// # Errors
    /// Reports native coordinate/access faults or failed queue allocation.
    pub fn flood_connection_costs(
        &mut self,
        seed: WorldPosition,
        water_zone: bool,
    ) -> Result<(), PlacementError> {
        self.connections.flood.clear();
        self.connections.flood.insert(seed, 0)?;
        let index = self.view().native_index(seed)?;
        self.cells[index].movement = Movement::Seed;
        let zone = self.coverage().map().raster().cells()[index].zone;
        while let Some(position) = self.connections.flood.pop() {
            let index = self.view().native_index(position)?;
            let current_zone = self.coverage().map().raster().cells()[index].zone;
            let cost = u32::from(if current_zone == zone {
                self.cells[index].movement.cost()
            } else {
                self.cells[index].zone_distance.cost()
            });
            let directions = if self.cells[index]
                .entrance
                .is_some_and(|kind| !kind.traits().enterable_from_north())
            {
                Direction::NorthWest.index()
            } else {
                Direction::ALL.len()
            };
            for &direction in Direction::ALL[..directions].iter().rev() {
                let point = position
                    .point
                    .checked_add(direction.offset())
                    .ok_or(PlacementError::CoordinateOverflow)?;
                if !self.view().contains(point) {
                    continue;
                }
                let next = WorldPosition {
                    point,
                    level: position.level,
                };
                let index = self.view().index(next)?;
                let next_zone = self.coverage().map().raster().cells()[index].zone;
                if next_zone.is_none() || !self.view().passable(index) {
                    continue;
                }
                if let Some(kind) = self.cells[index].entrance {
                    let traits = kind.traits();
                    if (traits.blocks_landing() && !traits.cleared_on_visit())
                        || (!traits.enterable_from_north() && direction.southward())
                    {
                        continue;
                    }
                }
                let mut next_cost = cost + crate::constants::RMG_CONNECTION_LAND_STEP_COST;
                if next_zone == zone {
                    if current_zone != zone {
                        continue;
                    }
                    let water = self.terrain.tiles()[index].terrain() == Terrain::Water;
                    if water {
                        next_cost =
                            cost + crate::constants::RMG_CONNECTION_WATER_OR_BORDER_STEP_COST;
                    }
                    if u32::from(self.cells[index].movement.cost()) <= next_cost {
                        continue;
                    }
                    if cost == 0
                        && self.cells[index].reservation == PathReservation::Open
                        && (!water || water_zone)
                    {
                        next_cost = 0;
                    }
                    self.cells[index].movement = Movement::arrived(next_cost, position)?;
                } else {
                    next_cost = cost + crate::constants::RMG_CONNECTION_WATER_OR_BORDER_STEP_COST;
                    if current_zone != zone && current_zone != next_zone {
                        continue;
                    }
                    if u32::from(self.cells[index].zone_distance.cost()) <= next_cost {
                        continue;
                    }
                    self.cells[index].zone_distance = ZoneDistance::from_flood(
                        u16::try_from(next_cost).map_err(|_| PlacementError::Arithmetic)?,
                        zone,
                        direction.opposite(),
                    );
                }
                self.connections.flood.insert(
                    next,
                    i32::try_from(next_cost).map_err(|_| PlacementError::Arithmetic)?,
                )?;
            }
        }
        Ok(())
    }
}
