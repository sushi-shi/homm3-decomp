//! Source-zone adapter, native diagnostics and late town queries.

use super::{
    random::Random,
    tokens::{self, Condition, Rule, Who},
    unique, ClockRead, Fault, Issue, Problem, Relation, Solution,
};
use crate::{
    request::{Request, TownChoice},
    rules::Ruleset,
    selection::SelectedTemplate,
    template::ZoneRole,
};
use std::{error::Error, fmt};

/// Native hint diagnostic categories, in recovered vtable-list order.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
pub enum DiagnosticKind {
    /// No faction types survive.
    NoFaction,
    /// No terrain types survive.
    NoTerrain,
    /// No town types survive.
    NoTownType,
    /// An internal native invariant failed.
    ProgrammingError,
    /// The final assignments violate a relation.
    SolutionCheckFailed,
    /// A relation contradicts the zone's same-type setting.
    ViolatesSameType,
    /// A faction relation references a forced-neutral zone.
    ViolatesNeutralFaction,
    /// A relation contradicts native-terrain matching.
    ViolatesTerrainMatch,
    /// The numbered neutral town does not exist.
    InvalidTown,
    /// The referenced source zone does not exist.
    InvalidZone,
    /// The referenced zone has no neutral towns.
    NoNeutralTown,
    /// The referenced zone has no player town.
    NoPlayerTown,
    /// The referenced zone has no ordinary towns.
    NoTown,
    /// A reference can only resolve after density placement.
    DensityTown,
    /// A whole-zone town pattern targets itself.
    PatternTargetsItself,
    /// A town or terrain token has no recognized syntax.
    SyntaxError,
    /// A terrain relation targets itself.
    TerrainRelatedToItself,
    /// A town relation targets itself.
    TownRelatedToItself,
    /// A faction relation targets itself.
    FactionRelatedToItself,
    /// Town equality and inequality conflict.
    IncompatibleTowns,
    /// Town/terrain requirements conflict.
    IncompatibleTownTerrain,
    /// Terrain requirements conflict.
    IncompatibleTerrains,
    /// Search exhausted its candidates.
    NoSolution,
    /// Search exceeded its clock budget.
    Timeout,
}

/// Ordered native diagnostic payload, before UI translation.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Diagnostic {
    /// Native diagnostic category.
    pub kind: DiagnosticKind,
    /// Whether this diagnostic prevents an initial search.
    pub fatal: bool,
    /// Source zone IDs, town indices or diagnostic operands, in native order.
    pub values: [i32; 4],
    /// Original token bytes or native diagnostic source filename.
    pub text: Vec<u8>,
}
impl Diagnostic {
    pub(super) fn new(kind: DiagnosticKind, fatal: bool, values: [i32; 4]) -> Self {
        Self {
            kind,
            fatal,
            values,
            text: Vec::new(),
        }
    }
}

/// A failure outside the solver's ordinary native diagnostics.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum ZoneFault {
    /// Constraint engine admission or replay failure.
    Engine(Fault),
    /// Native `std::stoi` would throw on this capture.
    Integer(Vec<u8>),
    /// The selected ruleset has no hint adapter.
    Ruleset(Ruleset),
    /// The request cannot provide generation-time faction choices.
    Request(crate::request::InputError),
    /// Unsupported source arithmetic or catalog dimensions.
    Domain,
    /// A late different-type relation would index the byte before the domain.
    UnsetDifferentTown {
        /// Source zone owning the unresolved town.
        zone: i32,
        /// Town index, with -1 meaning the player town.
        instance: i32,
    },
}
impl fmt::Display for ZoneFault {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "zone hints: {self:?}")
    }
}
impl Error for ZoneFault {}
impl From<Fault> for ZoneFault {
    fn from(value: Fault) -> Self {
        Self::Engine(value)
    }
}

