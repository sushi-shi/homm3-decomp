//! Hero metadata from canonical declarations and installed expansion records.

use super::HeroId;
use crate::{parse, raw, request::MapVersion, rules::Ruleset};
use homm3_resource::hdat::Container;
use std::{error::Error, fmt};

/// Catalog-bounded class identity.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct HeroClassId(u8);
impl HeroClassId {
    /// Native class-table ordinal.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0 as usize
    }
}

/// Hero fields used by generation, excluding native strings and pointers.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct HeroTraits {
    class: HeroClassId,
    original: bool,
    expansion: bool,
    special: bool,
}
impl HeroTraits {
    /// Class whose final two available heroes are reserved by `HotA` prisons.
    #[must_use]
    pub const fn class(self) -> HeroClassId {
        self.class
    }
    /// Default eligibility before water choices and template overrides.
    #[must_use]
    pub const fn available(self, version: MapVersion) -> bool {
        !self.special
            && match version {
                MapVersion::Restoration => self.original,
                MapVersion::ArmageddonsBlade | MapVersion::ShadowOfDeath => self.expansion,
            }
    }
}

/// An installed hero record cannot enter the catalog.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum HeroDataError {
    /// Required expanded hero has no record.
    Missing(usize),
    /// The native loader needs a 92-byte binary payload.
    Payload(usize),
    /// Hero or class ID is outside the pinned domain.
    Field {
        /// HDAT record byte offset.
        record: usize,
        /// Rejected field name.
        field: &'static str,
        /// Source value.
        value: i32,
    },
}
impl fmt::Display for HeroDataError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "hero data: {self:?}")
    }
}
impl Error for HeroDataError {}

