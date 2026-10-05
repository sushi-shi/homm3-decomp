//! Immutable installed terrain-frame metadata.

use super::{frame_count_index, FrameRange, Terrain, TerrainRuleError, RANGES, RANGE_COUNT};
use crate::{parse, raw, rules::Ruleset};
use homm3_resource::hdat::Container;
use std::{error::Error, fmt, num::NonZeroU32, sync::Arc};

/// Number of eight-byte pattern entries at pinned DLL RVA `0x25f860`.
pub const HOTA_PATTERN_COUNT: usize = 124;

/// An installed terrain record or frame table is invalid.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TerrainDataError {
    /// The supplied table must contain exactly 124 little-endian word pairs.
    PatternLength(usize),
    /// A pattern has an invalid shape, special tag or noncontiguous range.
    Pattern(usize),
    /// An HDAT terrain record has an unsupported ID or missing probability.
    Record(usize),
    /// One of the expanded terrain IDs has no installed record.
    Missing(usize),
}
impl fmt::Display for TerrainDataError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "terrain data: {self:?}")
    }
}
impl Error for TerrainDataError {}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub(super) struct FrameInfo {
    pub shape: u32,
    pub special: bool,
}

#[derive(Debug)]
struct AdditionalFrames {
    entries: [FrameInfo; HOTA_PATTERN_COUNT],
    ranges: [Option<FrameRange>; RANGE_COUNT],
    chance: [u32; 2],
}

/// Shared immutable frame data and versioned frame-selection policy.
/// Complete borrows canonical tables; clones share installed expansion data.
#[derive(Clone, Debug, Default)]
pub struct TerrainCatalog {
    additional: Option<Arc<AdditionalFrames>>,
}
impl TerrainCatalog {
    /// Canonical Complete frame tables, with no allocation.
    #[must_use]
    pub const fn complete() -> Self {
        Self { additional: None }
    }

    /// Load the installed `terrainNN` records and the 124 pattern entries at
    /// pinned `HotA.dll` RVA `0x25f860` (each entry is two little-endian u32s).
    /// The caller supplies that data slice, not a native pointer or executable
    /// code. DLL/file identity belongs to the resource-loading boundary.
    /// Later HDAT records replace earlier records in file order.
    ///
    /// # Errors
    /// Rejects short/invalid patterns, disjoint ranges, unsupported record IDs
    /// and absent new terrains. No partial catalog is returned.
    pub fn parse_hota181(data: Container<'_>, patterns: &[u8]) -> Result<Self, TerrainDataError> {
        if patterns.len() != HOTA_PATTERN_COUNT * 8 {
            return Err(TerrainDataError::PatternLength(patterns.len()));
        }
        let mut entries = [FrameInfo::default(); HOTA_PATTERN_COUNT];
        let mut ranges: [Option<FrameRange>; RANGE_COUNT] = [None; RANGE_COUNT];
        for (index, bytes) in patterns.chunks_exact(8).enumerate() {
            let shape = u32::from_le_bytes([bytes[0], bytes[1], bytes[2], bytes[3]]);
            let special = u32::from_le_bytes([bytes[4], bytes[5], bytes[6], bytes[7]]);
            if shape >= raw::RMG_TERRAIN_SHAPE_COUNT || shape == 1 || special > 1 {
                return Err(TerrainDataError::Pattern(index));
            }
            let key = shape as usize * 4 + special as usize;
            let frame = u32::try_from(index).map_err(|_| TerrainDataError::Pattern(index))?;
            ranges[key] = Some(if let Some(previous) = ranges[key] {
                if previous.first() + previous.count().get() != frame {
                    return Err(TerrainDataError::Pattern(index));
                }
                FrameRange::new(
                    previous.first(),
                    NonZeroU32::new(previous.count().get() + 1)
                        .ok_or(TerrainDataError::Pattern(index))?,
                )
            } else {
                FrameRange::new(frame, NonZeroU32::MIN)
            });
            entries[index] = FrameInfo {
                shape,
                special: special != 0,
            };
        }
        let mut chance = [None; 2];
        for record in data.entries() {
            let name = record.name();
            if name.len() < 7 || !name[..7].eq_ignore_ascii_case(b"terrain") {
                continue;
            }
            let id = parse::integer(name[7..].iter().copied()).unwrap_or(0);
            let target = usize::try_from(id)
                .ok()
                .and_then(|id| id.checked_sub(10))
                .and_then(|index| chance.get_mut(index))
                .ok_or(TerrainDataError::Record(record.offset()))?;
            let mut values = record.integers();
            let index = 1.min(values.len().saturating_sub(1));
            let probability = values
                .nth(index)
                .ok_or(TerrainDataError::Record(record.offset()))?;
            // RVA 0x1f20c0 passes this signed dword to the native unsigned
            // probability calculation. Preserve its bits, including negatives.
            *target = Some(u32::from_ne_bytes(probability.to_ne_bytes()));
        }
        Ok(Self {
            additional: Some(Arc::new(AdditionalFrames {
                entries,
                ranges,
                chance: [
                    chance[0].ok_or(TerrainDataError::Missing(10))?,
                    chance[1].ok_or(TerrainDataError::Missing(11))?,
                ],
            })),
        })
    }

