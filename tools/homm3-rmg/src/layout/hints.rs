//! Town/terrain constraint solving owned by zone layout.
//!
//! The graph and randomized backjumping search follow the pinned 1.8.1 DLL's
//! `typegen` library, RVAs 0x20bbd0..0x20f1b0. This is the constraint engine;
//! template hint parsing and late density-town queries are separate consumers.

mod random;
mod search;
mod tokens;
mod zones;
pub use zones::{Diagnostic, DiagnosticKind, ZoneFault, ZoneInput, ZoneSolution};
#[cfg(test)]
mod tests;

use crate::identity::OwnerId;
use std::{error::Error, fmt};

/// Native town-to-land-terrain mapping in solver order. The final two terrain
/// indices mean Highlands and Wasteland, not the map's Water and Rock.
/// Pinned DLL RVA 0x20c280.
pub const NATIVE_TERRAIN: [u8; 12] = [2, 2, 3, 7, 0, 0, 5, 4, 8, 4, 9, 3];

/// One town variable belonging to a particular problem.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TownId {
    owner: OwnerId,
    index: usize,
}
/// One terrain variable belonging to a particular problem.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TerrainId {
    owner: OwnerId,
    index: usize,
}

/// Equality or inequality between town types or native terrain types.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Relation {
    /// Require equality.
    Same,
    /// Require inequality.
    Different,
}

/// A native solver diagnostic. Contradictions are collected in source order;
/// they do not authorize skipping subsequent component searches.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Issue {
    /// A town or faction has no admitted types.
    EmptyTown(TownId),
    /// A terrain has no admitted types.
    EmptyTerrain(TerrainId),
    /// A relation directly targets itself (a native programming error).
    SelfTownRelation(Relation),
    /// A terrain relation directly targets itself.
    SelfTerrainRelation(Relation),
    /// Equality and inequality constrain the same towns.
    IncompatibleTowns(TownId, TownId),
    /// A town must both match and differ from a terrain.
    IncompatibleTownTerrain(TownId, TerrainId),
    /// Equal terrains must also differ.
    IncompatibleTerrains(TerrainId, TerrainId),
    /// A component exhausted its candidate assignments.
    NoSolution,
    /// A component exceeded the native ten-second wall-clock budget.
    Timeout,
    /// The resulting values do not satisfy the original relations.
    SolutionCheckFailed,
}

/// Invalid input to the constraint engine, distinct from a native diagnostic.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Fault {
    /// Process-local problem identities have been exhausted.
    IdentityExhausted,
    /// A handle belongs to a different problem.
    ForeignHandle,
    /// A fixed town is outside 0..12.
    TownType(u8),
    /// A clock observation required by the native search was not supplied.
    MissingClock(ClockRead),
}
impl fmt::Display for Fault {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "layout constraints: {self:?}")
    }
}
impl Error for Fault {}

/// A wall-clock read in the native search. Times use Windows FILETIME's signed
/// 100-nanosecond units; recording these makes long searches replayable.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ClockRead {
    /// Connected-component ordinal, in native source order.
    pub component: usize,
    /// Zero at component entry, otherwise a multiple of 500,000 iterations.
    pub steps: i32,
}

#[derive(Clone, Debug)]
struct Town {
    domain: [bool; 12],
    fixed: Option<u8>,
    same: Vec<usize>,
    different: Vec<usize>,
    matched: Vec<usize>,
    unmatched: Vec<usize>,
}
#[derive(Clone, Debug)]
struct Terrain {
    domain: [bool; 10],
    same: Vec<usize>,
    different: Vec<usize>,
    matched: Vec<usize>,
    unmatched: Vec<usize>,
}

/// Ordered town and terrain constraints. Factions use ordinary town variables;
/// source-zone labels and hint diagnostics belong to the template adapter.
#[derive(Debug)]
pub struct Problem {
    owner: OwnerId,
    towns: Vec<Town>,
    terrains: Vec<Terrain>,
    issues: Vec<Issue>,
}

fn unique<T: PartialEq>(values: &mut Vec<T>, value: T) {
    if !values.contains(&value) {
        values.push(value);
    }
}

impl Problem {
    /// Start an empty, independently owned constraint problem.
    ///
    /// # Errors
    /// Reports exhausted process-local ownership identities.
    pub fn new() -> Result<Self, Fault> {
        Ok(Self {
            owner: OwnerId::new().ok_or(Fault::IdentityExhausted)?,
            towns: Vec::new(),
            terrains: Vec::new(),
            issues: Vec::new(),
        })
    }
    fn town_id(&self, index: usize) -> TownId {
        TownId {
            owner: self.owner,
            index,
        }
    }
    fn terrain_id(&self, index: usize) -> TerrainId {
        TerrainId {
            owner: self.owner,
            index,
        }
    }
    fn check(&self, owner: OwnerId) -> Result<(), Fault> {
        if owner == self.owner {
            Ok(())
        } else {
            Err(Fault::ForeignHandle)
        }
    }

