//! Reusable local treasure geometry and exclusive arena ownership.
use super::{
    mutation::{apply_insertion, prepare_insertion},
    CellState, Direction, Memberships, ObjectArena, ObjectId, ObstacleEntrances, PathReservation,
    PendingTreasure, PlacementError, PlacementSurface, PlacementView,
};
use crate::{
    domain::{Level, WorldPosition},
    geometry::Point,
    identity::OwnerId,
    prototype::{
        advance_outline, outline_probe, OutlineError, PreparedPrototype, PrototypeCatalog,
    },
    raster::ZoneBounds,
    raw,
    rng::RetailRng,
};

const SIDE: usize = raw::RMG_TREASURE_GROUP_MAP_SIZE as usize;
const CELLS: usize = SIDE * SIDE;
const OUTLINE_STATES: usize = (SIDE + 2) * (SIDE + 2) * raw::RMG_CARDINAL_DIRECTION_COUNT as usize;

#[derive(Debug)]
pub(super) enum GroupObject {
    Treasure(PendingTreasure),
    Guard(ObjectId),
}
impl GroupObject {
    pub(super) fn object(&self) -> ObjectId {
        match self {
            Self::Treasure(pending) => pending.object(),
            Self::Guard(id) => *id,
        }
    }
    pub(super) fn pending(&self) -> Option<&PendingTreasure> {
        match self {
            Self::Treasure(pending) => Some(pending),
            Self::Guard(_) => None,
        }
    }
}

