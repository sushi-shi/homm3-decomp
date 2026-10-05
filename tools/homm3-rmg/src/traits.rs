//! Numeric RMG views of native creature, spell and artifact resources.
//!
//! Catalogs own fixed arrays. Parsing borrows spreadsheet cells and neither
//! copies localized strings nor allocates a vector for each row. Source-owned
//! initializers supply faction/level and spell flags; resources supply the
//! numeric fields overwritten by the native loaders.

use crate::{parse, raw, request::Town};
use homm3_resource::{Field, Spreadsheet, SpreadsheetRow};
use std::{error::Error, fmt, num::NonZeroI32};

mod artifact;
pub use artifact::{ArtifactCatalog, ArtifactClass, ArtifactId, ArtifactTraits, CombinationId};

/// A native trait spreadsheet.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TraitResource {
    /// `crtraits.txt`.
    Creatures,
    /// `sptraits.txt`.
    Spells,
    /// `artraits.txt`.
    Artifacts,
}

/// A trait resource cannot enter the generation domain.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct TraitError {
    /// Resource being parsed.
    pub resource: TraitResource,
    /// Failure at the text or numeric boundary.
    pub fault: TraitFault,
}

/// Location and cause of a trait parsing failure.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum TraitFault {
    /// Invalid resource encoding.
    Spreadsheet(homm3_resource::Error),
    /// The native loader indexes a missing row.
    MissingRow {
        /// Zero-based spreadsheet row.
        row: usize,
    },
    /// The native loader indexes a missing column.
    MissingColumn {
        /// Zero-based spreadsheet row.
        row: usize,
        /// Required number of columns.
        required: usize,
    },
    /// CRT integer prefix exceeds the signed 32-bit domain.
    IntegerOverflow {
        /// Zero-based spreadsheet row.
        row: usize,
        /// Zero-based column.
        column: usize,
    },
    /// No native slot class matches the artifact's marked equipment columns.
    UnknownArtifactSlots {
        /// Zero-based spreadsheet row.
        row: usize,
        /// Parsed equipment-slot bits.
        bits: u32,
    },
}
impl fmt::Display for TraitError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:?} traits: ", self.resource)?;
        match &self.fault {
            TraitFault::Spreadsheet(error) => error.fmt(f),
            TraitFault::MissingRow { row } => write!(f, "missing row {row}"),
            TraitFault::MissingColumn { row, required } => {
                write!(f, "row {row} requires {required} columns")
            }
            TraitFault::IntegerOverflow { row, column } => {
                write!(f, "integer overflow at row {row}, column {column}")
            }
            TraitFault::UnknownArtifactSlots { row, bits } => {
                write!(f, "unknown artifact slot mask {bits:#x} at row {row}")
            }
        }
    }
}
impl Error for TraitError {}

/// A creature subtype available to the generator, excluding war machines.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct CreatureId(u8);
impl CreatureId {
    /// Parse an object subtype into the RMG creature domain.
    #[must_use]
    pub fn parse(value: i32) -> Option<Self> {
        let value = u8::try_from(value).ok()?;
        (u32::from(value) < raw::RMG_CREATURE_TYPE_COUNT).then_some(Self(value))
    }
    /// Original creature-table index.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0 as usize
    }
}

/// Zero-based creature tier, with the unused-entry sentinel excluded.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct CreatureTier(u8);
impl CreatureTier {
    /// Index for the seven creature reward bands.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0 as usize
    }
}

