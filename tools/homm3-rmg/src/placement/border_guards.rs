//! Key tents, side-by-side border guards and protected crossing neighborhoods.
use super::{
    BorderColor, KeyTentColor, KeyTentCursor, Movement, Neighborhood, ObjectArena, PlacementError,
    PlacementMap,
};
use crate::{
    domain::WorldPosition, geometry::ZoneId, object::ObjectKind, prototype::PrototypeCatalog, raw,
    rng::RetailRng,
};

/// The two guard-row widths requested by native connection creation.
#[derive(Clone, Copy, Debug)]
#[repr(u32)]
pub enum BorderGuardCount {
    /// One guard at a crossing, gate or portal.
    Single = raw::RMG_SINGLE_BORDER_GUARD,
    /// One guard per cell of the row below a three-tile shipyard.
    Shipyard = raw::RMG_SHIPYARD_BORDER_GUARDS,
}

/// Native return behavior without claiming that missing guard art was placed.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BorderGuardPlacement {
    /// No matching tent prototype or no fitting tent site.
    NotPlaced,
    /// Native bug: reports color zero without placing or reserving anything.
    MissingGuardPrototype,
    /// Tent and guards were registered and their full color index reserved.
    Placed(KeyTentColor),
}
impl BorderGuardPlacement {
    /// Color observed by native callers. Missing guard art is success-shaped.
    #[must_use]
    #[expect(
        clippy::missing_panics_doc,
        reason = "private color admission bounds indices by the parsed i32 row count"
    )]
    pub fn reported_color(self) -> Option<i32> {
        match self {
            Self::NotPlaced => None,
            Self::MissingGuardPrototype => Some(0),
            Self::Placed(color) => {
                Some(i32::try_from(color.index()).expect("catalog row count fits i32"))
            }
        }
    }
}
impl PlacementMap<'_, '_, '_> {
    /// Place a key tent in its zone, then guards eastward from the given cell.
    /// Prototype lookup uses the full cursor; only final reservation admits it
    /// as an availability index. No terrain filter or prototype draw is used.
    ///
    /// # Errors
    /// Reports missing retail replay input, context/allocation/access faults or
    /// an invalid availability index after preserving earlier native mutations.
    #[expect(
        clippy::missing_panics_doc,
        reason = "canonical object-kind constants are in the parsed domain"
    )]
    pub fn place_border_guard(
        &mut self,
        mut position: WorldPosition,
        count: BorderGuardCount,
        key_tent_zone: ZoneId,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<BorderGuardPlacement, PlacementError> {
        self.prepare_object_context(objects, catalog)?;
        let KeyTentCursor::Value(color) = self.next_key_tent(catalog)? else {
            return Err(PlacementError::KeyTentReplayRequired);
        };
        let tent_kind = ObjectKind::parse(i32::try_from(raw::BORDER_TENT).unwrap()).unwrap();
        let guard_kind = ObjectKind::parse(i32::try_from(raw::BORDER_GUARD).unwrap()).unwrap();
        let Some(tent_prototype) = catalog.first_subtype(tent_kind, color) else {
            return Ok(BorderGuardPlacement::NotPlaced);
        };
        let Some(guard_prototype) = catalog.first_subtype(guard_kind, color) else {
            return Ok(BorderGuardPlacement::MissingGuardPrototype);
        };
        let tent = objects.create(catalog, tent_prototype)?;
        if !self.place_object_in_zone(tent, key_tent_zone, objects, catalog, rng)? {
            objects.discard_unplaced(tent)?;
            return Ok(BorderGuardPlacement::NotPlaced);
        }
        for _ in 0..count as u32 {
            let guard = objects.create(catalog, guard_prototype)?;
            let index = self.view().native_index(position)?;
            self.cells[index].clear_border();
            self.register_object(objects, catalog, guard, position)?;
            // Native increments even after the final guard.
            position.point.x = position
                .point
                .x
                .checked_add(1)
                .ok_or(PlacementError::CoordinateOverflow)?;
        }
        let color = self.key_tent_color(catalog, color)?;
        self.set_key_tent_disabled(catalog, color, true)?;
        Ok(BorderGuardPlacement::Placed(color))
    }

    /// Mark empty cells in a clipped 3x3 crossing, then clear its predecessor.
    /// Terrain and zone do not filter the patch. The full reported color is
    /// truncated only when assigning the native border bitfield.
    ///
    /// # Errors
    /// Reports coordinate or allocation-boundary access faults. Neighborhood
    /// mutations precede the center/predecessor lookup, as in native code.
    pub fn mark_border_connection_area(
        &mut self,
        position: WorldPosition,
        reported_color: i32,
    ) -> Result<(), PlacementError> {
        let color = BorderColor::from_subtype(reported_color);
        for nearby in Neighborhood::ThreeByThree.cells(position, self.view().side)? {
            self.mark_empty_border(nearby, color)?;
        }
        let index = self.view().native_index(position)?;
        if let Movement::Arrived { previous, .. } = self.cells[index].movement {
            if self.view().contains(previous.point) {
                let index = self.view().native_index(previous)?;
                self.cells[index].clear_border();
            }
        }
        Ok(())
    }
}

impl PlacementMap<'_, '_, '_> {
    pub(super) fn mark_empty_border(
        &mut self,
        position: WorldPosition,
        color: BorderColor,
    ) -> Result<(), PlacementError> {
        let index = self.view().native_index(position)?;
        if self.memberships.first(self.cells[index].objects).is_none() {
            self.cells[index].mark_border(color);
        }
        Ok(())
    }
}
