//! Weighted obstacle filling with reusable candidates and lazy overlap priorities.

use super::{
    offset_position, ObjectArena, ObjectId, PathReservation, PlacementError, PlacementMap,
};
use crate::{
    domain::{Level, WorldPosition},
    geometry::Point,
    identity::OwnerId,
    object::DECORATION_KINDS,
    placement_rules::{NeighbourScore, PlacementRules},
    prototype::{MaskCell, OverlapPriorities, PrototypeCatalog, PrototypeId},
    raw,
    rng::RetailRng,
};
use std::num::NonZeroU32;

#[derive(Clone, Copy, Default)]
enum OverlapOrder {
    #[default]
    None,
    Covers,
    Behind,
    Conflict,
}
impl OverlapOrder {
    fn touch(&mut self, covers: bool) {
        *self = match (*self, covers) {
            (Self::None | Self::Covers, true) => Self::Covers,
            (Self::None | Self::Behind, false) => Self::Behind,
            _ => Self::Conflict,
        };
    }
}
struct TouchedObject {
    id: ObjectId,
    order: OverlapOrder,
    adjacent: bool,
    blocked: bool,
}
struct Candidate {
    prototype: PrototypeId,
    position: WorldPosition,
    weight: i32,
}

/// Scratch storage retained across candidates, obstacle clusters, and maps.
/// Priority caches are valid only for the immutable catalog that created them.
#[derive(Default)]
pub struct ObstacleWorkspace {
    catalog: Option<OwnerId>,
    priorities: Vec<Option<OverlapPriorities>>,
    affected: Vec<TouchedObject>,
    candidates: Vec<Candidate>,
    pending: Vec<WorldPosition>,
}
impl ObstacleWorkspace {
    fn prepare(&mut self, catalog: &PrototypeCatalog<'_>) -> Result<(), PlacementError> {
        if self.catalog != Some(catalog.owner()) {
            self.priorities.clear();
            self.priorities.try_reserve(catalog.entries().len())?;
            self.priorities
                .resize_with(catalog.entries().len(), || None);
            self.catalog = Some(catalog.owner());
        }
        Ok(())
    }
    fn priority(
        &mut self,
        catalog: &PrototypeCatalog<'_>,
        id: PrototypeId,
        cell: MaskCell,
    ) -> Result<u8, PlacementError> {
        let entry = catalog
            .get(id)
            .ok_or(PlacementError::UnknownPrototype(id))?;
        let slot = &mut self.priorities[id.index()];
        if slot.is_none() {
            *slot = Some(OverlapPriorities::build(entry)?);
        }
        slot.as_ref()
            .unwrap()
            .get(cell)
            .ok_or(PlacementError::UnwrittenOverlap(id))
    }
}