    /// Generation policy belonging to this catalog.
    #[must_use]
    pub fn ruleset(&self) -> Ruleset {
        if self.additional.is_some() {
            Ruleset::HotA181
        } else {
            Ruleset::Complete
        }
    }

    /// Number of frames admitted for a terrain.
    #[must_use]
    pub fn frame_count(&self, terrain: Terrain) -> Option<usize> {
        if terrain.index() < raw::TERRAIN_RULES.len() {
            Some(frame_count_index(terrain.index()))
        } else {
            self.additional.as_ref().map(|data| data.entries.len())
        }
    }

    pub(super) fn info(&self, terrain: Terrain, frame: u8) -> Result<FrameInfo, TerrainRuleError> {
        let count = self
            .frame_count(terrain)
            .ok_or(TerrainRuleError::UnavailableTerrain(terrain))?;
        if usize::from(frame) >= count {
            return Err(TerrainRuleError::Frame { terrain, frame });
        }
        if let Some(additional) = self.additional.as_ref().filter(|_| terrain.index() >= 10) {
            return Ok(additional.entries[usize::from(frame)]);
        }
        Ok(match raw::TERRAIN_RULES[terrain.index()] {
            raw::TerrainRuleData::Pattern { entries, .. } => FrameInfo {
                shape: entries[usize::from(frame)].m_transition,
                special: entries[usize::from(frame)].m_special != 0,
            },
            raw::TerrainRuleData::Fixed => FrameInfo {
                shape: raw::ROCK_FRAMES[usize::from(frame)].m_transition,
                special: false,
            },
        })
    }

    pub(super) fn range(
        &self,
        terrain: Terrain,
        key: usize,
    ) -> Result<Option<FrameRange>, TerrainRuleError> {
        if terrain.index() < raw::TERRAIN_RULES.len() {
            return Ok(RANGES[terrain.index()][key]);
        }
        self.additional
            .as_ref()
            .map(|data| data.ranges[key])
            .ok_or(TerrainRuleError::UnavailableTerrain(terrain))
    }
    pub(super) fn chance(&self, terrain: Terrain) -> Result<Option<u32>, TerrainRuleError> {
        if terrain.index() >= 10 {
            return self
                .additional
                .as_ref()
                .map(|data| Some(data.chance[terrain.index() - 10]))
                .ok_or(TerrainRuleError::UnavailableTerrain(terrain));
        }
        Ok(match raw::TERRAIN_RULES[terrain.index()] {
            raw::TerrainRuleData::Pattern { chance, .. } => Some(chance),
            raw::TerrainRuleData::Fixed => None,
        })
    }
}
