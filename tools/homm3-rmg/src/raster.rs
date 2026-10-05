//! Clipping and rasterization of zone polygons and island coasts.

mod shape;
use shape::BoundaryShape;

use crate::{
    behavior::Behavior,
    domain::Level,
    geometry::{BoundaryEdge, GeometryError, Point, ZoneId},
    request::{Levels, MapSize, Water},
    rng::RetailRng,
    rules::Ruleset,
};
use std::{collections::TryReserveError, error::Error, fmt, num::NonZeroU32};

/// Zone ownership and terrain coverage before terrain painting.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ZoneCell {
    /// No zone until a boundary or fill assigns the cell.
    pub zone: Option<ZoneId>,
    /// Paint the assigned zone's terrain here, rather than water or rock.
    pub paint_terrain: bool,
}

/// Nonempty half-open rectangle enclosing cells of one zone.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ZoneBounds {
    minimum: Point,
    maximum: Point,
}
impl ZoneBounds {
    pub(crate) fn cell(point: Point) -> Self {
        Self {
            minimum: point,
            maximum: Point::new(point.x + 1, point.y + 1),
        }
    }
    pub(crate) fn include(&mut self, point: Point) {
        self.minimum.x = self.minimum.x.min(point.x);
        self.minimum.y = self.minimum.y.min(point.y);
        self.maximum.x = self.maximum.x.max(point.x + 1);
        self.maximum.y = self.maximum.y.max(point.y + 1);
    }
    /// Shared cells of two rectangles, or no overlap. The result stays nonempty.
    #[must_use]
    pub fn intersection(self, other: Self) -> Option<Self> {
        let minimum = Point::new(
            self.minimum.x.max(other.minimum.x),
            self.minimum.y.max(other.minimum.y),
        );
        let maximum = Point::new(
            self.maximum.x.min(other.maximum.x),
            self.maximum.y.min(other.maximum.y),
        );
        (minimum.x < maximum.x && minimum.y < maximum.y).then_some(Self { minimum, maximum })
    }
    /// Included top-left cell.
    #[must_use]
    pub const fn minimum(self) -> Point {
        self.minimum
    }
    /// Exclusive bottom-right coordinate.
    #[must_use]
    pub const fn maximum(self) -> Point {
        self.maximum
    }
}

/// Rasterization failed before a valid map cell could be written.
#[derive(Debug)]
pub enum RasterError {
    /// Clipping or an intermediate arithmetic operation is unsupported.
    Arithmetic,
    /// A path would address a cell outside this map or an absent plane.
    OutsideMap(Point, Level),
    /// An owned Voronoi ring lacks a required dual vertex.
    MissingVertex,
    /// A clipped ring does not return to its starting edge.
    UnclosedRing,
    /// Retail attempts a random draw with nonpositive displacement range.
    ZeroRoughness,
    /// The versioned irregular boundary requires its two Voronoi sites.
    MissingShape,
    /// A distance calculation failed.
    Geometry(GeometryError),
    /// Scratch or cell storage could not be reserved.
    Allocation(TryReserveError),
}
impl fmt::Display for RasterError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Arithmetic => f.write_str("unsupported boundary arithmetic"),
            Self::OutsideMap(point, level) => {
                write!(f, "boundary cell outside map: {point:?} on {level:?}")
            }
            Self::MissingVertex => f.write_str("zone boundary has no Voronoi vertex"),
            Self::UnclosedRing => f.write_str("clipped zone boundary does not close"),
            Self::ZeroRoughness => f.write_str("retail draws with zero boundary roughness"),
            Self::MissingShape => f.write_str("irregular boundary requires its Voronoi shape"),
            Self::Geometry(error) => error.fmt(f),
            Self::Allocation(error) => error.fmt(f),
        }
    }
}
impl Error for RasterError {}
impl From<GeometryError> for RasterError {
    fn from(value: GeometryError) -> Self {
        Self::Geometry(value)
    }
}
impl From<TryReserveError> for RasterError {
    fn from(value: TryReserveError) -> Self {
        Self::Allocation(value)
    }
}

