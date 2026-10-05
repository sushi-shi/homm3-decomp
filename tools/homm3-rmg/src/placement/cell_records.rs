//! `HotA`'s per-cell placement records.
//!
//! `HotA.dll` keeps a 13-byte record per map cell beside the generator's
//! cells. Zone construction writes the edge and reachability bytes (see
//! [`crate::terrain::PaintedTerrain::regions`]); object registration and
//! removal maintain the proximity bytes owned here. Later placement queries
//! read them to keep entrances, guards and airship yards apart.
//!
//! Records use the native flat index `(level * side + y) * side + x`. Some
//! native lookups deliberately leave `x` unclamped, so `x - 1` at column 0
//! reads the previous row's last cell; those quirks are kept. Lookups outside
//! the whole table read an empty record and discard writes, as in the
//! recovered reference (native code touches adjacent heap that no generator
//! stage reads).

use super::PlacementError;
use crate::{
    domain::{Level, WorldPosition},
    geometry::Point,
    object::ObjectKind,
    prototype::PreparedPrototype,
};
use std::collections::TryReserveError;

/// One byte of a native record.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
pub enum CellFlag {
    /// Zone-edge cell, from the terrain post-pass.
    ZoneEdge = 0,
    /// Within one cell of an object trigger.
    NearTrigger = 1,
    /// Within one cell of a monster trigger.
    NearMonster = 2,
    /// Within two cells of a monster trigger.
    MonsterArea = 3,
    /// Within the 3x3 band below an object entrance.
    NearEntrance = 4,
    /// The cell just below an object entrance.
    Entrance = 5,
    /// Near a generated road.
    NearRoad = 6,
    /// Within one cell of a strong monster's trigger.
    NearStrongMonster = 7,
    /// A strong monster's trigger cell.
    StrongMonster = 8,
    /// Inside a treasure group's area.
    TreasureArea = 9,
    /// Reachable from its zone center, from the terrain post-pass.
    ZoneReachable = 10,
    /// Within the 5x3 band below an airship yard.
    NearAirshipYard = 11,
    /// One of the three approach cells below an airship yard.
    AirshipYardEntrance = 12,
}

/// The thirteen record bytes as bits; each native byte holds zero or one.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct CellFlags(u16);
impl CellFlags {
    /// Whether this record byte is set.
    #[must_use]
    pub const fn contains(self, flag: CellFlag) -> bool {
        self.0 & (1 << flag as u8) != 0
    }
    fn set(&mut self, flag: CellFlag, value: bool) {
        if value {
            self.0 |= 1 << flag as u8;
        } else {
            self.0 &= !(1 << flag as u8);
        }
    }
}

/// Inputs of the registration stamp (RVA `0x1cbaf0`) that come from outside
/// the object's prototype.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct StampContext {
    /// The generator is inside `placeZoneTreasures` (`HotA.dll` VA
    /// `0x146873bc`). Outside it, every monster is strong and other objects
    /// also mark the cells below their entrances.
    pub zone_treasures: bool,
    /// For a monster during treasure placement: its creature AI value times
    /// its count exceeds the maximum guard value of the zone owning the
    /// object's anchor cell (zero when unowned).
    pub exceeds_zone_guard: bool,
}

/// Records for every cell of the map, reused across maps.
#[derive(Clone, Debug, Default)]
pub struct CellRecords {
    side: i32,
    cells: Vec<CellFlags>,
}

/// Inclusive-exclusive square around a cell, clamped to the map.
#[derive(Clone, Copy)]
struct Range {
    x: (i32, i32),
    y: (i32, i32),
}
impl Range {
    fn around(side: i32, x: i32, y: i32, radius: i32) -> Self {
        Self {
            x: ((x - radius).max(0), (x + radius + 1).min(side)),
            y: ((y - radius).max(0), (y + radius + 1).min(side)),
        }
    }
    fn cells(self) -> impl Iterator<Item = (i32, i32)> {
        let Self { x, y } = self;
        (x.0..x.1).flat_map(move |cx| (y.0..y.1).map(move |cy| (cx, cy)))
    }
}

