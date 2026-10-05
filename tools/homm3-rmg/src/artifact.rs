//! Map-owned artifact exclusions, template choices and quest selection.

use crate::{
    raw,
    request::Water,
    rng::RetailRng,
    rules::Ruleset,
    template::{apply_availability, Availability, MapOptions},
    traits::{ArtifactCatalog, ArtifactClass, ArtifactId, ArtifactTraits},
};
use std::{error::Error, fmt};

/// Invalid per-map artifact configuration.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ArtifactPoolError {
    /// Extended template choices require expansion rules.
    UnsupportedRuleset(Ruleset),
    /// A signed ID in this template field overflows the native integer domain.
    IdOverflow(&'static str),
}
impl fmt::Display for ArtifactPoolError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "artifact pool: {self:?}")
    }
}
impl Error for ArtifactPoolError {}

/// Availability observed at the native quest-completion callback.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct QuestArtifactSelection {
    /// Selected candidate, without claiming it until placement succeeds.
    pub artifact: Option<ArtifactId>,
    /// Unbanned treasure-class artifacts, before the combination filter.
    pub pool: usize,
    /// Artifacts passing both ordinary eligibility and the combination filter.
    pub candidates: usize,
}
impl QuestArtifactSelection {
    /// Native low-pool threshold. Under `HotA` this deliberately counts eligible
    /// components of permitted combinations even though selection excludes them.
    #[must_use]
    pub const fn pool_low(self) -> bool {
        self.pool < raw::RMG_LOW_QUEST_ARTIFACT_COUNT as usize
    }
}

