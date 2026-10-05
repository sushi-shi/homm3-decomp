//! `HotA`'s frame-aware object scoring policies.
//!
//! `HotA.dll` RVA `0x1c6090` replaces Complete's obstacle scorer. Unlike
//! Complete, it reads each blocked cell's terrain *frame*: a transition frame
//! also counts the Dirt or Sand art along its edge. These policies own that
//! terrain gate, the decoration filter (RVA `0x1c7ea0`) and the same-prototype
//! spacing penalty (RVA `0x1c7c90`). The footprint, draw-list and neighbour
//! walks belong to the placement state that owns those cells; until `HotA`
//! placement is admitted, these policies are exercised directly.

use super::PlacementError;
use crate::{
    domain::Terrain, geometry::Point, placement_rules::PlacementRule, raw, rules::Ruleset,
    terrain_rules::TerrainTile,
};

/// Native result for an invalid placement (`0x88ca6bff`).
pub const INVALID_SCORE: i32 = -2_000_000_001;
/// Native "allowed, but forbidden by terrain" score (`0x88ca6c00`). Only a
/// single-draw-cell object can carry it out of the terrain gate.
pub const FORBIDDEN_SCORE: i32 = -2_000_000_000;

/// The terrain table's neutral band: scores in `(-5000, -4000]` mark a
/// terrain allowed under the object without counting it.
const fn counts(score: i32) -> bool {
    score <= raw::RMG_PLACEMENT_INVALID || score > -4000
}

/// Edge art drawn by a terrain frame, in addition to its own terrain.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum FrameEdge {
    /// Base or decorated fill, or a shape outside the transition families.
    Own,
    /// Blending edges, drawn against Dirt.
    Dirt,
    /// Hard edges, drawn against Sand.
    Sand,
    /// Mixed blending and hard edges.
    DirtAndSand,
}
impl FrameEdge {
    /// Shape families shared by RVAs `0x1c7f60` and `0x1c7ea0`. Shape 28
    /// (mixed) is classified by each caller: the two native switches differ.
    const fn of(shape: u32) -> Self {
        match shape {
            raw::SHAPE_N_W_BLEND
            | raw::SHAPE_W_BLEND
            | raw::SHAPE_N_BLEND
            | raw::SHAPE_SE_BLEND
            | raw::SHAPE_N_W_DIAG_BLEND
            | raw::SHAPE_SE_DIAG_BLEND
            | raw::SHAPE_NW_SE_BLEND
            | raw::SHAPE_N_W_SE_BLEND => Self::Dirt,
            raw::SHAPE_N_W_HARD
            | raw::SHAPE_W_HARD
            | raw::SHAPE_N_HARD
            | raw::SHAPE_SE_HARD
            | raw::SHAPE_N_W_DIAG_HARD
            | raw::SHAPE_SE_DIAG_HARD
            | raw::SHAPE_NW_SE_HARD
            | raw::SHAPE_N_W_SE_HARD => Self::Sand,
            raw::SHAPE_NW_BLEND_SE_HARD
            | raw::SHAPE_E_BLEND_SW_HARD
            | raw::SHAPE_S_BLEND_NE_HARD
            | raw::SHAPE_E_BLEND_SE_HARD
            | raw::SHAPE_S_BLEND_SE_HARD
            | raw::SHAPE_E_HARD_SW_BLEND
            | raw::SHAPE_S_HARD_NE_BLEND
            | raw::SHAPE_N_W_BLEND_SE_HARD
            | raw::SHAPE_N_W_HARD_SE_BLEND
            | raw::SHAPE_E_S_BLEND_SE_HARD => Self::DirtAndSand,
            _ => Self::Own,
        }
    }
    const fn dirt(self) -> bool {
        matches!(self, Self::Dirt | Self::DirtAndSand)
    }
    const fn sand(self) -> bool {
        matches!(self, Self::Sand | Self::DirtAndSand)
    }
}

fn score(rule: &PlacementRule, terrain: Terrain) -> Result<i32, PlacementError> {
    rule.terrain_score(terrain)
        .ok_or(PlacementError::TerrainRule(terrain))
}