/// Creature data consumed by RMG; unused entries remain representable.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct CreatureTraits {
    town: Option<Town>,
    tier: Option<CreatureTier>,
    ai_value: Option<NonZeroI32>,
    growth: i32,
    wandering_low: i32,
    wandering_high: i32,
}
impl CreatureTraits {
    /// Faction, or `None` for a neutral creature.
    #[must_use]
    pub const fn town(self) -> Option<Town> {
        self.town
    }
    /// Tier, or `None` for an unused creature entry.
    #[must_use]
    pub const fn tier(self) -> Option<CreatureTier> {
        self.tier
    }
    /// AI value; zero is explicit and cannot become an unchecked divisor.
    /// Signed custom values are retained, with arithmetic checked at use sites.
    #[must_use]
    pub const fn ai_value(self) -> Option<NonZeroI32> {
        self.ai_value
    }
    /// Weekly growth as read by the native loader.
    #[must_use]
    pub const fn growth(self) -> i32 {
        self.growth
    }
    /// Native low/high wandering counts. Their sum is checked when used;
    /// neither their order nor positivity is assumed from resource bytes.
    #[must_use]
    pub const fn wandering_counts(self) -> (i32, i32) {
        (self.wandering_low, self.wandering_high)
    }
}

/// All native creature rows, including the trailing war-machine records.
#[derive(Clone, Debug)]
pub struct CreatureCatalog([CreatureTraits; raw::CREATURE_FACTIONS_AND_LEVELS.len()]);
impl CreatureCatalog {
    /// Parse the native section layout, retaining only RMG numeric fields.
    ///
    /// # Errors
    /// Rejects malformed text, missing indexed cells and integer overflow.
    ///
    /// # Panics
    /// If compiled-in C++ metadata contains an unknown faction or creature tier.
    /// Resource bytes do not control these fields.
    pub fn parse(bytes: &[u8]) -> Result<Self, TraitError> {
        parse_rows(
            bytes,
            TraitResource::Creatures,
            creature_row,
            |id, row_index, row| {
                let fields = prefix::<24>(row_index, row)?;
                let (faction, level) = raw::CREATURE_FACTIONS_AND_LEVELS[id];
                // These are source-owned constants, not resource discriminants.
                let town = if faction == raw::eTownNeutral {
                    None
                } else {
                    Some(Town::parse(faction).expect("canonical creature faction"))
                };
                let tier = if level == -1 {
                    None
                } else {
                    assert!((0..7).contains(&level), "canonical creature tier");
                    Some(CreatureTier(u8::try_from(level).unwrap()))
                };
                Ok(CreatureTraits {
                    town,
                    tier,
                    ai_value: NonZeroI32::new(number(&fields, row_index, 10)?),
                    growth: number(&fields, row_index, 11)?,
                    wandering_low: number(&fields, row_index, 21)?,
                    wandering_high: number(&fields, row_index, 22)?,
                })
            },
        )
        .map(Self)
    }
    /// Lookup with a previously parsed RMG creature subtype.
    #[must_use]
    pub const fn get(&self, id: CreatureId) -> &CreatureTraits {
        &self.0[id.index()]
    }
    /// Native table order, including the final war machines, for diagnostics.
    #[must_use]
    pub fn entries(&self) -> &[CreatureTraits] {
        &self.0
    }
}

/// A spell/ability index in the Complete trait table.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct SpellId(u8);
impl SpellId {
    /// Parse a native spell ID, including creature abilities.
    #[must_use]
    pub fn parse(value: i32) -> Option<Self> {
        let value = u8::try_from(value).ok()?;
        (usize::from(value) < raw::SPELL_FLAGS.len()).then_some(Self(value))
    }
    /// Original table index; reward selection separately excludes abilities.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0 as usize
    }
    /// Every spell and ability in table order.
    pub fn all() -> impl Iterator<Item = Self> {
        const { assert!(raw::SPELL_FLAGS.len() <= 1 << u8::BITS) };
        (0..=u8::MAX).map(Self).take(raw::SPELL_FLAGS.len())
    }
}

