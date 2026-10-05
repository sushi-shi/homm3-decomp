//! Zone ownership, border and reachability repair after terrain brushes.
use super::{BrushStrength, TerrainError, TerrainWorkspace};
use crate::{
    boundaries::{BoundaryMap, ZoneOrigin},
    domain::Terrain,
    placement::GuardStrength,
    raw,
    rng::RetailRng,
    template::ZoneMonsters,
};

/// Spatial classifications retained for subsequent object/connection placement.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct TerrainRegion {
    edge: bool,
    reachable: bool,
}
impl TerrainRegion {
    /// Land cell whose 3x3 window crosses an eligible zone boundary.
    #[must_use]
    pub const fn is_edge(self) -> bool {
        self.edge
    }
    /// Reached from a zone's saved center, including terminal border cells.
    #[must_use]
    pub const fn is_reachable(self) -> bool {
        self.reachable
    }
}

impl TerrainWorkspace {
    // RVA 0x1ce1c0, then the outer splice's growing bounds and recenter pass.
    pub(super) fn finish_regions(
        &mut self,
        map: &mut BoundaryMap<'_>,
        rng: &mut RetailRng,
    ) -> Result<(), TerrainError> {
        set_guard_caps(map)?;
        let mut pending = std::mem::take(&mut self.pending);
        let result = self.flood_water(map, rng, &mut pending);
        self.pending = pending;
        result?;
        self.classify_regions(map)?;
        map.recenter()?;
        Ok(())
    }

    // Z, X, Y scan. Each newly discovered component gets its own strength-one
    // brush; flushing between components can affect the next scan's terrain.
    fn flood_water(
        &mut self,
        map: &mut BoundaryMap<'_>,
        rng: &mut RetailRng,
        pending: &mut Vec<usize>,
    ) -> Result<(), TerrainError> {
        let side = map.raster().dimension();
        let plane = side * side;
        for start in (0..self.tiles.len()).step_by(plane) {
            for x in 0..side {
                for y in 0..side {
                    let owner_index = start + y * side + x;
                    let Some(owner) = map.raster().cells()[owner_index].zone else {
                        continue;
                    };
                    for direction in (0..8).step_by(2) {
                        let Some(seed) = neighbour(y * side + x, direction, side) else {
                            continue;
                        };
                        if map.raster().cells()[start + seed].zone.is_some()
                            || self.tiles[start + seed].terrain() != Terrain::Water
                        {
                            continue;
                        }
                        let terrain = self.tiles[owner_index].terrain();
                        let mut brush = self.brush(start, side, terrain, rng);
                        brush.initial_strength =
                            BrushStrength::parse(1).ok_or(TerrainError::Arithmetic)?;
                        let cells = &mut map.raster_mut().cells_mut()[start..start + plane];
                        pending.clear();
                        pending.try_reserve(1)?;
                        pending.push(seed);
                        brush.paint(seed)?;
                        cells[seed].paint_terrain = terrain != Terrain::Water;
                        cells[seed].zone = Some(owner);
                        while let Some(index) = pending.pop() {
                            for direction in (0..8).step_by(2).rev() {
                                let Some(next) = neighbour(index, direction, side) else {
                                    continue;
                                };
                                if cells[next].zone.is_some()
                                    || brush.terrain(next) != Terrain::Water
                                {
                                    continue;
                                }
                                brush.paint(next)?;
                                cells[next].paint_terrain = terrain != Terrain::Water;
                                cells[next].zone = Some(owner);
                                pending.try_reserve(1)?;
                                pending.push(next);
                            }
                        }
                        brush.finish()?;
                    }
                }
            }
        }
        Ok(())
    }

