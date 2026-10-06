//! Ordered treasure offers, nested retries, and guarded local group assembly.
use super::{
    ObjectArena, ObstacleEntrances, PendingTreasure, PlacementError, TreasureGeneration,
    TreasureGenerationError, TreasureGroupWorkspace,
};
use crate::{domain::WorldPosition, geometry::ZoneId, raw, rng::RetailRng};
use std::ops::{Range, RangeInclusive};

/// Whether selection favors greater value per occupied footprint cell.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TreasurePacking {
    /// Retain every eligible definition.
    Ordinary,
    /// Prefer more value per occupied cell using native thresholds.
    Compact,
}
#[derive(Clone, Copy)]
pub(super) enum TreasurePurpose {
    First,
    Additional,
    Replacement(WorldPosition),
}
impl TreasureGeneration<'_> {
    #[expect(
        clippy::too_many_lines,
        reason = "selection filters, lazy valuations and draws must stay in native order"
    )]
    pub(super) fn select_treasure(
        &mut self,
        zone: ZoneId,
        range: RangeInclusive<i32>,
        purpose: TreasurePurpose,
        packing: TreasurePacking,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<Option<(PendingTreasure, i32)>, TreasureGenerationError> {
        self.require_arena(objects)?;
        self.ready.map().zone(zone)?;
        self.offers.clear();
        let mut total_weight = 0_i32;
        let mut best_per_cell = 0_i32;
        for (definition, def) in self.ready.catalog.iter() {
            let kind = def.kind();
            let traits = kind.traits();
            if !matches!(purpose, TreasurePurpose::First)
                && traits.blocks_landing()
                && !traits.cleared_on_visit()
            {
                continue;
            }
            if matches!(purpose, TreasurePurpose::Replacement(_))
                && def.reward().requires_linked_placement()
            {
                continue;
            }
            if self.ready.map().object_count(kind) >= raw::MAP_OBJECT_LIMITS[kind.index()]
                || self
                    .ready
                    .map()
                    .zone_object_count(zone, kind)
                    .ok_or(PlacementError::UnknownZone(zone))?
                    >= raw::ZONE_OBJECT_LIMITS[kind.index()]
            {
                continue;
            }
            let value = self.ready.value(definition, zone)?;
            if value < 0 || !range.contains(&value) {
                continue;
            }
            let Some(selected) = self.choose_prototype(definition, zone, rng)? else {
                continue;
            };
            let entry = self
                .ready
                .catalog
                .prototypes()
                .get(selected.prototype())
                .expect("selected catalog art");
            if let TreasurePurpose::Replacement(position) = purpose {
                if position.point.x >= 0
                    && self.ready.map().footprint_blocked(
                        entry,
                        position,
                        Some(zone),
                        ObstacleEntrances::Reject,
                    )?
                {
                    continue;
                }
            }
            if packing == TreasurePacking::Compact {
                let occupied = i32::try_from(
                    entry
                        .image_mask()
                        .size()
                        .map_err(PlacementError::from)?
                        .cells()
                        .filter(|&cell| entry.prototype().occupies(cell))
                        .count(),
                )
                .map_err(|_| PlacementError::Arithmetic)?;
                let per_cell = value
                    .checked_div(occupied)
                    .ok_or(PlacementError::Arithmetic)?;
                if per_cell
                    < best_per_cell
                        .checked_mul(3)
                        .ok_or(PlacementError::Arithmetic)?
                        / 4
                {
                    continue;
                }
                if best_per_cell < per_cell.checked_mul(3).ok_or(PlacementError::Arithmetic)? / 4 {
                    total_weight = 0;
                    self.offers.clear();
                    best_per_cell = per_cell;
                }
            }
            let density =
                i32::try_from(def.density().get()).map_err(|_| PlacementError::Arithmetic)?;
            total_weight = total_weight
                .checked_add(density)
                .ok_or(PlacementError::Arithmetic)?;
            self.offers.try_reserve(1).map_err(PlacementError::from)?;
            self.offers.push(selected);
        }
        if self.offers.is_empty() {
            return Ok(None);
        }
        let mut remaining = i32::try_from(rng.draw()).unwrap() % total_weight;
        for &selected in &self.offers {
            remaining -= i32::try_from(
                self.ready
                    .catalog
                    .get(selected.definition())
                    .unwrap()
                    .density()
                    .get(),
            )
            .unwrap();
            if remaining < 0 {
                let value = self.ready.value(selected.definition(), zone)?;
                return Ok(self
                    .generate(selected, objects, rng)?
                    .map(|pending| (pending, value)));
            }
        }
        unreachable!("positive weights cover the roulette draw")
    }
    fn treasure_with_retries(
        &mut self,
        zone: ZoneId,
        range: RangeInclusive<i32>,
        purpose: TreasurePurpose,
        packing: TreasurePacking,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<Option<(PendingTreasure, i32)>, TreasureGenerationError> {
        for _ in 0..raw::RMG_TREASURE_ATTEMPTS {
            if let Some(treasure) =
                self.select_treasure(zone, range.clone(), purpose, packing, objects, rng)?
            {
                return Ok(Some(treasure));
            }
        }
        Ok(None)
    }
    pub(super) fn fill_group(
        &mut self,
        group: &mut TreasureGroupWorkspace,
        zone: ZoneId,
        target: i32,
        packing: TreasurePacking,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<i32, TreasureGenerationError> {
        let Some((first, mut total)) = self.treasure_with_retries(
            zone,
            target / 4..=target,
            TreasurePurpose::First,
            packing,
            objects,
            rng,
        )?
        else {
            return Ok(0);
        };
        if let Err(rejected) = self.add_centered_to_group(group, first, objects) {
            self.discard(rejected.pending, objects)?;
            return Err(rejected.error);
        }
        while total < target {
            let remainder = target
                .checked_sub(total)
                .ok_or(PlacementError::Arithmetic)?;
            if remainder < i32::try_from(raw::RMG_TREASURE_MINIMUM_REMAINDER).unwrap()
                && remainder < total / 2
            {
                break;
            }
            let range =
                remainder / 4..=remainder.checked_mul(5).ok_or(PlacementError::Arithmetic)? / 4;
            let mut added = None;
            for _ in 0..raw::RMG_TREASURE_ATTEMPTS {
                let Some((pending, value)) = self.treasure_with_retries(
                    zone,
                    range.clone(),
                    TreasurePurpose::Additional,
                    packing,
                    objects,
                    rng,
                )?
                else {
                    break;
                };
                match self.try_add_to_group(group, pending, objects, rng) {
                    Ok(None) => {
                        added = Some(value);
                        break;
                    }
                    Ok(Some(rejected)) => self.discard(rejected, objects)?,
                    Err(rejected) => {
                        self.discard(rejected.pending, objects)?;
                        return Err(rejected.error);
                    }
                }
            }
            let Some(value) = added else {
                break;
            };
            total = total.checked_add(value).ok_or(PlacementError::Arithmetic)?;
        }
        group.update_bounds();
        Ok(total)
    }
    /// Assemble treasures and an optional guard on the reusable scratch map.
    /// A failed guard fit discards its treasures. A zero-valued group can return
    /// false while retaining objects, which still require explicit discard.
    /// The bounds follow template bands: the upper value is excluded when it
    /// exceeds the lower value; otherwise the upper value is used without a draw.
    ///
    /// # Errors
    /// Reports an undisposed previous group or a native selection/geometry fault.
    #[expect(
        clippy::missing_panics_doc,
        reason = "the RNG draw is 15-bit and all later identities are admitted before use"
    )]
    pub fn assemble_group(
        &mut self,
        group: &mut TreasureGroupWorkspace,
        zone: ZoneId,
        value_bounds: Range<i32>,
        packing: TreasurePacking,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<bool, TreasureGenerationError> {
        self.check_group(group, objects)?;
        self.ready.map().zone(zone)?;
        if !group.objects.is_empty() {
            return Err(TreasureGenerationError::GroupNotEmpty);
        }
        group.reset_after_disposal();
        let (minimum, maximum) = (value_bounds.start, value_bounds.end);
        let target = if maximum <= minimum {
            maximum
        } else {
            let count = maximum
                .checked_sub(minimum)
                .ok_or(PlacementError::Arithmetic)?;
            (i32::try_from(rng.draw()).unwrap() % count)
                .checked_add(minimum)
                .ok_or(PlacementError::Arithmetic)?
        };
        let total = self.fill_group(group, zone, target, packing, objects, rng)?;
        if total == 0 {
            return Ok(false);
        }
        let value = self
            .ready
            .map()
            .zone_guard_value(total, self.ready.map().zone(zone)?)?;
        if value > 0 {
            let catalog = self.ready.catalog.prototypes();
            let guard = self.ready.paths.map_mut().create_guard(
                value,
                zone,
                objects,
                catalog,
                self.ready.catalog.creatures(),
                rng,
            )?;
            self.arena = objects.owner();
            if let Some(guard) = guard {
                match group.add_guard(guard, objects, catalog, rng) {
                    Ok(true) => {}
                    Ok(false) => {
                        self.discard_group(group, objects)?;
                        objects.discard_unplaced(guard)?;
                        return Ok(false);
                    }
                    Err(error) => {
                        if !group.objects().any(|id| id == guard) {
                            objects.discard_unplaced(guard)?;
                        }
                        return Err(error.into());
                    }
                }
            }
        }
        group.prepare_placement()?;
        Ok(true)
    }
}
