//! Zone placement, relaxation and terrain selection in serial RNG order.

use crate::{
    domain::{Level, Terrain, WorldPosition},
    geometry::{GeometryError, Point, ZoneId},
    raw,
    request::{Levels, Request, Town, TownChoice, Water},
    rng::RetailRng,
    rules::Ruleset,
    selection::{choose_flag, Player, SelectedTemplate},
    template::{Template, Zone, ZoneRole},
};
use std::{collections::TryReserveError, error::Error, fmt, num::NonZeroU32};

mod choices;
pub mod hints;
mod positioning;

use self::{choices::ZoneChoices, positioning::Positioning};

const CREATURE_TOWN_CHOICES: NonZeroU32 = match NonZeroU32::new(raw::RMG_TERRAIN_TOWN_CHOICE_COUNT)
{
    Some(count) => count,
    None => panic!("the source terrain town domain must not be empty"),
};

/// A placement failure or an unsupported arithmetic domain.
#[derive(Debug)]
pub enum LayoutError {
    /// The selected ruleset needs a seed and hint-clock input.
    HintsRequired,
    /// Template and request use different generation rules.
    RulesetMismatch,
    /// Hint setup or a late town query encountered an unsupported native access.
    Hints(hints::ZoneFault),
    /// Request generation choices cannot be represented.
    Request(crate::request::InputError),
    /// The requested zone does not exist in the completed layout.
    UnknownZone(ZoneId),
    /// Hotfix rejects this layout; retail would draw modulo zero.
    NoCandidates(ZoneId),
    /// An intermediate cannot be represented by the original signed arithmetic.
    Arithmetic,
    /// A distance calculation could not be represented.
    Geometry(GeometryError),
    /// Workspace storage could not be reserved.
    Allocation(TryReserveError),
}
impl fmt::Display for LayoutError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::HintsRequired => {
                f.write_str("layout requires an original seed and hint-clock input")
            }
            Self::RulesetMismatch => f.write_str("layout request and template use different rules"),
            Self::Hints(error) => error.fmt(f),
            Self::Request(error) => error.fmt(f),
            Self::UnknownZone(zone) => write!(f, "layout has no zone {}", zone.index()),
            Self::NoCandidates(zone) => write!(f, "zone {} has no legal positions", zone.index()),
            Self::Arithmetic => f.write_str("unsupported zone-layout arithmetic"),
            Self::Geometry(error) => error.fmt(f),
            Self::Allocation(error) => error.fmt(f),
        }
    }
}
impl Error for LayoutError {}
impl From<hints::ZoneFault> for LayoutError {
    fn from(value: hints::ZoneFault) -> Self {
        Self::Hints(value)
    }
}
impl From<GeometryError> for LayoutError {
    fn from(value: GeometryError) -> Self {
        Self::Geometry(value)
    }
}
impl From<TryReserveError> for LayoutError {
    fn from(value: TryReserveError) -> Self {
        Self::Allocation(value)
    }
}

#[derive(Clone, Copy, Debug)]
struct PositionedZone {
    id: ZoneId,
    position: WorldPosition,
    alignment: Option<Town>,
}

/// A completed zone layout. Zero scaled size remains a defined retail case.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ZoneLayout {
    id: ZoneId,
    position: WorldPosition,
    scaled_size: u32,
    alignment: Option<Town>,
    terrain: Terrain,
    creature_town: Option<Town>,
}
impl ZoneLayout {
    /// Owning template zone.
    #[must_use]
    pub const fn id(self) -> ZoneId {
        self.id
    }
    /// Scaled zone centre and plane.
    #[must_use]
    pub const fn position(self) -> WorldPosition {
        self.position
    }
    /// Radius after scaling into the map; may be zero.
    #[must_use]
    pub const fn scaled_size(self) -> u32 {
        self.scaled_size
    }
    /// Zone's town faction, or neutral if no town is allowed.
    #[must_use]
    pub const fn alignment(self) -> Option<Town> {
        self.alignment
    }
    /// Resolved terrain, including underground substitution.
    #[must_use]
    pub const fn terrain(self) -> Terrain {
        self.terrain
    }
    /// Creature faction selected for dwellings and rewards; `None` is neutral.
    #[must_use]
    pub const fn creature_town(self) -> Option<Town> {
        self.creature_town
    }
}