/// Half-open square map bounds, constructed from a supported map size.
#[derive(Clone, Copy, Debug)]
pub struct MapBounds {
    side: i32,
}
impl MapBounds {
    /// Bounds from `(0, 0)` through `(side - 1, side - 1)` inclusive.
    #[must_use]
    #[expect(
        clippy::missing_panics_doc,
        reason = "the closed map-size domain fits i32"
    )]
    pub fn new(size: MapSize) -> Self {
        Self {
            side: i32::try_from(size.dimension()).unwrap(),
        }
    }
    /// Whether a signed plane point is inside these bounds.
    #[must_use]
    pub fn contains(self, point: Point) -> bool {
        (0..self.side).contains(&point.x) && (0..self.side).contains(&point.y)
    }
    /// Nearest map point, clamping the axes independently.
    #[must_use]
    pub fn clamp(self, point: Point) -> Point {
        Point::new(
            point.x.clamp(0, self.side - 1),
            point.y.clamp(0, self.side - 1),
        )
    }
    /// Clip toward the original opposite endpoint, in left/top/right/bottom order.
    /// Rejected clips return the original point, as in the C++ helper.
    ///
    /// # Errors
    /// Reports overflow or undefined signed division in clipping arithmetic.
    pub fn clip(self, point: Point, toward: Point) -> Result<Point, RasterError> {
        self.clip_for(point, toward, Ruleset::Complete)
    }
    /// Clip with the selected version's segment-rejection rule.
    ///
    /// # Errors
    /// Reports unsupported clipping arithmetic.
    pub fn clip_for(
        self,
        point: Point,
        toward: Point,
        rules: Ruleset,
    ) -> Result<Point, RasterError> {
        let clipped = self.clip_original(point, toward)?;
        // HotA RVA 0x1cb120: reject intersections beyond both original ends.
        if rules == Ruleset::HotA181
            && !self.contains(point)
            && !self.contains(toward)
            && (clipped.x < point.x.min(toward.x)
                || clipped.x > point.x.max(toward.x)
                || clipped.y < point.y.min(toward.y)
                || clipped.y > point.y.max(toward.y))
        {
            return Ok(point);
        }
        Ok(clipped)
    }
    fn clip_original(self, point: Point, toward: Point) -> Result<Point, RasterError> {
        if self.contains(point) {
            return Ok(point);
        }
        let delta = subtract(toward, point)?;
        let mut clipped = point;
        for (horizontal, maximum) in [(true, false), (false, false), (true, true), (false, true)] {
            let (value, divisor) = if horizontal {
                (clipped.x, delta.x)
            } else {
                (clipped.y, delta.y)
            };
            let outside = if maximum {
                value >= self.side
            } else {
                value < 0
            };
            if outside && divisor != 0 {
                let edge = if maximum { self.side - 1 } else { 0 };
                clipped = add(
                    clipped,
                    multiply_divide(
                        delta,
                        edge.checked_sub(value).ok_or(RasterError::Arithmetic)?,
                        divisor,
                    )?,
                )?;
                let (original_axis, clipped_axis) = if horizontal {
                    (point.y, clipped.y)
                } else {
                    (point.x, clipped.x)
                };
                if (original_axis >= 0 && clipped_axis < 0)
                    || (original_axis < self.side && clipped_axis >= self.side)
                {
                    return Ok(point);
                }
            }
        }
        Ok(clipped)
    }
}

