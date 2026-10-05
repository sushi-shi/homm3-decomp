//! World registration precedes floor transfer and ordered completion callbacks.
use super::{
    offset_position, ObjectArena, PathReservation, PlacementError, TreasureGeneration,
    TreasureGenerationError, TreasureGroupWorkspace,
};
use crate::{
    domain::{Terrain, WorldPosition},
    geometry::{Point, ZoneId},
    raw,
    rng::RetailRng,
};

impl TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
    /// Select a site, publish every group root, copy floor reservations, then
    /// complete roots in insertion order. Successful commit empties scratch
    /// references without deleting objects now owned by the world.
    ///
    /// # Errors
    /// Reports native geometry, registration or completion faults. Earlier
    /// mutations remain applied; random draws and object IDs are not rewound.
    pub fn place_group(
        &mut self,
        group: &mut TreasureGroupWorkspace,
        zone: ZoneId,
        spacing: i32,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<bool, TreasureGenerationError> {
        let Some(origin) = self.find_group_site(group, zone, spacing, objects, rng)? else {
            return Ok(false);
        };
        self.commit_group(group, origin, objects, rng)?;
        Ok(true)
    }
    fn commit_group(
        &mut self,
        group: &mut TreasureGroupWorkspace,
        origin: WorldPosition,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<(), TreasureGenerationError> {
        let owner = group.owner.ok_or(PlacementError::EmptyOutline)?;
        for id in group.objects() {
            objects.require_group(id, owner)?;
        }
        for id in group.objects() {
            let anchor = objects.positioned(id)?.position();
            let position = offset_position(origin, anchor.point)?;
            objects.publish(id);
            self.ready.paths.map_mut().register_object(
                objects,
                self.ready.catalog.prototypes(),
                id,
                position,
            )?;
        }
        // From here on reset only forgets references, even if a callback removes
        // or replaces a root. No group disposal may reclaim published records.
        group.owner = None;
        self.transfer_group_floor(group, origin)?;
        for index in 0..group.objects.len() {
            self.complete_treasure(group.objects[index].object(), objects, rng)?;
        }
        group.objects.clear();
        group.reset_after_disposal();
        Ok(())
    }
    fn transfer_group_floor(
        &mut self,
        group: &mut TreasureGroupWorkspace,
        origin: WorldPosition,
    ) -> Result<(), PlacementError> {
        let map = self.ready.paths.map_mut();
        let side = map.view().signed_side();
        let group_side = i32::try_from(raw::RMG_TREASURE_GROUP_MAP_SIZE).unwrap();
        let minimum = Point::new(
            origin
                .point
                .x
                .checked_neg()
                .ok_or(PlacementError::CoordinateOverflow)?
                .max(0),
            origin
                .point
                .y
                .checked_neg()
                .ok_or(PlacementError::CoordinateOverflow)?
                .max(0),
        );
        let maximum = Point::new(
            side.checked_sub(origin.point.x)
                .ok_or(PlacementError::CoordinateOverflow)?
                .min(group_side),
            side.checked_sub(origin.point.y)
                .ok_or(PlacementError::CoordinateOverflow)?
                .min(group_side),
        );
        for y in minimum.y..maximum.y {
            for x in minimum.x..maximum.x {
                let local = Point::new(x, y);
                let source = group.view().index(super::treasure_groups::local(local))?;
                let target = map.view().native_index(offset_position(origin, local)?)?;
                let old = map.cells[target].reservation;
                let can_block = group.cells[source].can_block_floor(group.view().terrain(source));
                if map.view().terrain(target) != Terrain::Water
                    && can_block
                    && map.view().passable(target)
                    && map.cells[target].entrance.is_none()
                {
                    map.cells[target].release_path();
                    if group.cells[source].reservation == PathReservation::Obstacle {
                        map.cells[target].mark_obstacle();
                    }
                }
                if old == PathReservation::Obstacle {
                    group.cells[source].mark_obstacle();
                } else {
                    group.cells[source].clear_obstacle();
                }
                if old == PathReservation::Open {
                    group.cells[source].open_path();
                } else {
                    group.cells[source].release_path();
                }
            }
        }
        Ok(())
    }
}