/// One source-zone record supplied to the hint library. Terrain masks use the
/// solver's land-only order: 0..7, Highlands, Wasteland.
#[derive(Clone, Debug)]
#[expect(
    clippy::struct_excessive_bools,
    reason = "independent native template settings"
)]
pub struct ZoneInput {
    /// Template source number, which may be sparse or repeated.
    pub id: i32,
    /// Allowed town types before a player's fixed choice.
    pub towns: [bool; 12],
    /// Allowed land terrains.
    pub terrains: [bool; 10],
    /// Optional Allowed Factions byte string; empty means unspecified.
    pub factions: Vec<u8>,
    /// All ordinary towns must have one type.
    pub same_type: bool,
    /// The terrain matches the player town or first neutral town.
    pub native_terrain: bool,
    /// At least one player town is requested.
    pub player_town: bool,
    /// Number of initial neutral towns; negative counts create none.
    pub neutral_towns: i32,
    /// Town hint tokens.
    pub town_hint: Vec<u8>,
    /// Terrain hint tokens.
    pub terrain_hint: Vec<u8>,
    /// Faction hint tokens.
    pub faction_hint: Vec<u8>,
    /// Force neutral creatures instead of a faction.
    pub neutral_creatures: bool,
    /// A chosen player town, if any.
    pub chosen_town: Option<u8>,
    /// Native density-town flag (the basic-neutral-town density column).
    pub density_towns: bool,
}
impl ZoneInput {
    /// An unrestricted zone with no towns, hints or overrides.
    #[must_use]
    pub fn new(id: i32) -> Self {
        Self {
            id,
            towns: [true; 12],
            terrains: [true; 10],
            factions: Vec::new(),
            same_type: false,
            native_terrain: false,
            player_town: false,
            neutral_towns: 0,
            town_hint: Vec::new(),
            terrain_hint: Vec::new(),
            faction_hint: Vec::new(),
            neutral_creatures: false,
            chosen_town: None,
            density_towns: false,
        }
    }
}

#[derive(Clone, Copy, Debug)]
struct TownKey {
    zone: i32,
    instance: i32,
}
#[derive(Debug)]
struct Context {
    zones: Vec<ZoneInput>,
    rules: Vec<Rule>,
    towns: Vec<TownKey>,
    terrains: Vec<i32>,
    diagnostics: Vec<Diagnostic>,
    reported: usize,
}

impl Context {
    fn emit(&mut self, kind: DiagnosticKind, fatal: bool, values: [i32; 4]) {
        unique(&mut self.diagnostics, Diagnostic::new(kind, fatal, values));
    }
    fn program(&mut self, file: &[u8], line: i32) {
        let mut diagnostic =
            Diagnostic::new(DiagnosticKind::ProgrammingError, true, [line, 0, 0, 0]);
        diagnostic.text = file.to_vec();
        unique(&mut self.diagnostics, diagnostic);
    }
    fn density(&self, zone: i32) -> bool {
        self.zones.iter().any(|z| z.id == zone && z.density_towns)
    }
    fn ordinary(&self, zone: i32) -> Vec<usize> {
        self.towns
            .iter()
            .enumerate()
            .filter_map(|(i, t)| (t.zone == zone && t.instance != -2).then_some(i))
            .collect()
    }
    fn check_zone(&mut self, own: i32, other: i32) -> bool {
        if !self.terrains.contains(&own) {
            self.program(b"condition.cpp", 0x202);
            return false;
        }
        if self.terrains.contains(&other) {
            return true;
        }
        self.emit(DiagnosticKind::InvalidZone, false, [own, other, 0, 0]);
        false
    }
    fn select(&mut self, own: i32, zone: i32, who: Who, density: bool, report: bool) -> Vec<usize> {
        use DiagnosticKind as D;
        let mut result = Vec::new();
        for (i, town) in self
            .towns
            .iter()
            .enumerate()
            .filter(|(_, t)| t.zone == zone)
        {
            let matches = match who {
                Who::Neutral => town.instance >= 0,
                Who::All => town.instance >= -1,
                Who::Player => town.instance == -1,
                Who::Faction => town.instance == -2,
                Who::Index(index) => town.instance == index,
            };
            if matches {
                result.push(i);
                if !matches!(who, Who::Neutral | Who::All) {
                    break;
                }
            }
        }
        if !result.is_empty() {
            return result;
        }
        let index = match who {
            Who::Neutral | Who::All => None,
            Who::Player => Some(-1),
            Who::Faction => Some(-2),
            Who::Index(i) => Some(i),
        };
        match index {
            Some(-1) => self.emit(D::NoPlayerTown, false, [own, zone, 0, 0]),
            Some(-2) => self.program(b"condition.cpp", 0x26f),
            Some(index) if !density => self.emit(D::InvalidTown, false, [own, zone, index, 0]),
            Some(index) if report => self.emit(D::DensityTown, false, [own, zone, 3, index]),
            None if !density => self.emit(
                if who == Who::Neutral {
                    D::NoNeutralTown
                } else {
                    D::NoTown
                },
                false,
                [own, zone, 0, 0],
            ),
            None if report => self.emit(
                D::DensityTown,
                false,
                [own, zone, if who == Who::Neutral { 1 } else { 2 }, 0],
            ),
            _ => (),
        }
        result
    }
    fn flush(&mut self, problem: &Problem) {
        use DiagnosticKind as D;
        for issue in &problem.issues[self.reported..] {
            let (kind, values) = match *issue {
                Issue::EmptyTown(id) => {
                    let key = self.towns[id.index];
                    if key.instance == -2 {
                        (D::NoFaction, [key.zone, 0, 0, 0])
                    } else {
                        (D::NoTownType, [key.instance, key.zone, 0, 0])
                    }
                }
                Issue::EmptyTerrain(id) => (D::NoTerrain, [self.terrains[id.index], 0, 0, 0]),
                Issue::SelfTownRelation(relation) => {
                    self.program(
                        b"town.cpp",
                        if relation == Relation::Same {
                            0x1f
                        } else {
                            0x38
                        },
                    );
                    continue;
                }
                Issue::SelfTerrainRelation(relation) => {
                    self.program(
                        b"terrain.cpp",
                        if relation == Relation::Same {
                            0x37
                        } else {
                            0x46
                        },
                    );
                    continue;
                }
                Issue::IncompatibleTowns(a, b) => {
                    let a = self.towns[a.index];
                    let b = self.towns[b.index];
                    (
                        D::IncompatibleTowns,
                        [a.zone, a.instance, b.zone, b.instance],
                    )
                }
                Issue::IncompatibleTownTerrain(a, b) => {
                    let a = self.towns[a.index];
                    (
                        D::IncompatibleTownTerrain,
                        [a.zone, a.instance, self.terrains[b.index], 0],
                    )
                }
                Issue::IncompatibleTerrains(a, b) => (
                    D::IncompatibleTerrains,
                    [self.terrains[a.index], self.terrains[b.index], 0, 0],
                ),
                Issue::NoSolution => (D::NoSolution, [0; 4]),
                Issue::Timeout => (D::Timeout, [0; 4]),
                Issue::SolutionCheckFailed => (D::SolutionCheckFailed, [0; 4]),
            };
            self.emit(kind, true, values);
        }
        self.reported = problem.issues.len();
    }

