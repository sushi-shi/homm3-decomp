//! Object-fit queries over borrowed terrain and reusable placement state.

use crate::{
    boundaries::{BoundaryZone, TerrainCoverage},
    domain::{Terrain, WorldPosition},
    geometry::{Point, ZoneId},
    object::ObjectKind,
    prototype::{OutlineError, OutlineWorkspace, PreparedPrototype, PrototypeFault, PrototypeId},
    raw,
    terrain::PaintedTerrain,
};
use std::{collections::TryReserveError, error::Error, fmt};

mod objects;
pub use objects::{ObjectArena, ObjectGeometry, ObjectId};
mod mutation;
pub use mutation::BorderColor;
mod neighborhood;
pub use neighborhood::Neighborhood;
use objects::{Chain, Memberships};
mod registration;
use registration::Registration;
pub use registration::{KeyTentChoice, KeyTentColor};

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
    /// A generated path requires clearance here.
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
    /// Objects are being registered with a different catalog, mode or format.
    CatalogContext,
    /// Retail erases a missing global-list entry after that list allocated storage.
    NotRegistered(ObjectId),
    /// A signed per-type counter or path cost cannot be represented.
    Arithmetic,
    /// Border guard subtype does not index the native tent availability vector.
    KeyTentSubtype(i32),
    /// Process-local object ownership tags have been exhausted.
    IdentityExhausted,
    /// Prototype ID does not belong to the supplied catalog.
    UnknownPrototype(PrototypeId),
    /// Object ID does not belong to the supplied arena or its current generation.
    UnknownObject(ObjectId),
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
            Self::CatalogContext => {
                f.write_str("object catalog does not match the registered map context")
            }
            Self::NotRegistered(id) => {
                write!(f, "retail removes absent registered object {}", id.index())
            }
            Self::Arithmetic => f.write_str("object count or distance arithmetic overflow"),
            Self::KeyTentSubtype(value) => write!(
                f,
                "key-tent subtype {value} does not index availability storage"
            ),
            Self::IdentityExhausted => f.write_str("object ownership identities exhausted"),
            Self::UnknownPrototype(id) => {
                write!(f, "prototype {} belongs to another catalog", id.index())
            }
            Self::UnknownObject(id) => write!(
                f,
                "object {} belongs to another arena generation",
                id.index()
            ),
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
pub struct CellState {
    objects: Chain,
    retail_membership_storage: bool,
    border: Option<BorderColor>,
    object_distance: u16,
    passable: bool,
    entrance: Option<ObjectKind>,
    reservation: PathReservation,
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
        }
    }
}

/// Reusable flat placement state; no per-cell collections or terrain copies.
#[derive(Default, Debug)]
pub struct PlacementWorkspace {
    cells: Vec<CellState>,
    memberships: Memberships,
    registration: Registration,
}
impl PlacementWorkspace {
    /// Consume painted terrain into the placement stage and reset cell state.
    ///
    /// # Errors
    /// Returns a failed state-buffer reservation.
    pub fn begin<'state, 'zones, 'tiles>(
        &'state mut self,
        terrain: PaintedTerrain<'zones, 'tiles>,
    ) -> Result<PlacementMap<'state, 'zones, 'tiles>, PlacementError> {
        let count = terrain.tiles().len();
        self.cells
            .try_reserve(count.saturating_sub(self.cells.len()))?;
        self.cells.resize(count, CellState::default());
        self.cells.fill(CellState::default());
        self.memberships.reset();
        self.registration
            .reset(terrain.coverage().map().zones().len())?;
        Ok(PlacementMap {
            terrain,
            cells: &mut self.cells,
            memberships: &mut self.memberships,
            registration: &mut self.registration,
        })
    }
}

/// Placement stage owns its terrain-stage token while borrowing all map buffers.
pub struct PlacementMap<'state, 'zones, 'tiles> {
    terrain: PaintedTerrain<'zones, 'tiles>,
    cells: &'state mut [CellState],
    memberships: &'state mut Memberships,
    registration: &'state mut Registration,
}
impl PlacementMap<'_, '_, '_> {
    /// Existing painted tiles and zone coverage, without copying either buffer.
    #[must_use]
    pub const fn terrain(&self) -> &PaintedTerrain<'_, '_> {
        &self.terrain
    }

    fn view(&self) -> PlacementView<'_> {
        PlacementView {
            side: self.terrain.coverage().map().raster().dimension(),
            terrain: self.terrain.tiles(),
            zones: self.terrain.coverage().map().raster().cells(),
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
        zone: &BoundaryZone,
        clearance: OutlineClearance,
    ) -> Result<bool, PlacementError> {
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
        zone: &BoundaryZone,
        outline: &mut OutlineWorkspace,
    ) -> Result<bool, PlacementError> {
        self.view()
            .can_place(entry, anchor, zone.id(), zone.terrain(), outline)
    }

    /// Zone ownership remains borrowed from the boundary workspace.
    #[must_use]
    pub const fn coverage(&self) -> &TerrainCoverage<'_> {
        self.terrain.coverage()
    }
}

