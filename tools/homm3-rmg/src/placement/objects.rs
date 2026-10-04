//! Stable object geometry and ordered per-cell membership, without per-cell heaps.

use super::{registration::entrance_position, Fort, PlacementError, TownPayload};
use crate::{
    domain::WorldPosition,
    identity::OwnerId,
    object::ObjectKind,
    prototype::{GuardStack, PrototypeCatalog, PrototypeId},
    raw,
    selection::Player,
};
use std::{collections::TryReserveError, num::NonZeroUsize};

/// Native serialized object counter, shared by towns, monsters and prisons.
/// Distinct from arena and prototype identities.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct MapObjectId(pub(super) i32);
impl MapObjectId {
    /// Source counter value assigned when the payload is constructed.
    #[must_use]
    pub const fn value(self) -> i32 {
        self.0
    }
}

/// Geometry identity, distinct from serialized town/monster IDs and prototype IDs.
/// Valid until discarded or the owning arena is reset; moving preserves it.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ObjectId {
    index: usize,
    owner: OwnerId,
    generation: usize,
}
impl ObjectId {
    /// Reusable record-slot index, not a serialized counter or a complete identity.
    #[must_use]
    pub const fn index(self) -> usize {
        self.index
    }
}

/// Geometry shared by world and temporary treasure-group maps.
#[derive(Clone, Copy, Debug)]
pub struct ObjectGeometry {
    prototype: PrototypeId,
    kind: ObjectKind,
    position: Option<WorldPosition>,
}
impl ObjectGeometry {
    /// Prepared prototype supplying this object's footprint.
    #[must_use]
    pub const fn prototype(self) -> PrototypeId {
        self.prototype
    }
    /// Last assigned anchor; removal preserves it, and creation has no position.
    #[must_use]
    pub const fn position(self) -> Option<WorldPosition> {
        self.position
    }
    /// Adventure-object identity used by entrance queries.
    #[must_use]
    pub const fn kind(self) -> ObjectKind {
        self.kind
    }
}

/// A monster's native serialized payload. Position and prototype live in geometry.
#[derive(Clone, Copy, Debug)]
pub struct MonsterPayload {
    id: MapObjectId,
    count: i32,
}
impl MonsterPayload {
    /// Counter shared with towns and prisons, including on `RoE` maps.
    #[must_use]
    pub const fn id(self) -> MapObjectId {
        self.id
    }
    /// Native signed quantity; its low two bytes are serialized.
    #[must_use]
    pub const fn count(self) -> i32 {
        self.count
    }
    /// The source guard constructor's fixed disposition.
    #[must_use]
    pub const fn disposition(self) -> u32 {
        raw::RMG_GUARD_DISPOSITION
    }
}

