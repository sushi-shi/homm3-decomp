//! Generator object order, counters, tent availability and entrance-distance flood.

use super::{Direction, ObjectArena, ObjectId, PlacementError, PlacementMap};
use crate::{
    behavior::Behavior,
    domain::WorldPosition,
    geometry::{Point, ZoneId},
    identity::OwnerId,
    object::ObjectKind,
    prototype::{Prototype, PrototypeCatalog},
    raw,
    request::MapVersion,
    worklist::Worklist,
};

const KINDS: usize = raw::ADVENTURE_OBJECT_TRAIT_COUNT as usize;

/// An index admitted against a catalog's border-tent prototype family.
/// This domain follows loaded prototype count, rather than the eight wire colors.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct KeyTentColor {
    index: usize,
    owner: OwnerId,
}
impl KeyTentColor {
    /// Availability-vector index, also used as a prototype subtype.
    #[must_use]
    pub const fn index(self) -> usize {
        self.index
    }
}

/// Native next-color cursor, including retail's uninitialized initial value.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum KeyTentCursor {
    /// Retail has not recalculated its cursor; a recorded replay input is needed.
    #[default]
    ReplayRequired,
    /// Raw subtype used for prototype lookup. A scan may return the family
    /// length; this does not imply that a matching prototype is absent.
    Value(i32),
}

#[derive(Debug)]
pub(super) struct Registration {
    next_object_id: i32,
    active: Vec<ObjectId>,
    retail_active_storage: bool,
    counts: [i32; KINDS],
    zone_counts: Vec<[i32; KINDS]>,
    queue: Worklist<WorldPosition>,
    catalog_owner: Option<OwnerId>,
    disabled_key_tents: Vec<bool>,
    next_key_tent: KeyTentCursor,
}
impl Default for Registration {
    fn default() -> Self {
        Self {
            next_object_id: i32::try_from(crate::constants::RMG_FIRST_OBJECT_ID).unwrap(),
            active: Vec::new(),
            retail_active_storage: false,
            counts: [0; KINDS],
            zone_counts: Vec::new(),
            queue: Worklist::default(),
            catalog_owner: None,
            disabled_key_tents: Vec::new(),
            next_key_tent: KeyTentCursor::ReplayRequired,
        }
    }
}
impl Registration {
    pub(super) fn reset(&mut self, zones: usize) -> Result<(), PlacementError> {
        self.next_object_id = i32::try_from(crate::constants::RMG_FIRST_OBJECT_ID).unwrap();
        self.active.clear();
        self.retail_active_storage = false;
        self.counts.fill(0);
        self.zone_counts
            .try_reserve(zones.saturating_sub(self.zone_counts.len()))?;
        self.zone_counts.resize(zones, [0; KINDS]);
        self.zone_counts.fill([0; KINDS]);
        self.queue.clear();
        self.catalog_owner = None;
        self.disabled_key_tents.clear();
        self.next_key_tent = KeyTentCursor::ReplayRequired;
        Ok(())
    }

    pub(super) fn require_catalog(
        &self,
        catalog: &PrototypeCatalog<'_>,
    ) -> Result<(), PlacementError> {
        if self.catalog_owner == Some(catalog.owner()) {
            Ok(())
        } else {
            Err(PlacementError::CatalogContext)
        }
    }
    pub(super) const fn tent_cursor(&self) -> KeyTentCursor {
        self.next_key_tent
    }
    fn prepare_catalog(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        behavior: Behavior,
        version: MapVersion,
    ) -> Result<(), PlacementError> {
        if catalog.behavior() != behavior
            || catalog.version() != version
            || self
                .catalog_owner
                .is_some_and(|owner| owner != catalog.owner())
        {
            return Err(PlacementError::CatalogContext);
        }
        if self.catalog_owner.is_none() {
            let family = ObjectKind::parse(i32::try_from(raw::BORDER_TENT).unwrap()).unwrap();
            let count = catalog.family(family).len();
            self.disabled_key_tents.try_reserve(count)?;
            self.disabled_key_tents.resize(count, false);
            self.catalog_owner = Some(catalog.owner());
            self.next_key_tent = match behavior {
                Behavior::Hotfix => {
                    KeyTentCursor::Value(i32::try_from(raw::KEY_LIGHT_BLUE).unwrap())
                }
                Behavior::Retail(profile) => profile
                    .initial_key_tent_color
                    .map_or(KeyTentCursor::ReplayRequired, KeyTentCursor::Value),
            };
        }
        Ok(())
    }

