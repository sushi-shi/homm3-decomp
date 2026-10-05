//! Terrain painting with borrowed tiles and reusable, ordered repair queues.

mod regions;
pub use regions::TerrainRegion;

use crate::{
    boundaries::{BoundaryError, TerrainCoverage},
    domain::{Level, Terrain},
    geometry::Point,
    line::Reflection,
    raw,
    request::Water,
    rng::{RetailRng, RngCheckpoint},
    rules::Ruleset,
    terrain_rules::{
        self, BrushStrength, TerrainCatalog, TerrainRuleError, TerrainShape, TerrainTile,
        TerrainTransition,
    },
};
use std::{collections::TryReserveError, error::Error, fmt};

/// Terrain generation could not allocate, select a frame or repair zone state.
#[derive(Debug)]
pub enum TerrainError {
    /// A zone summary cannot be recomputed.
    Boundary(BoundaryError),
    /// Native guard or coordinate arithmetic cannot be represented.
    Arithmetic,
    /// The map and installed frame catalog belong to different generation rules.
    Ruleset {
        /// Map's generation rules.
        requested: crate::rules::Ruleset,
        /// Frame catalog's rules.
        catalog: crate::rules::Ruleset,
    },
    /// A source rule has no frame for the requested transition.
    Rule(TerrainRuleError),
    /// A reusable buffer could not grow.
    Allocation(TryReserveError),
}
impl fmt::Display for TerrainError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Boundary(error) => error.fmt(f),
            Self::Arithmetic => f.write_str("unsupported terrain post-pass arithmetic"),
            Self::Ruleset { requested, catalog } => write!(
                f,
                "terrain catalog {catalog:?} does not match map rules {requested:?}"
            ),
            Self::Rule(error) => error.fmt(f),
            Self::Allocation(error) => error.fmt(f),
        }
    }
}
impl Error for TerrainError {}
impl From<BoundaryError> for TerrainError {
    fn from(error: BoundaryError) -> Self {
        Self::Boundary(error)
    }
}
impl From<TerrainRuleError> for TerrainError {
    fn from(error: TerrainRuleError) -> Self {
        Self::Rule(error)
    }
}
impl From<TryReserveError> for TerrainError {
    fn from(error: TryReserveError) -> Self {
        Self::Allocation(error)
    }
}

/// Completed terrain stage, borrowing both zone state and tile storage.
pub struct PaintedTerrain<'zones, 'tiles> {
    coverage: TerrainCoverage<'zones>,
    workspace: &'tiles mut TerrainWorkspace,
    rng: RngCheckpoint,
}
impl<'zones> PaintedTerrain<'zones, '_> {
    pub(crate) fn with_brush(
        &mut self,
        level: Level,
        terrain: Terrain,
        rng: &mut RetailRng,
        paint: impl FnOnce(&TerrainCoverage<'_>, &mut Brush<'_>) -> Result<(), TerrainError>,
    ) -> Result<(), TerrainError> {
        let side = self.coverage.map().raster().dimension();
        let mut brush = self
            .workspace
            .brush(level.index() * side * side, side, terrain, rng);
        paint(&self.coverage, &mut brush)?;
        brush.finish()?;
        Ok(())
    }
    // One brush spans the full plane and finishes once after this ordered batch.
    // Callers supply admitted plane-local indices; repairs may extend beyond
    // the requested cells while staying on the same plane.
    pub(crate) fn repaint(
        &mut self,
        level: Level,
        terrain: Terrain,
        indices: impl Iterator<Item = usize>,
        rng: &mut RetailRng,
    ) -> Result<(), TerrainError> {
        self.with_brush(level, terrain, rng, |_, brush| {
            for index in indices {
                brush.paint(index)?;
            }
            Ok(())
        })
    }
    pub(crate) fn coverage_mut(&mut self) -> &mut TerrainCoverage<'zones> {
        &mut self.coverage
    }
    /// Zone state and ownership used by subsequent placement stages.
    #[must_use]
    pub const fn coverage(&self) -> &TerrainCoverage<'_> {
        &self.coverage
    }
    /// Tiles in X, Y, level traversal order.
    #[must_use]
    pub fn tiles(&self) -> &[TerrainTile] {
        &self.workspace.tiles
    }
    /// Region border/reachability state; empty under Complete rules.
    #[must_use]
    pub fn regions(&self) -> &[TerrainRegion] {
        &self.workspace.regions
    }
    /// Historical RNG checkpoint after the initial terrain stage finishes.
    #[must_use]
    pub const fn rng(&self) -> RngCheckpoint {
        self.rng
    }
}

