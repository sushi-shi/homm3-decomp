//! Quest and tent callbacks, including nested placement and retail retained roots.
use super::{
    ObjectArena, ObjectId, ObjectPayload, PlacementError, TreasureGeneration,
    TreasureGenerationError, TreasureGroupWorkspace, TreasurePacking,
};
use crate::{geometry::ZoneId, object::ObjectKind, raw, rng::RetailRng, traits::ArtifactId};

impl TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
    pub(super) fn complete_treasure(
        &mut self,
        object: ObjectId,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<(), TreasureGenerationError> {
        match *objects
            .payload(object)
            .ok_or(PlacementError::UnknownObject(object))?
        {
            ObjectPayload::QuestArtifact(quest) => {
                let Some(child) = quest.pending_seer() else {
                    return Ok(());
                };
                let available = (0..raw::ARTIFACT_COUNT)
                    .filter(|&id| self.artifact_available(ArtifactId::parse(id).unwrap()))
                    .count();
                if available < raw::RMG_LOW_QUEST_ARTIFACT_COUNT as usize {
                    self.ready.quests.pool_low = true;
                }
                if available == 0 {
                    objects.recycle_unplaced(child)?;
                } else {
                    let selected = rng.draw() as usize % available;
                    let artifact = (0..raw::ARTIFACT_COUNT)
                        .map(|id| ArtifactId::parse(id).unwrap())
                        .filter(|&id| self.artifact_available(id))
                        .nth(selected)
                        .unwrap();
                    let ObjectPayload::Seer(hut) = objects.payload_mut(child)? else {
                        return Err(PlacementError::TreasureCleanupRequired(child).into());
                    };
                    hut.artifact = Some(artifact);
                    let prototype = self
                        .ready
                        .catalog
                        .prototypes()
                        .first_subtype(
                            kind(raw::ARTIFACT),
                            i32::try_from(artifact.index()).unwrap(),
                        )
                        .ok_or(TreasureGenerationError::MissingArtifact(artifact))?;
                    objects.swap_prototype(object, self.ready.catalog.prototypes(), prototype)?;
                    // Entrance type is cached in Rust, while native reads it from
                    // the first member's current prototype. Refresh only that cache;
                    // keep the old footprint, counts and distance state untouched.
                    let map = self.ready.paths.map_mut();
                    for cell in map.cells.iter_mut() {
                        if cell.entrance.is_some()
                            && map.memberships.first(cell.objects) == Some(object)
                        {
                            cell.entrance = Some(kind(raw::ARTIFACT));
                        }
                    }
                    let origin = self.object_zone(object, objects)?;
                    let placed = self.with_nested_group(|generation, group| {
                        group.add_direct(child, objects, generation.ready.catalog.prototypes())?;
                        group.prepare_placement()?;
                        if generation.place_quest_group(group, origin, objects, rng)? {
                            return Ok(true);
                        }
                        let value = generation.ready.value(quest.definition(), origin)?;
                        generation.replace_with_treasure(object, value, objects, rng)?;
                        generation.discard_group(group, objects)?;
                        Ok(false)
                    })?;
                    if placed {
                        self.ready.quests.used[artifact.index()] = true;
                        let count = i32::try_from(
                            self.ready
                                .catalog
                                .prototypes()
                                .family(kind(raw::SEER))
                                .len(),
                        )
                        .map_err(|_| PlacementError::Arithmetic)?;
                        self.ready.quests.next_seer = self
                            .ready
                            .quests
                            .next_seer
                            .checked_add(1)
                            .and_then(|next| next.checked_rem(count))
                            .ok_or(PlacementError::Arithmetic)?;
                    }
                }
                let ObjectPayload::QuestArtifact(quest) = objects.payload_mut(object)? else {
                    unreachable!("completion retains parent payload")
                };
                quest.seer = None;
                self.retire_removed_parent(object, objects)?;
            }
            ObjectPayload::KeyTent(value) => {
                let target = value.checked_mul(3).ok_or(PlacementError::Arithmetic)? / 2;
                if !self.place_tent_guard(object, target, objects, rng)? {
                    self.replace_with_treasure(object, value, objects, rng)?;
                    self.retire_removed_parent(object, objects)?;
                }
            }
            _ => {}
        }
        Ok(())
    }
    fn artifact_available(&self, artifact: ArtifactId) -> bool {
        self.artifacts().get(artifact).quest_eligible() && !self.ready.quests.used[artifact.index()]
    }
    fn retire_removed_parent(
        &mut self,
        object: ObjectId,
        objects: &mut ObjectArena,
    ) -> Result<(), PlacementError> {
        if !self.ready.map().active_objects().contains(&object) {
            objects.retire(
                object,
                !self.ready.map().coverage().map().behavior().is_hotfix(),
            )?;
        }
        Ok(())
    }
    fn object_zone(
        &self,
        object: ObjectId,
        objects: &ObjectArena,
    ) -> Result<ZoneId, TreasureGenerationError> {
        let position = objects.positioned(object)?.position();
        let view = self.ready.map().view();
        view.aliased_cell(position)?
            .zone()
            .ok_or(TreasureGenerationError::UnassignedReplacement(position))
    }
    fn with_nested_group<T>(
        &mut self,
        action: impl FnOnce(
            &mut Self,
            &mut TreasureGroupWorkspace,
        ) -> Result<T, TreasureGenerationError>,
    ) -> Result<T, TreasureGenerationError> {
        let mut group = self.nested_groups.pop().unwrap_or_default();
        let result = if group.objects.is_empty() {
            action(self, &mut group)
        } else {
            Err(TreasureGenerationError::GroupNotEmpty)
        };
        self.nested_groups
            .try_reserve(1)
            .map_err(PlacementError::from)?;
        self.nested_groups.push(group);
        result
    }
    fn place_tent_guard(
        &mut self,
        object: ObjectId,
        target: i32,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<bool, TreasureGenerationError> {
        let geometry = objects
            .get(object)
            .ok_or(PlacementError::UnknownObject(object))?;
        let catalog = self.ready.catalog.prototypes();
        let color = catalog
            .get(geometry.prototype())
            .ok_or(PlacementError::UnknownPrototype(geometry.prototype()))?
            .prototype()
            .subtype();
        let Some(prototype) = catalog.first_subtype(kind(raw::BORDER_GUARD), color) else {
            return Ok(false);
        };
        let origin = self.object_zone(object, objects)?;
        self.with_nested_group(|generation, group| {
            let guard = objects.create(generation.ready.catalog.prototypes(), prototype)?;
            let catalog = generation.ready.catalog.prototypes();
            let color = generation
                .ready
                .paths
                .map_mut()
                .key_tent_color(catalog, color)?;
            generation
                .ready
                .paths
                .map_mut()
                .set_key_tent_disabled(catalog, color, true)?;
            let filled = generation.fill_group(
                group,
                origin,
                target,
                TreasurePacking::Ordinary,
                objects,
                rng,
            )? != 0;
            let guarded = filled
                && group.add_guard(guard, objects, generation.ready.catalog.prototypes(), rng)?;
            if guarded {
                group.prepare_placement()?;
                if generation.place_quest_group(group, origin, objects, rng)? {
                    return Ok(true);
                }
            } else {
                objects.discard_unplaced(guard)?;
            }
            generation.discard_group(group, objects)?;
            generation.ready.paths.map_mut().set_key_tent_disabled(
                generation.ready.catalog.prototypes(),
                color,
                false,
            )?;
            Ok(false)
        })
    }
}
fn kind(value: u32) -> ObjectKind {
    ObjectKind::parse(i32::try_from(value).unwrap()).unwrap()
}
