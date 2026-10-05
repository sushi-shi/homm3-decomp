//! Weighted zone bands reuse one group; completion uses its separate scratch pool.
use super::{
    density::Density, ObjectArena, PlacementError, PlacementMap, SelectedTreasure,
    TreasureGeneration, TreasureGenerationError, TreasureGroupWorkspace, TreasurePacking,
};
use crate::{
    domain::Terrain,
    geometry::ZoneId,
    raw,
    rng::RetailRng,
    template::{Placement, TreasureBand},
};
use std::num::NonZeroU32;

/// Every zone's treasure bands are placed; final decoration follows.
pub(crate) struct TreasuresPlaced<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards> {
    generation: TreasureGeneration<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>,
}
impl<'state, 'zones, 'tiles> TreasuresPlaced<'state, 'zones, 'tiles, '_, '_, '_, '_> {
    pub(super) fn map_mut(&mut self) -> &mut PlacementMap<'state, 'zones, 'tiles> {
        self.generation.map_mut()
    }
    /// Payload context retained for serialization.
    pub(super) const fn generation(&self) -> &TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
        &self.generation
    }
}
impl<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>
    TreasureGeneration<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>
{
    /// Place all zones' treasure bands in stored map order, including water zones.
    /// Connections are not rebuilt between zones. The workspace lends its offer
    /// and nested-group buffers and takes them back on both success and failure.
    ///
    /// # Errors
    /// Reports native arithmetic, selection, placement and completion faults.
    pub(crate) fn place_all_treasures(
        mut self,
        group: &mut TreasureGroupWorkspace,
        offers: &mut Vec<SelectedTreasure>,
        nested_groups: &mut Vec<Box<TreasureGroupWorkspace>>,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<
        TreasuresPlaced<'state, 'zones, 'tiles, 'defs, 'assets, 'source, 'rewards>,
        TreasureGenerationError,
    > {
        self.exchange_scratch(offers, nested_groups);
        let placed = self.place_treasures_in_zone_order(group, objects, rng);
        self.exchange_scratch(offers, nested_groups);
        placed?;
        Ok(TreasuresPlaced { generation: self })
    }
}
impl TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
    fn place_treasures_in_zone_order(
        &mut self,
        group: &mut TreasureGroupWorkspace,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<(), TreasureGenerationError> {
        self.require_arena(objects)?;
        for index in 0..self.ready.map().coverage().map().zones().len() {
            self.place_zone_treasures(group, ZoneId::new(index), objects, rng)?;
        }
        Ok(())
    }
    fn place_zone_treasures(
        &mut self,
        group: &mut TreasureGroupWorkspace,
        zone: ZoneId,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<(), TreasureGenerationError> {
        let zone = self.ready.map().zone(zone)?;
        let bands = match self.ready.map().coverage().map().template_zone(&zone) {
            Some(rules) => *rules.treasure(),
            None => water_bands(),
        };
        let placements = bands.map(|band| Placement {
            initial_count: 0,
            density: if band.maximum < i32::try_from(raw::RMG_TREASURE_MINIMUM_VALUE).unwrap() {
                None
            } else {
                band.density
            },
        });
        let mut density = Density::new(&placements)?;
        let area = if zone.terrain() == Terrain::Water {
            raw::RMG_WATER_TREASURE_DENSITY_AREA
        } else {
            raw::RMG_LAND_TREASURE_DENSITY_AREA
        };
        let Some(spacing) =
            density.spacing(i32::try_from(area).map_err(|_| PlacementError::Arithmetic)?)
        else {
            return Ok(());
        };
        while let Some(index) = density.next()? {
            let mut placed = false;
            for packing in [TreasurePacking::Ordinary, TreasurePacking::Compact] {
                for _ in 0..raw::RMG_TREASURE_ATTEMPTS {
                    if self.assemble_group(
                        group,
                        zone.id(),
                        bands[index].minimum..bands[index].maximum,
                        packing,
                        objects,
                        rng,
                    )? {
                        if self.place_group(group, zone.id(), spacing, objects, rng)? {
                            placed = true;
                            break;
                        }
                        self.discard_group(group, objects)?;
                    } else {
                        // Native reset forgets zero-valued roots without releasing
                        // their reservations or prototype references, in BOTH modes.
                        Self::retain_abandoned_group(group, objects)?;
                    }
                }
                if placed {
                    break;
                }
            }
            if !placed {
                density.finish(index);
            }
        }
        Ok(())
    }
    fn retain_abandoned_group(
        group: &mut TreasureGroupWorkspace,
        objects: &mut ObjectArena,
    ) -> Result<(), TreasureGenerationError> {
        if let Some(owner) = group.owner {
            for id in group.objects() {
                objects.require_group(id, owner)?;
                Self::check_pending_child(id, objects)?;
            }
            group.clear_memberships();
            for id in group.objects() {
                if let Some(super::ObjectPayload::QuestArtifact(quest)) =
                    objects.payload(id).copied()
                {
                    if let Some(child) = quest.pending_seer() {
                        objects.retire(child, true)?;
                    }
                }
                objects.retire(id, true)?;
            }
        }
        group.objects.clear();
        group.reset_after_disposal();
        Ok(())
    }
}
fn water_bands() -> [TreasureBand; raw::RMG_TREASURE_BAND_COUNT as usize] {
    let mut bands = [TreasureBand {
        minimum: 0,
        maximum: 0,
        density: None,
    }; raw::RMG_TREASURE_BAND_COUNT as usize];
    bands[0] = TreasureBand {
        minimum: i32::try_from(crate::constants::RMG_WATER_TREASURE_0_MINIMUM).unwrap(),
        maximum: i32::try_from(crate::constants::RMG_WATER_TREASURE_0_MAXIMUM).unwrap(),
        density: NonZeroU32::new(crate::constants::RMG_WATER_TREASURE_0_DENSITY),
    };
    bands[1] = TreasureBand {
        minimum: i32::try_from(crate::constants::RMG_WATER_TREASURE_1_MINIMUM).unwrap(),
        maximum: i32::try_from(crate::constants::RMG_WATER_TREASURE_1_MAXIMUM).unwrap(),
        density: NonZeroU32::new(crate::constants::RMG_WATER_TREASURE_1_DENSITY),
    };
    bands
}