/// Owned map tiles, region state and worklists reused across brushes and generations.
#[derive(Default, Debug)]
pub struct TerrainWorkspace {
    catalog: TerrainCatalog,
    tiles: Vec<TerrainTile>,
    repair: OrderedCells,
    other: OrderedCells,
    claimed: OrderedCells,
    regions: Vec<TerrainRegion>,
    pending: Vec<usize>,
}
impl TerrainWorkspace {
    /// Reusable painting storage using a shared, immutable frame catalog.
    #[must_use]
    pub fn with_catalog(catalog: TerrainCatalog) -> Self {
        Self {
            catalog,
            ..Self::default()
        }
    }

    /// Paint initial terrain and zones in source order, then repair zone state
    /// where required by the generation rules.
    /// Consumes coverage so the stage cannot accidentally be painted twice.
    ///
    /// # Errors
    /// Reports allocation, catalog, transition or zone-repair failures.
    #[expect(
        clippy::missing_panics_doc,
        reason = "source frame and private coverage bounds are valid by construction"
    )]
    pub fn paint<'zones, 'tiles>(
        &'tiles mut self,
        mut coverage: TerrainCoverage<'zones>,
        rng: &mut RetailRng,
    ) -> Result<PaintedTerrain<'zones, 'tiles>, TerrainError> {
        let map = coverage.map();
        if self.catalog.ruleset() != map.request().ruleset() {
            return Err(TerrainError::Ruleset {
                requested: map.request().ruleset(),
                catalog: self.catalog.ruleset(),
            });
        }
        let side = map.raster().dimension();
        let plane = side * side;
        let count = map.raster().cells().len();
        let extended = map.request().ruleset() == Ruleset::HotA181;
        let strength = if extended {
            BrushStrength::parse(1).unwrap()
        } else {
            BrushStrength::GENERATOR
        };
        let surface = if extended
            && map.template().options().rock_blocks.is_some()
            && map.water() == Water::None
        {
            Terrain::Dirt
        } else {
            Terrain::Water
        };
        let initial = self.catalog.parse_tile(
            Terrain::Water,
            u8::try_from(raw::RMG_WATER_BASE_FRAME).expect("canonical water frame fits u8"),
            Reflection::default(),
        )?;
        self.tiles
            .try_reserve(count.saturating_sub(self.tiles.len()))?;
        self.tiles.resize(count, initial);
        self.tiles.fill(initial);
        self.repair.reset(plane)?;
        self.other.reset(plane)?;
        self.claimed.reset(plane)?;
        self.regions.clear();
        // Underground is painted before surface, even though storage is surface-first.
        if count > plane {
            let mut brush = self.brush(plane, side, Terrain::Rock, rng);
            brush.initial_strength = strength;
            brush.paint_all()?;
        }
        let mut brush = self.brush(0, side, surface, rng);
        brush.initial_strength = strength;
        brush.paint_all()?;
        for zone_index in 0..coverage.map().zones().len() {
            let map = coverage.map();
            let zone = map.zones()[zone_index];
            if zone.terrain() == Terrain::Water {
                continue;
            }
            let start = zone.position().level.index() * plane;
            let mut brush = self.brush(start, side, zone.terrain(), rng);
            brush.initial_strength = strength;
            if let Some(bounds) = zone.bounds() {
                for y in bounds.minimum().y..bounds.maximum().y {
                    for x in bounds.minimum().x..bounds.maximum().x {
                        let index =
                            usize::try_from(y).unwrap() * side + usize::try_from(x).unwrap();
                        let cell = map.raster().cells()[start + index];
                        if cell.zone == Some(zone.id()) && cell.paint_terrain {
                            brush.paint(index)?;
                        }
                    }
                }
            }
            // RVA 0x1c96d0 sets ownership context only for destructor repairs.
            // Painter repairs read terrain, not zone ownership, so collect the
            // claimed cells during the flush and apply their union afterward.
            brush.capture_claims = extended;
            brush.finish()?;
            if extended {
                let map = coverage.map_mut();
                while let Some(index) = self.claimed.first() {
                    self.claimed.remove(index);
                    let cell = &mut map.raster_mut().cells_mut()[start + index];
                    cell.zone = Some(zone.id());
                    cell.paint_terrain = true;
                    map.zones_mut()[zone_index].include_cell(Point::new(
                        i32::try_from(index % side).unwrap(),
                        i32::try_from(index / side).unwrap(),
                    ));
                }
            }
        }
        if extended {
            self.finish_regions(coverage.map_mut(), rng)?;
        }
        Ok(PaintedTerrain {
            coverage,
            workspace: self,
            rng: rng.checkpoint(),
        })
    }

    fn brush<'a>(
        &'a mut self,
        start: usize,
        side: usize,
        terrain: Terrain,
        rng: &'a mut RetailRng,
    ) -> Brush<'a> {
        Brush {
            catalog: &self.catalog,
            initial_strength: BrushStrength::GENERATOR,
            capture_claims: false,
            claimed: &mut self.claimed,
            tiles: &mut self.tiles[start..start + side * side],
            side,
            terrain,
            repair: &mut self.repair,
            other: &mut self.other,
            rng,
        }
    }
}

