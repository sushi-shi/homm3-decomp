//! Terrain frame domains, ordered transition classification, and frame choice.
//!
//! Cell shapes are unreflected; sprite flips provide the other orientations.
//! `NwCorner` has north and west sides; `Se` has only that diagonal.
//! North is up; C is the cell, e an edge, . none, ? not fixed.
//! ```text
//!   N_W    W      N      SE
//!   ? e ?  ? . .  ? e ?  . . ?
//!   e C ?  e C ?  . C .  . C .
//!   ? ? .  ? . .  . ? .  ? . e
//! ```
//! Rock frames carry their reflection in the artwork, not sprite flags:
//! ```text
//!   none   flipX  flipY  both
//!   . e .  . e .  . . .  . . .
//!   e C .  . C e  e C .  . C e
//!   . . .  . . .  . e .  . e .
//! ```

use crate::{
    domain::Terrain,
    line::{FrameRange, Reflection},
    raw,
    rng::RetailRng,
};
use std::{error::Error, fmt, num::NonZeroU32};

/// Special-frame probability multiplier, bounded by the full brush strength.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct BrushStrength(u32);
impl BrushStrength {
    /// Strength used by map generation.
    pub const GENERATOR: Self = Self(raw::RMG_BRUSH_STRENGTH);
    /// Parse a multiplier from zero through full strength.
    #[must_use]
    pub const fn parse(value: u32) -> Option<Self> {
        if value <= raw::RMG_FULL_BRUSH_STRENGTH {
            Some(Self(value))
        } else {
            None
        }
    }
    /// Reduce the special-frame probability beside one decorated neighbour.
    #[must_use]
    pub const fn halved(self) -> Self {
        Self(self.0 / 2)
    }
}