    // RVA 0x1f9ad0: neutral-town restrictions run before every other condition.
    #[expect(
        clippy::too_many_lines,
        reason = "ordered native condition dispatcher preserves diagnostic order"
    )]
    fn apply(
        &mut self,
        problem: &mut Problem,
        results: Option<&[Option<u8>]>,
    ) -> Result<(), ZoneFault> {
        for restrictions in [true, false] {
            for index in 0..self.rules.len() {
                let rule = self.rules[index].clone();
                if matches!(
                    rule.condition,
                    Condition::Restrict {
                        who: Who::Neutral,
                        ..
                    }
                ) != restrictions
                {
                    continue;
                }
                let own = rule.own;
                match rule.condition {
                    Condition::Pattern(other) => {
                        if !self.check_zone(own, other) {
                            continue;
                        }
                        if own == other {
                            self.program(b"sametowntypepattern.cpp", 0x34);
                            continue;
                        }
                        let density = self.density(own); // Native reuses the source flag for the target.
                        let mine = self.select(own, own, Who::All, density, true);
                        let theirs = self.select(own, other, Who::All, density, true);
                        for a in mine {
                            if let Some(&b) = theirs
                                .iter()
                                .find(|&&b| self.towns[b].instance == self.towns[a].instance)
                            {
                                problem.relate_towns(
                                    problem.town_id(a),
                                    problem.town_id(b),
                                    Relation::Same,
                                )?;
                                self.flush(problem);
                            }
                        }
                    }
                    Condition::Towns {
                        zone,
                        who,
                        target,
                        relation,
                    } => {
                        if !self.check_zone(own, zone) {
                            continue;
                        }
                        let mine = self.select(own, own, who, self.density(own), true);
                        let theirs = self.select(own, zone, target, self.density(zone), true);
                        for a in mine {
                            for &b in &theirs {
                                if a != b {
                                    problem.relate_towns(
                                        problem.town_id(a),
                                        problem.town_id(b),
                                        relation,
                                    )?;
                                    self.flush(problem);
                                }
                            }
                        }
                    }
                    Condition::Restrict { who, allowed } => {
                        for town in self.select(own, own, who, self.density(own), false) {
                            if results.is_some_and(|values| values[town].is_some()) {
                                continue;
                            }
                            problem.restrict_town(problem.town_id(town), allowed)?;
                            self.flush(problem);
                        }
                    }
                    Condition::TownTerrain {
                        town_zone,
                        terrain_zone,
                        who,
                        relation,
                    } => {
                        if !self.check_zone(own, town_zone) || !self.check_zone(own, terrain_zone) {
                            continue;
                        }
                        let terrain = self
                            .terrains
                            .iter()
                            .position(|&z| z == terrain_zone)
                            .ok_or(ZoneFault::Domain)?;
                        for town in self.select(own, town_zone, who, self.density(town_zone), true)
                        {
                            problem.relate_town_terrain(
                                problem.town_id(town),
                                problem.terrain_id(terrain),
                                relation,
                            )?;
                        }
                    }
                    Condition::Terrain { zone, relation } => {
                        if !self.check_zone(own, zone) {
                            continue;
                        }
                        if own == zone {
                            self.program(b"terraintoterraincondition.cpp", 0x3e);
                            continue;
                        }
                        let a = self
                            .terrains
                            .iter()
                            .position(|&z| z == own)
                            .ok_or(ZoneFault::Domain)?;
                        let b = self
                            .terrains
                            .iter()
                            .position(|&z| z == zone)
                            .ok_or(ZoneFault::Domain)?;
                        problem.relate_terrains(
                            problem.terrain_id(a),
                            problem.terrain_id(b),
                            relation,
                        )?;
                        self.flush(problem);
                    }
                }
            }
        }
        Ok(())
    }
}