/// Whether a decoration with this rule may stand on a tile (RVA `0x1c7ea0`).
/// Edge art must also be allowed; a plain frame needs a non-neutral score.
///
/// # Errors
/// Reports a rule table without the tile terrain's score column.
pub fn allows_decoration(rule: &PlacementRule, tile: TerrainTile) -> Result<bool, PlacementError> {
    let land = score(rule, tile.terrain())?;
    // Unlike the seen-terrain switch, shape 28 belongs to the mixed group.
    let edge = match tile.shape() {
        raw::SHAPE_E_S_BLEND_NE_SW_HARD => FrameEdge::DirtAndSand,
        shape => FrameEdge::of(shape),
    };
    if edge == FrameEdge::Own {
        return Ok(land > -4000);
    }
    Ok(land > raw::RMG_PLACEMENT_INVALID
        && (!edge.dirt() || score(rule, Terrain::Dirt)? > raw::RMG_PLACEMENT_INVALID)
        && (!edge.sand() || score(rule, Terrain::Sand)? > raw::RMG_PLACEMENT_INVALID))
}

/// Terrains under an object's blocked draw cells: native byte bit 1 counts a
/// terrain's score, bit 2 records that a blocked cell's own terrain showed it.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct TerrainTally {
    seen: [u8; Ruleset::HotA181.terrain_count()],
}

/// Terrain gate result. `terrain_ok` is the native flag later read by
/// decoration (`HotA.dll` VA `0x14687348`).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct TerrainGate {
    /// `None` rejects the placement; otherwise the running score, possibly
    /// [`FORBIDDEN_SCORE`] for a single-draw-cell object.
    pub score: Option<i32>,
    /// Whether no counted terrain fell into the forbidden range.
    pub terrain_ok: bool,
}

impl TerrainTally {
    const COUNTED: u8 = 1;
    const OCCUPIED: u8 = 2;

    /// Record one blocked draw cell (RVA `0x1c7f60`).
    ///
    /// # Errors
    /// Reports a rule table without a needed terrain score column.
    pub fn mark(&mut self, rule: &PlacementRule, tile: TerrainTile) -> Result<(), PlacementError> {
        let land = tile.terrain();
        let slot = self
            .seen
            .get_mut(land.index())
            .ok_or(PlacementError::TerrainRule(land))?;
        *slot |= Self::OCCUPIED;
        // Native default arm: base frames and shape 28 count their own terrain
        // unconditionally, without consulting the neutral band.
        let edge = FrameEdge::of(tile.shape());
        if edge == FrameEdge::Own {
            *slot |= Self::COUNTED;
            return Ok(());
        }
        if counts(score(rule, land)?) {
            *slot |= Self::COUNTED;
        }
        for (shows, terrain) in [(edge.dirt(), Terrain::Dirt), (edge.sand(), Terrain::Sand)] {
            if shows && counts(score(rule, terrain)?) {
                self.seen[terrain.index()] |= Self::COUNTED;
            }
        }
        Ok(())
    }

    /// Sum counted terrain scores after the footprint walk. A terrain score
    /// at or below -4000, or a total below -1000, rejects multi-cell objects
    /// and forbids single-cell ones. A counted positive score is required,
    /// and some positive terrain must lie under a blocked cell itself.
    ///
    /// # Errors
    /// Reports a missing score column or a sum outside the signed domain.
    pub fn gate(
        &self,
        rule: &PlacementRule,
        ruleset: Ruleset,
        draw_cells: u32,
    ) -> Result<TerrainGate, PlacementError> {
        let mut gate = TerrainGate {
            score: Some(0),
            terrain_ok: true,
        };
        let mut positive = false;
        let mut occupied_positive = false;
        let terrains = (0..ruleset.terrain_count())
            .filter_map(|index| Terrain::parse_for(i32::try_from(index).ok()?, ruleset));
        for terrain in terrains {
            let seen = self.seen[terrain.index()];
            if seen & Self::COUNTED == 0 {
                continue;
            }
            let value = score(rule, terrain)?;
            if value <= -4000 {
                gate.terrain_ok = false;
                if draw_cells != 1 {
                    gate.score = None;
                    return Ok(gate);
                }
                gate.score = Some(FORBIDDEN_SCORE);
            } else if let Some(total) = gate.score.filter(|&total| total > FORBIDDEN_SCORE) {
                gate.score = Some(total.checked_add(value).ok_or(PlacementError::Arithmetic)?);
            }
            if value > 0 {
                positive = true;
                occupied_positive |= seen & Self::OCCUPIED != 0;
            }
        }
        if gate.score.is_some_and(|total| total < -1000) {
            gate.terrain_ok = false;
            if draw_cells != 1 {
                gate.score = None;
                return Ok(gate);
            }
            gate.score = Some(FORBIDDEN_SCORE);
        }
        if !positive || !occupied_positive {
            gate.score = None;
        }
        Ok(gate)
    }
}

