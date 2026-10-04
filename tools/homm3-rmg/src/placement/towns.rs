//! Primary and additional towns in native category, candidate and RNG order.

use super::{
    density::Density, neighborhood::Neighborhood, registration::entrance_position, MapObjectId,
    ObjectArena, ObjectId, PlacementError, PlacementMap, PlacementView,
};
use crate::{
    boundaries::{BoundaryZone, ZoneOrigin},
    domain::WorldPosition,
    geometry::Point,
    object::ObjectKind,
    prototype::{OutlineWorkspace, PreparedPrototype, PrototypeCatalog, PrototypeId},
    raw,
    request::{MapVersion, Town, PLAYER_COUNT},
    rng::{RetailRng, RngCheckpoint},
    selection::{select_allowed_town, Player},
    template::{Placement, ZoneRole},
};
use std::{collections::TryReserveError, error::Error, fmt, num::NonZeroU32};

/// Fort building emitted with a generated town.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Fort {
    /// Basic town without a fort.
    Absent,
    /// Town starts with its fort built.
    Present,
}

/// A town payload associated with stable geometry and its native serialized ID.
#[derive(Clone, Copy, Debug)]
pub struct TownObject {
    object: ObjectId,
    id: MapObjectId,
    owner: Option<Player>,
    fort: Fort,
    entrance: WorldPosition,
}
impl TownObject {
    /// Geometry and prototype identity in the shared arena.
    #[must_use]
    pub const fn object(self) -> ObjectId {
        self.object
    }
    /// Counter shared with monsters and prisons, independent of arena index.
    #[must_use]
    pub const fn id(self) -> MapObjectId {
        self.id
    }
    /// Assigned player color, or a neutral town.
    #[must_use]
    pub const fn owner(self) -> Option<Player> {
        self.owner
    }
    /// Starting fort state.
    #[must_use]
    pub const fn fort(self) -> Fort {
        self.fort
    }
    /// Trigger position also registered as a road target.
    #[must_use]
    pub const fn entrance(self) -> WorldPosition {
        self.entrance
    }
}

/// Town placement fault or the hotfix's unsatisfied starting-town requirement.
#[derive(Debug)]
pub enum TownError {
    /// An unsafe placement or map access was reached.
    Placement(PlacementError),
    /// Loaded town prototypes do not contain the selected faction slot.
    MissingPrototype(Town),
    /// Density, count or candidate arithmetic cannot be represented.
    Arithmetic,
    /// Hotfix could not provide owned main towns for all requested players.
    MissingPlayerTowns,
}
impl fmt::Display for TownError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Placement(error) => error.fmt(f),
            Self::MissingPrototype(town) => {
                write!(f, "missing town prototype at faction slot {}", town.index())
            }
            Self::Arithmetic => f.write_str("town density or candidate arithmetic overflow"),
            Self::MissingPlayerTowns => {
                f.write_str("hotfix requires owned starting towns for all requested players")
            }
        }
    }
}
impl Error for TownError {}
impl From<PlacementError> for TownError {
    fn from(error: PlacementError) -> Self {
        Self::Placement(error)
    }
}
impl From<TryReserveError> for TownError {
    fn from(error: TryReserveError) -> Self {
        Self::Placement(error.into())
    }
}

#[derive(Default, Debug)]
pub(super) struct TownState {
    records: Vec<TownObject>,
    road_targets: Vec<WorldPosition>,
    candidates: Vec<WorldPosition>,
    outline: OutlineWorkspace,
}
impl TownState {
    pub(super) fn reset(&mut self) {
        self.records.clear();
        self.road_targets.clear();
        self.candidates.clear();
    }
}

/// Towns are placed and the hotfix player-town requirement has passed.
pub struct TownsPlaced<'state, 'zones, 'tiles> {
    pub(super) map: PlacementMap<'state, 'zones, 'tiles>,
    rng: RngCheckpoint,
}
impl TownsPlaced<'_, '_, '_> {
    /// Placement state for subsequent connections and object placement.
    #[must_use]
    pub const fn map(&self) -> &PlacementMap<'_, '_, '_> {
        &self.map
    }
    /// Town payloads in construction order, borrowing generation storage.
    #[must_use]
    pub fn towns(&self) -> &[TownObject] {
        &self.map.towns.records
    }
    /// Road targets in insertion order, without removing duplicates.
    #[must_use]
    pub fn road_targets(&self) -> &[WorldPosition] {
        &self.map.towns.road_targets
    }
    /// RNG after town placement and the starting-town check.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}