/// Completed layout with the exact template, request and water choice used to
/// produce it. Boundary generation consumes this token instead of admitting
/// unrelated zone slices and preparation arguments.
pub struct Layout<'workspace, 'context> {
    zones: &'workspace [ZoneLayout],
    template: &'context Template<'context>,
    request: &'context Request,
    water: Water,
    players: [Option<Player>; crate::request::PLAYER_COUNT],
    choices: ZoneChoices,
}
impl<'context> Layout<'_, 'context> {
    /// Completed zone positions in source order, borrowing workspace storage.
    #[must_use]
    pub const fn zones(&self) -> &[ZoneLayout] {
        self.zones
    }
    /// Selected template from which these zones were produced.
    #[must_use]
    pub const fn template(&self) -> &'context Template<'context> {
        self.template
    }
    /// Request supplying map dimensions, planes and behavior.
    #[must_use]
    pub const fn request(&self) -> &'context Request {
        self.request
    }
    /// Resolved water choice used during layout.
    #[must_use]
    pub const fn water(&self) -> Water {
        self.water
    }
    /// Solved hints and any native diagnostics, including late town queries.
    #[must_use]
    pub fn hints(&self) -> Option<&hints::ZoneSolution> {
        self.choices.hints()
    }

    /// Select another town from a source zone, retaining the constructor's
    /// per-zone query count and independent hint RNG. This does not place it.
    ///
    /// # Errors
    /// Reports an unknown zone or a fault in native hint/query arithmetic.
    pub fn select_zone_town(
        &mut self,
        zone: ZoneId,
        rng: &mut RetailRng,
    ) -> Result<Option<Town>, LayoutError> {
        let zone = self
            .template
            .zones()
            .get(zone.index())
            .ok_or(LayoutError::UnknownZone(zone))?;
        self.choices.town(zone, rng)
    }
    pub(crate) const fn players(&self) -> [Option<Player>; crate::request::PLAYER_COUNT] {
        self.players
    }
}

/// Per-generation layout scratch. Capacity survives repeated calls.
#[derive(Default, Debug)]
pub struct LayoutWorkspace {
    positioned: Vec<PositionedZone>,
    candidates: Vec<WorldPosition>,
    completed: Vec<ZoneLayout>,
}
impl LayoutWorkspace {
    /// Position each zone, reposition every zone twice, and choose terrain.
    ///
    /// The returned stage token borrows the workspace and preparation context,
    /// preventing reuse while its layout is still in use. Scratch buffers contain
    /// no shared RNG state.
    ///
    /// # Errors
    /// Reports exhausted candidates, arithmetic faults, or failed reservations.
    pub fn generate<'workspace, 'context>(
        &'workspace mut self,
        selected: &SelectedTemplate<'context>,
        request: &'context Request,
        water: Water,
        rng: &mut RetailRng,
    ) -> Result<Layout<'workspace, 'context>, LayoutError> {
        if selected.template().ruleset() != Ruleset::Complete {
            return Err(LayoutError::HintsRequired);
        }
        self.generate_inner(selected, request, water, rng, ZoneChoices::Complete)
    }

