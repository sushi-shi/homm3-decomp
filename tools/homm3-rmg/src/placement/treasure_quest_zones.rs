//! Quest destinations ranked through the full directed zone graph.
use super::{
    ObjectArena, PlacementError, TreasureGeneration, TreasureGenerationError,
    TreasureGroupWorkspace,
};
use crate::{
    boundaries::ZoneOrigin, domain::Terrain, geometry::ZoneId, raw, rng::RetailRng,
    template::ZoneRole,
};
impl TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
    pub(super) fn place_quest_group(
        &mut self,
        group: &mut TreasureGroupWorkspace,
        origin: ZoneId,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<bool, TreasureGenerationError> {
        self.ready.map().zone(origin)?;
        let map = self.ready.map().coverage().map();
        let count = map.zones().len();
        group
            .quest_scores
            .try_reserve(count.saturating_sub(group.quest_scores.len()))
            .map_err(PlacementError::from)?;
        group.quest_scores.resize(count, 0);
        group
            .quest_scores
            .fill(i32::try_from(raw::RMG_QUEST_UNREACHED_DISTANCE).unwrap());
        group.quest_scores[origin.index()] = 0;
        group.quest_pending.clear();
        group
            .quest_pending
            .try_reserve(1)
            .map_err(PlacementError::from)?;
        group.quest_pending.push(origin);
        while let Some(current) = group.quest_pending.pop() {
            let distance = group.quest_scores[current.index()]
                .checked_add(1)
                .ok_or(PlacementError::Arithmetic)?;
            for edge in map
                .connections()
                .iter()
                .filter(|edge| edge.source == current)
            {
                if group.quest_scores[edge.destination.index()] <= distance {
                    continue;
                }
                group.quest_scores[edge.destination.index()] = distance;
                let index = group
                    .quest_pending
                    .partition_point(|id| distance < group.quest_scores[id.index()]);
                group
                    .quest_pending
                    .try_reserve(1)
                    .map_err(PlacementError::from)?;
                group.quest_pending.insert(index, edge.destination);
            }
        }
        for score in &mut group.quest_scores {
            let base = if *score == 1 {
                i32::try_from(raw::RMG_QUEST_ADJACENT_ZONE_SCORE).unwrap()
            } else {
                score
                    .checked_mul(i32::try_from(raw::RMG_QUEST_DISTANCE_SCALE).unwrap())
                    .ok_or(PlacementError::Arithmetic)?
            };
            *score = base
                .checked_add(i32::try_from(rng.draw() % raw::RMG_QUEST_DISTANCE_SCALE).unwrap())
                .ok_or(PlacementError::Arithmetic)?;
        }
        group.quest_candidates.clear();
        for zone in map.zones() {
            let junction = match zone.origin() {
                ZoneOrigin::Template(id) => matches!(
                    map.template().zones()[id.index()].role(),
                    ZoneRole::Junction(_)
                ),
                ZoneOrigin::Water => false,
            };
            let score = group.quest_scores[zone.id().index()];
            if zone.id() == origin
                || junction
                || zone.terrain() == Terrain::Water
                || score > i32::try_from(raw::RMG_QUEST_MAXIMUM_SCORE).unwrap()
            {
                continue;
            }
            let at = group
                .quest_candidates
                .partition_point(|id| score >= group.quest_scores[id.index()]);
            group
                .quest_candidates
                .try_reserve(1)
                .map_err(PlacementError::from)?;
            group.quest_candidates.insert(at, zone.id());
        }
        for index in 0..group.quest_candidates.len() {
            if self.place_group(
                group,
                group.quest_candidates[index],
                i32::try_from(raw::RMG_QUEST_GROUP_SPACING).unwrap(),
                objects,
                rng,
            )? {
                return Ok(true);
            }
        }
        Ok(false)
    }
}
