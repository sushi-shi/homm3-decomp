//! Connection floods, predecessor routes and deferred water-border painting.
use super::{
    ConnectionError, Movement, Neighborhood, ObjectArena, PathReservation, PlacementError,
    PlacementMap, TownsPlaced, WaterIslands, ZoneDistance,
};
use crate::{
    domain::{Level, Terrain, WorldPosition},
    geometry::{Point, ZoneId},
    object::ObjectKind,
    prototype::PrototypeCatalog,
    raw,
    rng::{RetailRng, RngCheckpoint},
};

/// Whether opening a predecessor route also clears surrounding obstacles.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PathWidth {
    /// Only open cells on the route.
    Narrow,
    /// Clear obstacle marks on same-zone cells in a clipped 3x3 neighborhood.
    Wide,
}
#[derive(Clone, Copy, Debug)]
pub(super) struct PaintRequest {
    index: usize,
    terrain: Terrain,
}

// A selected seed carries whether it already has passable path clearance.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Seed {
    Clear(WorldPosition),
    Unclear(WorldPosition),
}
impl Seed {
    const fn position(self) -> WorldPosition {
        match self {
            Self::Clear(position) | Self::Unclear(position) => position,
        }
    }
}
fn resolve_seed(
    found: Option<Seed>,
    previous: &mut Option<WorldPosition>,
    hotfix: bool,
    zone: ZoneId,
) -> Result<Option<Seed>, ConnectionError> {
    if let Some(seed) = found {
        *previous = Some(seed.position());
        return Ok(Some(seed));
    }
    if hotfix {
        return Ok(None);
    }
    previous
        .map(|position| Some(Seed::Unclear(position)))
        .ok_or(ConnectionError::SeedReplayRequired(zone))
}

/// First connection cost maps and their widened routes are ready.
pub struct ConnectionPaths<'state, 'zones, 'tiles> {
    islands: WaterIslands<'state, 'zones, 'tiles>,
    rng: RngCheckpoint,
}
impl ConnectionPaths<'_, '_, '_> {
    /// Placement state and movement/predecessor maps.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.islands.map()
    }
    /// Town payloads and retained earlier checkpoint.
    #[must_use]
    pub const fn towns(&self) -> &TownsPlaced<'_, '_, '_> {
        self.islands.towns()
    }
    /// RNG after opening all initial zone routes.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}
/// Initial routes and repaired water borders are ready for zone crossings.
pub struct RepairedWaterBorders<'state, 'zones, 'tiles> {
    paths: ConnectionPaths<'state, 'zones, 'tiles>,
    rng: RngCheckpoint,
}
impl RepairedWaterBorders<'_, '_, '_> {
    /// Placement and terrain after deferred border painting.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.paths.map()
    }
    /// Retained town payloads and road targets.
    #[must_use]
    pub const fn towns(&self) -> &TownsPlaced<'_, '_, '_> {
        self.paths.towns()
    }
    /// RNG after the last border brush finishes.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}