    /// Solve this template's hints from the original map seed, then run the
    /// shared positioning and terrain stages. The returned layout owns late
    /// town-query state. Complete uses its ordinary path and never reads clock.
    ///
    /// Clock observations are FILETIME ticks requested by the hint solver;
    /// missing observations are errors, and native diagnostics stay in the
    /// returned layout. The seed is independent of the advanced CRT stream.
    ///
    /// # Errors
    /// Reports incompatible rules, hint faults or ordinary layout failures.
    pub fn generate_with_hints<'workspace, 'context>(
        &'workspace mut self,
        selected: &SelectedTemplate<'context>,
        request: &'context Request,
        water: Water,
        seed: u32,
        rng: &mut RetailRng,
        clock: impl FnMut(hints::ClockRead) -> Option<i64>,
    ) -> Result<Layout<'workspace, 'context>, LayoutError> {
        if selected.template().ruleset() != request.ruleset() {
            return Err(LayoutError::RulesetMismatch);
        }
        let choices = match selected.template().ruleset() {
            Ruleset::Complete => ZoneChoices::Complete,
            Ruleset::HotA181 => ZoneChoices::from_selected(selected, request, seed, clock)?,
        };
        self.generate_inner(selected, request, water, rng, choices)
    }

    fn generate_inner<'workspace, 'context>(
        &'workspace mut self,
        selected: &SelectedTemplate<'context>,
        request: &'context Request,
        water: Water,
        rng: &mut RetailRng,
        mut choices: ZoneChoices,
    ) -> Result<Layout<'workspace, 'context>, LayoutError> {
        if selected.template().ruleset() != request.ruleset() {
            return Err(LayoutError::RulesetMismatch);
        }
        let towns = request.generation_towns().map_err(LayoutError::Request)?;
        let zones = selected.template().zones();
        self.positioned.clear();
        self.completed.clear();
        self.positioned.try_reserve(zones.len())?;
        self.completed.try_reserve(zones.len())?;
        let side =
            i32::try_from(request.size().dimension()).map_err(|_| LayoutError::Arithmetic)?;
        let positioning = Positioning::new(selected.template(), request, water)?;
        for zone in zones {
            // Complete draws before the request override; constrained layouts
            // query the independent hint stream instead.
            let mut alignment = choices.town(zone, rng)?;
            let owner = match zone.role() {
                ZoneRole::Human(slot) | ZoneRole::Computer(slot) => Some(slot),
                ZoneRole::Treasure(owner) | ZoneRole::Junction(owner) => owner,
            };
            let town_count = zone.towns()[0]
                .initial_count
                .checked_add(zone.towns()[1].initial_count)
                .ok_or(LayoutError::Arithmetic)?;
            if town_count > 0 {
                if let Some(player) = owner.and_then(|slot| selected.player(slot)) {
                    if let TownChoice::Fixed(town) = towns[player.index()] {
                        alignment = Some(town);
                    }
                }
            }
            let mut placed = PositionedZone {
                id: zone.id(),
                alignment,
                position: WorldPosition {
                    point: Point::new(0, 0),
                    level: Level::Surface,
                },
            };
            self.position(
                &mut placed,
                zones,
                positioning,
                request.behavior().is_hotfix(),
                rng,
            )?;
            self.positioned.push(placed);
        }
        for _ in 0..2 {
            for index in 0..self.positioned.len() {
                let mut placed = self.positioned[index];
                self.position(
                    &mut placed,
                    zones,
                    positioning,
                    request.behavior().is_hotfix(),
                    rng,
                )?;
                self.positioned[index] = placed;
            }
        }
        let mut bounds = Bounds::default();
        for placed in &self.positioned {
            bounds.include(
                placed.position.point,
                positioning.radius(&zones[placed.id.index()]),
            )?;
        }
        let span = bounds.span()?;
        let origin = positioning.origin(&self.positioned, zones, bounds)?;
        for placed in &self.positioned {
            let zone = &zones[placed.id.index()];
            let mut position = placed.position;
            position.point = Point::new(
                scale(position.point.x, origin.x, side, span)?,
                scale(position.point.y, origin.y, side, span)?,
            );
            let scaled_size = u32::try_from(scale(zone_size(zone), 0, side, span)?)
                .map_err(|_| LayoutError::Arithmetic)?;
            let terrain = choices.terrain(zone, placed.alignment, position.level, rng);
            let creature_town = choices.creature_town(zone, placed.alignment, terrain, rng);
            self.completed.push(ZoneLayout {
                id: placed.id,
                position,
                scaled_size,
                alignment: placed.alignment,
                terrain,
                creature_town,
            });
        }
        Ok(Layout {
            zones: &self.completed,
            template: selected.template(),
            request,
            water,
            players: *selected.players(),
            choices,
        })
    }

