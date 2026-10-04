//! Road/river shape selection and reflection, independent of map ownership.
//!
//! North is up; `#` is a line tile. Reflections provide other orientations.
//! ```text
//!   END_S  END_E  NS     EW     SE     NES    ESW    CROSS
//!   . . .  . . .  . # .  . . .  . . .  . # .  . . .  . # .
//!   . # .  . # #  . # .  # # #  . # #  . # #  # # #  # # #
//!   . # .  . . .  . # .  . . .  . # .  . # .  . # .  . # .
//! ```
//! `SE_VARIANT` is SE with NE or SW also present. `END_S` also covers isolation.

use crate::{raw, rng::RetailRng};
use std::num::NonZeroU32;

/// An actual road or river shape, excluding the C++ count sentinel.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum LinePattern {
    /// Southward end, also used for an isolated road tile.
    EndSouth = raw::LINE_END_S,
    /// Eastward end.
    EndEast = raw::LINE_END_E,
    /// North-south straight.
    NorthSouth = raw::LINE_NS,
    /// East-west straight.
    EastWest = raw::LINE_EW,
    /// Southeast corner.
    SouthEast = raw::LINE_SE,
    /// Southeast corner with a northeast or southwest diagonal.
    SouthEastVariant = raw::LINE_SE_VARIANT,
    /// North-east-south junction.
    NorthEastSouth = raw::LINE_NES,
    /// East-south-west junction.
    EastSouthWest = raw::LINE_ESW,
    /// Four-way junction.
    Cross = raw::LINE_CROSS,
}

/// An orthogonal reflection, with no invalid numeric flag states.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Reflection {
    /// Reflect east/west.
    pub flip_x: bool,
    /// Reflect north/south.
    pub flip_y: bool,
}

impl Reflection {
    /// A canonical direction `d` reads the source neighbour at `order[d]`.
    ///
    /// ```text
    ///   none      flipY     flipX     both
    ///   NW N NE   SW S SE   NE N NW   SE S SW
    ///   W  .  E   W  .  E   E  .  W   E  .  W
    ///   SW S SE   NW N NE   SE S SW   NE N NW
    /// ```
    #[must_use]
    pub fn neighbour_order(self) -> &'static [usize; raw::TILE_DIR_COUNT as usize] {
        &raw::REFLECTED_NEIGHBOURS[usize::from(self.flip_x)][usize::from(self.flip_y)]
    }
}

/// A shape and its orientation, before frame randomness is applied.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct LineSelection {
    /// Unreflected shape.
    pub pattern: LinePattern,
    /// Reflection applied to that shape.
    pub reflection: Reflection,
}

impl LineSelection {
    const fn new(pattern: LinePattern, flip_x: bool, flip_y: bool) -> Self {
        Self {
            pattern,
            reflection: Reflection { flip_x, flip_y },
        }
    }
}

/// A nonempty contiguous range of sprite frames for a shape.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct FrameRange {
    first: u32,
    count: NonZeroU32,
}
impl FrameRange {
    pub(crate) const fn new(first: u32, count: NonZeroU32) -> Self {
        Self { first, count }
    }
    /// First sprite frame for this shape.
    #[must_use]
    pub const fn first(self) -> u32 {
        self.first
    }

    /// Number of alternatives, guaranteed nonzero.
    #[must_use]
    pub const fn count(self) -> NonZeroU32 {
        self.count
    }

    /// Select a frame with one retail RNG draw, including singleton ranges.
    pub fn select(self, rng: &mut RetailRng) -> u32 {
        self.first + rng.below(self.count)
    }
}

/// Immutable built-in shape ranges, calculated once at compile time.
#[derive(Clone, Copy, Debug)]
pub struct LineTable {
    ranges: [Option<FrameRange>; raw::LINE_PATTERN_COUNT as usize],
}

impl LineTable {
    /// Road tiles include ends and the diagonal corner variant.
    pub const ROAD: Self = Self::build(&raw::ROAD_PATTERNS);
    /// River tiles extend isolated/end tiles with a straight shape.
    pub const RIVER: Self = Self::build(&raw::RIVER_PATTERNS);

    const fn build(frames: &[u32]) -> Self {
        let mut ranges: [Option<FrameRange>; raw::LINE_PATTERN_COUNT as usize] =
            [None; raw::LINE_PATTERN_COUNT as usize];
        let mut index = 0;
        let mut frame_number = 0;
        while index < frames.len() {
            let pattern = frames[index] as usize;
            assert!(pattern < ranges.len(), "unknown source line pattern");
            ranges[pattern] = Some(match ranges[pattern] {
                None => FrameRange {
                    first: frame_number,
                    count: NonZeroU32::MIN,
                },
                Some(previous) => {
                    assert!(
                        index > 0 && frames[index - 1] == frames[index],
                        "source frames must be contiguous"
                    );
                    FrameRange {
                        first: previous.first,
                        count: match NonZeroU32::new(previous.count.get() + 1) {
                            Some(count) => count,
                            None => panic!("source line frame count overflow"),
                        },
                    }
                }
            });
            index += 1;
            frame_number += 1;
        }
        Self { ranges }
    }

