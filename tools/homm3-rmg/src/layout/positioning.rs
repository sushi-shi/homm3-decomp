//! Versioned policies within the shared zone-positioning pass.
//!
//! `HotA` 1.8.1: recovered `hota_rmg_zones.cpp` and composed `rmg.cpp`
//! (`hota-rmg` 3a1421ef5). Addresses below are HotA.dll RVAs. Integer
//! operations explicitly marked wrapping preserve the native intermediate
//! widths; unrepresentable floating conversions remain typed faults.

use super::{zone_size, Bounds, LayoutError, PositionedZone};
use crate::{
    domain::{Level, WorldPosition},
    geometry::Point,
    request::{Levels, Request, Water},
    rules::Ruleset,
    template::{ConnectionKind, Template, Zone},
};

#[derive(Clone, Copy)]
pub(super) struct Positioning {
    rules: Ruleset,
    pub levels: Levels,
    pub map_size: i32,
}
impl Positioning {
    pub fn new(
        template: &Template<'_>,
        request: &Request,
        water: Water,
    ) -> Result<Self, LayoutError> {
        let rules = template.ruleset();
        let levels = match rules {
            Ruleset::Complete => request.levels(),
            Ruleset::HotA181 => request.constructor_parameters().levels,
        };
        let map_size = match rules {
            Ruleset::Complete => {
                let minimum = template
                    .zones()
                    .iter()
                    .map(zone_size)
                    .min()
                    .unwrap_or(32000)
                    .min(32000);
                let side = i32::try_from(request.size().dimension())
                    .map_err(|_| LayoutError::Arithmetic)?;
                let divisor = match water {
                    Water::None => 5,
                    Water::Normal => 6,
                    Water::Islands => 7,
                };
                minimum.checked_mul(side).ok_or(LayoutError::Arithmetic)? / divisor
            }
            Ruleset::HotA181 => spacing(
                template.zones().iter().map(zone_size),
                levels,
                water,
                template.options().zone_sparseness,
            )?,
        };
        Ok(Self {
            rules,
            levels,
            map_size,
        })
    }

    // RVAs 0x1be480/0x1be4b0: only the enclosing-square radius changes;
    // candidate circles, overlap tests and final scaled sizes use full sizes.
    pub fn radius(self, zone: &Zone) -> i32 {
        let size = zone_size(zone);
        match self.rules {
            Ruleset::Complete => size,
            Ruleset::HotA181 => size.wrapping_mul(4) / 5,
        }
    }

    // RVA 0x1d50e0: teleport/random links do not seed candidates. They can
    // still lead to the ordinary all-placed-zones fallback.
    pub fn seeds_candidates(self, kind: ConnectionKind) -> bool {
        self.rules == Ruleset::Complete
            || !matches!(kind, ConnectionKind::Teleport | ConnectionKind::Random)
    }

    // RVAs 0x1d5120/0x1d51e0, including the following retail increment.
    pub fn connection_weight(self, kind: ConnectionKind, same_level: bool) -> usize {
        if self.rules == Ruleset::Complete {
            return 1;
        }
        match kind {
            ConnectionKind::Automatic => 1,
            ConnectionKind::Ground => {
                if same_level {
                    101
                } else {
                    0
                }
            }
            ConnectionKind::Underground => {
                if same_level {
                    usize::from(self.levels == Levels::Surface)
                } else {
                    101
                }
            }
            ConnectionKind::Teleport | ConnectionKind::Random => 0,
        }
    }

    // RVA 0x1d5460: explicit placement replaces the underground-town rule,
    // but only on two-level maps. The first surface candidate still bypasses
    // can_place, as it does in the composed native positionZone body.
    pub fn required_level(self, zone: &Zone) -> Option<Level> {
        if self.rules == Ruleset::HotA181 && self.levels == Levels::Underground {
            zone.options().placement
        } else {
            None
        }
    }

    pub fn origin(
        self,
        placed: &[PositionedZone],
        zones: &[Zone],
        bounds: Bounds,
    ) -> Result<Point, LayoutError> {
        let span = bounds.span()?;
        match self.rules {
            Ruleset::Complete => Ok(Point::new(
                bounds
                    .minimum
                    .x
                    .checked_sub(span)
                    .and_then(|x| x.checked_add(bounds.maximum.x))
                    .ok_or(LayoutError::Arithmetic)?
                    / 2,
                bounds
                    .minimum
                    .y
                    .checked_sub(span)
                    .and_then(|y| y.checked_add(bounds.maximum.y))
                    .ok_or(LayoutError::Arithmetic)?
                    / 2,
            )),
            Ruleset::HotA181 => weighted_origin(
                placed
                    .iter()
                    .map(|p| (p.position.point, zone_size(&zones[p.id.index()]))),
                span,
            ),
        }
    }

