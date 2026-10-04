//! Native cell transitions and clipped footprint insertion/removal.

use super::{
    CellState, Memberships, ObjectArena, ObjectId, PathReservation, PlacementError, PlacementMap,
    PlacementView,
};
use crate::{
    domain::WorldPosition,
    geometry::Point,
    object::ObjectKind,
    prototype::{PreparedPrototype, PrototypeCatalog},
    raw,
};

/// A value representable by the native border-connection colour field.
/// This storage domain is wider than the selectable key-tent palette.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct BorderColor(u8);
impl BorderColor {
    /// Native bitfield assignment truncates the reported prototype subtype.
    pub(super) fn from_subtype(subtype: i32) -> Self {
        let mask = (1_i32 << raw::RMG_BORDER_COLOR_BITS) - 1;
        Self(u8::try_from(subtype & mask).expect("border field fits a byte"))
    }
    /// Admit a stored colour without truncating an external value.
    #[must_use]
    pub const fn parse(value: u8) -> Option<Self> {
        if (value as u32) < (1 << raw::RMG_BORDER_COLOR_BITS) {
            Some(Self(value))
        } else {
            None
        }
    }
    /// Native stored colour value.
    #[must_use]
    pub const fn value(self) -> u8 {
        self.0
    }
}

impl CellState {
    /// Whether no object has cleared the passability flag; rock is checked separately.
    #[must_use]
    pub const fn passable(&self) -> bool {
        self.passable
    }
    /// First object's kind when this cell is flagged as an entrance.
    #[must_use]
    pub const fn entrance(&self) -> Option<ObjectKind> {
        self.entrance
    }
    /// Native path/obstacle reservation state.
    #[must_use]
    pub const fn reservation(&self) -> PathReservation {
        self.reservation
    }
    /// Pending border connection and its stored colour.
    #[must_use]
    pub const fn border(&self) -> Option<BorderColor> {
        self.border
    }
    /// Current distance from a generated object's entrance.
    #[must_use]
    pub const fn object_distance(&self) -> u16 {
        self.object_distance
    }
    pub(super) fn open_path(&mut self) {
        if self.border.is_none() {
            self.reservation = PathReservation::Open;
        }
    }
    pub(super) fn mark_obstacle(&mut self) {
        if self.border.is_none() {
            self.reservation = PathReservation::Obstacle;
        }
    }
    pub(super) fn clear_obstacle(&mut self) {
        if self.border.is_none() && self.reservation == PathReservation::Obstacle {
            self.reservation = PathReservation::Unreserved;
        }
    }
    pub(super) fn release_path(&mut self) {
        if self.border.is_none() && self.reservation == PathReservation::Open {
            self.reservation = PathReservation::Unreserved;
        }
    }
    pub(super) fn mark_border(&mut self, color: BorderColor) {
        self.mark_obstacle();
        self.border = Some(color);
    }
    pub(super) fn clear_border(&mut self) {
        self.border = None;
        self.open_path();
    }
}

