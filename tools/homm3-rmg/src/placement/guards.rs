//! Guard payload construction and placement using admitted zone affiliations.
use super::{ObjectArena, ObjectId, PlacementError, PlacementMap};
use crate::{
    behavior::Behavior,
    boundaries::ZoneOrigin,
    domain::WorldPosition,
    geometry::ZoneId,
    prototype::{GuardError, GuardFactions, PrototypeCatalog},
    raw,
    rng::RetailRng,
    traits::CreatureCatalog,
};
use std::{error::Error, fmt};

/// Guard creation reached an unsafe native selection or placement operation.
#[derive(Debug)]
pub enum GuardPlacementError {
    /// Invalid ownership, prototype, allocation or cell access.
    Placement(PlacementError),
    /// A deferred retail creature-selection fault.
    Selection(GuardError),
    /// Retail indexes the zone array with the unassigned sentinel.
    UnassignedCell(WorldPosition),
}
impl fmt::Display for GuardPlacementError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Placement(error) => error.fmt(f),
            Self::Selection(error) => error.fmt(f),
            Self::UnassignedCell(position) => {
                write!(f, "retail guard indexes an unassigned zone at {position:?}")
            }
        }
    }
}
impl Error for GuardPlacementError {}
impl From<PlacementError> for GuardPlacementError {
    fn from(error: PlacementError) -> Self {
        Self::Placement(error)
    }
}
impl From<GuardError> for GuardPlacementError {
    fn from(error: GuardError) -> Self {
        Self::Selection(error)
    }
}
impl PlacementMap<'_, '_, '_> {
    /// Construct an unplaced guard, assigning its ID only after successful selection.
    /// Geometry and payload share the supplied arena; no map membership is added.
    ///
    /// # Errors
    /// Reports foreign context, missing zones, selection faults or allocation failure.
    pub fn create_guard(
        &mut self,
        value: i32,
        zone: ZoneId,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<Option<ObjectId>, GuardPlacementError> {
        self.prepare_object_context(objects, catalog)?;
        let map = self.coverage().map();
        let zone = self.zone(zone)?;
        // Added water zones clear allowed factions, but retail leaves their
        // alignment-matching flag unwritten. That flag is an explicit replay input.
        let empty = [false; raw::TOWN_TYPE_COUNT as usize + 1];
        let factions = match zone.origin() {
            ZoneOrigin::Template(id) => {
                GuardFactions::from_zone(&map.template().zones()[id.index()], zone.alignment())
            }
            ZoneOrigin::Water => match (map.behavior(), zone.alignment()) {
                (Behavior::Retail(profile), Some(town)) if profile.water_guards_match_alignment => {
                    GuardFactions::Matching(town)
                }
                _ => GuardFactions::Allowed(&empty),
            },
        };
        let Some(stack) = catalog.select_guard(value, factions, creatures, rng)? else {
            return Ok(None);
        };
        let id = self.claim_object_id()?;
        Ok(Some(objects.create_monster(catalog, stack, id)?))
    }

    /// Place a guard unless its cell is occupied or no creature qualifies.
    /// Hotfix skips out-of-bounds/unassigned cells; retail retains flat indexing
    /// and resolves the zone before checking occupancy.
    ///
    /// # Errors
    /// Reports unsafe retail zone/cell access, foreign context or guard creation failure.
    pub fn place_guard(
        &mut self,
        value: i32,
        position: WorldPosition,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        creatures: &CreatureCatalog,
        rng: &mut RetailRng,
    ) -> Result<Option<ObjectId>, GuardPlacementError> {
        self.prepare_object_context(objects, catalog)?;
        let hotfix = self.coverage().map().behavior().is_hotfix();
        if hotfix && !self.view().contains(position.point) {
            return Ok(None);
        }
        let index = self.view().native_index(position)?;
        let Some(zone) = self.coverage().map().raster().cells()[index].zone else {
            return if hotfix {
                Ok(None)
            } else {
                Err(GuardPlacementError::UnassignedCell(position))
            };
        };
        if self.memberships.first(self.cells[index].objects).is_some() {
            return Ok(None);
        }
        let Some(guard) = self.create_guard(value, zone, objects, catalog, creatures, rng)? else {
            return Ok(None);
        };
        self.register_object(objects, catalog, guard, position)?;
        Ok(Some(guard))
    }
}