/// Valid terrain shapes, excluding the unused ID 1 and count sentinel.
///
/// Each diagram shows the canonical orientation before [`Reflection`]. North is
/// up; `C` is the cell, `b` a blend edge, `h` a hard edge, `.` no edge, `?` an
/// unspecified neighbour, and `p` an additional terrain probe.
///
/// These describe shape families, not complete matcher predicates: [`classify`]
/// resolves overlapping rules in priority order and also recognizes offset
/// corners. The diagonal variants refine an already selected corner using
/// clamped terrain probes; the eight edge kinds alone do not select them.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum TerrainShape {
    /// No transition edges.
    ///
    /// ```text
    /// . . .
    /// . C .
    /// . . .
    /// ```
    Fill = raw::SHAPE_FILL,
    /// Blending north and west sides.
    ///
    /// ```text
    /// ? b ?
    /// b C ?
    /// ? ? ?
    /// ```
    NwCornerBlend = raw::SHAPE_N_W_BLEND,
    /// Blending west side.
    ///
    /// ```text
    /// ? ? ?
    /// b C ?
    /// ? ? ?
    /// ```
    WestBlend = raw::SHAPE_W_BLEND,
    /// Blending north side.
    ///
    /// ```text
    /// ? b ?
    /// ? C ?
    /// ? ? ?
    /// ```
    NorthBlend = raw::SHAPE_N_BLEND,
    /// Blending southeast diagonal.
    ///
    /// ```text
    /// ? . ?
    /// . C .
    /// ? . b
    /// ```
    SeBlend = raw::SHAPE_SE_BLEND,
    /// Blending northwest corner on a diagonal edge.
    ///
    /// At least one `p` (NE or SW) has the same terrain as `C`.
    /// ```text
    /// ? b p
    /// b C ?
    /// p ? ?
    /// ```
    NwDiagonalBlend = raw::SHAPE_N_W_DIAG_BLEND,
    /// Blending southeast inner corner on a diagonal edge.
    ///
    /// `C` is at the top left here. At least one `p`, two cells east or
    /// south, has terrain different from `C`.
    /// ```text
    /// C . p
    /// . b ?
    /// p ? ?
    /// ```
    SeDiagonalBlend = raw::SHAPE_SE_DIAG_BLEND,
    /// Hard north and west sides.
    ///
    /// ```text
    /// ? h ?
    /// h C ?
    /// ? ? ?
    /// ```
    NwCornerHard = raw::SHAPE_N_W_HARD,
    /// Hard west side.
    ///
    /// ```text
    /// ? ? ?
    /// h C ?
    /// ? ? ?
    /// ```
    WestHard = raw::SHAPE_W_HARD,
    /// Hard north side.
    ///
    /// ```text
    /// ? h ?
    /// ? C ?
    /// ? ? ?
    /// ```
    NorthHard = raw::SHAPE_N_HARD,
    /// Hard southeast diagonal.
    ///
    /// ```text
    /// ? . ?
    /// . C .
    /// ? . h
    /// ```
    SeHard = raw::SHAPE_SE_HARD,
    /// Hard northwest corner on a diagonal edge.
    ///
    /// At least one `p` (NE or SW) has the same terrain as `C`.
    /// ```text
    /// ? h p
    /// h C ?
    /// p ? ?
    /// ```
    NwDiagonalHard = raw::SHAPE_N_W_DIAG_HARD,
    /// Hard southeast inner corner on a diagonal edge.
    ///
    /// `C` is at the top left here. At least one `p`, two cells east or
    /// south, has terrain different from `C`.
    /// ```text
    /// C . p
    /// . h ?
    /// p ? ?
    /// ```
    SeDiagonalHard = raw::SHAPE_SE_DIAG_HARD,
    /// Opposite northwest and southeast blending diagonals.
    ///
    /// ```text
    /// b . ?
    /// . C .
    /// ? . b
    /// ```
    NwSeBlend = raw::SHAPE_NW_SE_BLEND,
    /// Northwest blend and southeast hard diagonals.
    ///
    /// ```text
    /// b . ?
    /// . C .
    /// ? . h
    /// ```
    NwBlendSeHard = raw::SHAPE_NW_BLEND_SE_HARD,
    /// Opposite northwest and southeast hard diagonals.
    ///
    /// ```text
    /// h . ?
    /// . C .
    /// ? . h
    /// ```
    NwSeHard = raw::SHAPE_NW_SE_HARD,
    /// East blend and southwest hard edges.
    ///
    /// ```text
    /// ? ? ?
    /// ? C b
    /// h ? ?
    /// ```
    EastBlendSwHard = raw::SHAPE_E_BLEND_SW_HARD,
    /// South blend and northeast hard edges.
    ///
    /// ```text
    /// ? ? h
    /// ? C ?
    /// ? b ?
    /// ```
    SouthBlendNeHard = raw::SHAPE_S_BLEND_NE_HARD,
    /// East blend and southeast hard edges.
    ///
    /// ```text
    /// ? ? ?
    /// ? C b
    /// ? ? h
    /// ```
    EastBlendSeHard = raw::SHAPE_E_BLEND_SE_HARD,
    /// South blend and southeast hard edges.
    ///
    /// ```text
    /// ? ? ?
    /// ? C ?
    /// ? b h
    /// ```
    SouthBlendSeHard = raw::SHAPE_S_BLEND_SE_HARD,
    /// East hard and southwest blend edges.
    ///
    /// Also covers a blending south side when SW is not hard.
    /// ```text
    /// ? ? ?
    /// ? C h
    /// b ? ?
    /// ```
    EastHardSwBlend = raw::SHAPE_E_HARD_SW_BLEND,
    /// South hard and northeast blend edges.
    ///
    /// Also covers a blending east side when NE is not hard.
    /// ```text
    /// ? ? b
    /// ? C ?
    /// ? h ?
    /// ```
    SouthHardNeBlend = raw::SHAPE_S_HARD_NE_BLEND,
    /// Blending northwest outer corner and southeast diagonal.
    ///
    /// ```text
    /// ? b ?
    /// b C ?
    /// ? ? b
    /// ```
    NwCornerSeBlend = raw::SHAPE_N_W_SE_BLEND,
    /// Hard northwest outer corner and southeast diagonal.
    ///
    /// ```text
    /// ? h ?
    /// h C ?
    /// ? ? h
    /// ```
    NwCornerSeHard = raw::SHAPE_N_W_SE_HARD,
    /// Blending northwest outer corner and hard southeast diagonal.
    ///
    /// ```text
    /// ? b ?
    /// b C ?
    /// ? ? h
    /// ```
    NwCornerBlendSeHard = raw::SHAPE_N_W_BLEND_SE_HARD,
    /// Hard northwest outer corner and blending southeast diagonal.
    ///
    /// ```text
    /// ? h ?
    /// h C ?
    /// ? ? b
    /// ```
    NwCornerHardSeBlend = raw::SHAPE_N_W_HARD_SE_BLEND,
    /// East/south blending sides with a hard southeast diagonal.
    ///
    /// ```text
    /// ? ? ?
    /// ? C b
    /// ? b h
    /// ```
    EastSouthBlendSeHard = raw::SHAPE_E_S_BLEND_SE_HARD,
    /// East/south blending sides with hard northeast/southwest diagonals.
    ///
    /// ```text
    /// ? ? h
    /// ? C b
    /// h b ?
    /// ```
    EastSouthBlendNeSwHard = raw::SHAPE_E_S_BLEND_NE_SW_HARD,
}