/// Selected zone hints with persistent per-map solver state and diagnostics.
#[derive(Debug)]
pub struct ZoneSolution {
    context: Context,
    solution: Solution,
}
impl ZoneSolution {
    /// Feed the shared selected-template model to the native hint algorithm.
    ///
    /// # Errors
    /// Reports incompatible rules, source arithmetic, parser exceptions or a
    /// missing clock observation. Native hint diagnostics remain in the result.
    pub fn from_selected(
        selected: &SelectedTemplate<'_>,
        request: &Request,
        seed: u32,
        clock: impl FnMut(ClockRead) -> Option<i64>,
    ) -> Result<Self, ZoneFault> {
        for rules in [request.ruleset(), selected.template().ruleset()] {
            if rules != Ruleset::HotA181 {
                return Err(ZoneFault::Ruleset(rules));
            }
        }
        let choices = request.generation_towns().map_err(ZoneFault::Request)?;
        let mut zones = Vec::new();
        for zone in selected.template().zones() {
            if zone.allowed_towns().len() != 12 || zone.allowed_terrain().len() != 12 {
                return Err(ZoneFault::Domain);
            }
            let options = zone.options();
            let mut input = ZoneInput::new(zone.source_number());
            input.towns.copy_from_slice(zone.allowed_towns());
            input.terrains =
                std::array::from_fn(|i| zone.allowed_terrain()[if i < 8 { i } else { i + 2 }]);
            input.factions = options.allowed_factions.clone().unwrap_or_default();
            input.same_type = zone.neutral_towns_match_alignment();
            input.native_terrain = zone.use_native_terrain();
            input.player_town = zone.towns()[0]
                .initial_count
                .checked_add(zone.towns()[1].initial_count)
                .ok_or(ZoneFault::Domain)?
                > 0;
            input.neutral_towns = zone.towns()[2]
                .initial_count
                .checked_add(zone.towns()[3].initial_count)
                .ok_or(ZoneFault::Domain)?;
            input.town_hint = options.town_hint.clone().unwrap_or_default();
            input.terrain_hint = options.terrain_hint.clone().unwrap_or_default();
            input.faction_hint = options.faction_hint.clone().unwrap_or_default();
            input.neutral_creatures = options.force_neutral_creatures;
            input.density_towns = zone.towns()[3].density.is_some();
            if let ZoneRole::Human(slot) | ZoneRole::Computer(slot) = zone.role() {
                if let Some(player) = selected.player(slot) {
                    if let TownChoice::Fixed(town) = choices[player.index()] {
                        input.chosen_town =
                            Some(u8::try_from(town.index()).map_err(|_| ZoneFault::Domain)?);
                    }
                }
            }
            zones.push(input);
        }
        Self::solve(zones, seed, clock)
    }

