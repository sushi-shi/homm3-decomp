//! Dry junction routes share the boundary splitter and reusable flood storage.
use super::{ConnectionError, ConnectionsPlaced, Movement, PlacementMap, TownsPlaced};
use crate::{
    boundaries::{BoundaryZone, ZoneOrigin},
    domain::{Terrain, WorldPosition},
    geometry::Point,
    raster::{boundary_midpoint, BoundaryDisplacement, MapBounds},
    raw,
    rng::{RetailRng, RngCheckpoint},
    template::ZoneRole,
};

/// Both connection passes and the following dry-junction preparation are complete.
pub struct JunctionsPrepared<'state, 'zones, 'tiles> {
    connections: ConnectionsPlaced<'state, 'zones, 'tiles>,
    rng: RngCheckpoint,
}
impl<'state, 'zones, 'tiles> JunctionsPrepared<'state, 'zones, 'tiles> {
    pub(super) fn map_mut(&mut self) -> &mut PlacementMap<'state, 'zones, 'tiles> {
        self.connections.map_mut()
    }
    /// Current placement state, including junction paths and their cost floods.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.connections.map()
    }
    /// Earlier town payloads remain available for subsequent placement.
    #[must_use]
    pub const fn towns(&self) -> &TownsPlaced<'_, '_, '_> {
        self.connections.towns()
    }
    /// RNG after carving all dry junction routes in zone order.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}
impl<'state, 'zones, 'tiles> ConnectionsPlaced<'state, 'zones, 'tiles> {
    /// Reset and connect dry junction zones in template order.
    /// Consuming this stage prevents repeating the subdivision draws.
    ///
    /// # Errors
    /// Reports native cell access, invalid predecessors, subdivision arithmetic,
    /// retail zero roughness or failed workspace allocation.
    pub fn prepare_junctions(
        mut self,
        rng: &mut RetailRng,
    ) -> Result<JunctionsPrepared<'state, 'zones, 'tiles>, ConnectionError> {
        for index in 0..self.map().coverage().map().zones().len() {
            let zone = self.map().coverage().map().zones()[index];
            let ZoneOrigin::Template(id) = zone.origin() else {
                continue;
            };
            if zone.terrain() != Terrain::Water
                && matches!(
                    self.map().coverage().map().template().zones()[id.index()].role(),
                    ZoneRole::Junction(_)
                )
            {
                self.map_mut().prepare_junction_zone(zone, rng)?;
            }
        }
        Ok(JunctionsPrepared {
            connections: self,
            rng: rng.checkpoint(),
        })
    }
}
impl PlacementMap<'_, '_, '_> {
    fn prepare_junction_zone(
        &mut self,
        zone: BoundaryZone,
        rng: &mut RetailRng,
    ) -> Result<(), ConnectionError> {
        let level = zone.position().level;
        if let Some(bounds) = zone.bounds() {
            for y in bounds.minimum().y..bounds.maximum().y {
                for x in bounds.minimum().x..bounds.maximum().x {
                    let index = self.view().index(WorldPosition {
                        point: Point::new(x, y),
                        level,
                    })?;
                    if self.coverage().map().raster().cells()[index].zone == Some(zone.id())
                        && self.terrain.tiles()[index].terrain() != Terrain::Water
                    {
                        self.cells[index].movement = Movement::Unreached;
                        if self.memberships.first(self.cells[index].objects).is_none() {
                            self.cells[index].mark_obstacle();
                        }
                    }
                }
            }
        }
        let Some(&first) = self.zone_entrances(zone.id())?.first() else {
            return Ok(());
        };
        let first = WorldPosition {
            point: first,
            level,
        };
        // Native explicitly writes this seed before allocating the flood queue.
        let index = self.view().native_index(first)?;
        self.cells[index].movement = Movement::Seed;
        self.flood_connection_costs(first, false)?;
        for entrance in 1..self.zone_entrances(zone.id())?.len() {
            let from = self.zone_entrances(zone.id())?[entrance];
            let position = WorldPosition { point: from, level };
            let index = self.view().native_index(position)?;
            let cost = self.cells[index].movement.cost();
            // Unlike open_connection_path, equality with the limit is reached.
            if cost == 0 || u32::from(cost) > raw::RMG_REACHED_COST_LIMIT {
                continue;
            }
            let seed = self.junction_route_seed(position)?;
            self.connect_junction_entrance(from, seed.point, zone, rng)?;
            self.flood_connection_costs(position, false)?;
        }
        Ok(())
    }

    fn junction_route_seed(
        &self,
        mut position: WorldPosition,
    ) -> Result<WorldPosition, ConnectionError> {
        // Follow full XYZ predecessors, without imposing zone or decreasing-cost
        // restrictions absent from native. A repeated cell cannot reach a seed.
        for _ in 0..self.cells.len() {
            let index = self.view().native_index(position)?;
            position = match self.cells[index].movement {
                Movement::Arrived { previous, .. } => previous,
                Movement::Initial | Movement::Unreached | Movement::Seed => {
                    return Err(ConnectionError::InvalidPredecessor(position));
                }
            };
            let index = self.view().native_index(position)?;
            if matches!(
                self.cells[index].movement,
                Movement::Seed | Movement::Arrived { cost: 0, .. }
            ) {
                return Ok(position);
            }
        }
        Err(ConnectionError::InvalidPredecessor(position))
    }

    fn connect_junction_entrance(
        &mut self,
        mut from: Point,
        to: Point,
        zone: BoundaryZone,
        rng: &mut RetailRng,
    ) -> Result<(), ConnectionError> {
        self.connections.junction_targets.clear();
        self.connections.junction_targets.try_reserve(1)?;
        self.connections.junction_targets.push(to);
        let bounds = MapBounds::new(self.coverage().map().request().size());
        while let Some(to) = self.connections.junction_targets.pop() {
            if let Some(midpoint) = boundary_midpoint(
                from,
                to,
                zone.scaled_size(),
                BoundaryDisplacement::Full,
                self.coverage().map().behavior(),
                rng,
            )? {
                self.connections.junction_targets.try_reserve(2)?;
                self.connections.junction_targets.extend([to, midpoint]);
            } else {
                let position = WorldPosition {
                    point: bounds.clamp(from),
                    level: zone.position().level,
                };
                let index = self.view().index(position)?;
                if self.coverage().map().raster().cells()[index].zone == Some(zone.id()) {
                    self.cells[index].open_path();
                    self.clear_nearby_obstacles(position, Some(zone.id()))?;
                }
                from = to;
            }
        }
        Ok(())
    }
}
