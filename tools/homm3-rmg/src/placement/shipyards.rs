//! Shipyard placement and ordered water reachability for zone connections.
use super::connections::draw_connection_prototype;
use super::{
    offset_position, registration::entrance_position, BorderGuardCount, CellState, ConnectingZones,
    ConnectionError, ConnectionId, Direction, ObjectArena, PathReservation, PlacementError,
    PlacementMap,
};
use crate::{
    boundaries::ZoneConnection,
    domain::{Level, Terrain, WorldPosition},
    geometry::{Point, ZoneId},
    object::ObjectKind,
    prototype::{PreparedPrototype, PrototypeCatalog},
    raw,
    rng::RetailRng,
    traits::CreatureCatalog,
};
use std::num::NonZeroU32;

impl CellState {
    /// Reached by this zone's water flood, including adjoining land.
    #[must_use]
    pub const fn connection_visited(&self) -> bool {
        self.connection_visited
    }
}
impl ConnectingZones<'_, '_, '_> {
    /// Reset visits on this zone's entire level before its connection pass.
    ///
    /// # Errors
    /// Reports a zone not present in this map.
    pub fn clear_connection_visits(&mut self, zone: ZoneId) -> Result<(), PlacementError> {
        let level = self.map().zone(zone)?.position().level;
        self.map_mut().clear_connection_visits(level);
        Ok(())
    }
    /// Attempt a shipyard, completing only this directed record on success.
    /// An already-reached destination needs no new object but still draws art.
    ///
    /// # Errors
    /// Reports foreign context, empty art, arithmetic, placement or guard faults.
    pub fn try_shipyard_connection(
        &mut self,
        id: ConnectionId,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<bool, ConnectionError> {
        let connection = self.connection(id)?;
        let map = self.map_mut();
        map.prepare_object_context(objects, catalog)?;
        if !map.create_shipyard_connection(connection, objects, catalog, creatures, rng)? {
            return Ok(false);
        }
        map.terrain
            .coverage_mut()
            .map_mut()
            .complete_connection(id.index());
        Ok(true)
    }
}
impl PlacementMap<'_, '_, '_> {
    fn clear_connection_visits(&mut self, level: Level) {
        let side = self.view().side;
        let start = level.index() * side * side;
        for cell in &mut self.cells[start..start + side * side] {
            cell.connection_visited = false;
        }
    }
    // Cardinal LIFO flood across path-clearance water; adjoining land is marked
    // visited but not expanded. Even a non-open seed is visited and expanded.
    fn flood_connection_region(&mut self, position: WorldPosition) -> Result<(), PlacementError> {
        self.connections.water_stack.clear();
        self.connections.water_stack.try_reserve(1)?;
        self.connections.water_stack.push(position);
        let index = self.view().native_index(position)?;
        self.cells[index].connection_visited = true;
        while let Some(position) = self.connections.water_stack.pop() {
            for direction in Direction::ALL
                .into_iter()
                .step_by(raw::RMG_CARDINAL_DIRECTION_STEP as usize)
            {
                let nearby = offset_position(position, direction.offset())?;
                if !self.view().contains(nearby.point) {
                    continue;
                }
                let index = self.view().native_index(nearby)?;
                let water = self.terrain.tiles()[index].terrain() == Terrain::Water;
                if self.cells[index].connection_visited
                    || (water && self.cells[index].reservation != PathReservation::Open)
                {
                    continue;
                }
                self.cells[index].connection_visited = true;
                if water {
                    self.connections.water_stack.try_reserve(1)?;
                    self.connections.water_stack.push(nearby);
                }
            }
        }
        Ok(())
    }
    /// Check the native shipyard land pad, first clear-water probe and far side.
    /// The pad uses native flat addressing; only water/far-side columns are clipped.
    ///
    /// # Errors
    /// Reports coordinate overflow or an accessed cell outside the allocation.
    pub fn can_place_shipyard(&self, position: WorldPosition) -> Result<bool, PlacementError> {
        let below = offset_position(position, Point::new(0, 1))?;
        let side = i32::try_from(self.view().side).map_err(|_| PlacementError::Arithmetic)?;
        if below.point.y >= side {
            return Ok(false);
        }
        let left = position
            .point
            .x
            .checked_sub(2)
            .ok_or(PlacementError::CoordinateOverflow)?;
        for y in position.point.y..=below.point.y {
            for x in left..=position.point.x {
                let index = self.view().native_index(WorldPosition {
                    point: Point::new(x, y),
                    level: position.level,
                })?;
                if self.terrain.tiles()[index].terrain() == Terrain::Water
                    || self.cells[index].entrance.is_some()
                    || !self.view().passable(index)
                {
                    return Ok(false);
                }
            }
        }
        for (x, y) in raw::SHIPYARD_WATER_OFFSETS {
            let water = offset_position(position, Point::new(x, y))?;
            if !(0..side).contains(&water.point.x) {
                continue;
            }
            let index = self.view().native_index(water)?;
            if self.terrain.tiles()[index].terrain() == Terrain::Water
                && self.cells[index].reservation == PathReservation::Open
            {
                let far = offset_position(position, Point::new(if x < 0 { 1 } else { -3 }, 0))?;
                if !(0..side).contains(&far.point.x) {
                    return Ok(false);
                }
                return Ok(
                    self.terrain.tiles()[self.view().native_index(far)?].terrain()
                        != Terrain::Water,
                );
            }
        }
        Ok(false)
    }
    // Water probe order (shared source table). North is up; P is the anchor.
    //   0 # # P 1
    //   2 . . . 3
    // Unlike fit checks, the first water cell seeds the flood even if not open.
    pub(super) fn flood_shipyard_water(
        &mut self,
        position: WorldPosition,
    ) -> Result<(), PlacementError> {
        let side = i32::try_from(self.view().side).map_err(|_| PlacementError::Arithmetic)?;
        for (x, y) in raw::SHIPYARD_WATER_OFFSETS {
            let water = offset_position(position, Point::new(x, y))?;
            if (0..side).contains(&water.point.x)
                && self.terrain.tiles()[self.view().native_index(water)?].terrain()
                    == Terrain::Water
            {
                self.flood_connection_region(water)?;
                break;
            }
        }
        Ok(())
    }
    fn open_shipyard_approach(
        &mut self,
        position: WorldPosition,
        entry: &PreparedPrototype<'_>,
        zone: ZoneId,
    ) -> Result<WorldPosition, PlacementError> {
        let size = entry.image_mask().size().map_err(PlacementError::from)?;
        let mut approach = offset_position(position, Point::new(0, 1))?;
        let left = position
            .point
            .x
            .checked_sub(i32::from(size.width()) - 1)
            .ok_or(PlacementError::CoordinateOverflow)?;
        for x in left..=position.point.x {
            approach.point.x = x;
            let index = self.view().native_index(approach)?;
            self.cells[index].open_path();
            self.connections
                .crossing
                .append_entrance(zone, approach.point)?;
        }
        Ok(approach)
    }
    fn create_shipyard_connection(
        &mut self,
        connection: ZoneConnection,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<bool, ConnectionError> {
        let source = self.zone(connection.source)?;
        let destination = self.zone(connection.destination)?;
        if source.position().level != destination.position().level {
            return Ok(false);
        }
        let family = ObjectKind::parse(i32::try_from(raw::SHIPYARD).unwrap()).unwrap();
        let prototype = draw_connection_prototype(catalog, family, rng)?;
        let entry = catalog.get(prototype).unwrap();
        self.connections.candidates.clear();
        let Some(bounds) = source.bounds() else {
            return Ok(false);
        };
        for y in bounds.minimum().y..bounds.maximum().y {
            for x in bounds.minimum().x..bounds.maximum().x {
                let position = WorldPosition {
                    point: Point::new(x, y),
                    level: source.position().level,
                };
                let index = self.view().native_index(position)?;
                if self.coverage().map().raster().cells()[index].zone != Some(source.id())
                    || self.cells[index]
                        .zone_distance
                        .connection()
                        .map(|(zone, _)| zone)
                        != Some(destination.id())
                {
                    continue;
                }
                if self.cells[index].connection_visited {
                    return Ok(true);
                }
                if self.terrain.tiles()[index].terrain() != Terrain::Water
                    && usize::try_from(y + 1).is_ok_and(|row| row < self.view().side)
                {
                    for site_x in x..=x + 2 {
                        let site = WorldPosition {
                            point: Point::new(site_x, y),
                            level: position.level,
                        };
                        if self.connection_object_fits(entry, site, source)?
                            && self.can_place_shipyard(site)?
                        {
                            self.connections.candidates.try_reserve(1)?;
                            self.connections.candidates.push(site);
                        }
                    }
                }
            }
        }
        let Some(count) = NonZeroU32::new(
            u32::try_from(self.connections.candidates.len())
                .map_err(|_| PlacementError::Arithmetic)?,
        ) else {
            return Ok(false);
        };
        let object = objects.create_shipyard(catalog, prototype)?;
        let position = self.connections.candidates[rng.below(count) as usize];
        self.register_object(objects, catalog, object, position)?;
        let entrance = entrance_position(entry.prototype(), position)?;
        self.append_road_target(entrance)?;
        let mut approach = self.open_shipyard_approach(position, entry, source.id())?;
        self.flood_shipyard_water(position)?;
        let mut guard_value = self.connection_guard_value(connection)?;
        if connection.border_guard {
            approach.point.x = entrance
                .point
                .x
                .checked_sub(1)
                .ok_or(PlacementError::CoordinateOverflow)?;
            if self
                .place_border_guard(
                    approach,
                    BorderGuardCount::Shipyard,
                    destination.id(),
                    objects,
                    catalog,
                    rng,
                )?
                .reported_color()
                .is_some()
            {
                guard_value = 0;
            }
        }
        if guard_value > 0 {
            approach.point.x = entrance.point.x;
            self.place_guard(guard_value, approach, objects, catalog, creatures, rng)?;
        }
        Ok(true)
    }
}