/// Immutable, versioned hero metadata shared by generation runs.
#[derive(Clone, Debug)]
pub struct HeroCatalog {
    entries: Box<[HeroTraits]>,
    rules: Ruleset,
}
impl HeroCatalog {
    /// Canonical Complete data, excluding portrait-only table rows.
    #[must_use]
    #[expect(
        clippy::missing_panics_doc,
        reason = "canonical C++ hero classes fit the declared class domain"
    )]
    pub fn complete() -> Self {
        let entries = (0..raw::RMG_HERO_COUNT as usize)
            .map(|id| {
                let (original, expansion, special) = raw::HERO_AVAILABILITY[id];
                let class = u8::try_from(raw::HERO_CLASSES[id]).expect("canonical hero class");
                assert!(usize::from(class) < Ruleset::Complete.hero_class_count());
                HeroTraits {
                    class: HeroClassId(class),
                    original,
                    expansion,
                    special,
                }
            })
            .collect();
        Self {
            entries,
            rules: Ruleset::Complete,
        }
    }
    /// Apply installed `heroNNN` records and the pinned final availability patches.
    /// Native DLL RVA 0x16aafa copies 92 bytes per record. Class is at +8 and
    /// availability bytes at +0x38..+0x3a. No native pointer is dereferenced.
    ///
    /// # Errors
    /// Rejects missing expanded rows, short payloads and out-of-domain IDs.
    pub fn parse_hota181(data: Container<'_>) -> Result<Self, HeroDataError> {
        let rules = Ruleset::HotA181;
        let mut entries: Vec<_> = Self::complete().entries.iter().copied().map(Some).collect();
        entries.resize(rules.hero_count(), None);
        for record in data.entries() {
            let name = record.name();
            if name.len() != 7 || !name[..4].eq_ignore_ascii_case(b"hero") {
                continue;
            }
            let fault = |field, value| HeroDataError::Field {
                record: record.offset(),
                field,
                value,
            };
            let id = parse::integer(name[4..].iter().copied()).unwrap_or(0);
            let entry = usize::try_from(id)
                .ok()
                .and_then(|id| entries.get_mut(id))
                .ok_or_else(|| fault("hero ID", id))?;
            let bytes = record
                .payload()
                .filter(|b| b.len() >= 92)
                .ok_or(HeroDataError::Payload(record.offset()))?;
            let class = i32::from_le_bytes([bytes[8], bytes[9], bytes[10], bytes[11]]);
            let class = u8::try_from(class)
                .ok()
                .filter(|&id| usize::from(id) < rules.hero_class_count())
                .ok_or_else(|| fault("hero class", class))?;
            *entry = Some(HeroTraits {
                class: HeroClassId(class),
                original: bytes[0x38] != 0,
                expansion: bytes[0x39] != 0,
                special: bytes[0x3a] != 0,
            });
        }
        let mut entries = entries
            .into_iter()
            .enumerate()
            .map(|(id, row)| row.ok_or(HeroDataError::Missing(id)))
            .collect::<Result<Vec<_>, _>>()?;
        // Native final writes, DLL RVA 0x16b536..0x16b568.
        entries[4].expansion = true;
        for hero in [20, 71, 144] {
            entries[hero].special = true;
        }
        Ok(Self {
            entries: entries.into_boxed_slice(),
            rules,
        })
    }
    /// Catalog's native generation rules.
    #[must_use]
    pub const fn ruleset(&self) -> Ruleset {
        self.rules
    }
    /// Native row order, including heroes disabled by default.
    #[must_use]
    pub fn entries(&self) -> &[HeroTraits] {
        &self.entries
    }
    /// Admit an ID against this catalog.
    #[must_use]
    pub fn id(&self, value: i32) -> Option<HeroId> {
        let id = u8::try_from(value).ok()?;
        (usize::from(id) < self.entries.len()).then_some(HeroId(id))
    }
    /// Checked lookup, including IDs obtained from another ruleset.
    #[must_use]
    pub fn get(&self, hero: HeroId) -> Option<&HeroTraits> {
        self.entries.get(hero.index())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn complete_catalog_keeps_source_availability() {
        let catalog = HeroCatalog::complete();
        for (id, row) in catalog.entries().iter().enumerate() {
            for version in [
                MapVersion::Restoration,
                MapVersion::ArmageddonsBlade,
                MapVersion::ShadowOfDeath,
            ] {
                assert_eq!(
                    row.available(version),
                    super::super::source_available(id, version)
                );
            }
        }
        assert!(catalog.id(156).is_none());
        assert!(catalog.id(-1).is_none());
    }

    fn payload(class: i32, flags: [u8; 3]) -> Vec<u8> {
        let mut bytes = vec![0xff; 92]; // Native pointers deliberately unusable.
        bytes[8..12].copy_from_slice(&class.to_le_bytes());
        bytes[0x38..0x3b].copy_from_slice(&flags);
        bytes
    }

    fn records(extra: &[(usize, Vec<u8>)], omit: Option<usize>) -> Vec<u8> {
        fn word(bytes: &mut Vec<u8>, value: usize) {
            bytes.extend(i32::try_from(value).unwrap().to_le_bytes());
        }
        fn string(bytes: &mut Vec<u8>, value: &[u8]) {
            word(bytes, value.len());
            bytes.extend(value);
        }
        let entries: Vec<_> = (156..215)
            .filter(|id| Some(*id) != omit)
            .map(|id| {
                (
                    id,
                    payload(18 + i32::try_from((id - 156) / 10).unwrap(), [1, 1, 0]),
                )
            })
            .chain(extra.iter().cloned())
            .collect();
        let mut bytes = b"HDAT\x02\0\0\0".to_vec();
        word(&mut bytes, entries.len());
        for (id, payload) in entries {
            string(&mut bytes, format!("hero{id:03}").as_bytes());
            string(&mut bytes, b"");
            word(&mut bytes, 0);
            bytes.push(1);
            string(&mut bytes, &payload);
            word(&mut bytes, 0);
        }
        bytes
    }

    fn expanded_catalog() -> HeroCatalog {
        let bytes = records(&[], None);
        HeroCatalog::parse_hota181(Container::parse(&bytes).unwrap()).unwrap()
    }

    #[test]
    fn records_replace_rows_before_final_availability_patches() {
        let bytes = records(
            &[
                (4, payload(0, [0, 0, 0])),
                (20, payload(2, [1, 1, 0])),
                (71, payload(8, [1, 1, 0])),
                (144, payload(0, [1, 1, 0])),
                (214, payload(23, [0, 1, 0])),
            ],
            None,
        );
        let catalog = HeroCatalog::parse_hota181(Container::parse(&bytes).unwrap()).unwrap();
        assert_eq!(catalog.ruleset(), Ruleset::HotA181);
        assert_eq!(catalog.entries().len(), 215);
        assert!(catalog.entries()[4].available(MapVersion::ShadowOfDeath));
        assert!(!catalog.entries()[4].available(MapVersion::Restoration));
        for id in [20, 71, 144] {
            assert!(!catalog.entries()[id].available(MapVersion::ShadowOfDeath));
        }
        let last = catalog.id(214).unwrap();
        assert_eq!(catalog.get(last).unwrap().class().index(), 23);
        assert!(catalog
            .get(last)
            .unwrap()
            .available(MapVersion::ShadowOfDeath));
        assert!(!catalog
            .get(last)
            .unwrap()
            .available(MapVersion::Restoration));
        assert!(!last.available(MapVersion::ShadowOfDeath));
        assert!(HeroCatalog::complete().get(last).is_none());
        assert!(catalog.id(215).is_none());
    }

    #[test]
    fn incomplete_payloads_and_invalid_domains_cannot_enter_catalog() {
        let bytes = records(&[], Some(200));
        assert!(matches!(
            HeroCatalog::parse_hota181(Container::parse(&bytes).unwrap()),
            Err(HeroDataError::Missing(200))
        ));
        for (id, payload, field) in [
            (214, vec![0; 91], None),
            (215, payload(23, [0, 1, 0]), Some("hero ID")),
            (214, payload(24, [0, 1, 0]), Some("hero class")),
            (214, payload(-1, [0, 1, 0]), Some("hero class")),
        ] {
            let bytes = records(&[(id, payload)], None);
            let err = HeroCatalog::parse_hota181(Container::parse(&bytes).unwrap()).unwrap_err();
            match (err, field) {
                (HeroDataError::Payload(_), None) => {}
                (HeroDataError::Field { field: actual, .. }, Some(expected)) => {
                    assert_eq!(actual, expected)
                }
                other => panic!("unexpected error: {other:?}"),
            }
        }
    }

    #[test]
    fn overrides_follow_water_substitutions_and_starting_claims_preserve_counts() {
        use crate::{hero::HeroPool, request::Water, template::Availability};
        let catalog = expanded_catalog();
        for water in [Water::None, Water::Normal, Water::Islands] {
            let pool =
                HeroPool::prepare(&catalog, MapVersion::ShadowOfDeath, water, &[], 8, &[]).unwrap();
            for id in [3, 122, 174, 214] {
                assert_eq!(
                    pool.is_disabled(catalog.id(id).unwrap()),
                    water == Water::None
                );
            }
            for id in [175, 176, 159, 210] {
                assert_eq!(
                    pool.is_disabled(catalog.id(id).unwrap()),
                    water != Water::None
                );
            }
            let enabled = (0..215)
                .filter(|&id| !pool.is_disabled(catalog.id(id).unwrap()))
                .count();
            assert_eq!(pool.prison_limit(), Some(enabled.saturating_sub(128)));
        }
        let mut rules = vec![Availability::Inherit; 215];
        rules[214] = Availability::Enabled;
        rules[175] = Availability::Disabled;
        let starting = catalog.id(214).unwrap();
        let mut pool = HeroPool::prepare(
            &catalog,
            MapVersion::ShadowOfDeath,
            Water::None,
            &rules,
            2,
            &[starting],
        )
        .unwrap();
        assert!(!pool.is_disabled(starting));
        assert!(pool.is_disabled(catalog.id(175).unwrap()));
        let initial = pool.remaining_by_class().to_vec();
        let mut rng = crate::rng::RetailRng::new(42);
        while let Some(hero) = pool.select_prison(&mut rng) {
            assert_ne!(hero, starting);
        }
        assert_eq!(
            pool.remaining_by_class(),
            initial.iter().map(|&n| n.min(2)).collect::<Vec<_>>()
        );
    }

    #[test]
    fn prison_singleton_skips_draw_and_release_restores_class_reservation() {
        use crate::{hero::HeroPool, request::Water, template::Availability};
        let catalog = expanded_catalog();
        let mut rules = vec![Availability::Disabled; 215];
        for id in [211, 212, 213] {
            rules[id] = Availability::Enabled;
        }
        let held = [catalog.id(211).unwrap(), catalog.id(212).unwrap()];
        let make_pool = || {
            HeroPool::prepare(
                &catalog,
                MapVersion::ShadowOfDeath,
                Water::None,
                &rules,
                8,
                &held,
            )
            .unwrap()
        };
        let mut pool = make_pool();
        assert_eq!(pool.prison_limit(), Some(0));
        assert_eq!(pool.remaining_by_class()[23], 3);
        let mut rng = crate::rng::RetailRng::new(42);
        let checkpoint = rng.checkpoint();
        let hero = pool.select_prison(&mut rng).unwrap();
        assert_eq!(hero.index(), 213);
        assert_eq!(pool.remaining_by_class()[23], 2);
        assert_eq!(rng.checkpoint(), checkpoint);
        assert!(pool.select_prison(&mut rng).is_none());
        pool.release_prison(hero);
        assert_eq!(pool.remaining_by_class()[23], 3);
        assert_eq!(pool.select_prison(&mut rng), Some(hero));
        assert_eq!(make_pool().select_prison(&mut rng), Some(hero));
        assert_eq!(rng.checkpoint(), checkpoint);
    }

    #[test]
    fn prison_order_is_ascending_and_class_reserves_stop_selection() {
        use crate::{hero::HeroPool, request::Water, template::Availability};
        let catalog = expanded_catalog();
        let mut rules = vec![Availability::Disabled; 215];
        for id in [211, 212, 213, 214] {
            rules[id] = Availability::Enabled;
        }
        let mut pool = HeroPool::prepare(
            &catalog,
            MapVersion::ShadowOfDeath,
            Water::None,
            &rules,
            0,
            &[],
        )
        .unwrap();
        let mut rng = crate::rng::RetailRng::new(1);
        // CRT draws 41 then 18467: ranks 1/4 then 2/3 in ascending order.
        assert_eq!(pool.select_prison(&mut rng), catalog.id(212));
        assert_eq!(pool.select_prison(&mut rng), catalog.id(214));
        assert_eq!(rng.draws(), 2);
        assert_eq!(pool.remaining_by_class()[23], 2);
        assert!(pool.select_prison(&mut rng).is_none());
        assert_eq!(rng.draws(), 2);
    }
}