/// Flat cells in X, Y, level order. Resetting retains existing capacity.
#[derive(Debug)]
pub struct ZoneRaster {
    bounds: MapBounds,
    levels: Levels,
    rules: Ruleset,
    cells: Vec<ZoneCell>,
}
impl ZoneRaster {
    /// Allocate empty zone ownership for a supported map shape.
    ///
    /// # Errors
    /// Reports a failed reservation.
    pub fn new(size: MapSize, levels: Levels) -> Result<Self, RasterError> {
        Self::new_for(size, levels, Ruleset::Complete)
    }
    /// Allocate ownership using a versioned boundary policy.
    ///
    /// # Errors
    /// Reports a failed reservation.
    pub fn new_for(size: MapSize, levels: Levels, rules: Ruleset) -> Result<Self, RasterError> {
        let mut grid = Self {
            rules,
            bounds: MapBounds::new(size),
            levels,
            cells: Vec::new(),
        };
        grid.reset_for(size, levels, rules)?;
        Ok(grid)
    }
    /// Clear ownership and resize only when the requested shape needs it.
    ///
    /// # Errors
    /// Reports a failed reservation.
    pub fn reset(&mut self, size: MapSize, levels: Levels) -> Result<(), RasterError> {
        self.reset_for(size, levels, Ruleset::Complete)
    }
    /// Clear ownership and select the boundary policy for the next map.
    ///
    /// # Errors
    /// Reports a failed reservation.
    pub fn reset_for(
        &mut self,
        size: MapSize,
        levels: Levels,
        rules: Ruleset,
    ) -> Result<(), RasterError> {
        let dimension = size.dimension() as usize;
        let count = dimension * dimension * levels.count() as usize;
        self.cells
            .try_reserve(count.saturating_sub(self.cells.len()))?;
        self.cells.resize(count, ZoneCell::default());
        self.cells.fill(ZoneCell::default());
        self.bounds = MapBounds::new(size);
        self.levels = levels;
        self.rules = rules;
        Ok(())
    }
    /// Raster cells in wire traversal order, with no row allocations.
    #[must_use]
    pub fn cells(&self) -> &[ZoneCell] {
        &self.cells
    }
    pub(crate) fn cells_mut(&mut self) -> &mut [ZoneCell] {
        &mut self.cells
    }
    /// Bounds shared by both planes.
    #[must_use]
    pub const fn bounds(&self) -> MapBounds {
        self.bounds
    }
    /// Cells per row on each plane.
    #[must_use]
    #[expect(
        clippy::missing_panics_doc,
        reason = "private bounds always have a supported positive dimension"
    )]
    pub fn dimension(&self) -> usize {
        usize::try_from(self.bounds.side).unwrap()
    }
    fn index(&self, point: Point, level: Level) -> Result<usize, RasterError> {
        if !self.bounds.contains(point)
            || (level == Level::Underground && self.levels == Levels::Surface)
        {
            return Err(RasterError::OutsideMap(point, level));
        }
        let side = usize::try_from(self.bounds.side).unwrap();
        Ok(
            (level.index() * side + usize::try_from(point.y).unwrap()) * side
                + usize::try_from(point.x).unwrap(),
        )
    }
    fn cell(&mut self, point: Point, level: Level) -> Result<&mut ZoneCell, RasterError> {
        let index = self.index(point, level)?;
        Ok(&mut self.cells[index])
    }
    fn assign(
        &mut self,
        point: Point,
        level: Level,
        zone: ZoneId,
        mark_terrain: bool,
    ) -> Result<(), RasterError> {
        let cell = self.cell(point, level)?;
        cell.zone = Some(zone);
        cell.paint_terrain |= mark_terrain;
        Ok(())
    }
}

/// How a stroke changes zone ownership and terrain coverage.
#[derive(Clone, Copy, Debug)]
pub enum Stroke {
    /// Assign boundary cells to a zone, optionally painting its terrain.
    Zone {
        /// Whether this plane receives terrain from its zone boundaries.
        paint_terrain: bool,
    },
    /// Only mark cells already belonging to this zone; displace half as far.
    Island,
}

/// Native boundary displacement also used by junction paths.
#[derive(Clone, Copy, Debug)]
pub(crate) enum BoundaryDisplacement {
    Full,
    Half,
}

// Returns no midpoint at a terminal segment. The caller pushes the far endpoint
// before the midpoint, walks the near half first, and emits only terminal FROM.
//
//   from -------- midpoint -------- to
//      next half       pending half
pub(crate) fn boundary_midpoint(
    from: Point,
    to: Point,
    roughness: u32,
    displacement: BoundaryDisplacement,
    behavior: Behavior,
    rng: &mut RetailRng,
) -> Result<Option<Point>, RasterError> {
    let mut midpoint = from
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
    if length > 1 {
        let divisor = if matches!(displacement, BoundaryDisplacement::Half) {
            2
        } else {
            1
        };
        let limit =
            (u32::try_from(length).map_err(|_| RasterError::Arithmetic)? / divisor).min(roughness);
        let limit = if behavior.is_hotfix() {
            limit.max(1)
        } else {
            limit
        };
        let Some(count) = NonZeroU32::new(limit) else {
            // rand runs before retail's division by zero.
            rng.draw();
            return Err(RasterError::ZeroRoughness);
        };
        let displacement = rng.centered_offset(count);
        midpoint = add(
            midpoint,
            multiply_divide(perpendicular, displacement, length)?,
        )?;
    }
    Ok(Some(midpoint))
}