/// Relationship of one neighbour to the centre terrain.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum TerrainEdge {
    /// No visible transition edge.
    None = raw::RMG_NEIGHBOUR_NO_EDGE,
    /// Blend between compatible terrains.
    Blend = raw::RMG_NEIGHBOUR_BLEND_EDGE,
    /// Hard transition.
    Hard = raw::RMG_NEIGHBOUR_HARD_EDGE,
}

/// Classified shape and its sprite reflection before fixed-table adjustment.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TerrainTransition {
    /// Canonical shape.
    pub shape: TerrainShape,
    /// Orthogonal reflection.
    pub reflection: Reflection,
}

/// Terrain plus a frame known to belong to that terrain's source table.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TerrainTile {
    terrain: Terrain,
    frame: u8,
    reflection: Reflection,
}
impl TerrainTile {
    /// Parse an existing tile's frame against its terrain table.
    ///
    /// # Errors
    /// Rejects frames outside the canonical table.
    pub fn parse(
        terrain: Terrain,
        frame: u8,
        reflection: Reflection,
    ) -> Result<Self, TerrainRuleError> {
        if usize::from(frame) >= frame_count(terrain) {
            return Err(TerrainRuleError::Frame { terrain, frame });
        }
        Ok(Self {
            terrain,
            frame,
            reflection,
        })
    }
    /// Terrain type.
    #[must_use]
    pub const fn terrain(self) -> Terrain {
        self.terrain
    }
    /// Frame valid for this tile's terrain.
    #[must_use]
    pub const fn frame(self) -> u8 {
        self.frame
    }
    /// Sprite reflection; rock uses reflected art and clears these flags.
    #[must_use]
    pub const fn reflection(self) -> Reflection {
        self.reflection
    }
    /// Whether the source frame is decorated.
    #[must_use]
    pub fn is_special(self) -> bool {
        match raw::TERRAIN_RULES[self.terrain.index()] {
            raw::TerrainRuleData::Pattern { entries, .. } => {
                entries[usize::from(self.frame)].m_special != 0
            }
            raw::TerrainRuleData::Fixed => false,
        }
    }
    fn shape(self) -> u32 {
        match raw::TERRAIN_RULES[self.terrain.index()] {
            raw::TerrainRuleData::Pattern { entries, .. } => {
                entries[usize::from(self.frame)].m_transition
            }
            raw::TerrainRuleData::Fixed => raw::ROCK_FRAMES[usize::from(self.frame)].m_transition,
        }
    }
}