    fn classify_regions(&mut self, map: &mut BoundaryMap<'_>) -> Result<(), TerrainError> {
        let side = map.raster().dimension();
        let plane = side * side;
        let cells = map.raster().cells();
        self.regions
            .try_reserve(cells.len().saturating_sub(self.regions.len()))?;
        self.regions.resize(cells.len(), TerrainRegion::default());
        self.regions.fill(TerrainRegion::default());
        for (index, cell) in cells.iter().enumerate() {
            if cell.zone.is_none() || self.tiles[index].terrain() == Terrain::Water {
                continue;
            }
            let start = index / plane * plane;
            self.regions[index].edge = (0..8)
                .filter_map(|direction| neighbour(index % plane, direction, side))
                .any(|nearby| {
                    let nearby = start + nearby;
                    match cells[nearby].zone {
                        None => self.tiles[nearby].terrain() != Terrain::Rock,
                        owner => {
                            owner != cell.zone && self.tiles[nearby].terrain() != Terrain::Water
                        }
                    }
                });
        }
        for zone in map.zones() {
            let saved = zone.saved_center().ok_or(TerrainError::Arithmetic)?;
            let position = if map.raster().bounds().contains(saved.point) {
                saved
            } else {
                zone.position()
            };
            if !map.raster().bounds().contains(position.point) {
                continue;
            }
            let start = position.level.index() * plane;
            let seed = usize::try_from(position.point.y).map_err(|_| TerrainError::Arithmetic)?
                * side
                + usize::try_from(position.point.x).map_err(|_| TerrainError::Arithmetic)?;
            self.pending.clear();
            self.pending.try_reserve(1)?;
            self.pending.push(seed);
            // Native seeds even an edge or a cell owned by another zone.
            self.regions[start + seed].reachable = true;
            while let Some(index) = self.pending.pop() {
                for direction in (0..8).rev() {
                    let Some(next) = neighbour(index, direction, side) else {
                        continue;
                    };
                    if cells[start + next].zone != Some(zone.id()) {
                        continue;
                    }
                    let record = &mut self.regions[start + next];
                    if record.reachable {
                        continue;
                    }
                    record.reachable = true;
                    if !record.edge {
                        self.pending.try_reserve(1)?;
                        self.pending.push(next);
                    }
                }
            }
        }
        for (cell, region) in map.raster_mut().cells_mut().iter_mut().zip(&self.regions) {
            if !region.reachable {
                cell.zone = None;
            }
        }
        Ok(())
    }
}

fn neighbour(index: usize, direction: usize, side: usize) -> Option<usize> {
    // RMG order starts east, unlike the terrain painter's tile directions.
    let offset = raw::DIRECTIONS[direction];
    let x = (index % side).checked_add_signed(offset.0 as isize)?;
    let y = (index / side).checked_add_signed(offset.1 as isize)?;
    (x < side && y < side).then_some(y * side + x)
}

fn set_guard_caps(map: &mut BoundaryMap<'_>) -> Result<(), TerrainError> {
    for index in 0..map.zones().len() {
        let zone = map.zones()[index];
        let mut cap = 0;
        if let ZoneOrigin::Template(id) = zone.origin() {
            let slot = &map.template().zones()[id.index()];
            let strength = match slot.monsters() {
                ZoneMonsters::None => None,
                ZoneMonsters::Weak => Some(raw::RMG_ZONE_MONSTERS_WEAK),
                ZoneMonsters::Average => Some(raw::RMG_ZONE_MONSTERS_AVERAGE),
                ZoneMonsters::Strong => Some(raw::RMG_ZONE_MONSTERS_STRONG),
            };
            if let Some(strength) = strength {
                let value = if slot.options().max_block_value >= 0 {
                    slot.options().max_block_value
                } else {
                    slot.treasure()
                        .iter()
                        .map(|band| band.maximum)
                        .max()
                        .unwrap_or(0)
                        / 3
                };
                let strength = (i32::from(map.request().strength().get()) - 3
                    + i32::try_from(strength).map_err(|_| TerrainError::Arithmetic)?)
                .clamp(0, 5);
                cap = GuardStrength::parse(strength)
                    .ok_or(TerrainError::Arithmetic)?
                    .scale(value)
                    .map_err(|_| TerrainError::Arithmetic)?;
            }
        }
        map.zones_mut()[index].set_guard_cap(cap);
    }
    Ok(())
}
