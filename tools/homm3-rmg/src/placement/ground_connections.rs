//! Frozen border scans and per-record land crossing attempts.
use super::{
    BorderGuardCount, ConnectionError, ObjectArena, PathWidth, PlacementError, PlacementMap,
};
use crate::{
    boundaries::ZoneConnection,
    domain::{Terrain, WorldPosition},
    prototype::PrototypeCatalog,
    raw,
    rng::RetailRng,
    traits::CreatureCatalog,
};
use std::num::NonZeroU32;
impl PlacementMap<'_, '_, '_> {
    pub(super) fn collect_connection_borders(&mut self) -> Result<(), PlacementError> {
        for index in 0..self.cells.len() {
            if self.cells[index].zone_distance.connection().is_none()
                || self.terrain.tiles()[index].terrain() == Terrain::Water
                || !self.view().passable(index)
            {
                continue;
            }
            let position = self.position_at(index);
            let other = self.connection_neighbor(position)?;
            let other_index = self.view().native_index(other)?;
            let zones = self.coverage().map().raster().cells();
            if self.terrain.tiles()[other_index].terrain() != Terrain::Water
                && zones[other_index].zone != zones[index].zone
            {
                self.connections.crossing.borders.try_reserve(1)?;
                self.connections.crossing.borders.push(position);
            }
        }
        Ok(())
    }
    fn connection_neighbor(
        &self,
        position: WorldPosition,
    ) -> Result<WorldPosition, PlacementError> {
        let index = self.view().native_index(position)?;
        let offset = self.cells[index].zone_distance.direction().offset();
        Ok(WorldPosition {
            point: position
                .point
                .checked_add(offset)
                .ok_or(PlacementError::CoordinateOverflow)?,
            level: position.level,
        })
    }
    pub(super) fn create_ground_connection(
        &mut self,
        connection: ZoneConnection,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<bool, ConnectionError> {
        let source = self.zone(connection.source)?;
        let destination = self.zone(connection.destination)?;
        if source.position().level != destination.position().level
            || source.terrain() == Terrain::Water
            || destination.terrain() == Terrain::Water
        {
            return Ok(false);
        }
        self.connections.candidates.clear();
        let mut eligible = 0_u32;
        let mut best = raw::RMG_MAXIMUM_CROSSING_COST;
        for border in 0..self.connections.crossing.borders.len() {
            let position = self.connections.crossing.borders[border];
            let index = self.view().native_index(position)?;
            let cell = &self.cells[index];
            if self.coverage().map().raster().cells()[index].zone != Some(source.id())
                || cell.zone_distance.connection().map(|(zone, _)| zone) != Some(destination.id())
                || self.memberships.first(cell.objects).is_some()
            {
                continue;
            }
            let other = self.connection_neighbor(position)?;
            let other_index = self.view().native_index(other)?;
            if self
                .memberships
                .first(self.cells[other_index].objects)
                .is_some()
            {
                continue;
            }
            eligible = eligible.checked_add(1).ok_or(PlacementError::Arithmetic)?;
            let cost = u32::from(self.cells[index].zone_distance.cost());
            if cost <= best {
                if cost < best {
                    best = cost;
                    self.connections.candidates.clear();
                }
                self.connections.candidates.try_reserve(1)?;
                self.connections.candidates.push(position);
            }
        }
        if self.connections.candidates.is_empty() {
            return Ok(false);
        }
        let mut guard_value = self.connection_guard_value(connection)?;
        // Native dead path with ordinary floods, which assign crossing cost >=10.
        if best == 1 && guard_value == 0 && !connection.border_guard {
            return Ok(true);
        }
        let count = usize::try_from(eligible.div_ceil(raw::RMG_BORDER_CELLS_PER_CROSSING))
            .map_err(|_| PlacementError::Arithmetic)?
            .min(self.connections.candidates.len());
        for _ in 0..count {
            let remaining = u32::try_from(self.connections.candidates.len())
                .map_err(|_| PlacementError::Arithmetic)?;
            let selected = rng.below(
                NonZeroU32::new(remaining).expect("fixed crossing count never exceeds candidates"),
            ) as usize;
            let position = self.connections.candidates[selected];
            let other = self.connection_neighbor(position)?;
            let width = if connection.border_guard {
                PathWidth::Narrow
            } else {
                PathWidth::Wide
            };
            self.open_connection_path(position, width, objects, catalog, rng)?;
            self.connections
                .crossing
                .append_entrance(source.id(), position.point)?;
            self.open_connection_path(other, width, objects, catalog, rng)?;
            self.connections
                .crossing
                .append_entrance(destination.id(), other.point)?;
            self.connections.candidates.remove(selected);
            if connection.border_guard {
                for (at, tent_zone) in [(position, destination.id()), (other, source.id())] {
                    let result = self.place_border_guard(
                        at,
                        BorderGuardCount::Single,
                        tent_zone,
                        objects,
                        catalog,
                        rng,
                    )?;
                    if let Some(color) = result.reported_color() {
                        self.mark_border_connection_area(at, color)?;
                        guard_value = 0;
                    }
                }
            }
            if guard_value > 0 {
                let at = if rng.draw() & 1 == 0 { position } else { other };
                self.place_guard(guard_value, at, objects, catalog, creatures, rng)?;
            }
        }
        Ok(true)
    }
}