/// Scratch for one treasure group, retained between attempts. Nested completion
/// groups use a separate workspace while the outer group's ordered objects live.
///
/// The scratch surface is dirt, has no zone, and starts with open path clearance.
/// Anchors are local to its one surface plane; committing will translate them.
///
/// ```text
///       x=0                    15
/// y=0   +-----------------------+
///       |   object mask         |
///       |     # # #             |    local anchor P
///       |     # # P             |    footprint grows north and west
/// y=15  +-----------------------+
/// ```
///
/// Only cells have fixed storage. Candidate entries retain duplicates from
/// different entrances, so their reusable vector is not limited to 256 entries.
#[derive(Debug)]
pub struct TreasureGroupWorkspace {
    pub(super) cells: [CellState; CELLS],
    pub(super) memberships: Memberships,
    pub(super) objects: Vec<GroupObject>,
    pub(super) candidates: Vec<Point>,
    pub(super) outline: Vec<Point>,
    pub(super) outline_marks: [bool; CELLS],
    bounds: Option<ZoneBounds>,
    pub(super) guard_entrance: Option<Point>,
    pub(super) owner: Option<OwnerId>,
}
impl Default for TreasureGroupWorkspace {
    #[expect(
        clippy::large_stack_arrays,
        reason = "one fixed 16x16 workspace avoids a separate cell allocation; nested groups use separate workspaces"
    )]
    fn default() -> Self {
        Self {
            cells: [CellState::default(); CELLS],
            memberships: Memberships::default(),
            objects: Vec::new(),
            candidates: Vec::new(),
            outline: Vec::new(),
            outline_marks: [false; CELLS],
            bounds: None,
            guard_entrance: None,
            owner: None,
        }
    }
}
impl TreasureGroupWorkspace {
    pub(super) fn view(&self) -> PlacementView<'_> {
        PlacementView {
            side: SIDE,
            surface: PlacementSurface::Group,
            cells: &self.cells,
        }
    }
    /// Objects in insertion order, without transferring their group ownership.
    pub fn objects(&self) -> impl Iterator<Item = ObjectId> + '_ {
        self.objects.iter().map(GroupObject::object)
    }
    /// Last computed nonempty bounds. Reset preserves these native cached bounds.
    #[must_use]
    pub const fn bounds(&self) -> Option<ZoneBounds> {
        self.bounds
    }
    /// Cached clockwise outline; repeated points are significant.
    #[must_use]
    pub fn outline(&self) -> &[Point] {
        &self.outline
    }
    /// Read one admitted local cell, without exposing mutable membership state.
    ///
    /// # Errors
    /// Rejects coordinates outside the scratch map.
    pub fn cell(&self, point: Point) -> Result<&CellState, PlacementError> {
        Ok(&self.cells[self.view().index(local(point))?])
    }
    /// Whether placement preparation marked this local cell as outline.
    ///
    /// # Errors
    /// Rejects coordinates outside the scratch map.
    pub fn is_outline(&self, point: Point) -> Result<bool, PlacementError> {
        Ok(self.outline_marks[self.view().index(local(point))?])
    }
    /// Object identities covering a local cell, in insertion order.
    ///
    /// # Errors
    /// Rejects coordinates outside the scratch map.
    pub fn objects_at(
        &self,
        point: Point,
    ) -> Result<impl Iterator<Item = ObjectId> + '_, PlacementError> {
        Ok(self.memberships.iter(self.cell(point)?.objects))
    }
    pub(super) fn add(
        &mut self,
        pending: PendingTreasure,
        anchor: Point,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
    ) -> Result<(), (PlacementError, PendingTreasure)> {
        match self.prepare_add(pending.object(), anchor, objects, catalog) {
            Ok(touched) => {
                let object = pending.object();
                // Native ordering: append the owned object before populating cells.
                self.objects.push(GroupObject::Treasure(pending));
                apply_insertion(
                    &mut self.cells,
                    &mut self.memberships,
                    objects,
                    object,
                    local(anchor),
                    &touched,
                );
                Ok(())
            }
            Err(error) => Err((error, pending)),
        }
    }
    pub(super) fn prepare_add(
        &mut self,
        object: ObjectId,
        anchor: Point,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
    ) -> Result<super::mutation::Footprint, PlacementError> {
        self.objects.try_reserve(1)?;
        let touched = prepare_insertion(
            SIDE,
            &self.cells,
            &mut self.memberships,
            objects,
            catalog,
            object,
            local(anchor),
        )?;
        let owner = match self.owner {
            Some(owner) => owner,
            None => OwnerId::new().ok_or(PlacementError::IdentityExhausted)?,
        };
        objects.claim_group(object, owner)?;
        self.owner = Some(owner);
        Ok(touched)
    }
    pub(super) fn centered(entry: &PreparedPrototype<'_>) -> Result<Point, PlacementError> {
        let size = entry.image_mask().size()?;
        // Both addends are admitted small unsigned dimensions.
        Ok(Point::new(
            (i32::try_from(SIDE).unwrap() + i32::from(size.width())) / 2,
            (i32::try_from(SIDE).unwrap() + i32::from(size.height())) / 2,
        ))
    }
    pub(super) fn choose_fit(
        &mut self,
        entry: &PreparedPrototype<'_>,
        objects: &ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<Option<Point>, PlacementError> {
        let size = entry.image_mask().size()?;
        let trigger = super::registration::trigger_offset(entry.prototype());
        self.candidates.clear();
        for pending in &self.objects {
            let object = pending.object();
            let geometry = objects
                .get(object)
                .ok_or(PlacementError::UnknownObject(object))?;
            let entrance = objects
                .entrance(catalog, object)?
                .ok_or(PlacementError::UnknownObject(object))?;
            for direction in Direction::ALL.into_iter().rev() {
                if !geometry.kind().traits().enterable_from_north()
                    && !matches!(
                        direction,
                        Direction::SouthWest | Direction::South | Direction::SouthEast
                    )
                {
                    continue;
                }
                let candidate = entrance
                    .point
                    .checked_add(direction.offset())
                    .and_then(|point| point.checked_add(trigger))
                    .ok_or(PlacementError::CoordinateOverflow)?;
                if candidate.x >= i32::from(size.width()) + 2
                    && candidate.y >= i32::from(size.height()) + 2
                    && candidate.x < i32::try_from(SIDE).unwrap() - 3
                    && candidate.y < i32::try_from(SIDE).unwrap() - 3
                    && self.can_fit(entry, candidate)?
                {
                    self.candidates.try_reserve(1)?;
                    self.candidates.push(candidate);
                }
            }
        }
        if self.candidates.is_empty() {
            return Ok(None);
        }
        Ok(Some(
            self.candidates[rng.draw() as usize % self.candidates.len()],
        ))
    }
    pub(super) fn can_fit(
        &self,
        entry: &PreparedPrototype<'_>,
        anchor: Point,
    ) -> Result<bool, PlacementError> {
        let view = self.view();
        let kind = entry.prototype().kind();
        let trigger = super::registration::trigger_offset(entry.prototype());
        let entrance = anchor
            .checked_add(Point::new(-trigger.x, -trigger.y))
            .ok_or(PlacementError::CoordinateOverflow)?;
        let neighbor = |direction: Direction| -> Result<usize, PlacementError> {
            let point = entrance
                .checked_add(direction.offset())
                .ok_or(PlacementError::CoordinateOverflow)?;
            view.native_index(local(point))
        };
        // These native reads precede the footprint's whole-bounds check.
        if !kind.traits().enterable_from_north() {
            for direction in &Direction::ALL[raw::RMG_FIRST_NORTHERN_DIRECTION as usize..] {
                if self.cells[neighbor(*direction)?].entrance.is_some() {
                    return Ok(false);
                }
            }
        }
        for direction in &Direction::ALL[..raw::RMG_FIRST_NORTHERN_DIRECTION as usize] {
            if self.cells[neighbor(*direction)?]
                .entrance
                .is_some_and(|kind| {
                    let traits = kind.traits();
                    !(traits.cleared_on_visit() && traits.enterable_from_north())
                })
            {
                return Ok(false);
            }
        }
        let guard =
            kind.index() == raw::MONSTER as usize || kind.index() == raw::BORDER_GUARD as usize;
        let obstacles = if guard {
            ObstacleEntrances::Allow
        } else {
            ObstacleEntrances::Reject
        };
        if view.footprint_blocked(entry, local(anchor), None, obstacles)? {
            return Ok(false);
        }
        if !guard {
            return Ok(true);
        }
        for direction in Direction::ALL {
            let index = neighbor(direction)?;
            if self.cells[index].entrance.is_none()
                && view.passable(index)
                && self.cells[index].reservation != PathReservation::Obstacle
            {
                return Ok(true);
            }
        }
        Ok(false)
    }
    pub(super) fn clear_outline_cell(&self, index: usize) -> bool {
        self.view().clear_outline_cell(index)
    }
    pub(super) fn update_bounds(&mut self) {
        self.bounds = None;
        for index in 0..CELLS {
            if !self.clear_outline_cell(index) {
                let point = Point::new(
                    i32::try_from(index % SIDE).unwrap(),
                    i32::try_from(index / SIDE).unwrap(),
                );
                match &mut self.bounds {
                    Some(bounds) => bounds.include(point),
                    bounds => *bounds = Some(ZoneBounds::cell(point)),
                }
            }
        }
    }
    pub(super) fn trace_outline(&mut self) -> Result<(), PlacementError> {
        if !self.outline.is_empty() {
            return Ok(());
        }
        let Some(index) = (0..CELLS).find(|&index| !self.clear_outline_cell(index)) else {
            return Ok(());
        };
        // Signed points admit the one-cell halo. Start immediately north of the
        // first occupied/reserved cell; unlike prototype outlines this is row-first.
        let start = Point::new(
            i32::try_from(index % SIDE).unwrap(),
            i32::try_from(index / SIDE).unwrap() - 1,
        );
        let mut position = start;
        let mut direction = raw::RMG_DIRECTION_SOUTH as usize;
        for _ in 0..OUTLINE_STATES {
            self.outline.try_reserve(1)?;
            self.outline.push(position);
            for _ in 0..raw::RMG_CARDINAL_DIRECTION_COUNT {
                let nearby = outline_probe(position, &mut direction);
                if !self.view().contains(nearby)
                    || self.clear_outline_cell(self.view().index(local(nearby))?)
                {
                    break;
                }
            }
            advance_outline(&mut position, &mut direction);
            if position == start {
                return Ok(());
            }
        }
        Err(OutlineError::NonTerminating.into())
    }
    /// Compute bounds, trace the cached outline, then mark native flat cells.
    ///
    /// # Errors
    /// Reports a nonterminating walk, failed storage growth or out-of-allocation
    /// outline mark. Earlier marks remain applied when native would fault.
    pub fn prepare_placement(&mut self) -> Result<(), PlacementError> {
        self.update_bounds();
        self.trace_outline()?;
        for &point in &self.outline {
            let index = self.view().native_index(local(point))?;
            self.outline_marks[index] = true;
        }
        Ok(())
    }
    pub(super) fn clear_memberships(&mut self) {
        self.memberships.reset();
        self.cells.fill(CellState::default());
    }
    pub(super) fn reset_after_disposal(&mut self) {
        debug_assert!(self.objects.is_empty());
        self.clear_memberships();
        self.outline.clear();
        self.outline_marks.fill(false);
        self.candidates.clear();
        self.owner = None;
        self.guard_entrance = None;
        // Native reset does not write the cached bounds.
    }
}
pub(super) fn local(point: Point) -> WorldPosition {
    WorldPosition {
        point,
        level: Level::Surface,
    }
}

#[cfg(test)]
mod tests;
