//! Artifact metadata used for quest eligibility and map availability.

use crate::rules::Ruleset;

mod data;
pub use data::ArtifactDataError;

use super::{parse_rows, prefix, raw, TraitError, TraitFault, TraitResource};

/// Artifact identity, excluding the no-artifact sentinel.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct ArtifactId(u8);
impl ArtifactId {
    /// Parse a Complete-era artifact ID.
    #[must_use]
    pub fn parse(value: i32) -> Option<Self> {
        let value = u8::try_from(value).ok()?;
        (i32::from(value) < raw::ARTIFACT_COUNT).then_some(Self(value))
    }
    /// Native table and serialized index.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0 as usize
    }
}

/// Index of an assembled artifact's combination recipe.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct CombinationId(usize);
impl CombinationId {
    /// Original recipe-table ordinal, distinct from artifact ID.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0
    }
}

/// Native artifact classes; every resource row has exactly one class.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum ArtifactClass {
    /// An initialized but unused expansion slot.
    Unused = 0,
    /// Spellbook, war machine or other non-random artifact.
    Special = crate::constants::ARTIFACT_CLASS_SPECIAL,
    /// Treasure-class artifacts may be seer-hut quest items.
    Treasure = crate::constants::ARTIFACT_CLASS_TREASURE,
    /// Minor artifact.
    Minor = crate::constants::ARTIFACT_CLASS_MINOR,
    /// Major artifact.
    Major = crate::constants::ARTIFACT_CLASS_MAJOR,
    /// Relic artifact.
    Relic = crate::constants::ARTIFACT_CLASS_RELIC,
}

/// Immutable artifact properties consumed by the generator.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ArtifactTraits {
    class: ArtifactClass,
    disabled: bool,
    combination: Option<CombinationId>,
    component_of: Option<CombinationId>,
}
impl ArtifactTraits {
    /// R/J/N/T classification from the spreadsheet.
    #[must_use]
    pub const fn class(self) -> ArtifactClass {
        self.class
    }
    /// Native artifact-loader exclusion flag.
    #[must_use]
    pub const fn disabled(self) -> bool {
        self.disabled
    }
    /// Combination recipe when this is an assembled artifact.
    #[must_use]
    pub const fn combination(self) -> Option<CombinationId> {
        self.combination
    }
    /// Combination recipe this artifact supplies as a component.
    #[must_use]
    pub const fn component_of(self) -> Option<CombinationId> {
        self.component_of
    }
    /// Trait eligibility before a generator checks its already-used pool.
    #[must_use]
    pub fn quest_eligible(self) -> bool {
        !self.disabled && self.class == ArtifactClass::Treasure
    }
}

/// Versioned artifact table; parsing does not copy names, descriptions or prices.
#[derive(Clone, Debug)]
pub struct ArtifactCatalog {
    entries: Box<[ArtifactTraits]>,
    rules: Ruleset,
}
impl ArtifactCatalog {
    /// Parse the rows consumed by the native artifact loader.
    ///
    /// Slot masks must identify a native equipment class; the C++ loader's
    /// unchecked search otherwise runs off its table. RMG does not retain the
    /// class index, costs, localized strings or the unrelated slot-name resource.
    ///
    /// # Errors
    /// Rejects malformed text, missing indexed cells and unknown slot masks.
    pub fn parse(bytes: &[u8]) -> Result<Self, TraitError> {
        let entries: [ArtifactTraits; raw::ARTIFACT_COUNT as usize] = parse_rows(
            bytes,
            TraitResource::Artifacts,
            |id| id + 2,
            |id, row_index, row| {
                let fields = prefix::<23>(row_index, row)?;
                let mut slots = 0_u32;
                for (column, &bit) in raw::ARTIFACT_SLOT_COLUMNS.iter().enumerate() {
                    if !matches!(fields[column + 2].decoded().next(), None | Some(0 | b' ')) {
                        slots |= 1 << bit;
                    }
                }
                if !raw::ARTIFACT_SLOT_MASKS.contains(&slots) {
                    return Err(TraitFault::UnknownArtifactSlots {
                        row: row_index,
                        bits: slots,
                    });
                }
                let class = match fields[21].decoded().next() {
                    Some(b'R') => ArtifactClass::Relic,
                    Some(b'J') => ArtifactClass::Major,
                    Some(b'N') => ArtifactClass::Minor,
                    Some(b'T') => ArtifactClass::Treasure,
                    _ => ArtifactClass::Special,
                };
                let combination = raw::COMBINATION_ARTIFACTS
                    .iter()
                    .position(|&artifact| artifact as usize == id)
                    .map(CombinationId);
                Ok(ArtifactTraits {
                    class,
                    disabled: raw::DISABLED_ARTIFACTS
                        .iter()
                        .any(|&artifact| artifact as usize == id),
                    combination,
                    component_of: raw::ARTIFACT_COMPONENTS
                        .iter()
                        .rposition(|row| row[id])
                        .map(CombinationId),
                })
            },
        )?;
        Ok(Self {
            entries: Box::new(entries),
            rules: Ruleset::Complete,
        })
    }
    /// Admit an artifact identity against this catalog.
    #[must_use]
    pub fn id(&self, value: i32) -> Option<ArtifactId> {
        let id = u8::try_from(value).ok()?;
        (usize::from(id) < self.entries.len()).then_some(ArtifactId(id))
    }
    /// Catalog's native generation rules.
    #[must_use]
    pub const fn ruleset(&self) -> Ruleset {
        self.rules
    }
    /// Checked lookup, including identities obtained from another ruleset.
    #[must_use]
    pub fn get(&self, id: ArtifactId) -> Option<&ArtifactTraits> {
        self.entries.get(id.index())
    }
    /// Native order, preserving deterministic eligibility scans.
    #[must_use]
    pub fn entries(&self) -> &[ArtifactTraits] {
        &self.entries
    }
}

#[cfg(test)]
mod pool_tests;