    fn key_tent_color(&self, subtype: i32) -> Result<KeyTentColor, PlacementError> {
        let index =
            usize::try_from(subtype).map_err(|_| PlacementError::KeyTentSubtype(subtype))?;
        if index >= self.disabled_key_tents.len() {
            return Err(PlacementError::KeyTentSubtype(subtype));
        }
        Ok(KeyTentColor {
            index,
            owner: self.catalog_owner.expect("catalog admitted before color"),
        })
    }

    fn rescan_key_tents(&mut self) {
        let index = self
            .disabled_key_tents
            .iter()
            .position(|&disabled| !disabled)
            .unwrap_or(self.disabled_key_tents.len());
        // Parsed prototype row counts fit i32; filtering cannot enlarge them.
        self.next_key_tent = KeyTentCursor::Value(i32::try_from(index).unwrap());
    }

    fn set_key_tent_disabled(
        &mut self,
        color: KeyTentColor,
        disabled: bool,
    ) -> Result<(), PlacementError> {
        if Some(color.owner) != self.catalog_owner {
            return Err(PlacementError::CatalogContext);
        }
        self.disabled_key_tents[color.index] = disabled;
        self.rescan_key_tents();
        Ok(())
    }
}

impl PlacementMap<'_, '_, '_> {
    pub(super) fn prepare_object_context(
        &mut self,
        objects: &ObjectArena,
        catalog: &PrototypeCatalog<'_>,
    ) -> Result<(), PlacementError> {
        self.prepare_registration(catalog)?;
        // Raw insertion binds cell membership even with no registered objects
        // or when the footprint is entirely clipped away.
        if !self.memberships.accepts_arena(objects) {
            return Err(PlacementError::ArenaContext);
        }
        Ok(())
    }

    /// Next native payload ID, for generation diagnostics and replay checkpoints.
    #[must_use]
    pub const fn next_object_id(&self) -> i32 {
        self.registration.next_object_id
    }
    pub(super) fn claim_object_id(&mut self) -> Result<super::MapObjectId, PlacementError> {
        let id = super::MapObjectId(self.registration.next_object_id);
        self.registration.next_object_id = self
            .registration
            .next_object_id
            .checked_add(1)
            .ok_or(PlacementError::Arithmetic)?;
        Ok(id)
    }
    pub(super) fn prepare_registration(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
    ) -> Result<(), PlacementError> {
        self.registration.prepare_catalog(
            catalog,
            self.coverage().map().behavior(),
            self.coverage().map().version(),
        )
    }

    /// Registered objects in native insertion order, including duplicate entries.
    #[must_use]
    pub fn active_objects(&self) -> &[ObjectId] {
        &self.registration.active
    }

    /// Signed native count indexed by actual object kind, without family aliases.
    #[must_use]
    pub fn object_count(&self, kind: ObjectKind) -> i32 {
        self.registration.counts[kind.index()]
    }

    /// Entrance-zone count. Native triggerless removals can make this negative.
    #[must_use]
    pub fn zone_object_count(&self, zone: ZoneId, kind: ObjectKind) -> Option<i32> {
        self.registration
            .zone_counts
            .get(zone.index())
            .map(|counts| counts[kind.index()])
    }

    /// Admit a raw tent subtype against this generation's prototype catalog.
    ///
    /// # Errors
    /// Returns a mismatched catalog context or an out-of-range subtype.
    pub fn key_tent_color(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        subtype: i32,
    ) -> Result<KeyTentColor, PlacementError> {
        self.prepare_registration(catalog)?;
        self.registration.key_tent_color(subtype)
    }

    /// Read the native tent cursor, retaining retail's initial replay requirement.
    ///
    /// # Errors
    /// Returns a mismatched catalog context or a failed availability allocation.
    pub fn next_key_tent(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
    ) -> Result<KeyTentCursor, PlacementError> {
        self.prepare_registration(catalog)?;
        Ok(self.registration.next_key_tent)
    }

    /// Stored tent cursor without initializing or replacing catalog context.
    #[must_use]
    pub const fn key_tent_cursor(&self) -> KeyTentCursor {
        self.registration.tent_cursor()
    }

    /// Read a color's reservation without changing the cursor or allocating.
    ///
    /// # Errors
    /// Rejects colors not admitted against this map's current catalog.
    pub fn key_tent_disabled(&self, color: KeyTentColor) -> Result<bool, PlacementError> {
        if Some(color.owner) != self.registration.catalog_owner {
            return Err(PlacementError::CatalogContext);
        }
        Ok(self.registration.disabled_key_tents[color.index])
    }

    /// Reserve or release a tent color and rescan the first enabled color.
    ///
    /// # Errors
    /// Returns a color or prototype catalog belonging to another context.
    pub fn set_key_tent_disabled(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        color: KeyTentColor,
        disabled: bool,
    ) -> Result<(), PlacementError> {
        self.prepare_registration(catalog)?;
        self.registration.set_key_tent_disabled(color, disabled)
    }

    /// Insert the footprint, append the global list, count the kind and flood
    /// entrance distances, in source order. No random numbers are consumed.
    ///
    /// # Errors
    /// Reports context, identity, geometry, allocation or arithmetic faults.
    /// A fault after footprint insertion retains earlier native mutations.
    pub fn register_object(
        &mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        object: ObjectId,
        anchor: WorldPosition,
    ) -> Result<(), PlacementError> {
        self.prepare_registration(catalog)?;
        let geometry = *objects
            .get(object)
            .ok_or(PlacementError::UnknownObject(object))?;
        let prototype = catalog
            .get(geometry.prototype())
            .ok_or(PlacementError::UnknownPrototype(geometry.prototype()))?
            .prototype();
        self.add_base_object(objects, catalog, object, anchor)?;
        let count = &mut self.registration.counts[geometry.kind().index()];
        *count = count.checked_add(1).ok_or(PlacementError::Arithmetic)?;
        if prototype.entrance().is_some() {
            let entrance = entrance_position(prototype, anchor)?;
            let index = self.view().native_index(entrance)?;
            if let Some(zone) = self.coverage().map().raster().cells()[index].zone {
                let count =
                    &mut self.registration.zone_counts[zone.index()][geometry.kind().index()];
                *count = count.checked_add(1).ok_or(PlacementError::Arithmetic)?;
            }
            self.flood_object_distance(entrance)?;
        }
        Ok(())
    }

    // GeneratorBase::addObject: footprint and list membership, without the
    // derived generator's counters, tent state or entrance-distance flood.
    pub(super) fn add_base_object(
        &mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        object: ObjectId,
        anchor: WorldPosition,
    ) -> Result<(), PlacementError> {
        self.registration.active.try_reserve(1)?;
        self.insert_object(objects, catalog, object, anchor)?;
        self.registration.active.push(object);
        self.registration.retail_active_storage = true;
        Ok(())
    }

    fn flood_object_distance(&mut self, entrance: WorldPosition) -> Result<(), PlacementError> {
        let index = self.view().native_index(entrance)?;
        self.cells[index].object_distance = 0;
        self.registration.queue.clear();
        self.registration.queue.insert(entrance, 0)?;
        while let Some(position) = self.registration.queue.pop() {
            // Use the cell's current cost, even for a stale queued entry.
            let distance = self.cells[self.view().native_index(position)?].object_distance;
            for direction in Direction::ALL {
                let Point { x: dx, y: dy } = direction.offset();
                let step = direction.chamfer_cost();
                let cost = u32::from(distance) + step;
                let point = Point::new(
                    position
                        .point
                        .x
                        .checked_add(dx)
                        .ok_or(PlacementError::CoordinateOverflow)?,
                    position
                        .point
                        .y
                        .checked_add(dy)
                        .ok_or(PlacementError::CoordinateOverflow)?,
                );
                if !self.view().contains(point) {
                    continue;
                }
                let next = WorldPosition {
                    point,
                    level: position.level,
                };
                let index = self.view().index(next)?;
                if cost >= u32::from(self.cells[index].object_distance) {
                    continue;
                }
                self.cells[index].object_distance =
                    u16::try_from(cost).map_err(|_| PlacementError::Arithmetic)?;
                self.registration.queue.insert(
                    next,
                    i32::try_from(cost).map_err(|_| PlacementError::Arithmetic)?,
                )?;
            }
        }
        Ok(())
    }

    /// Remove the first global occurrence, decrement native counters, release a
    /// border guard's tent color, then erase its footprint. Keep object ownership.
    ///
    /// # Errors
    /// Reports foreign objects, unsafe legacy entrance accesses, or
    /// retail end-iterator erasure. Earlier mutations remain applied on failure.
    #[expect(
        clippy::missing_panics_doc,
        reason = "active IDs are inserted only after assigning their anchor; arena reset invalidates them"
    )]
    pub fn unregister_object(
        &mut self,
        objects: &ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        object: ObjectId,
    ) -> Result<(), PlacementError> {
        self.prepare_registration(catalog)?;
        if !self.memberships.accepts(object) {
            return Err(PlacementError::UnknownObject(object));
        }
        let geometry = *objects
            .get(object)
            .ok_or(PlacementError::UnknownObject(object))?;
        let prototype = catalog
            .get(geometry.prototype())
            .ok_or(PlacementError::UnknownPrototype(geometry.prototype()))?
            .prototype();
        if let Some(index) = self
            .registration
            .active
            .iter()
            .position(|&candidate| candidate == object)
        {
            self.registration.active.remove(index);
            let count = &mut self.registration.counts[geometry.kind().index()];
            *count = count.checked_sub(1).ok_or(PlacementError::Arithmetic)?;
            // Source removeObject calls getEntrance even without a trigger, using
            // the legacy no-trigger cell (mask width, mask height).
            let anchor = geometry.position().expect("registration assigns an anchor");
            let index = self
                .view()
                .native_index(entrance_position(prototype, anchor)?)?;
            if let Some(zone) = self.coverage().map().raster().cells()[index].zone {
                let count =
                    &mut self.registration.zone_counts[zone.index()][geometry.kind().index()];
                *count = count.checked_sub(1).ok_or(PlacementError::Arithmetic)?;
            }
        } else if self.registration.retail_active_storage
            && !self.coverage().map().behavior().is_hotfix()
        {
            return Err(PlacementError::NotRegistered(object));
        }
        if geometry.kind().index() == raw::BORDER_GUARD as usize {
            let color = self.registration.key_tent_color(prototype.subtype())?;
            self.registration.set_key_tent_disabled(color, false)?;
        }
        self.erase_footprint(objects, catalog, object)
    }
}

// Native no-trigger coordinates are the mask-frame limits, not (-1,-1).
pub(super) fn trigger_offset(prototype: &Prototype) -> Point {
    let (x, y) = prototype.entrance().map_or(
        (
            i32::try_from(raw::OBJECT_MASK_WIDTH).unwrap(),
            i32::try_from(raw::OBJECT_MASK_HEIGHT).unwrap(),
        ),
        |cell| (i32::from(cell.x()), i32::from(cell.y())),
    );
    Point::new(x, y)
}

pub(super) fn entrance_position(
    prototype: &Prototype,
    anchor: WorldPosition,
) -> Result<WorldPosition, PlacementError> {
    let Point { x, y } = trigger_offset(prototype);
    Ok(WorldPosition {
        point: Point::new(
            anchor
                .point
                .x
                .checked_sub(x)
                .ok_or(PlacementError::CoordinateOverflow)?,
            anchor
                .point
                .y
                .checked_sub(y)
                .ok_or(PlacementError::CoordinateOverflow)?,
        ),
        level: anchor.level,
    })
}
