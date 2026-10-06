//! Final terrain decoration operates directly on existing tile and cell buffers.

use super::{
    CellState, Neighborhood, PathReservation, PlacementError, PlacementMap, TreasureGeneration,
    TreasuresPlaced,
};
use crate::{
    domain::{Level, Terrain, WorldPosition},
    rng::RetailRng,
    terrain::TerrainError,
};

/// Unused underground floor is rock and occupied zone floor is restored.
pub(crate) struct UndergroundDecorated<'map> {
    treasures: TreasuresPlaced<'map>,
}
impl<'map> UndergroundDecorated<'map> {
    pub(super) fn map_mut(&mut self) -> &mut PlacementMap<'map> {
        self.treasures.map_mut()
    }
    pub(super) const fn generation(&self) -> &TreasureGeneration<'_> {
        self.treasures.generation()
    }
}
impl<'map> TreasuresPlaced<'map> {
    /// Fill unused underground floor with rock after all treasures are placed.
    ///
    /// # Errors
    /// Reports a missing terrain frame during painting or repair.
    pub(crate) fn decorate_underground(
        mut self,
        rng: &mut RetailRng,
    ) -> Result<UndergroundDecorated<'map>, TerrainError> {
        self.map_mut().decorate_underground(rng)?;
        Ok(UndergroundDecorated { treasures: self })
    }
}

/// Dry neighbours of water are marked coastal in both planes.
pub(crate) struct CoastsMarked<'map> {
    underground: UndergroundDecorated<'map>,
}
impl<'map> CoastsMarked<'map> {
    pub(super) fn map_mut(&mut self) -> &mut PlacementMap<'map> {
        self.underground.map_mut()
    }
    pub(super) const fn generation(&self) -> &TreasureGeneration<'_> {
        self.underground.generation()
    }
}
impl<'map> UndergroundDecorated<'map> {
    /// Mark coasts once, after underground decoration has settled the terrain.
    /// No RNG is consumed.
    ///
    /// # Errors
    /// Reports coordinates outside admitted map dimensions.
    pub(crate) fn mark_coastal_tiles(mut self) -> Result<CoastsMarked<'map>, PlacementError> {
        self.map_mut().mark_coastal_tiles()?;
        Ok(CoastsMarked { underground: self })
    }
}

impl CellState {
    /// Land beside water, including diagonal neighbours; rock is never coastal.
    #[must_use]
    pub const fn is_coastal(&self) -> bool {
        self.coastal
    }
}

impl PlacementMap<'_> {
    /// Fill unused underground floor with rock, then restore occupied zone floor.
    /// One brush spans all zones; changing terrain finishes its queued repairs.
    ///
    /// # Errors
    /// Reports a missing terrain frame during painting or repair.
    // Cannot panic: zone bounds enclose cells of the zone raster.
    fn decorate_underground(&mut self, rng: &mut RetailRng) -> Result<(), TerrainError> {
        let layout = self.terrain.coverage().map().raster().layout();
        let plane = layout.plane();
        if self.cells.len() == plane {
            return Ok(());
        }
        let cells = &self.cells[plane..];
        let memberships = &self.memberships;
        self.terrain
            .with_brush(Level::Underground, Terrain::Rock, rng, |coverage, brush| {
                for (index, cell) in cells.iter().enumerate() {
                    if cell.can_block_floor(brush.terrain(index)) {
                        brush.paint(index)?;
                    }
                }
                let mut current = Terrain::Rock;
                for zone in coverage.map().zones() {
                    if zone.position().level != Level::Underground {
                        continue;
                    }
                    let terrain = zone.terrain();
                    if current == Terrain::Rock {
                        brush.change_terrain(terrain)?;
                        current = terrain;
                    }
                    if let Some(bounds) = zone.bounds() {
                        for point in bounds.points() {
                            let index = layout
                                .plane_index(point)
                                .expect("zone bounds enclose map cells");
                            if brush.terrain(index) == Terrain::Rock
                                && coverage.map().raster().cells()[plane + index].zone
                                    == Some(zone.id())
                                && (cells[index].reservation == PathReservation::Open
                                    || memberships.first(cells[index].objects).is_some())
                            {
                                if terrain != current {
                                    brush.change_terrain(terrain)?;
                                    current = terrain;
                                }
                                brush.paint(index)?;
                            }
                        }
                    }
                }
                Ok(())
            })
    }

    /// Mark dry neighbours of water in both planes after underground decoration.
    /// Existing coastal flags remain set, as in the native map helper.
    ///
    /// # Errors
    /// Reports coordinates outside admitted map dimensions.
    fn mark_coastal_tiles(&mut self) -> Result<(), PlacementError> {
        let layout = self.terrain.coverage().map().raster().layout();
        let (side, plane) = (layout.side(), layout.plane());
        for level in [Level::Surface, Level::Underground] {
            let start = level.index() * plane;
            if start >= self.cells.len() {
                break;
            }
            for index in 0..plane {
                if self.terrain.tiles()[start + index].terrain() != Terrain::Water {
                    continue;
                }
                let center = WorldPosition {
                    point: layout.point(index),
                    level,
                };
                for position in Neighborhood::ThreeByThree.cells(center, side)? {
                    let nearby = self.view().index(position)?;
                    if !matches!(
                        self.terrain.tiles()[nearby].terrain(),
                        Terrain::Water | Terrain::Rock
                    ) {
                        self.cells[nearby].coastal = true;
                    }
                }
            }
        }
        Ok(())
    }
}
