//! Bounded, reusable geometry for object placement; no heap allocation.

use super::{MaskCell, PreparedPrototype, PrototypeFault};
use crate::{geometry::Point, raw};
use std::{error::Error, fmt};

// The walk's state is a halo cell and one of four cardinal headings. If it
// has not returned after this many steps, a state repeats and retail loops.
const OUTLINE_STATES: usize = (raw::OBJECT_MASK_WIDTH as usize + 2)
    * (raw::OBJECT_MASK_HEIGHT as usize + 2)
    * raw::RMG_CARDINAL_DIRECTION_COUNT as usize;

/// A footprint cannot be traced safely.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum OutlineError {
    /// Image dimensions do not fit the fixed mask.
    Prototype(PrototypeFault),
    /// The native walk repeats without returning to its starting cell.
    NonTerminating,
}
impl fmt::Display for OutlineError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Prototype(error) => error.fmt(f),
            Self::NonTerminating => {
                f.write_str("prototype outline walk does not return to its start")
            }
        }
    }
}
impl Error for OutlineError {}

/// Reusable scratch for a single outline. Borrow the result until the next walk.
/// The maximum mask frame bounds storage; loading prototypes allocates no outlines.
#[derive(Debug)]
pub struct OutlineWorkspace {
    points: [Point; OUTLINE_STATES],
}
impl Default for OutlineWorkspace {
    fn default() -> Self {
        Self {
            points: [Point::new(0, 0); OUTLINE_STATES],
        }
    }
}
impl OutlineWorkspace {
    /// Trace the native clockwise outline, relative to the bottom-right anchor.
    /// An unoccupied bottom row produces an empty outline, including terrain holes.
    ///
    /// A full 3x2 footprint gets this ring, walked clockwise from the start S.
    /// North is up; # is the footprint, P its bottom-right cell, o the outline.
    /// ```text
    ///   o o o o o
    ///   o # # # o
    ///   o # # P o
    ///   o o o S o
    /// ```
    ///
    /// # Errors
    /// Rejects invalid dimensions and a native walk that would never terminate.
    pub fn trace(&mut self, entry: &PreparedPrototype<'_>) -> Result<&[Point], OutlineError> {
        let size = entry.image_mask().size().map_err(OutlineError::Prototype)?;
        let prototype = entry.prototype();
        let Some(x) = (0..size.width()).find(|&x| prototype.occupies(MaskCell { x, y: 0 })) else {
            return Ok(&self.points[..0]);
        };
        let start = Point::new(-i32::from(x), 1);
        let mut position = start;
        let mut direction = raw::RMG_DIRECTION_NORTH as usize;
        for count in 0..OUTLINE_STATES {
            self.points[count] = position;
            for _ in 0..raw::RMG_CARDINAL_DIRECTION_COUNT {
                direction = (direction + raw::RMG_DIRECTION_COUNT as usize
                    - raw::RMG_CARDINAL_DIRECTION_STEP as usize)
                    % raw::RMG_DIRECTION_COUNT as usize;
                let (dx, dy) = raw::DIRECTIONS[direction];
                let nearby = Point::new(position.x + dx, position.y + dy);
                let cell = u8::try_from(-nearby.x)
                    .ok()
                    .zip(u8::try_from(-nearby.y).ok())
                    .filter(|&(x, y)| x < size.width() && y < size.height())
                    .map(|(x, y)| MaskCell { x, y });
                if cell.is_none_or(|cell| !prototype.occupies(cell)) {
                    break;
                }
            }
            let (dx, dy) = raw::DIRECTIONS[direction];
            position = Point::new(position.x + dx, position.y + dy);
            direction = (direction + raw::RMG_DIRECTION_COUNT as usize / 2)
                % raw::RMG_DIRECTION_COUNT as usize;
            if position == start {
                return Ok(&self.points[..=count]);
            }
        }
        Err(OutlineError::NonTerminating)
    }
}

