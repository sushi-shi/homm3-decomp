//! Midpoint-displacement masks for the bounded water-zone islands.
use super::{ConnectionError, PlacementError};
use crate::{geometry::Point, raw, rng::RetailRng};
use std::num::NonZeroU32;

const MAX_SIDE: usize = 2 * raw::RMG_ISLAND_MAXIMUM_RADIUS as usize;

// Corner indices and edge midpoints, with X right and Y down:
//   0     min_y    2
//   min_x centre   max_x
//   1     max_y    3
// Corners below follow 0, 1, 2, 3; Midpoints names the four edge averages.
#[derive(Clone, Copy, Debug, Default)]
#[expect(
    clippy::struct_field_names,
    reason = "source corners identify both coordinate bounds"
)]
struct Corners {
    min_x_min_y: i32,
    min_x_max_y: i32,
    max_x_min_y: i32,
    max_x_max_y: i32,
}
#[derive(Clone, Copy, Debug)]
struct Region {
    minimum: Point,
    maximum: Point,
    corners: Corners,
    variation: i32,
}
#[derive(Clone, Copy, Debug, Default)]
struct Midpoints {
    min_x: i32,
    min_y: i32,
    max_x: i32,
    max_y: i32,
}

/// Admitted dimensions of a bounded, nonempty island mask.
#[derive(Clone, Copy)]
pub(super) struct MaskSize {
    width: usize,
    height: usize,
}
impl MaskSize {
    pub(super) fn parse(width: i32, height: i32) -> Result<Self, PlacementError> {
        let width = usize::try_from(width).map_err(|_| PlacementError::Arithmetic)?;
        let height = usize::try_from(height).map_err(|_| PlacementError::Arithmetic)?;
        if !(1..=MAX_SIDE).contains(&width) || !(1..=MAX_SIDE).contains(&height) {
            return Err(PlacementError::Arithmetic);
        }
        Ok(Self { width, height })
    }
    pub(super) const fn width(self) -> usize {
        self.width
    }
}

/// Borrowed mask, with no padding exposed to painting.
pub(super) struct IslandMask<'a> {
    size: MaskSize,
    cells: &'a [u8],
}
impl IslandMask<'_> {
    pub(super) const fn size(&self) -> MaskSize {
        self.size
    }
    pub(super) fn cells(&self) -> &[u8] {
        self.cells
    }
}

#[derive(Debug)]
pub(super) struct NoiseWorkspace {
    pending: Vec<Region>,
    mask: [u8; MAX_SIDE * MAX_SIDE],
}
impl Default for NoiseWorkspace {
    fn default() -> Self {
        Self {
            pending: Vec::new(),
            mask: [0; MAX_SIDE * MAX_SIDE],
        }
    }
}
impl NoiseWorkspace {
    pub(super) fn clear(&mut self) {
        self.pending.clear();
    }
    pub(super) fn generate(
        &mut self,
        size: MaskSize,
        rng: &mut RetailRng,
    ) -> Result<IslandMask<'_>, ConnectionError> {
        self.clear();
        let width = i32::try_from(size.width).unwrap();
        let height = i32::try_from(size.height).unwrap();
        // This noise grid uses bounds X for rows and Y for columns:
        // subdivision receives (height, width), and output uses X*width + Y.
        let region = Region {
            minimum: Point::new(0, 0),
            maximum: Point::new(height, width),
            corners: Corners::default(),
            variation: (height + width) / 4 + 1,
        };
        self.subdivide(region, Midpoints::default(), region.variation / 2)?;
        while let Some(mut region) = self.pending.pop() {
            let min = region.minimum;
            let max = region.maximum;
            if max.y == min.y + 1 && max.x == min.x + 1 {
                if min.x >= 0 && min.x < height && min.y >= 0 && min.y < width {
                    self.mask[usize::try_from(min.x * width + min.y).unwrap()] =
                        u8::try_from(region.corners.min_x_min_y.clamp(0, i32::from(u8::MAX)))
                            .unwrap();
                }
                continue;
            }
            if max.x < 0 || max.y < 0 || min.x >= height || min.y >= width {
                continue;
            }
            let c = region.corners;
            let mut m = Midpoints {
                min_x: (c.min_x_max_y + c.min_x_min_y) / 2,
                min_y: (c.max_x_min_y + c.min_x_min_y) / 2,
                max_x: (c.max_x_max_y + c.max_x_min_y) / 2,
                max_y: (c.max_x_max_y + c.min_x_max_y) / 2,
            };
            let mut center = (c.max_x_max_y + c.max_x_min_y + c.min_x_max_y + c.min_x_min_y) / 4;
            if region.variation > 1 {
                let range = NonZeroU32::new(u32::try_from(region.variation).unwrap()).unwrap();
                m.min_x += rng.centered_offset(range);
                m.min_y += rng.centered_offset(range);
                m.max_x += rng.centered_offset(range);
                m.max_y += rng.centered_offset(range);
                center += rng.centered_offset(range);
            }
            region.variation = (region.variation - 1) / 2 + 1;
            self.subdivide(region, m, center)?;
        }
        // Admitted dimensions cap subdivision depth and corner sums; arithmetic
        // above cannot overflow. Every cell is written by exactly one leaf.
        Ok(IslandMask {
            size,
            cells: &self.mask[..size.width * size.height],
        })
    }

    // Each nondegenerate quadrant inherits the region's variation. Quadrants are
    // pushed in the order shown, with x growing right and y down. Each keeps its
    // outer region corner; the centre value and the two edge midpoints beside it
    // become its other corners.
    //          minY
    //   minX   4 | 2   maxX
    //          --+--
    //          3 | 1
    //          maxY
    fn subdivide(
        &mut self,
        region: Region,
        m: Midpoints,
        center: i32,
    ) -> Result<(), ConnectionError> {
        // Unlike branching paths, noise subdivision has no +1 before division.
        let middle = Point::new(
            (region.minimum.x + region.maximum.x) / 2,
            (region.minimum.y + region.maximum.y) / 2,
        );
        let c = region.corners;
        self.append(Region {
            minimum: middle,
            corners: Corners {
                min_x_min_y: center,
                min_x_max_y: m.max_y,
                max_x_min_y: m.max_x,
                max_x_max_y: c.max_x_max_y,
            },
            ..region
        })?;
        self.append(Region {
            minimum: Point::new(middle.x, region.minimum.y),
            maximum: Point::new(region.maximum.x, middle.y),
            corners: Corners {
                min_x_min_y: m.min_y,
                min_x_max_y: center,
                max_x_min_y: c.max_x_min_y,
                max_x_max_y: m.max_x,
            },
            ..region
        })?;
        self.append(Region {
            minimum: Point::new(region.minimum.x, middle.y),
            maximum: Point::new(middle.x, region.maximum.y),
            corners: Corners {
                min_x_min_y: m.min_x,
                min_x_max_y: c.min_x_max_y,
                max_x_min_y: center,
                max_x_max_y: m.max_y,
            },
            ..region
        })?;
        self.append(Region {
            maximum: middle,
            corners: Corners {
                min_x_min_y: c.min_x_min_y,
                min_x_max_y: m.min_x,
                max_x_min_y: m.min_y,
                max_x_max_y: center,
            },
            ..region
        })?;
        Ok(())
    }
    // Omit collapsed dimensions, but preserve reversed bounds.
    fn append(&mut self, region: Region) -> Result<(), ConnectionError> {
        if region.minimum.x != region.maximum.x && region.minimum.y != region.maximum.y {
            self.pending.try_reserve(1)?;
            self.pending.push(region);
        }
        Ok(())
    }
}