// A private slice view also permits focused synthetic maps in tests without
// manufacturing completed generation-stage tokens or copying production grids.
struct PlacementView<'a> {
    side: usize,
    terrain: &'a [crate::terrain_rules::TerrainTile],
    zones: &'a [crate::raster::ZoneCell],
    cells: &'a [CellState],
}
impl PlacementView<'_> {
    // Source getMapItem performs flat signed indexing without an XY check.
    // Negative X may alias the preceding row while still addressing this allocation.
    // Keep this separate from queries whose source explicitly checks containsXY.
    fn native_index(&self, position: WorldPosition) -> Result<usize, PlacementError> {
        let side = i32::try_from(self.side).map_err(|_| PlacementError::CoordinateOverflow)?;
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
        usize::try_from(point.x).is_ok_and(|x| x < self.side)
            && usize::try_from(point.y).is_ok_and(|y| y < self.side)
    }
    fn index(&self, position: WorldPosition) -> Result<usize, PlacementError> {
        if !self.contains(position.point) {
            return Err(PlacementError::OutsideMap(position));
        }
        let x =
            usize::try_from(position.point.x).map_err(|_| PlacementError::OutsideMap(position))?;
        let y =
            usize::try_from(position.point.y).map_err(|_| PlacementError::OutsideMap(position))?;
        let index = (position.level.index() * self.side + y) * self.side + x;
        if index >= self.cells.len() {
            return Err(PlacementError::OutsideMap(position));
        }
        Ok(index)
    }
    fn passable(&self, index: usize) -> bool {
        self.cells[index].passable && self.terrain[index].terrain() != Terrain::Rock
    }
    fn blocked(&self, index: usize, zone: Option<ZoneId>) -> bool {
        !self.passable(index)
            || self.cells[index].entrance.is_some()
            || self.zones[index].zone != zone
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
        for cell in size.cells() {
            let (x, y) = (cell.x(), cell.y());
            // Bounds above prove every footprint coordinate is in range.
            let index = self.index(WorldPosition {
                point: Point::new(anchor.point.x - i32::from(x), anchor.point.y - i32::from(y)),
                level: anchor.level,
            })?;
            if prototype.is_trigger(cell)
                && (self.blocked(index, zone)
                    || (obstacles == ObstacleEntrances::Reject
                        && self.cells[index].reservation == PathReservation::Obstacle))
            {
                return Ok(true);
            }
            if !prototype.is_passable(cell)
                && (self.blocked(index, zone)
                    || (self.terrain[index].terrain() == Terrain::Water) != water_only)
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
                let index = self.index(WorldPosition {
                    point,
                    level: anchor.level,
                })?;
                if entrances == OutlineEntrances::Reject && self.cells[index].entrance.is_some() {
                    return Ok(false);
                }
                self.blocked(index, Some(zone))
                    || (clearance == OutlineClearance::Require
                        && self.cells[index].reservation != PathReservation::Open)
                    || (self.terrain[index].terrain() == Terrain::Water)
                        != (zone_terrain == Terrain::Water)
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
        // Map position of an object's trigger cell. Footprint mask cell (column, row)
        // lies at position - (column, row): the mask grows west and north from the
        // object's bottom-right cell P. North is up.
        //   (2,1) (1,1) (0,1)
        //   (2,0) (1,0) (0,0)=P
        let point = Point::new(
            anchor.point.x - i32::from(trigger.x()),
            anchor.point.y - i32::from(trigger.y()) + 1,
        );
        if usize::try_from(point.y).is_ok_and(|y| y >= self.side) {
            return Ok(false);
        }
        let index = self.index(WorldPosition {
            point,
            level: anchor.level,
        })?;
        Ok(self.passable(index)
            && self.zones[index].zone == Some(zone)
            && self.cells[index]
                .entrance
                .is_none_or(|kind| kind.traits().cleared_on_visit())
            && (self.terrain[index].terrain() == Terrain::Water)
                == (zone_terrain == Terrain::Water))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{domain::Level, line::Reflection, raster::ZoneCell, terrain_rules::TerrainTile};

    #[test]
    fn native_flat_access_preserves_in_allocation_aliases_and_rejects_overflow() {
        let cells = vec![CellState::default(); 36 * 36];
        let view = PlacementView {
            side: 36,
            terrain: &[],
            zones: &[],
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
                        side: 1,
                        terrain: &tiles,
                        zones: &zones,
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
                side: 3,
                terrain: &terrain,
                zones: &zones,
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
        cells[0].entrance = Some(ObjectKind::parse(i32::try_from(raw::RESOURCE).unwrap()).unwrap());
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
            side: 2,
            terrain: &terrain,
            zones: &zones,
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