/// Invalid external frame or unsupported terrain/shape combination.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TerrainRuleError {
    /// A frame cannot belong to this terrain.
    Frame {
        /// Supplied terrain.
        terrain: Terrain,
        /// Supplied frame.
        frame: u8,
    },
    /// The original rule would select from an empty frame range.
    MissingRange {
        /// Terrain being painted.
        terrain: Terrain,
        /// Requested transition.
        transition: TerrainTransition,
    },
}
impl fmt::Display for TerrainRuleError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Frame { terrain, frame } => {
                write!(f, "frame {frame} is outside {terrain:?}'s table")
            }
            Self::MissingRange {
                terrain,
                transition,
            } => write!(f, "no {terrain:?} frames for {transition:?}"),
        }
    }
}
impl Error for TerrainRuleError {}

const RANGE_COUNT: usize = raw::RMG_TERRAIN_SHAPE_COUNT as usize * 4;
const RANGES: [[Option<FrameRange>; RANGE_COUNT]; raw::TERRAIN_RULES.len()] = build_ranges();

#[expect(
    clippy::cast_possible_truncation,
    reason = "the source frame count is asserted to fit u8 before iteration"
)]
const fn build_ranges() -> [[Option<FrameRange>; RANGE_COUNT]; raw::TERRAIN_RULES.len()] {
    let mut result: [[Option<FrameRange>; RANGE_COUNT]; raw::TERRAIN_RULES.len()] =
        [[None; RANGE_COUNT]; raw::TERRAIN_RULES.len()];
    let mut terrain = 0;
    while terrain < result.len() {
        let count = frame_count_index(terrain);
        assert!(
            count <= u8::MAX as usize + 1,
            "terrain frames must fit the tile record"
        );
        let mut frame = 0;
        while frame < count {
            let key = match raw::TERRAIN_RULES[terrain] {
                raw::TerrainRuleData::Pattern { entries, .. } => {
                    entries[frame].m_transition as usize * 4 + entries[frame].m_special as usize
                }
                raw::TerrainRuleData::Fixed => {
                    let entry = raw::ROCK_FRAMES[frame];
                    entry.m_transition as usize * 4
                        + entry.m_flipX as usize * 2
                        + entry.m_flipY as usize
                }
            };
            result[terrain][key] = Some(match result[terrain][key] {
                None => FrameRange::new(frame as u32, NonZeroU32::MIN),
                Some(previous) => {
                    assert!(
                        previous.first() as usize + previous.count().get() as usize == frame,
                        "terrain frame ranges must be contiguous"
                    );
                    let Some(count) = NonZeroU32::new(previous.count().get() + 1) else {
                        panic!("frame range overflow");
                    };
                    FrameRange::new(previous.first(), count)
                }
            });
            frame += 1;
        }
        terrain += 1;
    }
    result
}
const fn frame_count_index(index: usize) -> usize {
    match raw::TERRAIN_RULES[index] {
        raw::TerrainRuleData::Pattern { entries, .. } => entries.len(),
        raw::TerrainRuleData::Fixed => raw::ROCK_FRAMES.len(),
    }
}
const fn frame_count(terrain: Terrain) -> usize {
    frame_count_index(terrain.index())
}

/// Whether this terrain permits disconnected matching neighbour runs.
#[must_use]
pub fn allows_separated(terrain: Terrain) -> bool {
    matches!(
        raw::TERRAIN_RULES[terrain.index()],
        raw::TerrainRuleData::Pattern {
            separated: true,
            ..
        }
    )
}
fn blends(terrain: Terrain) -> bool {
    matches!(
        raw::TERRAIN_RULES[terrain.index()],
        raw::TerrainRuleData::Pattern { blends: true, .. }
    )
}

/// Classify an edge before considering reflections or diagonal probes.
#[must_use]
pub fn neighbour_kind(terrain: Terrain, neighbour: Terrain) -> TerrainEdge {
    if terrain == neighbour || terrain == Terrain::Sand {
        TerrainEdge::None
    } else if blends(terrain) && blends(neighbour) {
        if terrain == Terrain::Dirt {
            TerrainEdge::None
        } else {
            TerrainEdge::Blend
        }
    } else {
        TerrainEdge::Hard
    }
}

