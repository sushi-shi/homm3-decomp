//! Entrance reservations and the native guard fan on the local group map.
use super::{
    mutation::apply_insertion,
    registration::{entrance_position, trigger_offset},
    treasure_groups::{local, GroupObject},
    Direction, ObjectArena, ObjectId, PathReservation, PlacementError, TreasureGroupWorkspace,
};
use crate::{geometry::Point, prototype::PrototypeCatalog, raw, rng::RetailRng};

impl TreasureGroupWorkspace {
    pub(super) fn add_guard(
        &mut self,
        guard: ObjectId,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<bool, PlacementError> {
        self.trace_outline()?;
        let mut last_trigger = None;
        for index in 0..self.objects.len() {
            let object = self.objects[index].object();
            let object = objects.positioned(object)?;
            let entry = catalog
                .get(object.prototype())
                .ok_or(PlacementError::UnknownPrototype(object.prototype()))?;
            last_trigger = Some(trigger_offset(entry.prototype()));
            let entrance = entrance_position(entry.prototype(), object.position())?;
            let count = if object.kind().traits().enterable_from_north() {
                raw::RMG_DIRECTION_COUNT
            } else {
                raw::RMG_FIRST_NORTHERN_DIRECTION
            };
            for direction in Direction::ALL[..count as usize].iter().rev() {
                let point = entrance
                    .point
                    .checked_add(direction.offset())
                    .ok_or(PlacementError::CoordinateOverflow)?;
                let index = self.view().native_index(local(point))?;
                if self.cells[index].entrance.is_some() || !self.view().passable(index) {
                    continue;
                }
                self.cells[index].mark_obstacle();
                for x in point.x - 1..=point.x + 1 {
                    for y in point.y - 1..=point.y + 1 {
                        let index = self.view().native_index(local(Point::new(x, y)))?;
                        if self.view().passable(index) && self.cells[index].entrance.is_none() {
                            self.cells[index].release_path();
                        }
                    }
                }
            }
        }
        let geometry = objects
            .get(guard)
            .ok_or(PlacementError::UnknownObject(guard))?;
        let entry = catalog
            .get(geometry.prototype())
            .ok_or(PlacementError::UnknownPrototype(geometry.prototype()))?;
        for index in (0..self.outline.len()).rev() {
            let point = self.outline[index];
            let cell = self.view().native_index(local(point))?;
            if self.cells[cell].reservation != PathReservation::Obstacle
                || !self.can_fit(entry, point)?
            {
                self.outline.remove(index);
            }
        }
        if self.outline.is_empty() {
            return Ok(false);
        }
        let point = self.outline[rng.draw() as usize % self.outline.len()];
        let touched = self.prepare_add(guard, point, objects, catalog)?;
        self.objects.push(GroupObject::Direct(guard));
        apply_insertion(
            &mut self.cells,
            &mut self.memberships,
            objects,
            guard,
            local(point),
            &touched,
        );
        // Both modes retain the last TREASURE's trigger, not the guard's trigger.
        let trigger = last_trigger.ok_or(PlacementError::EmptyOutline)?;
        let entrance = point
            .checked_add(Point::new(-trigger.x, -trigger.y))
            .ok_or(PlacementError::CoordinateOverflow)?;
        self.open_guard_fan(
            entrance,
            entry.prototype().kind().index() == raw::BORDER_GUARD as usize,
        )?;
        self.guard_entrance = Some(entrance);
        self.outline.clear();
        self.update_bounds();
        self.trace_outline()?;
        Ok(true)
    }
    fn open_guard_fan(
        &mut self,
        entrance: Point,
        border_guard: bool,
    ) -> Result<(), PlacementError> {
        // North is up; G is the computed entrance (including the trigger bug).
        // Each number is the first-ring direction that opens that cell:
        //   5 5 6 7 7
        //   5 5 6 7 7
        //   4 4 G 0 0
        //   3 3 2 1 1
        //   3 3 2 1 1
        for direction in Direction::ALL {
            let point = entrance
                .checked_add(direction.offset())
                .ok_or(PlacementError::CoordinateOverflow)?;
            let index = self.view().native_index(local(point))?;
            if !self.view().passable(index)
                || (border_guard && self.cells[index].reservation == PathReservation::Obstacle)
            {
                continue;
            }
            self.cells[index].open_path();
            let diagonal = direction.index() % 2 != 0;
            let start = if diagonal {
                (direction.index() + 7) % 8
            } else {
                direction.index()
            };
            for step in 0..if diagonal { 3 } else { 1 } {
                let nearby = point
                    .checked_add(Direction::ALL[(start + step) % 8].offset())
                    .ok_or(PlacementError::CoordinateOverflow)?;
                if !self.view().contains(nearby) {
                    continue;
                }
                let index = self.view().index(local(nearby))?;
                if self.cells[index].reservation == PathReservation::Unreserved
                    && self.view().passable(index)
                {
                    self.cells[index].open_path();
                }
            }
        }
        Ok(())
    }
}