/// Reusable boundary/fill worklists and the most recently traced polygon.
#[derive(Default, Debug)]
pub struct RasterWorkspace {
    pending: Vec<Point>,
    ring: Vec<BoundaryEdge>,
    polygon: Vec<Point>,
}
impl RasterWorkspace {
    /// Bresenham line with half-major initial error. Complete overwrites final
    /// ownership without painting it; `HotA` preserves owned endpoints and
    /// paints the final cell even when it belongs to another zone.
    ///
    /// # Errors
    /// Reports coordinates outside the map or unsupported arithmetic.
    pub fn straight(
        grid: &mut ZoneRaster,
        mut from: Point,
        mut to: Point,
        zone: ZoneId,
        level: Level,
        paint: bool,
    ) -> Result<(), RasterError> {
        if !grid.bounds.contains(from) {
            return Err(RasterError::OutsideMap(from, level));
        }
        if !grid.bounds.contains(to) {
            return Err(RasterError::OutsideMap(to, level));
        }
        if from.x > to.x {
            std::mem::swap(&mut from, &mut to);
        }
        let dx = to.x - from.x;
        let dy = to.y - from.y;
        let vertical = dy.abs();
        let step_y = if dy > 0 { 1 } else { -1 };
        let (major, minor, axial) = if dx > vertical {
            (dx, vertical, Point::new(1, 0))
        } else {
            (vertical, dx, Point::new(0, step_y))
        };
        let diagonal = Point::new(1, step_y);
        let mut error = major / 2;
        let mut first = true;
        while from != to {
            // HotA RVA 0x1cb570 preserves an already-owned first cell.
            if grid.rules == Ruleset::Complete || !first || grid.cell(from, level)?.zone.is_none() {
                grid.assign(from, level, zone, paint)?;
            }
            first = false;
            error += minor;
            if error < major {
                from = add(from, axial)?;
            } else {
                error -= major;
                from = add(from, diagonal)?;
            }
        }
        if grid.rules == Ruleset::HotA181 {
            // RVA 0x1cb5b0/0x1cb550: preserve final ownership but mark terrain.
            let cell = grid.cell(to, level)?;
            cell.zone.get_or_insert(zone);
            cell.paint_terrain |= paint;
            Ok(())
        } else {
            grid.assign(to, level, zone, false)
        }
    }