    fn position(
        &mut self,
        current: &mut PositionedZone,
        zones: &[Zone],
        positioning: Positioning,
        hotfix: bool,
        rng: &mut RetailRng,
    ) -> Result<(), LayoutError> {
        self.candidates.clear();
        let levels = positioning.levels;
        if self.positioned.is_empty() {
            self.candidates.try_reserve(2)?;
            self.candidates.push(current.position);
            if levels == Levels::Underground {
                let position = WorldPosition {
                    level: Level::Underground,
                    ..current.position
                };
                self.append(current, position, zones, positioning)?;
            }
        } else {
            for connection in zones[current.id.index()].connections() {
                if !positioning.seeds_candidates(connection.options().kind) {
                    continue;
                }
                let Some(destination) = connection.destination() else {
                    continue;
                };
                let destination = destination.index();
                if destination < self.positioned.len() {
                    let center = if destination == current.id.index() {
                        *current
                    } else {
                        self.positioned[destination]
                    };
                    self.append_around(center, current, zones, positioning)?;
                }
            }
            if self.candidates.is_empty() {
                for index in 0..self.positioned.len() {
                    let center = if index == current.id.index() {
                        *current
                    } else {
                        self.positioned[index]
                    };
                    self.append_around(center, current, zones, positioning)?;
                }
            }
            self.filter(current, zones, positioning)?;
        }
        let count = u32::try_from(self.candidates.len()).map_err(|_| LayoutError::Arithmetic)?;
        let Some(count) = NonZeroU32::new(count) else {
            // Retail calls rand before its modulo-zero fault; hotfix checks first.
            if !hotfix {
                rng.draw();
            }
            return Err(LayoutError::NoCandidates(current.id));
        };
        current.position = self.candidates[rng.below(count) as usize];
        Ok(())
    }

    fn append(
        &mut self,
        current: &mut PositionedZone,
        position: WorldPosition,
        zones: &[Zone],
        positioning: Positioning,
    ) -> Result<(), LayoutError> {
        // Rejected trials also move the zone. A later self-connection observes it.
        current.position = position;
        if can_place(*current, &self.positioned, zones, positioning)? {
            self.candidates.try_reserve(1)?;
            self.candidates.push(position);
        }
        Ok(())
    }

    fn append_around(
        &mut self,
        center: PositionedZone,
        current: &mut PositionedZone,
        zones: &[Zone],
        positioning: Positioning,
    ) -> Result<(), LayoutError> {
        let center_size = zone_size(&zones[center.id.index()]);
        let current_size = zone_size(&zones[current.id.index()]);
        let radius = center_size
            .checked_add(current_size)
            .ok_or(LayoutError::Arithmetic)?;
        self.append_circle(center.position, current, radius, zones, positioning)?;
        if positioning.levels == Levels::Underground {
            let other = WorldPosition {
                level: center.position.level.other(),
                ..center.position
            };
            self.append(current, other, zones, positioning)?;
            self.append_circle(
                other,
                current,
                center_size.max(current_size),
                zones,
                positioning,
            )?;
        }
        Ok(())
    }

    fn append_circle(
        &mut self,
        center: WorldPosition,
        current: &mut PositionedZone,
        radius: i32,
        zones: &[Zone],
        positioning: Positioning,
    ) -> Result<(), LayoutError> {
        // k * 11.25 degrees clockwise; north is up, Y grows south.
        //         24
        //   16     .     0
        //          8
        for (&x, &y) in raw::RADIAL_COSINE_BITS.iter().zip(&raw::RADIAL_SINE_BITS) {
            let point = Point::new(
                radial(center.point.x, radius, x)?,
                radial(center.point.y, radius, y)?,
            );
            self.append(
                current,
                WorldPosition { point, ..center },
                zones,
                positioning,
            )?;
        }
        Ok(())
    }