/// Select a fill, keeping an existing fill frame without any random draw.
///
/// # Errors
/// Reports an empty canonical frame range.
pub fn select_base(
    terrain: Terrain,
    strength: BrushStrength,
    old: Option<TerrainTile>,
    rng: &mut RetailRng,
) -> Result<TerrainTile, TerrainRuleError> {
    if let Some(tile) =
        old.filter(|tile| tile.terrain == terrain && tile.shape() == raw::SHAPE_FILL)
    {
        return Ok(TerrainTile {
            reflection: Reflection::default(),
            ..tile
        });
    }
    let mut special = false;
    if let raw::TerrainRuleData::Pattern { chance, .. } = raw::TERRAIN_RULES[terrain.index()] {
        if RANGES[terrain.index()][raw::SHAPE_FILL as usize * 4 + 1].is_some() {
            let percent = chance * strength.0 / raw::RMG_FULL_BRUSH_STRENGTH;
            special = rng.draw() % 100 < percent;
        }
    }
    let transition = TerrainTransition {
        shape: TerrainShape::Fill,
        reflection: Reflection::default(),
    };
    let frame = select_range(terrain, transition, usize::from(special), rng)?;
    Ok(TerrainTile {
        terrain,
        frame,
        reflection: transition.reflection,
    })
}

/// Select a non-special transition, retaining an already matching frame.
///
/// # Errors
/// Reports a transition that has no frame for this terrain/reflection.
pub fn select_transition(
    terrain: Terrain,
    transition: TerrainTransition,
    old: Option<TerrainTile>,
    rng: &mut RetailRng,
) -> Result<TerrainTile, TerrainRuleError> {
    let fixed = matches!(
        raw::TERRAIN_RULES[terrain.index()],
        raw::TerrainRuleData::Fixed
    );
    let reflection = if fixed {
        Reflection::default()
    } else {
        transition.reflection
    };
    let old = old.filter(|tile| tile.terrain == terrain && tile.shape() == transition.shape as u32);
    let old = old.filter(|tile| {
        if !fixed {
            return true;
        }
        let entry = raw::ROCK_FRAMES[usize::from(tile.frame)];
        (entry.m_flipX != 0) == transition.reflection.flip_x
            && (entry.m_flipY != 0) == transition.reflection.flip_y
    });
    let key = transition.shape as usize * 4
        + if fixed {
            usize::from(transition.reflection.flip_x) * 2
                + usize::from(transition.reflection.flip_y)
        } else {
            0
        };
    let frame = match old {
        Some(tile) => tile.frame,
        None => select_range(terrain, transition, key, rng)?,
    };
    Ok(TerrainTile {
        terrain,
        frame,
        reflection,
    })
}
fn select_range(
    terrain: Terrain,
    transition: TerrainTransition,
    key: usize,
    rng: &mut RetailRng,
) -> Result<u8, TerrainRuleError> {
    let Some(range) = RANGES[terrain.index()][key] else {
        // The original calls rand() before its empty-range division fault.
        rng.draw();
        return Err(TerrainRuleError::MissingRange {
            terrain,
            transition,
        });
    };
    Ok(u8::try_from(range.select(rng)).expect("source-derived frame ranges fit u8"))
}

/// Priority-ordered classification under four reflections. A family tries all
/// reflections before the next family, deciding which overlapping shape wins.
#[must_use]
pub fn classify(neighbours: &[TerrainEdge; raw::TILE_DIR_COUNT as usize]) -> TerrainTransition {
    // With no transition edges, the shape is already known. The native painter
    // likewise skips its pattern search for uniform terrain interiors.
    if neighbours.iter().all(|edge| *edge == TerrainEdge::None) {
        return TerrainTransition {
            shape: TerrainShape::Fill,
            reflection: Reflection::default(),
        };
    }
    for family in 0..10 {
        for [flip_x, flip_y] in raw::LINE_REFLECTIONS {
            let mut reflection = Reflection { flip_x, flip_y };
            let order = reflection.neighbour_order();
            let ring = std::array::from_fn(|index| neighbours[order[index]]);
            if let Some((shape, invert)) = classify_family(family, ring) {
                if invert {
                    reflection.flip_x = !flip_x;
                    reflection.flip_y = !flip_y;
                }
                return TerrainTransition { shape, reflection };
            }
        }
    }
    TerrainTransition {
        shape: TerrainShape::Fill,
        reflection: Reflection::default(),
    }
}

