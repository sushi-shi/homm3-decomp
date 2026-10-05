//! Spatial domains shared by layout, painting and object placement.

use crate::{geometry::Point, raw};

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