    fn filter(
        &mut self,
        current: &PositionedZone,
        zones: &[Zone],
        positioning: Positioning,
    ) -> Result<(), LayoutError> {
        if positioning.levels == Levels::Underground {
            let mut occupied = [false; raw::RMG_MAP_LEVEL_COUNT as usize];
            for other in &self.positioned {
                if other.id != current.id {
                    occupied[other.position.level.index()] = true;
                }
            }
            // Retail bug retained in hotfix: a sole unused-level candidate at
            // index zero does not trigger filtering.
            if self
                .candidates
                .iter()
                .rposition(|c| !occupied[c.level.index()])
                .is_some_and(|i| i > 0)
            {
                self.candidates.retain(|c| !occupied[c.level.index()]);
            }
        }
        let connections = |position| -> Result<usize, LayoutError> {
            let trial = PositionedZone {
                position,
                ..*current
            };
            let mut count = 0;
            for connection in zones[current.id.index()].connections() {
                let Some(id) = connection.destination() else {
                    continue;
                };
                if let Some(&other) = self.positioned.get(id.index()) {
                    let other = if id == current.id { trial } else { other };
                    if can_connect(trial, other, zones)? {
                        count += positioning.connection_weight(
                            connection.options().kind,
                            position.level == other.position.level,
                        );
                    }
                }
            }
            Ok(count)
        };
        let mut best = 0;
        for &position in &self.candidates {
            best = best.max(connections(position)?);
        }
        // Compact in order, without allocating a parallel score array.
        let mut kept = 0;
        for index in 0..self.candidates.len() {
            let position = self.candidates[index];
            if connections(position)? >= best {
                self.candidates[kept] = position;
                kept += 1;
            }
        }
        self.candidates.truncate(kept);
        let mut bounds = Bounds::default();
        for other in &self.positioned {
            if other.id != current.id {
                bounds.include(
                    other.position.point,
                    positioning.radius(&zones[other.id.index()]),
                )?;
            }
        }
        let candidate_size = |position: WorldPosition| -> Result<i32, LayoutError> {
            let mut candidate = bounds;
            candidate.include(
                position.point,
                positioning.radius(&zones[current.id.index()]),
            )?;
            Ok(positioning.map_size.max(candidate.span()?))
        };
        let mut best_size = 32000;
        for &position in &self.candidates {
            best_size = best_size.min(candidate_size(position)?);
        }
        let mut kept = 0;
        for index in 0..self.candidates.len() {
            let position = self.candidates[index];
            if candidate_size(position)? <= best_size {
                self.candidates[kept] = position;
                kept += 1;
            }
        }
        self.candidates.truncate(kept);
        positioning.repel(current, &self.positioned, zones, &mut self.candidates)?;
        Ok(())
    }
}

fn zone_size(zone: &Zone) -> i32 {
    // Zone construction admits only a positive signed source size.
    i32::try_from(zone.size().get()).unwrap()
}

fn choose_terrain(
    zone: &Zone,
    alignment: Option<Town>,
    level: Level,
    rng: &mut RetailRng,
) -> Terrain {
    let terrain = if let Some(town) = alignment.filter(|_| zone.use_native_terrain()) {
        Terrain::parse(i32::try_from(raw::NATIVE_TERRAIN[town.index()]).unwrap()).unwrap()
    } else {
        let mut allowed = zone.allowed_terrain().to_vec();
        if level == Level::Surface {
            allowed[Terrain::Subterranean.index()] = false;
        }
        choose_flag(&allowed, rng).map_or(Terrain::Dirt, |index| {
            Terrain::parse(i32::try_from(index).unwrap()).unwrap()
        })
    };
    if level == Level::Underground && terrain != Terrain::Lava {
        Terrain::Subterranean
    } else {
        terrain
    }
}