/// Overlap order is defined only for drawn cells inside the image dimensions.
/// Unwritten C++ array entries remain inaccessible through this value.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct OverlapPriorities {
    values: [u8; raw::OBJECT_MASK_CELLS as usize],
    written: u64,
}
impl OverlapPriorities {
    /// Compute the native column-first priority recurrence in fixed storage.
    ///
    /// Visit order for a 3x2 image, north up and anchor `P` at ordinal 0:
    /// each column runs south to north, then the walk moves one column west.
    /// The digits are visit ordinals, not the resulting overlap priorities.
    ///
    /// ```text
    ///   5 3 1
    ///   4 2 P
    /// ```
    ///
    /// # Errors
    /// Returns invalid image dimensions before traversing the footprint.
    pub fn build(entry: &PreparedPrototype<'_>) -> Result<Self, PrototypeFault> {
        let size = entry.image_mask().size()?;
        let prototype = entry.prototype();
        let mut result = Self {
            values: [0; raw::OBJECT_MASK_CELLS as usize],
            written: 0,
        };
        for x in 0..size.width() {
            let mut priority = u8::from(!prototype.underlay());
            for y in 0..size.height() {
                let cell = MaskCell { x, y };
                if y > 0 && !prototype.underlay() {
                    if prototype.is_passable(cell) {
                        if x > 0 && !prototype.is_passable(MaskCell { x: x - 1, y }) {
                            // Passability extends outside the draw mask. A blocked
                            // left cell is therefore drawn and already initialized.
                            priority = result.values[MaskCell { x: x - 1, y }.bit() as usize];
                        } else {
                            priority += 1;
                        }
                    } else if prototype.is_passable(MaskCell { x, y: y - 1 }) {
                        priority = 1;
                    } else {
                        priority += 1;
                    }
                }
                // A priority is at most the number of visited cells (48), so
                // increments fit u8 even when a value comes from a prior column.
                if entry.image_mask().draws(cell) {
                    result.values[cell.bit() as usize] = priority;
                    result.written |= 1 << cell.bit();
                }
            }
        }
        Ok(result)
    }
    /// A drawn cell's priority, or no value for a cell native code never writes.
    #[must_use]
    pub const fn get(&self, cell: MaskCell) -> Option<u8> {
        if self.written & (1 << cell.bit()) != 0 {
            Some(self.values[cell.bit() as usize])
        } else {
            None
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::object::ObjectKind;
    use crate::prototype::{FootprintSize, Image, ImageMask, Prototype};
    use std::borrow::Cow;

    fn prototype(draw: u64, blocked: u64, underlay: bool) -> (Prototype, Image<'static>) {
        (
            Prototype {
                source_row: 0,
                image: 0,
                kind: ObjectKind::parse(i32::try_from(raw::TERRAIN_HOLE).unwrap()).unwrap(),
                subtype: 0,
                category: 0,
                underlay,
                passable: !blocked,
                trigger: 0,
                terrain: 0,
                recommended: 0,
                entrance: None,
            },
            Image {
                name: Cow::Borrowed(b"test.def"),
                mask: ImageMask {
                    size: super::super::ImageDimensions::Footprint(
                        FootprintSize::parse(3, 2).unwrap(),
                    ),
                    draw,
                    shadow: 0,
                },
            },
        )
    }
    fn prepared<'a>(prototype: &'a Prototype, image: &'a Image<'a>) -> PreparedPrototype<'a> {
        PreparedPrototype {
            prototype,
            image,
            footprint: prototype.footprint(image.mask),
            preferred: None,
            rule: None,
        }
    }
    fn cells(points: &[(u8, u8)]) -> u64 {
        points
            .iter()
            .fold(0, |mask, &(x, y)| mask | 1 << MaskCell { x, y }.bit())
    }

    #[test]
    fn clockwise_ring_preserves_ascii_order_and_reuses_storage() {
        let mask = cells(&[(0, 0), (1, 0), (2, 0), (0, 1), (1, 1), (2, 1)]);
        let (prototype, image) = prototype(mask, mask, false);
        let entry = prepared(&prototype, &image);
        let mut workspace = OutlineWorkspace::default();
        let result = workspace.trace(&entry).unwrap();
        let expected = [
            (0, 1),
            (-1, 1),
            (-2, 1),
            (-3, 1),
            (-3, 0),
            (-3, -1),
            (-3, -2),
            (-2, -2),
            (-1, -2),
            (0, -2),
            (1, -2),
            (1, -1),
            (1, 0),
            (1, 1),
        ];
        assert!(result
            .iter()
            .copied()
            .eq(expected.map(|(x, y)| Point::new(x, y))));
        let address = result.as_ptr();
        assert_eq!(workspace.trace(&entry).unwrap().as_ptr(), address);
        let priorities = OverlapPriorities::build(&entry).unwrap();
        for x in 0..3 {
            assert_eq!(priorities.get(MaskCell { x, y: 0 }), Some(1));
            assert_eq!(priorities.get(MaskCell { x, y: 1 }), Some(2));
        }
        assert_eq!(priorities.get(MaskCell { x: 3, y: 0 }), None);
    }

    #[test]
    fn empty_bottom_row_is_defined_and_underlays_have_zero_priority() {
        let mask = cells(&[(0, 1), (2, 1)]);
        let (prototype, image) = prototype(mask, mask, true);
        let entry = prepared(&prototype, &image);
        assert!(OutlineWorkspace::default()
            .trace(&entry)
            .unwrap()
            .is_empty());
        let priorities = OverlapPriorities::build(&entry).unwrap();
        assert_eq!(priorities.get(MaskCell { x: 0, y: 1 }), Some(0));
        assert_eq!(priorities.get(MaskCell { x: 1, y: 1 }), None);
    }

    #[test]
    fn all_small_masks_have_bounded_closed_outlines_including_disconnected_shapes() {
        let mut workspace = OutlineWorkspace::default();
        for bits in 0_u32..1 << 16 {
            let mut mask = 0;
            for bit in 0..16 {
                if bits & (1 << bit) != 0 {
                    mask |= 1
                        << MaskCell {
                            x: bit % 4,
                            y: bit / 4,
                        }
                        .bit();
                }
            }
            let (prototype, mut image) = prototype(mask, mask, false);
            image.mask.size =
                super::super::ImageDimensions::Footprint(FootprintSize::parse(4, 4).unwrap());
            let entry = prepared(&prototype, &image);
            let outline = workspace.trace(&entry).unwrap();
            assert_eq!(outline.is_empty(), bits.trailing_zeros() >= 4);
            for (index, &point) in outline.iter().enumerate() {
                assert!((-4..=1).contains(&point.x) && (-4..=1).contains(&point.y));
                if let Some(cell) = u8::try_from(-point.x)
                    .ok()
                    .zip(u8::try_from(-point.y).ok())
                    .and_then(|(x, y)| MaskCell::parse(x, y))
                {
                    assert!(!prototype.occupies(cell));
                }
                let next = outline[(index + 1) % outline.len()];
                assert_eq!((point.x - next.x).abs() + (point.y - next.y).abs(), 1);
            }
        }
    }
}