#[derive(Clone, Copy)]
struct Rules {
    categories: [Placement; raw::RMG_TOWN_CATEGORY_COUNT as usize],
    owner: Option<Player>,
    neutral_matches: bool,
    allowed: [bool; raw::TOWN_TYPE_COUNT as usize],
}
impl Rules {
    fn category(self, index: usize) -> (Option<Player>, Fort) {
        let player = index == raw::RMG_TOWN_PLAYER_CASTLE as usize
            || index == raw::RMG_TOWN_PLAYER_BASIC as usize;
        let fortified = index == raw::RMG_TOWN_PLAYER_CASTLE as usize
            || index == raw::RMG_TOWN_NEUTRAL_CASTLE as usize;
        (
            if player { self.owner } else { None },
            if fortified {
                Fort::Present
            } else {
                Fort::Absent
            },
        )
    }
    fn choose_town(self, version: MapVersion, rng: &mut RetailRng) -> Town {
        select_allowed_town(&self.allowed, rng).unwrap_or_else(|| {
            let count = if version == MapVersion::Restoration {
                raw::TOWN_TYPE_ROE_COUNT
            } else {
                raw::TOWN_TYPE_COUNT
            };
            Town::parse(i32::try_from(rng.below(NonZeroU32::new(count).unwrap())).unwrap()).unwrap()
        })
    }
}

impl<'state, 'zones, 'tiles> PlacementMap<'state, 'zones, 'tiles> {
    /// Place all primary towns, then all fixed and density towns in source order.
    /// Consumes the placement-stage token so this stage cannot run twice.
    ///
    /// # Errors
    /// Reports missing prototypes, native arithmetic/access faults, allocation
    /// failure or the hotfix starting-town requirement after all town attempts.
    pub fn place_towns(
        mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        rng: &mut RetailRng,
    ) -> Result<TownsPlaced<'state, 'zones, 'tiles>, TownError> {
        self.prepare_registration(catalog)?;
        for index in 0..self.coverage().map().zones().len() {
            let zone = self.coverage().map().zones()[index];
            let Some(rules) = self.town_rules(zone) else {
                continue;
            };
            for category in 0..rules.categories.len() {
                if rules.categories[category].initial_count > 0 {
                    let (owner, fort) = rules.category(category);
                    if self.try_primary_town(
                        objects,
                        catalog,
                        zone,
                        zone.alignment(),
                        owner,
                        fort,
                        rng,
                    )? {
                        break;
                    }
                }
            }
        }
        for index in 0..self.coverage().map().zones().len() {
            let zone = self.coverage().map().zones()[index];
            let Some(rules) = self.town_rules(zone) else {
                continue;
            };
            self.additional_towns(objects, catalog, zone, rules, rng)?;
        }
        if self.coverage().map().behavior().is_hotfix() && !self.has_player_towns() {
            return Err(TownError::MissingPlayerTowns);
        }
        Ok(TownsPlaced {
            map: self,
            rng: rng.checkpoint(),
        })
    }
}