/// A subset of the four magic schools; no undefined bits are admitted.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SpellSchools(u32);
impl SpellSchools {
    /// Parse a wire school mask, including the empty ability mask.
    #[must_use]
    pub const fn parse(bits: u32) -> Option<Self> {
        if bits & !raw::eSchoolAll == 0 {
            Some(Self(bits))
        } else {
            None
        }
    }
    /// Original school bits.
    #[must_use]
    pub const fn bits(self) -> u32 {
        self.0
    }
    /// Whether either mask shares any school with the other.
    #[must_use]
    pub const fn intersects(self, other: Self) -> bool {
        self.0 & other.0 != 0
    }
}

/// Spell values RMG compares when choosing scrolls and Pandora rewards.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SpellTraits {
    level: i32,
    schools: SpellSchools,
    disabled: bool,
}
impl SpellTraits {
    /// Resource level; this is a comparison value, never an array index.
    #[must_use]
    pub const fn level(self) -> i32 {
        self.level
    }
    /// School membership (abilities may have none).
    #[must_use]
    pub const fn schools(self) -> SpellSchools {
        self.schools
    }
    /// Whether the native map loader disables this spell by default.
    #[must_use]
    pub const fn disabled_by_default(self) -> bool {
        self.disabled
    }
}

/// Immutable spell and ability metadata, with no per-row allocation.
#[derive(Clone, Debug)]
pub struct SpellCatalog([SpellTraits; raw::SPELL_FLAGS.len()]);
impl SpellCatalog {
    /// Parse adventure spells, combat spells and abilities in native order.
    ///
    /// # Errors
    /// Rejects malformed text, missing indexed cells and integer overflow.
    pub fn parse(bytes: &[u8]) -> Result<Self, TraitError> {
        parse_rows(
            bytes,
            TraitResource::Spells,
            spell_row,
            |id, row_index, row| {
                let fields = prefix::<33>(row_index, row)?;
                let mut schools = 0;
                for (column, school) in [
                    (3, raw::eSchoolEarth),
                    (4, raw::eSchoolWater),
                    (5, raw::eSchoolFire),
                    (6, raw::eSchoolAir),
                ] {
                    // Native tests the first byte, not whitespace-trimmed content.
                    if !matches!(fields[column].decoded().next(), None | Some(0 | b' ')) {
                        schools |= school;
                    }
                }
                Ok(SpellTraits {
                    level: number(&fields, row_index, 2)?,
                    schools: SpellSchools(schools),
                    disabled: raw::SPELL_FLAGS[id] & raw::RMG_SPELL_DISABLED_BY_DEFAULT != 0,
                })
            },
        )
        .map(Self)
    }
    /// Lookup with a parsed spell/ability ID.
    #[must_use]
    pub const fn get(&self, id: SpellId) -> &SpellTraits {
        &self.0[id.index()]
    }
    /// Complete native order, for deterministic selection and diagnostics.
    #[must_use]
    pub fn entries(&self) -> &[SpellTraits] {
        &self.0
    }
    /// Identified entries in native order.
    pub fn iter(&self) -> impl Iterator<Item = (SpellId, &SpellTraits)> {
        SpellId::all().zip(&self.0)
    }
}

fn creature_row(id: usize) -> usize {
    // Eight 14-creature factions, six neutrals, Conflux, expansion neutrals,
    // then five war machines. Native skips three rows between each section.
    let preceding_sections = match id {
        0..=111 => id / 14,
        112..=117 => 8,
        118..=131 => 9,
        132..=144 => 10,
        _ => 11,
    };
    2 + id + 3 * preceding_sections
}

fn spell_row(id: usize) -> usize {
    5 + id
        + match id {
            0..=9 => 0,
            10..=69 => 3,
            _ => 6,
        }
}