/// Set iteration is row-major (Y, then X), matching `TRmgGridPoint` ordering.
/// Tracking the first occupied word avoids rescanning earlier empty rows.
#[derive(Default, Debug)]
struct OrderedCells {
    words: Vec<u64>,
    first: usize,
}
impl OrderedCells {
    fn reset(&mut self, cells: usize) -> Result<(), TryReserveError> {
        let count = cells.div_ceil(u64::BITS as usize);
        self.words
            .try_reserve(count.saturating_sub(self.words.len()))?;
        self.words.resize(count, 0);
        self.words.fill(0);
        self.first = count;
        Ok(())
    }
    fn contains(&self, index: usize) -> bool {
        self.words[index / 64] & (1 << (index % 64)) != 0
    }
    fn insert(&mut self, index: usize) {
        self.words[index / 64] |= 1 << (index % 64);
        self.first = self.first.min(index / 64);
    }
    fn remove(&mut self, index: usize) {
        self.words[index / 64] &= !(1 << (index % 64));
    }
    fn first(&mut self) -> Option<usize> {
        while self.first < self.words.len() {
            let word = self.words[self.first];
            if word != 0 {
                return Some(self.first * 64 + word.trailing_zeros() as usize);
            }
            self.first += 1;
        }
        None
    }
}

// A gap cell C has other terrain x on both sides along one axis. North is up:
//   horizontal  vertical
//                   x
//     x C x         C
//                   x
#[derive(Clone, Copy)]
enum Axis {
    Horizontal,
    Vertical,
}
impl Axis {
    fn perpendicular(self) -> Self {
        match self {
            Self::Horizontal => Self::Vertical,
            Self::Vertical => Self::Horizontal,
        }
    }
    fn offset(self) -> (isize, isize) {
        match self {
            Self::Horizontal => (1, 0),
            Self::Vertical => (0, 1),
        }
    }
}