/// Bounds of an object's blocked draw cells, `[min, max)` on both axes.
/// The empty value keeps the native ±32000 initializers.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct BlockedBounds {
    min: Point,
    max: Point,
}
impl Default for BlockedBounds {
    fn default() -> Self {
        Self {
            min: Point::new(32000, 32000),
            max: Point::new(-32000, -32000),
        }
    }
}
impl BlockedBounds {
    /// Extend the bounds with one blocked cell. Native code tests `x >= max`
    /// before assigning `x + 1`, which is an ordinary half-open union.
    pub fn include(&mut self, cell: Point) {
        self.min.x = self.min.x.min(cell.x);
        self.min.y = self.min.y.min(cell.y);
        if cell.x >= self.max.x {
            self.max.x = cell.x + 1;
        }
        if cell.y >= self.max.y {
            self.max.y = cell.y + 1;
        }
    }

    /// Distance to the nearest cell (within seven of the bounds) holding an
    /// object of the same prototype (RVA `0x1c7c90`): 1 when it touches the
    /// bounds, else the gap along one separating axis; `None` when there is
    /// none. `holds` reports whether an in-map cell holds such an object.
    ///
    /// The native gaps are asymmetric: the west and north gaps count from the
    /// first blocked column/row, the east and south gaps from one past the
    /// last. A diagonal cell takes the *smaller* of its two axis gaps.
    ///
    /// # Errors
    /// Propagates errors from `holds`.
    pub fn same_prototype_distance<E>(
        self,
        side: i32,
        mut holds: impl FnMut(Point) -> Result<bool, E>,
    ) -> Result<Option<i32>, E> {
        let Self { min, max } = self;
        let (x0, y0, x1, y1) = clamp_rectangle(side, min.x - 7, min.y - 7, max.x + 7, max.y + 7);
        let mut best = None;
        for x in x0..x1 {
            for y in y0..y1 {
                if !holds(Point::new(x, y))? {
                    continue;
                }
                let west = x < min.x - 1;
                let east = x > max.x;
                let north = y < min.y - 1;
                let south = y > max.y;
                let distance = match (west, east) {
                    (false, false) if !north && !south => return Ok(Some(1)),
                    (true, _) if north => (min.y - y).min(min.x - x),
                    (true, _) if south => (y - max.y).min(min.x - x),
                    (true, _) => min.x - x,
                    (_, true) if north => (min.y - y).min(x - max.x),
                    (_, true) if south => (y - max.y).min(x - max.x),
                    (_, true) => x - max.x,
                    _ if y < min.y => min.y - y,
                    _ => y - max.y,
                };
                best = Some(best.map_or(distance, |best: i32| best.min(distance)));
            }
        }
        Ok(best)
    }
}

/// Clamp `[x0, x1) x [y0, y1)` to the square map; an empty result collapses
/// to its origin corner (RVA `0x1cac70`).
#[must_use]
pub fn clamp_rectangle(side: i32, x0: i32, y0: i32, x1: i32, y1: i32) -> (i32, i32, i32, i32) {
    let (x0, y0) = (x0.max(0), y0.max(0));
    let (x1, y1) = (x1.min(side), y1.min(side));
    if x1 <= x0 || y1 <= y0 {
        (x0, y0, x0, y0)
    } else {
        (x0, y0, x1, y1)
    }
}

/// Final adjustment after neighbour scoring: strict scoring adds three per
/// blocked cell less nine, then any same-prototype object within seven cells
/// costs `(8 - distance) * 20`. Invalid and forbidden scores pass through.
///
/// # Errors
/// Reports a score outside the signed domain.
pub fn finish(
    score: i32,
    relaxed: bool,
    blocked_cells: u32,
    distance: Option<i32>,
) -> Result<i32, PlacementError> {
    if score <= FORBIDDEN_SCORE {
        return Ok(score);
    }
    let mut score = score;
    if !relaxed {
        let blocked = i32::try_from(blocked_cells).map_err(|_| PlacementError::Arithmetic)?;
        score = blocked
            .checked_mul(3)
            .and_then(|bonus| score.checked_add(bonus))
            .and_then(|total| total.checked_sub(9))
            .ok_or(PlacementError::Arithmetic)?;
    }
    if let Some(distance) = distance.filter(|&distance| distance < 30000) {
        score = score
            .checked_sub(((8 - distance) * 20).max(0))
            .ok_or(PlacementError::Arithmetic)?;
    }
    Ok(score)
}

#[cfg(test)]
mod tests;