#[expect(
    clippy::too_many_lines,
    reason = "the ordered classifier families are one decision table"
)]
fn classify_family(
    family: usize,
    ring: [TerrainEdge; raw::TILE_DIR_COUNT as usize],
) -> Option<(TerrainShape, bool)> {
    use TerrainEdge::{Blend, Hard, None as NoEdge};
    use TerrainShape::{
        EastBlendSeHard, EastBlendSwHard, EastHardSwBlend, EastSouthBlendNeSwHard,
        EastSouthBlendSeHard, NorthBlend, NorthHard, NwBlendSeHard, NwCornerBlend,
        NwCornerBlendSeHard, NwCornerHard, NwCornerHardSeBlend, NwCornerSeBlend, NwCornerSeHard,
        NwSeBlend, NwSeHard, SeBlend, SeHard, SouthBlendNeHard, SouthBlendSeHard, SouthHardNeBlend,
        WestBlend, WestHard,
    };
    let at = |direction: u32| ring[direction as usize];
    let north = at(raw::TILE_DIR_NORTH);
    let northeast = at(raw::TILE_DIR_NORTHEAST);
    let east = at(raw::TILE_DIR_EAST);
    let southeast = at(raw::TILE_DIR_SOUTHEAST);
    let south = at(raw::TILE_DIR_SOUTH);
    let southwest = at(raw::TILE_DIR_SOUTHWEST);
    let west = at(raw::TILE_DIR_WEST);
    let northwest = at(raw::TILE_DIR_NORTHWEST);
    let corner = |kind| north == kind && west == kind;
    let offset_corner =
        |kind| (west == kind && northeast == kind) || (north == kind && southwest == kind);
    let shape = match family {
        0 => {
            if east != Blend || south != Blend {
                return None;
            }
            if northeast == Hard && southwest == Hard {
                EastSouthBlendNeSwHard
            } else if southeast == Hard {
                EastSouthBlendSeHard
            } else {
                return None;
            }
        }
        1 => {
            if corner(Blend) && southeast != NoEdge {
                if southeast == Blend {
                    NwCornerSeBlend
                } else {
                    NwCornerBlendSeHard
                }
            } else if corner(Hard) && southeast != NoEdge {
                if southeast == Hard {
                    NwCornerSeHard
                } else {
                    NwCornerHardSeBlend
                }
            } else {
                return None;
            }
        }
        2 => {
            if east == Hard && south == Blend {
                if southwest == Hard {
                    return Some((NwCornerHard, true));
                }
                EastHardSwBlend
            } else if east == Blend && south == Hard {
                if northeast == Hard {
                    return Some((NwCornerHard, true));
                }
                SouthHardNeBlend
            } else {
                return None;
            }
        }
        3 => {
            if east != Blend || south != Blend {
                return None;
            }
            if southwest == Hard {
                EastBlendSwHard
            } else if northeast == Hard {
                SouthBlendNeHard
            } else {
                return None;
            }
        }
        4 => {
            if corner(Blend) {
                NwCornerBlend
            } else if corner(Hard) {
                NwCornerHard
            } else {
                return None;
            }
        }
        5 => {
            if east == Blend && southwest == Hard {
                EastBlendSwHard
            } else if south == Blend && northeast == Hard {
                SouthBlendNeHard
            } else if east == Hard && southwest == Blend {
                EastHardSwBlend
            } else if south == Hard && northeast == Blend {
                SouthHardNeBlend
            } else if offset_corner(Blend) {
                NwCornerBlend
            } else if offset_corner(Hard) {
                NwCornerHard
            } else {
                return None;
            }
        }
        6 => {
            if east == Blend && southeast == Hard {
                EastBlendSeHard
            } else if south == Blend && southeast == Hard {
                SouthBlendSeHard
            } else {
                return None;
            }
        }
        7 => {
            if north == Blend {
                NorthBlend
            } else if north == Hard {
                NorthHard
            } else if west == Blend {
                WestBlend
            } else if west == Hard {
                WestHard
            } else {
                return None;
            }
        }
        8 => {
            if northwest == Blend && southeast == Blend {
                NwSeBlend
            } else if northwest == Blend && southeast == Hard {
                NwBlendSeHard
            } else if northwest == Hard && southeast == Hard {
                NwSeHard
            } else {
                return None;
            }
        }
        9 => {
            if southeast == Blend {
                SeBlend
            } else if southeast == Hard {
                SeHard
            } else {
                return None;
            }
        }
        _ => unreachable!("classifier has ten families"),
    };
    Some((shape, false))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn existing_fill_frames_keep_the_frame_and_rng_but_clear_reflections() {
        for terrain in [
            Terrain::Dirt,
            Terrain::Sand,
            Terrain::Grass,
            Terrain::Snow,
            Terrain::Swamp,
            Terrain::Rough,
            Terrain::Subterranean,
            Terrain::Lava,
            Terrain::Water,
            Terrain::Rock,
        ] {
            let mut rng = RetailRng::new(42);
            let fill = select_base(terrain, BrushStrength::GENERATOR, None, &mut rng).unwrap();
            let checkpoint = rng.checkpoint();
            let reflected = TerrainTile::parse(
                terrain,
                fill.frame(),
                Reflection {
                    flip_x: true,
                    flip_y: true,
                },
            )
            .unwrap();
            assert_eq!(
                select_base(terrain, BrushStrength::GENERATOR, Some(reflected), &mut rng).unwrap(),
                fill
            );
            assert_eq!(rng.checkpoint(), checkpoint);
            assert!(TerrainTile::parse(
                terrain,
                u8::try_from(frame_count(terrain)).unwrap(),
                Reflection::default()
            )
            .is_err());
        }
        assert!(BrushStrength::parse(raw::RMG_FULL_BRUSH_STRENGTH).is_some());
        assert!(BrushStrength::parse(raw::RMG_FULL_BRUSH_STRENGTH + 1).is_none());
    }

    #[test]
    fn rock_reflection_is_in_the_frame_and_missing_ranges_fault_after_drawing() {
        let mut rng = RetailRng::new(42);
        for [flip_x, flip_y] in raw::LINE_REFLECTIONS {
            let transition = TerrainTransition {
                shape: TerrainShape::NwCornerHard,
                reflection: Reflection { flip_x, flip_y },
            };
            let tile = select_transition(Terrain::Rock, transition, None, &mut rng).unwrap();
            let entry = raw::ROCK_FRAMES[usize::from(tile.frame())];
            assert_eq!(entry.m_transition, raw::SHAPE_N_W_HARD);
            assert_eq!(entry.m_flipX != 0, flip_x);
            assert_eq!(entry.m_flipY != 0, flip_y);
            assert_eq!(tile.reflection(), Reflection::default());
            let checkpoint = rng.checkpoint();
            assert_eq!(
                select_transition(Terrain::Rock, transition, Some(tile), &mut rng).unwrap(),
                tile
            );
            assert_eq!(rng.checkpoint(), checkpoint);
        }
        let checkpoint = rng.checkpoint();
        let unavailable = TerrainTransition {
            shape: TerrainShape::NwCornerBlend,
            reflection: Reflection::default(),
        };
        assert!(matches!(
            select_transition(Terrain::Rock, unavailable, None, &mut rng),
            Err(TerrainRuleError::MissingRange { .. })
        ));
        assert_eq!(rng.checkpoint().draws, checkpoint.draws + 1);
    }
}