    /// Append a town or faction, in template order. A fixed player town replaces
    /// the supplied domain, as in DLL RVA 0x20f810.
    ///
    /// # Errors
    /// Rejects fixed town types outside the twelve-faction domain.
    pub fn add_town(&mut self, mut domain: [bool; 12], fixed: Option<u8>) -> Result<TownId, Fault> {
        if let Some(value) = fixed {
            if usize::from(value) >= domain.len() {
                return Err(Fault::TownType(value));
            }
            domain.fill(false);
            domain[usize::from(value)] = true;
        }
        let id = self.town_id(self.towns.len());
        self.towns.push(Town {
            domain,
            fixed,
            same: Vec::new(),
            different: Vec::new(),
            matched: Vec::new(),
            unmatched: Vec::new(),
        });
        Ok(id)
    }

    /// Append a zone's allowed land terrains, in source order.
    pub fn add_terrain(&mut self, domain: [bool; 10]) -> TerrainId {
        let id = self.terrain_id(self.terrains.len());
        self.terrains.push(Terrain {
            domain,
            same: Vec::new(),
            different: Vec::new(),
            matched: Vec::new(),
            unmatched: Vec::new(),
        });
        id
    }

    /// Intersect a town domain; a fixed player's choice ignores restrictions.
    ///
    /// # Errors
    /// Rejects handles from other problems before changing any domain.
    pub fn restrict_town(&mut self, id: TownId, allowed: [bool; 12]) -> Result<(), Fault> {
        self.check(id.owner)?;
        if self.towns[id.index].fixed.is_none() {
            for (value, keep) in self.towns[id.index].domain.iter_mut().zip(allowed) {
                *value &= keep;
            }
            if !self.towns[id.index].domain.contains(&true) {
                unique(&mut self.issues, Issue::EmptyTown(id));
            }
        }
        Ok(())
    }

    /// Replace the terrain restriction when a zone requires native town terrain.
    ///
    /// # Errors
    /// Rejects handles from other problems.
    pub fn allow_all_terrain(&mut self, id: TerrainId) -> Result<(), Fault> {
        self.check(id.owner)?;
        self.terrains[id.index].domain.fill(true);
        Ok(())
    }

    /// Relate two towns. Native town links retain duplicate entries.
    ///
    /// # Errors
    /// Rejects handles from other problems. Native contradictions are diagnostics.
    pub fn relate_towns(&mut self, a: TownId, b: TownId, relation: Relation) -> Result<(), Fault> {
        self.check(a.owner)?;
        self.check(b.owner)?;
        if a == b {
            unique(&mut self.issues, Issue::SelfTownRelation(relation));
            return Ok(());
        }
        let opposite = match relation {
            Relation::Same => &self.towns[a.index].different,
            Relation::Different => &self.towns[a.index].same,
        };
        if opposite.contains(&b.index) {
            unique(&mut self.issues, Issue::IncompatibleTowns(a, b));
            return Ok(());
        }
        match relation {
            Relation::Same => {
                self.towns[a.index].same.push(b.index);
                self.towns[b.index].same.push(a.index);
            }
            Relation::Different => {
                self.towns[a.index].different.push(b.index);
                self.towns[b.index].different.push(a.index);
            }
        }
        Ok(())
    }

    /// Relate a town's native terrain and a zone's terrain, with unique links.
    ///
    /// # Errors
    /// Rejects handles from other problems.
    pub fn relate_town_terrain(
        &mut self,
        town: TownId,
        terrain: TerrainId,
        relation: Relation,
    ) -> Result<(), Fault> {
        self.check(town.owner)?;
        self.check(terrain.owner)?;
        match relation {
            Relation::Same => {
                unique(&mut self.towns[town.index].matched, terrain.index);
                unique(&mut self.terrains[terrain.index].matched, town.index);
            }
            Relation::Different => {
                unique(&mut self.towns[town.index].unmatched, terrain.index);
                unique(&mut self.terrains[terrain.index].unmatched, town.index);
            }
        }
        Ok(())
    }

