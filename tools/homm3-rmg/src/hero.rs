//! Source-owned hero availability and per-generation prison selection.

use crate::{
    raw,
    request::{MapVersion, Water},
    rng::RetailRng,
    rules::Ruleset,
    template::Availability,
};
use std::{error::Error, fmt, num::NonZeroU32};

mod catalog;
pub use catalog::{HeroCatalog, HeroClassId, HeroDataError, HeroTraits};

/// A catalog-admitted hero ID, excluding the no-hero sentinel.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct HeroId(u8);
impl HeroId {
    /// Parse a generator hero ID without admitting the no-hero sentinel.
    #[must_use]
    pub fn parse(value: i32) -> Option<Self> {
        let value = u8::try_from(value).ok()?;
        (u32::from(value) < raw::RMG_HERO_COUNT).then_some(Self(value))
    }
    /// Original table and serialized index.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0 as usize
    }
    /// Complete availability; use a versioned catalog for expansion heroes.
    #[must_use]
    pub const fn available(self, version: MapVersion) -> bool {
        source_available(self.index(), version)
    }
}

/// Invalid per-map hero configuration.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum HeroPoolError {
    /// Availability overrides must be empty or cover the complete catalog.
    OverrideCount(usize),
    /// Only eight player colors exist.
    PlayerCount(u8),
    /// A starting hero belongs outside this catalog.
    StartingHero(HeroId),
}
impl fmt::Display for HeroPoolError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "hero pool: {self:?}")
    }
}
impl Error for HeroPoolError {}

/// Map-owned hero exclusions, starting-hero claims and per-class availability.
/// No process-global state or allocated selection candidates.
#[derive(Clone, Debug)]
pub struct HeroPool {
    disabled: Box<[bool]>,
    taken: Box<[bool]>,
    classes: Box<[HeroClassId]>,
    remaining: Box<[usize]>,
    version: MapVersion,
    rules: Ruleset,
    prison_limit: Option<usize>,
}
impl HeroPool {
    /// Initialize Complete's native availability flags.
    #[must_use]
    #[expect(
        clippy::missing_panics_doc,
        reason = "canonical Complete setup has no overrides or starting hero IDs to reject"
    )]
    pub fn new(version: MapVersion) -> Self {
        Self::prepare(&HeroCatalog::complete(), version, Water::None, &[], 0, &[])
            .expect("canonical Complete hero pool")
    }
    /// Initialize versioned hero availability in native constructor order.
    /// `HotA` applies water substitutions, then pack overrides, then class counts
    /// and the initial prison cap. Starting-hero claims are separate from those
    /// counts, matching the later request hook.
    ///
    /// # Errors
    /// Rejects incomplete override vectors, excessive players and foreign IDs.
    pub fn prepare(
        catalog: &HeroCatalog,
        version: MapVersion,
        water: Water,
        overrides: &[Availability],
        players: u8,
        starting: &[HeroId],
    ) -> Result<Self, HeroPoolError> {
        let count = catalog.entries().len();
        if !overrides.is_empty() && overrides.len() != count {
            return Err(HeroPoolError::OverrideCount(overrides.len()));
        }
        if usize::from(players) > crate::request::PLAYER_COUNT {
            return Err(HeroPoolError::PlayerCount(players));
        }
        let rules = catalog.ruleset();
        let mut disabled: Vec<_> = catalog
            .entries()
            .iter()
            .map(|row| !row.available(version))
            .collect();
        if rules == Ruleset::HotA181 {
            // DLL RVA 0x1b9302: unconditional water/land swaps precede overrides.
            let water = water != Water::None;
            for hero in [3, 122, 174, 214] {
                disabled[hero] = !water;
            }
            for hero in [175, 176, 159, 210] {
                disabled[hero] = water;
            }
        }
        for (disabled, rule) in disabled.iter_mut().zip(overrides) {
            match rule {
                Availability::Inherit => {}
                Availability::Enabled => *disabled = false,
                Availability::Disabled => *disabled = true,
            }
        }
        let classes: Box<[_]> = catalog.entries().iter().map(|row| row.class()).collect();
        let mut remaining = vec![0; rules.hero_class_count()].into_boxed_slice();
        for (&class, &disabled) in classes.iter().zip(&disabled) {
            if !disabled {
                remaining[class.index()] += 1;
            }
        }
        // DLL RVA 0x1b9410: max(enabled - 48 - 10 * players, 0).
        let prison_limit = (rules == Ruleset::HotA181).then(|| {
            disabled
                .iter()
                .filter(|&&b| !b)
                .count()
                .saturating_sub(48 + 10 * usize::from(players))
        });
        let mut taken = vec![false; count].into_boxed_slice();
        for &hero in starting {
            *taken
                .get_mut(hero.index())
                .ok_or(HeroPoolError::StartingHero(hero))? = true;
        }
        Ok(Self {
            disabled: disabled.into_boxed_slice(),
            taken,
            classes,
            remaining,
            version,
            rules,
            prison_limit,
        })
    }
    /// Initial native map-wide prison cap, or Complete's ordinary object limit.
    #[must_use]
    pub const fn prison_limit(&self) -> Option<usize> {
        self.prison_limit
    }
    /// Available heroes per class; starting-hero claims do not reduce these counts.
    #[must_use]
    pub fn remaining_by_class(&self) -> &[usize] {
        &self.remaining
    }
    /// Whether excluded or used; an ID absent from this catalog is unavailable.
    #[must_use]
    pub fn is_disabled(&self, hero: HeroId) -> bool {
        self.disabled.get(hero.index()).copied().unwrap_or(true)
    }
    /// Claim a hero as used. Foreign IDs and duplicate claims have no effect.
    pub fn disable(&mut self, hero: HeroId) {
        if let Some(disabled) = self.disabled.get_mut(hero.index()) {
            if !*disabled {
                *disabled = true;
                self.remaining[self.classes[hero.index()].index()] -= 1;
            }
        }
    }
    pub(crate) fn release_prison(&mut self, hero: HeroId) {
        if let Some(disabled) = self.disabled.get_mut(hero.index()) {
            if *disabled {
                *disabled = false;
                self.remaining[self.classes[hero.index()].index()] += 1;
            }
        }
    }
    fn prison_eligible(&self, index: usize) -> bool {
        !self.disabled[index]
            && !self.taken[index]
            && (self.rules == Ruleset::Complete || self.remaining[self.classes[index].index()] > 2)
    }
    /// Claim a prison hero in native order. Complete scans down and always draws
    /// for a nonempty pool; `HotA` scans up and skips the singleton draw.
    pub fn select_prison(&mut self, rng: &mut RetailRng) -> Option<HeroId> {
        let limit = match self.rules {
            Ruleset::HotA181 => self.disabled.len(),
            Ruleset::Complete => match self.version {
                MapVersion::Restoration => raw::RMG_ROE_HERO_COUNT as usize,
                MapVersion::ArmageddonsBlade | MapVersion::ShadowOfDeath => {
                    raw::RMG_PRISON_HERO_COUNT as usize
                }
            },
        };
        let count = NonZeroU32::new(
            u32::try_from((0..limit).filter(|&id| self.prison_eligible(id)).count()).ok()?,
        )?;
        let rank = if self.rules == Ruleset::HotA181 && count.get() == 1 {
            0
        } else {
            rng.below(count) as usize
        };
        let index = match self.rules {
            Ruleset::Complete => (0..limit)
                .rev()
                .filter(|&id| self.prison_eligible(id))
                .nth(rank),
            Ruleset::HotA181 => (0..limit).filter(|&id| self.prison_eligible(id)).nth(rank),
        }?;
        let hero = HeroId(u8::try_from(index).ok()?);
        self.disable(hero);
        Some(hero)
    }
}

