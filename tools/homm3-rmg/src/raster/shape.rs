//! Edge-local limits for midpoint displacement (`HotA` 1.8.1).

use super::{add, multiply_divide, subtract, Point, RasterError, RetailRng};

#[derive(Clone, Copy, Debug)]
pub(crate) struct BoundaryShape {
    corners: [Point; 4],
    lines: [(i32, i32, i32); 4],
    radius: i32,
}

impl BoundaryShape {
    // RVA 0x1ce000: shrink both sites toward the clipped edge midpoint.
    pub(super) fn new(
        from: Point,
        to: Point,
        own: Point,
        opposite: Point,
    ) -> Result<Self, RasterError> {
        let midpoint = from
            .subdivision_midpoint(to)
            .ok_or(RasterError::Arithmetic)?;
        let shrink = |point: Point| {
            let axis = |value: i32, center: i32| {
                if value > center + 1 {
                    value - 2
                } else if value < center - 1 {
                    value + 2
                } else {
                    value
                }
            };
            Point::new(axis(point.x, midpoint.x), axis(point.y, midpoint.y))
        };
        let corners = [shrink(own), from, shrink(opposite), to];
        let lines = std::array::from_fn(|index| {
            let a = corners[index];
            let b = corners[(index + 1) % 4];
            (
                a.y.wrapping_sub(b.y),
                b.x.wrapping_sub(a.x),
                a.x.wrapping_mul(b.y).wrapping_sub(a.y.wrapping_mul(b.x)),
            )
        });
        let delta = subtract(to, from)?;
        let square = delta
            .x
            .wrapping_mul(delta.x)
            .wrapping_add(delta.y.wrapping_mul(delta.y));
        let radius = 1 - truncate(f64::from(square).sqrt() * -0.5)?;
        Ok(Self {
            corners,
            lines,
            radius,
        })
    }

    // RVA 0x1cacd0: either winding is accepted; the edges are excluded.
    fn contains(self, point: Point) -> bool {
        let crosses: [i32; 4] = std::array::from_fn(|index| {
            let a = self.corners[index];
            let b = self.corners[(index + 1) % 4];
            a.x.wrapping_sub(point.x)
                .wrapping_mul(b.y.wrapping_sub(a.y))
                .wrapping_sub(
                    a.y.wrapping_sub(point.y)
                        .wrapping_mul(b.x.wrapping_sub(a.x)),
                )
        });
        crosses.iter().all(|&cross| cross > 0) || crosses.iter().all(|&cross| cross < 0)
    }

