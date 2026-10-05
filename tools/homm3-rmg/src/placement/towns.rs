//! Primary and additional towns in native category, candidate and RNG order.

use super::{
    density::Density, neighborhood::Neighborhood, registration::entrance_position, MapObjectId,
    ObjectArena, ObjectId, ObjectPayload, PlacementError, PlacementMap, PlacementView,
};
use crate::{
    behavior::TownMask,
    boundaries::BoundaryZone,
    domain::WorldPosition,
    geometry::Point,
    object::ObjectKind,
    prototype::{OutlineWorkspace, PreparedPrototype, PrototypeCatalog, PrototypeRef},
    raw,
    request::{MapVersion, PerColour, Town, PLAYER_COUNT},
    rng::{RetailRng, RngCheckpoint},
    selection::Player,
    template::{Placement, SlotUse, TownCategory, ZoneRole},
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

/// A town payload stored beside its arena geometry, without cached coordinates.
#[derive(Clone, Copy, Debug)]
pub struct TownPayload {
    id: MapObjectId,
    owner: Option<Player>,
    fort: Fort,
}
impl TownPayload {
    pub(super) const fn new(id: MapObjectId, owner: Option<Player>, fort: Fort) -> Self {
        Self { id, owner, fort }
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
    candidates: Vec<WorldPosition>,
    outline: OutlineWorkspace,
}
impl TownState {
    pub(super) fn reset(&mut self) {
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
    /// Registered town identities and their borrowed payloads in native order.
    /// Removed or merely constructed objects are excluded.
    ///
    /// # Errors
    /// Reports an arena that does not own this map's memberships.
    pub fn towns<'a>(
        &'a self,
        objects: &'a ObjectArena,
    ) -> Result<impl Iterator<Item = (ObjectId, &'a TownPayload)> + 'a, PlacementError> {
        self.map.town_payloads(objects)
    }
    /// Road targets in insertion order, without removing duplicates.
    #[must_use]
    pub fn road_targets(&self) -> &[WorldPosition] {
        self.map.road_targets()
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
    allowed: TownMask,
}
impl Rules {
    const fn placement(self, category: TownCategory) -> Placement {
        self.categories[category.index()]
    }
    const fn category(self, category: TownCategory) -> (Option<Player>, Fort) {
        (
            if category.is_player() {
                self.owner
            } else {
                None
            },
            if category.has_fort() {
                Fort::Present
            } else {
                Fort::Absent
            },
        )
    }
    fn choose_town(self, version: MapVersion, rng: &mut RetailRng) -> Town {
        self.allowed.choose(rng).unwrap_or_else(|| {
            // Native draws `rand() % TOWN_TYPE[_ROE]_COUNT`; Conflux is last.
            let mut playable = TownMask::ALL;
            if version == MapVersion::Restoration {
                playable.remove(Town::CONFLUX);
            }
            playable.choose(rng).expect("every map version has towns")
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
        self.prepare_object_context(objects, catalog)?;
        for index in 0..self.coverage().map().zones().len() {
            let zone = self.coverage().map().zones()[index];
            let Some(rules) = self.town_rules(zone) else {
                continue;
            };
            for category in TownCategory::ALL {
                if rules.placement(category).initial_count > 0 {
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
        if self.coverage().map().behavior().is_hotfix()
            && !self.has_player_towns(objects, catalog)?
        {
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
        let map = self.coverage().map();
        let source = map.template_zone(&zone)?;
        Some(Rules {
            categories: *source.towns(),
            owner: source.role().owner().and_then(|slot| map.player(slot)),
            neutral_matches: source.neutral_towns_match_alignment(),
            allowed: source.allowed_towns(),
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
            layout: map.raster().layout(),
            surface: super::PlacementSurface::World {
                terrain: self.terrain.tiles(),
                zones: map.raster().cells(),
            },
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

    fn town_prototype<'c>(
        catalog: &'c PrototypeCatalog<'_>,
        town: Town,
    ) -> Result<PrototypeRef<'c>, TownError> {
        let kind = ObjectKind::TOWN;
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
        let entry = prototype.entry();
        self.towns.candidates.clear();
        let Some(bounds) = zone.bounds() else {
            return Ok(false);
        };
        let mut best =
            i32::try_from(crate::constants::RMG_PRIMARY_TOWN_MAXIMUM_SQUARED_DISTANCE).unwrap();
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
        for category in TownCategory::ALL {
            let placement = rules.placement(category);
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
        while let Some(index) = density.next()? {
            let (owner, fort) = rules.category(TownCategory::ALL[index]);
            if !self
                .try_additional_town(objects, catalog, zone, rules, owner, fort, spacing, rng)?
            {
                density.finish(index);
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
        let entry = prototype.entry();
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
                for nearby in Neighborhood::ThreeByThree.cells(entrance, self.view().side())? {
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
        prototype: PrototypeRef<'_>,
        owner: Option<Player>,
        fort: Fort,
        rng: &mut RetailRng,
    ) -> Result<WorldPosition, TownError> {
        let id = self.claim_object_id()?;
        let object = objects.create_town(catalog, prototype.id(), id, owner, fort)?;
        let count =
            u32::try_from(self.towns.candidates.len()).map_err(|_| TownError::Arithmetic)?;
        let selected = rng.below(NonZeroU32::new(count).expect("caller found candidates")) as usize;
        let position = self.towns.candidates[selected];
        self.register_object(objects, catalog, object, position)?;
        let entrance = entrance_position(prototype.entry().prototype(), position)?;
        self.append_road_target(entrance)?;
        self.open_entrance_approach(entrance)?;
        Ok(entrance)
    }

    fn town_payloads<'a>(
        &'a self,
        objects: &'a ObjectArena,
    ) -> Result<impl Iterator<Item = (ObjectId, &'a TownPayload)> + 'a, PlacementError> {
        if !self.memberships.accepts_arena(objects) {
            return Err(PlacementError::ArenaContext);
        }
        Ok(self
            .active_objects()
            .iter()
            .filter_map(move |&object| match objects.payload(object) {
                Some(ObjectPayload::Town(town)) => Some((object, town)),
                _ => None,
            }))
    }

    fn has_player_towns(
        &self,
        objects: &ObjectArena,
        catalog: &PrototypeCatalog<'_>,
    ) -> Result<bool, PlacementError> {
        let mut seated = PerColour::new([None; PLAYER_COUNT]);
        let map = self.coverage().map();
        for zone in map.zones() {
            let (Some(entrance), Some(rules)) = (zone.primary_town(), map.template_zone(zone))
            else {
                continue;
            };
            let (ZoneRole::Human(slot) | ZoneRole::Computer(slot)) = rules.role() else {
                continue;
            };
            let Some(player) = map.player(slot) else {
                continue;
            };
            let mut owned = false;
            for (object, town) in self.town_payloads(objects)? {
                if town.owner == Some(player)
                    && objects.positioned(object)?.entrance(catalog)? == entrance
                {
                    owned = true;
                    break;
                }
            }
            if !owned {
                return Ok(false);
            }
            if matches!(rules.role(), ZoneRole::Human(_)) {
                seated[player] = Some(SlotUse::Human);
            } else {
                seated[player].get_or_insert(SlotUse::Computer);
            }
        }
        let human_count = usize::from(map.request().human_players().get());
        let humans = seated
            .iter()
            .filter(|(_, use_)| **use_ == Some(SlotUse::Human))
            .count();
        let players = seated.iter().filter(|(_, use_)| use_.is_some()).count();
        Ok(humans >= human_count
            && players >= human_count + usize::from(map.request().computer_players().get()))
    }
}