#[derive(Clone, Copy)]
struct TouchedCell {
    index: usize,
    trigger: bool,
    first_kind: Option<ObjectKind>,
}
pub(super) struct Footprint {
    cells: [TouchedCell; raw::OBJECT_MASK_CELLS as usize],
    len: usize,
}
impl PlacementMap<'_, '_, '_> {
    /// Read a cell's placement flags without exposing mutable membership metadata.
    ///
    /// # Errors
    /// Reports a coordinate or plane outside the map.
    pub fn cell(&self, position: WorldPosition) -> Result<&CellState, PlacementError> {
        Ok(&self.cells[self.view().index(position)?])
    }
    /// Object identities in native insertion order, borrowing shared link storage.
    ///
    /// # Errors
    /// Reports a coordinate or plane outside the map.
    pub fn objects_at(
        &self,
        position: WorldPosition,
    ) -> Result<impl Iterator<Item = ObjectId> + '_, PlacementError> {
        let chain = self.cells[self.view().index(position)?].objects;
        Ok(self.memberships.iter(chain))
    }
    /// Open a path unless a border connection protects the cell.
    ///
    /// # Errors
    /// Reports a coordinate or plane outside the map.
    pub fn open_path(&mut self, position: WorldPosition) -> Result<(), PlacementError> {
        let index = self.view().index(position)?;
        self.cells[index].open_path();
        Ok(())
    }
    /// Mark obstacles unless a border connection protects the cell.
    ///
    /// # Errors
    /// Reports a coordinate or plane outside the map.
    pub fn mark_obstacle(&mut self, position: WorldPosition) -> Result<(), PlacementError> {
        let index = self.view().index(position)?;
        self.cells[index].mark_obstacle();
        Ok(())
    }
    /// Clear an obstacle mark while retaining any existing path clearance.
    ///
    /// # Errors
    /// Reports a coordinate or plane outside the map.
    pub fn clear_obstacle(&mut self, position: WorldPosition) -> Result<(), PlacementError> {
        let index = self.view().index(position)?;
        self.cells[index].clear_obstacle();
        Ok(())
    }
    /// Release path clearance unless a border connection protects the cell.
    ///
    /// # Errors
    /// Reports a coordinate or plane outside the map.
    pub fn release_path(&mut self, position: WorldPosition) -> Result<(), PlacementError> {
        let index = self.view().index(position)?;
        self.cells[index].release_path();
        Ok(())
    }
    /// Mark a border connection; an existing connection retains its tile flags.
    ///
    /// # Errors
    /// Reports a coordinate or plane outside the map.
    pub fn mark_border(
        &mut self,
        position: WorldPosition,
        color: BorderColor,
    ) -> Result<(), PlacementError> {
        let index = self.view().index(position)?;
        self.cells[index].mark_border(color);
        Ok(())
    }
    /// Remove a border connection and open its cell.
    ///
    /// # Errors
    /// Reports a coordinate or plane outside the map.
    pub fn clear_border(&mut self, position: WorldPosition) -> Result<(), PlacementError> {
        let index = self.view().index(position)?;
        self.cells[index].clear_border();
        Ok(())
    }

    /// Apply the clipped native footprint and update the object's anchor.
    /// Trigger cells open paths; other occupied cells clear passability. Both
    /// append membership in order. Generator counts and distance flooding are
    /// separate operations, just as in the source map versus generator layers.
    ///
    /// # Errors
    /// Reports foreign IDs, unsafe geometry, or failed membership reservation.
    /// All required storage is reserved before changing cells or the anchor.
    pub fn insert_object(
        &mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        object: ObjectId,
        anchor: WorldPosition,
    ) -> Result<(), PlacementError> {
        self.prepare_registration(catalog)?;
        objects.require_world_insertion(object)?;
        let touched = prepare_insertion(
            self.view().side,
            self.cells,
            self.memberships,
            objects,
            catalog,
            object,
            anchor,
        )?;
        objects.publish(object);
        apply_insertion(
            self.cells,
            self.memberships,
            objects,
            object,
            anchor,
            &touched,
        );
        Ok(())
    }

    /// Erase one occurrence from each occupied footprint cell while keeping the
    /// object and its anchor alive. Remaining overlaps retain native flags;
    /// only an empty cell clears its entrance and restores passability.
    /// This is the cell portion of generator removal, without global counters.
    ///
    /// # Errors
    /// Reports foreign objects, unsafe geometry, or retail's end-iterator
    /// erase fault. Earlier cell erasures remain applied if that fault is reached.
    pub fn erase_footprint(
        &mut self,
        objects: &ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        object: ObjectId,
    ) -> Result<(), PlacementError> {
        self.prepare_registration(catalog)?;
        let geometry = objects
            .get(object)
            .ok_or(PlacementError::UnknownObject(object))?;
        if !self.memberships.accepts(object) {
            return Err(PlacementError::UnknownObject(object));
        }
        let entry = catalog
            .get(geometry.prototype())
            .ok_or(PlacementError::UnknownPrototype(geometry.prototype()))?;
        // Native construction assigns (-1,-1,-1). Every footprint row is
        // clipped before the missing plane can be accessed.
        let Some(anchor) = geometry.position() else {
            return Ok(());
        };
        let touched = self.view().footprint(entry, anchor)?;
        let hotfix = self.coverage().map().behavior().is_hotfix();
        for touched in &touched.cells[..touched.len] {
            let cell = &mut self.cells[touched.index];
            if self.memberships.remove(&mut cell.objects, object) {
                if let Some(first) = self.memberships.first(cell.objects) {
                    if cell.entrance.is_some() {
                        cell.entrance = Some(
                            objects
                                .get(first)
                                .ok_or(PlacementError::UnknownObject(first))?
                                .kind(),
                        );
                    }
                } else {
                    cell.entrance = None;
                    cell.passable = true;
                }
                cell.object_distance = super::CLEARED_DISTANCE;
            } else if !hotfix && cell.retail_membership_storage {
                return Err(PlacementError::MissingMembership(object));
            }
        }
        Ok(())
    }
}

