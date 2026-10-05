//! `HotA`'s per-cell draw lists.
//!
//! Besides the generator's cell memberships, `HotA.dll` lists every object
//! whose *draw* mask covers a cell (registration RVA `0x1c6ce0`, removal RVA
//! `0x1c7890`). Its placement checks use these lists to keep sprite overlap
//! order consistent (RVA `0x1c79d0`) and to score overlapping objects.
//! Lists share one link arena, so cells own no separate heap storage.

use super::{
    objects::{Chain, Memberships},
    ObjectArena, ObjectId, ObstacleWorkspace, PlacementError,
};
use crate::{
    domain::WorldPosition,
    geometry::Point,
    prototype::{MaskCell, PreparedPrototype, PrototypeCatalog, PrototypeId},
};
use std::collections::TryReserveError;

/// Ordered draw lists for every map cell, reused across maps.
#[derive(Debug, Default)]
pub struct DrawLists {
    side: i32,
    chains: Vec<Chain>,
    links: Memberships,
}

/// An object whose overlap order relative to a candidate has been compared.
#[derive(Clone, Copy)]
struct Touched {
    object: ObjectId,
    covers: bool,
    behind: bool,
}

impl DrawLists {
    /// Empty every list for a new map, retaining storage.
    ///
    /// # Errors
    /// Reports failed growth.
    pub fn reset(&mut self, side: usize, levels: usize) -> Result<(), TryReserveError> {
        let count = side.saturating_mul(side).saturating_mul(levels);
        self.chains.clear();
        self.chains.try_reserve(count)?;
        self.chains.resize(count, Chain::default());
        self.links.reset();
        self.side = i32::try_from(side).unwrap_or(i32::MAX);
        Ok(())
    }

    fn index(&self, position: WorldPosition) -> Result<usize, PlacementError> {
        let Point { x, y } = position.point;
        let side = usize::try_from(self.side).map_err(|_| PlacementError::OutsideMap(position))?;
        if !(0..self.side).contains(&x) || !(0..self.side).contains(&y) {
            return Err(PlacementError::OutsideMap(position));
        }
        let index = (position.level.index() * side + usize::try_from(y).unwrap()) * side
            + usize::try_from(x).unwrap();
        if index < self.chains.len() {
            Ok(index)
        } else {
            Err(PlacementError::OutsideMap(position))
        }
    }

    /// Objects drawn over an in-map cell, in registration order.
    ///
    /// # Errors
    /// Reports a position outside the map.
    pub fn objects(
        &self,
        position: WorldPosition,
    ) -> Result<impl Iterator<Item = ObjectId> + '_, PlacementError> {
        let index = self.index(position)?;
        Ok(self.links.iter(self.chains[index]))
    }

    /// Append a registered object to each drawn cell (RVA `0x1c6ce0`).
    ///
    /// # Errors
    /// Reports unusable dimensions, a foreign arena, a plane absent from the
    /// map or failed growth.
    pub fn stamp(
        &mut self,
        prototype: &PreparedPrototype<'_>,
        object: ObjectId,
        anchor: WorldPosition,
    ) -> Result<(), PlacementError> {
        if !self.links.accepts(object) {
            return Err(PlacementError::ArenaContext);
        }
        let mut count = 0;
        for (_, position) in drawn(self.side, prototype, anchor)? {
            self.index(position)?;
            count += 1;
        }
        self.links.reserve(count)?;
        for (_, position) in drawn(self.side, prototype, anchor)? {
            let index = self.index(position)?;
            self.links.append(&mut self.chains[index], object);
        }
        Ok(())
    }

    /// Erase the first occurrence of a removed object from each drawn cell
    /// at its anchor (RVA `0x1c7890`); cells without it are unchanged.
    ///
    /// # Errors
    /// Reports unusable dimensions or a plane absent from the map.
    pub fn unstamp(
        &mut self,
        prototype: &PreparedPrototype<'_>,
        object: ObjectId,
        anchor: WorldPosition,
    ) -> Result<(), PlacementError> {
        for (_, position) in drawn(self.side, prototype, anchor)? {
            let index = self.index(position)?;
            self.links.remove(&mut self.chains[index], object);
        }
        Ok(())
    }

    /// Whether a candidate keeps one overlap order with every object already
    /// drawn under it (RVA `0x1c79d0`). An existing object is behind when its
    /// priority at the shared cell is higher, or equal while only the
    /// candidate is a decoration; otherwise the candidate covers it. An
    /// object both behind and covered is a conflict.
    ///
    /// # Errors
    /// Reports foreign/stale identities, unusable dimensions or unwritten
    /// overlap priorities.
    pub fn consistent_draw_order(
        &self,
        scratch: &mut ObstacleWorkspace,
        catalog: &PrototypeCatalog<'_>,
        objects: &ObjectArena,
        candidate: PrototypeId,
        anchor: WorldPosition,
    ) -> Result<bool, PlacementError> {
        scratch.prepare(catalog)?;
        let entry = catalog
            .get(candidate)
            .ok_or(PlacementError::UnknownPrototype(candidate))?;
        let decoration = entry.prototype().kind().traits().is_decoration();
        let mut touched: Vec<Touched> = Vec::new();
        for (cell, position) in drawn(self.side, entry, anchor)? {
            let priority = scratch.priority(catalog, candidate, cell)?;
            for id in self.links.iter(self.chains[self.index(position)?]) {
                let object = objects.positioned(id)?;
                let other = catalog
                    .get(object.prototype())
                    .ok_or(PlacementError::UnknownPrototype(object.prototype()))?;
                let tie_behind = decoration && !other.prototype().kind().traits().is_decoration();
                let local = u8::try_from(object.position().point.x - position.point.x)
                    .ok()
                    .zip(u8::try_from(object.position().point.y - position.point.y).ok())
                    .and_then(|(x, y)| MaskCell::parse(x, y))
                    .ok_or(PlacementError::UnwrittenOverlap(object.prototype()))?;
                let existing = scratch.priority(catalog, object.prototype(), local)?;
                let behind = existing > priority || (tie_behind && existing == priority);
                let slot = if let Some(slot) = touched.iter().position(|t| t.object == id) {
                    slot
                } else {
                    touched.try_reserve(1)?;
                    touched.push(Touched {
                        object: id,
                        covers: false,
                        behind: false,
                    });
                    touched.len() - 1
                };
                let mark = &mut touched[slot];
                if behind {
                    mark.behind = true;
                } else {
                    mark.covers = true;
                }
                if mark.behind && mark.covers {
                    return Ok(false);
                }
            }
        }
        Ok(true)
    }
}

/// Drawn cells of `prototype` anchored at `anchor` inside the map's columns
/// and rows, in native row-then-column order. The plane is not clipped here:
/// callers report a plane absent from the map.
fn drawn(
    side: i32,
    prototype: &PreparedPrototype<'_>,
    anchor: WorldPosition,
) -> Result<impl Iterator<Item = (MaskCell, WorldPosition)>, PlacementError> {
    let mask = prototype.image_mask();
    Ok(mask.size()?.cells().filter_map(move |cell| {
        let point = Point::new(
            anchor.point.x - i32::from(cell.x()),
            anchor.point.y - i32::from(cell.y()),
        );
        ((0..side).contains(&point.x) && (0..side).contains(&point.y) && mask.draws(cell))
            .then_some((
                cell,
                WorldPosition {
                    point,
                    level: anchor.level,
                },
            ))
    }))
}

#[cfg(test)]
mod tests;
