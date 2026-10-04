//! Bounded adventure-object identities and source-derived immutable traits.

use crate::raw;

const COUNT: usize = raw::ADVENTURE_OBJECT_TRAIT_COUNT as usize;
const _: () = assert!(raw::ADVENTURE_OBJECT_TRAIT_COUNT <= u16::MAX as u32);

/// An index into the Complete adventure-object domain, including unnamed IDs.
/// Object payloads are represented separately from this catalog identity.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct ObjectKind(u16);
impl ObjectKind {
    /// Artifact object kind.
    pub const ARTIFACT: Self = Self::from_source(raw::ARTIFACT);
    /// Border guard object kind.
    pub const BORDER_GUARD: Self = Self::from_source(raw::BORDER_GUARD);
    /// Border tent object kind.
    pub const BORDER_TENT: Self = Self::from_source(raw::BORDER_TENT);
    /// Cursed ground object kind.
    pub const CURSED_GROUND: Self = Self::from_source(raw::CURSED_GROUND);
    /// One-way monolith entrance.
    pub const LITH_ONEWAY_ENTRANCE: Self = Self::from_source(raw::LITH_ONEWAY_ENTRANCE);
    /// One-way monolith exit.
    pub const LITH_ONEWAY_EXIT: Self = Self::from_source(raw::LITH_ONEWAY_EXIT);
    /// Two-way monolith.
    pub const LITH_TWOWAY: Self = Self::from_source(raw::LITH_TWOWAY);
    /// Mine object kind.
    pub const MINE: Self = Self::from_source(raw::MINE);
    /// Monster object kind.
    pub const MONSTER: Self = Self::from_source(raw::MONSTER);
    /// Random artifact object kind.
    pub const RANDOM_ARTIFACT: Self = Self::from_source(raw::RANDOM_ARTIFACT);
    /// Random monster object kind.
    pub const RANDOM_MONSTER: Self = Self::from_source(raw::RANDOM_MONSTER);
    /// Resource object kind.
    pub const RESOURCE: Self = Self::from_source(raw::RESOURCE);
    /// Seer object kind.
    pub const SEER: Self = Self::from_source(raw::SEER);
    /// Shipyard object kind.
    pub const SHIPYARD: Self = Self::from_source(raw::SHIPYARD);
    /// Terrain hole object kind.
    pub const TERRAIN_HOLE: Self = Self::from_source(raw::TERRAIN_HOLE);
    /// Terrain river delta object kind.
    pub const TERRAIN_RIVER_DELTA: Self = Self::from_source(raw::TERRAIN_RIVER_DELTA);
    /// Town object kind.
    pub const TOWN: Self = Self::from_source(raw::TOWN);
    /// Underground gate object kind.
    pub const UNDERGROUND_GATE: Self = Self::from_source(raw::UNDERGROUND_GATE);

    // Named identities come from bindgen; fail compilation if their domain changes.
    #[expect(clippy::cast_possible_truncation, reason = "source domain fits u16")]
    const fn from_source(value: u32) -> Self {
        assert!(value < raw::ADVENTURE_OBJECT_TRAIT_COUNT);
        Self(value as u16)
    }

    /// Parse a raw object tag before it can index a trait or prototype table.
    #[must_use]
    #[expect(
        clippy::cast_possible_truncation,
        clippy::cast_sign_loss,
        reason = "the explicit bounds fit u16"
    )]
    pub const fn parse(value: i32) -> Option<Self> {
        if value >= 0 && (value as u32) < raw::ADVENTURE_OBJECT_TRAIT_COUNT {
            Some(Self(value as u16))
        } else {
            None
        }
    }
    /// Source table index and wire discriminant.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0 as usize
    }
    /// The alias used for prototype buckets and placement rules.
    #[must_use]
    pub const fn family(self) -> Self {
        TRAITS[self.index()].family
    }
    /// Immutable behavior flags initialized by the C++ trait loader.
    #[must_use]
    pub const fn traits(self) -> ObjectTraits {
        TRAITS[self.index()]
    }
}

