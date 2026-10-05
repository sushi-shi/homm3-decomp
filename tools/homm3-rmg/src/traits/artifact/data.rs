//! Installed artifact numeric streams and combination membership.

use super::{
    raw, ArtifactCatalog, ArtifactClass, ArtifactTraits, CombinationId, Ruleset, TraitError,
};
use crate::parse;
use homm3_resource::hdat::{Container, Entry};
use std::{error::Error, fmt};

/// Installed artifact data cannot enter the pinned catalog.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum ArtifactDataError {
    /// Invalid base spreadsheet.
    Traits(TraitError),
    /// The selected `artsinfo0` record is absent.
    MissingInfo,
    /// Missing or invalid field, with its HDAT record offset.
    Field {
        /// Containing record's byte offset.
        record: usize,
        /// Field being admitted.
        field: &'static str,
        /// Rejected value, or `None` when absent.
        value: Option<i32>,
    },
    /// Numeric prefix overflows signed 32-bit arithmetic.
    Integer {
        /// Containing record's byte offset.
        record: usize,
        /// Offset in its selected string field.
        token: usize,
    },
    /// An expanded recipe has no assembled artifact after all stream writes.
    MissingCombination(usize),
}
impl fmt::Display for ArtifactDataError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "artifact data: {self:?}")
    }
}
impl Error for ArtifactDataError {}

fn fault(record: Entry<'_>, field: &'static str, value: Option<i32>) -> ArtifactDataError {
    ArtifactDataError::Field {
        record: record.offset(),
        field,
        value,
    }
}
fn stream(record: Entry<'_>) -> Result<Vec<i32>, ArtifactDataError> {
    let strings = record.strings();
    let field = strings
        .clone()
        .nth(strings.len().saturating_sub(1).min(8))
        .ok_or_else(|| fault(record, "numeric string", None))?;
    parse::integer_stream(field).map_err(|token| ArtifactDataError::Integer {
        record: record.offset(),
        token,
    })
}
fn artifact_id(record: Entry<'_>, value: i32) -> Result<usize, ArtifactDataError> {
    usize::try_from(value)
        .ok()
        .filter(|&id| id < Ruleset::HotA181.artifact_count())
        .ok_or_else(|| fault(record, "artifact ID", Some(value)))
}

fn apply_records(
    entries: &mut [ArtifactTraits],
    data: Container<'_>,
) -> Result<(), ArtifactDataError> {
    for record in data.entries() {
        let name = record.name();
        if name.len() != 6 || !name[..3].eq_ignore_ascii_case(b"art") {
            continue;
        }
        let id = artifact_id(
            record,
            parse::integer(name[3..].iter().copied()).unwrap_or(0),
        )?;
        if matches!(id, 144 | 145) {
            continue;
        }
        let values = stream(record)?;
        if let Some(&class) = values.get(2) {
            entries[id].class = match class {
                0 => ArtifactClass::Unused,
                1 => ArtifactClass::Special,
                2 => ArtifactClass::Treasure,
                4 => ArtifactClass::Minor,
                8 => ArtifactClass::Major,
                16 => ArtifactClass::Relic,
                _ => return Err(fault(record, "artifact class", Some(class))),
            };
        }
        if let Some(&disabled) = values.get(3) {
            entries[id].disabled = disabled != 0;
        }
    }
    Ok(())
}

