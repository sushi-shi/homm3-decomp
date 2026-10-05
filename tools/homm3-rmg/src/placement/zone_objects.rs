//! Reusable random fitting-site placement for tents and portals.
use super::{ObjectArena, ObjectId, PlacementError, PlacementMap, PlacementView};
use crate::{
    domain::WorldPosition,
    geometry::{Point, ZoneId},
    prototype::{OutlineWorkspace, PrototypeCatalog},
    rng::RetailRng,
};
use std::num::NonZeroU32;

#[derive(Default, Debug)]
pub(super) struct ZonePlacementScratch {
    // Kept separate from connection candidates: placing a tent may occur while
    // a crossing still owns its frozen list of candidate sites.
    pub(super) candidates: Vec<WorldPosition>,
    outline: OutlineWorkspace,
}
impl ZonePlacementScratch {
    pub(super) fn reset(&mut self) {
        self.candidates.clear();
    }
}
impl PlacementMap<'_, '_, '_> {
    /// Register an existing object at a random fitting anchor in this zone.
    /// Candidates retain native row order, including the singleton RNG draw.
    /// No fitting site leaves the object unchanged; a new object may be discarded.
    ///
    /// # Errors
    /// Reports a foreign object/catalog, unknown zone, allocation failure or
    /// unsafe native geometry. Failure to find a fitting site returns `false`.
    pub fn place_object_in_zone(
        &mut self,
        object: ObjectId,
        zone: ZoneId,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<bool, PlacementError> {
        self.prepare_object_context(objects, catalog)?;
        let geometry = objects
            .get(object)
            .ok_or(PlacementError::UnknownObject(object))?;
        let entry = catalog
            .get(geometry.prototype())
            .ok_or(PlacementError::UnknownPrototype(geometry.prototype()))?;
        let zone = self.zone(zone)?;
        let Some(bounds) = zone.bounds() else {
            return Ok(false);
        };
        let size = entry.image_mask().size()?;
        let minimum = Point::new(
            bounds
                .minimum()
                .x
                .checked_add(i32::from(size.width()) - 1)
                .ok_or(PlacementError::CoordinateOverflow)?,
            bounds
                .minimum()
                .y
                .checked_add(i32::from(size.height()) - 1)
                .ok_or(PlacementError::CoordinateOverflow)?,
        );
        let map = self.terrain.coverage().map();
        let view = PlacementView {
            layout: map.raster().layout(),
            surface: super::PlacementSurface::World {
                terrain: self.terrain.tiles(),
                zones: map.raster().cells(),
            },
            cells: self.cells,
        };
        self.zone_placement.candidates.clear();
        for y in minimum.y..bounds.maximum().y {
            for x in minimum.x..bounds.maximum().x {
                let position = WorldPosition {
                    point: Point::new(x, y),
                    level: zone.position().level,
                };
                let index = view.native_index(position)?;
                if view.zone(index) == Some(zone.id())
                    && view.can_place(
                        entry,
                        position,
                        zone.id(),
                        zone.terrain(),
                        &mut self.zone_placement.outline,
                    )?
                {
                    self.zone_placement.candidates.try_reserve(1)?;
                    self.zone_placement.candidates.push(position);
                }
            }
        }
        let count = u32::try_from(self.zone_placement.candidates.len())
            .map_err(|_| PlacementError::Arithmetic)?;
        let Some(count) = NonZeroU32::new(count) else {
            return Ok(false);
        };
        let position = self.zone_placement.candidates[rng.below(count) as usize];
        self.register_object(objects, catalog, object, position)?;
        Ok(true)
    }
}
