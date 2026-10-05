//! Versioned creature record loading; only RMG numeric fields are retained.

use super::{CreatureCatalog, CreatureTier, CreatureTraits, TraitError};
use crate::{parse, request::Town, rules::Ruleset};
use homm3_resource::hdat::Container;
use std::{error::Error, fmt, num::NonZeroI32};

/// A creature catalog cannot be initialized from the supplied resources.
#[derive(Debug)]
pub enum CreatureDataError {
    /// The base creature spreadsheet failed to load.
    Base(TraitError),
    /// An extended record is missing or malformed.
    Record(CreatureDataFault),
}
/// Location or identity of an invalid extended creature record.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CreatureDataFault {
    /// An expanded slot has no source data.
    Missing(usize),
    /// The native loader would copy beyond the available binary payload.
    Payload {
        /// HDAT record byte offset.
        record: usize,
    },
    /// A source field cannot enter the versioned domain.
    Field {
        /// HDAT record byte offset.
        record: usize,
        /// Field being admitted.
        field: &'static str,
        /// Native signed value.
        value: i32,
    },
}
impl fmt::Display for CreatureDataError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Base(error) => error.fmt(f),
            Self::Record(fault) => write!(f, "creature data: {fault:?}"),
        }
    }
}
impl Error for CreatureDataError {}