pub(crate) struct Brush<'a> {
    catalog: &'a TerrainCatalog,
    initial_strength: BrushStrength,
    capture_claims: bool,
    claimed: &'a mut OrderedCells,
    tiles: &'a mut [TerrainTile],
    side: usize,
    terrain: Terrain,
    repair: &'a mut OrderedCells,
    other: &'a mut OrderedCells,
    rng: &'a mut RetailRng,
}
impl Brush<'_> {
    pub(crate) fn change_terrain(&mut self, terrain: Terrain) -> Result<(), TerrainRuleError> {
        self.finish()?;
        self.terrain = terrain;
        Ok(())
    }
    fn offset(&self, index: usize, x: isize, y: isize) -> Option<usize> {
        let x = (index % self.side).checked_add_signed(x)?;
        let y = (index / self.side).checked_add_signed(y)?;
        (x < self.side && y < self.side).then_some(y * self.side + x)
    }
    fn clamped(&self, index: usize, x: isize, y: isize) -> usize {
        let x = (index % self.side)
            .saturating_add_signed(x)
            .min(self.side - 1);
        let y = (index / self.side)
            .saturating_add_signed(y)
            .min(self.side - 1);
        y * self.side + x
    }
    pub(crate) fn terrain(&self, index: usize) -> Terrain {
        self.tiles[index].terrain()
    }
    fn strength(&self, index: usize, terrain: Terrain) -> BrushStrength {
        let mut strength = self.initial_strength;
        // W, N, E, S: each matching decorated neighbour halves strength.
        for (x, y) in [(-1, 0), (0, -1), (1, 0), (0, 1)] {
            if let Some(nearby) = self.offset(index, x, y) {
                let tile = self.tiles[nearby];
                if tile.terrain() == terrain && tile.is_special() {
                    strength = strength.halved();
                }
            }
        }
        strength
    }
    fn neighbours(&self, index: usize) -> ([TerrainTile; 8], usize) {
        let mut tiles = [self.tiles[index]; 8];
        let mut count = 0;
        if self.catalog.ruleset() == crate::rules::Ruleset::HotA181 {
            for direction in 0..8 {
                let (x, y) = direction_offset(direction);
                if let Some(nearby) = self.offset(index, x, y) {
                    tiles[count] = self.tiles[nearby];
                    count += 1;
                }
            }
        }
        (tiles, count)
    }
    fn base(&mut self, index: usize) -> Result<(), TerrainRuleError> {
        let (neighbours, count) = self.neighbours(index);
        self.tiles[index] = self.catalog.select_base(
            self.terrain,
            self.strength(index, self.terrain),
            None,
            &neighbours[..count],
            self.rng,
        )?;
        Ok(())
    }
    pub(crate) fn paint(&mut self, index: usize) -> Result<(), TerrainRuleError> {
        if self.terrain(index) == self.terrain {
            self.base(index)
        } else {
            self.paint_point(index)
        }
    }
    fn paint_all(mut self) -> Result<(), TerrainRuleError> {
        for index in 0..self.tiles.len() {
            self.paint(index)?;
        }
        self.finish()
    }
    fn paint_point(&mut self, index: usize) -> Result<(), TerrainRuleError> {
        self.base(index)?;
        if self.capture_claims {
            self.claimed.insert(index);
        }
        self.other.remove(index);
        if terrain_rules::allows_separated(self.terrain) {
            // Painting closes this axis; a remaining perpendicular gap still needs repair.
            for (x, y, axis) in [
                (0, -1, Axis::Vertical),
                (0, 1, Axis::Vertical),
                (-1, 0, Axis::Horizontal),
                (1, 0, Axis::Horizontal),
            ] {
                if let Some(nearby) = self.offset(index, x, y) {
                    if self.repair.contains(nearby)
                        && self
                            .gap(nearby, self.terrain(nearby), axis.perpendicular())
                            .is_none()
                    {
                        self.repair.remove(nearby);
                        self.queue_others(nearby);
                    }
                }
            }
        } else {
            for direction in 0..raw::TILE_DIR_COUNT as usize {
                let (x, y) = direction_offset(direction);
                if let Some(nearby) = self.offset(index, x, y) {
                    if self.terrain(nearby) != self.terrain {
                        continue;
                    }
                    if self.repair.contains(nearby) {
                        if !self.needs_repair(nearby) {
                            self.repair.remove(nearby);
                            self.queue_others(nearby);
                        }
                    } else if self.needs_repair(nearby) {
                        self.repair.insert(nearby);
                    }
                }
            }
        }
        if self.needs_repair(index) {
            self.repair.insert(index);
        } else {
            self.queue_others(index);
        }
        Ok(())
    }
    fn queue_cardinal(&mut self, index: usize, x: isize, y: isize) -> bool {
        if let Some(nearby) = self.offset(index, x, y) {
            if self.terrain(nearby) != self.terrain {
                self.other.insert(nearby);
                return true;
            }
        }
        false
    }
    fn queue_others(&mut self, index: usize) {
        // At most one neighbour per cardinal axis, preferring north and west.
        if !self.queue_cardinal(index, 0, -1) {
            self.queue_cardinal(index, 0, 1);
        }
        if !self.queue_cardinal(index, -1, 0) {
            self.queue_cardinal(index, 1, 0);
        }
        for (x, y) in [(-1, -1), (1, -1), (-1, 1), (1, 1)] {
            if let Some(nearby) = self.offset(index, x, y) {
                let terrain = self.terrain(nearby);
                if terrain != self.terrain && !terrain_rules::allows_separated(terrain) {
                    self.other.insert(nearby);
                }
            }
        }
    }
    fn gap(&self, index: usize, terrain: Terrain, axis: Axis) -> Option<(usize, usize)> {
        let (x, y) = axis.offset();
        let negative = self.offset(index, -x, -y)?;
        let positive = self.offset(index, x, y)?;
        (self.terrain(negative) != terrain && self.terrain(positive) != terrain)
            .then_some((negative, positive))
    }
    fn matching(&self, index: usize) -> [bool; raw::TILE_DIR_COUNT as usize] {
        let terrain = self.terrain(index);
        let mut matches = std::array::from_fn(|direction| {
            let (x, y) = direction_offset(direction);
            self.terrain(self.clamped(index, x, y)) == terrain
        });
        // Diagonals match only beside at least one matching cardinal neighbour.
        for direction in (1..matches.len()).step_by(2) {
            matches[direction] &=
                matches[direction - 1] || matches[(direction + 1) % matches.len()];
        }
        matches
    }
    fn separated(&self, index: usize) -> bool {
        // More than one matching run around the ring means separated neighbours.
        // North is up; # matches the centre C's terrain, . does not.
        //   . # .
        //   . C .
        //   . # .
        let matches = self.matching(index);
        (0..matches.len())
            .filter(|&d| matches[d] && !matches[(d + 1) % matches.len()])
            .count()
            > 1
    }
    fn needs_repair(&self, index: usize) -> bool {
        let terrain = self.terrain(index);
        self.gap(index, terrain, Axis::Horizontal).is_some()
            || self.gap(index, terrain, Axis::Vertical).is_some()
            || (!terrain_rules::allows_separated(terrain) && self.separated(index))
    }
    fn repair_gap(
        &mut self,
        negative: usize,
        positive: usize,
        axis: Axis,
    ) -> Result<(), TerrainRuleError> {
        let fill_positive = if self.needs_repair(negative) {
            false
        } else if self.needs_repair(positive) {
            true
        } else {
            let cross_axis = axis.perpendicular();
            self.gap(negative, self.terrain, cross_axis).is_some()
                && self.gap(positive, self.terrain, cross_axis).is_none()
        };
        self.paint_point(if fill_positive { positive } else { negative })
    }
    fn repair_point(&mut self, index: usize) -> Result<(), TerrainRuleError> {
        for axis in [Axis::Vertical, Axis::Horizontal] {
            if let Some((negative, positive)) = self.gap(index, self.terrain(index), axis) {
                self.repair_gap(negative, positive, axis)?;
            }
        }
        if terrain_rules::allows_separated(self.terrain) || !self.separated(index) {
            return Ok(());
        }
        let matches = self.matching(index);
        // Run-length encode the nonmatching ring, starting after a known
        // match so a wrapped gap stays together. At most four gaps fit.
        // Fill the lightest gaps; ties go to the first gap in source order.
        let first = matches
            .iter()
            .position(|&matched| matched)
            .expect("separated ring contains matches");
        let mut gaps = [(0, 0, 0); raw::TILE_DIR_COUNT as usize / 2];
        let mut count = 0;
        let mut direction = (first + 1) % matches.len();
        while direction != first {
            if matches[direction] {
                direction = (direction + 1) % matches.len();
                continue;
            }
            let gap = &mut gaps[count];
            gap.0 = direction;
            loop {
                gap.1 += 1;
                gap.2 += if direction % 2 == 0 { 2 } else { 1 };
                direction = (direction + 1) % matches.len();
                if direction == first || matches[direction] {
                    break;
                }
            }
            count += 1;
        }
        while count > 1 {
            let smallest = (0..count).min_by_key(|&g| gaps[g].2).unwrap();
            let (start, length, _) = gaps[smallest];
            for step in 0..length {
                let (x, y) = direction_offset((start + step) % matches.len());
                if let Some(nearby) = self.offset(index, x, y) {
                    self.paint_point(nearby)?;
                }
            }
            gaps.copy_within(smallest + 1..count, smallest);
            count -= 1;
        }
        Ok(())
    }
    fn finish(&mut self) -> Result<(), TerrainRuleError> {
        loop {
            // Repair deliberately peeks; painting must remove the repaired point.
            while let Some(index) = self.repair.first() {
                self.repair_point(index)?;
            }
            while let Some(index) = self.other.first() {
                self.other.remove(index);
                if self.needs_repair(index) {
                    self.paint_point(index)?;
                }
            }
            if self.repair.first().is_none() {
                break;
            }
        }
        self.transitions()
    }
    fn transitions(&mut self) -> Result<(), TerrainRuleError> {
        // C++ counts each terrain boundary once towards E, SE, S and SW,
        // adding it to both cells. North is up; C counts each o:
        //   . . .
        //   . C o
        //   o o o
        // Only zero/nonzero is used. Terrain stays unchanged during this pass,
        // so a direct neighbour comparison removes the edge-count allocation.
        for index in 0..self.tiles.len() {
            let tile = self.tiles[index];
            let neighbours = std::array::from_fn(|direction| {
                let (x, y) = direction_offset(direction);
                terrain_rules::neighbour_kind(
                    tile.terrain(),
                    self.terrain(self.clamped(index, x, y)),
                )
            });
            let mut transition = terrain_rules::classify(&neighbours);
            self.refine_corner(index, &mut transition);
            let (neighbours, count) = self.neighbours(index);
            self.tiles[index] = if transition.shape == TerrainShape::Fill {
                self.catalog.select_base(
                    tile.terrain(),
                    self.strength(index, tile.terrain()),
                    Some(tile),
                    &neighbours[..count],
                    self.rng,
                )?
            } else {
                self.catalog.select_transition(
                    tile.terrain(),
                    transition,
                    Some(tile),
                    &neighbours[..count],
                    self.rng,
                )?
            };
        }
        Ok(())
    }
    fn refine_corner(&self, index: usize, transition: &mut TerrainTransition) {
        let x = if transition.reflection.flip_x { -1 } else { 1 };
        let y = if transition.reflection.flip_y { -1 } else { 1 };
        let terrain = self.terrain(index);
        // Outer NW corner probes: either diagonal has the cell's terrain.
        // North is up; C is the cell, e an edge, o a probe, ? not tested.
        //   ? e o
        //   e C ?
        //   o ? ?
        let outer = || {
            self.terrain(self.clamped(index, -x, y)) == terrain
                || self.terrain(self.clamped(index, x, -y)) == terrain
        };
        // Inner SE corner probes: either cell two steps away differs.
        //   C . o
        //   . e ?
        //   o ? ?
        let inner = || {
            self.terrain(self.clamped(index, 2 * x, 0)) != terrain
                || self.terrain(self.clamped(index, 0, 2 * y)) != terrain
        };
        transition.shape = match transition.shape {
            TerrainShape::NwCornerBlend if outer() => TerrainShape::NwDiagonalBlend,
            TerrainShape::NwCornerHard if outer() => TerrainShape::NwDiagonalHard,
            TerrainShape::SeBlend if inner() => TerrainShape::SeDiagonalBlend,
            TerrainShape::SeHard if inner() => TerrainShape::SeDiagonalHard,
            shape => shape,
        };
    }
}

