//! Spatial domains shared by layout, painting and object placement.

use crate::{geometry::Point, raw, rng::RetailRng};
use std::{fmt, marker::PhantomData, num::NonZeroU32};

/// A finite domain whose values have dense native ordinals.
///
/// Native code stores such domains as flag arrays and recovers values from
/// array positions; `FlagSet` keeps the element type instead.
pub trait Ordinal: Copy + Eq + 'static {
    /// Every value in ordinal order, so `ALL[value.index()] == value`.
    const ALL: &'static [Self];
    /// Dense native ordinal.
    fn index(self) -> usize;
}

/// A native flag array over a small ordinal domain, kept as a typed bit set.
#[derive(Clone, Copy, PartialEq, Eq)]
pub struct FlagSet<T> {
    bits: u16,
    domain: PhantomData<T>,
}
impl<T: Ordinal> FlagSet<T> {
    /// No members, so selection consumes no random draw.
    pub const NONE: Self = Self::from_bits(0);
    /// Every value in the domain.
    pub const ALL: Self = {
        assert!(T::ALL.len() < u16::BITS as usize);
        Self::from_bits((1 << T::ALL.len()) - 1)
    };

    const fn from_bits(bits: u16) -> Self {
        Self {
            bits,
            domain: PhantomData,
        }
    }
    /// Admit only bits corresponding to values in the domain.
    #[must_use]
    pub const fn parse(bits: u16) -> Option<Self> {
        if bits & !Self::ALL.bits == 0 {
            Some(Self::from_bits(bits))
        } else {
            None
        }
    }
    /// Collect members from a per-value predicate, in ordinal order.
    pub fn from_fn(mut member: impl FnMut(T) -> bool) -> Self {
        let mut set = Self::NONE;
        for &value in T::ALL {
            if member(value) {
                set.insert(value);
            }
        }
        set
    }
    /// Bit `value.index()` records whether that value is a member.
    #[must_use]
    pub const fn bits(self) -> u16 {
        self.bits
    }
    /// Number of members.
    #[must_use]
    pub const fn count(self) -> u32 {
        self.bits.count_ones()
    }
    /// Whether no value is a member.
    #[must_use]
    pub const fn is_empty(self) -> bool {
        self.bits == 0
    }
    /// Whether this value is a member.
    #[must_use]
    pub fn contains(self, value: T) -> bool {
        self.bits & (1 << value.index()) != 0
    }
    /// Add a member.
    pub fn insert(&mut self, value: T) {
        self.bits |= 1 << value.index();
    }
    /// Remove a member.
    pub fn remove(&mut self, value: T) {
        self.bits &= !(1 << value.index());
    }
    /// Members in ordinal order.
    pub fn iter(self) -> impl Iterator<Item = T> {
        T::ALL
            .iter()
            .copied()
            .filter(move |&value| self.contains(value))
    }
    /// Native flag-array selection: count the set flags, draw `rand() % count`
    /// and take that member in ordinal order. An empty set returns `None`
    /// without drawing; a singleton still consumes its draw.
    pub fn choose(self, rng: &mut RetailRng) -> Option<T> {
        let count = NonZeroU32::new(self.count())?;
        self.iter().nth(rng.below(count) as usize)
    }
}
impl<T: Ordinal + fmt::Debug> fmt::Debug for FlagSet<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_set().entries(self.iter()).finish()
    }
}

/// A world plane. A request separately determines whether underground exists.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Level {
    /// Above ground.
    Surface,
    /// Below ground.
    Underground,
}
impl Level {
    /// Original plane ordinal.
    #[must_use]
    pub const fn index(self) -> usize {
        match self {
            Self::Surface => raw::RMG_SURFACE_LEVEL as usize,
            Self::Underground => raw::RMG_UNDERGROUND_LEVEL as usize,
        }
    }
    /// The other plane of a two-level map.
    #[must_use]
    pub const fn other(self) -> Self {
        match self {
            Self::Surface => Self::Underground,
            Self::Underground => Self::Surface,
        }
    }
}

/// Signed position during layout, which may lie outside the eventual map.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct WorldPosition {
    /// Signed horizontal position, with Y growing south.
    pub point: Point,
    /// World plane, considered separately from planar distance.
    pub level: Level,
}