impl CreatureCatalog {
    /// Load the base spreadsheet and the pinned `HotA` 1.8.1 creature records.
    /// The caller must supply the spreadsheet resolved with that installation's
    /// resource precedence. The DLL copies 116 bytes from each `monstNNN`
    /// payload (RVA 0x16a39f); string pointer fields are deliberately not read.
    /// Later records replace earlier rows in source order. Final tier/faction
    /// patches follow RVA 0x16b5df..0x16b633.
    ///
    /// # Errors
    /// Rejects malformed base data, missing expanded rows, short payloads and
    /// unknown creature, faction or tier IDs. Nothing is committed on failure.
    #[expect(
        clippy::missing_panics_doc,
        reason = "Factory ID 10 belongs to the pinned twelve-town domain"
    )]
    pub fn parse_hota181(base: &[u8], data: Container<'_>) -> Result<Self, CreatureDataError> {
        let base = Self::parse(base).map_err(CreatureDataError::Base)?;
        let rules = Ruleset::HotA181;
        let mut entries: Vec<_> = base.entries.iter().copied().map(Some).collect();
        entries.resize(rules.creature_count(), None);
        for record in data.entries() {
            let name = record.name();
            if name.len() != 8 || !name[..5].eq_ignore_ascii_case(b"monst") {
                continue;
            }
            let fault = |field, value| {
                CreatureDataError::Record(CreatureDataFault::Field {
                    record: record.offset(),
                    field,
                    value,
                })
            };
            // The native caller takes atoi of the final three name bytes.
            let index = parse::integer(name[5..].iter().copied()).unwrap_or(0);
            let entry = usize::try_from(index)
                .ok()
                .and_then(|id| entries.get_mut(id))
                .ok_or_else(|| fault("creature ID", index))?;
            let bytes =
                record
                    .payload()
                    .filter(|b| b.len() >= 116)
                    .ok_or(CreatureDataError::Record(CreatureDataFault::Payload {
                        record: record.offset(),
                    }))?;
            let word = |offset| {
                i32::from_le_bytes([
                    bytes[offset],
                    bytes[offset + 1],
                    bytes[offset + 2],
                    bytes[offset + 3],
                ])
            };
            let faction = word(0);
            let town = if faction == -1 {
                None
            } else {
                Some(Town::parse_for(faction, rules).map_err(|_| fault("town", faction))?)
            };
            let level = word(4);
            let tier = match level {
                -1 => None,
                0..=6 => Some(CreatureTier(
                    u8::try_from(level).map_err(|_| fault("tier", level))?,
                )),
                _ => return Err(fault("tier", level)),
            };
            *entry = Some(CreatureTraits {
                town,
                tier,
                ai_value: NonZeroI32::new(word(0x40)),
                growth: word(0x44),
                wandering_low: word(0x6c),
                wandering_high: word(0x70),
            });
        }
        let mut entries = entries
            .into_iter()
            .enumerate()
            .map(|(id, entry)| {
                entry.ok_or(CreatureDataError::Record(CreatureDataFault::Missing(id)))
            })
            .collect::<Result<Vec<_>, _>>()?;
        // These writes occur after all HDAT records, including replacements for
        // old creatures. They are not metadata inferred from creature names.
        for (id, tier) in [(123, 2), (125, 4), (129, 3)] {
            entries[id].tier = Some(CreatureTier(tier));
        }
        entries[138].town = Some(Town::parse_for(10, rules).expect("pinned Factory ID"));
        Ok(Self {
            entries: entries.into_boxed_slice(),
            rules,
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn base() -> Vec<u8> {
        (0..185)
            .map(|row| {
                let mut fields = [""; 24];
                if row == 2 {
                    fields[10] = "602";
                }
                format!("{}\r\n", fields.join("\t"))
            })
            .collect::<String>()
            .into_bytes()
    }

    fn blob(town: i32, level: i32) -> Vec<u8> {
        let mut data = vec![0xff; 116]; // Deliberately invalid native pointers.
        for (offset, value) in [
            (0, town),
            (4, level),
            (0x40, 602),
            (0x44, 7),
            (0x6c, 12),
            (0x70, 20),
        ] {
            data[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
        }
        data
    }

    fn records(extra: &[(usize, Vec<u8>)], omit: Option<usize>) -> Vec<u8> {
        fn word(bytes: &mut Vec<u8>, value: usize) {
            bytes.extend(i32::try_from(value).unwrap().to_le_bytes());
        }
        fn string(bytes: &mut Vec<u8>, value: &[u8]) {
            word(bytes, value.len());
            bytes.extend(value);
        }
        let entries: Vec<_> = (150..200)
            .filter(|id| Some(*id) != omit)
            .map(|id| (id, blob(9, 2)))
            .chain(extra.iter().cloned())
            .collect();
        let mut bytes = b"HDAT\x02\0\0\0".to_vec();
        word(&mut bytes, entries.len());
        for (id, payload) in entries {
            string(&mut bytes, format!("monst{id:03}").as_bytes());
            string(&mut bytes, b"");
            word(&mut bytes, 0);
            bytes.push(1);
            string(&mut bytes, &payload);
            word(&mut bytes, 0);
        }
        bytes
    }

    #[test]
    fn expanded_rows_and_final_patches_share_checked_catalog_lookups() {
        let mut replacement = blob(11, 6);
        replacement[0x40..0x44].copy_from_slice(&0i32.to_le_bytes());
        let data = records(
            &[(199, replacement), (123, blob(1, 6)), (138, blob(1, 0))],
            None,
        );
        let source = base();
        let catalog =
            CreatureCatalog::parse_hota181(&source, Container::parse(&data).unwrap()).unwrap();
        assert_eq!(catalog.ruleset(), Ruleset::HotA181);
        assert_eq!(catalog.entries().len(), 200);
        let sea_dog = catalog.get(catalog.id(151).unwrap()).unwrap();
        assert_eq!(sea_dog.town().unwrap().index(), 9);
        assert_eq!(sea_dog.tier().unwrap().index(), 2);
        assert_eq!(sea_dog.ai_value().unwrap().get(), 602);
        assert_eq!(sea_dog.growth(), 7);
        assert_eq!(sea_dog.wandering_counts(), (12, 20));
        let last = catalog.get(catalog.id(199).unwrap()).unwrap();
        assert_eq!(last.town().unwrap().index(), 11);
        assert_eq!(last.ai_value(), None);
        assert_eq!(catalog.entries()[123].tier().unwrap().index(), 2);
        assert_eq!(catalog.entries()[125].tier().unwrap().index(), 4);
        assert_eq!(catalog.entries()[129].tier().unwrap().index(), 3);
        assert_eq!(catalog.entries()[138].town().unwrap().index(), 10);
        assert!(catalog.id(-1).is_none());
        assert!(catalog.id(200).is_none());
        let complete = CreatureCatalog::parse(&source).unwrap();
        assert!(complete.id(145).is_none());
        assert!(complete.get(catalog.id(151).unwrap()).is_none());
    }

    #[test]
    fn missing_short_and_out_of_domain_records_fail_before_catalog_admission() {
        let source = base();
        let data = records(&[], Some(184));
        assert!(matches!(
            CreatureCatalog::parse_hota181(&source, Container::parse(&data).unwrap()),
            Err(CreatureDataError::Record(CreatureDataFault::Missing(184)))
        ));
        for (id, payload, field) in [
            (199, vec![0; 115], None),
            (200, blob(9, 2), Some("creature ID")),
            (151, blob(12, 2), Some("town")),
            (151, blob(-2, 2), Some("town")),
            (151, blob(9, 7), Some("tier")),
        ] {
            let data = records(&[(id, payload)], None);
            let Err(CreatureDataError::Record(fault)) =
                CreatureCatalog::parse_hota181(&source, Container::parse(&data).unwrap())
            else {
                panic!("expected record error");
            };
            match (fault, field) {
                (CreatureDataFault::Payload { .. }, None) => {}
                (CreatureDataFault::Field { field: actual, .. }, Some(expected)) => {
                    assert_eq!(actual, expected)
                }
                other => panic!("wrong record error: {other:?}"),
            }
        }
    }

    #[test]
    fn guards_use_expanded_catalog_and_require_a_loaded_monster_prototype() {
        use crate::{
            behavior::{Behavior, RetailProfile},
            placement_rules::PlacementRules,
            prototype::{GuardFactions, ImageMask, PrototypeSource},
            request::MapVersion,
            rng::RetailRng,
        };
        let data = records(&[], None);
        let creatures =
            CreatureCatalog::parse_hota181(&base(), Container::parse(&data).unwrap()).unwrap();
        let behavior = Behavior::Retail(RetailProfile::default());
        let rules = PlacementRules::parse(b"header\r\nheader\r\nheader\r\n", behavior).unwrap();
        // All 50 expanded rows are eligible, but only creature 199 has art.
        // The second art record must win without adding a selection candidate.
        let row = |name: &str| {
            format!(
                "{name} {:048b} {:048b} 000000001 000000001 54 199 0 0",
                0u64,
                1u64 << 47
            )
        };
        let text = format!("2\r\n{}\r\n{}\r\n", row("first.def"), row("last.def"));
        let source = PrototypeSource::parse(text.as_bytes(), |_| {
            ImageMask::parse(&[1, 1, 0, 0, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0]).map(Some)
        })
        .unwrap();
        let prototypes = source
            .prepare(&rules, MapVersion::ShadowOfDeath, behavior)
            .unwrap();
        let mut rng = RetailRng::new(42);
        let guard = prototypes
            .select_guard(
                16 * 602,
                GuardFactions::Allowed(&[true; 13]),
                &creatures,
                &mut rng,
            )
            .unwrap()
            .unwrap();
        assert_eq!(guard.creature().index(), 199);
        assert_eq!(
            prototypes
                .get(guard.prototype())
                .unwrap()
                .prototype()
                .source_row(),
            1
        );
        assert_eq!(rng.draws(), 3); // Selection plus two variation draws.
        let end = rng.checkpoint();
        assert!(prototypes
            .select_guard(
                602,
                GuardFactions::Allowed(&[true; 13]),
                &creatures,
                &mut rng
            )
            .unwrap()
            .is_none());
        assert_eq!(rng.checkpoint(), end);
        // RoE clears slots 173..199 and evaluates 0..116, but the final scan
        // can still select unfiltered 151, ignoring its value and faction.
        let row = |id| {
            format!(
                "guard.def {:048b} {:048b} 000000001 000000001 54 {id} 0 0",
                0u64,
                1u64 << 47
            )
        };
        let text = format!("3\r\n{}\r\n{}\r\n{}\r\n", row(0), row(151), row(199));
        let source = PrototypeSource::parse(text.as_bytes(), |_| {
            ImageMask::parse(&[1, 1, 0, 0, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0]).map(Some)
        })
        .unwrap();
        let prototypes = source
            .prepare(&rules, MapVersion::Restoration, behavior)
            .unwrap();
        let mut rng = RetailRng::new(42);
        let guard = prototypes
            .select_guard(
                602,
                GuardFactions::Matching(Town::parse(0).unwrap()),
                &creatures,
                &mut rng,
            )
            .unwrap()
            .unwrap();
        assert_eq!(guard.creature().index(), 151);
        assert_eq!(guard.count(), 1);
        assert_eq!(rng.draws(), 1);
    }
}
