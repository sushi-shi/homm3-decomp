//! Connection stage ownership and native per-zone dispatch.
use super::{
    ConnectionError, ObjectArena, PlacementError, PlacementMap, RepairedWaterBorders, TownsPlaced,
};
use crate::{
    boundaries::ZoneConnection,
    domain::{Terrain, WorldPosition},
    geometry::{Point, ZoneId},
    identity::OwnerId,
    prototype::PrototypeCatalog,
    rng::{RetailRng, RngCheckpoint},
    traits::CreatureCatalog,
};
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
    pub(super) borders: Vec<WorldPosition>,
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
    pub(super) fn append_entrance(
        &mut self,
        zone: ZoneId,
        point: Point,
    ) -> Result<(), PlacementError> {
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
impl<'state, 'zones, 'tiles> ConnectingZones<'state, 'zones, 'tiles> {
    pub(super) fn map_mut(&mut self) -> &mut PlacementMap<'state, 'zones, 'tiles> {
        self.repaired.map_mut()
    }
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
    pub(super) fn reverse_connection(&self, connection: ZoneConnection) -> Option<usize> {
        self.map()
            .coverage()
            .map()
            .connections()
            .iter()
            .position(|other| {
                other.source == connection.destination && other.destination == connection.source
            })
    }
    pub(super) fn complete_bidirectional(
        &mut self,
        id: ConnectionId,
        connection: ZoneConnection,
        reverse: Option<usize>,
    ) -> Result<(), ConnectionError> {
        let graph = self.map_mut().terrain.coverage_mut().map_mut();
        graph.complete_connection(id.index);
        let reverse = reverse.ok_or(ConnectionError::MissingReverse {
            source: connection.source,
            destination: connection.destination,
        })?;
        graph.complete_connection(reverse);
        Ok(())
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
        let reverse = self.reverse_connection(connection);
        let map = self.repaired.map_mut();
        map.prepare_object_context(objects, catalog)?;
        if !map.create_ground_connection(connection, objects, catalog, creatures, rng)? {
            return Ok(false);
        }
        self.complete_bidirectional(id, connection, reverse)?;
        Ok(true)
    }
}

/// First-pass attempts are complete; remaining edges need shipyard retries or portals.
pub struct DirectConnections<'state, 'zones, 'tiles> {
    pub(super) connecting: ConnectingZones<'state, 'zones, 'tiles>,
    rng: RngCheckpoint,
}
impl DirectConnections<'_, '_, '_> {
    /// Shared map state after the complete first connection pass.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.connecting.map()
    }
    /// Town records remain available to subsequent placement stages.
    #[must_use]
    pub const fn towns(&self) -> &TownsPlaced<'_, '_, '_> {
        self.connecting.towns()
    }
    /// RNG after all first-pass connection attempts.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}
impl<'state, 'zones, 'tiles> ConnectingZones<'state, 'zones, 'tiles> {
    /// Run the native first pass in zone/adjoining-record order, retaining
    /// water visits between a zone's attempts and clearing its entire level first.
    ///
    /// # Errors
    /// Reports prototype, arithmetic, allocation, placement or guard faults.
    pub fn connect_direct_zones(
        mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<DirectConnections<'state, 'zones, 'tiles>, ConnectionError> {
        self.map_mut().prepare_object_context(objects, catalog)?;
        for index in 0..self.map().coverage().map().zones().len() {
            let zone = self.map().coverage().map().zones()[index];
            if zone.terrain() == Terrain::Water {
                continue;
            }
            self.clear_connection_visits(zone.id())?;
            for id in self.connection_ids() {
                let connection = self.connection(id)?;
                if connection.source != zone.id() || connection.connected {
                    continue;
                }
                if self.try_ground_connection(id, objects, catalog, creatures, rng)?
                    || self.try_shipyard_connection(id, objects, catalog, creatures, rng)?
                {
                    continue;
                }
                if self.map().zone(connection.destination)?.terrain() != Terrain::Water {
                    self.try_gate_connection(id, objects, catalog, creatures, rng)?;
                }
            }
        }
        Ok(DirectConnections {
            connecting: self,
            rng: rng.checkpoint(),
        })
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
}