    /// Available frames for a pattern; some river patterns have no frames.
    #[must_use]
    pub const fn range(&self, pattern: LinePattern) -> Option<FrameRange> {
        self.ranges[pattern as usize]
    }

    /// Select a shape using the retail branch and reflection preference order.
    /// Does not draw randomness or allocate.
    #[must_use]
    pub fn select(&self, neighbours: &[bool; raw::TILE_DIR_COUNT as usize]) -> LineSelection {
        let n = neighbours[raw::TILE_DIR_NORTH as usize];
        let e = neighbours[raw::TILE_DIR_EAST as usize];
        let s = neighbours[raw::TILE_DIR_SOUTH as usize];
        let w = neighbours[raw::TILE_DIR_WEST as usize];
        if n && e && s && w {
            return LineSelection::new(LinePattern::Cross, false, false);
        }
        if n && s {
            return if e {
                LineSelection::new(LinePattern::NorthEastSouth, false, false)
            } else if w {
                LineSelection::new(LinePattern::NorthEastSouth, true, false)
            } else {
                LineSelection::new(LinePattern::NorthSouth, false, false)
            };
        }
        if e && w {
            return if s {
                LineSelection::new(LinePattern::EastSouthWest, false, false)
            } else if n {
                LineSelection::new(LinePattern::EastSouthWest, false, true)
            } else {
                LineSelection::new(LinePattern::EastWest, false, false)
            };
        }
        let has_variant = self.range(LinePattern::SouthEastVariant).is_some();
        for [flip_x, flip_y] in raw::LINE_REFLECTIONS {
            let reflection = Reflection { flip_x, flip_y };
            let order = reflection.neighbour_order();
            if neighbours[order[raw::TILE_DIR_EAST as usize]]
                && neighbours[order[raw::TILE_DIR_SOUTH as usize]]
            {
                let diagonal = neighbours[order[raw::TILE_DIR_NORTHEAST as usize]]
                    || neighbours[order[raw::TILE_DIR_SOUTHWEST as usize]];
                return LineSelection {
                    reflection,
                    pattern: if has_variant && diagonal {
                        LinePattern::SouthEastVariant
                    } else {
                        LinePattern::SouthEast
                    },
                };
            }
        }
        if self.range(LinePattern::EndSouth).is_some() {
            if e || w {
                LineSelection::new(LinePattern::EndEast, w, false)
            } else {
                LineSelection::new(LinePattern::EndSouth, false, !s)
            }
        } else {
            LineSelection::new(
                if e || w {
                    LinePattern::EastWest
                } else {
                    LinePattern::NorthSouth
                },
                false,
                false,
            )
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn every_neighbour_mask_selects_available_frames() {
        for mask in 0_u32..256 {
            let neighbours = std::array::from_fn(|bit| mask & (1 << bit) != 0);
            for table in [LineTable::ROAD, LineTable::RIVER] {
                let selection = table.select(&neighbours);
                assert!(table.range(selection.pattern).is_some(), "mask {mask}");
            }
        }
    }

    #[test]
    fn isolated_road_and_river_use_different_shapes() {
        let neighbours = [false; raw::TILE_DIR_COUNT as usize];
        assert_eq!(
            LineTable::ROAD.select(&neighbours),
            LineSelection::new(LinePattern::EndSouth, false, true)
        );
        assert_eq!(
            LineTable::RIVER.select(&neighbours),
            LineSelection::new(LinePattern::NorthSouth, false, false)
        );
    }

    #[test]
    fn source_reflections_are_involutions() {
        for [flip_x, flip_y] in raw::LINE_REFLECTIONS {
            let order = Reflection { flip_x, flip_y }.neighbour_order();
            for direction in 0..order.len() {
                assert_eq!(order[order[direction]], direction);
            }
        }
    }

    #[test]
    fn frame_ranges_reconstruct_source_arrays() {
        for (frames, table) in [
            (raw::ROAD_PATTERNS.as_slice(), LineTable::ROAD),
            (raw::RIVER_PATTERNS.as_slice(), LineTable::RIVER),
        ] {
            for (pattern, range) in table.ranges.iter().enumerate() {
                if let Some(range) = range {
                    let first = range.first as usize;
                    let end = first + range.count.get() as usize;
                    assert!(frames[first..end]
                        .iter()
                        .all(|&frame| frame as usize == pattern));
                }
            }
        }
    }
}