impl PlacementView<'_> {
    pub(super) fn footprint(
        &self,
        entry: &PreparedPrototype<'_>,
        anchor: WorldPosition,
    ) -> Result<Footprint, PlacementError> {
        let size = entry.image_mask().size()?;
        let mut result = Footprint {
            cells: [TouchedCell {
                index: 0,
                trigger: false,
                first_kind: None,
            }; raw::OBJECT_MASK_CELLS as usize],
            len: 0,
        };
        for cell in size.cells() {
            if !entry.prototype().occupies(cell) {
                continue;
            }
            // Native code clips an entire row before evaluating any X offset.
            let y = anchor
                .point
                .y
                .checked_sub(i32::from(cell.y()))
                .ok_or(PlacementError::CoordinateOverflow)?;
            if !usize::try_from(y).is_ok_and(|y| y < self.side) {
                continue;
            }
            let x = anchor
                .point
                .x
                .checked_sub(i32::from(cell.x()))
                .ok_or(PlacementError::CoordinateOverflow)?;
            let point = Point::new(x, y);
            if !self.contains(point) {
                continue;
            }
            let index = self.index(WorldPosition {
                point,
                level: anchor.level,
            })?;
            result.cells[result.len] = TouchedCell {
                index,
                trigger: entry.prototype().is_trigger(cell),
                first_kind: None,
            };
            result.len += 1;
        }
        Ok(result)
    }
}
pub(super) fn prepare_insertion(
    side: usize,
    cells: &[CellState],
    memberships: &mut Memberships,
    objects: &ObjectArena,
    catalog: &PrototypeCatalog<'_>,
    object: ObjectId,
    anchor: WorldPosition,
) -> Result<Footprint, PlacementError> {
    let geometry = *objects
        .get(object)
        .ok_or(PlacementError::UnknownObject(object))?;
    if !memberships.accepts(object) {
        return Err(PlacementError::UnknownObject(object));
    }
    let entry = catalog
        .get(geometry.prototype())
        .ok_or(PlacementError::UnknownPrototype(geometry.prototype()))?;
    let view = PlacementView {
        side,
        surface: super::PlacementSurface::Group,
        cells,
    };
    let mut touched = view.footprint(entry, anchor)?;
    for cell in &mut touched.cells[..touched.len] {
        cell.first_kind = Some(match memberships.first(cells[cell.index].objects) {
            Some(first) => objects
                .get(first)
                .ok_or(PlacementError::UnknownObject(first))?
                .kind(),
            None => geometry.kind(),
        });
    }
    memberships.reserve(touched.len)?;
    Ok(touched)
}
pub(super) fn apply_insertion(
    cells: &mut [CellState],
    memberships: &mut Memberships,
    objects: &mut ObjectArena,
    object: ObjectId,
    anchor: WorldPosition,
    touched: &Footprint,
) {
    // Even a completely clipped footprint binds this map's object arena.
    memberships.bind(object);
    objects.set_position(object, anchor);
    for touched in &touched.cells[..touched.len] {
        let cell = &mut cells[touched.index];
        if touched.trigger {
            cell.entrance = touched.first_kind;
            cell.open_path();
        } else {
            cell.passable = false;
        }
        memberships.append(&mut cell.objects, object);
        cell.retail_membership_storage = true;
    }
}