/// Behavior initialized from the five canonical adventure-object tables.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[expect(
    clippy::struct_excessive_bools,
    reason = "four independent source-table flags, not exclusive states"
)]
pub struct ObjectTraits {
    family: ObjectKind,
    decoration: bool,
    cleared_on_visit: bool,
    blocks_landing: bool,
    enterable_from_north: bool,
}
impl ObjectTraits {
    /// Decorative obstacle, serialized ahead of other map objects.
    #[must_use]
    pub const fn is_decoration(self) -> bool {
        self.decoration
    }
    /// Visiting removes the object's blocking effect.
    #[must_use]
    pub const fn cleared_on_visit(self) -> bool {
        self.cleared_on_visit
    }
    /// The object vetoes landing at its trigger.
    #[must_use]
    pub const fn blocks_landing(self) -> bool {
        self.blocks_landing
    }
    /// The trigger admits entry and departure on its northern side.
    #[must_use]
    pub const fn enterable_from_north(self) -> bool {
        self.enterable_from_north
    }
}

const TRAITS: [ObjectTraits; COUNT] = build_traits();
const fn build_traits() -> [ObjectTraits; COUNT] {
    let mut traits = [ObjectTraits {
        family: ObjectKind(0),
        decoration: false,
        cleared_on_visit: false,
        blocks_landing: false,
        enterable_from_north: false,
    }; COUNT];
    let mut identity = 0_u16;
    while (identity as usize) < COUNT {
        traits[identity as usize].family = ObjectKind(identity);
        identity += 1;
    }
    let mut index = 0;
    while index < raw::OBJECT_NAME_ROWS.len() {
        let row = raw::OBJECT_NAME_ROWS[index];
        let Some(kind) = ObjectKind::parse(row.m_objectType) else {
            panic!("invalid source alias owner")
        };
        let Some(family) = ObjectKind::parse(row.m_nameRow) else {
            panic!("invalid source alias destination")
        };
        traits[kind.index()].family = family;
        index += 1;
    }
    index = 0;
    while index < raw::OBJECT_DECORATION_IDS.len() {
        traits[raw::OBJECT_DECORATION_IDS[index] as usize].decoration = true;
        index += 1;
    }
    index = 0;
    while index < raw::OBJECT_CLEARED_IDS.len() {
        traits[raw::OBJECT_CLEARED_IDS[index] as usize].cleared_on_visit = true;
        index += 1;
    }
    index = 0;
    while index < raw::OBJECT_LAND_BLOCKED_IDS.len() {
        traits[raw::OBJECT_LAND_BLOCKED_IDS[index] as usize].blocks_landing = true;
        index += 1;
    }
    index = 0;
    while index < raw::OBJECT_NORTH_IDS.len() {
        traits[raw::OBJECT_NORTH_IDS[index] as usize].enterable_from_north = true;
        index += 1;
    }
    traits
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn aliases_remain_distinct_objects_with_their_own_behavior_flags() {
        let original = ObjectKind::CURSED_GROUND;
        let alias_row = raw::OBJECT_NAME_ROWS
            .iter()
            .find(|row| row.m_nameRow == i32::try_from(original.index()).unwrap())
            .unwrap();
        let alias = ObjectKind::parse(alias_row.m_objectType).unwrap();
        assert_ne!(alias, original);
        assert_eq!(alias.family(), original);
        for row in raw::OBJECT_NAME_ROWS {
            let kind = ObjectKind::parse(row.m_objectType).unwrap();
            assert_eq!(
                kind.family().index(),
                usize::try_from(row.m_nameRow).unwrap()
            );
        }
        for index in 0..COUNT {
            let kind = ObjectKind::parse(i32::try_from(index).unwrap()).unwrap();
            assert_eq!(
                kind.traits().is_decoration(),
                raw::OBJECT_DECORATION_IDS.contains(&u32::try_from(index).unwrap())
            );
        }
        assert!(ObjectKind::parse(-1).is_none());
        assert!(ObjectKind::parse(i32::try_from(COUNT).unwrap()).is_none());
    }
}
