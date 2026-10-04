//! Frozen border scans and per-record land crossing attempts.
use super::{
    BorderGuardCount, ConnectionError, GuardStrength, ObjectArena, PathWidth, PlacementError,
    PlacementMap, RepairedWaterBorders, TownsPlaced,
};
use crate::{
    boundaries::ZoneConnection,
    domain::{Terrain, WorldPosition},
    geometry::{Point, ZoneId},
    identity::OwnerId,
    prototype::PrototypeCatalog,
    raw,
    rng::RetailRng,
    traits::CreatureCatalog,
};
use std::num::NonZeroU32;

/// One directed graph record; duplicates remain distinct and handles cannot
/// select an edge belonging to another connection stage.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ConnectionId {
    owner: OwnerId,
    index: usize,
}
impl ConnectionId {
    /// Flat record index, preserving its source zone's adjacency order.
    #[must_use]
    pub const fn index(self) -> usize {
        self.index
    }
}
#[derive(Default, Debug)]
pub(super) struct CrossingState {
    borders: Vec<WorldPosition>,
    // Native per-zone sequences support ordered traversal and first-entrance
    // indexing. Retain even inactive zones' capacities across map resets.
    entrances: Vec<Vec<Point>>,
}
impl CrossingState {
    pub(super) fn reset(&mut self) {
        self.borders.clear();
        for entries in &mut self.entrances {
            entries.clear();
        }
    }
    fn begin(&mut self, zones: usize) -> Result<(), PlacementError> {
        self.entrances
            .try_reserve(zones.saturating_sub(self.entrances.len()))?;
        self.entrances
            .resize_with(zones.max(self.entrances.len()), Vec::new);
        Ok(())
    }
    fn append_entrance(&mut self, zone: ZoneId, point: Point) -> Result<(), PlacementError> {
        let entries = &mut self.entrances[zone.index()];
        entries.try_reserve(1)?;
        entries.push(point);
        Ok(())
    }
}

/// Connection creation shares one initial border scan and live mutable cells.
/// Each native edge will try ground, shipyard, then gate in that order; this
/// token deliberately does not represent a separate whole-map ground pass.
pub struct ConnectingZones<'state, 'zones, 'tiles> {
    repaired: RepairedWaterBorders<'state, 'zones, 'tiles>,
    owner: OwnerId,
}
impl<'state, 'zones, 'tiles> RepairedWaterBorders<'state, 'zones, 'tiles> {
    /// Freeze potential border coordinates before trying any connection.
    ///
    /// # Errors
    /// Reports foreign object context, allocation or native cell-access faults.
    pub fn begin_connections(
        mut self,
        objects: &ObjectArena,
        catalog: &PrototypeCatalog<'_>,
    ) -> Result<ConnectingZones<'state, 'zones, 'tiles>, ConnectionError> {
        let map = self.map_mut();
        map.prepare_object_context(objects, catalog)?;
        map.connections
            .crossing
            .begin(map.coverage().map().zones().len())?;
        let owner = OwnerId::new().ok_or(PlacementError::IdentityExhausted)?;
        map.collect_connection_borders()?;
        Ok(ConnectingZones {
            repaired: self,
            owner,
        })
    }
}
impl ConnectingZones<'_, '_, '_> {
    /// Current shared terrain, cells, objects and directed graph state.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.repaired.map()
    }
    /// Earlier town stage, retaining primary entrances and road targets.
    #[must_use]
    pub const fn towns(&self) -> &TownsPlaced<'_, '_, '_> {
        self.repaired.towns()
    }
    /// Copied handles can be iterated while mutating this stage, without a
    /// temporary ID collection. Filter by each source zone for native order.
    #[must_use]
    pub fn connection_ids(&self) -> impl ExactSizeIterator<Item = ConnectionId> + 'static {
        let owner = self.owner;
        (0..self.map().coverage().map().connections().len())
            .map(move |index| ConnectionId { owner, index })
    }
    /// Resolve a checked handle to this graph's current record.
    ///
    /// # Errors
    /// Rejects a handle belonging to a different connection stage.
    pub fn connection(&self, id: ConnectionId) -> Result<ZoneConnection, ConnectionError> {
        if id.owner != self.owner {
            return Err(ConnectionError::UnknownConnection(id));
        }
        self.map()
            .coverage()
            .map()
            .connections()
            .get(id.index)
            .copied()
            .ok_or(ConnectionError::UnknownConnection(id))
    }
    /// Try land crossings for this record, then complete it and its first
    /// reverse record on success. The dispatcher skips already-completed edges.
    ///
    /// # Errors
    /// Reports identity, arithmetic, placement or guard failures. A missing
    /// reverse record faults after crossing mutations and completing this edge.
    pub fn try_ground_connection(
        &mut self,
        id: ConnectionId,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<bool, ConnectionError> {
        let connection = self.connection(id)?;
        let reverse = self
            .map()
            .coverage()
            .map()
            .connections()
            .iter()
            .position(|other| {
                other.source == connection.destination && other.destination == connection.source
            });
        let map = self.repaired.map_mut();
        map.prepare_object_context(objects, catalog)?;
        if !map.create_ground_connection(connection, objects, catalog, creatures, rng)? {
            return Ok(false);
        }
        let graph = map.terrain.coverage_mut().map_mut();
        graph.complete_connection(id.index);
        let reverse = reverse.ok_or(ConnectionError::MissingReverse {
            source: connection.source,
            destination: connection.destination,
        })?;
        graph.complete_connection(reverse);
        Ok(true)
    }
}
impl PlacementMap<'_, '_, '_> {
    /// Native entrance sequence, including repeated coordinates, for this zone.
    ///
    /// # Errors
    /// Reports a zone outside this map.
    pub fn zone_entrances(&self, zone: ZoneId) -> Result<&[Point], PlacementError> {
        self.zone(zone)?;
        Ok(self
            .connections
            .crossing
            .entrances
            .get(zone.index())
            .map_or(&[], Vec::as_slice))
    }
    fn collect_connection_borders(&mut self) -> Result<(), PlacementError> {
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
    fn create_ground_connection(
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
        let mut guard_value = if connection.unguarded {
            0
        } else {
            GuardStrength::from(self.coverage().map().request().strength())
                .scale(connection.value)?
        };
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
