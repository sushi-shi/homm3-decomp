//! Parsed `rand_trn.txt` with original row identities and flat score storage.

use crate::{behavior::Behavior, domain::Terrain, object::ObjectKind, parse, raw};
use homm3_resource::{Field, Spreadsheet, SpreadsheetRow};
use std::{collections::TryReserveError, error::Error, fmt};

const PREFIX: usize = raw::RMG_PLACEMENT_COLUMN_NEIGHBOUR_SCORES as usize;
const FIRST_ROW: usize = raw::RMG_FIRST_DATA_ROW as usize;

/// Compacted rule identity, distinct from its original spreadsheet row.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PlacementRuleId(usize);
impl PlacementRuleId {
    /// Dense storage position, for diagnostics.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0
    }
}

/// A placement rule with all identities parsed and score columns available.
#[derive(Clone, Debug)]
pub struct PlacementRule {
    source_row: usize,
    object: ObjectKind,
    subtype: i32,
    terrain: Terrain,
    terrain_scores: [i32; raw::RMG_TERRAIN_COUNT as usize],
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
    /// Score for the terrain under a blocked footprint cell.
    #[must_use]
    pub const fn terrain_score(&self, terrain: Terrain) -> i32 {
        self.terrain_scores[terrain.index()]
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
    rules: Vec<PlacementRule>,
    scores: Vec<i32>,
    columns: usize,
}
impl PlacementRules {
    /// Parse all rows up to the first blank leading field. Hotfix skips short
    /// rows and invalid identities, retaining their columns in neighbour scores.
    ///
    /// # Errors
    /// Reports malformed text, integer overflow, retail index faults or allocation failure.
    pub fn parse(bytes: &[u8], behavior: Behavior) -> Result<Self, PlacementRuleError> {
        let sheet = Spreadsheet::parse(bytes).map_err(PlacementRuleError::Spreadsheet)?;
        let columns = sheet.rows().skip(FIRST_ROW).take_while(nonblank).count();
        let row_scores = columns.checked_mul(2).ok_or(PlacementRuleError::Capacity)?;
        let required = PREFIX
            .checked_add(row_scores)
            .ok_or(PlacementRuleError::Capacity)?;
        let mut result = Self {
            rules: Vec::new(),
            scores: Vec::new(),
            columns,
        };
        result.rules.try_reserve(columns)?;
        for (source_row, row) in sheet.rows().skip(FIRST_ROW).take(columns).enumerate() {
            let row_index = source_row + FIRST_ROW;
            if row.len() < required {
                if behavior.is_hotfix() {
                    continue;
                }
                return Err(PlacementRuleError::RetailShortRow {
                    row: row_index,
                    required,
                });
            }
            let mut fields = row.cells();
            let mut prefix = [None; PREFIX];
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
            let (Some(object), Some(terrain)) =
                (ObjectKind::parse(object), Terrain::parse(terrain))
            else {
                if behavior.is_hotfix() {
                    continue;
                }
                return Err(PlacementRuleError::RetailIdentity {
                    row: row_index,
                    object,
                    terrain,
                });
            };
            let mut terrain_scores = [raw::RMG_PLACEMENT_INVALID; raw::RMG_TERRAIN_COUNT as usize];
            for (column, score) in (raw::RMG_PLACEMENT_COLUMN_TERRAIN_SCORES..).zip(
                terrain_scores
                    .iter_mut()
                    .take(raw::eTerrainWater as usize + 1),
            ) {
                *score = number(column)?;
            }
            result.scores.try_reserve(row_scores)?;
            for (offset, field) in fields.take(row_scores).enumerate() {
                result
                    .scores
                    .push(integer(field, row_index, PREFIX + offset)?);
            }
            result.rules.push(PlacementRule {
                source_row,
                object,
                terrain,
                terrain_scores,
                subtype: number(raw::RMG_PLACEMENT_COLUMN_SUBTYPE)?,
            });
        }
        Ok(result)
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
            .map(|(index, rule)| (PlacementRuleId(index), rule))
    }
    /// Matrix column count, including rejected hotfix rows.
    #[must_use]
    pub const fn source_rows(&self) -> usize {
        self.columns
    }
    /// Rule lookup after choosing a source-derived ID.
    #[must_use]
    pub fn get(&self, rule: PlacementRuleId) -> Option<&PlacementRule> {
        self.rules.get(rule.0)
    }
    /// Bind the last matching rule. The caller supplies the prototype family.
    #[must_use]
    pub fn find(
        &self,
        family: ObjectKind,
        subtype: i32,
        terrain: Terrain,
    ) -> Option<PlacementRuleId> {
        self.rules
            .iter()
            .rposition(|rule| {
                rule.object == family && rule.subtype == subtype && rule.terrain == terrain
            })
            .map(PlacementRuleId)
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
            .get((rule.0 * 2 + matrix) * self.columns + column)
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
    fn kind(value: u32) -> ObjectKind {
        ObjectKind::parse(i32::try_from(value).unwrap()).unwrap()
    }

    #[test]
    fn hotfix_skips_invalid_identity_but_keeps_original_score_columns() {
        let mut skipped = row(raw::ADVENTURE_OBJECT_TRAIT_COUNT, 1, 3);
        skipped[raw::RMG_PLACEMENT_COLUMN_TERRAIN_SCORES as usize] = "999999999999999".into();
        let bytes = sheet(&[row(raw::TOWN, 0, 3), skipped, row(raw::MONSTER, 2, 3)]);
        let rules = PlacementRules::parse(&bytes, Behavior::Hotfix).unwrap();
        assert_eq!(rules.source_rows(), 3);
        assert_eq!(rules.rules().len(), 2);
        let town = rules.find(kind(raw::TOWN), 0, Terrain::Dirt).unwrap();
        let monster = rules.find(kind(raw::MONSTER), 0, Terrain::Dirt).unwrap();
        assert_eq!(rules.get(monster).unwrap().source_row(), 2);
        assert_eq!(
            rules.neighbour_score(town, monster, NeighbourScore::Adjacent),
            Some(2)
        );
        assert_eq!(
            rules.neighbour_score(monster, town, NeighbourScore::Blocked),
            Some(1200)
        );
        assert_eq!(rules.get(town).unwrap().terrain_score(Terrain::Water), 9);
        assert_eq!(
            rules.get(town).unwrap().terrain_score(Terrain::Rock),
            raw::RMG_PLACEMENT_INVALID
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
            let selected = rules.find(kind(raw::TOWN), 0, Terrain::Dirt).unwrap();
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
        let town = rules.find(kind(raw::TOWN), 0, Terrain::Dirt).unwrap();
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