/// Flat cell order shared by map and group grids: `(level * side + y) * side + x`.
///
/// The one place that converts between flat indices and signed coordinates;
/// supported sides fit a byte, so every conversion is exact.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) struct CellLayout {
    side: u8,
}
impl CellLayout {
    pub(crate) const fn new(side: u8) -> Self {
        Self { side }
    }
    #[allow(clippy::cast_possible_truncation)] // asserted below
    pub(crate) const fn map(size: crate::request::MapSize) -> Self {
        const { assert!(raw::MAP_DIMENSION_EXTRA_LARGE <= u8::MAX as u32) };
        Self::new(size.dimension() as u8)
    }
    /// Cells per row.
    pub(crate) const fn side(self) -> usize {
        self.side as usize
    }
    /// Cells per row, for signed coordinate arithmetic.
    pub(crate) fn signed_side(self) -> i32 {
        i32::from(self.side)
    }
    /// Cells per plane.
    pub(crate) const fn plane(self) -> usize {
        self.side() * self.side()
    }
    /// Index of a point inside one square plane, or none outside it.
    pub(crate) fn plane_index(self, point: Point) -> Option<usize> {
        let x = usize::try_from(point.x).ok().filter(|&x| x < self.side())?;
        let y = usize::try_from(point.y).ok().filter(|&y| y < self.side())?;
        Some(y * self.side() + x)
    }
    /// Index of an in-square position on its plane, or none outside the square.
    pub(crate) fn index(self, position: WorldPosition) -> Option<usize> {
        Some(position.level.index() * self.plane() + self.plane_index(position.point)?)
    }
    /// Plane coordinates of a flat index.
    #[allow(clippy::cast_possible_truncation, clippy::cast_possible_wrap)]
    pub(crate) const fn point(self, index: usize) -> Point {
        // Both remainders are below `side`, which fits a byte.
        Point::new(
            (index % self.side()) as i32,
            (index / self.side() % self.side()) as i32,
        )
    }
    /// Position of a flat index; any index past the first plane is underground.
    pub(crate) const fn position(self, index: usize) -> WorldPosition {
        WorldPosition {
            point: self.point(index),
            level: if index < self.plane() {
                Level::Surface
            } else {
                Level::Underground
            },
        }
    }
}

/// Complete-era terrain values, excluding unchecked wire discriminants.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(i32)]
pub enum Terrain {
    /// Dirt.
    Dirt = raw::eTerrainDirt,
    /// Sand.
    Sand = raw::eTerrainSand,
    /// Grass.
    Grass = raw::eTerrainGrass,
    /// Snow.
    Snow = raw::eTerrainSnow,
    /// Swamp.
    Swamp = raw::eTerrainSwamp,
    /// Rough.
    Rough = raw::eTerrainRough,
    /// Subterranean.
    Subterranean = raw::eTerrainSubterranean,
    /// Lava.
    Lava = raw::eTerrainLava,
    /// Water.
    Water = raw::eTerrainWater,
    /// Impassable underground rock.
    Rock = raw::eTerrainRock,
}
impl Terrain {
    /// Parse a terrain discriminant without admitting unknown values.
    #[must_use]
    pub const fn parse(value: i32) -> Option<Self> {
        match value {
            raw::eTerrainDirt => Some(Self::Dirt),
            raw::eTerrainSand => Some(Self::Sand),
            raw::eTerrainGrass => Some(Self::Grass),
            raw::eTerrainSnow => Some(Self::Snow),
            raw::eTerrainSwamp => Some(Self::Swamp),
            raw::eTerrainRough => Some(Self::Rough),
            raw::eTerrainSubterranean => Some(Self::Subterranean),
            raw::eTerrainLava => Some(Self::Lava),
            raw::eTerrainWater => Some(Self::Water),
            raw::eTerrainRock => Some(Self::Rock),
            _ => None,
        }
    }
    /// Index into terrain tables and the wire discriminant.
    #[must_use]
    pub const fn index(self) -> usize {
        self as usize
    }
    /// Every terrain in table order.
    pub const ALL: [Self; raw::RMG_TERRAIN_COUNT as usize] = [
        Self::Dirt,
        Self::Sand,
        Self::Grass,
        Self::Snow,
        Self::Swamp,
        Self::Rough,
        Self::Subterranean,
        Self::Lava,
        Self::Water,
        Self::Rock,
    ];
}
impl Ordinal for Terrain {
    const ALL: &'static [Self] = &Self::ALL;
    fn index(self) -> usize {
        Self::index(self)
    }
}