    // RVA 0x1d4720: after both original filters, retain scores within three
    // of the maximum. Compaction keeps the same order as descending erase.
    pub fn repel(
        self,
        current: &PositionedZone,
        placed: &[PositionedZone],
        zones: &[Zone],
        candidates: &mut Vec<WorldPosition>,
    ) -> Result<(), LayoutError> {
        if self.rules == Ruleset::Complete
            || !zones[current.id.index()].options().zone_repulsion
            || !placed
                .iter()
                .any(|p| zones[p.id.index()].options().zone_repulsion)
        {
            return Ok(());
        }
        let score = |candidate| {
            repulsion_score(
                candidate,
                zone_size(&zones[current.id.index()]),
                placed
                    .iter()
                    .filter(|p| p.id != current.id && zones[p.id.index()].options().zone_repulsion)
                    .map(|p| (p.position, zone_size(&zones[p.id.index()]))),
            )
        };
        let mut best = -1;
        for &candidate in candidates.iter() {
            best = best.max(score(candidate)?);
        }
        let mut kept = 0;
        for index in 0..candidates.len() {
            if score(candidates[index])? >= best.wrapping_sub(3) {
                candidates[kept] = candidates[index];
                kept += 1;
            }
        }
        candidates.truncate(kept);
        Ok(())
    }
}

// RVA 0x1bd5c0: size products are 32-bit BEFORE sign extension. The
// accumulated square sum is converted to double as an unsigned 64-bit value.
#[expect(
    clippy::cast_sign_loss,
    clippy::cast_possible_truncation,
    clippy::cast_precision_loss,
    reason = "native unsigned conversion and low-DWORD retention"
)]
fn spacing(
    sizes: impl Iterator<Item = i32>,
    levels: Levels,
    water: Water,
    sparseness: f64,
) -> Result<i32, LayoutError> {
    let (mut sum, mut squares, mut count) = (0_i64, 0_i64, 0_u64);
    for size in sizes {
        sum = sum.wrapping_add(i64::from(size));
        squares = squares.wrapping_add(i64::from(size.wrapping_mul(size).wrapping_mul(2)));
        count += 1;
    }
    let mean = (sum as u64).checked_div(count).unwrap_or(0) as i32;
    if levels == Levels::Surface {
        squares = squares.wrapping_mul(2);
    }
    let mut extent =
        truncate_i32(((squares as u64) as f64).sqrt())?.wrapping_add(mean.wrapping_mul(2));
    if water == Water::Islands {
        extent = extent.wrapping_mul(2);
    }
    truncate_i32(f64::from(extent) * sparseness)
}

// RVA 0x1be4e0: integer weighted centroid, then source-ordered adjustments.
#[expect(
    clippy::cast_possible_truncation,
    reason = "native signed division retains the low DWORD"
)]
fn weighted_origin(
    zones: impl Iterator<Item = (Point, i32)> + Clone,
    span: i32,
) -> Result<Point, LayoutError> {
    let (mut x, mut y, mut weight) = (0_i64, 0_i64, 0_i64);
    for (point, size) in zones.clone() {
        let square = i64::from(size.wrapping_mul(size));
        x = x.wrapping_add(i64::from(point.x).wrapping_mul(square));
        y = y.wrapping_add(i64::from(point.y).wrapping_mul(square));
        weight = weight.wrapping_add(square);
    }
    // At 0x1be611 the zero-weight branch keeps both accumulated low
    // DWORDs. The recovered C++ approximation resets them to zero instead.
    let mut center = if weight == 0 {
        Point::new(x as i32, y as i32)
    } else {
        Point::new(
            x.checked_div(weight).ok_or(LayoutError::Arithmetic)? as i32,
            y.checked_div(weight).ok_or(LayoutError::Arithmetic)? as i32,
        )
    };
    let half = span / 2;
    for (point, size) in zones {
        let reach = size.wrapping_sub(size / 4);
        for (coordinate, center) in [(point.x, &mut center.x), (point.y, &mut center.y)] {
            if coordinate
                .wrapping_sub(reach)
                .wrapping_sub(center.wrapping_sub(half))
                < 0
            {
                *center = coordinate.wrapping_sub(reach).wrapping_add(half);
            }
            if coordinate
                .wrapping_add(reach)
                .wrapping_add(1)
                .wrapping_sub(center.wrapping_add(half))
                > 0
            {
                *center = coordinate
                    .wrapping_add(reach)
                    .wrapping_add(1)
                    .wrapping_sub(half);
            }
        }
    }
    Ok(Point::new(
        center.x.wrapping_sub(half),
        center.y.wrapping_sub(half),
    ))
}

fn repulsion_score(
    candidate: WorldPosition,
    size: i32,
    others: impl Iterator<Item = (WorldPosition, i32)>,
) -> Result<i64, LayoutError> {
    let mut score = 0_i64;
    for (other, other_size) in others {
        score = score.wrapping_add(i64::from(candidate.point.distance(other.point)?));
        if candidate.level != other.level {
            let separation = i64::from(size.wrapping_add(other_size).wrapping_mul(2));
            score = score.wrapping_add(separation.wrapping_mul(separation));
        }
    }
    Ok(score)
}

fn truncate_i32(value: f64) -> Result<i32, LayoutError> {
    if !value.is_finite()
        || value.trunc() < f64::from(i32::MIN)
        || value.trunc() > f64::from(i32::MAX)
    {
        return Err(LayoutError::Arithmetic);
    }
    #[allow(clippy::cast_possible_truncation)]
    Ok(value as i32)
}

#[cfg(test)]
mod tests;