impl CellRecords {
    /// Clear the table for a new map, retaining storage.
    ///
    /// # Errors
    /// Reports failed growth.
    pub fn reset(&mut self, side: usize, levels: usize) -> Result<(), TryReserveError> {
        let count = side.saturating_mul(side).saturating_mul(levels);
        self.cells.clear();
        self.cells.try_reserve(count)?;
        self.cells.resize(count, CellFlags::default());
        self.side = i32::try_from(side).unwrap_or(i32::MAX);
        Ok(())
    }

    /// Record at an in-map position.
    #[must_use]
    pub fn get(&self, position: WorldPosition) -> Option<CellFlags> {
        let Point { x, y } = position.point;
        if !(0..self.side).contains(&x) || !(0..self.side).contains(&y) {
            return None;
        }
        self.flat(x, y, position.level.index())
            .map(|index| self.cells[index])
    }

    /// Set a byte written by another stage, such as the terrain post-pass.
    ///
    /// # Errors
    /// Reports a position outside the map.
    pub fn set(
        &mut self,
        position: WorldPosition,
        flag: CellFlag,
        value: bool,
    ) -> Result<(), PlacementError> {
        let Point { x, y } = position.point;
        let index = ((0..self.side).contains(&x) && (0..self.side).contains(&y))
            .then(|| self.flat(x, y, position.level.index()))
            .flatten()
            .ok_or(PlacementError::OutsideMap(position))?;
        self.cells[index].set(flag, value);
        Ok(())
    }

    fn flat(&self, x: i32, y: i32, level: usize) -> Option<usize> {
        let side = i64::from(self.side);
        let index = (i64::try_from(level).ok()? * side + i64::from(y)) * side + i64::from(x);
        usize::try_from(index)
            .ok()
            .filter(|&index| index < self.cells.len())
    }
    fn read(&self, x: i32, y: i32, level: usize, flag: CellFlag) -> bool {
        self.flat(x, y, level)
            .is_some_and(|index| self.cells[index].contains(flag))
    }
    fn write(&mut self, x: i32, y: i32, level: usize, flag: CellFlag, value: bool) {
        if let Some(index) = self.flat(x, y, level) {
            self.cells[index].set(flag, value);
        }
    }
    fn fill(&mut self, range: Range, level: usize, flag: CellFlag) {
        for (x, y) in range.cells() {
            self.write(x, y, level, flag, true);
        }
    }
    /// Map position named by a native flat index, if inside the table.
    fn position(&self, x: i32, y: i32, level: usize) -> Option<WorldPosition> {
        let index = self.flat(x, y, level)?;
        let side = usize::try_from(self.side).ok()?;
        let plane = side * side;
        Some(WorldPosition {
            point: Point::new(
                i32::try_from(index % side).ok()?,
                i32::try_from(index % plane / side).ok()?,
            ),
            level: if index < plane {
                Level::Surface
            } else {
                Level::Underground
            },
        })
    }

    /// Stamp an object's proximity bytes after registration (RVA `0x1cbaf0`).
    /// Only trigger cells inside the map stamp; boxes are clipped to the map.
    ///
    /// # Errors
    /// Reports unusable prototype dimensions.
    pub fn stamp(
        &mut self,
        prototype: &PreparedPrototype<'_>,
        anchor: WorldPosition,
        context: StampContext,
    ) -> Result<(), PlacementError> {
        let object = prototype.prototype();
        if object.entrance().is_none() {
            return Ok(());
        }
        let kind = object.kind();
        let monster = kind == ObjectKind::MONSTER;
        let strong = monster && (!context.zone_treasures || context.exceeds_zone_guard);
        let marks_entrance = !context.zone_treasures
            && kind != ObjectKind::RESOURCE
            && !monster
            && kind != ObjectKind::BORDER_GUARD;
        let yard = kind == ObjectKind::SHIPYARD && object.subtype() == 1;
        let level = anchor.level.index();
        let side = self.side;
        for cell in prototype.image_mask().size()?.cells() {
            let x = anchor.point.x - i32::from(cell.x());
            let y = anchor.point.y - i32::from(cell.y());
            if !(0..side).contains(&x) || !(0..side).contains(&y) || !object.is_trigger(cell) {
                continue;
            }
            let near = Range::around(side, x, y, 1);
            self.fill(near, level, CellFlag::NearTrigger);
            if monster {
                self.fill(near, level, CellFlag::NearMonster);
                if strong {
                    self.fill(near, level, CellFlag::NearStrongMonster);
                }
                self.fill(Range::around(side, x, y, 2), level, CellFlag::MonsterArea);
            }
            if marks_entrance && y < side - 1 {
                self.write(x, y + 1, level, CellFlag::Entrance, true);
                let below = Range {
                    x: near.x,
                    y: (y, (y + 3).min(side)),
                };
                self.fill(below, level, CellFlag::NearEntrance);
                if yard {
                    // Unclamped x - 1 / x + 1: flat neighbours, possibly on
                    // the adjacent row.
                    for dx in [0, 1, -1] {
                        self.write(x + dx, y + 1, level, CellFlag::AirshipYardEntrance, true);
                    }
                    let band = Range {
                        x: ((x - 2).max(0), (x + 3).min(side)),
                        y: (y, (y + 3).min(side)),
                    };
                    self.fill(band, level, CellFlag::NearAirshipYard);
                }
            }
            if strong {
                self.write(x, y, level, CellFlag::StrongMonster, true);
            }
        }
        Ok(())
    }