/// A terrain a template zone may request; water and rock are never zone choices.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(i32)]
pub enum LandTerrain {
    /// Dirt, also the fallback when a template allows no terrain.
    Dirt = raw::eTerrainDirt,
    /// Sand.
    Sand = raw::eTerrainSand,
    /// Grass.
    Grass = raw::eTerrainGrass,
    /// Snow.
    Snow = raw::eTerrainSnow,
    /// Swamp.
    Swamp = raw::eTerrainSwamp,
    /// Rough.
    Rough = raw::eTerrainRough,
    /// Subterranean; surface zones exclude it before choosing.
    Subterranean = raw::eTerrainSubterranean,
    /// Lava.
    Lava = raw::eTerrainLava,
}
impl LandTerrain {
    /// Template allowed-terrain columns, in source order.
    pub const ALL: [Self; raw::eTerrainWater as usize] = [
        Self::Dirt,
        Self::Sand,
        Self::Grass,
        Self::Snow,
        Self::Swamp,
        Self::Rough,
        Self::Subterranean,
        Self::Lava,
    ];
    /// The painted terrain.
    #[must_use]
    pub const fn terrain(self) -> Terrain {
        match self {
            Self::Dirt => Terrain::Dirt,
            Self::Sand => Terrain::Sand,
            Self::Grass => Terrain::Grass,
            Self::Snow => Terrain::Snow,
            Self::Swamp => Terrain::Swamp,
            Self::Rough => Terrain::Rough,
            Self::Subterranean => Terrain::Subterranean,
            Self::Lava => Terrain::Lava,
        }
    }
}
impl Ordinal for LandTerrain {
    const ALL: &'static [Self] = &Self::ALL;
    fn index(self) -> usize {
        self as usize
    }
}
impl From<LandTerrain> for Terrain {
    fn from(terrain: LandTerrain) -> Self {
        terrain.terrain()
    }
}

/// The seven primary resources, excluding reward/dialog-only discriminants.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(i32)]
pub enum Resource {
    /// Wood.
    Wood = raw::WOOD,
    /// Mercury.
    Mercury = raw::MERCURY,
    /// Ore.
    Ore = raw::ORE,
    /// Sulfur.
    Sulfur = raw::SULFUR,
    /// Crystal.
    Crystal = raw::CRYSTAL,
    /// Gems.
    Gems = raw::GEMS,
    /// Gold.
    Gold = raw::GOLD,
}
impl Resource {
    /// Native resource iteration and cost-column order.
    pub const ALL: [Self; raw::NUM_RESOURCES as usize] = [
        Self::Wood,
        Self::Mercury,
        Self::Ore,
        Self::Sulfur,
        Self::Crystal,
        Self::Gems,
        Self::Gold,
    ];
    /// Admit a primary-resource discriminant.
    #[must_use]
    pub const fn parse(value: i32) -> Option<Self> {
        match value {
            raw::WOOD => Some(Self::Wood),
            raw::MERCURY => Some(Self::Mercury),
            raw::ORE => Some(Self::Ore),
            raw::SULFUR => Some(Self::Sulfur),
            raw::CRYSTAL => Some(Self::Crystal),
            raw::GEMS => Some(Self::Gems),
            raw::GOLD => Some(Self::Gold),
            _ => None,
        }
    }
    /// Index into a resource row.
    #[must_use]
    pub const fn index(self) -> usize {
        self as usize
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn dense<T: Ordinal + fmt::Debug>() {
        for (index, &value) in T::ALL.iter().enumerate() {
            assert_eq!(value.index(), index, "{value:?}");
        }
    }

    #[test]
    fn ordinal_domains_are_dense_and_land_terrain_matches_wire_values() {
        dense::<Terrain>();
        dense::<LandTerrain>();
        dense::<crate::request::Town>();
        dense::<crate::template::GuardAffinity>();
        for (index, town) in crate::request::Town::ALL.into_iter().enumerate() {
            assert_eq!(
                Ok(town),
                crate::request::Town::parse(i32::try_from(index).unwrap())
            );
        }
        for land in LandTerrain::ALL {
            assert_eq!(land.terrain().index(), land.index());
        }
    }

    #[test]
    fn cell_layout_round_trips_both_planes() {
        let layout = CellLayout::new(3);
        for index in 0..2 * layout.plane() {
            let position = layout.position(index);
            assert_eq!(layout.index(position), Some(index));
            assert_eq!(
                position.level,
                if index < 9 {
                    Level::Surface
                } else {
                    Level::Underground
                }
            );
        }
        assert_eq!(layout.point(5), Point::new(2, 1));
        for outside in [Point::new(-1, 0), Point::new(3, 0), Point::new(0, 3)] {
            assert_eq!(layout.plane_index(outside), None);
        }
    }

    #[test]
    fn flag_set_selection_preserves_empty_singleton_and_rank_draws() {
        let mut rng = RetailRng::new(1);
        assert_eq!(FlagSet::<LandTerrain>::NONE.choose(&mut rng), None);
        assert_eq!(rng.draws(), 0);
        let mut set = FlagSet::NONE;
        set.insert(LandTerrain::Snow);
        set.insert(LandTerrain::Lava);
        // The first seed-one draw is 41: rank 1 of two members.
        assert_eq!(set.choose(&mut rng), Some(LandTerrain::Lava));
        assert_eq!(rng.draws(), 1);
        set.remove(LandTerrain::Lava);
        assert_eq!(set.choose(&mut rng), Some(LandTerrain::Snow));
        assert_eq!(rng.draws(), 2);
        assert!(FlagSet::<Terrain>::parse(1 << 10).is_none());
        assert_eq!(FlagSet::<Terrain>::ALL.count(), 10);
    }
}