    // RVA 0x1cadc0: intersect the perpendicular with the original edge's
    // quad, then clamp to the circle's sagitta and half the zone roughness.
    pub(super) fn midpoint(
        self,
        from: Point,
        to: Point,
        roughness: u32,
        rng: &mut RetailRng,
    ) -> Result<Option<Point>, RasterError> {
        let midpoint = from
            .subdivision_midpoint(to)
            .ok_or(RasterError::Arithmetic)?;
        if midpoint == from || midpoint == to {
            return Ok(None);
        }
        let delta = subtract(to, from)?;
        let perpendicular = Point::new(
            delta.y.checked_neg().ok_or(RasterError::Arithmetic)?,
            delta.x,
        );
        let length = perpendicular.distance(Point::new(0, 0))?;
        if length <= 1 {
            return Ok(Some(midpoint));
        }
        let mut positive = -1.0;
        let mut negative = 1.0;
        if self.contains(midpoint) {
            for (a, b, c) in self.lines {
                let denominator = a
                    .wrapping_mul(perpendicular.x)
                    .wrapping_add(b.wrapping_mul(perpendicular.y));
                if denominator == 0 {
                    continue;
                }
                let numerator = a
                    .wrapping_mul(midpoint.x)
                    .wrapping_add(b.wrapping_mul(midpoint.y))
                    .wrapping_add(c)
                    .wrapping_neg();
                let crossing = f64::from(numerator) / f64::from(denominator);
                if crossing > 0.0 {
                    if positive < 0.0 || positive > crossing {
                        positive = crossing;
                    }
                } else if negative > 0.0 || negative < crossing {
                    negative = crossing;
                }
            }
        } else {
            positive = 0.0;
            negative = 0.0;
        }
        let raw_upper = (truncate(f64::from(length) * positive)? - 1).max(0);
        let raw_lower = (truncate(f64::from(length) * negative)? + 1).min(0);
        let square = self
            .radius
            .wrapping_mul(self.radius)
            .wrapping_sub(length.wrapping_mul(length) / 4);
        let sagitta = truncate(f64::from(self.radius) - f64::from(square).sqrt())?;
        let limit = (length / 2 - 1).min(sagitta.max(3));
        let roughness = i32::try_from(roughness).map_err(|_| RasterError::Arithmetic)? / 2;
        let upper = raw_upper.min(limit).min(roughness);
        let lower = raw_lower.max(-limit).max(-roughness);
        let displacement = if lower >= upper || lower > 0 || upper < 0 {
            0
        } else if lower >= -4 && upper <= 4 {
            if rng.draw() % 2 != 0 {
                upper
            } else {
                lower
            }
        } else {
            let count = u32::try_from(i64::from(raw_upper) - i64::from(raw_lower) + 1)
                .map_err(|_| RasterError::Arithmetic)?;
            let sign = i64::from(raw_lower) + i64::from(rng.draw() % count);
            match sign.cmp(&0) {
                std::cmp::Ordering::Less => lower,
                std::cmp::Ordering::Equal => 0,
                std::cmp::Ordering::Greater => upper,
            }
        };
        Ok(Some(add(
            midpoint,
            multiply_divide(perpendicular, displacement, length)?,
        )?))
    }
}

fn truncate(value: f64) -> Result<i32, RasterError> {
    if !value.is_finite()
        || value.trunc() < f64::from(i32::MIN)
        || value.trunc() > f64::from(i32::MAX)
    {
        return Err(RasterError::Arithmetic);
    }
    #[allow(clippy::cast_possible_truncation)]
    Ok(value as i32)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn constrained_displacement_keeps_endpoint_draws_and_skips_empty_ranges() {
        let from = Point::new(10, 5);
        let to = Point::new(10, 15);
        let shape = BoundaryShape::new(from, to, Point::new(0, 10), Point::new(20, 10)).unwrap();
        for (seed, expected) in [(1, Point::new(7, 10)), (0, Point::new(13, 10))] {
            let mut rng = RetailRng::new(seed);
            assert_eq!(
                shape.midpoint(from, to, 8, &mut rng).unwrap(),
                Some(expected)
            );
            assert_eq!(rng.draws(), 1);
        }
        let mut rng = RetailRng::new(1);
        assert_eq!(
            shape.midpoint(from, to, 0, &mut rng).unwrap(),
            Some(Point::new(10, 10))
        );
        assert_eq!(rng.draws(), 0);
        assert_eq!(
            shape
                .midpoint(from, Point::new(10, 6), 8, &mut rng)
                .unwrap(),
            None
        );
        assert_eq!(rng.draws(), 0);

        let to = Point::new(10, 35);
        let wide = BoundaryShape::new(from, to, Point::new(-20, 20), Point::new(40, 20)).unwrap();
        assert_eq!(
            wide.midpoint(from, to, 50, &mut rng).unwrap(),
            Some(Point::new(0, 20))
        );
        assert_eq!(rng.draws(), 1);
        // A midpoint on the quad boundary is outside, even if roughness permits noise.
        let flat = BoundaryShape::new(from, to, from, to).unwrap();
        assert_eq!(
            flat.midpoint(from, to, 50, &mut rng).unwrap(),
            Some(Point::new(10, 20))
        );
        assert_eq!(rng.draws(), 1);
    }
}