    /// Relate two zone terrains, with unique mirrored links.
    ///
    /// # Errors
    /// Rejects handles from other problems. Self-links become native diagnostics.
    pub fn relate_terrains(
        &mut self,
        a: TerrainId,
        b: TerrainId,
        relation: Relation,
    ) -> Result<(), Fault> {
        self.check(a.owner)?;
        self.check(b.owner)?;
        if a == b {
            unique(&mut self.issues, Issue::SelfTerrainRelation(relation));
            return Ok(());
        }
        match relation {
            Relation::Same => {
                unique(&mut self.terrains[a.index].same, b.index);
                unique(&mut self.terrains[b.index].same, a.index);
            }
            Relation::Different => {
                unique(&mut self.terrains[a.index].different, b.index);
                unique(&mut self.terrains[b.index].different, a.index);
            }
        }
        Ok(())
    }

    /// Search under the pinned solver rules, preserving candidate order and its
    /// independent random stream. All native diagnostics survive in the result.
    /// The clock callback supplies observations; no host clock is read implicitly.
    ///
    /// # Errors
    /// Reports a missing clock observation. Native no-solution and timeout cases
    /// return a solution with diagnostics and the native fallback values.
    pub fn solve(
        mut self,
        seed: u32,
        mut clock: impl FnMut(ClockRead) -> Option<i64>,
    ) -> Result<Solution, Fault> {
        self.check_domains();
        if !self.issues.is_empty() {
            return Ok(self.unsolved());
        }
        let mut graph = search::Graph::build(&mut self);
        let mut random = random::Random::new(seed);
        graph.search(&mut random, &mut self.issues, &mut clock)?;
        let towns = self
            .towns
            .iter()
            .enumerate()
            .map(|(i, t)| t.fixed.or(Some(graph.town_value(i))))
            .collect();
        let terrains = (0..self.terrains.len())
            .map(|i| Some(graph.terrain_value(i)))
            .collect();
        let mut solution = Solution {
            problem: self,
            towns,
            terrains,
            random: Some(random),
        };
        if solution.problem.issues.is_empty() && !solution.check() {
            solution.problem.issues.push(Issue::SolutionCheckFailed);
        }
        Ok(solution)
    }

    fn check_domains(&mut self) {
        for i in 0..self.towns.len() {
            if !self.towns[i].domain.contains(&true) {
                let id = self.town_id(i);
                unique(&mut self.issues, Issue::EmptyTown(id));
            }
        }
        for i in 0..self.terrains.len() {
            if !self.terrains[i].domain.contains(&true) {
                let id = self.terrain_id(i);
                unique(&mut self.issues, Issue::EmptyTerrain(id));
            }
        }
    }
    fn unsolved(self) -> Solution {
        Solution {
            towns: self.towns.iter().map(|t| t.fixed).collect(),
            terrains: vec![None; self.terrains.len()],
            problem: self,
            random: None,
        }
    }
}

/// Solver values and diagnostics, retaining the independent RNG for later town
/// selection. Even a failed native search may produce values; inspect `issues`.
#[derive(Debug)]
pub struct Solution {
    problem: Problem,
    towns: Vec<Option<u8>>,
    terrains: Vec<Option<u8>>,
    random: Option<random::Random>,
}
impl Solution {
    /// Ordered native diagnostics.
    #[must_use]
    pub fn issues(&self) -> &[Issue] {
        &self.problem.issues
    }
    /// MT19937 words consumed, independent of the ordinary RMG CRT stream.
    /// Absent when preparation failed before the native seed operation.
    #[must_use]
    pub fn draws(&self) -> Option<u64> {
        self.random.as_ref().map(|random| random.draws)
    }
    /// A town's selected type; absent when preparation failed before search.
    ///
    /// # Errors
    /// Rejects a handle from another problem.
    pub fn town(&self, id: TownId) -> Result<Option<u8>, Fault> {
        self.problem.check(id.owner)?;
        Ok(self.towns[id.index])
    }
    /// A terrain in solver order (0..7, Highlands, Wasteland).
    ///
    /// # Errors
    /// Rejects a handle from another problem.
    pub fn terrain(&self, id: TerrainId) -> Result<Option<u8>, Fault> {
        self.problem.check(id.owner)?;
        Ok(self.terrains[id.index])
    }
    fn check(&self) -> bool {
        let native = |town: usize| self.towns[town].map(|v| NATIVE_TERRAIN[usize::from(v)]);
        self.problem.towns.iter().enumerate().all(|(i, t)| {
            t.same.iter().all(|&j| self.towns[i] == self.towns[j])
                && t.different.iter().all(|&j| self.towns[i] != self.towns[j])
                && t.matched.iter().all(|&j| native(i) == self.terrains[j])
                && t.unmatched.iter().all(|&j| native(i) != self.terrains[j])
        }) && self.problem.terrains.iter().enumerate().all(|(i, t)| {
            t.same.iter().all(|&j| self.terrains[i] == self.terrains[j])
                && t.different
                    .iter()
                    .all(|&j| self.terrains[i] != self.terrains[j])
        })
    }
}