    /// Depth-first midpoint displacement, preserving draw order and rounding.
    /// For `HotA` zone strokes use [`Self::trace`], which supplies the two sites
    /// needed to constrain displacement. Island strokes do not need those sites.
    ///
    /// # Errors
    /// Reports a missing shape, invalid arithmetic, retail zero roughness,
    /// or reservation failure.
    #[expect(
        clippy::too_many_arguments,
        reason = "one boundary stroke carries geometry, ownership and RNG policy"
    )]
    pub fn irregular(
        &mut self,
        grid: &mut ZoneRaster,
        from: Point,
        to: Point,
        zone: ZoneId,
        level: Level,
        roughness: u32,
        stroke: Stroke,
        behavior: Behavior,
        rng: &mut RetailRng,
    ) -> Result<(), RasterError> {
        self.irregular_inner(
            grid, from, to, zone, level, roughness, stroke, behavior, None, rng,
        )
    }

    #[expect(
        clippy::too_many_arguments,
        reason = "one stroke plus its optional edge-local displacement limits"
    )]
    fn irregular_inner(
        &mut self,
        grid: &mut ZoneRaster,
        mut from: Point,
        to: Point,
        zone: ZoneId,
        level: Level,
        roughness: u32,
        stroke: Stroke,
        behavior: Behavior,
        shape: Option<BoundaryShape>,
        rng: &mut RetailRng,
    ) -> Result<(), RasterError> {
        let constrained = grid.rules == Ruleset::HotA181 && matches!(stroke, Stroke::Zone { .. });
        if constrained && shape.is_none() {
            return Err(RasterError::MissingShape);
        }
        if constrained {
            // RVA 0x1cb500: claim the unclamped end before subdivision.
            let cell = grid.cell(to, level)?;
            if cell.zone.is_none() {
                cell.zone = Some(zone);
                if let Stroke::Zone { paint_terrain } = stroke {
                    cell.paint_terrain |= paint_terrain;
                }
            }
        }
        let mut first = true;
        self.pending.clear();
        self.push(to)?;
        while let Some(to) = self.pending.pop() {
            let displacement = if matches!(stroke, Stroke::Island) {
                BoundaryDisplacement::Half
            } else {
                BoundaryDisplacement::Full
            };
            let midpoint = if let Some(shape) = shape {
                shape.midpoint(from, to, roughness, rng)?
            } else {
                boundary_midpoint(from, to, roughness, displacement, behavior, rng)?
            };
            let Some(midpoint) = midpoint else {
                let point = grid.bounds.clamp(from);
                match stroke {
                    Stroke::Zone { paint_terrain } => {
                        // RVA 0x1cb4c0: preserve only the first terminal cell.
                        if !constrained || !first || grid.cell(point, level)?.zone.is_none() {
                            grid.assign(point, level, zone, paint_terrain)?;
                        }
                        first = false;
                    }
                    Stroke::Island => {
                        let cell = grid.cell(point, level)?;
                        if cell.zone == Some(zone) {
                            cell.paint_terrain = true;
                        }
                    }
                }
                from = to;
                continue;
            };
            self.push(to)?;
            self.push(midpoint)?;
        }
        Ok(())
    }

    fn push(&mut self, point: Point) -> Result<(), RasterError> {
        self.pending.try_reserve(1)?;
        self.pending.push(point);
        Ok(())
    }
    fn vertex(&self, index: usize) -> Result<Point, RasterError> {
        self.ring[index].vertex.ok_or(RasterError::MissingVertex)
    }
    fn polygon_point(&mut self, point: Point) -> Result<(), RasterError> {
        self.polygon.try_reserve(1)?;
        self.polygon.push(point);
        Ok(())
    }

    /// Clip a site's Voronoi ring, draw its owned edges and retain its polygon.
    /// Shared edges are drawn only by the lower-indexed zone.
    ///
    /// # Errors
    /// Reports missing vertices, an invalid clipped ring, arithmetic or storage failure.
    #[expect(
        clippy::too_many_arguments,
        clippy::too_many_lines,
        reason = "tracing combines an immutable diagram with one mutable raster"
    )]
    pub fn trace(
        &mut self,
        grid: &mut ZoneRaster,
        edges: impl IntoIterator<Item = BoundaryEdge>,
        zone: ZoneId,
        level: Level,
        water: Water,
        irregular: bool,
        sizes: &[u32],
        behavior: Behavior,
        rng: &mut RetailRng,
    ) -> Result<&[Point], RasterError> {
        self.ring.clear();
        self.polygon.clear();
        for edge in edges {
            self.ring.try_reserve(1)?;
            self.ring.push(edge);
        }
        if self.ring.is_empty() {
            return Err(RasterError::UnclosedRing);
        }
        let bounds = grid.bounds;
        let paint = level == Level::Underground || water != Water::Islands;
        let last = bounds.side - 1;
        let upper_left = Point::new(0, 0);
        let upper_right = Point::new(last, 0);
        let lower_left = Point::new(0, last);
        let lower_right = Point::new(last, last);
        let next = |index| (index + 1) % self.ring.len();
        let mut start = None;
        for index in 0..self.ring.len() {
            let original_from = self.vertex(index)?;
            let original_to = self.vertex(next(index))?;
            let from = bounds.clip_for(original_from, original_to, grid.rules)?;
            let to = bounds.clip_for(original_to, original_from, grid.rules)?;
            if bounds.contains(from) && from != to {
                start = Some(index);
                break;
            }
        }
        let Some(first) = start else {
            for (from, to) in [
                (lower_right, upper_right),
                (upper_right, upper_left),
                (upper_left, lower_left),
                (lower_left, lower_right),
            ] {
                Self::straight(grid, from, to, zone, level, paint)?;
                self.polygon_point(from)?;
            }
            return Ok(&self.polygon);
        };
        let mut edge = first;
        for _ in 0..self.ring.len() {
            let mut next = (edge + 1) % self.ring.len();
            let neighbour = self.ring[next].opposite_zone;
            let original_from = self.vertex(edge)?;
            let original_to = self.vertex(next)?;
            let mut from = bounds.clip_for(original_from, original_to, grid.rules)?;
            let mut to = bounds.clip_for(original_to, original_from, grid.rules)?;
            self.polygon_point(from)?;
            if neighbour.is_none_or(|other| other > zone) {
                let roughness = *sizes.get(zone.index()).ok_or(RasterError::UnclosedRing)?;
                let roughness = if let Some(other) = neighbour {
                    roughness.min(*sizes.get(other.index()).ok_or(RasterError::UnclosedRing)?)
                } else {
                    roughness
                };
                if irregular {
                    let shape = if grid.rules == Ruleset::HotA181 {
                        Some(BoundaryShape::new(
                            from,
                            to,
                            self.ring[next].site,
                            self.ring[next].opposite_site,
                        )?)
                    } else {
                        None
                    };
                    self.irregular_inner(
                        grid,
                        from,
                        to,
                        zone,
                        level,
                        roughness,
                        Stroke::Zone {
                            paint_terrain: paint,
                        },
                        behavior,
                        shape,
                        rng,
                    )?;
                } else {
                    Self::straight(grid, from, to, zone, level, paint)?;
                }
            }
            edge = next;
            if to != original_to {
                from = to;
                let mut found = false;
                for _ in 0..self.ring.len() {
                    next = (next + 1) % self.ring.len();
                    to = bounds.clip_for(self.vertex(edge)?, self.vertex(next)?, grid.rules)?;
                    if bounds.contains(to) {
                        found = true;
                        break;
                    }
                    edge = next;
                }
                if !found {
                    return Err(RasterError::UnclosedRing);
                }
                self.border(grid, from, to, zone, level, paint)?;
            }
            if edge == first {
                return Ok(&self.polygon);
            }
        }
        Err(RasterError::UnclosedRing)
    }

    fn border(
        &mut self,
        grid: &mut ZoneRaster,
        mut from: Point,
        to: Point,
        zone: ZoneId,
        level: Level,
        mark_terrain: bool,
    ) -> Result<(), RasterError> {
        let last = grid.bounds.side - 1;
        // Follow the map border clockwise, one corner at a time.
        // North is up:  upperLeft -> upperRight
        //                   ^             v
        //               lowerLeft <- lowerRight
        for pass in 0..4 {
            let same_edge = |a: i32, b: i32| a == b && (a == 0 || a == last);
            let finished = if grid.rules == Ruleset::HotA181 {
                (if pass == 0 {
                    same_edge(from.x, to.x)
                } else {
                    from.x == to.x
                }) || same_edge(from.y, to.y)
            } else {
                from.x == to.x || from.y == to.y
            };
            if finished {
                Self::straight(grid, from, to, zone, level, mark_terrain)?;
                self.polygon_point(from)?;
                return Ok(());
            }
            let corner = if from.x == 0 && from != Point::new(0, 0) {
                Point::new(0, 0)
            } else if from.y == 0 && from != Point::new(last, 0) {
                Point::new(last, 0)
            } else if from.x == last && from != Point::new(last, last) {
                Point::new(last, last)
            } else {
                Point::new(0, last)
            };
            Self::straight(grid, from, corner, zone, level, mark_terrain)?;
            self.polygon_point(from)?;
            from = corner;
        }
        Err(RasterError::UnclosedRing)
    }

    /// Four-connected scanline fill. If the zone centre is outside the map,
    /// clip toward the adjacent interior site with greatest edge clearance.
    ///
    /// # Errors
    /// Reports unsupported clipping, an invalid seed, or reservation failure.
    pub fn fill_zone(
        &mut self,
        grid: &mut ZoneRaster,
        mut position: Point,
        level: Level,
        zone: ZoneId,
        mark_terrain: bool,
        edges: impl IntoIterator<Item = BoundaryEdge>,
    ) -> Result<(), RasterError> {
        if !grid.bounds.contains(position) {
            let mut best = None;
            let mut clearance = 0;
            // C++ tests the initial ring edge last.
            let mut edges = edges.into_iter();
            let first = edges.next();
            for edge in edges.chain(first) {
                let point = edge.opposite_site;
                let margin = point
                    .x
                    .min(grid.bounds.side - point.x - 1)
                    .min(point.y)
                    .min(grid.bounds.side - point.y - 1);
                if margin > clearance {
                    clearance = margin;
                    best = Some(point);
                }
            }
            let Some(best) = best else {
                return Ok(());
            };
            position = grid.bounds.clip_for(position, best, grid.rules)?;
        }
        self.pending.clear();
        self.push(position)?;
        while let Some(mut point) = self.pending.pop() {
            while point.x > 0
                && grid
                    .cell(Point::new(point.x - 1, point.y), level)?
                    .zone
                    .is_none()
            {
                point.x -= 1;
            }
            let mut upper = None;
            let mut lower = None;
            while point.x < grid.bounds.side && grid.cell(point, level)?.zone.is_none() {
                grid.assign(point, level, zone, mark_terrain)?;
                if point.y > 0 {
                    self.scan_span(grid, Point::new(point.x, point.y - 1), level, &mut upper)?;
                }
                if point.y < grid.bounds.side - 1 {
                    self.scan_span(grid, Point::new(point.x, point.y + 1), level, &mut lower)?;
                }
                point.x += 1;
            }
            if let Some(seed) = upper {
                self.push(seed)?;
            }
            if let Some(seed) = lower {
                self.push(seed)?;
            }
        }
        Ok(())
    }

    fn scan_span(
        &mut self,
        grid: &mut ZoneRaster,
        point: Point,
        level: Level,
        seed: &mut Option<Point>,
    ) -> Result<(), RasterError> {
        if grid.cell(point, level)?.zone.is_none() {
            seed.get_or_insert(point);
        } else if let Some(point) = seed.take() {
            self.push(point)?;
        }
        Ok(())
    }

    /// Inset the polygon toward the recentered zone, then mark its island.
    /// Every surface zone is processed, including added water zones.
    ///
    /// # Errors
    /// Reports an empty polygon, arithmetic, zero retail roughness or storage failure.
    #[expect(
        clippy::too_many_arguments,
        reason = "island stroke inherits zone identity, geometry and behavior"
    )]
    pub fn inset_island(
        &mut self,
        grid: &mut ZoneRaster,
        center: Point,
        zone: ZoneId,
        polygon: impl DoubleEndedIterator<Item = Point>,
        scaled_size: u32,
        behavior: Behavior,
        rng: &mut RetailRng,
    ) -> Result<(), RasterError> {
        let mut polygon = polygon;
        let first = polygon.next().ok_or(RasterError::UnclosedRing)?;
        let mut previous = inset_point(first, center)?;
        for vertex in polygon.rev().chain(std::iter::once(first)) {
            let point = inset_point(vertex, center)?;
            self.irregular(
                grid,
                point,
                previous,
                zone,
                Level::Surface,
                scaled_size / 2,
                Stroke::Island,
                behavior,
                rng,
            )?;
            previous = point;
        }
        self.pending.clear();
        self.push(center)?;
        while let Some(point) = self.pending.pop() {
            // Cardinal flood order is east, south, west, north, as in RMG.
            for offset in [
                Point::new(1, 0),
                Point::new(0, 1),
                Point::new(-1, 0),
                Point::new(0, -1),
            ] {
                let next = add(point, offset)?;
                if !grid.bounds.contains(next) {
                    continue;
                }
                let cell = grid.cell(next, Level::Surface)?;
                if cell.paint_terrain || cell.zone != Some(zone) {
                    continue;
                }
                cell.paint_terrain = true;
                self.push(next)?;
            }
        }
        Ok(())
    }
}

