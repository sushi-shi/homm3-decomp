//! Replacement leaves the removed object's lifetime to its completion callback.
use super::{
    treasure_assembly::TreasurePurpose, ObjectArena, ObjectId, PlacementError, TreasureGeneration,
    TreasureGenerationError, TreasurePacking,
};
use crate::rng::RetailRng;

impl TreasureGeneration<'_, '_, '_, '_, '_, '_, '_> {
    /// Remove a registered object and offer a replacement worth 1–1.5 times its
    /// value at the retained anchor. The removed record stays alive so its owning
    /// completion callback can apply the mode-specific lifetime policy.
    ///
    /// # Errors
    /// Reports native removal, missing zone, arithmetic or factory/placement faults.
    pub fn replace_with_treasure(
        &mut self,
        object: ObjectId,
        value: i32,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<Option<ObjectId>, TreasureGenerationError> {
        self.require_arena(objects)?;
        let position = objects.positioned(object)?.position();
        self.ready.paths.map_mut().unregister_object(
            objects,
            self.ready.catalog.prototypes(),
            object,
        )?;
        let map = self.ready.map();
        let zone = map
            .view()
            .aliased_cell(position)?
            .zone()
            .ok_or(TreasureGenerationError::UnassignedReplacement(position))?;
        let maximum = value.checked_mul(3).ok_or(PlacementError::Arithmetic)? / 2;
        let Some((pending, _)) = self.select_treasure(
            zone,
            value..=maximum,
            TreasurePurpose::Replacement(position),
            TreasurePacking::Ordinary,
            objects,
            rng,
        )?
        else {
            return Ok(None);
        };
        let replacement = pending.object();
        self.ready.paths.map_mut().register_object(
            objects,
            self.ready.catalog.prototypes(),
            replacement,
            position,
        )?;
        Ok(Some(replacement))
    }
}