impl ArtifactCatalog {
    /// Load `artNNN` numeric streams and `artsinfo0` combinations over the base
    /// spreadsheet. The DLL initializes slots 144 onward before applying records
    /// (RVA 0x108220), ignores records 144/145, and then assigns recipe membership.
    /// Partial numeric records retain fields not yet overwritten. Combination
    /// components accumulate when a stream names the same recipe more than once.
    ///
    /// # Errors
    /// Rejects invalid base data, missing numeric strings, integer overflow and
    /// indices/classes outside the pinned 1.8.1 domains.
    pub fn parse_hota181(bytes: &[u8], data: Container<'_>) -> Result<Self, ArtifactDataError> {
        let mut catalog = Self::parse(bytes).map_err(ArtifactDataError::Traits)?;
        catalog.rules = Ruleset::HotA181;
        let mut entries = catalog.entries.into_vec();
        entries.resize(
            catalog.rules.artifact_count(),
            ArtifactTraits {
                class: ArtifactClass::Unused,
                disabled: true,
                combination: None,
                component_of: None,
            },
        );
        // Metadata is registered separately; the last matching artsinfo0 wins.
        let info = data
            .entries()
            .filter(|record| {
                let name = record.name();
                name.len() == 9
                    && name[..8].eq_ignore_ascii_case(b"artsinfo")
                    && parse::integer(name[8..].iter().copied()) == Some(0)
            })
            .last()
            .ok_or(ArtifactDataError::MissingInfo)?;
        let mut integers = info.integers();
        let index = integers.len().saturating_sub(1).min(1);
        let extra = integers.nth(index);
        if extra != Some(4) {
            return Err(fault(info, "additional combination count", extra));
        }
        apply_records(&mut entries, data)?;
        let mut combinations: Vec<_> = raw::COMBINATION_ARTIFACTS
            .iter()
            .zip(&raw::ARTIFACT_COMPONENTS)
            .map(|(&id, components)| (Some(id as usize), components.to_vec()))
            .collect();
        for (_, components) in &mut combinations {
            components.resize(entries.len(), false);
        }
        combinations.resize_with(catalog.rules.combination_count(), || {
            (None, vec![false; entries.len()])
        });
        // RVA 0x1058e0: update assembled ID first, then OR each in-range component.
        let values = stream(info)?;
        let mut values = values.into_iter();
        while let Some(combo) = values.next() {
            let index = usize::try_from(combo)
                .ok()
                .filter(|&i| i < combinations.len())
                .ok_or_else(|| fault(info, "combination ID", Some(combo)))?;
            let Some(assembled) = values.next() else {
                break;
            };
            combinations[index].0 = Some(artifact_id(info, assembled)?);
            let Some(count) = values.next() else {
                break;
            };
            for component in values.by_ref().take(usize::try_from(count).unwrap_or(0)) {
                // The native unsigned comparison ignores negative/out-of-range components.
                if let Ok(id) = artifact_id(info, component) {
                    combinations[index].1[id] = true;
                }
            }
        }
        // HD 0x44cd01: after the record overlay, visit recipes in ordinal order.
        for entry in &mut entries {
            entry.combination = None;
            entry.component_of = None;
        }
        for (index, (assembled, components)) in combinations.into_iter().enumerate() {
            let assembled = assembled.ok_or(ArtifactDataError::MissingCombination(index))?;
            entries[assembled].combination = Some(CombinationId(index));
            for (entry, component) in entries.iter_mut().zip(components) {
                if component {
                    entry.component_of = Some(CombinationId(index));
                }
            }
        }
        catalog.entries = entries.into_boxed_slice();
        Ok(catalog)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn base() -> Vec<u8> {
        format!(
            "header\r\nheader\r\n{}",
            format!("{}\r\n", "\t".repeat(22)).repeat(144)
        )
        .into_bytes()
    }
    fn container(extra: &[(&str, &str)], recipes: &str) -> Vec<u8> {
        fn word(bytes: &mut Vec<u8>, n: usize) {
            bytes.extend(i32::try_from(n).unwrap().to_le_bytes());
        }
        fn string(bytes: &mut Vec<u8>, value: &[u8]) {
            word(bytes, value.len());
            bytes.extend(value);
        }
        let mut bytes = b"HDAT\x02\0\0\0".to_vec();
        word(&mut bytes, extra.len() + 1);
        for (name, text) in std::iter::once(("artsinfo0", recipes)).chain(extra.iter().copied()) {
            string(&mut bytes, name.as_bytes());
            string(&mut bytes, b"");
            word(&mut bytes, 1); // Field 8 clamps to this only string.
            string(&mut bytes, text.as_bytes());
            bytes.push(0);
            word(&mut bytes, 2);
            word(&mut bytes, 0);
            word(&mut bytes, 4);
        }
        bytes
    }
    const RECIPES: &str =
        "12 141 3 66 67 68 13 142 3 57 58 59 14 160 3 115 116 117 15 143 4 10 16 22 28";

    #[test]
    fn partial_overlays_preserve_defaults_and_reserved_rows_cannot_be_enabled() {
        let bytes = container(
            &[
                ("art141", "0 2 16 0"),
                ("ART165", "10000 9 16 0"),
                ("art165", "10000 9 2"), // Keep the prior enabled flag.
                ("art144", "0 9 2 0"),
                ("art145", "0 9 2 0"),
            ],
            RECIPES,
        );
        let catalog =
            ArtifactCatalog::parse_hota181(&base(), Container::parse(&bytes).unwrap()).unwrap();
        let row = catalog.get(catalog.id(165).unwrap()).unwrap();
        assert_eq!(row.class(), ArtifactClass::Treasure);
        assert!(row.quest_eligible());
        for id in [144, 145, 146] {
            let row = catalog.get(catalog.id(id).unwrap()).unwrap();
            assert_eq!(row.class(), ArtifactClass::Unused);
            assert!(row.disabled());
        }
        let row = catalog.get(catalog.id(141).unwrap()).unwrap();
        assert!(!row.disabled());
        assert_eq!(row.combination().unwrap().index(), 12);
        assert!(catalog.id(166).is_none());
        assert!(ArtifactCatalog::parse(&base())
            .unwrap()
            .get(catalog.id(165).unwrap())
            .is_none());
    }

    #[test]
    fn combination_updates_accumulate_bits_and_last_recipe_owns_shared_components() {
        let recipes = format!("{RECIPES} 12 141 4 -1 166 152 165 15 143 1 152");
        let bytes = container(&[], &recipes);
        let catalog =
            ArtifactCatalog::parse_hota181(&base(), Container::parse(&bytes).unwrap()).unwrap();
        for (id, combo) in [(66, 12), (165, 12), (152, 15), (10, 15)] {
            assert_eq!(
                catalog
                    .get(catalog.id(id).unwrap())
                    .unwrap()
                    .component_of()
                    .unwrap()
                    .index(),
                combo
            );
        }
        assert_eq!(catalog.entries()[129].combination().unwrap().index(), 0);
    }

    #[test]
    fn invalid_classes_indices_and_missing_recipe_roots_fail_admission() {
        for (name, text, field) in [
            ("art166", "0 0 2 0", "artifact ID"),
            ("art165", "0 0 3 0", "artifact class"),
        ] {
            let bytes = container(&[(name, text)], RECIPES);
            assert!(
                matches!(ArtifactCatalog::parse_hota181(&base(), Container::parse(&bytes).unwrap()),
                Err(ArtifactDataError::Field { field: actual, .. }) if actual == field)
            );
        }
        let bytes = container(&[], "12 141 0");
        assert!(matches!(
            ArtifactCatalog::parse_hota181(&base(), Container::parse(&bytes).unwrap()),
            Err(ArtifactDataError::MissingCombination(13))
        ));
        let bytes = container(&[("art165", "0 0 2147483648")], RECIPES);
        assert!(matches!(
            ArtifactCatalog::parse_hota181(&base(), Container::parse(&bytes).unwrap()),
            Err(ArtifactDataError::Integer { token: 4, .. })
        ));
    }
}