fn parse_rows<T: Copy, const N: usize>(
    bytes: &[u8],
    resource: TraitResource,
    source_row: impl Fn(usize) -> usize,
    mut parse_row: impl FnMut(usize, usize, SpreadsheetRow<'_>) -> Result<T, TraitFault>,
) -> Result<[T; N], TraitError> {
    let error = |fault| TraitError { resource, fault };
    let sheet = Spreadsheet::parse(bytes).map_err(|fault| error(TraitFault::Spreadsheet(fault)))?;
    let mut rows = sheet.rows().enumerate();
    let mut parsed = [None; N];
    for (id, entry) in parsed.iter_mut().enumerate() {
        let wanted = source_row(id);
        let (_, row) = rows
            .find(|(index, _)| *index == wanted)
            .ok_or_else(|| error(TraitFault::MissingRow { row: wanted }))?;
        *entry = Some(parse_row(id, wanted, row).map_err(error)?);
    }
    Ok(parsed.map(|entry| entry.expect("all native trait rows parsed")))
}

fn prefix<const N: usize>(
    row_index: usize,
    row: SpreadsheetRow<'_>,
) -> Result<[Field<'_>; N], TraitFault> {
    let mut fields = row.cells();
    let mut result = [None; N];
    for field in &mut result {
        *field = Some(fields.next().ok_or(TraitFault::MissingColumn {
            row: row_index,
            required: N,
        })?);
    }
    Ok(result.map(|field| field.expect("required fields parsed")))
}

fn number(fields: &[Field<'_>], row: usize, column: usize) -> Result<i32, TraitFault> {
    parse::integer(fields[column].decoded()).ok_or(TraitFault::IntegerOverflow { row, column })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sheet(rows: usize, columns: usize, edits: &[(usize, usize, &str)]) -> Vec<u8> {
        let mut text = String::new();
        for row in 0..rows {
            for column in 0..columns {
                if column > 0 {
                    text.push('\t');
                }
                if let Some((_, _, value)) =
                    edits.iter().find(|(r, c, _)| *r == row && *c == column)
                {
                    text.push_str(value);
                }
            }
            text.push_str("\r\n");
        }
        text.into_bytes()
    }

    #[test]
    fn source_sentinels_and_numeric_prefixes_survive_parsing() {
        let data = sheet(
            185,
            24,
            &[
                (2, 10, "  +42tail"),
                (2, 11, "-3"),
                (2, 21, "9"),
                (2, 22, "4"),
                (151, 10, "0"),
            ],
        );
        let catalog = CreatureCatalog::parse(&data).unwrap();
        let first = catalog.get(CreatureId::parse(0).unwrap());
        assert_eq!(first.town().unwrap().index(), 0);
        assert_eq!(first.tier().unwrap().index(), 0);
        assert_eq!(first.ai_value().unwrap().get(), 42);
        assert_eq!(first.growth(), -3);
        assert_eq!(first.wandering_counts(), (9, 4));
        let unused = catalog.get(CreatureId::parse(122).unwrap());
        assert_eq!(unused.tier(), None);
        assert_eq!(unused.ai_value(), None);
        assert_eq!(catalog.get(CreatureId::parse(139).unwrap()).town(), None);
        assert!(CreatureId::parse(-1).is_none());
        assert!(CreatureId::parse(145).is_none());
    }

    #[test]
    fn loader_reports_actual_index_fault_beyond_weak_native_row_check() {
        // Native checks for only 179 rows, then indexes through row 184.
        let error = CreatureCatalog::parse(&sheet(179, 24, &[])).unwrap_err();
        assert_eq!(error.fault, TraitFault::MissingRow { row: 180 });
        let error = CreatureCatalog::parse(&sheet(185, 23, &[])).unwrap_err();
        assert_eq!(
            error.fault,
            TraitFault::MissingColumn {
                row: 2,
                required: 24
            }
        );
        let error = CreatureCatalog::parse(&sheet(185, 24, &[(2, 10, "2147483648")])).unwrap_err();
        assert_eq!(
            error.fault,
            TraitFault::IntegerOverflow { row: 2, column: 10 }
        );
    }

    #[test]
    fn spell_school_cells_test_first_byte_without_trimming() {
        let data = sheet(
            92,
            33,
            &[
                (5, 2, "5suffix"),
                (5, 3, " x"),
                (5, 4, "x"),
                (5, 5, "\n"),
                (5, 6, "\"x\""),
                (91, 2, "-1"),
            ],
        );
        let catalog = SpellCatalog::parse(&data).unwrap();
        let first = catalog.get(SpellId::parse(0).unwrap());
        assert_eq!(first.level(), 5);
        assert_eq!(
            first.schools().bits(),
            raw::eSchoolWater | raw::eSchoolFire | raw::eSchoolAir
        );
        assert_eq!(catalog.get(SpellId::parse(80).unwrap()).level(), -1);
        assert!(SpellId::parse(81).is_none());
        assert!(SpellId::parse(-1).is_none());
        assert!(SpellSchools::parse(16).is_none());
        assert!(!SpellSchools::parse(0).unwrap().intersects(first.schools()));
    }

    #[test]
    fn spell_resource_faults_keep_original_row_and_column() {
        assert_eq!(
            SpellCatalog::parse(&sheet(91, 33, &[])).unwrap_err().fault,
            TraitFault::MissingRow { row: 91 }
        );
        assert_eq!(
            SpellCatalog::parse(&sheet(92, 32, &[])).unwrap_err().fault,
            TraitFault::MissingColumn {
                row: 5,
                required: 33
            }
        );
        assert_eq!(
            SpellCatalog::parse(&sheet(92, 33, &[(18, 2, "-2147483649")]))
                .unwrap_err()
                .fault,
            TraitFault::IntegerOverflow { row: 18, column: 2 }
        );
    }

    #[test]
    fn artifact_classes_and_exclusions_keep_native_semantics() {
        let data = sheet(
            146,
            23,
            &[
                (2, 21, "R"),
                (3, 21, "J"),
                (4, 21, "N"),
                (5, 21, "T"),
                (6, 21, " T"),
                (143, 21, "T"),
            ],
        );
        let catalog = ArtifactCatalog::parse(&data).unwrap();
        assert_eq!(
            catalog.get(ArtifactId::parse(0).unwrap()).class(),
            ArtifactClass::Relic
        );
        assert_eq!(
            catalog.get(ArtifactId::parse(1).unwrap()).class(),
            ArtifactClass::Major
        );
        assert_eq!(
            catalog.get(ArtifactId::parse(2).unwrap()).class(),
            ArtifactClass::Minor
        );
        assert!(catalog.get(ArtifactId::parse(3).unwrap()).quest_eligible());
        assert!(!catalog.get(ArtifactId::parse(4).unwrap()).quest_eligible());
        assert!(!catalog
            .get(ArtifactId::parse(141).unwrap())
            .quest_eligible());
        assert!(catalog.get(ArtifactId::parse(141).unwrap()).disabled());
        assert_eq!(
            catalog
                .get(ArtifactId::parse(129).unwrap())
                .combination()
                .unwrap()
                .index(),
            0
        );
        assert!(ArtifactId::parse(-1).is_none());
        assert!(ArtifactId::parse(144).is_none());
    }

    #[test]
    fn unknown_artifact_slot_class_faults_before_native_unbounded_search() {
        // Columns 2/3 mark two unrelated single-slot classes, not a supported pair.
        let error =
            ArtifactCatalog::parse(&sheet(146, 23, &[(2, 2, "x"), (2, 3, "x")])).unwrap_err();
        assert_eq!(
            error.fault,
            TraitFault::UnknownArtifactSlots {
                row: 2,
                bits: (1 << 17) | (1 << 16)
            }
        );
        assert_eq!(
            ArtifactCatalog::parse(&sheet(145, 23, &[]))
                .unwrap_err()
                .fault,
            TraitFault::MissingRow { row: 145 }
        );
        assert_eq!(
            ArtifactCatalog::parse(&sheet(146, 22, &[]))
                .unwrap_err()
                .fault,
            TraitFault::MissingColumn {
                row: 2,
                required: 23
            }
        );
    }
}