fn inset_point(point: Point, center: Point) -> Result<Point, RasterError> {
    let delta = subtract(center, point)?;
    let length = delta.distance(Point::new(0, 0))?;
    if length == 0 {
        return Ok(point);
    }
    let displacement = 4.max(length / 4).min(length / 2);
    add(point, multiply_divide(delta, displacement, length)?)
}

fn add(first: Point, second: Point) -> Result<Point, RasterError> {
    first.checked_add(second).ok_or(RasterError::Arithmetic)
}
fn subtract(first: Point, second: Point) -> Result<Point, RasterError> {
    first.checked_sub(second).ok_or(RasterError::Arithmetic)
}
fn multiply_divide(point: Point, numerator: i32, denominator: i32) -> Result<Point, RasterError> {
    point
        .checked_scale_ratio(numerator, denominator)
        .ok_or(RasterError::Arithmetic)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::behavior::RetailProfile;

    #[test]
    fn versioned_boundaries_preserve_owned_endpoints_with_distinct_paint_rules() {
        let mut grid =
            ZoneRaster::new_for(MapSize::Small, Levels::Surface, Ruleset::HotA181).unwrap();
        let own = ZoneId::new(0);
        let other = ZoneId::new(1);
        let from = Point::new(3, 3);
        let to = Point::new(3, 7);
        let level = Level::Surface;
        let mut work = RasterWorkspace::default();
        for straight in [true, false] {
            grid.reset_for(MapSize::Small, Levels::Surface, Ruleset::HotA181)
                .unwrap();
            grid.assign(from, level, other, false).unwrap();
            grid.assign(to, level, other, false).unwrap();
            if straight {
                RasterWorkspace::straight(&mut grid, from, to, own, level, true).unwrap();
            } else {
                let mut rng = RetailRng::new(1);
                let shape =
                    BoundaryShape::new(from, to, Point::new(0, 5), Point::new(6, 5)).unwrap();
                work.irregular_inner(
                    &mut grid,
                    from,
                    to,
                    own,
                    level,
                    0,
                    Stroke::Zone {
                        paint_terrain: true,
                    },
                    Behavior::Retail(RetailProfile::default()),
                    Some(shape),
                    &mut rng,
                )
                .unwrap();
                assert_eq!(rng.draws(), 0);
            }
            assert_eq!(
                *grid.cell(from, level).unwrap(),
                ZoneCell {
                    zone: Some(other),
                    paint_terrain: false
                }
            );
            assert_eq!(
                *grid.cell(to, level).unwrap(),
                ZoneCell {
                    zone: Some(other),
                    paint_terrain: straight
                }
            );
            for y in 4..7 {
                assert_eq!(
                    *grid.cell(Point::new(3, y), level).unwrap(),
                    ZoneCell {
                        zone: Some(own),
                        paint_terrain: true
                    }
                );
            }
        }
    }

    #[test]
    fn segment_clipping_and_corner_walk_do_not_cross_the_map_interior() {
        let bounds = MapBounds::new(MapSize::Small);
        let from = Point::new(-3, 2);
        let to = Point::new(-1, 2);
        assert_eq!(bounds.clip(from, to).unwrap(), Point::new(0, 2));
        assert_eq!(bounds.clip_for(from, to, Ruleset::HotA181).unwrap(), from);
        assert_eq!(
            bounds
                .clip_for(from, Point::new(40, 2), Ruleset::HotA181)
                .unwrap(),
            Point::new(0, 2)
        );
        let mut grid =
            ZoneRaster::new_for(MapSize::Small, Levels::Surface, Ruleset::HotA181).unwrap();
        let mut work = RasterWorkspace::default();
        work.border(
            &mut grid,
            Point::new(10, 0),
            Point::new(10, 35),
            ZoneId::new(0),
            Level::Surface,
            true,
        )
        .unwrap();
        assert_eq!(
            work.polygon,
            [Point::new(10, 0), Point::new(35, 0), Point::new(35, 35)]
        );
        assert!(grid
            .cell(Point::new(10, 17), Level::Surface)
            .unwrap()
            .zone
            .is_none());
        assert!(
            grid.cell(Point::new(35, 17), Level::Surface)
                .unwrap()
                .paint_terrain
        );
    }

    #[test]
    fn straight_boundary_marks_all_but_final_cell_and_never_clears_a_mark() {
        let mut grid = ZoneRaster::new(MapSize::Small, Levels::Surface).unwrap();
        let zone = ZoneId::new(0);
        RasterWorkspace::straight(
            &mut grid,
            Point::new(0, 0),
            Point::new(3, 0),
            zone,
            Level::Surface,
            true,
        )
        .unwrap();
        assert!(grid.cells()[..3]
            .iter()
            .all(|cell| cell.paint_terrain && cell.zone == Some(zone)));
        assert_eq!(
            grid.cells()[3],
            ZoneCell {
                zone: Some(zone),
                paint_terrain: false
            }
        );
        RasterWorkspace::straight(
            &mut grid,
            Point::new(0, 0),
            Point::new(3, 0),
            zone,
            Level::Surface,
            false,
        )
        .unwrap();
        assert!(grid.cells()[0].paint_terrain);
    }

    #[test]
    fn hotfix_zero_roughness_keeps_the_draws_and_retail_reports_its_fault() {
        let mut grid = ZoneRaster::new(MapSize::Small, Levels::Surface).unwrap();
        let mut scratch = RasterWorkspace::default();
        let mut rng = RetailRng::new(1);
        let stroke = Stroke::Zone {
            paint_terrain: true,
        };
        let zone = ZoneId::new(0);
        let from = Point::new(3, 3);
        let to = Point::new(3, 7);
        assert!(matches!(
            scratch.irregular(
                &mut grid,
                from,
                to,
                zone,
                Level::Surface,
                0,
                stroke,
                Behavior::Retail(RetailProfile::default()),
                &mut rng
            ),
            Err(RasterError::ZeroRoughness)
        ));
        assert_eq!(rng.draws(), 1);
        scratch
            .irregular(
                &mut grid,
                from,
                to,
                zone,
                Level::Surface,
                0,
                stroke,
                Behavior::Hotfix,
                &mut rng,
            )
            .unwrap();
        assert_eq!(rng.draws(), 4);
        for y in 3..7 {
            assert_eq!(grid.cells()[y * 36 + 3].zone, Some(zone));
        }
        assert!(grid.cells()[7 * 36 + 3].zone.is_none());
    }

    #[test]
    fn island_stroke_cannot_mark_another_zone_and_reset_keeps_capacity() {
        let mut grid = ZoneRaster::new(MapSize::Small, Levels::Surface).unwrap();
        let capacity = grid.cells.capacity();
        let zone = ZoneId::new(0);
        let mut scratch = RasterWorkspace::default();
        grid.assign(Point::new(3, 3), Level::Surface, zone, false)
            .unwrap();
        scratch
            .irregular(
                &mut grid,
                Point::new(3, 3),
                Point::new(3, 5),
                zone,
                Level::Surface,
                1,
                Stroke::Island,
                Behavior::Hotfix,
                &mut RetailRng::new(1),
            )
            .unwrap();
        assert!(grid.cells()[3 * 36 + 3].paint_terrain);
        assert!(!grid.cells()[4 * 36 + 3].paint_terrain);
        grid.reset(MapSize::Small, Levels::Surface).unwrap();
        assert_eq!(grid.cells.capacity(), capacity);
        assert!(grid.cells().iter().all(|cell| *cell == ZoneCell::default()));
    }
}
