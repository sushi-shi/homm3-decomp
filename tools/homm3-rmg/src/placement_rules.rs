//! Parsed `rand_trn.txt` with original row identities and flat score storage.

use crate::{
    behavior::Behavior, domain::Terrain, identity::OwnerId, object::ObjectKind, parse, raw,
    rules::Ruleset,
};
use homm3_resource::{Field, Spreadsheet, SpreadsheetRow};
use std::{collections::TryReserveError, error::Error, fmt};

#[cfg(test)]
const PREFIX: usize = raw::RMG_PLACEMENT_COLUMN_NEIGHBOUR_SCORES as usize;
const FIRST_ROW: usize = raw::RMG_FIRST_DATA_ROW as usize;

/// Compacted rule identity, distinct from its original spreadsheet row.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PlacementRuleId {
    index: usize,
    owner: OwnerId,
}
impl PlacementRuleId {
    /// Dense storage position, for diagnostics.
    #[must_use]
    pub const fn index(self) -> usize {
        self.index
    }
}

/// A placement rule with all identities parsed and score columns available.
#[derive(Clone, Debug)]
pub struct PlacementRule {
    source_row: usize,
    object: ObjectKind,
    subtype: i32,
    terrain: Terrain,
    terrain_scores: Box<[i32]>,
}
impl PlacementRule {
    /// Original data-row ordinal, including skipped hotfix rows.
    #[must_use]
    pub const fn source_row(&self) -> usize {
        self.source_row
    }
    /// Object identity as written in the rule table (no alias applied here).
    #[must_use]
    pub const fn object(&self) -> ObjectKind {
        self.object
    }
    /// Object subtype as written in the table.
    #[must_use]
    pub const fn subtype(&self) -> i32 {
        self.subtype
    }
    /// Preferred terrain matched when binding prototypes.
    #[must_use]
    pub const fn terrain(&self) -> Terrain {
        self.terrain
    }
    /// Score for the terrain under a blocked footprint cell; absent when this
    /// rule table does not contain that terrain's score column.
    #[must_use]
    pub fn terrain_score(&self, terrain: Terrain) -> Option<i32> {
        self.terrain_scores.get(terrain.index()).copied()
    }
}

/// Which neighbour matrix to use while scoring a placement.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum NeighbourScore {
    /// Touching another object's footprint.
    Adjacent,
    /// Blocking another object's footprint.
    Blocked,
}

/// A placement table cannot safely enter the generation domain.
#[derive(Debug)]
pub enum PlacementRuleError {
    /// Process-local ownership tags have been exhausted; no tag is reused.
    IdentityExhausted,
    /// Malformed resource encoding.
    Spreadsheet(homm3_resource::Error),
    /// A numeric prefix overflows signed 32-bit conversion.
    IntegerOverflow {
        /// Zero-based spreadsheet row.
        row: usize,
        /// Zero-based column.
        column: usize,
    },
    /// Retail indexes a missing field.
    RetailShortRow {
        /// Zero-based spreadsheet row.
        row: usize,
        /// Required field count.
        required: usize,
    },
    /// Retail indexes an object or terrain table with an invalid identity.
    RetailIdentity {
        /// Zero-based spreadsheet row.
        row: usize,
        /// Parsed object ID.
        object: i32,
        /// Parsed terrain ID.
        terrain: i32,
    },
    /// Score matrix dimensions overflow the host address domain.
    Capacity,
    /// Storage could not be reserved.
    Allocation(TryReserveError),
}
impl fmt::Display for PlacementRuleError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::IdentityExhausted => f.write_str("placement rule ownership identities exhausted"),
            Self::Spreadsheet(error) => error.fmt(f),
            Self::IntegerOverflow { row, column } => write!(
                f,
                "rand_trn.txt row {row}, column {column}: integer overflow"
            ),
            Self::RetailShortRow { row, required } => write!(
                f,
                "rand_trn.txt row {row}: retail requires {required} fields"
            ),
            Self::RetailIdentity {
                row,
                object,
                terrain,
            } => write!(
                f,
                "rand_trn.txt row {row}: invalid retail identity {object}/{terrain}"
            ),
            Self::Capacity => f.write_str("placement score matrix exceeds addressable storage"),
            Self::Allocation(error) => error.fmt(f),
        }
    }
}
impl Error for PlacementRuleError {}
impl From<TryReserveError> for PlacementRuleError {
    fn from(error: TryReserveError) -> Self {
        Self::Allocation(error)
    }
}