/// Per-generation mutable availability over a borrowed immutable catalog.
/// Complete's exclusions are its used quest artifacts. `HotA` shares these bans
/// with template settings and output; successful quests add their artifact here.
#[derive(Clone, Debug)]
pub struct ArtifactPool<'a> {
    catalog: &'a ArtifactCatalog,
    excluded: Box<[bool]>,
    combinations: Box<[bool]>,
    overrides: Box<[Availability]>,
    combination_overrides: Box<[Availability]>,
}
impl<'a> ArtifactPool<'a> {
    /// Apply native constructor defaults, before any template has been selected.
    #[must_use]
    pub fn new(catalog: &'a ArtifactCatalog, water: Water) -> Self {
        let rules = catalog.ruleset();
        let mut excluded = vec![false; catalog.entries().len()].into_boxed_slice();
        let mut combinations = vec![false; rules.combination_count()].into_boxed_slice();
        if rules == Ruleset::HotA181 {
            // DLL RVA 0x1b8f30 prologue: assembled artifacts and fixed bans.
            for (ban, row) in excluded.iter_mut().zip(catalog.entries()) {
                *ban = row.combination().is_some();
            }
            for id in [127, 128, 161, 165, 126, 83, 57, 58, 59] {
                excluded[id] = true;
            }
            if water == Water::None {
                for id in [71, 90, 123, 136] {
                    excluded[id] = true;
                }
            }
            combinations[1] = true; // Cloak of the Undead King.
        }
        Self {
            catalog,
            excluded,
            combinations,
            overrides: vec![Availability::Inherit; catalog.entries().len()].into_boxed_slice(),
            combination_overrides: vec![Availability::Inherit; rules.combination_count()]
                .into_boxed_slice(),
        }
    }
    /// Apply the selected template's artifact and combination choices, in native
    /// order. Disabled traits and non-random classes ignore explicit overrides.
    /// The no-artifacts quirk resets artifact 8's override without lifting its ban.
    ///
    /// # Errors
    /// Rejects Complete rules and ID overflow. Invalid input leaves the pool intact.
    pub fn apply_template(&mut self, options: &MapOptions) -> Result<(), ArtifactPoolError> {
        if self.catalog.ruleset() != Ruleset::HotA181 {
            return Err(ArtifactPoolError::UnsupportedRuleset(
                self.catalog.ruleset(),
            ));
        }
        let mut overrides = self.overrides.clone();
        let mut combinations = self.combination_overrides.clone();
        if let Some(text) = &options.artifacts {
            apply_availability(text, &mut overrides)
                .map_err(|()| ArtifactPoolError::IdOverflow("artifacts"))?;
        }
        if let Some(text) = &options.combination_artifacts {
            apply_availability(text, &mut combinations)
                .map_err(|()| ArtifactPoolError::IdOverflow("combination artifacts"))?;
        }
        if options.artifacts.is_some() {
            let mut any_allowed = false;
            for ((row, ban), rule) in self
                .catalog
                .entries()
                .iter()
                .zip(&mut self.excluded)
                .zip(&mut overrides)
            {
                if !row.disabled()
                    && !matches!(row.class(), ArtifactClass::Special | ArtifactClass::Unused)
                {
                    apply_rule(*rule, ban);
                    any_allowed |= !*ban;
                } else {
                    *rule = Availability::Inherit;
                }
            }
            if !any_allowed {
                overrides[8] = Availability::Inherit;
            }
        }
        if options.combination_artifacts.is_some() {
            for (&rule, ban) in combinations.iter().zip(&mut self.combinations) {
                apply_rule(rule, ban);
            }
        }
        self.overrides = overrides;
        self.combination_overrides = combinations;
        Ok(())
    }
    /// Shared immutable traits, without copying the catalog into each run.
    #[must_use]
    pub const fn catalog(&self) -> &'a ArtifactCatalog {
        self.catalog
    }
    /// Effective exclusion bitmap, including successful quest claims.
    #[must_use]
    pub fn exclusions(&self) -> &[bool] {
        &self.excluded
    }
    /// Whether a combination is forbidden; its components may then serve quests.
    #[must_use]
    pub fn combination_bans(&self) -> &[bool] {
        &self.combinations
    }
    /// Normalized template overrides, retained separately from effective bans.
    #[must_use]
    pub fn overrides(&self) -> &[Availability] {
        &self.overrides
    }
    /// Normalized combination overrides.
    #[must_use]
    pub fn combination_overrides(&self) -> &[Availability] {
        &self.combination_overrides
    }
    /// Whether this identity is excluded; foreign IDs are unavailable.
    #[must_use]
    pub fn is_excluded(&self, artifact: ArtifactId) -> bool {
        self.excluded.get(artifact.index()).copied().unwrap_or(true)
    }
    /// Exclude an artifact after successful quest placement. Returns whether
    /// this changes the map's pool; duplicate or foreign claims have no effect.
    pub fn exclude(&mut self, artifact: ArtifactId) -> bool {
        let Some(ban) = self.excluded.get_mut(artifact.index()) else {
            return false;
        };
        !std::mem::replace(ban, true)
    }
    fn candidate(&self, row: ArtifactTraits) -> bool {
        self.catalog.ruleset() == Ruleset::Complete
            || (row.combination().is_none()
                && row
                    .component_of()
                    .is_none_or(|combo| self.combinations[combo.index()]))
    }
    /// Scan in native ascending order and choose without claiming. Empty pools
    /// consume no RNG; a singleton still draws in both Complete and `HotA`.
    #[must_use]
    pub fn select_quest(&self, rng: &mut RetailRng) -> QuestArtifactSelection {
        let eligible = || {
            self.catalog
                .entries()
                .iter()
                .enumerate()
                .filter(|&(id, row)| !self.excluded[id] && row.quest_eligible())
        };
        let pool = eligible().count();
        let candidates = eligible().filter(|&(_, row)| self.candidate(*row)).count();
        let artifact = if candidates == 0 {
            None
        } else {
            let rank = rng.draw() as usize % candidates;
            eligible()
                .filter(|&(_, row)| self.candidate(*row))
                .nth(rank)
                .and_then(|(id, _)| i32::try_from(id).ok())
                .and_then(|id| self.catalog.id(id))
        };
        QuestArtifactSelection {
            artifact,
            pool,
            candidates,
        }
    }
}
fn apply_rule(rule: Availability, ban: &mut bool) {
    match rule {
        Availability::Inherit => {}
        Availability::Enabled => *ban = false,
        Availability::Disabled => *ban = true,
    }
}
