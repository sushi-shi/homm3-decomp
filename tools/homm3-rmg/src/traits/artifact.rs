//! Artifact metadata used for quest eligibility and map availability.

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
    /// Every Complete-era artifact in table order.
    pub fn all() -> impl Iterator<Item = Self> {
        const { assert!(0 < raw::ARTIFACT_COUNT && raw::ARTIFACT_COUNT <= 1 << u8::BITS) };
        #[allow(clippy::cast_sign_loss)]
        (0..=u8::MAX).map(Self).take(raw::ARTIFACT_COUNT as usize)
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
    /// Trait eligibility before a generator checks its already-used pool.
    #[must_use]
    pub fn quest_eligible(self) -> bool {
        !self.disabled && self.class == ArtifactClass::Treasure
    }
}

/// Fixed artifact table; parsing does not copy names, descriptions or prices.
#[derive(Clone, Debug)]
pub struct ArtifactCatalog([ArtifactTraits; raw::ARTIFACT_COUNT as usize]);
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
        parse_rows(
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
                })
            },
        )
        .map(Self)
    }
    /// Lookup with a parsed artifact ID.
    #[must_use]
    pub const fn get(&self, id: ArtifactId) -> &ArtifactTraits {
        &self.0[id.index()]
    }
    /// Native order, preserving deterministic eligibility scans.
    #[must_use]
    pub fn entries(&self) -> &[ArtifactTraits] {
        &self.0
    }
}