/// Immutable rules and two matrices; no per-rule neighbour vectors.
#[derive(Debug)]
pub struct PlacementRules {
    behavior: Behavior,
    ruleset: Ruleset,
    owner: OwnerId,
    rules: Vec<PlacementRule>,
    scores: Vec<i32>,
    columns: usize,
}
impl PlacementRules {
    pub(crate) const fn behavior(&self) -> Behavior {
        self.behavior
    }
    fn id(&self, index: usize) -> PlacementRuleId {
        PlacementRuleId {
            index,
            owner: self.owner,
        }
    }
    /// Parse all rows up to the first blank leading field. Hotfix skips short
    /// rows and invalid identities, retaining their columns in neighbour scores.
    ///
    /// # Errors
    /// Reports malformed text, integer overflow, retail index faults, allocation failure
    /// or exhausted ownership tags.
    pub fn parse(bytes: &[u8], behavior: Behavior) -> Result<Self, PlacementRuleError> {
        Self::parse_for(bytes, behavior, Ruleset::Complete)
    }

    /// Read the versioned terrain columns and neighbour matrices. `HotA` uses
    /// columns 16 onward for additional terrains, followed by both matrices;
    /// rock has no input column. Its native loader does not skip invalid rows.
    ///
    /// # Errors
    /// Reports malformed input, unsafe native indexing, allocation failure or
    /// exhausted ownership tags.
    pub fn parse_for(
        bytes: &[u8],
        behavior: Behavior,
        ruleset: Ruleset,
    ) -> Result<Self, PlacementRuleError> {
        let terrain_count = ruleset.terrain_count();
        let prefix_count = 6 + terrain_count;
        let skip_invalid = ruleset == Ruleset::Complete && behavior.is_hotfix();
        let sheet = Spreadsheet::parse(bytes).map_err(PlacementRuleError::Spreadsheet)?;
        let columns = sheet.rows().skip(FIRST_ROW).take_while(nonblank).count();
        let row_scores = columns.checked_mul(2).ok_or(PlacementRuleError::Capacity)?;
        let required = prefix_count
            .checked_add(row_scores)
            .ok_or(PlacementRuleError::Capacity)?;
        let mut result = Self {
            behavior,
            ruleset,
            owner: OwnerId::new().ok_or(PlacementRuleError::IdentityExhausted)?,
            rules: Vec::new(),
            scores: Vec::new(),
            columns,
        };
        result.rules.try_reserve(columns)?;
        for (source_row, row) in sheet.rows().skip(FIRST_ROW).take(columns).enumerate() {
            let row_index = source_row + FIRST_ROW;
            if row.len() < required {
                if skip_invalid {
                    continue;
                }
                return Err(PlacementRuleError::RetailShortRow {
                    row: row_index,
                    required,
                });
            }
            let mut fields = row.cells();
            let mut prefix = vec![None; prefix_count];
            for (slot, field) in prefix.iter_mut().zip(fields.by_ref()) {
                *slot = Some(field);
            }
            let number = |column: u32| {
                parse::integer(prefix[column as usize].into_iter().flat_map(Field::decoded)).ok_or(
                    PlacementRuleError::IntegerOverflow {
                        row: row_index,
                        column: column as usize,
                    },
                )
            };
            let object = number(raw::RMG_PLACEMENT_COLUMN_OBJECT_TYPE)?;
            let terrain = number(raw::RMG_PLACEMENT_COLUMN_TERRAIN)?;
            let (Some(object), Some(terrain)) = (
                ObjectKind::parse(object),
                Terrain::parse_for(terrain, ruleset),
            ) else {
                if skip_invalid {
                    continue;
                }
                return Err(PlacementRuleError::RetailIdentity {
                    row: row_index,
                    object,
                    terrain,
                });
            };
            let mut terrain_scores = vec![raw::RMG_PLACEMENT_INVALID; terrain_count];
            for (column, score) in (raw::RMG_PLACEMENT_COLUMN_TERRAIN_SCORES..).zip(
                terrain_scores
                    .iter_mut()
                    .take(raw::eTerrainWater as usize + 1),
            ) {
                *score = number(column)?;
            }
            for (terrain, score) in terrain_scores.iter_mut().enumerate().skip(10) {
                *score =
                    number(u32::try_from(6 + terrain).map_err(|_| PlacementRuleError::Capacity)?)?;
            }
            result.scores.try_reserve(row_scores)?;
            for (offset, field) in fields.take(row_scores).enumerate() {
                result
                    .scores
                    .push(integer(field, row_index, prefix_count + offset)?);
            }
            result.rules.push(PlacementRule {
                source_row,
                object,
                terrain,
                terrain_scores: terrain_scores.into_boxed_slice(),
                subtype: number(raw::RMG_PLACEMENT_COLUMN_SUBTYPE)?,
            });
        }
        Ok(result)
    }
    /// Generation rules that own this table's column layout and binding policy.
    #[must_use]
    pub const fn ruleset(&self) -> Ruleset {
        self.ruleset
    }
    /// Retained rules in source order.
    #[must_use]
    pub fn rules(&self) -> &[PlacementRule] {
        &self.rules
    }
    /// Retained rules paired with their dense identities, in source order.
    #[must_use = "iterators are lazy"]
    pub fn iter(
        &self,
    ) -> impl ExactSizeIterator<Item = (PlacementRuleId, &PlacementRule)> + DoubleEndedIterator
    {
        self.rules
            .iter()
            .enumerate()
            .map(|(index, rule)| (self.id(index), rule))
    }
    /// Matrix column count, including rejected hotfix rows.
    #[must_use]
    pub const fn source_rows(&self) -> usize {
        self.columns
    }
    /// Rule lookup after choosing a source-derived ID.
    #[must_use]
    pub fn get(&self, rule: PlacementRuleId) -> Option<&PlacementRule> {
        (rule.owner == self.owner)
            .then(|| self.rules.get(rule.index))
            .flatten()
    }
    /// Bind by type, subtype and preferred terrain: last match for Complete,
    /// first match for `HotA`. The caller supplies the versioned prototype bucket.
    #[must_use]
    pub fn find(
        &self,
        family: ObjectKind,
        subtype: i32,
        terrain: Terrain,
    ) -> Option<PlacementRuleId> {
        let matches = |rule: &PlacementRule| {
            rule.object == family && rule.subtype == subtype && rule.terrain == terrain
        };
        let index = match self.ruleset {
            Ruleset::Complete => self.rules.iter().rposition(matches),
            Ruleset::HotA181 => self.rules.iter().position(matches),
        };
        index.map(|index| self.id(index))
    }
    /// Look up another retained rule using its original source column.
    #[must_use]
    pub fn neighbour_score(
        &self,
        rule: PlacementRuleId,
        neighbour: PlacementRuleId,
        kind: NeighbourScore,
    ) -> Option<i32> {
        self.get(rule)?;
        let column = self.get(neighbour)?.source_row;
        let matrix = match kind {
            NeighbourScore::Adjacent => 0,
            NeighbourScore::Blocked => 1,
        };
        self.scores
            .get((rule.index * 2 + matrix) * self.columns + column)
            .copied()
    }
}