fn can_place(
    current: PositionedZone,
    placed: &[PositionedZone],
    zones: &[Zone],
    positioning: Positioning,
) -> Result<bool, LayoutError> {
    let zone = &zones[current.id.index()];
    if let Some(level) = positioning.required_level(zone) {
        if current.position.level != level {
            return Ok(false);
        }
    } else if matches!(zone.role(), ZoneRole::Human(_) | ZoneRole::Computer(_))
        && current.position.level == Level::Underground
        && !current.alignment.is_some_and(|town| {
            matches!(
                i32::try_from(town.index()).unwrap(),
                raw::TOWN_INFERNO | raw::TOWN_NECROPOLIS | raw::TOWN_DUNGEON
            )
        })
    {
        return Ok(false);
    }
    for other in placed {
        if other.id == current.id || other.position.level != current.position.level {
            continue;
        }
        let distance = current.position.point.distance(other.position.point)?;
        let combined = zone_size(zone)
            .checked_add(zone_size(&zones[other.id.index()]))
            .ok_or(LayoutError::Arithmetic)?;
        if distance.checked_mul(10).ok_or(LayoutError::Arithmetic)?
            < combined.checked_mul(8).ok_or(LayoutError::Arithmetic)?
        {
            return Ok(false);
        }
    }
    Ok(true)
}

fn can_connect(
    first: PositionedZone,
    second: PositionedZone,
    zones: &[Zone],
) -> Result<bool, LayoutError> {
    let distance = first.position.point.distance(second.position.point)?;
    let first_size = zone_size(&zones[first.id.index()]);
    let second_size = zone_size(&zones[second.id.index()]);
    let combined = first_size
        .checked_add(second_size)
        .ok_or(LayoutError::Arithmetic)?;
    if first.position.level == second.position.level {
        Ok(combined.checked_mul(11).ok_or(LayoutError::Arithmetic)?
            >= distance.checked_mul(10).ok_or(LayoutError::Arithmetic)?)
    } else {
        Ok(combined >= distance && combined - distance > first_size.min(second_size) / 2)
    }
}

#[derive(Clone, Copy, Default)]
struct Bounds {
    minimum: Point,
    maximum: Point,
}
impl Bounds {
    fn include(&mut self, point: Point, size: i32) -> Result<(), LayoutError> {
        self.minimum.x = self
            .minimum
            .x
            .min(point.x.checked_sub(size).ok_or(LayoutError::Arithmetic)?);
        self.minimum.y = self
            .minimum
            .y
            .min(point.y.checked_sub(size).ok_or(LayoutError::Arithmetic)?);
        self.maximum.x = self.maximum.x.max(
            point
                .x
                .checked_add(size)
                .and_then(|v| v.checked_add(1))
                .ok_or(LayoutError::Arithmetic)?,
        );
        self.maximum.y = self.maximum.y.max(
            point
                .y
                .checked_add(size)
                .and_then(|v| v.checked_add(1))
                .ok_or(LayoutError::Arithmetic)?,
        );
        Ok(())
    }
    fn span(self) -> Result<i32, LayoutError> {
        Ok(self
            .maximum
            .x
            .checked_sub(self.minimum.x)
            .ok_or(LayoutError::Arithmetic)?
            .max(
                self.maximum
                    .y
                    .checked_sub(self.minimum.y)
                    .ok_or(LayoutError::Arithmetic)?,
            ))
    }
}

fn scale(value: i32, origin: i32, side: i32, span: i32) -> Result<i32, LayoutError> {
    value
        .checked_sub(origin)
        .and_then(|v| v.checked_mul(side))
        .and_then(|v| v.checked_div(span))
        .ok_or(LayoutError::Arithmetic)
}

fn radial(center: i32, radius: i32, direction_bits: u64) -> Result<i32, LayoutError> {
    let value = f64::from(center) + f64::from(radius) * f64::from_bits(direction_bits);
    // Rust float casts saturate. Model unsupported x87 conversions explicitly.
    if !value.is_finite()
        || value.trunc() < f64::from(i32::MIN)
        || value.trunc() > f64::from(i32::MAX)
    {
        return Err(LayoutError::Arithmetic);
    }
    #[allow(clippy::cast_possible_truncation)]
    Ok(value as i32)
}
