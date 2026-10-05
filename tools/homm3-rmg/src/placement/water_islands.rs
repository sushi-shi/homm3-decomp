//! Water-zone islands: chamfer spacing, noise masks and full-plane terrain brushes.
use super::{
    island_noise::MaskSize, ConnectionBorders, ConnectionError, Direction, PlacementError,
    PlacementMap, TownsPlaced, ZoneDistance,
};
use crate::{
    boundaries::BoundaryZone,
    domain::{Level, Terrain, WorldPosition},
    geometry::{Point, ZoneId},
    raw,
    rng::{RetailRng, RngCheckpoint},
};
use std::num::NonZeroU32;

// Retail draws `rand() % eTerrainSubterranean`: dirt through rough.
#[allow(clippy::cast_sign_loss)]
const ISLAND_TERRAINS: NonZeroU32 = match NonZeroU32::new(raw::eTerrainSubterranean as u32) {
    Some(count) => count,
    None => panic!("the island terrain domain must not be empty"),
};

/// Water-zone islands are painted and ready for zone connection pathfinding.
pub struct WaterIslands<'state, 'zones, 'tiles> {
    pub(super) borders: ConnectionBorders<'state, 'zones, 'tiles>,
    rng: RngCheckpoint,
}
impl WaterIslands<'_, '_, '_> {
    /// Map state with repainted island terrain and spacing distances.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.borders.map()
    }
    /// Retained town payloads, road targets and historical town checkpoint.
    #[must_use]
    pub const fn towns(&self) -> &TownsPlaced<'_, '_, '_> {
        self.borders.towns()
    }
    /// RNG after the last island brush and distance flood.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}
impl<'state, 'zones, 'tiles> ConnectionBorders<'state, 'zones, 'tiles> {
    /// Paint islands in water zones, preserving zone order and flood side effects.
    /// Consuming this stage prevents repeating random island placement.
    ///
    /// # Errors
    /// Reports native arithmetic/access faults, failed brushes or allocation failure.
    pub fn place_water_islands(
        mut self,
        rng: &mut RetailRng,
    ) -> Result<WaterIslands<'state, 'zones, 'tiles>, ConnectionError> {
        let map = &mut self.towns.map;
        for index in 0..map.coverage().map().zones().len() {
            let zone = map.coverage().map().zones()[index];
            if zone.terrain() == Terrain::Water {
                map.place_water_zone_islands(zone, rng)?;
            }
        }
        Ok(WaterIslands {
            borders: self,
            rng: rng.checkpoint(),
        })
    }
}