    /// Re-derive proximity bytes around a removed object (RVA `0x1cc160`).
    /// `entrance` reports the actual kind of the first object at a map cell
    /// whose entrance flag is set, after the removal.
    ///
    /// The bytes cleared before re-derivation are addressed by the
    /// prototype-local column/row rather than by the map cell, as natively.
    ///
    /// # Errors
    /// Reports unusable prototype dimensions or a failing `entrance` query.
    pub fn unstamp(
        &mut self,
        prototype: &PreparedPrototype<'_>,
        anchor: WorldPosition,
        mut entrance: impl FnMut(WorldPosition) -> Result<Option<ObjectKind>, PlacementError>,
    ) -> Result<(), PlacementError> {
        let object = prototype.prototype();
        if object.entrance().is_none() {
            return Ok(());
        }
        let monster = object.kind() == ObjectKind::MONSTER;
        let level = anchor.level.index();
        let side = self.side;
        for cell in prototype.image_mask().size()?.cells() {
            let x = anchor.point.x - i32::from(cell.x());
            let y = anchor.point.y - i32::from(cell.y());
            if !(0..side).contains(&x) || !(0..side).contains(&y) || !object.is_trigger(cell) {
                continue;
            }
            let (column, row) = (i32::from(cell.x()), i32::from(cell.y()));
            let mut strong = false;
            if monster {
                strong = self.read(column, row, level, CellFlag::StrongMonster);
                self.write(column, row, level, CellFlag::StrongMonster, false);
            }
            let mut below_entrance = false;
            let mut yard = false;
            if y < side - 1 {
                below_entrance = self.read(column, row + 1, level, CellFlag::Entrance);
                self.write(column, row + 1, level, CellFlag::Entrance, false);
                for dx in [0, 1, -1] {
                    yard |= self.read(column + dx, row + 1, level, CellFlag::AirshipYardEntrance);
                    self.write(
                        column + dx,
                        row + 1,
                        level,
                        CellFlag::AirshipYardEntrance,
                        false,
                    );
                }
            }
            let near = Range::around(side, x, y, 1);
            for (cx, cy) in near.cells() {
                let value = self.entrance_near(cx, cy, level, 1, false, &mut entrance)?;
                self.write(cx, cy, level, CellFlag::NearTrigger, value);
                if monster {
                    let value = self.entrance_near(cx, cy, level, 1, true, &mut entrance)?;
                    self.write(cx, cy, level, CellFlag::NearMonster, value);
                    if strong {
                        let value = self.strong_monster_near(cx, cy, level, &mut entrance)?;
                        self.write(cx, cy, level, CellFlag::NearStrongMonster, value);
                    }
                }
            }
            if monster {
                for (cx, cy) in Range::around(side, x, y, 2).cells() {
                    let value = self.entrance_near(cx, cy, level, 2, true, &mut entrance)?;
                    self.write(cx, cy, level, CellFlag::MonsterArea, value);
                }
            }
            if below_entrance && y < side - 1 {
                let band = Range {
                    x: near.x,
                    y: (y, (y + 3).min(side)),
                };
                for (cx, cy) in band.cells() {
                    let value = self.entrance_above_near(cx, cy, level, &mut entrance)?;
                    self.write(cx, cy, level, CellFlag::NearEntrance, value);
                }
            }
            if yard && y < side - 1 {
                let band = Range {
                    x: ((x - 2).max(0), (x + 3).min(side)),
                    y: (y, (y + 3).min(side)),
                };
                for (cx, cy) in band.cells() {
                    let value = self.yard_entrance_near(cx, cy, level, &mut entrance)?;
                    self.write(cx, cy, level, CellFlag::NearAirshipYard, value);
                }
            }
        }
        Ok(())
    }

