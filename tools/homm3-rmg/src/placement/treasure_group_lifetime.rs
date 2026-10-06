//! Treasure tokens enter and leave exclusive scratch ownership through their generation.
use super::{
    ObjectArena, PendingTreasure, PlacementError, TreasureGeneration, TreasureGenerationError,
    TreasureGroupWorkspace,
};
use crate::rng::RetailRng;

/// A failed group insertion retains the generated object for retry or cleanup.
#[derive(Debug)]
pub struct RejectedTreasure {
    /// Native fault or context mismatch encountered before ownership transfer.
    pub error: TreasureGenerationError,
    /// Still-owned object; discard explicitly through its generation.
    pub pending: PendingTreasure,
}
impl TreasureGeneration<'_> {
    pub(super) fn check_group(
        &self,
        group: &TreasureGroupWorkspace,
        objects: &ObjectArena,
    ) -> Result<(), TreasureGenerationError> {
        self.require_arena(objects)?;
        if let Some(pending) = group
            .objects
            .iter()
            .find_map(super::treasure_groups::GroupObject::pending)
        {
            self.require_definition(pending)?;
        }
        Ok(())
    }
    fn group_entry<'a>(
        &'a self,
        group: &TreasureGroupWorkspace,
        pending: &PendingTreasure,
        objects: &ObjectArena,
    ) -> Result<&'a crate::prototype::PreparedPrototype<'a>, TreasureGenerationError> {
        self.check_group(group, objects)?;
        self.require_definition(pending)?;
        let geometry = objects
            .get(pending.object())
            .ok_or(PlacementError::UnknownObject(pending.object()))?;
        if geometry.position().is_some() {
            return Err(PlacementError::PreviouslyPlaced(pending.object()).into());
        }
        self.ready()
            .catalog()
            .prototypes()
            .get(geometry.prototype())
            .ok_or_else(|| PlacementError::UnknownPrototype(geometry.prototype()).into())
    }
    /// Consume a pending treasure into the group's center without a fit query.
    /// This preserves the source's first-object path and consumes no RNG.
    ///
    /// # Errors
    /// Returns the original ownership token with any context, geometry or storage fault.
    pub fn add_centered_to_group(
        &mut self,
        group: &mut TreasureGroupWorkspace,
        pending: PendingTreasure,
        objects: &mut ObjectArena,
    ) -> Result<(), RejectedTreasure> {
        let anchor = self
            .group_entry(group, &pending, objects)
            .and_then(|entry| TreasureGroupWorkspace::centered(entry).map_err(Into::into));
        let anchor = match anchor {
            Ok(anchor) => anchor,
            Err(error) => return Err(RejectedTreasure { error, pending }),
        };
        group
            .add(
                pending,
                anchor,
                objects,
                self.ready().catalog().prototypes(),
            )
            .map_err(|(error, pending)| RejectedTreasure {
                error: error.into(),
                pending,
            })
    }
    /// Fit beside existing entrances in native order, retaining repeated candidates.
    /// A successful choice draws once, including when there is one candidate.
    /// `None` means the group took ownership; `Some` returns a rejected object.
    ///
    /// # Errors
    /// Returns the original token with any fault. RNG draws are never rewound.
    pub fn try_add_to_group(
        &mut self,
        group: &mut TreasureGroupWorkspace,
        pending: PendingTreasure,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<Option<PendingTreasure>, RejectedTreasure> {
        let candidate = self
            .group_entry(group, &pending, objects)
            .and_then(|entry| {
                group
                    .choose_fit(entry, objects, self.ready().catalog().prototypes(), rng)
                    .map_err(Into::into)
            });
        let candidate = match candidate {
            Ok(Some(candidate)) => candidate,
            Ok(None) => return Ok(Some(pending)),
            Err(error) => return Err(RejectedTreasure { error, pending }),
        };
        group
            .add(
                pending,
                candidate,
                objects,
                self.ready().catalog().prototypes(),
            )
            .map(|()| None)
            .map_err(|(error, pending)| RejectedTreasure {
                error: error.into(),
                pending,
            })
    }
    /// Clear scratch memberships, release reservations and delete owned records in
    /// object order. Retains all buffers, RNG state and consumed serialized IDs.
    ///
    /// # Errors
    /// Rejects a foreign group/arena or stale record before any cleanup.
    pub fn discard_group(
        &mut self,
        group: &mut TreasureGroupWorkspace,
        objects: &mut ObjectArena,
    ) -> Result<(), TreasureGenerationError> {
        self.check_group(group, objects)?;
        if let Some(owner) = group.owner {
            for pending in &group.objects {
                if let Some(pending) = pending.pending() {
                    self.require_definition(pending)?;
                }
                objects.require_group(pending.object(), owner)?;
                Self::check_pending_child(pending.object(), objects)?;
            }
            group.clear_memberships();
            for pending in group.objects.drain(..) {
                self.release_payload(pending.object(), objects)?;
                objects.recycle_group(pending.object(), owner)?;
            }
        }
        group.objects.clear();
        group.reset_after_disposal();
        Ok(())
    }
}