impl PlacementMap<'_, '_, '_> {
    fn town_rules(&self, zone: BoundaryZone) -> Option<Rules> {
        let ZoneOrigin::Template(id) = zone.origin() else {
            return None;
        };
        let map = self.coverage().map();
        let source = &map.template().zones()[id.index()];
        let slot = match source.role() {
            ZoneRole::Human(slot) | ZoneRole::Computer(slot) => Some(slot),
            ZoneRole::Treasure(slot) | ZoneRole::Junction(slot) => slot,
        };
        Some(Rules {
            categories: *source.towns(),
            owner: slot.and_then(|slot| map.player(slot)),
            neutral_matches: source.neutral_towns_match_alignment(),
            allowed: *source.allowed_towns(),
        })
    }

    fn town_fits(
        &mut self,
        entry: &PreparedPrototype<'_>,
        position: WorldPosition,
        zone: BoundaryZone,
    ) -> Result<bool, TownError> {
        let map = self.terrain.coverage().map();
        let view = PlacementView {
            side: map.raster().dimension(),
            terrain: self.terrain.tiles(),
            zones: map.raster().cells(),
            cells: self.cells,
        };
        Ok(view.can_place(
            entry,
            position,
            zone.id(),
            zone.terrain(),
            &mut self.towns.outline,
        )?)
    }

    fn town_prototype(
        catalog: &PrototypeCatalog<'_>,
        town: Town,
    ) -> Result<PrototypeId, TownError> {
        let kind = ObjectKind::parse(i32::try_from(raw::TOWN).unwrap()).unwrap();
        catalog
            .at(kind, town.index())
            .ok_or(TownError::MissingPrototype(town))
    }

    #[expect(
        clippy::too_many_arguments,
        reason = "source town placement inputs retain owner and fort semantics"
    )]
    fn try_primary_town(
        &mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        zone: BoundaryZone,
        town: Option<Town>,
        owner: Option<Player>,
        fort: Fort,
        rng: &mut RetailRng,
    ) -> Result<bool, TownError> {
        let Some(town) = town else {
            return Ok(false);
        };
        let prototype = Self::town_prototype(catalog, town)?;
        let entry = catalog.get(prototype).unwrap();
        self.towns.candidates.clear();
        let Some(bounds) = zone.bounds() else {
            return Ok(false);
        };
        let mut best = i32::try_from(raw::RMG_PRIMARY_TOWN_MAXIMUM_SQUARED_DISTANCE).unwrap();
        for y in bounds.minimum().y..bounds.maximum().y {
            for x in bounds.minimum().x..bounds.maximum().x {
                let position = WorldPosition {
                    point: Point::new(x, y),
                    level: zone.position().level,
                };
                let index = self.view().index(position)?;
                if self.coverage().map().raster().cells()[index].zone != Some(zone.id()) {
                    continue;
                }
                let score = position.point.squared_distance(zone.position().point);
                if score <= best && self.town_fits(entry, position, zone)? {
                    if score < best {
                        self.towns.candidates.clear();
                        best = score;
                    }
                    self.towns.candidates.try_reserve(1)?;
                    self.towns.candidates.push(position);
                }
            }
        }
        if self.towns.candidates.is_empty() {
            return Ok(false);
        }
        let entrance = self.place_town_candidate(objects, catalog, prototype, owner, fort, rng)?;
        self.terrain
            .coverage_mut()
            .map_mut()
            .record_primary_town(zone.id(), entrance, town);
        Ok(true)
    }

    fn additional_towns(
        &mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        zone: BoundaryZone,
        rules: Rules,
        rng: &mut RetailRng,
    ) -> Result<(), TownError> {
        let mut skip_primary = true;
        for (category, placement) in rules.categories.iter().enumerate() {
            if placement.initial_count <= 0 {
                continue;
            }
            let (owner, fort) = rules.category(category);
            for _ in i32::from(skip_primary)..placement.initial_count {
                self.try_additional_town(objects, catalog, zone, rules, owner, fort, 0, rng)?;
            }
            skip_primary = false;
        }
        let mut density = Density::new(&rules.categories)?;
        let Some(spacing) =
            density.spacing(i32::try_from(raw::RMG_TOWN_AND_MINE_DENSITY_AREA).unwrap())
        else {
            return Ok(());
        };
        while let Some(category) = density.next()? {
            let (owner, fort) = rules.category(category);
            if !self
                .try_additional_town(objects, catalog, zone, rules, owner, fort, spacing, rng)?
            {
                density.finish(category);
            }
        }
        Ok(())
    }

    #[expect(
        clippy::too_many_arguments,
        reason = "source town placement inputs retain category and density semantics"
    )]
    fn try_additional_town(
        &mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        zone: BoundaryZone,
        rules: Rules,
        owner: Option<Player>,
        fort: Fort,
        spacing: i32,
        rng: &mut RetailRng,
    ) -> Result<bool, TownError> {
        // placeAdditionalTowns caches alignment at entry. Even if a hotfix primary
        // later resolves a neutral zone, further attempts use this original value.
        let town = if (owner.is_none() && !rules.neutral_matches) || zone.alignment().is_none() {
            rules.choose_town(self.coverage().map().version(), rng)
        } else {
            zone.alignment().unwrap()
        };
        let current = self.coverage().map().zones()[zone.id().index()];
        if current.primary_town().is_none() {
            return self.try_primary_town(objects, catalog, current, Some(town), owner, fort, rng);
        }
        let prototype = Self::town_prototype(catalog, town)?;
        let entry = catalog.get(prototype).unwrap();
        let size = entry.image_mask().size().map_err(PlacementError::from)?;
        let Some(bounds) = zone.bounds() else {
            return Ok(false);
        };
        self.towns.candidates.clear();
        let mut best = spacing;
        for y in bounds.minimum().y + i32::from(size.height())..bounds.maximum().y {
            for x in bounds.minimum().x + i32::from(size.width())..bounds.maximum().x {
                let position = WorldPosition {
                    point: Point::new(x, y),
                    level: zone.position().level,
                };
                let entrance = entrance_position(entry.prototype(), position)?;
                let index = self.view().native_index(entrance)?;
                if self.coverage().map().raster().cells()[index].zone != Some(zone.id()) {
                    continue;
                }
                let score = i32::from(self.cells[index].object_distance);
                if score < best || !self.town_fits(entry, position, zone)? {
                    continue;
                }
                let mut valid = true;
                for nearby in Neighborhood::ThreeByThree.cells(entrance, self.view().side)? {
                    let index = self.view().index(nearby)?;
                    if self.coverage().map().raster().cells()[index].zone != Some(zone.id()) {
                        valid = false;
                    }
                }
                if !valid {
                    continue;
                }
                if score > best {
                    self.towns.candidates.clear();
                    best = score;
                }
                self.towns.candidates.try_reserve(1)?;
                self.towns.candidates.push(position);
            }
        }
        if self.towns.candidates.is_empty() {
            return Ok(false);
        }
        self.place_town_candidate(objects, catalog, prototype, owner, fort, rng)?;
        Ok(true)
    }

    fn place_town_candidate(
        &mut self,
        objects: &mut ObjectArena,
        catalog: &PrototypeCatalog<'_>,
        prototype: PrototypeId,
        owner: Option<Player>,
        fort: Fort,
        rng: &mut RetailRng,
    ) -> Result<WorldPosition, TownError> {
        self.towns.records.try_reserve(1)?;
        self.towns.road_targets.try_reserve(1)?;
        let id = self.claim_object_id()?;
        let object = objects.create(catalog, prototype)?;
        let count =
            u32::try_from(self.towns.candidates.len()).map_err(|_| TownError::Arithmetic)?;
        let selected = rng.below(NonZeroU32::new(count).expect("caller found candidates")) as usize;
        let position = self.towns.candidates[selected];
        self.register_object(objects, catalog, object, position)?;
        let entrance = entrance_position(catalog.get(prototype).unwrap().prototype(), position)?;
        self.towns.records.push(TownObject {
            object,
            id,
            owner,
            fort,
            entrance,
        });
        self.towns.road_targets.push(entrance);
        self.open_entrance_approach(entrance)?;
        Ok(entrance)
    }

    fn has_player_towns(&self) -> bool {
        let mut humans = [false; PLAYER_COUNT];
        let mut players = [false; PLAYER_COUNT];
        let map = self.coverage().map();
        for zone in map.zones() {
            let (Some(entrance), ZoneOrigin::Template(id)) = (zone.primary_town(), zone.origin())
            else {
                continue;
            };
            let role = map.template().zones()[id.index()].role();
            let (ZoneRole::Human(slot) | ZoneRole::Computer(slot)) = role else {
                continue;
            };
            let Some(player) = map.player(slot) else {
                continue;
            };
            if !self
                .towns
                .records
                .iter()
                .any(|town| town.owner == Some(player) && town.entrance == entrance)
            {
                return false;
            }
            players[player.index()] = true;
            if matches!(role, ZoneRole::Human(_)) {
                humans[player.index()] = true;
            }
        }
        let human_count = usize::from(map.request().human_players().get());
        humans.into_iter().filter(|&yes| yes).count() >= human_count
            && players.into_iter().filter(|&yes| yes).count()
                >= human_count + usize::from(map.request().computer_players().get())
    }
}
