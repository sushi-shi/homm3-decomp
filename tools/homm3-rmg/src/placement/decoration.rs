//! Final terrain decoration operates directly on existing tile and cell buffers.

use super::{CellState, Neighborhood, PathReservation, PlacementError, PlacementMap};
use crate::{
    domain::{Level, Terrain, WorldPosition},
    geometry::Point,
    rng::RetailRng,
    terrain::TerrainError,
};

impl CellState {
    /// Land beside water, including diagonal neighbours; rock is never coastal.
    #[must_use]
    pub const fn is_coastal(&self) -> bool {
        self.coastal
    }
}

impl PlacementMap<'_, '_, '_> {
    /// Fill unused underground floor with rock, then restore occupied zone floor.
    /// One brush spans all zones; changing terrain finishes its queued repairs.
    ///
    /// # Errors
    /// Reports a missing terrain frame during painting or repair.
    #[expect(
        clippy::missing_panics_doc,
        reason = "clipped coordinates and parsed map dimensions fit the native index domain"
    )]
    pub fn decorate_underground(&mut self, rng: &mut RetailRng) -> Result<(), TerrainError> {
        let side = self.terrain.coverage().map().raster().dimension();
        let plane = side * side;
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
                        for y in bounds.minimum().y..bounds.maximum().y {
                            for x in bounds.minimum().x..bounds.maximum().x {
                                let index = usize::try_from(y).unwrap() * side
                                    + usize::try_from(x).unwrap();
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
                }
                Ok(())
            })
    }

    /// Mark dry neighbours of water in both planes after underground decoration.
    /// Existing coastal flags remain set, as in the native map helper.
    ///
    /// # Errors
    /// Reports coordinates outside admitted map dimensions.
    #[expect(
        clippy::missing_panics_doc,
        reason = "parsed map dimensions and allocated tile indexes fit i32"
    )]
    pub fn mark_coastal_tiles(&mut self) -> Result<(), PlacementError> {
        let side = self.view().side;
        let plane = side * side;
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
                    point: Point::new(
                        i32::try_from(index % side).unwrap(),
                        i32::try_from(index / side).unwrap(),
                    ),
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
