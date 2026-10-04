//! Clockwise movement directions and zone-path flood state.
use super::{CellState, CLEARED_DISTANCE};
use crate::{
    geometry::{Point, ZoneId},
    raw,
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
            raw::RMG_CHAMFER_CARDINAL_COST
        } else {
            raw::RMG_CHAMFER_DIAGONAL_COST
        }
    }
}

/// Distance plus optional native connection metadata.
/// Absence of a connection never exposes an uninitialized direction.
#[derive(Clone, Copy, Debug)]
pub struct ZoneDistance {
    cost: u16,
    connection: Option<(ZoneId, Direction)>,
}
impl Default for ZoneDistance {
    fn default() -> Self {
        Self {
            cost: CLEARED_DISTANCE,
            connection: None,
        }
    }
}
impl ZoneDistance {
    pub(super) fn water(cost: u16, direction: Direction) -> Self {
        // Source setWaterZoneDistance writes zone zero, not the absent sentinel.
        Self {
            cost,
            connection: Some((ZoneId::new(0), direction)),
        }
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
        self.connection
    }
}
impl CellState {
    /// Current water-spacing or subsequent connection-flood metadata.
    #[must_use]
    pub const fn zone_distance(&self) -> ZoneDistance {
        self.zone_distance
    }
}