const fn source_available(index: usize, version: MapVersion) -> bool {
    if index >= raw::RMG_HERO_COUNT as usize {
        return false;
    }
    let (original, expansion, special) = raw::HERO_AVAILABILITY[index];
    !special
        && match version {
            MapVersion::Restoration => original,
            MapVersion::ArmageddonsBlade | MapVersion::ShadowOfDeath => expansion,
        }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn singleton_draws_once_then_empty_pool_keeps_the_rng() {
        let mut pool = HeroPool::new(MapVersion::ShadowOfDeath);
        for value in 1..i32::try_from(raw::RMG_HERO_COUNT).unwrap() {
            pool.disable(HeroId::parse(value).unwrap());
        }
        let mut rng = RetailRng::new(1);
        assert_eq!(pool.select_prison(&mut rng), HeroId::parse(0));
        assert_eq!(rng.draws(), 1);
        let end = rng.checkpoint();
        assert!(pool.select_prison(&mut rng).is_none());
        assert_eq!(rng.checkpoint(), end);
        assert!(HeroId::parse(-1).is_none());
        assert!(HeroId::parse(156).is_none());
    }

    #[test]
    fn prison_exhaustion_claims_each_eligible_hero_once() {
        for version in [
            MapVersion::Restoration,
            MapVersion::ArmageddonsBlade,
            MapVersion::ShadowOfDeath,
        ] {
            let mut pool = HeroPool::new(version);
            let mut rng = RetailRng::new(42);
            let mut seen = [false; raw::RMG_HERO_COUNT as usize];
            while let Some(hero) = pool.select_prison(&mut rng) {
                assert!(hero.available(version));
                assert!(!seen[hero.index()]);
                assert!(pool.is_disabled(hero));
                seen[hero.index()] = true;
            }
            assert_eq!(
                rng.draws(),
                u64::try_from(seen.iter().filter(|&&used| used).count()).unwrap()
            );
            let limit = if version == MapVersion::Restoration {
                raw::RMG_ROE_HERO_COUNT
            } else {
                raw::RMG_PRISON_HERO_COUNT
            };
            for value in 0..i32::try_from(limit).unwrap() {
                let hero = HeroId::parse(value).unwrap();
                assert_eq!(seen[hero.index()], hero.available(version));
            }
        }
    }
}
