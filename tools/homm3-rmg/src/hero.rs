//! Source-owned hero availability and per-generation prison selection.

use crate::{raw, request::MapVersion, rng::RetailRng};
use std::num::NonZeroU32;

/// A playable Complete-era hero ID, excluding portrait-only table rows.
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
    /// Every generator hero in table order.
    pub fn all() -> impl Iterator<Item = Self> {
        const { assert!(raw::RMG_HERO_COUNT <= 1 << u8::BITS) };
        (0..=u8::MAX).map(Self).take(raw::RMG_HERO_COUNT as usize)
    }
    /// Availability before generated heroes are removed from the pool.
    #[must_use]
    pub const fn available(self, version: MapVersion) -> bool {
        source_available(self.index(), version)
    }
}

/// Map-owned hero exclusions. No process-global state or allocated candidates.
#[derive(Clone, Debug)]
pub struct HeroPool {
    disabled: [bool; raw::RMG_HERO_COUNT as usize],
    version: MapVersion,
}
impl HeroPool {
    /// Initialize the same availability flags as the native RMG constructor.
    #[must_use]
    pub fn new(version: MapVersion) -> Self {
        Self {
            disabled: std::array::from_fn(|index| !source_available(index, version)),
            version,
        }
    }
    /// Whether this hero is excluded or already used in the map.
    #[must_use]
    pub const fn is_disabled(&self, hero: HeroId) -> bool {
        self.disabled[hero.index()]
    }
    /// Remove a hero when another generated object claims it.
    pub fn disable(&mut self, hero: HeroId) {
        self.disabled[hero.index()] = true;
    }
    pub(crate) fn release_prison(&mut self, hero: HeroId) {
        self.disabled[hero.index()] = false;
    }
    /// Select and claim a prison hero in native descending-index order.
    /// An empty pool consumes no random draw; a singleton still consumes one.
    pub fn select_prison(&mut self, rng: &mut RetailRng) -> Option<HeroId> {
        let limit = match self.version {
            MapVersion::Restoration => raw::RMG_ROE_HERO_COUNT,
            MapVersion::ArmageddonsBlade | MapVersion::ShadowOfDeath => raw::RMG_PRISON_HERO_COUNT,
        } as usize;
        let eligible = || {
            self.disabled[..limit]
                .iter()
                .enumerate()
                .rev()
                .filter(|(_, disabled)| !**disabled)
        };
        let count = NonZeroU32::new(u32::try_from(eligible().count()).ok()?)?;
        let rank = rng.below(count) as usize;
        let (index, _) = eligible().nth(rank)?;
        let hero = HeroId(u8::try_from(index).ok()?);
        self.disable(hero);
        Some(hero)
    }
}

const fn source_available(index: usize, version: MapVersion) -> bool {
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
        for value in 1..const { crate::constants::signed(raw::RMG_HERO_COUNT) } {
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