fn direction_offset(direction: usize) -> (isize, isize) {
    // Tile classifiers start at N; the canonical movement table starts at E.
    let movement = (direction + raw::RMG_DIRECTION_NORTH as usize) % raw::DIRECTIONS.len();
    let (x, y) = raw::DIRECTIONS[movement];
    (
        isize::try_from(x).expect("unit direction offset"),
        isize::try_from(y).expect("unit direction offset"),
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::BTreeSet;

    #[test]
    fn indexed_worklist_preserves_set_order_when_earlier_cells_are_requeued() {
        let mut queue = OrderedCells::default();
        queue.reset(513).unwrap();
        let mut reference = BTreeSet::new();
        for index in [512, 127, 128, 64, 0, 65, 127, 300] {
            queue.insert(index);
            reference.insert(index);
        }
        for index in [64, 300, 10] {
            queue.remove(index);
            reference.remove(&index);
        }
        for _ in 0..3 {
            let next = reference.pop_first().unwrap();
            assert_eq!(queue.first(), Some(next));
            queue.remove(next);
        }
        for index in [1, 60, 64, 500] {
            queue.insert(index);
            reference.insert(index);
        }
        while let Some(next) = reference.pop_first() {
            assert_eq!(queue.first(), Some(next));
            assert!(queue.contains(next));
            queue.remove(next);
        }
        assert_eq!(queue.first(), None);
        let storage = queue.words.as_ptr();
        let capacity = queue.words.capacity();
        queue.reset(256).unwrap();
        queue.insert(255);
        queue.reset(513).unwrap();
        assert_eq!(queue.words.as_ptr(), storage);
        assert_eq!(queue.words.capacity(), capacity);
        assert_eq!(queue.first(), None);
    }

    #[test]
    fn successive_brushes_reuse_storage_and_finish_with_empty_worklists() {
        let initial = TerrainTile::parse(
            Terrain::Water,
            u8::try_from(raw::RMG_WATER_BASE_FRAME).unwrap(),
            Reflection::default(),
        )
        .unwrap();
        let side = 36;
        let mut workspace = TerrainWorkspace {
            tiles: vec![initial; side * side],
            ..TerrainWorkspace::default()
        };
        workspace.repair.reset(side * side).unwrap();
        workspace.other.reset(side * side).unwrap();
        let tiles = workspace.tiles.as_ptr();
        let repair = workspace.repair.words.as_ptr();
        let other = workspace.other.words.as_ptr();
        let mut rng = RetailRng::new(1);
        for terrain in [
            Terrain::Rock,
            Terrain::Subterranean,
            Terrain::Water,
            Terrain::Dirt,
        ] {
            workspace
                .brush(0, side, terrain, &mut rng)
                .paint_all()
                .unwrap();
            assert!(workspace.tiles.iter().all(|tile| tile.terrain() == terrain));
            assert_eq!(workspace.repair.first(), None);
            assert_eq!(workspace.other.first(), None);
            assert_eq!(workspace.tiles.as_ptr(), tiles);
            assert_eq!(workspace.repair.words.as_ptr(), repair);
            assert_eq!(workspace.other.words.as_ptr(), other);
        }
    }
}
