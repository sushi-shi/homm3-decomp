//! Stable object geometry and ordered per-cell membership, without per-cell heaps.

use super::PlacementError;
use crate::{
    domain::WorldPosition,
    identity::OwnerId,
    object::ObjectKind,
    prototype::{PrototypeCatalog, PrototypeId},
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
/// Valid until the owning arena is reset; moving an object does not change it.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ObjectId {
    index: usize,
    owner: OwnerId,
}
impl ObjectId {
    /// Dense geometry-record index, not a serialized object counter.
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

/// Contiguous geometry records. Payload ownership is separate from map membership:
/// native objects may appear in a temporary group and subsequently the world map.
#[derive(Default, Debug)]
pub struct ObjectArena {
    records: Vec<ObjectGeometry>,
    owner: Option<OwnerId>,
}
impl ObjectArena {
    pub(super) const fn owner(&self) -> Option<OwnerId> {
        self.owner
    }
    /// Clear a completed generation, invalidating its IDs and retaining capacity.
    pub fn reset(&mut self) {
        self.records.clear();
        self.owner = None;
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
        self.records.try_reserve(1)?;
        let id = ObjectId {
            index: self.records.len(),
            owner,
        };
        self.records.push(ObjectGeometry {
            prototype,
            kind: entry.prototype().kind(),
            position: None,
        });
        Ok(id)
    }
    /// Read geometry if the ID belongs to an existing record.
    #[must_use]
    pub fn get(&self, id: ObjectId) -> Option<&ObjectGeometry> {
        (Some(id.owner) == self.owner)
            .then(|| self.records.get(id.index))
            .flatten()
    }
    pub(super) fn set_position(&mut self, id: ObjectId, position: WorldPosition) {
        self.records[id.index].position = Some(position);
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
        let object = |index| ObjectId { index, owner };
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
