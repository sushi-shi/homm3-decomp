//! Installed object recipes, separate from admitted per-run definitions.

use crate::parse;
use homm3_resource::hdat::Container;
use std::{error::Error, fmt};

/// A resource recipe before catalog-aware object/subtype admission.
/// Signed values, including native inheritance/unlimited sentinels, survive
/// parsing; the definition builder owns their interpretation.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ObjectRecipe {
    /// Native object family ID.
    pub object_type: i32,
    /// Native object subtype.
    pub subtype: i32,
    /// Treasure value.
    pub value: i32,
    /// Selection density.
    pub density: i32,
    /// Per-map placement limit, including -1 for unlimited.
    pub maximum_per_map: i32,
    /// Per-zone placement limit, including -1 for unlimited.
    pub maximum_per_zone: i32,
}

/// Integer overflow in an installed object recipe stream.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct RecipeDataError {
    /// Absolute offset of the containing HDAT record.
    pub record_offset: usize,
    /// Byte offset of the token within its string field.
    pub token_offset: usize,
}
impl fmt::Display for RecipeDataError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "object recipe integer overflow in HDAT record {} at token {}",
            self.record_offset, self.token_offset
        )
    }
}
impl Error for RecipeDataError {}

/// Additional definitions and template-enablable defaults from installed data.
/// These immutable lists can be shared by independent generation workspaces.
#[derive(Clone, Debug, Default)]
pub struct ObjectRecipes {
    additional: Vec<ObjectRecipe>,
    defaults: Vec<ObjectRecipe>,
}
impl ObjectRecipes {
    /// Load `rmgobjects` records using the native loader's name and field rules.
    /// Matching is case-insensitive for the first ten bytes; the last byte is
    /// parsed as the list index. Field 7 is clamped to the final string. Later
    /// matching records replace earlier lists, including with an empty list.
    /// A trailing incomplete six-integer record is ignored, as in the native
    /// definition loops (DLL RVAs 0x1b97b0 and 0x1c2260).
    ///
    /// # Errors
    /// Rejects integer prefix overflow, with source record and token offsets.
    pub fn parse(data: Container<'_>) -> Result<Self, RecipeDataError> {
        let mut result = Self::default();
        for entry in data.entries() {
            let name = entry.name();
            if !name
                .get(..10)
                .is_some_and(|s| s.eq_ignore_ascii_case(b"rmgobjects"))
            {
                continue;
            }
            // Names have at least ten bytes here; a nonnumeric suffix means 0.
            let index = parse::integer(name[name.len() - 1..].iter().copied());
            let target = match index {
                Some(0) => &mut result.additional,
                Some(1) => &mut result.defaults,
                _ => continue,
            };
            let strings = entry.strings();
            let Some(field) = strings.clone().nth(strings.len().saturating_sub(1).min(7)) else {
                continue;
            };
            let values = parse::integer_stream(field).map_err(|token_offset| RecipeDataError {
                record_offset: entry.offset(),
                token_offset,
            })?;
            target.clear();
            target.extend(values.chunks_exact(6).map(|v| ObjectRecipe {
                object_type: v[0],
                subtype: v[1],
                value: v[2],
                density: v[3],
                maximum_per_map: v[4],
                maximum_per_zone: v[5],
            }));
        }
        Ok(result)
    }
    /// Definitions appended during generator construction, in resource order.
    #[must_use]
    pub fn additional(&self) -> &[ObjectRecipe] {
        &self.additional
    }
    /// Optional definitions used for inheritance and explicit template enables.
    #[must_use]
    pub fn defaults(&self) -> &[ObjectRecipe] {
        &self.defaults
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn container(entries: &[(&[u8], &[&[u8]])]) -> Vec<u8> {
        fn word(bytes: &mut Vec<u8>, n: usize) {
            bytes.extend(i32::try_from(n).unwrap().to_le_bytes());
        }
        fn string(bytes: &mut Vec<u8>, s: &[u8]) {
            word(bytes, s.len());
            bytes.extend(s);
        }
        let mut bytes = b"HDAT\x02\0\0\0".to_vec();
        word(&mut bytes, entries.len());
        for (name, strings) in entries {
            string(&mut bytes, name);
            string(&mut bytes, b"");
            word(&mut bytes, strings.len());
            for s in *strings {
                string(&mut bytes, s);
            }
            bytes.push(0);
            word(&mut bytes, 0);
        }
        bytes
    }

    #[test]
    fn field_clamping_replacement_and_suffix_quirks_follow_loader() {
        let bytes = container(&[
            (b"rmgobjects0", &[b"99 0 100 10 -1 1"]),
            (b"RMGOBJECTSgarbage", &[b"213 0 100 50 -1 -1 7 8"]),
            (b"rmgobjects1", &[b"145 0 5000 200 -1 -1"]),
            (b"rmgobjects2", &[b"10 0 1 1 1 1"]),
            (b"rmgobjects0", &[]), // No strings preserves the earlier list.
        ]);
        let recipes = ObjectRecipes::parse(Container::parse(&bytes).unwrap()).unwrap();
        assert_eq!(recipes.additional().len(), 1);
        assert_eq!(
            recipes.additional()[0],
            ObjectRecipe {
                object_type: 213,
                subtype: 0,
                value: 100,
                density: 50,
                maximum_per_map: -1,
                maximum_per_zone: -1
            }
        );
        assert_eq!(recipes.defaults()[0].object_type, 145);
        let bytes = container(&[
            (b"rmgobjects0", &[b"10 0 1 1 1 1"]),
            (b"rmgobjects0", &[b""]),
        ]);
        assert!(ObjectRecipes::parse(Container::parse(&bytes).unwrap())
            .unwrap()
            .additional()
            .is_empty());
    }

    #[test]
    fn field_seven_wins_and_errors_locate_its_integer_token() {
        let bytes = container(&[(
            b"rmgobjects0",
            &[
                b"wrong",
                b"",
                b"",
                b"",
                b"",
                b"",
                b"",
                b"88 3 7000 100 32 1",
                b"wrong",
            ],
        )]);
        let recipes = ObjectRecipes::parse(Container::parse(&bytes).unwrap()).unwrap();
        assert_eq!(recipes.additional()[0].object_type, 88);
        let bytes = container(&[(b"rmgobjects1", &[b"1 0 2147483648"])]);
        assert_eq!(
            ObjectRecipes::parse(Container::parse(&bytes).unwrap()).unwrap_err(),
            RecipeDataError {
                record_offset: 12,
                token_offset: 4
            }
        );
    }
}