    /// Solve fresh source-zone inputs. Repeated map generation creates fresh
    /// conditions; late town queries retain and reapply this solve's conditions.
    ///
    /// # Errors
    /// Reports invalid numeric captures, invalid fixed towns or missing clock
    /// observations. Ordinary native diagnostic outcomes are returned as values.
    #[expect(
        clippy::too_many_lines,
        reason = "source-ordered setup passes preserve diagnostics and seeding gates"
    )]
    pub fn solve(
        zones: Vec<ZoneInput>,
        seed: u32,
        clock: impl FnMut(ClockRead) -> Option<i64>,
    ) -> Result<Self, ZoneFault> {
        let mut context = Context {
            zones,
            rules: Vec::new(),
            towns: Vec::new(),
            terrains: Vec::new(),
            diagnostics: Vec::new(),
            reported: 0,
        };
        let mut problem = Problem::new()?;
        for zone in &context.zones {
            if zone.player_town {
                problem.add_town(zone.towns, zone.chosen_town)?;
                context.towns.push(TownKey {
                    zone: zone.id,
                    instance: -1,
                });
            }
            for instance in 0..zone.neutral_towns {
                problem.add_town(zone.towns, None)?;
                context.towns.push(TownKey {
                    zone: zone.id,
                    instance,
                });
            }
            problem.add_terrain(zone.terrains);
            context.terrains.push(zone.id);
        }
        for zone in &context.zones {
            tokens::parse(
                zone.id,
                [&zone.town_hint, &zone.terrain_hint, &zone.faction_hint],
                &mut context.diagnostics,
                &mut context.rules,
            )?;
        }
        let mut factions = Vec::new();
        for rule in &context.rules {
            match rule.condition {
                Condition::Towns {
                    zone, who, target, ..
                } => {
                    if who == Who::Faction {
                        factions.push(rule.own);
                    }
                    if target == Who::Faction {
                        factions.push(zone);
                    }
                }
                Condition::TownTerrain {
                    town_zone,
                    who: Who::Faction,
                    ..
                } => factions.push(town_zone),
                _ => (),
            }
        }
        for zone in &context.zones {
            if !zone.factions.is_empty() || factions.contains(&zone.id) {
                problem.add_town(
                    if zone.factions.is_empty() {
                        zone.towns
                    } else {
                        tokens::town_set(&zone.factions)
                    },
                    None,
                )?;
                context.towns.push(TownKey {
                    zone: zone.id,
                    instance: -2,
                });
            }
        }
        context.apply(&mut problem, None)?;
        for index in 0..context.zones.len() {
            let zone = &context.zones[index];
            let (id, same, native, has_player) = (
                zone.id,
                zone.same_type,
                zone.native_terrain,
                zone.player_town,
            );
            let towns = context.ordinary(id);
            if same {
                for (i, &a) in towns.iter().enumerate() {
                    for &b in &towns[i + 1..] {
                        if problem.towns[a].different.contains(&b) {
                            context.emit(DiagnosticKind::ViolatesSameType, true, [id, 0, 0, 0]);
                        } else {
                            problem.relate_towns(
                                problem.town_id(a),
                                problem.town_id(b),
                                Relation::Same,
                            )?;
                            context.flush(&problem);
                        }
                    }
                }
            }
            if native {
                let terrain = context
                    .terrains
                    .iter()
                    .position(|&z| z == id)
                    .ok_or(ZoneFault::Domain)?;
                problem.allow_all_terrain(problem.terrain_id(terrain))?;
                for town in towns {
                    if context.towns[town].instance != if has_player { -1 } else { 0 } {
                        continue;
                    }
                    if problem.towns[town].unmatched.contains(&terrain) {
                        context.emit(DiagnosticKind::ViolatesTerrainMatch, true, [id, 0, 0, 0]);
                    } else {
                        problem.relate_town_terrain(
                            problem.town_id(town),
                            problem.terrain_id(terrain),
                            Relation::Same,
                        )?;
                    }
                }
            }
        }
        // Native sorts the violating (source, target) pairs before reporting.
        let mut neutral = Vec::new();
        for rule in &context.rules {
            if let Condition::Towns {
                zone, who, target, ..
            } = rule.condition
            {
                for referenced in [
                    (who == Who::Faction).then_some(rule.own),
                    (target == Who::Faction).then_some(zone),
                ]
                .into_iter()
                .flatten()
                {
                    if context
                        .zones
                        .iter()
                        .any(|z| z.id == referenced && z.neutral_creatures)
                    {
                        neutral.push((rule.own, referenced));
                    }
                }
            }
        }
        neutral.sort_unstable();
        for (own, other) in neutral {
            context.emit(
                DiagnosticKind::ViolatesNeutralFaction,
                true,
                [own, other, 0, 0],
            );
        }
        problem.check_domains();
        context.flush(&problem);
        let solution = if context.diagnostics.iter().any(|error| error.fatal) {
            problem.unsolved()
        } else {
            problem.solve(seed, clock)?
        };
        context.flush(&solution.problem);
        Ok(Self { context, solution })
    }

    /// Ordered, deduplicated native diagnostics, including late-query diagnostics.
    #[must_use]
    pub fn diagnostics(&self) -> &[Diagnostic] {
        &self.context.diagnostics
    }
    /// Independent solver RNG draws; absent until its first seed operation.
    #[must_use]
    pub fn draws(&self) -> Option<u64> {
        self.solution.draws()
    }
    /// The first terrain variable for a source zone, in solver terrain order.
    #[must_use]
    pub fn terrain(&self, zone: i32) -> Option<u8> {
        self.context
            .terrains
            .iter()
            .position(|&z| z == zone)
            .and_then(|i| self.solution.terrains[i])
    }
    /// Native faction result: -1 forced neutral, -2 unspecified, otherwise 0..11.
    #[must_use]
    pub fn faction(&self, zone: i32) -> i32 {
        if self
            .context
            .zones
            .iter()
            .any(|z| z.id == zone && z.neutral_creatures)
        {
            return -1;
        }
        self.context
            .towns
            .iter()
            .position(|key| key.zone == zone && key.instance == -2)
            .and_then(|i| self.solution.towns[i])
            .map_or(-2, i32::from)
    }
    /// Query or create a town. -1 is the player town; nonnegative indices are
    /// neutral-town instances. An empty late domain returns zero once but leaves
    /// its stored result unresolved, exactly as the native selector does.
    ///
    /// # Errors
    /// Reports a native out-of-bounds domain write caused by a different-type
    /// relation to an unresolved town, or a constraint engine input fault.
    pub fn town(&mut self, zone: i32, instance: i32) -> Result<Option<u8>, ZoneFault> {
        if let Some(index) = self
            .context
            .towns
            .iter()
            .position(|key| key.zone == zone && key.instance == instance && key.instance != -2)
        {
            return Ok(self.solution.towns[index]);
        }
        let Some(input) = self.context.zones.iter().find(|z| z.id == zone) else {
            self.context.program(b"typegenerator_impl.cpp", 0x1b0);
            return Ok(None);
        };
        let same = input.same_type;
        let id = self.solution.problem.add_town(input.towns, None)?;
        self.context.towns.push(TownKey { zone, instance });
        self.solution.towns.push(None);
        let towns = self.context.ordinary(zone);
        if same && towns.len() > 1 {
            self.solution.towns[id.index] = self.solution.towns[towns[0]];
            return Ok(self.solution.towns[id.index]);
        }
        self.context
            .apply(&mut self.solution.problem, Some(&self.solution.towns))?;
        let town = &self.solution.problem.towns[id.index];
        if let Some(&other) = town.same.first() {
            self.solution.towns[id.index] = self.solution.towns[other];
            return Ok(self.solution.towns[id.index]);
        }
        let mut allowed = town.domain;
        for &other in &town.different {
            let Some(value) = self.solution.towns[other] else {
                let key = self.context.towns[other];
                return Err(ZoneFault::UnsetDifferentTown {
                    zone: key.zone,
                    instance: key.instance,
                });
            };
            allowed[usize::from(value)] = false;
        }
        for &terrain in &town.matched {
            for (i, keep) in allowed.iter_mut().enumerate() {
                *keep &= Some(super::NATIVE_TERRAIN[i]) == self.solution.terrains[terrain];
            }
        }
        for &terrain in &town.unmatched {
            for (i, keep) in allowed.iter_mut().enumerate() {
                *keep &= Some(super::NATIVE_TERRAIN[i]) != self.solution.terrains[terrain];
            }
        }
        if !allowed.contains(&true) {
            allowed = town.domain;
        }
        if !allowed.contains(&true) {
            return Ok(Some(0));
        }
        let mut candidates: Vec<u8> = (0_u8..12).filter(|&i| allowed[usize::from(i)]).collect();
        self.solution
            .random
            .get_or_insert_with(|| Random::new(0))
            .shuffle(&mut candidates);
        self.solution.towns[id.index] = Some(candidates[0]);
        Ok(self.solution.towns[id.index])
    }
}