impl PlacementMap<'_, '_, '_> {
    /// Score one obstacle anchor against terrain, reservations and existing objects.
    /// Both behavior modes preserve the native overwritten blocked-cell marks.
    ///
    /// # Errors
    /// Reports foreign input identities, unwritten overlap priorities, arithmetic
    /// overflow, allocation failure, or invalid prototype geometry.
    #[expect(
        clippy::missing_panics_doc,
        clippy::too_many_lines,
        clippy::needless_range_loop,
        reason = "admitted footprint bounds make indexes safe; preserve native row-first traversal over column-major marks and first-touch scoring order"
    )]
    pub fn score_obstacle(
        &self,
        scratch: &mut ObstacleWorkspace,
        catalog: &PrototypeCatalog<'_>,
        rules: &PlacementRules,
        objects: &ObjectArena,
        prototype: PrototypeId,
        position: WorldPosition,
    ) -> Result<i32, PlacementError> {
        self.registration.require_catalog(catalog)?;
        if !self.memberships.accepts_arena(objects) {
            return Err(PlacementError::ArenaContext);
        }
        scratch.prepare(catalog)?;
        scratch.affected.clear();
        let entry = catalog
            .get(prototype)
            .ok_or(PlacementError::UnknownPrototype(prototype))?;
        let rule_id = entry.rule().ok_or(PlacementError::RuleContext)?;
        let rule = rules.get(rule_id).ok_or(PlacementError::RuleContext)?;
        // Scoring reads image dimensions; it does not walk the outline.
        // Retail includes decorations with an unoccupied bottom row, even
        // though hotfix catalog admission excludes them.
        let footprint = entry.image_mask().size()?;
        let side = i32::try_from(self.view().side).map_err(|_| PlacementError::Arithmetic)?;
        let mut terrain_seen = [false; raw::RMG_TERRAIN_COUNT as usize];
        // Footprint plus a one-cell border; marks[c][r] is P + (1-c, 1-r).
        // c grows west, r north. North is up; # is a drawn 3x2 footprint:
        //   c: 4 3 2 1 0
        //      o o o o o   r=3
        //      o # # # o   r=2
        //      o # # P o   r=1
        //      o o o o o   r=0
        let mut marks =
            [[0_u32; raw::OBJECT_MASK_HEIGHT as usize + 2]; raw::OBJECT_MASK_WIDTH as usize + 2];
        for row in 0..footprint.height() {
            let y = position
                .point
                .y
                .checked_sub(i32::from(row))
                .ok_or(PlacementError::CoordinateOverflow)?;
            if !(0..side).contains(&y) {
                continue;
            }
            for column in 0..footprint.width() {
                let x = position
                    .point
                    .x
                    .checked_sub(i32::from(column))
                    .ok_or(PlacementError::CoordinateOverflow)?;
                if !(0..side).contains(&x) {
                    continue;
                }
                let cell = MaskCell::parse(column, row).unwrap();
                if !entry.image_mask().draws(cell) {
                    continue;
                }
                let mark = &mut marks[usize::from(column) + 1][usize::from(row) + 1];
                *mark |= raw::RMG_PLACEMENT_OVERLAP;
                if !entry.prototype().is_passable(cell) {
                    let index = self.view().index(WorldPosition {
                        point: Point::new(x, y),
                        level: position.level,
                    })?;
                    let terrain = self.terrain.tiles()[index].terrain();
                    if !entry.prototype().allows_terrain(terrain)
                        || self.cells[index].reservation == PathReservation::Open
                    {
                        return Ok(raw::RMG_PLACEMENT_INVALID);
                    }
                    // Native assignment overwrites OVERLAP and BLOCKED. Consequently
                    // blocked-neighbour scores are never reached in either mode.
                    *mark = raw::RMG_PLACEMENT_ADJACENT;
                    terrain_seen[terrain.index()] = true;
                    let first_row = position.point.y - (y + 1).min(side) + 1;
                    let last_row = position.point.y - (y - 2).max(0) + 1;
                    let first_column = position.point.x - (x + 1).min(side) + 1;
                    let last_column = position.point.x - (x - 2).max(0) + 1;
                    for c in first_column..last_column {
                        for r in first_row..last_row {
                            marks[usize::try_from(c).unwrap()][usize::try_from(r).unwrap()] |=
                                raw::RMG_PLACEMENT_ADJACENT;
                        }
                    }
                }
            }
        }
        let mut score = 0_i32;
        let mut positive = false;
        for (index, seen) in terrain_seen.into_iter().enumerate() {
            if seen {
                let terrain = crate::domain::Terrain::parse(i32::try_from(index).unwrap()).unwrap();
                let value = rule.terrain_score(terrain);
                score = score.checked_add(value).ok_or(PlacementError::Arithmetic)?;
                positive |= value > 0;
            }
        }
        if score < raw::RMG_PLACEMENT_MINIMUM_TERRAIN_SCORE {
            return Ok(score);
        }
        if !positive {
            return Ok(raw::RMG_PLACEMENT_NO_TERRAIN_PREFERENCE);
        }
        // Build even if this candidate never overlaps an existing object.
        if scratch.priorities[prototype.index()].is_none() {
            scratch.priorities[prototype.index()] = Some(OverlapPriorities::build(entry)?);
        }
        for row in 0..usize::from(footprint.height()) + 2 {
            for column in 0..usize::from(footprint.width()) + 2 {
                let mark = marks[column][row];
                if mark == 0 {
                    continue;
                }
                let at = offset_position(
                    position,
                    Point::new(
                        1 - i32::try_from(column).unwrap(),
                        1 - i32::try_from(row).unwrap(),
                    ),
                )?;
                if !(0..side).contains(&at.point.x) || !(0..side).contains(&at.point.y) {
                    continue;
                }
                let index = self.view().index(at)?;
                if self.view().passable(index) {
                    continue;
                }
                let priority = if mark & raw::RMG_PLACEMENT_OVERLAP != 0 {
                    Some(
                        scratch.priority(
                            catalog,
                            prototype,
                            MaskCell::parse(
                                u8::try_from(column - 1).unwrap(),
                                u8::try_from(row - 1).unwrap(),
                            )
                            .unwrap(),
                        )?,
                    )
                } else {
                    None
                };
                for id in self.memberships.iter(self.cells[index].objects) {
                    let object = objects
                        .resolve(id)
                        .ok_or(PlacementError::UnknownObject(id))?;
                    let covers = if let Some(priority) = priority {
                        let object = object
                            .positioned()
                            .ok_or(PlacementError::UnpositionedObject(id))?;
                        let anchor = object.position();
                        let x = anchor
                            .point
                            .x
                            .checked_sub(at.point.x)
                            .ok_or(PlacementError::CoordinateOverflow)?;
                        let y = anchor
                            .point
                            .y
                            .checked_sub(at.point.y)
                            .ok_or(PlacementError::CoordinateOverflow)?;
                        let cell = u8::try_from(x)
                            .ok()
                            .zip(u8::try_from(y).ok())
                            .and_then(|(x, y)| MaskCell::parse(x, y))
                            .ok_or(PlacementError::UnwrittenOverlap(object.prototype()))?;
                        Some(scratch.priority(catalog, object.prototype(), cell)? <= priority)
                    } else {
                        None
                    };
                    let touched = if let Some(index) =
                        scratch.affected.iter().position(|item| item.id == id)
                    {
                        &mut scratch.affected[index]
                    } else {
                        scratch.affected.try_reserve(1)?;
                        scratch.affected.push(TouchedObject {
                            id,
                            order: OverlapOrder::None,
                            adjacent: false,
                            blocked: false,
                        });
                        scratch.affected.last_mut().unwrap()
                    };
                    if let Some(covers) = covers {
                        touched.order.touch(covers);
                    }
                    touched.adjacent |= mark & raw::RMG_PLACEMENT_ADJACENT != 0;
                    touched.blocked |= mark & raw::RMG_PLACEMENT_BLOCKED != 0;
                    if matches!(touched.order, OverlapOrder::Conflict) {
                        break;
                    }
                }
            }
        }
        for touched in &scratch.affected {
            let geometry = objects
                .get(touched.id)
                .ok_or(PlacementError::UnknownObject(touched.id))?;
            let other = catalog
                .get(geometry.prototype())
                .ok_or(PlacementError::UnknownPrototype(geometry.prototype()))?;
            if touched.blocked || touched.adjacent {
                if let Some(other_rule) = other.rule() {
                    let kind = if touched.blocked {
                        NeighbourScore::Blocked
                    } else {
                        NeighbourScore::Adjacent
                    };
                    let value = rules
                        .neighbour_score(rule_id, other_rule, kind)
                        .ok_or(PlacementError::RuleContext)?;
                    score = score.checked_add(value).ok_or(PlacementError::Arithmetic)?;
                } else if touched.blocked {
                    score = raw::RMG_PLACEMENT_INVALID;
                }
            }
            if matches!(touched.order, OverlapOrder::Conflict) {
                score = raw::RMG_PLACEMENT_INVALID;
            }
        }
        Ok(score)
    }

    /// Fill obstacle reservations in plane/Y/X order, then open remaining floor.
    /// Candidate vectors, pending positions and overlap metadata retain capacity.
    ///
    /// # Errors
    /// Reports placement-rule, object, geometry, allocation or arithmetic faults.
    #[expect(
        clippy::missing_panics_doc,
        reason = "parsed dimensions and tile indexes fit native signed coordinates"
    )]
    pub fn decorate_obstacles(
        &mut self,
        scratch: &mut ObstacleWorkspace,
        catalog: &PrototypeCatalog<'_>,
        rules: &PlacementRules,
        objects: &mut ObjectArena,
        rng: &mut RetailRng,
    ) -> Result<(), PlacementError> {
        if !self
            .cells
            .iter()
            .any(|cell| cell.reservation == PathReservation::Obstacle)
        {
            return Ok(());
        }
        self.prepare_object_context(objects, catalog)?;
        scratch.prepare(catalog)?;
        let side = self.view().side;
        let plane = side * side;
        for index in 0..self.cells.len() {
            if self.cells[index].reservation != PathReservation::Obstacle
                || !self.view().passable(index)
            {
                continue;
            }
            let position = WorldPosition {
                point: Point::new(
                    i32::try_from(index % side).unwrap(),
                    i32::try_from(index % plane / side).unwrap(),
                ),
                level: if index < plane {
                    Level::Surface
                } else {
                    Level::Underground
                },
            };
            self.fill_obstacles(scratch, catalog, rules, objects, position, rng)?;
        }
        for index in 0..self.cells.len() {
            if self.cells[index].reservation != PathReservation::Open && self.view().passable(index)
            {
                self.cells[index].open_path();
            }
        }
        Ok(())
    }

    fn fill_obstacles(
        &mut self,
        scratch: &mut ObstacleWorkspace,
        catalog: &PrototypeCatalog<'_>,
        rules: &PlacementRules,
        objects: &mut ObjectArena,
        start: WorldPosition,
        rng: &mut RetailRng,
    ) -> Result<(), PlacementError> {
        scratch.pending.clear();
        scratch.pending.try_reserve(1)?;
        scratch.pending.push(start);
        while let Some(position) = scratch.pending.pop() {
            let index = self.view().index(position)?;
            if !self.view().passable(index) {
                continue;
            }
            let terrain = self.terrain.tiles()[index].terrain();
            scratch.candidates.clear();
            let mut total = 0_i32;
            for kind in DECORATION_KINDS {
                for (ordinal, entry) in catalog.family(kind).iter().enumerate() {
                    let Some(rule) = entry.rule() else {
                        continue;
                    };
                    if rules
                        .get(rule)
                        .ok_or(PlacementError::RuleContext)?
                        .terrain_score(terrain)
                        <= raw::RMG_PLACEMENT_INVALID
                    {
                        continue;
                    }
                    let prototype = catalog.at(kind, ordinal).unwrap();
                    let footprint = entry.image_mask().size()?;
                    // Put S under each blocked footprint cell. For a fully
                    // blocked 3x2 object, candidate anchors are S and each o:
                    //   S o o
                    //   o o o
                    for row in 0..footprint.height() {
                        for column in 0..footprint.width() {
                            if entry
                                .prototype()
                                .is_passable(MaskCell::parse(column, row).unwrap())
                            {
                                continue;
                            }
                            let at = offset_position(
                                position,
                                Point::new(i32::from(column), i32::from(row)),
                            )?;
                            let weight = self
                                .score_obstacle(scratch, catalog, rules, objects, prototype, at)?;
                            if weight > 0 {
                                total = total
                                    .checked_add(weight)
                                    .ok_or(PlacementError::Arithmetic)?;
                                scratch.candidates.try_reserve(1)?;
                                scratch.candidates.push(Candidate {
                                    prototype,
                                    position: at,
                                    weight,
                                });
                            }
                        }
                    }
                }
            }
            let Some(total) =
                NonZeroU32::new(u32::try_from(total).map_err(|_| PlacementError::Arithmetic)?)
            else {
                continue;
            };
            let mut choice = i32::try_from(rng.below(total)).unwrap();
            let selected = scratch
                .candidates
                .iter()
                .find(|candidate| {
                    choice -= candidate.weight;
                    choice < 0
                })
                .unwrap();
            let at = selected.position;
            let footprint = catalog
                .get(selected.prototype)
                .unwrap()
                .image_mask()
                .size()?;
            let object = objects.create(catalog, selected.prototype)?;
            // fillObstaclesFrom dispatches virtual addObject to the generator.
            self.register_object(objects, catalog, object, at)?;
            let side = i32::try_from(self.view().side).unwrap();
            for y in (at.point.y - i32::from(footprint.height())).max(0)..(at.point.y + 2).min(side)
            {
                for x in
                    (at.point.x - i32::from(footprint.width())).max(0)..(at.point.x + 2).min(side)
                {
                    let nearby = WorldPosition {
                        point: Point::new(x, y),
                        level: at.level,
                    };
                    let index = self.view().index(nearby)?;
                    if self.cells[index].reservation == PathReservation::Obstacle
                        && self.view().passable(index)
                    {
                        self.cells[index].clear_obstacle();
                        scratch.pending.try_reserve(1)?;
                        scratch.pending.push(nearby);
                    }
                }
            }
        }
        Ok(())
    }
}