impl PlacementMap<'_, '_, '_> {
    fn place_water_zone_islands(
        &mut self,
        zone: BoundaryZone,
        rng: &mut RetailRng,
    ) -> Result<(), ConnectionError> {
        let Some(bounds) = zone.bounds() else {
            return Ok(());
        };
        let level = zone.position().level;
        let minimum = bounds.minimum();
        let maximum = bounds.maximum();
        let side = self.view().signed_side();
        let unreached = const { crate::constants::narrow_u16(raw::RMG_UNREACHED_COST) };
        // The source resets the entire rectangle, including cells of other zones.
        for y in minimum.y..maximum.y {
            for x in minimum.x..maximum.x {
                let index = self.view().index(WorldPosition {
                    point: Point::new(x, y),
                    level,
                })?;
                self.cells[index].zone_distance = ZoneDistance::water(unreached, Direction::East);
            }
        }
        for y in (minimum.y - 1).max(0)..(maximum.y + 1).min(side) {
            for x in (minimum.x - 1).max(0)..(maximum.x + 1).min(side) {
                let position = WorldPosition {
                    point: Point::new(x, y),
                    level,
                };
                let index = self.view().index(position)?;
                if self.coverage().map().raster().cells()[index].zone != Some(zone.id()) {
                    self.flood_water_zone_distances(position, zone.id())?;
                }
            }
        }
        loop {
            self.connections.candidates.clear();
            for y in minimum.y.max(3)..maximum.y.min(side - 4) {
                for x in minimum.x.max(3)..maximum.x.min(side - 4) {
                    let position = WorldPosition {
                        point: Point::new(x, y),
                        level,
                    };
                    let index = self.view().index(position)?;
                    // No ownership/terrain/object predicate: the stored distance alone decides.
                    if u32::from(self.cells[index].zone_distance.cost())
                        >= raw::RMG_ISLAND_CLEARANCE
                    {
                        self.connections.candidates.try_reserve(1)?;
                        self.connections.candidates.push(position);
                    }
                }
            }
            let count = u32::try_from(self.connections.candidates.len())
                .map_err(|_| PlacementError::Arithmetic)?;
            let Some(count) = NonZeroU32::new(count) else {
                break;
            };
            let position = self.connections.candidates[rng.below(count) as usize];
            let index = self.view().index(position)?;
            let range = u32::from(self.cells[index].zone_distance.cost()) / 3 - 5;
            // Admitted candidates have cost >=20, proving a nonzero range.
            let radius = (rng.below(NonZeroU32::new(range).unwrap())
                + raw::RMG_ISLAND_MINIMUM_RADIUS)
                .min(raw::RMG_ISLAND_MAXIMUM_RADIUS);
            let radius = i32::try_from(radius).unwrap();
            let min = Point::new(
                (position.point.x - radius).max(0),
                (position.point.y - radius).max(0),
            );
            let max = Point::new(
                (position.point.x + radius).min(side),
                (position.point.y + radius).min(side),
            );
            self.create_water_zone_island(min, max, level, rng)?;
            self.flood_water_zone_distances(position, zone.id())?;
        }
        Ok(())
    }

    fn create_water_zone_island(
        &mut self,
        minimum: Point,
        maximum: Point,
        level: Level,
        rng: &mut RetailRng,
    ) -> Result<(), ConnectionError> {
        let mask_size = MaskSize::parse(maximum.x - minimum.x, maximum.y - minimum.y)?;
        let side = self.view().side();
        let x = usize::try_from(minimum.x).map_err(|_| PlacementError::Arithmetic)?;
        let y = usize::try_from(minimum.y).map_err(|_| PlacementError::Arithmetic)?;
        let terrain = Terrain::ALL[rng.below(ISLAND_TERRAINS) as usize];
        let mask = self.connections.noise.generate(mask_size, rng)?;
        let width = mask.size().width();
        let painted = mask
            .cells()
            .iter()
            .enumerate()
            .filter_map(|(index, value)| {
                (*value > 0).then_some((y + index / width) * side + x + index % width)
            });
        self.terrain.repaint(level, terrain, painted, rng)?;
        // Brush repair can change nearby cells. Mark every dry cell inside the
        // original rectangle, including preexisting land and zero-mask cells.
        for y in minimum.y..maximum.y {
            for x in minimum.x..maximum.x {
                let index = self.view().index(WorldPosition {
                    point: Point::new(x, y),
                    level,
                })?;
                if self.terrain.tiles()[index].terrain() != Terrain::Water {
                    self.cells[index].mark_obstacle();
                }
            }
        }
        Ok(())
    }

    // Eight-neighbour chamfer distances within one zone; a shorter distance
    // resets connection metadata without checking terrain or objects.
    fn flood_water_zone_distances(
        &mut self,
        seed: WorldPosition,
        zone: ZoneId,
    ) -> Result<(), ConnectionError> {
        self.connections.flood.clear();
        self.connections.flood.insert(seed, 0)?;
        let index = self.view().index(seed)?;
        self.cells[index].zone_distance = ZoneDistance::water(0, Direction::East);
        while let Some(position) = self.connections.flood.pop() {
            let index = self.view().index(position)?;
            let cost = u32::from(self.cells[index].zone_distance.cost());
            for direction in Direction::ALL {
                let point = position
                    .point
                    .checked_add(direction.offset())
                    .ok_or(PlacementError::CoordinateOverflow)?;
                if !self.view().contains(point) {
                    continue;
                }
                let next = WorldPosition {
                    point,
                    level: position.level,
                };
                let index = self.view().index(next)?;
                if self.coverage().map().raster().cells()[index].zone != Some(zone) {
                    continue;
                }
                let next_cost = cost + direction.chamfer_cost();
                if next_cost >= u32::from(self.cells[index].zone_distance.cost()) {
                    continue;
                }
                let next_cost = u16::try_from(next_cost).map_err(|_| PlacementError::Arithmetic)?;
                // Water spacing stores the forward direction, not its opposite.
                self.cells[index].zone_distance = ZoneDistance::water(next_cost, direction);
                self.connections.flood.insert(next, i32::from(next_cost))?;
            }
        }
        Ok(())
    }
}