/// Payload owned by the same arena record as an object's geometry.
/// A base object uses only prototype and position when serialized.
#[derive(Clone, Copy, Debug)]
pub enum ObjectPayload {
    /// Plain native `TRmgObject`, without additional serialized fields.
    Base,
    /// Artifact with native default treasure fields.
    Artifact,
    /// Scholar choosing its award when the map is played.
    Scholar,
    /// Shrine with a random spell sentinel.
    Shrine,
    /// Witch hut with the native skill-mask policy.
    WitchHut,
    /// Scroll with a spell selected during generation.
    Scroll(crate::traits::SpellId),
    /// Pandora reward; all unrelated native fields remain zero.
    Pandora(super::PandoraReward),
    /// Prison hero and reservation.
    Prison(super::PrisonPayload),
    /// Tent whose guard completion uses this definition value.
    KeyTent(i32),
    /// Pending or placed seer hut reward and quest artifact.
    Seer(super::SeerPayload),
    /// Artifact owning an unplaced seer hut until completion.
    QuestArtifact(super::QuestArtifactPayload),
    /// Unowned capturable object; serializes the native unowned byte and padding.
    Ownable,
    /// Resource pile with native default amount and no custom treasure.
    Resource,
    /// Player ownership and fort, with its shared native object ID.
    Town(TownPayload),
    /// Guard stack count, disposition and shared native object ID.
    Monster(MonsterPayload),
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Ownership {
    Unplaced,
    Group(OwnerId),
    Published,
    Retained,
}
#[derive(Clone, Copy, Debug)]
struct ObjectRecord {
    ownership: Ownership,
    geometry: ObjectGeometry,
    payload: ObjectPayload,
}

#[derive(Debug)]
enum ObjectSlot {
    Occupied {
        generation: usize,
        record: ObjectRecord,
    },
    Vacant {
        next: Option<usize>,
    },
}

/// Contiguous geometry and payload records shared by temporary and world maps.
/// Map membership never owns or copies a payload; removal retains its arena record.
#[derive(Default, Debug)]
pub struct ObjectArena {
    records: Vec<ObjectSlot>,
    owner: Option<OwnerId>,
    free: Option<usize>,
    next_generation: usize,
}
impl ObjectArena {
    pub(super) const fn owner(&self) -> Option<OwnerId> {
        self.owner
    }
    /// Clear a completed generation, invalidating its IDs and retaining capacity.
    pub fn reset(&mut self) {
        self.records.clear();
        self.owner = None;
        self.free = None;
        self.next_generation = 0;
    }
    /// Allocate a stable geometry identity without allocating an individual object.
    ///
    /// # Errors
    /// Reports an ID outside the supplied catalog or a failed arena reservation.
    pub fn create(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        prototype: PrototypeId,
    ) -> Result<ObjectId, PlacementError> {
        self.create_record(catalog, prototype, ObjectPayload::Base)
    }
    pub(super) fn create_shipyard(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        prototype: PrototypeId,
    ) -> Result<ObjectId, PlacementError> {
        Self::require_kind(catalog, prototype, raw::SHIPYARD)?;
        self.create_record(catalog, prototype, ObjectPayload::Ownable)
    }
    pub(super) fn create_mine(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        prototype: PrototypeId,
    ) -> Result<ObjectId, PlacementError> {
        Self::require_kind(catalog, prototype, raw::MINE)?;
        self.create_record(catalog, prototype, ObjectPayload::Ownable)
    }
    pub(super) fn create_resource(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        prototype: PrototypeId,
    ) -> Result<ObjectId, PlacementError> {
        Self::require_kind(catalog, prototype, raw::RESOURCE)?;
        self.create_record(catalog, prototype, ObjectPayload::Resource)
    }
    pub(super) fn create_town(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        prototype: PrototypeId,
        id: MapObjectId,
        owner: Option<Player>,
        fort: Fort,
    ) -> Result<ObjectId, PlacementError> {
        Self::require_kind(catalog, prototype, raw::TOWN)?;
        self.create_record(
            catalog,
            prototype,
            ObjectPayload::Town(TownPayload::new(id, owner, fort)),
        )
    }
    pub(super) fn create_monster(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        stack: GuardStack,
        id: MapObjectId,
    ) -> Result<ObjectId, PlacementError> {
        Self::require_kind(catalog, stack.prototype(), raw::MONSTER)?;
        self.create_record(
            catalog,
            stack.prototype(),
            ObjectPayload::Monster(MonsterPayload {
                id,
                count: stack.count(),
            }),
        )
    }
    fn require_kind(
        catalog: &PrototypeCatalog<'_>,
        prototype: PrototypeId,
        expected: u32,
    ) -> Result<(), PlacementError> {
        let actual = catalog
            .get(prototype)
            .ok_or(PlacementError::UnknownPrototype(prototype))?
            .prototype()
            .kind();
        let expected = ObjectKind::parse(i32::try_from(expected).unwrap()).unwrap();
        if actual != expected {
            return Err(PlacementError::PayloadKind { expected, actual });
        }
        Ok(())
    }
    // Reserve before a factory claims its native serialized ID. A failed
    // allocation must not consume that ID; a preceding hero claim is retained.
    pub(super) fn reserve_record(&mut self) -> Result<(), PlacementError> {
        if self.free.is_none() {
            self.records.try_reserve(1)?;
        }
        Ok(())
    }
    pub(super) fn create_record(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        prototype: PrototypeId,
        payload: ObjectPayload,
    ) -> Result<ObjectId, PlacementError> {
        let entry = catalog
            .get(prototype)
            .ok_or(PlacementError::UnknownPrototype(prototype))?;
        let owner = if let Some(owner) = self.owner {
            owner
        } else {
            let owner = OwnerId::new().ok_or(PlacementError::IdentityExhausted)?;
            self.owner = Some(owner);
            owner
        };
        let generation = self.next_generation;
        let next_generation = generation
            .checked_add(1)
            .ok_or(PlacementError::IdentityExhausted)?;
        let slot = ObjectSlot::Occupied {
            generation,
            record: ObjectRecord {
                ownership: Ownership::Unplaced,
                geometry: ObjectGeometry {
                    prototype,
                    kind: entry.prototype().kind(),
                    position: None,
                },
                payload,
            },
        };
        let index = if let Some(index) = self.free {
            let ObjectSlot::Vacant { next } = self.records[index] else {
                unreachable!("free list contains only vacant slots")
            };
            self.free = next;
            self.records[index] = slot;
            index
        } else {
            self.records.try_reserve(1)?;
            let index = self.records.len();
            self.records.push(slot);
            index
        };
        self.next_generation = next_generation;
        Ok(ObjectId {
            index,
            owner,
            generation,
        })
    }
    fn record(&self, id: ObjectId) -> Option<&ObjectRecord> {
        if Some(id.owner) != self.owner {
            return None;
        }
        match self.records.get(id.index)? {
            ObjectSlot::Occupied { generation, record } if *generation == id.generation => {
                Some(record)
            }
            _ => None,
        }
    }
    /// Read geometry if the full ID still identifies a live record.
    #[must_use]
    pub fn get(&self, id: ObjectId) -> Option<&ObjectGeometry> {
        self.record(id).map(|record| &record.geometry)
    }
    /// Read a typed payload using the same checked identity as its geometry.
    #[must_use]
    pub fn payload(&self, id: ObjectId) -> Option<&ObjectPayload> {
        self.record(id).map(|record| &record.payload)
    }
    /// Release a never-placed object after a failed placement attempt.
    /// Reuses its slot without allowing old IDs to identify a replacement.
    /// An object removed from a map keeps its position and cannot be discarded:
    /// another temporary or world map may still reference it.
    ///
    /// # Errors
    /// Reports a stale/foreign ID, prior placement, or a payload whose reservation
    /// or child ownership requires the treasure generation cleanup path.
    pub fn discard_unplaced(&mut self, id: ObjectId) -> Result<(), PlacementError> {
        let record = self.record(id).ok_or(PlacementError::UnknownObject(id))?;
        if record.ownership != Ownership::Unplaced {
            return Err(PlacementError::PreviouslyPlaced(id));
        }
        if matches!(
            record.payload,
            ObjectPayload::Prison(_) | ObjectPayload::QuestArtifact(_) | ObjectPayload::Seer(_)
        ) {
            return Err(PlacementError::TreasureCleanupRequired(id));
        }
        self.recycle_unplaced(id)
    }
    pub(super) fn recycle_unplaced(&mut self, id: ObjectId) -> Result<(), PlacementError> {
        let record = self.record(id).ok_or(PlacementError::UnknownObject(id))?;
        if record.ownership != Ownership::Unplaced {
            return Err(PlacementError::PreviouslyPlaced(id));
        }
        self.records[id.index] = ObjectSlot::Vacant { next: self.free };
        self.free = Some(id.index);
        Ok(())
    }
    fn record_mut(&mut self, id: ObjectId) -> Result<&mut ObjectRecord, PlacementError> {
        self.record(id).ok_or(PlacementError::UnknownObject(id))?;
        let ObjectSlot::Occupied { record, .. } = &mut self.records[id.index] else {
            unreachable!("checked live record")
        };
        Ok(record)
    }
    pub(super) fn require_world_insertion(&self, id: ObjectId) -> Result<(), PlacementError> {
        let record = self.record(id).ok_or(PlacementError::UnknownObject(id))?;
        if matches!(record.ownership, Ownership::Group(_)) {
            return Err(PlacementError::GroupOwned(id));
        }
        Ok(())
    }
    pub(super) fn publish(&mut self, id: ObjectId) {
        self.record_mut(id)
            .expect("insertion checked live record")
            .ownership = Ownership::Published;
    }
    pub(super) fn claim_group(
        &mut self,
        id: ObjectId,
        group: OwnerId,
    ) -> Result<(), PlacementError> {
        let record = self.record_mut(id)?;
        if record.ownership != Ownership::Unplaced {
            return Err(PlacementError::PreviouslyPlaced(id));
        }
        record.ownership = Ownership::Group(group);
        Ok(())
    }
    pub(super) fn require_group(&self, id: ObjectId, group: OwnerId) -> Result<(), PlacementError> {
        let record = self.record(id).ok_or(PlacementError::UnknownObject(id))?;
        if record.ownership != Ownership::Group(group) {
            return Err(PlacementError::PreviouslyPlaced(id));
        }
        Ok(())
    }
    // Only the owning group may call this after clearing all scratch memberships.
    pub(super) fn recycle_group(
        &mut self,
        id: ObjectId,
        group: OwnerId,
    ) -> Result<(), PlacementError> {
        self.require_group(id, group)?;
        self.records[id.index] = ObjectSlot::Vacant { next: self.free };
        self.free = Some(id.index);
        Ok(())
    }
    pub(super) fn payload_mut(
        &mut self,
        id: ObjectId,
    ) -> Result<&mut ObjectPayload, PlacementError> {
        Ok(&mut self.record_mut(id)?.payload)
    }
    pub(super) fn swap_prototype(
        &mut self,
        id: ObjectId,
        catalog: &PrototypeCatalog<'_>,
        prototype: PrototypeId,
    ) -> Result<(), PlacementError> {
        let entry = catalog
            .get(prototype)
            .ok_or(PlacementError::UnknownPrototype(prototype))?;
        let record = self.record_mut(id)?;
        record.geometry.prototype = prototype;
        record.geometry.kind = entry.prototype().kind();
        Ok(())
    }
    // Completion proved that this parent is absent from the active world list.
    // Retail keeps the entire record: old footprint membership can still refer
    // to it after a quest prototype swap, and its prototype ref survives output.
    pub(super) fn retire(&mut self, id: ObjectId, retain: bool) -> Result<(), PlacementError> {
        let record = self.record_mut(id)?;
        if retain {
            record.ownership = Ownership::Retained;
        } else {
            self.records[id.index] = ObjectSlot::Vacant { next: self.free };
            self.free = Some(id.index);
        }
        Ok(())
    }
    /// One reference per live record, including pending children and retained objects.
    /// Map memberships do not contribute additional prototype references.
    pub fn referenced_prototypes(&self) -> impl Iterator<Item = PrototypeId> + '_ {
        self.records.iter().filter_map(|slot| match slot {
            ObjectSlot::Occupied { record, .. } => Some(record.geometry.prototype),
            ObjectSlot::Vacant { .. } => None,
        })
    }
    /// Prototype references retained by retail's removed-but-not-deleted objects.
    /// Output includes these alongside references from active map objects.
    pub fn retained_prototypes(&self) -> impl Iterator<Item = PrototypeId> + '_ {
        self.records.iter().filter_map(|slot| match slot {
            ObjectSlot::Occupied { record, .. } if record.ownership == Ownership::Retained => {
                Some(record.geometry.prototype)
            }
            _ => None,
        })
    }
    /// Derive the current entrance from the anchor and its owning prototype.
    /// Unplaced records have no entrance.
    ///
    /// # Errors
    /// Reports foreign IDs/catalogs or unsafe native coordinate arithmetic.
    pub fn entrance(
        &self,
        catalog: &PrototypeCatalog<'_>,
        id: ObjectId,
    ) -> Result<Option<WorldPosition>, PlacementError> {
        let geometry = self.get(id).ok_or(PlacementError::UnknownObject(id))?;
        let prototype = catalog
            .get(geometry.prototype)
            .ok_or(PlacementError::UnknownPrototype(geometry.prototype))?
            .prototype();
        geometry
            .position
            .map(|position| entrance_position(prototype, position))
            .transpose()
    }
    pub(super) fn set_position(&mut self, id: ObjectId, position: WorldPosition) {
        assert_eq!(Some(id.owner), self.owner);
        let ObjectSlot::Occupied { generation, record } = &mut self.records[id.index] else {
            unreachable!("placement checked a live object")
        };
        assert_eq!(*generation, id.generation);
        record.geometry.position = Some(position);
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct LinkId(NonZeroUsize);
impl LinkId {
    fn index(self) -> usize {
        self.0.get() - 1
    }
}

// Both ends exist together or neither does. IDs stay private to the link arena.
#[derive(Clone, Copy, Debug, Default)]
pub(super) struct Chain(Option<(LinkId, LinkId)>);
#[derive(Clone, Copy, Debug)]
struct Link {
    object: ObjectId,
    next: Option<LinkId>,
}
#[derive(Debug)]
enum Slot {
    Used(Link),
    Free(Option<LinkId>),
}
#[derive(Debug, Default)]
pub(super) struct Memberships {
    slots: Vec<Slot>,
    free: Option<LinkId>,
    free_count: usize,
    owner: Option<OwnerId>,
}
impl Memberships {
    pub(super) fn reset(&mut self) {
        self.slots.clear();
        self.free = None;
        self.free_count = 0;
        self.owner = None;
    }
    pub(super) fn accepts_arena(&self, arena: &ObjectArena) -> bool {
        self.owner.is_none() || self.owner == arena.owner()
    }
    pub(super) fn accepts(&self, object: ObjectId) -> bool {
        self.owner.is_none_or(|owner| owner == object.owner)
    }
    pub(super) fn bind(&mut self, object: ObjectId) {
        debug_assert!(self.accepts(object));
        self.owner = Some(object.owner);
    }
    pub(super) fn reserve(&mut self, additional: usize) -> Result<(), TryReserveError> {
        self.slots
            .try_reserve(additional.saturating_sub(self.free_count))
    }
    fn link(&self, id: LinkId) -> &Link {
        let Slot::Used(link) = &self.slots[id.index()] else {
            unreachable!("live chains contain only used links")
        };
        link
    }
    fn link_mut(&mut self, id: LinkId) -> &mut Link {
        let Slot::Used(link) = &mut self.slots[id.index()] else {
            unreachable!("live chains contain only used links")
        };
        link
    }
    pub(super) fn first(&self, chain: Chain) -> Option<ObjectId> {
        chain.0.map(|(head, _)| self.link(head).object)
    }
    pub(super) fn append(&mut self, chain: &mut Chain, object: ObjectId) {
        self.bind(object);
        let link = Link { object, next: None };
        let id = if let Some(id) = self.free {
            let Slot::Free(next) = self.slots[id.index()] else {
                unreachable!("free-list invariant")
            };
            self.free = next;
            self.free_count -= 1;
            self.slots[id.index()] = Slot::Used(link);
            id
        } else {
            // Vec cannot contain usize::MAX nonzero-sized slots.
            let id = LinkId(NonZeroUsize::new(self.slots.len() + 1).unwrap());
            self.slots.push(Slot::Used(link));
            id
        };
        chain.0 = Some(if let Some((head, tail)) = chain.0 {
            self.link_mut(tail).next = Some(id);
            (head, id)
        } else {
            (id, id)
        });
    }
    /// Erase the first occurrence, like `std::find` followed by `vector::erase`.
    pub(super) fn remove(&mut self, chain: &mut Chain, object: ObjectId) -> bool {
        let Some((head, tail)) = chain.0 else {
            return false;
        };
        let mut previous = None;
        let mut cursor = Some(head);
        while let Some(id) = cursor {
            let link = *self.link(id);
            if link.object == object {
                if let Some(previous) = previous {
                    self.link_mut(previous).next = link.next;
                }
                let new_head = if id == head { link.next } else { Some(head) };
                chain.0 = new_head.map(|head| {
                    (
                        head,
                        if id == tail {
                            previous.unwrap_or(head)
                        } else {
                            tail
                        },
                    )
                });
                self.slots[id.index()] = Slot::Free(self.free);
                self.free = Some(id);
                self.free_count += 1;
                return true;
            }
            previous = Some(id);
            cursor = link.next;
        }
        false
    }
    pub(super) fn iter(&self, chain: Chain) -> impl Iterator<Item = ObjectId> + '_ {
        let mut cursor = chain.0.map(|(head, _)| head);
        std::iter::from_fn(move || {
            let link = self.link(cursor?);
            cursor = link.next;
            Some(link.object)
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn stable_membership_order_and_first_duplicate_removal_reuse_freed_slots() {
        let owner = OwnerId::new().unwrap();
        let object = |index| ObjectId {
            index,
            owner,
            generation: 0,
        };
        let mut arena = Memberships::default();
        let mut chain = Chain::default();
        arena.reserve(3).unwrap();
        for index in [0, 1, 0] {
            arena.append(&mut chain, object(index));
        }
        assert!(arena.iter(chain).eq([object(0), object(1), object(0)]));
        let address = arena.slots.as_ptr();
        assert!(arena.remove(&mut chain, object(0)));
        assert!(arena.iter(chain).eq([object(1), object(0)]));
        arena.reserve(1).unwrap();
        arena.append(&mut chain, object(2));
        assert_eq!(arena.slots.as_ptr(), address);
        assert_eq!(arena.slots.len(), 3);
        assert!(arena.remove(&mut chain, object(2))); // Tail.
        assert!(arena.remove(&mut chain, object(0))); // Tail again.
        assert!(arena.remove(&mut chain, object(1))); // Last node.
        assert!(chain.0.is_none());
        assert!(!arena.remove(&mut chain, object(1)));
        assert_eq!(arena.free_count, 3);
    }
}