fn nonblank(row: &SpreadsheetRow<'_>) -> bool {
    row.cells()
        .next()
        .and_then(|field| field.decoded().next())
        .is_some_and(|byte| byte != 0 && byte != b' ')
}
fn integer(field: Field<'_>, row: usize, column: usize) -> Result<i32, PlacementRuleError> {
    parse::integer(field.decoded()).ok_or(PlacementRuleError::IntegerOverflow { row, column })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::behavior::RetailProfile;

    fn row(object: u32, index: usize, count: usize) -> Vec<String> {
        let mut fields = vec![String::new(); PREFIX + 2 * count];
        fields[0] = "row".into();
        fields[raw::RMG_PLACEMENT_COLUMN_OBJECT_TYPE as usize] = object.to_string();
        fields[raw::RMG_PLACEMENT_COLUMN_TERRAIN as usize] = "0".into();
        for terrain in 0..=raw::eTerrainWater as usize {
            fields[raw::RMG_PLACEMENT_COLUMN_TERRAIN_SCORES as usize + terrain] =
                (terrain + 1).to_string();
        }
        for column in 0..count {
            fields[PREFIX + column] = (index * 100 + column).to_string();
            fields[PREFIX + count + column] = (1000 + index * 100 + column).to_string();
        }
        fields
    }
    fn sheet(rows: &[Vec<String>]) -> Vec<u8> {
        let mut text = "header\r\nheader\r\nheader\r\n".to_owned();
        for fields in rows {
            text.push_str(&fields.join("\t"));
            text.push_str("\r\n");
        }
        text.into_bytes()
    }

    #[test]
    fn hota_columns_skip_rock_and_first_binding_keeps_matrix_identity() {
        let mut rows = vec![row(raw::TOWN, 0, 2), row(raw::TOWN, 1, 2)];
        for fields in &mut rows {
            fields.splice(PREFIX..PREFIX, ["101".into(), "202".into()]);
            fields[raw::RMG_PLACEMENT_COLUMN_TERRAIN as usize] = "11".into();
        }
        let bytes = sheet(&rows);
        let rules = PlacementRules::parse_for(&bytes, Behavior::Hotfix, Ruleset::HotA181).unwrap();
        assert_eq!(rules.ruleset(), Ruleset::HotA181);
        let id = rules.find(ObjectKind::TOWN, 0, Terrain::Wasteland).unwrap();
        assert_eq!(id.index(), 0);
        let rule = rules.get(id).unwrap();
        assert_eq!(rule.terrain_score(Terrain::Water), Some(9));
        assert_eq!(rule.terrain_score(Terrain::Rock), Some(-5000));
        assert_eq!(rule.terrain_score(Terrain::Highlands), Some(101));
        assert_eq!(rule.terrain_score(Terrain::Wasteland), Some(202));
        let other = rules.iter().nth(1).unwrap().0;
        assert_eq!(
            rules.neighbour_score(id, other, NeighbourScore::Adjacent),
            Some(1)
        );
        assert_eq!(
            rules.neighbour_score(other, id, NeighbourScore::Blocked),
            Some(1100)
        );

        rows[0].truncate(PREFIX + 4);
        assert!(matches!(
            PlacementRules::parse_for(&sheet(&rows), Behavior::Hotfix, Ruleset::HotA181),
            Err(PlacementRuleError::RetailShortRow {
                row: 3,
                required: 22
            })
        ));
        rows[0] = row(raw::ADVENTURE_OBJECT_TRAIT_COUNT, 0, 2);
        rows[0].splice(PREFIX..PREFIX, ["0".into(), "0".into()]);
        assert!(matches!(
            PlacementRules::parse_for(&sheet(&rows), Behavior::Hotfix, Ruleset::HotA181),
            Err(PlacementRuleError::RetailIdentity { row: 3, .. })
        ));
    }

    #[test]
    fn handles_cannot_select_rules_or_score_columns_from_another_table() {
        let bytes = sheet(&[row(raw::TOWN, 0, 1)]);
        let first = PlacementRules::parse(&bytes, Behavior::Hotfix).unwrap();
        let second = PlacementRules::parse(&bytes, Behavior::Hotfix).unwrap();
        let a = first.iter().next().unwrap().0;
        let b = second.iter().next().unwrap().0;
        assert_eq!(a.index(), b.index());
        assert!(first.get(b).is_none());
        assert!(second.get(a).is_none());
        assert!(first
            .neighbour_score(a, b, NeighbourScore::Adjacent)
            .is_none());
        assert!(first
            .neighbour_score(b, a, NeighbourScore::Blocked)
            .is_none());
        let moved = first;
        assert_eq!(moved.get(a).unwrap().source_row(), 0);
    }

    #[test]
    fn hotfix_skips_invalid_identity_but_keeps_original_score_columns() {
        let mut skipped = row(raw::ADVENTURE_OBJECT_TRAIT_COUNT, 1, 3);
        skipped[raw::RMG_PLACEMENT_COLUMN_TERRAIN_SCORES as usize] = "999999999999999".into();
        let bytes = sheet(&[row(raw::TOWN, 0, 3), skipped, row(raw::MONSTER, 2, 3)]);
        let rules = PlacementRules::parse(&bytes, Behavior::Hotfix).unwrap();
        assert_eq!(rules.source_rows(), 3);
        assert_eq!(rules.rules().len(), 2);
        let town = rules.find(ObjectKind::TOWN, 0, Terrain::Dirt).unwrap();
        let monster = rules.find(ObjectKind::MONSTER, 0, Terrain::Dirt).unwrap();
        assert_eq!(rules.get(monster).unwrap().source_row(), 2);
        assert_eq!(
            rules.neighbour_score(town, monster, NeighbourScore::Adjacent),
            Some(2)
        );
        assert_eq!(
            rules.neighbour_score(monster, town, NeighbourScore::Blocked),
            Some(1200)
        );
        assert_eq!(
            rules.get(town).unwrap().terrain_score(Terrain::Water),
            Some(9)
        );
        assert_eq!(
            rules.get(town).unwrap().terrain_score(Terrain::Rock),
            Some(raw::RMG_PLACEMENT_INVALID)
        );
        assert_eq!(
            rules.get(town).unwrap().terrain_score(Terrain::Highlands),
            None
        );
        assert_eq!(
            rules.get(town).unwrap().terrain_score(Terrain::Wasteland),
            None
        );
        assert!(matches!(
            PlacementRules::parse(&bytes, Behavior::Retail(RetailProfile::default())),
            Err(PlacementRuleError::RetailIdentity { row: 4, .. })
        ));
    }

    #[test]
    fn last_matching_rule_wins_and_blank_leading_field_ends_the_table() {
        let bytes = sheet(&[
            row(raw::TOWN, 0, 2),
            row(raw::TOWN, 1, 2),
            vec![" ".into()],
            vec!["bad ignored row".into()],
        ]);
        for behavior in [Behavior::Hotfix, Behavior::Retail(RetailProfile::default())] {
            let rules = PlacementRules::parse(&bytes, behavior).unwrap();
            let selected = rules.find(ObjectKind::TOWN, 0, Terrain::Dirt).unwrap();
            assert_eq!(selected.index(), 1);
            assert_eq!(
                rules.neighbour_score(selected, selected, NeighbourScore::Adjacent),
                Some(101)
            );
        }
    }

    #[test]
    fn short_row_is_a_hotfix_skip_and_a_retail_fault() {
        let bytes = sheet(&[vec!["short row".into()], row(raw::TOWN, 1, 2)]);
        let rules = PlacementRules::parse(&bytes, Behavior::Hotfix).unwrap();
        assert_eq!(rules.rules().len(), 1);
        let town = rules.find(ObjectKind::TOWN, 0, Terrain::Dirt).unwrap();
        assert_eq!(
            rules.neighbour_score(town, town, NeighbourScore::Adjacent),
            Some(101)
        );
        assert!(matches!(
            PlacementRules::parse(&bytes, Behavior::Retail(RetailProfile::default())),
            Err(PlacementRuleError::RetailShortRow { row: 3, .. })
        ));
    }
}
