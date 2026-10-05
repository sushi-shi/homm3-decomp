//! Versioned generation rules, distinct from map encoding and bug-fix policy.

use crate::raw;

/// Native generation rules to reproduce. A ruleset describes algorithm choices;
/// immutable catalogs separately hold the actual game data.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Ruleset {
    /// English Complete 4.0. [`crate::behavior::Behavior`] chooses retail or hotfix.
    #[default]
    Complete,
    /// `HotA` 1.8.1, pinned to DLL SHA-256
    /// `1ed72766595ce5be50b0022fb42ed0369ec65ce0c5f2ef69bfcd76d3b6ceacc6`.
    HotA181,
}

impl Ruleset {
    /// Creature trait rows, including war machines and unused entries.
    #[must_use]
    pub const fn creature_count(self) -> usize {
        match self {
            Self::Complete => raw::CREATURE_FACTIONS_AND_LEVELS.len(),
            // Pinned initialization count, DLL VA 0x1064672c.
            Self::HotA181 => 200,
        }
    }
    /// Town domain expected from the installed catalog for this release.
    #[must_use]
    pub const fn town_count(self) -> usize {
        match self {
            Self::Complete => raw::TOWN_TYPE_COUNT as usize,
            // HotA game-data initialization, DLL VA 0x100ef33c.
            Self::HotA181 => 12,
        }
    }

    /// Terrain domain, including water and rock.
    #[must_use]
    pub const fn terrain_count(self) -> usize {
        match self {
            Self::Complete => raw::eTerrainRock as usize + 1,
            // HotA's initialized game-data cell, DLL VA 0x1025f830.
            Self::HotA181 => 12,
        }
    }

    /// Hero domain, including entries disabled for random generation.
    #[must_use]
    pub const fn hero_count(self) -> usize {
        match self {
            Self::Complete => raw::RMG_HERO_COUNT as usize,
            // Observed after initialization by the pinned oracle; VA 0x106463c0.
            Self::HotA181 => 215,
        }
    }
}
