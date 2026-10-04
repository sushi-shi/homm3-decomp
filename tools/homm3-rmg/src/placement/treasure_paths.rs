//! Primary-town faction weights and the post-mine connection rebuild.
use super::{ConnectionError, MinesPlaced, ObjectArena, PlacementError, PlacementMap, TownsPlaced};
use crate::{
    prototype::PrototypeCatalog,
    raw,
    request::Town,
    rng::{RetailRng, RngCheckpoint},
};

/// Native primary-town zone tally used when valuing creature rewards/dwellings.
/// Additional towns do not add entries; neutral alignment aliases the total.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct TownZoneCounts {
    total: i32,
    aligned: [i32; raw::TOWN_TYPE_COUNT as usize],
}
impl TownZoneCounts {
    fn add(&mut self, alignment: Option<Town>) -> Result<(), PlacementError> {
        // Retail generate 0x549bed–0x549c05: bucket[-1] aliases total at +0xf60,
        // immediately before the nine faction buckets at +0xf64. The subsequent
        // increment reloads total. Preserve this contained alias without indexing
        // outside an array: a neutral primary-town zone contributes twice.
        let bucket = alignment.map_or(&mut self.total, |town| &mut self.aligned[town.index()]);
        *bucket = bucket.checked_add(1).ok_or(PlacementError::Arithmetic)?;
        self.total = self
            .total
            .checked_add(1)
            .ok_or(PlacementError::Arithmetic)?;
        Ok(())
    }
    /// Native denominator. A neutral-alignment primary-town zone contributes two.
    #[must_use]
    pub const fn total(&self) -> i32 {
        self.total
    }
    /// Faction buckets in native order; neutral alignment has no bucket.
    #[must_use]
    pub const fn by_alignment(&self) -> &[i32; raw::TOWN_TYPE_COUNT as usize] {
        &self.aligned
    }
    /// Source getTownZoneCount: neutral alignment always reports zero.
    #[must_use]
    pub fn aligned(&self, town: Option<Town>) -> i32 {
        town.map_or(0, |town| self.aligned[town.index()])
    }
    /// Raise a reward value by the share of primary-town zones of its faction.
    ///
    /// # Errors
    /// Reports signed overflow at the native product or sum; division truncates.
    pub fn adjust(&self, value: i32, town: Option<Town>) -> Result<i32, PlacementError> {
        if self.total == 0 {
            return Ok(value);
        }
        let extra = self
            .aligned(town)
            .checked_mul(value)
            .ok_or(PlacementError::Arithmetic)?
            / self.total;
        value.checked_add(extra).ok_or(PlacementError::Arithmetic)
    }
}

/// Mine placement, faction tally and the following path rebuild are complete.
/// Treasure definitions are initialized earlier with assets; this is map state.
pub struct TreasurePaths<'state, 'zones, 'tiles> {
    mines: MinesPlaced<'state, 'zones, 'tiles>,
    town_zones: TownZoneCounts,
    rng: RngCheckpoint,
}
impl TreasurePaths<'_, '_, '_> {
    /// Current map after opening all post-mine zone routes.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        self.mines.map()
    }
    /// Existing town payloads and checkpoints.
    #[must_use]
    pub const fn towns(&self) -> &TownsPlaced<'_, '_, '_> {
        self.mines.towns()
    }
    /// Frozen faction weights used by subsequent treasure value calculations.
    #[must_use]
    pub const fn town_zones(&self) -> &TownZoneCounts {
        &self.town_zones
    }
    /// RNG after path rebuilding, including any border guard art selection.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}
impl<'state, 'zones, 'tiles> MinesPlaced<'state, 'zones, 'tiles> {
    /// Count primary-town zone alignments, then rebuild and open connection paths.
    ///
    /// # Errors
    /// Reports context, count overflow, path/predecessor, prototype or allocation faults.
    pub fn prepare_treasure_paths(
        mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<TreasurePaths<'state, 'zones, 'tiles>, ConnectionError> {
        self.map_mut().prepare_object_context(objects, catalog)?;
        let mut town_zones = TownZoneCounts::default();
        for zone in self.map().coverage().map().zones() {
            if zone.primary_town().is_some() {
                town_zones.add(zone.alignment())?;
            }
        }
        self.map_mut()
            .build_zone_connection_paths(objects, catalog, rng)?;
        Ok(TreasurePaths {
            mines: self,
            town_zones,
            rng: rng.checkpoint(),
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn neutral_primary_town_aliases_total_and_signed_adjustment_keeps_native_rounding() {
        let castle = Town::parse(raw::TOWN_CASTLE).unwrap();
        let mut counts = TownZoneCounts::default();
        assert_eq!(counts.adjust(i32::MAX, Some(castle)).unwrap(), i32::MAX);
        counts.add(Some(castle)).unwrap();
        counts.add(None).unwrap();
        assert_eq!(counts.total(), 3);
        assert_eq!(counts.aligned(Some(castle)), 1);
        assert_eq!(counts.aligned(None), 0);
        assert_eq!(counts.adjust(5, Some(castle)).unwrap(), 6);
        assert_eq!(counts.adjust(-5, Some(castle)).unwrap(), -6);
        counts.add(Some(castle)).unwrap();
        assert!(matches!(
            counts.adjust(i32::MAX / 2 + 1, Some(castle)),
            Err(PlacementError::Arithmetic)
        ));
    }
}
