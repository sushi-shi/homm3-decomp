//! Paired subterranean gates in overlapping zones on opposite levels.
//!
//! Project both zone bounds onto XY before scanning their intersection. Each
//! candidate must belong to its respective zone and fit the same gate art at
//! identical XY coordinates on both levels.
//! ```text
//!   source level:       P(x, y, source_z)
//!                       |
//!                       | same XY anchor and prototype
//!                       |
//!   destination level:  P(x, y, destination_z)
//! ```
//! Entrances likewise share XY; each approach is one cell south of its entrance.
use super::{
    connections::draw_connection_prototype, registration::entrance_position, BorderColor,
    BorderGuardCount, ConnectingZones, ConnectionError, ConnectionId, ObjectArena, PlacementError,
    PlacementMap,
};
use crate::{
    boundaries::{BoundaryZone, ZoneConnection},
    domain::{Terrain, WorldPosition},
    geometry::ZoneId,
    object::ObjectKind,
    prototype::{PreparedPrototype, PrototypeCatalog},
    raster::ZoneBounds,
    raw,
    rng::RetailRng,
    traits::CreatureCatalog,
};
use std::num::NonZeroU32;

impl ConnectingZones<'_, '_, '_> {
    /// Attempt paired gates, completing this record and its first reverse on success.
    /// The dispatcher skips water destinations; the native helper only checks source.
    ///
    /// # Errors
    /// Reports invalid context, art, arithmetic, placement or guard faults.
    /// A missing reverse faults after placement and completing this directed edge.
    pub fn try_gate_connection(
        &mut self,
        id: ConnectionId,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<bool, ConnectionError> {
        let connection = self.connection(id)?;
        let reverse = self.reverse_connection(connection);
        let map = self.map_mut();
        map.prepare_object_context(objects, catalog)?;
        if !map.create_gate_connection(connection, objects, catalog, creatures, rng)? {
            return Ok(false);
        }
        self.complete_bidirectional(id, connection, reverse)?;
        Ok(true)
    }
}
impl PlacementMap<'_, '_, '_> {
    fn create_gate_connection(
        &mut self,
        connection: ZoneConnection,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<bool, ConnectionError> {
        let source = self.zone(connection.source)?;
        let destination = self.zone(connection.destination)?;
        if source.position().level == destination.position().level
            || source.terrain() == Terrain::Water
        {
            return Ok(false);
        }
        // Gate sites lie where the two zones' bounds overlap.
        let Some(overlap) = source
            .bounds()
            .zip(destination.bounds())
            .and_then(|(a, b)| a.intersection(b))
        else {
            return Ok(false);
        };
        let family = ObjectKind::parse(i32::try_from(raw::UNDERGROUND_GATE).unwrap()).unwrap();
        let prototype = draw_connection_prototype(catalog, family, rng)?;
        let entry = catalog.get(prototype).unwrap();
        self.collect_gate_candidates(source, destination, overlap, entry)?;
        let Some(count) = NonZeroU32::new(
            u32::try_from(self.connections.candidates.len())
                .map_err(|_| PlacementError::Arithmetic)?,
        ) else {
            return Ok(false);
        };
        // Retail 0x542387 draws and loads the candidate before allocation at
        // 0x5423ad, despite the authored new-expression argument to the helper.
        let position = self.connections.candidates[rng.below(count) as usize];
        let object = objects.create(catalog, prototype)?;
        self.register_object(objects, catalog, object, position)?;
        let other = WorldPosition {
            level: destination.position().level,
            ..position
        };
        let other_object = objects.create(catalog, prototype)?;
        self.register_object(objects, catalog, other_object, other)?;
        let entrance = entrance_position(entry.prototype(), position)?;
        let other_entrance = WorldPosition {
            level: other.level,
            ..entrance
        };
        self.connections
            .crossing
            .append_entrance(source.id(), entrance.point)?;
        self.connections
            .crossing
            .append_entrance(destination.id(), other_entrance.point)?;
        let mut guard_value = self.connection_guard_value(connection)?;
        let approach = self.open_entrance_approach(entrance)?;
        let other_approach = self.open_entrance_approach(other_entrance)?;
        if connection.border_guard {
            // Success on either side suppresses both guards; a failed placement
            // does not undo the other side's objects. Always attempt both sides.
            for (at, tent_zone) in [(approach, destination.id()), (other_approach, source.id())] {
                if self.place_gate_border_guard(at, tent_zone, objects, catalog, rng)? {
                    guard_value = 0;
                }
            }
        }
        if guard_value > 0 {
            self.place_guard(guard_value, approach, objects, catalog, creatures, rng)?;
            self.place_guard(
                guard_value,
                other_approach,
                objects,
                catalog,
                creatures,
                rng,
            )?;
        }
        Ok(true)
    }
    fn collect_gate_candidates(
        &mut self,
        source: BoundaryZone,
        destination: BoundaryZone,
        overlap: ZoneBounds,
        entry: &PreparedPrototype<'_>,
    ) -> Result<(), PlacementError> {
        self.connections.candidates.clear();
        let mut best = 0_u32;
        for y in overlap.minimum().y..overlap.maximum().y {
            for x in overlap.minimum().x..overlap.maximum().x {
                let position = WorldPosition {
                    point: crate::geometry::Point::new(x, y),
                    level: source.position().level,
                };
                let index = self.view().native_index(position)?;
                if self.coverage().map().raster().cells()[index].zone != Some(source.id()) {
                    continue;
                }
                let other = WorldPosition {
                    level: destination.position().level,
                    ..position
                };
                let other_index = self.view().native_index(other)?;
                if self.coverage().map().raster().cells()[other_index].zone
                    != Some(destination.id())
                {
                    continue;
                }
                // Each native cell distance fits u16; their signed native sum
                // therefore fits i32 without widening away an overflow fault.
                let score = u32::from(self.cells[index].object_distance)
                    + u32::from(self.cells[other_index].object_distance);
                if score < best
                    || !self.object_fits(entry, position, source)?
                    || !self.object_fits(entry, other, destination)?
                {
                    continue;
                }
                if score > best {
                    best = score;
                    self.connections.candidates.clear();
                }
                self.connections.candidates.try_reserve(1)?;
                self.connections.candidates.push(position);
            }
        }
        Ok(())
    }
    fn place_gate_border_guard(
        &mut self,
        approach: WorldPosition,
        tent_zone: ZoneId,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<bool, PlacementError> {
        let Some(color) = self
            .place_border_guard(
                approach,
                BorderGuardCount::Single,
                tent_zone,
                objects,
                catalog,
                rng,
            )?
            .reported_color()
        else {
            return Ok(false);
        };
        let color = BorderColor::from_subtype(color);
        let mut side = approach;
        side.point.x = side
            .point
            .x
            .checked_sub(1)
            .ok_or(PlacementError::CoordinateOverflow)?;
        self.mark_empty_border(side, color)?;
        side.point.x = side
            .point
            .x
            .checked_add(2)
            .ok_or(PlacementError::CoordinateOverflow)?;
        self.mark_empty_border(side, color)?;
        Ok(true)
    }
}