impl<'state, 'zones, 'tiles> WaterIslands<'state, 'zones, 'tiles> {
    /// Build initial connection costs and open routes within each zone.
    ///
    /// # Errors
    /// Reports context, placement, arithmetic, predecessor or prototype failures.
    pub fn build_connection_paths(
        mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<ConnectionPaths<'state, 'zones, 'tiles>, ConnectionError> {
        self.borders
            .towns
            .map
            .build_zone_connection_paths(objects, catalog, rng)?;
        Ok(ConnectionPaths {
            islands: self,
            rng: rng.checkpoint(),
        })
    }
}
impl<'state, 'zones, 'tiles> ConnectionPaths<'state, 'zones, 'tiles> {
    /// Repair water borders without a directed zone connection, in plane order.
    ///
    /// # Errors
    /// Reports cell access, buffer allocation or terrain painting failure.
    pub fn repair_water_borders(
        mut self,
        rng: &mut RetailRng,
    ) -> Result<RepairedWaterBorders<'state, 'zones, 'tiles>, ConnectionError> {
        self.islands
            .borders
            .towns
            .map
            .repair_water_zone_borders(rng)?;
        Ok(RepairedWaterBorders {
            paths: self,
            rng: rng.checkpoint(),
        })
    }
}
impl PlacementMap<'_, '_, '_> {
    fn admit_connection_objects(
        &mut self,
        objects: &ObjectArena,
        catalog: &PrototypeCatalog<'_>,
    ) -> Result<(), PlacementError> {
        self.prepare_registration(catalog)?;
        // Raw insertion binds cell membership even with no registered objects
        // or when the footprint is entirely clipped away.
        if !self.memberships.accepts_arena(objects) {
            return Err(PlacementError::ArenaContext);
        }
        Ok(())
    }

    pub(super) fn build_zone_connection_paths(
        &mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<(), ConnectionError> {
        self.admit_connection_objects(objects, catalog)?;
        for cell in &mut *self.cells {
            cell.movement = Movement::unreached();
            cell.zone_distance = ZoneDistance::reset();
        }
        // Retail retains the previous zone's seed; hotfix requires a fresh one.
        // Retail 0x540780–0x540789 reads uninitialized locals before any seed
        // exists; the reconstructed C++ sentinel skip is not native behavior.
        let mut previous_seed = None;
        for index in 0..self.coverage().map().zones().len() {
            let zone = self.coverage().map().zones()[index];
            let mut found = None;
            if let Some(bounds) = zone.bounds() {
                'rows: for y in bounds.minimum().y..bounds.maximum().y {
                    for x in bounds.minimum().x..bounds.maximum().x {
                        let position = WorldPosition {
                            point: Point::new(x, y),
                            level: zone.position().level,
                        };
                        let index = self.view().index(position)?;
                        let terrain = self.terrain.tiles()[index].terrain();
                        if self.coverage().map().raster().cells()[index].zone == Some(zone.id())
                            && (terrain != Terrain::Water || zone.terrain() == Terrain::Water)
                            && self.memberships.first(self.cells[index].objects).is_none()
                        {
                            found = Some(Seed::Unclear(position));
                            if self.cells[index].reservation == PathReservation::Open
                                && self.view().passable(index)
                            {
                                found = Some(Seed::Clear(position));
                                break 'rows;
                            }
                        }
                    }
                }
            }
            let Some(seed) = resolve_seed(
                found,
                &mut previous_seed,
                self.coverage().map().behavior().is_hotfix(),
                zone.id(),
            )?
            else {
                continue;
            };
            let position = seed.position();
            if let Seed::Unclear(_) = seed {
                let index = self.view().native_index(position)?;
                self.cells[index].open_path();
            }
            self.flood_connection_costs(position, zone.terrain() == Terrain::Water)?;
            if let Some(bounds) = zone.bounds() {
                for y in bounds.minimum().y..bounds.maximum().y {
                    for x in bounds.minimum().x..bounds.maximum().x {
                        let position = WorldPosition {
                            point: Point::new(x, y),
                            level: zone.position().level,
                        };
                        let index = self.view().index(position)?;
                        if self.coverage().map().raster().cells()[index].zone == Some(zone.id())
                            && self.cells[index].reservation == PathReservation::Open
                            && self.view().passable(index)
                            && self.cells[index].movement.cost() != 0
                            && self.terrain.tiles()[index].terrain() != Terrain::Water
                        {
                            self.open_connection_path(
                                position,
                                PathWidth::Wide,
                                objects,
                                catalog,
                                rng,
                            )?;
                            // Reseed even when an unreached path did not open anything.
                            self.flood_connection_costs(
                                position,
                                zone.terrain() == Terrain::Water,
                            )?;
                        }
                    }
                }
            }
        }
        Ok(())
    }

    /// Open a reached predecessor route, creating guards for protected borders.
    /// Stops before every zero-cost cell; widening preserves the starting zone.
    ///
    /// # Errors
    /// Reports foreign context, missing guard prototype, unsafe cell access,
    /// broken/cyclic predecessors or allocation failure.
    #[expect(
        clippy::missing_panics_doc,
        reason = "canonical border guard kind is in the admitted object domain"
    )]
    pub fn open_connection_path(
        &mut self,
        mut position: WorldPosition,
        width: PathWidth,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<(), ConnectionError> {
        self.admit_connection_objects(objects, catalog)?;
        let mut index = self.view().native_index(position)?;
        let zone = self.coverage().map().raster().cells()[index].zone;
        if u32::from(self.cells[index].movement.cost()) >= raw::RMG_REACHED_COST_LIMIT {
            return Ok(());
        }
        let mut remaining = self.cells.len();
        while self.cells[index].movement.cost() > 0 {
            if remaining == 0 {
                return Err(ConnectionError::InvalidPredecessor(position));
            }
            remaining -= 1;
            if let Some(color) = self.cells[index].border {
                let kind = ObjectKind::parse(i32::try_from(raw::BORDER_GUARD).unwrap()).unwrap();
                let prototype = catalog
                    .choose(kind, i32::from(color.value()), Terrain::Dirt, rng)
                    .ok_or(ConnectionError::MissingBorderGuard(color))?;
                let object = objects.create(catalog, prototype)?;
                self.cells[index].clear_border();
                self.register_object(objects, catalog, object, position)?;
            }
            self.cells[index].open_path();
            let previous = self.cells[index]
                .movement
                .previous()
                .ok_or(ConnectionError::InvalidPredecessor(position))?;
            if width == PathWidth::Wide {
                self.clear_nearby_obstacles(position, zone)?;
            }
            position = previous;
            index = self.view().native_index(position)?;
        }
        Ok(())
    }

    // Widen an opened route by clearing same-zone obstacle marks around the
    // cell, keeping border-connection cells. This does not open clearance.
    fn clear_nearby_obstacles(
        &mut self,
        center: WorldPosition,
        zone: Option<ZoneId>,
    ) -> Result<(), PlacementError> {
        for position in Neighborhood::ThreeByThree.cells(center, self.view().side)? {
            let index = self.view().index(position)?;
            if self.coverage().map().raster().cells()[index].zone == zone {
                self.cells[index].clear_obstacle();
            }
        }
        Ok(())
    }

    fn repair_water_zone_borders(&mut self, rng: &mut RetailRng) -> Result<(), ConnectionError> {
        let plane = self.view().side * self.view().side;
        for level in [Level::Surface, Level::Underground]
            .into_iter()
            .take(self.coverage().map().request().levels().count() as usize)
        {
            self.connections.repairs.clear();
            let start = level.index() * plane;
            for index in start..start + plane {
                let Some(zone) = self.coverage().map().raster().cells()[index].zone else {
                    continue;
                };
                if self.terrain.tiles()[index].terrain() != Terrain::Water {
                    continue;
                }
                let Some((destination, _)) = self.cells[index].zone_distance.connection() else {
                    continue;
                };
                let position = self.position_at(index);
                let mut land = None;
                for nearby in Neighborhood::ThreeByThree.cells(position, self.view().side)? {
                    let index = self.view().index(nearby)?;
                    let terrain = self.terrain.tiles()[index].terrain();
                    if terrain != Terrain::Water
                        && self.view().passable(index)
                        && self.cells[index].reservation != PathReservation::Obstacle
                    {
                        land = Some(terrain);
                        break;
                    }
                }
                let Some(terrain) = land else {
                    continue;
                };
                if self
                    .coverage()
                    .map()
                    .connections()
                    .iter()
                    .any(|edge| edge.source == zone && edge.destination == destination)
                {
                    continue;
                }
                for nearby in Neighborhood::ThreeByThree.cells(position, self.view().side)? {
                    let index = self.view().index(nearby)?;
                    self.cells[index].mark_obstacle();
                    if self.terrain.tiles()[index].terrain() == Terrain::Water {
                        self.connections.repairs.try_reserve(1)?;
                        self.connections.repairs.push(PaintRequest {
                            index: index - start,
                            terrain,
                        });
                    }
                }
                self.release_neighborhood_path(position, Neighborhood::FiveByFive)?;
            }
            // Native changeTerrain finishes each consecutive run. Preserve
            // duplicates and append order; terrain remains unchanged until now.
            for run in self
                .connections
                .repairs
                .chunk_by(|a, b| a.terrain == b.terrain)
            {
                self.terrain.repaint(
                    level,
                    run[0].terrain,
                    run.iter().map(|paint| paint.index),
                    rng,
                )?;
            }
            self.connections.repairs.clear();
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn seedless_retail_requires_replay_then_reuses_the_previous_zone_seed() {
        let zone = ZoneId::new(0);
        let mut previous = None;
        assert!(
            matches!(resolve_seed(None, &mut previous, false, zone), Err(ConnectionError::SeedReplayRequired(found)) if found == zone)
        );
        assert_eq!(resolve_seed(None, &mut previous, true, zone).unwrap(), None);
        let position = WorldPosition {
            point: Point::new(4, 7),
            level: Level::Surface,
        };
        assert_eq!(
            resolve_seed(Some(Seed::Clear(position)), &mut previous, false, zone).unwrap(),
            Some(Seed::Clear(position))
        );
        assert_eq!(
            resolve_seed(None, &mut previous, false, zone).unwrap(),
            Some(Seed::Unclear(position))
        );
        assert_eq!(resolve_seed(None, &mut previous, true, zone).unwrap(), None);
        assert_eq!(previous, Some(position));
    }
}