    fn entrance_at(
        &self,
        x: i32,
        y: i32,
        level: usize,
        entrance: &mut impl FnMut(WorldPosition) -> Result<Option<ObjectKind>, PlacementError>,
    ) -> Result<Option<ObjectKind>, PlacementError> {
        self.position(x, y, level).map_or(Ok(None), entrance)
    }
    // RVA 0x1ccc60 (any entrance, 3x3) and 0x1cc9e0/0x1cc920 (monster
    // entrance, 3x3/5x5).
    fn entrance_near(
        &self,
        x: i32,
        y: i32,
        level: usize,
        radius: i32,
        monsters: bool,
        entrance: &mut impl FnMut(WorldPosition) -> Result<Option<ObjectKind>, PlacementError>,
    ) -> Result<bool, PlacementError> {
        for (cx, cy) in Range::around(self.side, x, y, radius).cells() {
            if let Some(kind) = self.entrance_at(cx, cy, level, entrance)? {
                if !monsters || kind == ObjectKind::MONSTER {
                    return Ok(true);
                }
            }
        }
        Ok(false)
    }
    // RVA 0x1ccb70: a monster entrance in the 3x3 whose own byte 8 is set.
    fn strong_monster_near(
        &self,
        x: i32,
        y: i32,
        level: usize,
        entrance: &mut impl FnMut(WorldPosition) -> Result<Option<ObjectKind>, PlacementError>,
    ) -> Result<bool, PlacementError> {
        for (cx, cy) in Range::around(self.side, x, y, 1).cells() {
            if self.entrance_at(cx, cy, level, entrance)? == Some(ObjectKind::MONSTER)
                && self.read(cx, cy, level, CellFlag::StrongMonster)
            {
                return Ok(true);
            }
        }
        Ok(false)
    }
    // RVA 0x1ccaa0: a 3x3 cell with byte 5 and an entrance just above it.
    fn entrance_above_near(
        &self,
        x: i32,
        y: i32,
        level: usize,
        entrance: &mut impl FnMut(WorldPosition) -> Result<Option<ObjectKind>, PlacementError>,
    ) -> Result<bool, PlacementError> {
        for (cx, cy) in Range::around(self.side, x, y, 1).cells() {
            if cy > 0
                && self.entrance_at(cx, cy - 1, level, entrance)?.is_some()
                && self.read(cx, cy, level, CellFlag::Entrance)
            {
                return Ok(true);
            }
        }
        Ok(false)
    }
    // RVA 0x1cc7c0: a 3x3 cell with byte 12 and an entrance above, above
    // right or above left. All three read the cell's own byte 12; the
    // diagonal lookups are flat and unclamped.
    fn yard_entrance_near(
        &self,
        x: i32,
        y: i32,
        level: usize,
        entrance: &mut impl FnMut(WorldPosition) -> Result<Option<ObjectKind>, PlacementError>,
    ) -> Result<bool, PlacementError> {
        for (cx, cy) in Range::around(self.side, x, y, 1).cells() {
            if cy <= 0 {
                continue;
            }
            let yard = self.read(cx, cy, level, CellFlag::AirshipYardEntrance);
            for dx in [0, 1, -1] {
                if self
                    .entrance_at(cx + dx, cy - 1, level, entrance)?
                    .is_some()
                    && yard
                {
                    return Ok(true);
                }
            }
        }
        Ok(false)
    }
}

#[cfg(test)]
mod tests;
