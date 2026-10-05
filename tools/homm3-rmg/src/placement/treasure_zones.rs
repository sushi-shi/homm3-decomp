//! Weighted zone bands reuse one group; completion uses its separate scratch pool.
use super::{
    density::Density, ObjectArena, PlacementError, TreasureGeneration, TreasureGenerationError,
    TreasureGroupWorkspace, TreasurePacking,
};
use crate::{
    boundaries::ZoneOrigin,
    domain::Terrain,
    geometry::ZoneId,
    raw,
    rng::RetailRng,
    template::{Placement, TreasureBand},
};
use std::num::NonZeroU32;
impl TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
    /// Place all zones' treasure bands in stored map order, including water zones.
    /// Connections are not rebuilt between zones.
    ///
    /// # Errors
    /// Reports native arithmetic, selection, placement and completion faults.
    pub fn place_all_treasures(
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
        let bands = match zone.origin() {
            ZoneOrigin::Template(id) => {
                *self.ready.map().coverage().map().template().zones()[id.index()].treasure()
            }
            ZoneOrigin::Water => water_bands(),
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
